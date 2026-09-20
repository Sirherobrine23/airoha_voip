#ifndef _DRV_DXS_ALM_ACLM_H
#define _DRV_DXS_ALM_ACLM_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_alm_aclm.h
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include <drv_tapi_config.h>

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ACLM constants */
#define DXS_ACLM_FR_MEASUREMENTS          35
#define DXS_ACLM_TH_MEASUREMENTS          37
#define DXS_ACLM_GT_MEASUREMENTS          30
#define DXS_ACLM_SNR_MEASUREMENTS         28
#define DXS_ACLM_INT_TIME_MS_DEFAULT      50
#define DXS_ACLM_AC_DELAY_MS_DEFAULT      20
#define DXS_ACLM_FR_TH_FREQ_MIN_HZ        100
#define DXS_ACLM_FR_FREQ_CENTER_HZ        1020
#define DXS_ACLM_FR_TH_FREQ_STEP_HZ       100
#define DXS_ACLM_GT_LEVEL_MIN_DBM0        (-55)
#define DXS_ACLM_SNR_LEVEL_MIN_DBM0       (-51)
#define DXS_ACLM_GT_SNR_LEVEL_STEP_DBM0   2

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

IFX_int32_t DXS_TAPI_LL_ALM_NLT_Test_Start (IFX_TAPI_LL_CH_t *pLLChannel,
                                            const IFX_TAPI_NLT_TEST_START_t *pArg);
IFX_int32_t DXS_TAPI_LL_ALM_NLT_Results_Get (IFX_TAPI_LL_CH_t *pLLChannel,
                                            const IFX_TAPI_NLT_RESULT_GET_t *pArg);
extern IFX_void_t DXS_NLT_Func_Register (IFX_TAPI_DRV_CTX_NLT_t *pNLT);

extern IFX_void_t dxs_aclm_meas_eventWakeUp (DXS_CHANNEL_t *pCh);

#endif /* _DRV_DXS_ALM_ACLM_H */
