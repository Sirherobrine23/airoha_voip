#ifndef _DRV_DXS_ALM_H
#define _DRV_DXS_ALM_H
/******************************************************************************

  Copyright 2014-2015 Lantiq Deutschland GmbH
  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016, 2020 Intel Corporation.
  Copyright 2021,2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_alm.h
*/
#ifdef DXS_FEAT_NLT
#include "drv_dxs_alm_aclm.h"
#endif

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/**
   Represents the minimal allowed value for programming the ALM Rx relative
   gain, due to TAPI limitation (device allows up to -89dB)
*/
#define DXS_ALM_RX_GAIN_MIN         -24

/**
   Represents the maximum allowed value for programming the ALM Rx relative gain
   0.55 dB rounded to 1 dB
*/
#define DXS_ALM_RX_GAIN_MAX         1

/**
   Represents the minimal allowed value for programming the ALM Tx relative gain
*/
#define DXS_ALM_TX_GAIN_MIN         -4

/**
   Represents the maximum allowed value for programming the ALM Tx relative
   gain due to TAPI limitation (device allows up to 86dB)
*/
#define DXS_ALM_TX_GAIN_MAX         24


#define SDD_EVT_TIMEOUT_MS 100
#define OPMODE_IGNORED 255

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
extern IFX_void_t    DXS_ALM_Func_Register(
                        IFX_TAPI_DRV_CTX_ALM_t *pAlm);

extern IFX_int32_t   DXS_ALM_Allocate_Ch_Structures(
                        DXS_CHANNEL_t *pCh);

extern IFX_void_t    DXS_ALM_Free_Ch_Structures(
                        DXS_CHANNEL_t *pCh);

extern IFX_void_t    DXS_ALM_InitCh(
                        DXS_CHANNEL_t *pCh);

extern IFX_int32_t   DXS_ALM_ChStop(
                        DXS_CHANNEL_t *pCh,
                        IFX_boolean_t bChipAccess);

extern IFX_boolean_t DXS_ALM_CapMeasInProgress(
                        DXS_CHANNEL_t *pCh);

extern IFX_int32_t   DXS_ALM_GR909_SetLimits(
                        DXS_CHANNEL_t *pCh,
                        IFX_TAPI_NLT_RMEAS_CFG_t nRmeas);

extern IFX_uint16_t  DXS_ALM_ElapsedTimeSinceLastHook(
                        DXS_CHANNEL_t *pCh,
                        IFX_uint16_t current_timestamp);

extern IFX_int32_t   DXS_ALM_Calibration (
                        DXS_CHANNEL_t *pCh);

extern IFX_int32_t DXS_ALM_OnLineModeChanged(DXS_CHANNEL_t *pCh,
   IFX_uint32_t nOpmode);

extern IFX_void_t irq_DXS_ALM_UpdateOpModeAndWakeUp (DXS_CHANNEL_t *pCh,
   IFX_uint8_t lm);
extern IFX_int32_t DXS_ALM_OpmodeSet (DXS_CHANNEL_t *pCh);
extern IFX_int32_t DXS_ALM_OpmodeModeSet (DXS_CHANNEL_t *pCh,
                        IFX_uint32_t nOperatingMode);
extern IFX_int32_t DXS_ALM_OpmodeGet (DXS_CHANNEL_t *pCh,
   IFX_uint8_t *pCurrentOpmode);

extern IFX_uint16_t DXS_ALM_rx_level_convert(
                        DXS_CHANNEL_t *pCh,
                        IFX_int32_t level_db);

#endif /* _DRV_DXS_ALM_H */
