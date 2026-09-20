#ifndef _DRV_DXS_MBX_H
#define _DRV_DXS_MBX_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_mbx.h
   Mailbox functions declarations.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

IFX_int32_t DXS_CmdWrite(DXS_DEVICE_t *pDev,
                         IFX_uint32_t *pCmd);

IFX_int32_t DXS_CmdRead(DXS_DEVICE_t *pDev,
                        IFX_uint32_t *pCmd,
                        IFX_uint32_t *pData);

IFX_int32_t DXS_DwldPatch(DXS_DEVICE_t *pDev,
                          IFX_uint8_t *pBuffer,
                          IFX_uint32_t nSize);

IFX_int32_t DXS_ObxRead(DXS_DEVICE_t *pDev,
                        IFX_uint32_t *pData,
                        IFX_uint8_t *length);

#endif /* _DRV_DXS_MBX_H */
