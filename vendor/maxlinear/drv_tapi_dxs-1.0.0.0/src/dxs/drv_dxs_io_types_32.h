#ifndef _DRV_DXS_IO_TYPES_32_H
#define _DRV_DXS_IO_TYPES_32_H
/******************************************************************************

  Copyright 2018, Intel Corporation

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_io_types_32.h
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#ifdef TAPI_FEAT_LX_COMPAT
#include <linux/compat.h>
/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

/* TAPI V4 */
#ifdef TAPI_ONE_DEVNODE
typedef struct
{
   /** Device index. */
   IFX_uint16_t dev;
   /** Channel 'module' index. */
   IFX_uint16_t ch;
   /** If IFX_TRUE distribute the BBD on all available channels,
     otherwise the ch field addresses the channel */
   IFX_uint32_t bBroadcast;
   /** Block based download buffer, big-endian aligned.
    *  Pointer value is carried by 4 byte wide type
    *  for alignment with 32 bit userspace */
   compat_uptr_t buf;
   /** size of buffer in bytes */
   IFX_uint32_t size;
} DXS_BBD_Download_32_t;
#else /* TAPI_ONE_DEVNODE */
typedef struct
{
   /** block based download buffer,
       big-endian aligned */
   compat_uptr_t buf;
   /** size of buffer in bytes */
   IFX_uint32_t size;
} bbd_format_32_t;
#define DXS_BBD_Download_32_t          bbd_format_32_t
#endif


typedef struct
{
   /** Device index. */
   IFX_uint16_t dev;
   /** Flag to consider while doing FW download.
       Refer to \ref DXS_IO_Init_t for more details.
      - NO_FW_DWLD
      - NO_ASDSP_DWLD
   */
   IFX_uint32_t   nEdspFlags;
   /** valid Firmware byte pointer
    *  4 byte wide type used for alignment with 32 bit userspace */
   compat_uptr_t  pPRAMfw;
   /** size of firmware in bytes */
   IFX_uint32_t   pram_size;
} DXS_FW_Download_32_t;


/** structure used for device initialization */
typedef struct
{
   /** Device index. */
   IFX_uint16_t dev;
   /** Firmware PRAM pointer or NULL if not needed. */
   compat_uptr_t  pPRAMfw ;
   /** size of PRAM firmware in bytes */
   IFX_uint32_t   pram_size;
   /** pointer to block based download format data */
   compat_uptr_t  pBBDbuf ;
   /** size of block based download buffer */
   IFX_uint32_t   bbd_size;
   /**
      Flags for initialization. Most of the flags are only used from
      experts to modify the default initialization.

      \arg NO_FW_DWLD      avoid firmware download
      \arg NO_ASDSP_DWLD   avoid ASDSP patch download
   */
   IFX_uint32_t   nFlags;
} DXS_IO_Init_32_t;

#endif /* TAPI_FEAT_LX_COMPAT */

#define FIO_DXS_BBD_DOWNLOAD_32 \
   _IOW (DXS_IOC_MAGIC, 202, DXS_BBD_Download_32_t)

#define FIO_DXS_FW_DOWNLOAD_32 \
  _IOW (DXS_IOC_MAGIC, 203, DXS_FW_Download_32_t)

#define FIO_DXS_INIT_32 \
  _IOW (DXS_IOC_MAGIC, 9, DXS_IO_Init_32_t)

#endif /* _DRV_DXS_IO_TYPES_32_H */
