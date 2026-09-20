/******************************************************************************

  Copyright 2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/* ========================================================================== */
/*                       Includes                                             */
/* ========================================================================== */

#include "drv_tapi.h"
#include "drv_tapi_api.h"
#include "drv_tapi_ll_interface.h"

#include "drv_tapi_linux_procfs.h"
#include "drv_tapi_linux_procfs_phone_detection.h"

/* ========================================================================== */
/*                       Local function declarations                          */
/* ========================================================================== */

#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
   static int tapi_proc_ppd_wr_open(struct inode *inode,
                                    struct file *file);
   static ssize_t tapi_proc_ppd_wr_device(struct file *file,
                                          const char __user *buffer,
                                          size_t count,
                                          loff_t *offset);
   static IFX_boolean_t tapi_proc_ppd_wr_devParseCfgLine(
                           IFX_char_t* sCfgLine,
                           IFX_boolean_t* bAllCh,
                           IFX_int32_t* pCh,
                           IFX_boolean_t* bEnable);
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE */

/* ========================================================================== */
/*                       Local variables                                      */
/* ========================================================================== */

#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
   #if (LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0))
      static const struct file_operations proc_fops_ppd_device = {
         .open = tapi_proc_ppd_wr_open,
         .write = tapi_proc_ppd_wr_device
      };
   #else
      static const struct proc_ops proc_fops_ppd_device = {
         .proc_open = tapi_proc_ppd_wr_open,
         .proc_write = tapi_proc_ppd_wr_device
      };
   #endif

   /* Common directory for phone detection devices */
   extern struct proc_dir_entry *proc_dir_ppd_dev;
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE */

/* ========================================================================== */
/*                       Local function definitions                           */
/* ========================================================================== */

#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
/**
   Displays help information about 'phone_detection' entry.

   \param  s            Pointer to sequence file struct.
*/
IFX_void_t tapi_proc_ppd_read_help(struct seq_file *s)
{
   seq_printf(s,
      "Reading the 'status' node shows the current status of Phone Detection on all devices.\n");
   seq_printf(s,
      "Writing to 'devices/<dev_name>' node allows to enable/disable the Phone Detection.\n");
   seq_printf(s,
      "feature on selected channel(s) of '<dev_name>' device.\n\n");

   seq_printf(s,
      "Valid configuration line can be:\n\n");
   seq_printf(s,
      "{-}{+}<channel>\n\n");
   seq_printf(s,
      "<channel> - channel number\n");
   seq_printf(s,
      "{-} - disable Phone Detection on this channel. If nothing follows this symbol,\n"
      "      it applies to all channels. \n");
   seq_printf(s,
      "{+} - enable Phone Detection on this channel. This is the default action\n"
      "      if none of the signs precedes a channel number. If nothing follows\n"
      "      this symbol, it applies to all channels.\n\n");
   seq_printf(s,
      "Examples of valid cfg lines:\n");
   seq_printf(s,
      "\"+\"  - enable Phone Detection on all channels\n");
   seq_printf(s,
      "\"-\"  - disable Phone Detection on all channels\n");
   seq_printf(s,
      "\"-1\" - disable Phone Detection on channel no 1\n");
   seq_printf(s,
      "\"2\"  - enable Phone Detection on channel no 2\n");
   seq_printf(s,
      "\"+2\" - enable Phone Detection on channel no 2\n\n");

   seq_printf(s,
      "Example of usage:\n");
   seq_printf(s,
      "echo \"-1\" > /proc/driver/tapi_dxs/phone_detection/devices/duslicxs0\n");
}
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE */


