/******************************************************************************

  Copyright 2023-2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_debug_buffer.c
   This module provide debug buffer functionality.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include "drv_tapi_debug_buffer.h"
#include "drv_tapi.h"
#include "drv_tapi_osmap_local.h"
#include <linux/math64.h>

#ifdef TAPI_FEAT_DEBUG_BUFFER


/* ========================================================================== */
/*                             Local macros                                   */
/* ========================================================================== */

/* Find useful function for debug buffer console printouts even if DEBUG and
   ENABLE_TRACE symbols are not defined. */
#if defined(ENABLE_TRACE)
   /* If all driver traces are enabled, just use TRACE macro */
   #define TRACE_DEBUG_BUFFER TRACE
#else
   #if defined(TAPI_LINUX_KERNEL_SPACE)
      #define TRACE_DEBUG_BUFFER(module_name, level, print_message) \
               pr_info print_message;
   #elif defined(TAPI_LINUX_USER_SPACE)
      #define TRACE_DEBUG_BUFFER(module_name, level, print_message) \
               printf print_message;
   #endif
#endif

/* ========================================================================== */
/*                             Local type definitions                         */
/* ========================================================================== */

struct TapiDebugBuffer
{
   /* Buffer address */
   struct TapiDebugBufferEntry *buffer;
   /* Debug buffer capacity (number of entries that can be stored in buffer) */
   IFX_size_t capacity;
   /* Protection for buffer configuration members access */
   TAPI_OS_mutex_t cfg_lock;
   /* If true - new entries will overwrite first ones when buffer becomes full
      (buffer will act as ring buffer) otherwise new entries will be discarded */
   IFX_boolean_t overwrite;
   /* Print new entries to console */
   IFX_boolean_t print_to_console;

   /* Protect data when writing to buffer */
   TAPI_OS_mutex_t write_lock;
   /* Buffer index that can be used to currently write into */
   IFX_ulong_t write_index;
   /* Number of written entries to buffer (when buffer works as ring buffer
      this number can be higher than buffer capacity) */
   IFX_ulong_t written_entries;
   /* Pause buffering - when set, adding entries to the buffer is stopped */
   IFX_boolean_t paused;
};


/* Definition of single entry in debug buffer entry type name lookup table */
struct TapiDebugBufferEntryTypeName
{
   enum TapiDebugBufferEntryType type;
   IFX_char_t name[4];
};

/* ========================================================================== */
/*                             Local variables                                */
/* ========================================================================== */

/* Main debug buffer structure holding buffer address, locks, config */
static struct TapiDebugBuffer DebugBuffer = {
   .capacity = TAPI_DEBUG_BUFFER_NUM_OF_ENTRIES,
   .overwrite = IFX_FALSE,
   .print_to_console = IFX_TRUE,
   .paused = IFX_FALSE
};

/* Helper structure holding entry type name enum to string translations */
static struct TapiDebugBufferEntryTypeName DebugBufferTypeNames[] =
{
   {TAPI_DBUF_SPI_REG_READ, "RD"},
   {TAPI_DBUF_SPI_REG_WRITE, "WR"},
   {TAPI_DBUF_USER_INFO, "UI"}
};

/* ========================================================================== */
/*                             Local functions dedeclarations                 */
/* ========================================================================== */

static IFX_int32_t tapi_debug_buffer_add_entry_find_index(IFX_size_t *index);
static IFX_int32_t tapi_debug_buffer_print_entry_to_console(
                                    const struct TapiDebugBufferEntry *entry,
                                    IFX_boolean_t print_timestamp);


/* ========================================================================== */
/*                             Functions definitions                          */
/* ========================================================================== */

/**
 * Allocate memory for TAPI debug buffer, init locks
 * 
 * \return IFX_int32_t IFX_SUCCESS if buffer was initialized successfully,
 *         otherwise IFX_ERROR
 */
IFX_int32_t tapi_debug_buffer_init()
{
   if (DebugBuffer.buffer == IFX_NULL)
      DebugBuffer.buffer = TAPI_OS_Malloc(DebugBuffer.capacity *
                                          sizeof(struct TapiDebugBufferEntry));

   /* Buffer initialized properly */
   if (DebugBuffer.buffer)
   {
      /* Clear memory to avoid garbage reading via e.g. ioctl if */
      memset(DebugBuffer.buffer, 0,
            (DebugBuffer.capacity * sizeof(struct TapiDebugBufferEntry)));

      if (TAPI_OS_MutexInit(&DebugBuffer.cfg_lock))
         goto error_free_buffer;

      if (TAPI_OS_MutexInit(&DebugBuffer.write_lock))
         goto error_clean_cfg_mutex;

      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
         ("TAPI debug buffer of capacity %lu initialized successfully.\n",
         DebugBuffer.capacity));

      return IFX_SUCCESS;
   }

