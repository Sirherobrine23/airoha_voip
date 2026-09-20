#ifndef _DRV_DXS_ACCESS_H
#define _DRV_DXS_ACCESS_H
/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_access.h
   Low level access macros and functions declarations.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include <drv_tapi_config.h>
#include "drv_dxs.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/** \defgroup DXS_MBX_ACCESS_PROTECTION Mailbox access protection macros */
/*@{*/

/** Protects host mailbox access.
\param  pDev - handle to device
\remark Protection is done against concurrent tasks and interrupts
*/
#define DXS_HOST_PROTECT(pDev) \
   do{\
         TAPI_OS_MutexGet(&(pDev)->mtxMbxAcc);\
   } while(0)

/** Releases host mailbox access protection
\param  pDev - handle to device
*/
#define DXS_HOST_RELEASE(pDev) \
   do{\
         TAPI_OS_MutexRelease(&(pDev)->mtxMbxAcc);\
   } while(0)

/*@}*/

/** Protects SPI access.
\param  pDev - handle to device
*/
#define DXS_SPI_PROTECT(pDev) \
   do{\
         TAPI_OS_MutexGet(&(pDev)->mtxSpiAcc);\
   } while(0)

/** Releases SPI access protection
\param  pDev - handle to device
*/
#define DXS_SPI_RELEASE(pDev) \
   do{\
         TAPI_OS_MutexRelease(&(pDev)->mtxSpiAcc);\
   } while(0)


/** Protect firmware download.
\param  pDev - handle to device
*/
#define DXS_FW_DL_PROTECT(pDev) \
   do{\
         TAPI_OS_MutexGet(&(pDev)->mtxFwDlAcc);\
   } while(0)

/** Releases firmware download protection.
\param  pDev - handle to device
*/
#define DXS_FW_DL_RELEASE(pDev) \
   do{\
         TAPI_OS_MutexRelease(&(pDev)->mtxFwDlAcc);\
   } while(0)


/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
extern IFX_void_t dxs_init_spi (
                        DXS_DEVICE_t *pDev);

extern IFX_void_t dxs_exit_spi (
                        DXS_DEVICE_t *pDev);

extern IFX_int32_t DXS_RegWrite(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t nValue);

extern IFX_int32_t DXS_RegRead(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t *pValue);

extern IFX_int32_t DXS_RegWriteMulti(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t *pValue,
                        IFX_uint8_t count);

extern IFX_int32_t DXS_RegReadMulti(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t *pValue,
                        IFX_uint8_t count);

extern IFX_int32_t DXS_reg_access_test(
                        DXS_DEVICE_t *pDev);

extern void DXS_cpw2b (
                        IFX_uint8_t *pBbuf,
                        const IFX_uint16_t * const pWbuf,
                        const IFX_uint32_t nWoffset,
                        const IFX_uint32_t nB);

extern void DXS_cpb2w (
                        IFX_uint16_t *pWbuf,
                        const IFX_uint8_t * const pBbuf,
                        IFX_uint32_t nB);

extern void DXS_cpb2dw (
                        IFX_uint32_t *pDWbuf,
                        const IFX_uint32_t nWoffset,
                        const IFX_uint8_t* const pBbuf,
                        const IFX_uint32_t nB);

extern const IFX_uint32_t DXS_spi_blocksize_get(
                        void);

#endif /* _DRV_DXS_ACCESS_H */
