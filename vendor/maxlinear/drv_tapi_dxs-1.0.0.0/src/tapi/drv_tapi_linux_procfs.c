/******************************************************************************

  Copyright 2023-2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_linux.c
   This file contains the implementation of High-Level TAPI Driver,
   Linux proc fs specific part.
*/

#ifdef LINUX
#ifdef __KERNEL__

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_tapi_config.h"

#ifdef TAPI_FEAT_PROCFS

#include "drv_tapi.h"
#include "drv_tapi_api.h"
#include "drv_tapi_debug.h"
#include "drv_tapi_event.h"
#include "drv_tapi_linux_procfs.h"
#include "drv_tapi_linux_procfs_phone_detection.h"

#ifndef UTS_RELEASE
   #include <generated/utsrelease.h>
#endif

#ifdef TAPI_FEAT_IOCTL_CAPABILITY_CHECK
   #include <linux/capability.h>
#endif

/* Get data about DXS devices */
#include "../dxs/drv_dxs_init.h"
#include "../dxs/drv_dxs_alm_priv.h"

#ifdef TAPI_FEAT_DEBUG_BUFFER
   #include "drv_tapi_debug_buffer.h"
#endif

#include <linux/math64.h>

/* ============================= */
/* Local types definitions       */
/* ============================= */

/* single call read proc entry callback function */
typedef IFX_void_t (*proc_single_callback_t)(struct seq_file *);

/* multiple call read proc entry callback function */
typedef int (*proc_callback_t)(struct seq_file *, int);

/* used to get the number of times multiple call read proc entry callback
* function should be called to get the full output (typically equals number
* of devices). The number returned by this function is assigned to nMaxPos
* field of proc_file_entry structure. */
typedef int (*proc_init_callback_t)(void);

/* write function */
typedef ssize_t (*proc_wr_t)(struct file *file, const char __user *buffer,
                             size_t count, loff_t *data);

struct proc_file_entry {
   /* function used for data output */
   proc_callback_t callback;
   /* current output position */
   int nPos;
   /* maximum output position */
   int nMaxPos;
};

struct proc_entry {
   const char *name;
   bool public;
   proc_single_callback_t single_callback;
   proc_callback_t callback;
   proc_init_callback_t init_callback;
   proc_wr_t write_function;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0))
   struct file_operations ops;
#else
   struct proc_ops ops;
#endif
};

/* ============================= */
/* Local functions declarations  */
/* ============================= */

static void *TAPI_seq_start(struct seq_file *s, loff_t *pos);
static void *TAPI_seq_next(struct seq_file *s, void *v, loff_t *pos);
static void TAPI_seq_stop(struct seq_file *s, void *v);
static int TAPI_seq_show(struct seq_file *s, void *v);

static IFX_void_t proc_get_tapi_version(struct seq_file *s);
#ifdef HAVE_CONFIG_H
static IFX_void_t proc_ConfigureGet(struct seq_file *s);
#endif
static IFX_void_t proc_get_tapi_status(struct seq_file *s);
static IFX_int32_t proc_get_dxs_status(struct seq_file *buf);
#ifdef ENABLE_TRACE
   static IFX_void_t proc_debug_level_get(struct seq_file *s);
   static ssize_t proc_debug_level_set(struct file *file, const char *buffer,
                                       size_t count, loff_t *data);
#endif
#ifdef DEBUG
   static IFX_void_t proc_get_ioctl_list(struct seq_file *s);
#endif
#ifdef TAPI_FEAT_DEBUG_BUFFER
   static IFX_void_t proc_print_debug_buffer(struct seq_file *s);
   static IFX_void_t proc_print_debug_buffer_status(struct seq_file *s);
   static IFX_void_t proc_debug_buffer_get_console_printout(struct seq_file *s);
   static ssize_t proc_debug_buffer_set_console_printout(struct file *file,
                                                         const char *buffer,
                                                         size_t count,
                                                         loff_t *data);
#endif
static IFX_void_t proc_get_tapi_registered_drivers(struct seq_file *s);
static IFX_void_t proc_read_bufferpool (struct seq_file *s);
static int proc_read_fifos (struct seq_file *s, int pos);
static int proc_get_dev_count(void);

/* ============================= */
/* External variables            */
/* ============================= */

/* Size of helper ioctl command number to name translation lookup table */
extern const IFX_uint32_t tapi_ioctl_table_size;
/* Lookup table for ioctl command number to name translation */
extern struct IoctlLookupTable const tapi_ioctl_table[];