error_clean_cfg_mutex:
   TAPI_OS_MutexDelete(&DebugBuffer.cfg_lock);

error_free_buffer:
   TAPI_OS_Free(DebugBuffer.buffer);
   DebugBuffer.buffer = IFX_NULL;

   TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("TAPI debug buffer init failed!\n"));

   return IFX_ERROR;
}


/**
 * Free memory allocated by TAPI debug buffer, delete initialized locks
 * 
 * \return IFX_void_t Nothing to return.
 */
IFX_void_t tapi_debug_buffer_free()
{
   if (DebugBuffer.buffer != IFX_NULL)
   {
      TAPI_OS_Free(DebugBuffer.buffer);

      TAPI_OS_MutexDelete(&DebugBuffer.cfg_lock);
      TAPI_OS_MutexDelete(&DebugBuffer.write_lock);
   }
}


/**
 * Get TAPI debug buffer status information
 * 
 * \param TapiDebugBufferStatus Pointer to TAPI debug buffer info status struct
 *                              (will be filled with status info)
 * 
 * \return IFX_ERROR if input param is invalid, IFX_SUCCESS after filling status
 *         info struct
 */
IFX_int32_t tapi_debug_buffer_get_status_info(struct TapiDebugBufferStatus *status)
{
   if (status == IFX_NULL)
      return IFX_ERROR;

   status->initialized = (DebugBuffer.buffer != IFX_NULL ? IFX_TRUE : IFX_FALSE);
   status->overwrite = DebugBuffer.overwrite;
   status->written_entries = DebugBuffer.written_entries;
   status->print_to_console = DebugBuffer.print_to_console;
   status->capacity = DebugBuffer.capacity;
   status->paused = DebugBuffer.paused;

   return IFX_SUCCESS;
}


/**
 * Get entry from debug buffer stored at given index
 * 
 * \param[in] index Index from buffer to retrieve data from
 * \param[out] entry Pointer to pointer to debug buffer entry data from given index.
 *                   In order to avoid whole data entry copying.
 * \return IFX_int32_t IFX_ERROR if input params are invalid,
 *         IFX_SUCCESS if pointer to data entry was written to entry pointer.
 */
IFX_int32_t tapi_debug_buffer_get_entry(IFX_size_t index, const struct TapiDebugBufferEntry **entry)
{
   if (DebugBuffer.buffer == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("TAPI debug buffer not initialized yet!\n"));
      return IFX_ERROR;
   }

   if (entry == IFX_NULL)
   {
      return IFX_ERROR;
   }

   if (index > DebugBuffer.capacity - 1)
      return IFX_ERROR;

   *entry = &DebugBuffer.buffer[index];
   return IFX_SUCCESS;
}


/**
 * Get debug buffer config switches
 * 
 * \param config Pointer to debug buffer config structure
 * \return IFX_int32_t IFX_ERROR if invalid input param is given
 */
IFX_int32_t tapi_debug_buffer_get_config(struct TapiDebugBufferConfig *config)
{
   if (config == IFX_NULL)
      return IFX_ERROR;

   TAPI_OS_MutexGet(&DebugBuffer.cfg_lock);
   config->overwrite = DebugBuffer.overwrite;
   config->print_to_console = DebugBuffer.print_to_console;
   config->paused = DebugBuffer.paused;
   TAPI_OS_MutexRelease(&DebugBuffer.cfg_lock);
   return IFX_SUCCESS;
}


/**
 * Set debug buffer config switches
 * 
 * \param config Pointer to debug buffer config structure
 * \return IFX_int32_t IFX_ERROR if invalid input param is given
 */
IFX_int32_t tapi_debug_buffer_set_config(const struct TapiDebugBufferConfig *config)
{
   if (config == IFX_NULL)
      return IFX_ERROR;

   TAPI_OS_MutexGet(&DebugBuffer.cfg_lock);
   DebugBuffer.print_to_console = config->print_to_console;
   DebugBuffer.overwrite = config->overwrite;
   DebugBuffer.paused = config->paused;
   TAPI_OS_MutexRelease(&DebugBuffer.cfg_lock);
   return IFX_SUCCESS;
}


