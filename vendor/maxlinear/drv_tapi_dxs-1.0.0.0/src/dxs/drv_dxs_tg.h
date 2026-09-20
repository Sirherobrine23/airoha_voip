#ifndef _DRV_DXS_TG_H
#define _DRV_DXS_TG_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_tg.h
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/**
   Represents the maximum number of frequencies allowed to be active
   simultaneously in each cadence of a simple tone
*/
#define DXS_TG_MAX_FREQ             2

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
/** Starts playing out a tone with the tone generators. */
extern IFX_int32_t  DXS_TAPI_LL_ALM_TG_Play(
                    IFX_TAPI_LL_CH_t *pLLChannel,
                    IFX_uint8_t res,
                    IFX_TAPI_TONE_SIMPLE_t const *pToneSimple);

/** Next step in playing out a tone with the tone generators. */
extern IFX_int32_t DXS_TAPI_LL_ALM_TG_Step(
                   IFX_TAPI_LL_CH_t *pLLChannel,
                   IFX_TAPI_TONE_SIMPLE_t const *pTone,
                   IFX_uint8_t res,
                   IFX_uint8_t *nToneStep);

/** Stops playing out a tone with the tone generators. */
extern IFX_int32_t  DXS_TAPI_LL_ALM_TG_Stop(
                    IFX_TAPI_LL_CH_t *pLLChannel,
                    IFX_uint8_t res);

#endif /* _DRV_DXS_TG_H */