extern const IFX_char_t DXS_DRV_WHATVERSION[];
extern const IFX_char_t TAPI_DXS_DRV_WHATVERSION[];


/* ============================= */
/* Local variables               */
/* ============================= */

static struct seq_operations TAPI_seq_ops = {
   .start   = TAPI_seq_start,
   .next = TAPI_seq_next,
   .stop = TAPI_seq_stop,
   .show = TAPI_seq_show
};

static struct proc_entry proc_entries[] = {
   {"version", true, proc_get_tapi_version},
#ifdef HAVE_CONFIG_H
   {"configure", false, proc_ConfigureGet},
#endif
   {"status", false, proc_get_tapi_status},
   {"registered_drivers", false, proc_get_tapi_registered_drivers},
   {"bufferpool", false, proc_read_bufferpool},
   {"fifo_status", false, NULL, proc_read_fifos, proc_get_dev_count},
#ifdef ENABLE_TRACE
   {"debug_level", false, proc_debug_level_get, NULL, NULL, proc_debug_level_set},
#endif
#ifdef DEBUG
   {"ioctl_list", false, proc_get_ioctl_list},
#endif
#ifdef TAPI_FEAT_DEBUG_BUFFER
   {"debug_buffer", false, proc_print_debug_buffer},
   {"debug_buffer_status", false, proc_print_debug_buffer_status},
   {"debug_buffer_console_printout", false,
      proc_debug_buffer_get_console_printout, NULL, NULL,
      proc_debug_buffer_set_console_printout}
#endif
};

/* Common proc dir for TAPI and DXS proc entries */
struct proc_dir_entry *proc_dir_tapi = NULL;

#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS
   struct proc_entry proc_entries_ppd[] = {
   #ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
      {"help", false, tapi_proc_ppd_read_help},
   #endif
      {"status", false, NULL, tapi_proc_ppd_read_status}
   };

   struct proc_dir_entry *proc_dir_ppd = NULL;

   #ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
      /* Common directory for phone detection devices */
      struct proc_dir_entry *proc_dir_ppd_dev = NULL;
   #endif
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS */

/* ============================= */
/* Local function definition     */
/* ============================= */

/**
   Read the version information from the driver.

   \param  s
   \return none
*/
static IFX_void_t proc_get_tapi_version(struct seq_file *s)
{
   seq_printf(s, "%s\n", &TAPI_DXS_DRV_WHATVERSION[4]);
   seq_printf(s, "%s\n", &TAPI_DRV_WHATVERSION[4]);
   seq_printf(s, "%s\n", &DXS_DRV_WHATVERSION[4]);

   {
      IFX_uint8_t dev = 0;
      DXS_DEVICE_t *pDev = IFX_NULL;

      for (dev = 0; dev < DXS_MAX_DEVICES; dev++)
      {
         DXS_GetDevice(dev, &pDev);

         if (pDev == IFX_NULL)
         {
            seq_printf(s, "Duslic device #%d: cannot get handle.\n", dev);
            continue;
         }

         if (!(pDev->nDevState & DS_BASIC_INIT))
         {
            seq_printf(s, "Duslic device #%d: not initialized yet.\n", dev);
            continue;
         }

         if (DXS_ReadFwVersion(pDev) != DXS_statusOk)
         {
            seq_printf(s, "Duslic device #%d: read FW version failed.\n", dev);
            continue;
         }

         seq_printf(s, "Device #%d: CHIP 0x%02X FW %d.%d.%d "
                       "DeviceID %d (0x%04X)\n",
                       dev, pDev->nChipRev,
                       (pDev->nFwRev >> 24) & 0xFF,
                       (pDev->nFwRev >> 16) & 0xFF,
                       (pDev->nFwRev >>  8) & 0xFF,
                       pDev->nDevId, pDev->nDevId);
      }
   }

   seq_printf(s, "Compiled for Linux kernel %s\n", UTS_RELEASE);
}

#ifdef HAVE_CONFIG_H
/**
   Read the configure parameters of the driver.

   \param  s
   \return none
*/
static IFX_void_t proc_ConfigureGet(struct seq_file *s)
{
#if IFXOS_BYTE_ORDER == IFXOS_BIG_ENDIAN
   seq_printf(s, "Compiled for big-endian\n");
#else
   seq_printf(s, "Compiled for little-endian\n");
#endif

   seq_printf(s, "configure %s\n", &DRV_TAPI_WHICHCONFIG[0]);
}
#endif /* HAVE_CONFIG_H */


