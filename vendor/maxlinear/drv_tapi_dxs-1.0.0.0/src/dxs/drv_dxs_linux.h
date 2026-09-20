#ifndef _DRV_DXS_LINUX_H
#define _DRV_DXS_LINUX_H
/******************************************************************************

  Copyright 2023 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_linux.h
   This file contains the declaration of DXS driver linux specific implementation.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

#ifdef TAPI_LINUX_KERNEL_SPACE
   #define GPIO_RESET_INTERVAL_MIN_VALUE  5
   #define GPIO_RESET_INTERVAL_MAX_VALUE  500
#endif /* TAPI_LINUX_KERNEL_SPACE */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

IFX_int32_t DXS_WaitForCmdMbxData(DXS_DEVICE_t *pDev);
IFX_int32_t DXS_WaitForSddOpmodeChEvt(DXS_CHANNEL_t *pCh);

#endif /* _DRV_DXS_IRQ_H */