#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS
/**
   Show the status of the phone detection on all devices

   \param  s            Pointer to sequence file struct.
   \param  pos          0 for table header, 1 for devices' status

   \return 0 in all cases.
*/
int tapi_proc_ppd_read_status (
                        struct seq_file *s,
                        int pos)
{
   /* In standalone TAPI for DXS driver there is only one low-level driver 
      registered which is Duslic driver */
   const IFX_TAPI_DRV_CTX_t *pLowLevDrvCtx = gHLDrvCtx[0].pDrvCtx;
   const TAPI_DEV *pTapiDev = IFX_NULL;
   IFX_int32_t j, k;
   
   /* Only pos 0 and 1 are allowed: 0 for printing table header,
      1 for printing PPD status for all Duslic devices and their channels */
   if (pos > 1)
   {
      return 0;
   }

   /* Print only the table header for pos 0. */
   if (pos == 0)
   {
      seq_printf(s,
         "========================================================================================================\n");
      seq_printf(s, "%10s|%3s|%6s|%4s|%5s|%20s|%5s|%15s|%6s|%6s|%6s|%6s|\n",
         "",   "",       "",   "", "phone",      "",   "act.",         "last",   "cap.",      "",       "",       "");
      seq_printf(s, "%10s|%3s|%6s|%4s|%5s|%20s|%5s|%15s|%6s|%6s|%6s|%6s|\n",
         "dev", "ch", "status", "sm",  "det.", "state",  "timer", "capacity[nF]", "thresh", "T1[s]", "T2[ms]", "T3[ms]");
      seq_printf(s,
         "========================================================================================================\n");

      return 0;
   }

   if (pLowLevDrvCtx == IFX_NULL)
      return 0;

   if (pLowLevDrvCtx->pTapiDev == IFX_NULL)
   {
      /* driver not initialised yet -> skip it */
      return 0;
   }

   /* Loop over all Duslic devices of the registered driver */
   for (j = 0; j < pLowLevDrvCtx->maxDevs; j++)
   {
      pTapiDev = &(pLowLevDrvCtx->pTapiDev[j]);


      if (!pTapiDev->bInitialized)
      {
         seq_printf(s,  "Device %s %d is not initialized yet.\n",
                     pLowLevDrvCtx->devNodeName, j);
         continue;
      }

      /* channel status */
      /* for the moment limit printout to two channels until the real
         number of channels is known internally */
      for (k = 0; (k < pTapiDev->nMaxChannel) && (k < 2); k++)
      {
         const TAPI_CHANNEL *pChannel = pTapiDev->pChannel + k;
         const TAPI_PPD_DATA_t *pTapiPpdData = pChannel->pTapiPpdData;

         if (pTapiPpdData == IFX_NULL)
         {
            continue;
         }
         IFX_TAPI_PPD_proc_read(s, pChannel, j);
      }
   }

   return 0;
}
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS */


#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
/**
   Open proc node for write

   \param  inode        Pointer to inode struct.
   \param  file         File structure for proc file.

   \return 0 in all cases.
*/
static int tapi_proc_ppd_wr_open(
                        struct inode *inode,
                        struct file *file)
{
   /* Copy the pointer to the tapi device struct from the inode to the file
      private data. */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
   #define PDE_DATA(inode) PDE(inode)->data
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5,17,0))
   #define PDE_DATA pde_data
#endif

   file->private_data = PDE_DATA(inode);

   return 0;
}
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE */


#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
/**
    Enable/Disable Phone detection functionality.

   \param  file         File structure for proc file.
   \param  buffer       Buffer holding the data.
   \param  count        Number of characters in buffer.
   \param  offset       unused.

   \return count        Number of processed characters
*/
static ssize_t tapi_proc_ppd_wr_device(
                        struct file *file,
                        const char __user *buffer,
                        size_t count,
                        loff_t *offset)
{
   enum { MAX_IN_BUF_SIZE = 100 };
   TAPI_DEV           *pTapiDev = (TAPI_DEV *)file->private_data;
   IFX_char_t *temp;
   IFX_int32_t len = -1;
   IFX_int32_t i;
   IFX_boolean_t bAllCh = IFX_FALSE;
   IFX_int32_t nCh = 0;
   IFX_boolean_t bEnable = IFX_FALSE;

   TAPI_UNUSED (offset);

   if (!pTapiDev || !pTapiDev->bInitialized)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("TAPI ERROR: Device not initalised yet.\n"));
      return -EFAULT;
   }

   temp = (IFX_char_t*)TAPI_OS_Malloc (MAX_IN_BUF_SIZE);
   if (temp == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("TAPI ERROR: Unable to allocate memory for TAPI private data.\n"));
      return -ENOMEM;
   }

   len = count;
   if (count > MAX_IN_BUF_SIZE)
   {
      len = MAX_IN_BUF_SIZE;
   }

   if (copy_from_user(temp, buffer, (unsigned int) len) != 0)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("TAPI ERROR: Data copy from user space failed.\n"));
      TAPI_OS_Free(temp);
      return -EFAULT;
   }

   /* Because we get string terminated with linefeed (0x0A)
       we terminate it with null terminator. */
   temp[len - 1] = '\0';

   if (tapi_proc_ppd_wr_devParseCfgLine(temp, &bAllCh, &nCh, &bEnable) !=
       IFX_TRUE)
   {
      TAPI_OS_Free(temp);
      return -EFAULT;
   }

   /* Enable/Disable Phone Detection functionality on channel(s) */
   for (i = 0; i < pTapiDev->nMaxChannel; i++)
   {
      TAPI_CHANNEL *pChannel = pTapiDev->pChannel + i;
      const TAPI_PPD_DATA_t *pTapiPpdData = pChannel->pTapiPpdData;

      if (pTapiPpdData == IFX_NULL)
      {
         continue;
      }
      if ((bAllCh == IFX_TRUE) || (nCh == pChannel->nChannel))
      {
         if (bEnable == IFX_TRUE)
            IFX_TAPI_PPD_EnPhoneDetOnCh(pChannel);
         else
            IFX_TAPI_PPD_DisPhoneDetOnCh(pChannel);
      }
   }

   TAPI_OS_Free(temp);
   return len;
}
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE */


#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
/**
   Parsing input string to extract commands for enabling/disabling Phone
   Detection functionality.

   \param  sCfgLine     Buffer with configuration line.
   \param  bAllCh       Will return IFX_TRUE if action should be performed
                        on all channels, otherwise IFX_FALSE.
   \param  pCh          Returns the channel on which Phone Detection should
                        be changed.
   \param  bEnable      Returns if enable (IFX_TRUE) or disable it (IFX_FALSE)
                        is selected.

   \return IFX_TRUE if configuration line was parsed or IFX_FALSE if not.

   \remarks Valid configuration line can be:

           "{-}{+}<channel>"

   <channel> - channel number

   {-} - disable Phone Detection on this channel. If nothing follows this
         symbol, it applies to all channels.
   {+} - enable Phone Detection on this channel. This is the default action
         if none of the signs precede a channel number. If nothing follows this
         symbol, it applies to all channels.

   Examples of valid cfg lines:
   ----------------------------
   "+" - enable Phone Detection on all channels
   "-" - disable Phone Detection on all channels
   "-1" - disable Phone Detection on channel no 1
   "2" - enable Phone Detection on channel no 2
   "+2" - enable Phone Detection on channel no 2
*/
static IFX_boolean_t tapi_proc_ppd_wr_devParseCfgLine(
                        IFX_char_t* sCfgLine,
                        IFX_boolean_t* bAllCh,
                        IFX_int32_t* pCh,
                        IFX_boolean_t* bEnable)
{
   /** Null terminiating character for strings */
   const IFX_char_t STRING_END = '\0';
   /** Sign for disabling channel(s) */
   const IFX_char_t DISABLE_CHAN = '-';
   /** Sign for enabling channel */
   const IFX_char_t ENABLE_CHAN = '+';

   IFX_char_t* leftover_tmp = IFX_NULL;
   IFX_char_t* start_tmp = IFX_NULL;
   IFX_int32_t ch = 0;

   TAPI_ASSERT (sCfgLine);
   TAPI_ASSERT (bAllCh);
   TAPI_ASSERT (pCh);

   *bEnable = IFX_TRUE;
   *bAllCh = IFX_FALSE;

   if (strlen(sCfgLine) == 1)
   {
      if (*sCfgLine == ENABLE_CHAN)
      {
         /* Enable it on all channels */
         *bEnable = IFX_TRUE;
         *bAllCh = IFX_TRUE;
         return IFX_TRUE;
      }
      else if (*sCfgLine == DISABLE_CHAN)
      {
         /* Disable it on all channels */
         *bEnable = IFX_FALSE;
         *bAllCh = IFX_TRUE;
          return IFX_TRUE;
      }
   }

   if (*sCfgLine == DISABLE_CHAN)
   {
      /* Will disable channel */
      *bEnable = IFX_FALSE;
      start_tmp = sCfgLine + 1;
   }
   else if (*sCfgLine == ENABLE_CHAN)
   {
      /* Will enable channel */
      *bEnable = IFX_TRUE;
      start_tmp = sCfgLine + 1;
   }
   else
   {
      start_tmp = sCfgLine;
   }

   /* ---------------     extract channel     ------------------- */

   ch = simple_strtol(start_tmp, &leftover_tmp, 10);

   if (*leftover_tmp != STRING_END)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
              ("ERROR, Invalid sign '%c'. (File: %s, line: %d)\n",
               *leftover_tmp, __FILE__, __LINE__));
      return IFX_FALSE;
   }

   if (abs(ch) > 128)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
              ("ERROR, Wrong channel number %d. (File: %s, line: %d)\n",
               ch, __FILE__, __LINE__));
      return IFX_FALSE;
   }
   *pCh = ch;

   return IFX_TRUE;
}
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE */


#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
/**
   Remove proc entry for device which supports phone detection.

   \param  pTapiDev     Pointer to TAPI_DEV structure.
*/
IFX_void_t TAPI_ProcPpdDeviceEntryRemove(
                        TAPI_DEV *pTapiDev)
{
   /* Only do the remove when the create was successful.
      Otherwise the procfs API will throw a warning. */
   if ((pTapiDev->nInitStatusFlags & TAPI_INITSTATUS_PPD_PROCFS_CREATED) == 0)
   {
      return;
   }
   pTapiDev->nInitStatusFlags &= ~TAPI_INITSTATUS_PPD_PROCFS_CREATED;

   {
      enum { ENTRY_NAME_BUF_SIZE = 20 };
      IFX_char_t EntryName[ENTRY_NAME_BUF_SIZE] = {0};

      (void) snprintf(EntryName, sizeof(EntryName)-1, "%.8s%.1u",
                      pTapiDev->pDevDrvCtx->drvName, pTapiDev->nDev);
      remove_proc_entry(EntryName, proc_dir_ppd_dev);
   }
}
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE */


#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
/**
   Install proc entry for device which supports phone detection.

   \param  pTapiDev     Pointer to TAPI_DEV structure.

   \return
     - TAPI_statusOk: if successful
     - error code: in case of an error
*/
IFX_int32_t TAPI_ProcPpdDeviceEntryInstall(
                        TAPI_DEV *pTapiDev)
{
   /* Exit if the procfs entry was already created. */
   if (pTapiDev->nInitStatusFlags & TAPI_INITSTATUS_PPD_PROCFS_CREATED)
   {
      return TAPI_statusOk;
   }

   if (proc_dir_ppd_dev != NULL)
   {
      enum { ENTRY_NAME_BUF_SIZE = 20 };
      IFX_char_t EntryName[ENTRY_NAME_BUF_SIZE] = {0};
      struct proc_dir_entry *proc_stat_node;

      /* Build directory name */
      snprintf(EntryName, sizeof(EntryName)-1, "%.8s%.1u",
               pTapiDev->pDevDrvCtx->drvName, pTapiDev->nDev);

      proc_stat_node =
         proc_create_data(EntryName, (S_IFREG | S_IWUGO), proc_dir_ppd_dev,
                          &proc_fops_ppd_device, pTapiDev);

      if (proc_stat_node == NULL)
      {
         /* errmsg: Device not added to proc fs */
         return TAPI_statusDeviceNotAddedToProcFs;
      }

      /* Remember for the remove function that the procfs entry was created. */
      pTapiDev->nInitStatusFlags |= TAPI_INITSTATUS_PPD_PROCFS_CREATED;
   }

   return TAPI_statusOk;
}
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE */