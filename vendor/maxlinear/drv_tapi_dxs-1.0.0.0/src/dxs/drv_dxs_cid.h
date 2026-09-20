#ifndef _DRV_DXS_CID_H
#define _DRV_DXS_CID_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_cid.h
   This file contains the declaration of the functions for CID operations.
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
#ifdef DXS_FEAT_CID

/** CID state machine events */
typedef enum
{
   /* initiate callerID sequence */
   DXS_CID_EVT_START,
   /* callerID data request interrupt */
   DXS_CID_EVT_CIS_REQ,
   /* callerID data buffer underrun interrupt */
   DXS_CID_EVT_CIS_BUF,
   /* offhook interrupt */
   DXS_CID_EVT_OFFHOOK
} DXS_CID_EVT_t;

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
extern IFX_int32_t DXS_CID_CidGenCtrl (DXS_CHANNEL_t *pCh,
                                       IFX_boolean_t bEn);

extern IFX_int32_t DXS_CID_SH (DXS_CHANNEL_t *pCh, DXS_CID_EVT_t evt);

extern IFX_int32_t DXS_TAPI_LL_CID_TX_Start (IFX_TAPI_LL_CH_t *pLLChannel,
                                             IFX_TAPI_CID_TX_t const *pCidData);

extern IFX_int32_t DXS_TAPI_LL_CID_TX_Stop (IFX_TAPI_LL_CH_t *pLLChannel);

extern IFX_int32_t DXS_CID_Allocate_Ch_Structures (DXS_CHANNEL_t *pCh);

extern IFX_void_t  DXS_CID_Free_Ch_Structures (DXS_CHANNEL_t *pCh);

extern IFX_void_t  DXS_CID_InitCh (DXS_CHANNEL_t *pCh);

extern IFX_int8_t  DXS_CID_GetRemainingBytes (DXS_CHANNEL_t *pCh);

#endif /* DXS_FEAT_CID */
#endif /* _DRV_DXS_CID_H */
