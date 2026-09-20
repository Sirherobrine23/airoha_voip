#ifndef _DRV_DXS_PCM_H
#define _DRV_DXS_PCM_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_pcm.h
   This file contains the defines and the global function declarations for
   the PCM highway.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "../tapi/drv_tapi_ll_interface.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/* PCM timeslot allocation management definitions */
#define DXS_PCM_MAX_TS              127
#define DXS_PCM_TS_ARRAY            ((DXS_PCM_MAX_TS + 3) / 32)

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
extern IFX_void_t   DXS_PCM_Func_Register(
                    IFX_TAPI_DRV_CTX_PCM_t *pPCM);

extern IFX_int32_t  DXS_PCM_IF_Stop (DXS_DEVICE_t *pDev);

extern IFX_int32_t  DXS_PCM_Allocate_Ch_Structures (DXS_CHANNEL_t *pCh);

extern IFX_void_t   DXS_PCM_Free_Ch_Structures (DXS_CHANNEL_t *pCh);

extern IFX_void_t   DXS_PCM_InitCh (DXS_CHANNEL_t *pCh);

extern IFX_int32_t  DXS_PCM_ChStop (DXS_CHANNEL_t *pCh);

extern IFX_int32_t  DXS_PCM_ChRxMute (DXS_CHANNEL_t *pCh, IFX_enDis_t nMute);
#endif /* _DRV_DXS_PCM_H */
