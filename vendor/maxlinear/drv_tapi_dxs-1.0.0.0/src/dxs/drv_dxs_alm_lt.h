#ifndef _DRV_DXS_ALM_LT_H
#define _DRV_DXS_ALM_LT_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_alm_lt.h
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
#ifdef DXS_FEAT_GR909
extern IFX_int32_t DXS_TAPI_LL_ALM_LT_GR909_Start (IFX_TAPI_LL_CH_t *pLLChannel,
                                     IFX_TAPI_GR909_START_t const *pGR909Start);
extern IFX_int32_t DXS_TAPI_LL_ALM_LT_GR909_Stop (IFX_TAPI_LL_CH_t *pLLChannel);
extern IFX_int32_t DXS_TAPI_LL_ALM_LT_GR909_Result (IFX_TAPI_LL_CH_t *pLLChannel,
                                         IFX_TAPI_GR909_RESULT_t *pGR909Result);
extern IFX_int32_t DXS_TAPI_LL_ALM_NLT_RmesConfig_Set(
                                    IFX_TAPI_LL_CH_t *pLLChannel,
                                    const IFX_TAPI_NLT_CONFIGURATION_RMES_t *pConfig);
#endif /* DXS_FEAT_GR909 */
#ifdef DXS_FEAT_CAPACITANCE_MEASUREMENT
extern IFX_int32_t DXS_TAPI_LL_ALM_LT_CheckCapMeasSup (
                                                IFX_TAPI_LL_CH_t *pLLChannel,
                                                IFX_boolean_t *pSupported);
extern IFX_int32_t DXS_TAPI_LL_ALM_LT_CapMeasStart (
                                                IFX_TAPI_LL_CH_t *pLLChannel,
                                                IFX_boolean_t bTip2RingOnly);
extern IFX_int32_t DXS_TAPI_LL_ALM_LT_CapMeasStop (
                                                  IFX_TAPI_LL_CH_t *pLLChannel);
extern IFX_int32_t DXS_TAPI_LL_ALM_LT_CapMeasResult (
                                                  IFX_TAPI_LL_CH_t *pLLChannel);
extern IFX_int32_t DXS_TAPI_LL_ALM_NLT_Cap_Result(IFX_TAPI_LL_CH_t *pLLChannel,
                                    IFX_TAPI_NLT_CAPACITANCE_RESULT_t *pResult);
#endif /* DXS_FEAT_CAPACITANCE_MEASUREMENT */
/* temporary solution. todo - create additional flag for this case */
#if (defined (DXS_FEAT_GR909) || defined(DXS_FEAT_CAPACITANCE_MEASUREMENT))
extern IFX_int32_t DXS_TAPI_LL_ALM_NLT_OLConfig_Set(
                                      IFX_TAPI_LL_CH_t *pLLChannel,
                                      const IFX_TAPI_NLT_CONFIGURATION_OL_t *pConfig);
extern IFX_int32_t DXS_TAPI_LL_ALM_NLT_OLConfig_Get(
                                      IFX_TAPI_LL_CH_t *pLLChannel,
                                      IFX_TAPI_NLT_CONFIGURATION_OL_t *pConfig);
#endif /* defined (DXS_FEAT_GR909) || defined(DXS_FEAT_CAPACITANCE_MEASUREMENT) */
extern IFX_int32_t  DXS_ALM_GR909_SetLimits (DXS_CHANNEL_t *pCh,
                                      IFX_TAPI_NLT_RMEAS_CFG_t nRmeas);

#endif /* _DRV_DXS_ALM_LT_H */
