#ifndef _DRV_DXS_IO_TYPES_H
#define _DRV_DXS_IO_TYPES_H
/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016          Intel Corporation.
  Copyright 2022          MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_io_types.h
   This file contains the type definitions specific to the DUSLIC XS
   driver interface and is used by applications.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include "drv_tapi_if_version.h"

#include "ifx_types.h"

#ifdef TAPI_VERSION3
   #include "lib_bbd.h"
   #undef TAPI_ONE_DEVNODE
   #ifdef TAPI_DXY_DOC
      #define TAPI3_DXY_DOC
   #endif /* TAPI_DXY_DOC */
#endif /* TAPI_VERSION3 */

#ifdef TAPI_VERSION4
   #ifndef TAPI_ONE_DEVNODE
      #define TAPI_ONE_DEVNODE
   #endif /* TAPI_ONE_DEVNODE */
   #ifdef TAPI_DXY_DOC
      #define TAPI4_DXY_DOC
   #endif /* TAPI_DXY_DOC */
#endif /* TAPI_VERSION4 */

#if defined (TAPI_VERSION3) && defined (TAPI_VERSION4)
   #error only single version can be specified
#endif /* defined (TAPI_VERSION3) && defined (TAPI_VERSION4) */

#if !defined (TAPI_VERSION3) && !defined (TAPI_VERSION4)
   #error Please specify TAPI version
#endif /* !defined (TAPI_VERSION3) && !defined (TAPI_VERSION4) */

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
/** DUSLIC XS Chip Revision */
typedef enum
{
   /** Engineering Samples */
   DXS_ES         = 0x09,
   /** Version V1.1 */
   DXS_V11        = 0x0A,
   /** Version V1.2 2-channel */
   DXS2_V12       = 0x0B,
   /** Version V1.2 1-channel */
   DXS1_V12       = 0x0C
} DXS_IO_CHIP_REVISION_t;

/* for compatibility with older applications */
#define DXS_V12 DXS2_V12

/** DUSLIC XS chip types */
typedef enum
{
   /** DUSLIC XS chip in VQFN68 package with 2 x FXS */
   DXS_TYPE_VQFN68_TWO_FXS = 0,
   /** DUSLIC XS chip in VQFN68 package with 1 x FXS */
   DXS_TYPE_VQFN68_ONE_FXS = 1,
   /** DUSLIC XS chip in VQFN48 package with 1 x FXS */
   DXS_TYPE_VQFN48_ONE_FXS = 5,
   /** DUSLIC XC chip in VQFN68 package with 2 x FXS */
   DXC_TYPE_VQFN68_TWO_FXS = 2,
   /** DUSLIC XC chip in VQFN48 package with 1 x FXS */
   DXC_TYPE_VQFN48_ONE_FXS = 7
} DXS_IO_CHIP_TYPE_t;

/** \addtogroup DXS_DRIVER_INTERFACE_INIT*/
/** @{ */

/** DUSLIC XS Basic Device Initialization structure */
typedef struct
{
   /** Device index. */
   IFX_uint16_t dev;
   /**
      DUSLIC XS device irq number, as defined by the OS.
      \remark
      If the value -1 is used, the device will be configured for
      polling mode.
   */
   IFX_int32_t nIrqNum;
   /**
      Clock fail irq (optional) -- reserved for later use
      \remark
      The value should be set to 0
   */
   IFX_int32_t nCfIrqNum;
} DXS_BasicDeviceInit_t;
/** @} */

/** \addtogroup DXS_DRIVER_INTERFACE_BASIC
 @{ */

/* ============================= */
/* Version Request               */
/* ============================= */

/** Version Io structure */
typedef struct
{
   /** Device index - input parameter. */
   IFX_uint16_t dev;
   /** number of supported FXS channels */
   IFX_uint8_t  nFxsCh;
   /** number of supported FXO channels */
   IFX_uint8_t  nFxoCh;
   /** chip revision */
   IFX_uint16_t nHwRev;
   /** asdsp revision */
   IFX_uint16_t nAsdspRev;
   /** FW revision */
   IFX_uint32_t nFwRev;
   /** Device ID */
   IFX_uint16_t nDevID;
   /** driver version */
   IFX_uint32_t nDrvVers;
} DXS_IO_Version_t;

/* ============================= */
/* Initialization                */
/* ============================= */

/** structure used for device initialization
 */
