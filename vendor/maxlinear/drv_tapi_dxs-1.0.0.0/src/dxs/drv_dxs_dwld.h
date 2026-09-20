#ifndef _DRV_DXS_DWLD_H
#define _DRV_DXS_DWLD_H
/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016 Intel Corporation.
  Copyright 2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_dwld.h
   This file contains the declaration of the download and CRC structures,
   macros and functions.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include <drv_tapi_config.h>

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
IFX_int32_t DXS_DwldFirmwareSelect(DXS_DEVICE_t *pDev,
                                   const DXS_FW_Download_t *pFwDwld);
IFX_int32_t DXS_DwldAndStartFW(DXS_DEVICE_t *pDev, DXS_FW_Download_t *pEdsp);

#ifdef TAPI_FEAT_LX_COMPAT
   IFX_int32_t DXS_DwldAndStartFW_32(DXS_DEVICE_t *pDev,
                                     DXS_FW_Download_32_t *pEdsp);
#endif

#endif /* _DRV_DXS_DWLD_H */