/**
 * Get number of entries that can be stored in TAPI debug buffer
 * 
 * \return TAPI debug buffer capacity (number of entries)
 */
const IFX_size_t tapi_debug_buffer_get_capacity(IFX_void_t)
{
   return DebugBuffer.capacity;
}


/**
 * Get number of entries that were already written to buffer
 * 
 * \return TAPI debug buffer capacity (number of entries)
 */
const IFX_size_t tapi_debug_buffer_get_num_of_written_entries(IFX_void_t)
{
   return DebugBuffer.written_entries;
}


/**
 * Get index of latest entry that we written into buffer
 * 
 * \param[out] rolled_over Will be set to true if buffer already rolled over
 * \return Index of entry that were written to buffer as last one (newest)
 */
const IFX_size_t tapi_debug_buffer_get_newest_entry_index(IFX_boolean_t *rolled_over)
{
   *rolled_over = (DebugBuffer.written_entries > DebugBuffer.capacity);

   return DebugBuffer.write_index;
}


/**
 * Prepare debug buffer entry for data write - find index to use for data storage
 * 
 * \return IFX_SUCCESS if index was found successfully, otherwise IFX_ERROR
 *         (e.g. when buffer is full and overwrite option is not set)
 */
static IFX_int32_t tapi_debug_buffer_add_entry_find_index(IFX_size_t *index)
{
   TAPI_OS_MutexGet(&DebugBuffer.write_lock);

   if (DebugBuffer.write_index > (DebugBuffer.capacity - 1))
   {
      TAPI_OS_MutexGet(&DebugBuffer.cfg_lock);
      if (DebugBuffer.overwrite)
         *index = 0;
      else
      {
         TAPI_OS_MutexRelease(&DebugBuffer.cfg_lock);
         TAPI_OS_MutexRelease(&DebugBuffer.write_lock);
         return IFX_ERROR;
      }

      TAPI_OS_MutexRelease(&DebugBuffer.cfg_lock);
   }
   else
   {
      *index = DebugBuffer.write_index;
   }

   DebugBuffer.write_index = *index + 1;
   DebugBuffer.written_entries++;

   TAPI_OS_MutexRelease(&DebugBuffer.write_lock);
   return IFX_SUCCESS;
}


/**
 * Add entry to TAPI debug buffer.
 * 
 * \param dev_number Device number
 * \param ch_number Channel number
 * \param type Entry type (e.g. to mark SPI read, write, user info)
 * \param register Register number (address) used in SPI reads/writes
 * \param data Pointer to data
 * \param data_length Data length given in bytes
 * \return IFX_int32_t IFX_ERROR if index for new entry cannot be found.
 *         IFX_SUCCESS if entry was added successfully.
 * 
 * \note Data larger than TAPI_DEBUG_BUFFER_ENTRY_DATA_SIZE will be automatically
 *       truncated and '+' sign will be inserted as last character in entry data
 *       field.
 */
IFX_int32_t tapi_debug_buffer_add_reg_entry(IFX_uint16_t dev_number,
                                            IFX_uint16_t ch_number,
                                            enum TapiDebugBufferEntryType type,
                                            const IFX_uint8_t reg_number,
                                            const IFX_uint8_t *const data,
                                            IFX_uint16_t data_length)
{
   /* Get timestamp at first to have it close to function call */
   IFX_ulong_t timestamp = TAPI_OS_GetTimestamp();
   /* Pinter do buffer entry or temporary storage for printing */
   struct TapiDebugBufferEntry *new_entry = IFX_NULL;
   /* Temporary storage for entry when only printing to console can be done */
   struct TapiDebugBufferEntry temp_entry = {0};
   /* Buffer entry index to which data will be written */
   IFX_size_t index = 0;

   /* Skip adding entry to buffer and even printing it to console when buffering
      is paused */
   if (DebugBuffer.paused)
      return IFX_SUCCESS;

   if (tapi_debug_buffer_add_entry_find_index(&index))
   {
      /* index not found - if printing to console is enabled then just print it */
      if (DebugBuffer.print_to_console)
         new_entry = &temp_entry;
      else
         return IFX_ERROR;
   }
   else
   {
      new_entry = &DebugBuffer.buffer[index];
   }

   new_entry->timestamp = timestamp;
   new_entry->dev_nr = dev_number;
   new_entry->ch_nr = ch_number;
   new_entry->type = type;
   new_entry->reg_number = reg_number;

   /* Prevent saving to many bytes in entry's data field */
   if (data_length > TAPI_DEBUG_BUFFER_ENTRY_DATA_SIZE)
   {
      data_length = TAPI_DEBUG_BUFFER_ENTRY_DATA_SIZE;
      memcpy(new_entry->data, data, data_length);

      /* Set buffer overflow sign at the end of data */
      new_entry->data[TAPI_DEBUG_BUFFER_ENTRY_DATA_SIZE - 1] = '+';
   }
   else
   {
      memcpy(new_entry->data, data, data_length);
   }

   new_entry->data_length = data_length;

   if (DebugBuffer.print_to_console)
      tapi_debug_buffer_print_entry_to_console(new_entry, false);

   return IFX_SUCCESS;
}