#ifdef ENABLE_TRACE
/**
   Read the debug level setting from the driver.

   \param   s Pointer to seq_file struct.
   \return  0 on success
*/
static IFX_void_t proc_debug_level_get(struct seq_file *s)
{
   IFX_uint32_t level = TAPI_GET_TRACE_GROUP_VARIABLE(TAPI_DXS);
   char *level_name[] =
      {
         "unknown",
         "VERBOSE",   /* 1 LOW */
         "NORMAL",    /* 2 NORMAL */
         "SILENT",    /* 3 HIGH */
         "NO OUTPUT"  /* 4 OFF */
      };

   if (level >= ARRAY_SIZE(level_name))
   {
      level = 0;
   }

   /* Headline */
   seq_printf(s, "Possible debug levels: 1 - all logs (verbose), "
               "2 - infos and errors, 3 - high priority only, 4 - off\n");
   seq_printf(s, "Current debug level: %d (%s)\n",
              level, level_name[level]);
}


/**
   Set the debug level of the driver.

   This function just looks at the first character of input. If it is within
   the range of 1 -- 4 this is set as the debug level. All other input is
   ignored.

   \param   file    File structure for proc file.
   \param   buffer  Buffer holding the data.
   \param   count   Number of characters in buffer.
   \param   data    Unused.

   \return  count   Number of processed characters.
*/
static ssize_t proc_debug_level_set(struct file *file, const char *buffer,
                                    size_t count, loff_t *data)
{
   IFX_void_t *result = IFX_NULL;

   /* Maximum input length for proc_debug_level_set function */
   #define TAPI_DXS_DEBUG_SET_INPUT_MAX_LEN 1

   IFX_char_t input_line[TAPI_DXS_DEBUG_SET_INPUT_MAX_LEN] = {0};

   if (count < TAPI_DXS_DEBUG_SET_INPUT_MAX_LEN)
   {
      /* error: No input. */
      return -EINVAL;
   }

   result = TAPI_OS_CpyUsr2Kern(&input_line[0], buffer,
                                TAPI_DXS_DEBUG_SET_INPUT_MAX_LEN);
   if (result == IFX_NULL)
   {
      /* error: Data copy from user space failed. */
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("Problem with data copying.\n"));
      return -ENOMEM;
   }

   /* Look only at the first character. */
   if (input_line[0] >= '1' && input_line[0] <= '4')
   {
      IFX_int8_t trace_level = (input_line[0] - '1') + DBG_LEVEL_LOW;
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("Set debug level: %d\n", trace_level));
      TAPI_TRACE_LEVEL_SET(TAPI_DXS, trace_level);
   }
   else
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("Invalid value (allowed from 1 to 4).\n"));
      /* error: debug level out of range (1 -- 4). */
      return -EFAULT;
   }

   return count;
}

#endif /* ENABLE_TRACE */

#ifdef DEBUG

/**
   Print all IOCTLs that can be handled by TAPI
   \param  s seq_file
   \return none
*/
static IFX_void_t proc_get_ioctl_list(struct seq_file *s)
{
   int elem = 0;
   for (elem = 0; elem < tapi_ioctl_table_size; ++elem)
   {
      seq_printf(s, "Idx: %d, name: %s, num: 0x%08X, arg size: %d\n",
         (tapi_ioctl_table[elem].val & 0xFF),
         tapi_ioctl_table[elem].name,
         tapi_ioctl_table[elem].val,
         ((tapi_ioctl_table[elem].val & 0xBFFF0000) >> 16)
        );
   }
}

#endif /* DEBUG */