typedef struct
{
   /** Device index. */
   IFX_uint16_t dev;
   /** Firmware PRAM pointer or NULL if not needed. */
   IFX_uint8_t    *pPRAMfw ;
   /** size of PRAM firmware in bytes */
   IFX_uint32_t   pram_size;
   /** pointer to block based download format data */
   IFX_uint8_t    *pBBDbuf ;
   /** size of block based download buffer */
   IFX_uint32_t   bbd_size;
   /**
      Flags for initialization. Most of the flags are only used from
      experts to modify the default initialization.

      \arg NO_FW_DWLD      avoid firmware download
      \arg NO_ASDSP_DWLD   avoid ASDSP patch download
   */
   IFX_uint32_t   nFlags;
} DXS_IO_Init_t;

#ifdef TAPI_ONE_DEVNODE
/**
   Structure used for DUSLIC XS bbd download.
*/
typedef struct
{
   /** Device index. */
   IFX_uint16_t dev;
   /** Channel 'module' index. */
   IFX_uint16_t ch;
   /** If IFX_TRUE distribute the BBD on all available channels,
     otherwise the ch field addresses the channel */
   IFX_uint8_t bBroadcast;
   /** block based download buffer,
       big-endian aligned */
   IFX_uint8_t *buf;
   /** size of buffer in bytes */
   IFX_uint32_t size;
} DXS_BBD_Download_t;
#else /* TAPI_ONE_DEVNODE */
/**
   Structure used for DUSLIC XS bbd download.
*/
#define DXS_BBD_Download_t                bbd_format_t
#endif /* TAPI_ONE_DEVNODE */

#ifdef TAPI_ONE_DEVNODE
/**
   Structure used for FIO_DXS_DEV_RESET argument.
*/
typedef struct
{
   /** Device index. */
   IFX_uint16_t dev;
} DXS_DevReset_t;

/**
   Structure used for FIO_DXS_CHIP_RESET argument.
*/
typedef struct
{
   /** Device index. */
   IFX_uint16_t dev;
   /** set 0 (deactivate reset) or 1 (activate reset) */
   IFX_uint16_t nReset;
} DXS_ChipReset_t;
#else /* TAPI_ONE_DEVNODE */
/**
   FIO_DXS_CHIP_RESET argument, set 0 (deactivate reset) or 1 (activate reset).
*/
#define DXS_ChipReset_t                   IFX_uint16_t
#endif /* TAPI_ONE_DEVNODE */

#ifdef TAPI_ONE_DEVNODE
/**
   Structure used for Test Chip Access.
*/
typedef struct
{
   /** Device index. */
   IFX_uint16_t dev;
   /** Maximum test value ( <= 0xFFFF ) */
   IFX_uint16_t max_val;
} DXS_TCA_t;
#else
#define DXS_TCA_t                         IFX_uint16_t
#endif /* TAPI_ONE_DEVNODE */

/**
   Structure used for DUSLIC XS firmware download.
*/
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
   /** valid Firmware byte pointer */
   IFX_uint8_t    *pPRAMfw;
   /** size of firmware in bytes */
   IFX_uint32_t   pram_size;
} DXS_FW_Download_t;

/* ============================= */
/* Basic Access                  */
/* ============================= */
/**
   IO structure for write and read chip commands for debugging
   purposes only.
*/
typedef struct
{
   /** Device index. */
   IFX_uint16_t dev;
   /** command 1 according users manual */
   IFX_uint16_t cmd1;
   /** command 2 according users manual */
   IFX_uint16_t cmd2;
   /** read or write data */
   IFX_uint16_t pData[32];
} DXS_IO_MB_CMD_t;

/* Size in 16-bit words of DXS_IO_MB_CMD_t commands and data, without device index */
#define DXS_IO_MB_CMD_SIZE ((sizeof(DXS_IO_MB_CMD_t) / sizeof(IFX_uint16_t)) - 1)

/** IO structure used for direct register access. */
typedef struct
{
   /** Device index. */
   IFX_uint16_t dev;
   /** offset to host register */
   IFX_uint16_t offset;
   /** number of bytes to read/write (one register => count = 2) */
   IFX_uint16_t count;
   /** contains written/read data */
   IFX_uint16_t pData[32];
} DXS_IO_RegAccess_t;
/** @} */

#endif /* _DRV_DXS_IO_TYPES_H */