/**
 * Add entry to TAPI debug buffer.
 * 
 * \param dev_number Device number
 * \param ch_number Channel number
 * \param type Entry type (e.g. to mark SPI read, write, user info)
 * \param data Pointer to data
 * \param data_length Data length given in bytes
 * \return IFX_int32_t IFX_ERROR if index for new entry cannot be found.
 *         IFX_SUCCESS if entry was added successfully.
 * 
 * \note Data larger than TAPI_DEBUG_BUFFER_ENTRY_DATA_SIZE will be automatically
 *       truncated and '+' sign will be inserted as last character in entry data
 *       field.
 */
IFX_int32_t tapi_debug_buffer_add_entry(IFX_uint16_t dev_number,
                                         IFX_uint16_t ch_number,
                                         enum TapiDebugBufferEntryType type,
                                         const IFX_uint8_t *const data,
                                         IFX_uint16_t data_length)
{
   return tapi_debug_buffer_add_reg_entry(dev_number, ch_number, type, 0,
                                          data, data_length);
}


/**
 * Add entry to buffer using user formatted string with variadic number of parameters
 * 
 * \param user_info Pointer to string with optional format specifiers
 * \param ... parameters to be used to fill string format specifiers
 * \return IFX_int32_t IFX_ERROR if user_info is IFX_NULL, otherwise return
 *         code from tapi_debug_buffer_add_entry() call.
 * 
 * \note String after formatting cannot be longer than entry data[] field. Longer
 *       string will be automatically truncated.
 *       This function adds entry with params: device = 0, channel = 0, type = TAPI_DBUF_USER_INFO
 */
IFX_int32_t tapi_debug_buffer_add_user_entry(const char *user_info, ...)
{
   va_list args;
   char user_info_buffer[TAPI_DEBUG_BUFFER_ENTRY_DATA_SIZE] = {0};
   IFX_uint16_t user_info_length;

   if (user_info == IFX_NULL)
      return IFX_ERROR;

   /* Skip adding entry to buffer and even printing it to console when buffering
      is paused */
   if (DebugBuffer.paused)
      return IFX_SUCCESS;

   va_start(args, user_info);

   user_info_length = vsnprintf(user_info_buffer,
                                sizeof(user_info_buffer),
                                user_info, args);

   va_end(args);

   return tapi_debug_buffer_add_entry(0, 0, TAPI_DBUF_USER_INFO,
                                      user_info_buffer, user_info_length);
}


/**
 * Print debug buffer entry. Do not use entry timestamp info,
 * instead leave only printk's one.
 * 
 * \param entry Pointer to debug buffer entry to print
 * \param print_timestamp If true, also entry timestamp will be printed
 * \return IFX_int32_t IFX_ERROR in case of invalid input param, otherwise
 *         IFX_SUCCESS
 */