#ifdef TAPI_FEAT_DEBUG_BUFFER
/**
   Print all entries currently stored in TAPI debug buffer
   \param  s seq_file
   \param entry pointer to TAPI debug buffer entry
   \return none
*/
static IFX_void_t proc_print_debug_buffer_entry(struct seq_file *s,
                                                const struct TapiDebugBufferEntry *entry)
{
   int data_byte = 0;

   if (entry == IFX_NULL)
   {
      seq_printf(s, "Invalid pointer to TAPI debug buffer entry!");
      return;
   }

#if defined(TAPI_LINUX_KERNEL_SPACE)
   {
      IFX_uint32_t nanoseconds = 0;
      div_u64_rem(entry->timestamp, TAPI_OS_TIME_NSEC_IN_SEC, &nanoseconds);

      seq_printf(s, "[%lld.%09d] [%2d] [%2d]  [%4s][%3X][%3d][",
         div_u64(entry->timestamp, TAPI_OS_TIME_NSEC_IN_SEC),
         nanoseconds,
         entry->dev_nr, entry->ch_nr,
         tapi_debug_buffer_get_entry_type_name(entry->type),
         entry->reg_number,
         entry->data_length);
   }
#else
   seq_printf(s, "[%lld.%09lld] [%2d] [%2d]  [%s] [%3d] [",
      (entry->timestamp / TAPI_OS_TIME_NSEC_IN_SEC),
      (entry->timestamp % TAPI_OS_TIME_NSEC_IN_SEC),
      entry->dev_nr, entry->ch_nr,
      tapi_debug_buffer_get_entry_type_name(entry->type),
      entry->data_length);
#endif

   /* Print data bytes */
   for (data_byte = 0; data_byte < entry->data_length; data_byte++)
   {
      if (entry->type == TAPI_DBUF_USER_INFO)
      {
         seq_printf(s, "%c", entry->data[data_byte]);
      }
      else
      {
         if (data_byte == (entry->data_length - 1))
         {
            if (entry->data_length == TAPI_DEBUG_BUFFER_ENTRY_DATA_SIZE &&
                entry->data[data_byte] == '+')
            {
               seq_printf(s, "%c", entry->data[data_byte]);
            }
            else
            {
               seq_printf(s, "%02X", entry->data[data_byte]);
            }
         }
         else
            seq_printf(s, "%02X ", entry->data[data_byte]);
      }
   }

   seq_printf(s, "]\n");
}


/**
 * Print all entries stored in TAPI debug buffer
 * 
 * \param s destination buffer
 * \return nothing to return
 */
static IFX_void_t proc_print_debug_buffer(struct seq_file *s)
{
   IFX_size_t entries_to_print = min(tapi_debug_buffer_get_capacity(),
                                     tapi_debug_buffer_get_num_of_written_entries());
   IFX_boolean_t buffer_rolled_over = IFX_FALSE;
   IFX_size_t newest_index = tapi_debug_buffer_get_newest_entry_index(&buffer_rolled_over);
   int index = 0;

   /* Print header */
   seq_printf(s, "[Timestamp]     [dev][ch]  [type][reg][len] [data...]\n");

   /* Print debug buffer entries */
   for (index = 0; index < entries_to_print; ++index)
   {
      int index_to_read = index;
      const struct TapiDebugBufferEntry *entry = IFX_NULL;

      if (buffer_rolled_over)
      {
         index_to_read = (newest_index + index) % tapi_debug_buffer_get_capacity();
      }

      if (tapi_debug_buffer_get_entry(index_to_read, &entry))
      {
         return;
      }

      proc_print_debug_buffer_entry(s, entry);
   }
}


/**
 * Print TAPI debug buffer status information
 * 
 * \param s destination buffer
 * \return nothing to return
 */
static IFX_void_t proc_print_debug_buffer_status(struct seq_file *s)
{
   struct TapiDebugBufferStatus status = {0};

   seq_printf(s, "TAPI debug buffer status information:\n");

   if (tapi_debug_buffer_get_status_info(&status))
   {
      seq_printf(s, "Error while reading status info!\n");
      return;
   }

   seq_printf(s, "initialized?: %d\n", status.initialized);
   seq_printf(s, "capacity: %lu\n", status.capacity);
   seq_printf(s, "written entries: %lu\n", status.written_entries);
   seq_printf(s, "overwrite enabled?: %d\n", status.overwrite);
   seq_printf(s, "print to console enabled?: %d\n", status.print_to_console);
}


/**
   Read current tapi debug buffer console printouts setting.

   \param   s Pointer to seq_file struct.
   \return  0 on success
*/
static IFX_void_t proc_debug_buffer_get_console_printout(struct seq_file *s)
{
   struct TapiDebugBufferConfig config = {0};

   tapi_debug_buffer_get_config(&config);
   seq_printf(s, "TAPI debug buffer console printouts: %s\n",
              (config.print_to_console ? "enabled" : "disabled"));
}


