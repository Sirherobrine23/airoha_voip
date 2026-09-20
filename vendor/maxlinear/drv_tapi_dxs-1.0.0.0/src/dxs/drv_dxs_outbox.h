#ifndef _DRV_DXS_OUTBOX_H
#define _DRV_DXS_OUTBOX_H
/******************************************************************************

  Copyright 2024  MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_outbox.h
   Outbox handling functions declarations.
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

IFX_int32_t DXS_outbox_handler(DXS_DEVICE_t *pDev);
IFX_int32_t DXS_outbox_handler_init(DXS_DEVICE_t *pDev);
IFX_int32_t DXS_outbox_handler_exit(DXS_DEVICE_t *pDev);

#endif /* _DRV_DXS_OUTBOX_H */