static IFX_int32_t tapi_debug_buffer_print_entry_to_console(
                                    const struct TapiDebugBufferEntry *entry,
                                    IFX_boolean_t print_timestamp)
{
   static IFX_char_t print_buffer[1024] = {0};
   /* Current index in print buffer */
   IFX_size_t curr_index = 0;

   if (entry == IFX_NULL)
      return IFX_ERROR;

   memset(print_buffer, 0, sizeof(print_buffer));

   if (print_timestamp)
   {
#if defined(TAPI_LINUX_KERNEL_SPACE)
      IFX_uint32_t nanoseconds = 0;
      div_u64_rem(entry->timestamp, TAPI_OS_TIME_NSEC_IN_SEC, &nanoseconds);

      curr_index += snprintf(
         &print_buffer[curr_index],
         sizeof(print_buffer) - curr_index,
         "[%lld.%09d] ",
         div_u64(entry->timestamp, TAPI_OS_TIME_NSEC_IN_SEC),
         nanoseconds);
#else
      curr_index += snprintf(
               &print_buffer[curr_index],
               sizeof(print_buffer) - curr_index,
               "[%lld.%09lld] ",
               (entry->timestamp / TAPI_OS_TIME_NSEC_IN_SEC),
               (entry->timestamp % TAPI_OS_TIME_NSEC_IN_SEC));
#endif
   }

   /* Add entry data */
   switch (entry->type)
   {
      case TAPI_DBUF_SPI_REG_READ:
      case TAPI_DBUF_SPI_REG_WRITE:
      {
         IFX_size_t i = 0;

         /* Add common info to buffer */
         curr_index += snprintf(&print_buffer[curr_index],
                     sizeof(print_buffer) - curr_index,
                     "[D:%02d] [C:%02d] [%s] [R:%02X] [L:%02d] [",
                     entry->dev_nr, entry->ch_nr,
                     tapi_debug_buffer_get_entry_type_name(entry->type),
                     entry->reg_number, entry->data_length);

         for (i = 0; i < entry->data_length; ++i)
         {
            /* Last data byte stored in buffer */
            if (i == (entry->data_length - 1))
            {
               /* Check if last data byte is '+' sign (means that data was truncated) */
               IFX_char_t *fmt_string = "%02X";
               if (entry->data_length == TAPI_DEBUG_BUFFER_ENTRY_DATA_SIZE &&
                   entry->data[i] == '+')
               {
                  fmt_string = "%c";
               }

               /* Do not add space separator after last data byte */
               curr_index += snprintf(&print_buffer[curr_index],
                  sizeof(print_buffer) - curr_index, fmt_string, entry->data[i]);
            }
            else
            {
               curr_index += snprintf(&print_buffer[curr_index],
                                 sizeof(print_buffer) - curr_index,
                                 "%02X ", entry->data[i]);
            }
         }
         break;
      }

      case TAPI_DBUF_USER_INFO:
      {
         curr_index += snprintf(&print_buffer[curr_index],
            sizeof(print_buffer) - curr_index,
            "[D:--] [C:--] [%s] [R:--] [L:%02d] [",
            tapi_debug_buffer_get_entry_type_name(entry->type), entry->data_length);
                     
         snprintf(&print_buffer[curr_index],
                  sizeof(print_buffer) - curr_index,
                  "%s", entry->data);
         break;
      }

      case TAPI_DBUF_ENTRY_TYPE_LAST:
         break;
   }

   TRACE_DEBUG_BUFFER(TAPI_DXS, DBG_LEVEL_HIGH, ("%s]\n", print_buffer));

   return IFX_SUCCESS;
}


/**
 * Dump all debug buffer entries to the console.
 * 
 * \param num_of_last_entries Number of last debug buffer entries to print.
 *                            If 0 is given all entries will be printed.
 * \return IFX_void_t         Nothing to return.
 */
IFX_void_t tapi_debug_buffer_dump_buffer_to_console(IFX_size_t num_of_last_entries)
{
   IFX_size_t entries_to_print =
      (tapi_debug_buffer_get_capacity() < tapi_debug_buffer_get_num_of_written_entries()) ?
       tapi_debug_buffer_get_capacity() : tapi_debug_buffer_get_num_of_written_entries();
   IFX_boolean_t buffer_rolled_over = IFX_FALSE;
   IFX_size_t newest_index = tapi_debug_buffer_get_newest_entry_index(&buffer_rolled_over);
   int index = 0;

   /* Print header */
   TRACE_DEBUG_BUFFER(TAPI_DXS, DBG_LEVEL_HIGH,
         ("[Timestamp]        [dev][ch] [type][len] [data...]\n"));

   if (num_of_last_entries > tapi_debug_buffer_get_capacity())
      num_of_last_entries = tapi_debug_buffer_get_capacity();

   if (num_of_last_entries > 0)
      entries_to_print = (entries_to_print < num_of_last_entries) ?
                          entries_to_print : num_of_last_entries;

   /* Print debug buffer entries */
   for (index = 0; index < entries_to_print; ++index)
   {
      int index_to_read = index;
      const struct TapiDebugBufferEntry *entry = IFX_NULL;

      if (buffer_rolled_over)
      {
         index_to_read = (newest_index + index) % tapi_debug_buffer_get_capacity();
      }

      if (tapi_debug_buffer_get_entry(index_to_read, &entry) != IFX_SUCCESS)
      {
         return;
      }

      tapi_debug_buffer_print_entry_to_console(entry, true);
   }
}