/**
   Enable/disable debug buffer console printouts

   This function just looks at the first character of input.
   If it is 0 it disabled tapi debug buffer runtime console printouts. For 1 it
   enables them. All other input is ignored.

   \param   file    File structure for proc file.
   \param   buffer  Buffer holding the data.
   \param   count   Number of characters in buffer.
   \param   data    Unused.

   \return  count   Number of processed characters.
*/
static ssize_t proc_debug_buffer_set_console_printout(struct file *file,
                                                      const char *buffer,
                                                      size_t count,
                                                      loff_t *data)
{
   IFX_void_t *result = IFX_NULL;

   /* Maximum input length for proc_debug_level_set function */
   #define TAPI_DXS_DBG_BUF_PRINTS_INPUT_MAX_LEN 1

   IFX_char_t input_line[TAPI_DXS_DBG_BUF_PRINTS_INPUT_MAX_LEN] = {0};

   if (count < TAPI_DXS_DBG_BUF_PRINTS_INPUT_MAX_LEN)
   {
      /* error: No input. */
      return -EINVAL;
   }

   result = TAPI_OS_CpyUsr2Kern(&input_line[0], buffer,
                                TAPI_DXS_DBG_BUF_PRINTS_INPUT_MAX_LEN);
   if (result == IFX_NULL)
   {
      /* error: Data copy from user space failed. */
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("Problem with data copying.\n"));
      return -ENOMEM;
   }

   /* Look only at the first character. */
   if (input_line[0] == '0' || input_line[0] == '1')
   {
      IFX_int8_t enable_printouts = input_line[0] - '0';
      struct TapiDebugBufferConfig config = {0};

      tapi_debug_buffer_get_config(&config);
      config.print_to_console = (IFX_boolean_t) enable_printouts;
      tapi_debug_buffer_set_config(&config);
   }
   else
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("Invalid value (allowed from 0 to 1).\n"));
      return -EFAULT;
   }

   return count;
}
#endif /* TAPI_FEAT_DEBUG_BUFFER */


/**
   Read the status information from the driver.

   \param s destination buffer

   \return
   none
*/
static IFX_void_t proc_get_tapi_status(struct seq_file *s)
{
   /* Print Duslic devices status */
   proc_get_dxs_status(s);
}


/**
   Read the Duslic device status.

   \param buf  destination buffer

   \return
   length
*/
static IFX_int32_t proc_get_dxs_status(struct seq_file *buf)
{
   IFX_uint8_t          dev, ch;
   DXS_DEVICE_t         *pDev    =  IFX_NULL;
   DXS_CHANNEL_t        *pCh     =  IFX_NULL;
   const char * const   DcDcNames[] =
      {
         "DC/DC type unknown",
         "Dedicated Inverting Buck-Boost Converter",
         "Combined Inverting Buck-Boost Converter",
         "Dedicated Inverting Boost Converter",
         "Combined Inverting Boost Converter",
         "Dedicated Buck-or-Boost Converter",
         "Combined Buck-or-Boost Converter",
         "Dedicated Inverting Flyback Converter",
         "Combined Inverting Flyback Converter",
         "Dedicated Inverting Boost Converter with Gate Driver",
         "Combined Inverting Boost Converter with Gate Driver",
         "Dedicated Inverting Boost Converter with Voltage Doubler",
         "Combined Inverting Boost Converter with Voltage Doubler"
      };

   if (nAllowedDcDcType < (sizeof(DcDcNames)/sizeof(char *)))
   {
      seq_printf(buf, "Driver enforces %s\n", DcDcNames[nAllowedDcDcType]);
   }

   for (dev = 0; dev < DXS_MAX_DEVICES; dev++)
   {
      DXS_GetDevice(dev, &pDev);

      if (pDev != IFX_NULL)
      {
         seq_printf (buf, "Duslic device #%d:\n", dev);

         if (pDev->nDevState & DS_BASIC_INIT)
         {
            if (pDev->caps.nALI != 0)
            {
               /* DC/DC converter setting */
               for (ch = 0; ch < pDev->caps.nALI; ch++)
               {
                  pCh = &pDev->pChannel[ch];

                  if (pCh->pALM->nDcDcType == DXS_DCDC_TYPE_NOTSET)
                  {
                     seq_printf (buf, "  Channel %d: "
                                    "DC/DC converter operation mode not set\n", ch);
                  }
                  else
                  {
                     const char *pConverterName;

                     pConverterName = DcDcNames[0];
                     if (pCh->pALM->nDcDcType <= (sizeof(DcDcNames)/sizeof(char *))-1)
                     {
                        pConverterName = DcDcNames[pCh->pALM->nDcDcType];
                     }

                     seq_printf (buf, "  Channel %d: %s\n", ch, pConverterName);
                  }
               }
            }
            else
            {
               seq_printf (buf, "  Caps ALI = 0. Read caps first.\n");
            }
         }
         else /* (pDev->nDevState & DS_BASIC_INIT) */
         {
            seq_printf (buf, "  not initialized yet.\n");
         }


         /* PCM timeslot setting */
         if (pDev->nDevState & DS_PCM_EN)
         {
            IFX_uint32_t   i, j, Idx, BitValue;

            /* Print the current timeslot allocation bitmap in groups of 8 bits. */

            seq_printf (buf, "  PCM TS RX: ");
            for (i = j = 0;  i < pDev->nMaxTimeslot; i++)
            {
               Idx = i >> 5;
               BitValue = 1 << (i & 0x1F);
               TAPI_ASSERT(Idx < DXS_PCM_TS_ARRAY );
               seq_printf (buf, (pDev->PcmRxTs[Idx] & BitValue) ? "1" : "0");
               if ((++j % 8) == 0)
               {
                  seq_printf (buf, " ");
               }
            }
            seq_printf (buf, "\n");

            seq_printf (buf, "  PCM TS TX: ");
            for (i = j = 0;  i < pDev->nMaxTimeslot; i++)
            {
               Idx = i >> 5;
               BitValue = 1 << (i & 0x1F);
               TAPI_ASSERT(Idx < DXS_PCM_TS_ARRAY );
               seq_printf (buf, (pDev->PcmTxTs[Idx] & BitValue) ? "1" : "0");
               if ((++j % 8) == 0)
               {
                  seq_printf (buf, " ");
               }
            }
            seq_printf (buf, "\n");
         }
         else
         {
            seq_printf (buf, "  PCM not enabled.\n");
         }
      } /* if (pDev != IFX_NULL) */
   }

   return 0;
}


/**
   Read the registered low level driver information

   \param  s

   \return
   none
*/
static IFX_void_t proc_get_tapi_registered_drivers(struct seq_file *s)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = IFX_NULL;
   IFX_int32_t i;

   seq_printf(s, "%-10s %-10s %-10s %7s %10s\n",
                  "LL-Driver", "devname", "version", "major", "devices");
   seq_printf(s, "===================================================\n");

   for (i = 0; i < TAPI_MAX_LL_DRIVERS; i++)
   {
      if (gHLDrvCtx [i].pDrvCtx != IFX_NULL)
      {
         pDrvCtx = gHLDrvCtx [i].pDrvCtx;
         seq_printf(s, "%-10s %-10s %-10s %7d %10d\n",
            pDrvCtx->drvName, pDrvCtx->devNodeName, pDrvCtx->drvVersion,
            pDrvCtx->majorNumber,pDrvCtx->maxDevs);
      }
   }
}


/**
   Read the current bufferpool status

   \param  s

   \return
   none
*/
static IFX_void_t proc_read_bufferpool(struct seq_file *s)
{
   seq_printf(s, "TAPIpool (free/total): "
                  "event (%4d/%4d)\n",
                  IFX_TAPI_EventWrpBufferPool_ElementAvailCountGet(),
                  IFX_TAPI_EventWrpBufferPool_ElementCountGet());
}


static void *TAPI_seq_start(struct seq_file *s, loff_t *pos)
{
   struct proc_file_entry *p = s->private;

   if (*pos > p->nMaxPos || *pos < 0)
      return NULL;

   /* set current position */
   p->nPos = *pos;

   return *pos ? p : SEQ_START_TOKEN;
}


static void *TAPI_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
   struct proc_file_entry *p = s->private;

   (*pos)++;
   if (*pos > p->nMaxPos || *pos < 0)
      return NULL;

   /* update current position */
   p->nPos = *pos;

   return p;
}


static void TAPI_seq_stop(struct seq_file *s, void *v)
{
}


static int TAPI_seq_show(struct seq_file *s, void *v)
{
   struct proc_file_entry *p = s->private;

   if (v != SEQ_START_TOKEN)
      return p->callback(s, p->nPos);

   /* print header */
   return p->callback(s, 0);
}


static int TAPI_proc_open(struct inode *inode, struct file *file)
{
   struct seq_file *s;
   struct proc_file_entry *p;
   struct proc_entry *entry;
   int ret = -1;

#ifdef TAPI_FEAT_PROCFS_CAPABILITY_CHECK
   if (!capable(CAP_SYS_PACCT))
   {
      return -EACCES;
   }
#endif

   ret = seq_open(file, &TAPI_seq_ops);
   if (ret)
      return ret;

   s = file->private_data;
   p = kmalloc(sizeof(*p), GFP_KERNEL);

   if (!p)
   {
      (void)seq_release(inode, file);
      return -ENOMEM;
   }

   entry = PDE_DATA(inode);

   p->callback = entry->callback;
   if (entry->init_callback)
      p->nMaxPos = entry->init_callback();
   else
      p->nMaxPos = 1;

   s->private = p;

   return 0;
}


