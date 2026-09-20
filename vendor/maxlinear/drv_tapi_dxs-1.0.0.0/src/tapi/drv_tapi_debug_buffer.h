#ifndef _DRV_TAPI_DEBUG_BUFFER_H
#define _DRV_TAPI_DEBUG_BUFFER_H
/******************************************************************************

  Copyright 2023 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_debug_buffer.h
   This header provide interface to the debug buffer functionality.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include <drv_tapi_io.h>
#include <drv_tapi_osmap.h>
#include <drv_tapi_config.h>

#ifdef TAPI_FEAT_DEBUG_BUFFER

/* ========================================================================== */
/*                       Global Macro Definitions                             */
/* ========================================================================== */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */




/* Structure holding debug buffer status information - read only */
struct TapiDebugBufferStatus
{
   /* True if memory for buffer was allocated */
   IFX_boolean_t initialized;
   /* Buffer capacity */
   IFX_size_t capacity;
   /* If true - new entries will overwrite first ones when buffer becomes full
      (buffer will act as ring buffer) otherwise new entries will be discarded */
   IFX_boolean_t overwrite;
   /* Number of written entries to buffer (when buffer works as ring buffer
      this number can be higher than buffer capacity) */
   IFX_ulong_t written_entries;
   /* Print new entries to console */
   IFX_boolean_t print_to_console;
   /* Buffering is paused - if true, no new entries are added to buffer */
   IFX_boolean_t paused;
};

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

/* Init, free buffer */

IFX_int32_t tapi_debug_buffer_init(IFX_void_t);
IFX_void_t tapi_debug_buffer_free(IFX_void_t);

/* Getters - status, single info*/

IFX_int32_t tapi_debug_buffer_get_status_info(struct TapiDebugBufferStatus *status);
const IFX_size_t tapi_debug_buffer_get_capacity(IFX_void_t);
const IFX_size_t tapi_debug_buffer_get_num_of_written_entries(IFX_void_t);
const IFX_size_t tapi_debug_buffer_get_newest_entry_index(IFX_boolean_t *rolled_over);
IFX_int32_t tapi_debug_buffer_get_config(struct TapiDebugBufferConfig *config);
IFX_int32_t tapi_debug_buffer_get_entry(IFX_size_t index,
                                        const struct TapiDebugBufferEntry **entry);
const IFX_char_t *tapi_debug_buffer_get_entry_type_name(
                                             enum TapiDebugBufferEntryType type);

/* Setters - config */

IFX_int32_t tapi_debug_buffer_set_config(const struct TapiDebugBufferConfig *config);

/* Add entry, dump/print content to console helpers... */

IFX_int32_t tapi_debug_buffer_add_entry(IFX_uint16_t dev_number,
                                         IFX_uint16_t ch_number,
                                         enum TapiDebugBufferEntryType type,
                                         const IFX_uint8_t *const data,
                                         IFX_uint16_t data_length);
IFX_int32_t tapi_debug_buffer_add_reg_entry(IFX_uint16_t dev_number,
                                            IFX_uint16_t ch_number,
                                            enum TapiDebugBufferEntryType type,
                                            const IFX_uint8_t reg_number,
                                            const IFX_uint8_t *const data,
                                            IFX_uint16_t data_length);
IFX_int32_t tapi_debug_buffer_add_user_entry(const char *user_info, ...);
IFX_void_t tapi_debug_buffer_dump_buffer_to_console(IFX_size_t num_of_last_entries);

/* TAPI debug buffer access via ioctls */

IFX_int32_t tapi_debug_buffer_ioctl_get_buffer(struct TapiDebugBufferContent *buffer_info);
IFX_int32_t tapi_debug_buffer_ioctl_get_cfg(struct TapiDebugBufferConfig *config);
IFX_int32_t tapi_debug_buffer_ioctl_set_cfg(const struct TapiDebugBufferConfig *config);

#else /* for disabled TAPI_FEAT_DEBUG_BUFFER use dummy macros */

/* Dummy add entry macro to be used when TAPI debug buffer feature is disabled */
#define tapi_debug_buffer_add_reg_entry(dev_number, ch_number, type, reg_number, data, data_length) \
   do { /* noop */} while (0);

/* Dummy add entry macro to be used when TAPI debug buffer feature is disabled */
#define tapi_debug_buffer_add_entry(dev_number, ch_number, type, data, data_length) \
   do { /* noop */} while (0);

/* Dummy add entry macro to be used when TAPI debug buffer feature is disabled */
#define tapi_debug_buffer_add_user_entry(user_info, ...) \
   do { /* noop */} while (0);

#endif /* TAPI_FEAT_DEBUG_BUFFER */
#endif /* _DRV_TAPI_DEBUG_BUFFER_H */