/**
 * Convert entry type name enum value into string
 * 
 * \param type Entry type enum value
 * \return const IFX_char_t* Human readable short representation
 */
const IFX_char_t *tapi_debug_buffer_get_entry_type_name(
                                             enum TapiDebugBufferEntryType type)
{
   IFX_uint8_t i = 0;
   for (i = 0; i < TAPI_DBUF_ENTRY_TYPE_LAST; ++i)
   {
      if (type == DebugBufferTypeNames[i].type)
         return DebugBufferTypeNames[i].name;
   }

   return "??";
}


/* TAPI debug buffer access via ioctls */

/**
 * Copy TAPI debug buffer content to user space area via IOCTL
 * 
 * \param[in,out] buffer_info Pointer to information about memory area debug buffer will
 *                    be copied into.
 *                    Number of copied debug buffer entries will be written
 *                    into copied_entries member.
 * \return IFX_int32_t TAPI_statusParam in case of invalid input params
 *                     TAPI_statusOk in case of successful copy
 */
IFX_int32_t tapi_debug_buffer_ioctl_get_buffer(struct TapiDebugBufferContent *buffer_info)
{
   const IFX_size_t kernel_buffer_size = DebugBuffer.capacity *
                                         sizeof(struct TapiDebugBufferEntry);
   const IFX_size_t written_entries_size = DebugBuffer.written_entries *
                                           sizeof(struct TapiDebugBufferEntry);
   IFX_size_t user_data_size_to_copy = 0;

   if (buffer_info == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("%s: Input parameter is null!\n", __FUNCTION__));
      return TAPI_statusParam;
   }

   if (buffer_info->data == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("%s: Pointer given to debug buffer is null!\n", __FUNCTION__));
      return TAPI_statusParam;
   }

   if (buffer_info->data_length == 0)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("%s: Given data length of debug buffer is zero!\n", __FUNCTION__));
      return TAPI_statusParam;
   }

   user_data_size_to_copy = buffer_info->data_length;

   /* Limit data size to copy to avoid copying internal kernel data */
   if (user_data_size_to_copy > kernel_buffer_size)
   {
      user_data_size_to_copy = kernel_buffer_size;
   }

   /* Limit data copying to only already used entries (do not copy zeroed entires)*/
   if (user_data_size_to_copy > written_entries_size)
   {
      user_data_size_to_copy = written_entries_size;
   }

   buffer_info->copied_entries = user_data_size_to_copy / sizeof(struct TapiDebugBufferEntry);
   TAPI_OS_CpyKern2Usr(buffer_info->data, DebugBuffer.buffer, user_data_size_to_copy);

   return TAPI_statusOk;
}


/**
 * Get current TAPI debug buffer configuration via IOCTL
 * 
 * \param config Pointer to structure with debug buffer configuration
 * \return IFX_int32_t TAPI_statusParam in case of invalid input params
 *                     TAPI_statusOk in case of successful operation
 */
IFX_int32_t tapi_debug_buffer_ioctl_get_cfg(struct TapiDebugBufferConfig *config)
{
   if (config == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("%s: Input parameter is null!\n", __FUNCTION__));
      return TAPI_statusParam;
   }

   TAPI_OS_MutexGet(&DebugBuffer.cfg_lock);
   config->overwrite = DebugBuffer.overwrite;
   config->print_to_console = DebugBuffer.print_to_console;
   config->paused = DebugBuffer.paused;
   TAPI_OS_MutexRelease(&DebugBuffer.cfg_lock);

   return TAPI_statusOk;
}


/**
 * Set current TAPI debug buffer configuration via IOCTL
 * 
 * \param config Pointer to structure with debug buffer configuration
 * \return IFX_int32_t TAPI_statusParam in case of invalid input params
 *                     TAPI_statusOk in case of successful operation
 */
IFX_int32_t tapi_debug_buffer_ioctl_set_cfg(const struct TapiDebugBufferConfig *config)
{
   if (config == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("%s: Input parameter is null!\n", __FUNCTION__));
      return TAPI_statusParam;
   }

   TAPI_OS_MutexGet(&DebugBuffer.cfg_lock);
   DebugBuffer.overwrite = config->overwrite;
   DebugBuffer.print_to_console = config->print_to_console;
   DebugBuffer.paused = config->paused;
   TAPI_OS_MutexRelease(&DebugBuffer.cfg_lock);

   return TAPI_statusOk;
}

#endif /* TAPI_FEAT_DEBUG_BUFFER */