static int TAPI_proc_release(struct inode *inode, struct file *file)
{
   return seq_release_private(inode, file);
}


static int TAPI_seq_single_show(struct seq_file *s, void *v)
{
   struct proc_entry *p = s->private;

   p->single_callback(s);
   return 0;
}


static int TAPI_proc_single_open(struct inode *inode, struct file *file)
{
#ifdef TAPI_FEAT_PROCFS_CAPABILITY_CHECK
   if (!capable(CAP_SYS_PACCT))
   {
      return -EACCES;
   }
#endif

   return single_open(file, TAPI_seq_single_show, PDE_DATA(inode));
}


static void TAPI_proc_entry_create(struct proc_dir_entry *parent_node,
                                   struct proc_entry *proc_entry)
{
   mode_t mode = S_IFREG | S_IRUGO;
   memset(&proc_entry->ops, 0, sizeof(proc_entry->ops));

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0))
   proc_entry->ops.owner = THIS_MODULE;

   if (proc_entry->single_callback)
   {
      proc_entry->ops.open = TAPI_proc_single_open;
      proc_entry->ops.release = single_release;
   }
   else
   {
      proc_entry->ops.open = TAPI_proc_open;
      proc_entry->ops.release = TAPI_proc_release;
   }

   proc_entry->ops.read = seq_read;
   proc_entry->ops.write = proc_entry->write_function;
   proc_entry->ops.llseek = seq_lseek;

#else /* for KERNEL_VERSION >= (5,6,0) */

   if (proc_entry->single_callback)
   {
      proc_entry->ops.proc_open = TAPI_proc_single_open;
      proc_entry->ops.proc_release = single_release;
   }
   else
   {
      proc_entry->ops.proc_open = TAPI_proc_open;
      proc_entry->ops.proc_release = TAPI_proc_release;
   }

   proc_entry->ops.proc_read = seq_read;
   proc_entry->ops.proc_write = proc_entry->write_function;
   proc_entry->ops.proc_lseek = seq_lseek;
#endif

   if (proc_entry->write_function != NULL)
#ifdef TAPI_FEAT_PROCFS_STRICT_MODES
      mode |= S_IWUSR;
#else
      mode |= S_IWUGO;
#endif

#ifdef TAPI_FEAT_PROCFS_STRICT_MODES
   if (proc_entry->public)
   {
      /* Clear write and execute permission of group and others. */
      mode &= ~(S_IWGRP | S_IXGRP | S_IWOTH | S_IXOTH);
   }
   else
   {
      /* Clear all permissions of group and others. */
      mode &= ~(S_IRWXG | S_IRWXO);
   }
#endif /* TAPI_FEAT_PROCFS_STRICT_MODES */

   proc_create_data(proc_entry->name, mode, parent_node, &proc_entry->ops,
                    proc_entry);
}


/**
   Get device count

   \return  number of registered devices
*/
static int proc_get_dev_count (void)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx  = IFX_NULL;
   IFX_int32_t        i, nDeviceCounter = 0;

   /* for all LL-drivers */
   for (i = 0; i < TAPI_MAX_LL_DRIVERS; i++)
   {
      if (gHLDrvCtx [i].pDrvCtx == IFX_NULL)
      {
         continue;
      }

      pDrvCtx = gHLDrvCtx [i].pDrvCtx;

      if (pDrvCtx->pTapiDev == IFX_NULL)
      {
         /* driver not initialised yet -> skip it */
         break;
      }

      nDeviceCounter += pDrvCtx->maxDevs;
   } /* for all LL-drivers */
   return nDeviceCounter;
}


/**
   Read the current fifo status

   \param   s
   \param   pos     Device number starting from 1

   \return  zero or an error code
*/
static int proc_read_fifos (struct seq_file *s, int pos)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx  = IFX_NULL;
   TAPI_DEV           *pTapiDev = IFX_NULL;
   TAPI_CHANNEL       *pChannel = IFX_NULL;
   IFX_int32_t        i, j, k, nDeviceCounter = 0;
   IFX_boolean_t      bDeviceFound = IFX_FALSE;
   IFX_TAPI_EVENT_FIFO_COUNTERS_t evtCounters;

   /* pos = 0 means print header, but this procfs entry has no header */
   if (pos <= 0)
      return 0;

   /* for all LL-drivers */
   for (i = 0; i < TAPI_MAX_LL_DRIVERS; i++)
   {
      if (bDeviceFound)
         break;
      if (gHLDrvCtx [i].pDrvCtx == IFX_NULL)
      {
         continue;
      }

      pDrvCtx = gHLDrvCtx [i].pDrvCtx;

      if (pDrvCtx->pTapiDev == IFX_NULL)
      {
         /* driver not initialised yet -> skip it */
         break;
      }

      /* for all devices */
      for (j = 0; j < pDrvCtx->maxDevs; j++)
      {
         pTapiDev = &(pDrvCtx->pTapiDev[j]);

         if (++nDeviceCounter < pos)
            continue;
         bDeviceFound = IFX_TRUE;
         /* Headline (once per device) */
         seq_printf(s, "-- %s --\n"
                       "DEV%2d: evtH evtL ", pDrvCtx->drvName, j);

         /* Data line (one per channel) */

         /* Loop over all TAPI channels of this device */
         for (k=0; k < pDrvCtx->maxChannels; k++)
         {
            pChannel = &pTapiDev->pChannel[k];

            seq_printf(s, "\n CH%2d:", k);

            /* Event fifos */
            if (TAPI_statusOk ==
                IFX_TAPI_EventFifoCounters (pChannel, &evtCounters))
            {
               seq_printf(s, " %4d %4d ",
                              evtCounters.nHighWaiting,
                              evtCounters.nLowWaiting);
            }
            else
            {
               seq_printf(s, " FAIL FAIL ");
            }
         } /* for all channels */
         seq_printf(s, "\n");
         break;
      } /* for all devices */
   } /* for all LL-drivers */
   return 0;
}


/**
   Initialize and install the proc entry

   \return
   -1 or 0 on success
*/
IFX_int32_t proc_EntriesInstall(void)
{
   IFX_uint32_t i;

   /* install the proc entry */
   TRACE(TAPI_DXS, DBG_LEVEL_LOW, ("TAPI DXS: using proc fs.\n"));
   proc_dir_tapi = proc_mkdir("driver/" DRV_TAPI_NAME, IFX_NULL);
   if (proc_dir_tapi != IFX_NULL)
   {
      for (i = 0; i < ARRAY_SIZE(proc_entries); i++)
         TAPI_proc_entry_create(proc_dir_tapi, &proc_entries[i]);

#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS
      proc_dir_ppd = proc_mkdir("phone_detection", proc_dir_tapi);
      if (proc_dir_ppd != IFX_NULL)
      {
         for (i = 0; i < ARRAY_SIZE(proc_entries_ppd); i++)
            TAPI_proc_entry_create(proc_dir_ppd, &proc_entries_ppd[i]);

#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
         proc_dir_ppd_dev = proc_mkdir("devices", proc_dir_ppd);
         if (proc_dir_ppd_dev == IFX_NULL)
         {
            TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                 ("TAPI DXS: cannot create proc entry for phone "
                  "detection devices.\n"));
            return -1;
         }
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE */
      }
      else
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("TAPI DXS: cannot create phone detection proc entry\n"));
         return -1;
      }
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS */
   }
   else
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("TAPI DXS: cannot create proc entry\n"));
      return -1;
   }

   return 0;
}


/**
   Remove proc filesystem entries.

   \return
   None.

   \remarks
   Called by the kernel.
*/
IFX_void_t proc_EntriesRemove(void)
{
   IFX_uint32_t i;

   if (proc_dir_tapi != IFX_NULL)
   {
#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS
      if (proc_dir_ppd != NULL)
      {
#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
         if (proc_dir_ppd_dev != NULL)
         {
            remove_proc_entry("devices" , proc_dir_ppd);
            proc_dir_ppd_dev = NULL;
         }
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE */

         for (i = 0; i < ARRAY_SIZE(proc_entries_ppd); i++)
         {
            remove_proc_entry(proc_entries_ppd[i].name, proc_dir_ppd);
         }

         remove_proc_entry("phone_detection" , proc_dir_tapi);
         proc_dir_ppd = NULL;
      }
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS */

      for (i = 0; i < ARRAY_SIZE(proc_entries); i++)
      {
         remove_proc_entry(proc_entries[i].name, proc_dir_tapi);
      }

      remove_proc_entry("driver/" DRV_TAPI_NAME , NULL);
      proc_dir_tapi = NULL;
   }

   return;
}

#endif /* TAPI_FEAT_PROCFS */
#endif /* __KERNEL__ */
#endif /* LINUX */
