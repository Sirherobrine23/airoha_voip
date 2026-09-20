#ifndef _DRV_DXS_DTMF_H
#define _DRV_DXS_DTMF_H
/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016, Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_dtmf.h
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/* minimum level for dtmf/at is -20dB */
#define DXS_DTMF_AT_LEVEL_MIN          -30

#define DXS_DTMF_MAX_BYTES             64

/**
   Represents the minimal allowed for programming the DTMF Receiver Level
*/
#define DXS_DTMF_RX_LEVEL_MIN         -40

/**
   Represents the maximal allowed value for programming the DTMF Receiver Level
*/
#define DXS_DTMF_RX_LEVEL_MAX         -6

/**
   Represents the minimal value represented by table DXS_DtmfRxTwist[]
   which can be used to program the DTMF Receiver Twist
*/
#define DXS_DTMF_RX_TWIST_MIN         1

/**
   Represents the maximum value represented by table DXS_DtmfRxTwist[]
   which can be used to program the DTMF Receiver Gain
*/
#define DXS_DTMF_RX_TWIST_MAX         12

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
/**
   DTMF state machine events
*/
typedef enum
{
   /* start of transmission of dtmf digits */
   DXS_DTMF_EVT_START,
   /* timer for digit or interdigit time expires */
   DXS_DTMF_EVT_TIMEOUT,
   /* all digits have been transmitted */
   DXS_DTMF_EVT_COMPLETE,
   /* stop dtmf transmission immediately */
   DXS_DTMF_EVT_STOP
} DXS_DTMF_EVT_t;

/**
   enum for activation or deactivation of the dtmf/at generator
*/
typedef enum
{
   /* disable dtmf/at generator */
   DXS_DTMF_AT_CTRL_DIS = 0,
   /* enable dtmf/at generator */
   DXS_DTMF_AT_CTRL_EN
} DXS_DTMF_AT_CTRL_t;

/**
   enum how to mix the DTMF/AT generator signal with the PCM path:
   - adding
   - adding with voice muting
*/
typedef enum
{
   /** dtmf/at outuput is added to pcm rx path */
   DXS_DTMF_AT_ADD_DTMF_AT,
   /** dtmf/at output is fed, voice is muted */
   DXS_DTMF_AT_ADD_DTMF_AT_MUTE
} DXS_DTMF_AT_ADD_t;

/**
   structure to store parameters for DTMF/AT
*/
struct DXS_DTMF_AT_CFG
{
   /* frequency A */
   IFX_uint32_t         nFreq1;
   /* frequency B */
   IFX_uint32_t         nFreq2;
   /* level for frequency A */
   IFX_int32_t          nLevel1;
   /* level for frequency B */
   IFX_int32_t          nLevel2;
   DXS_DTMF_AT_CTRL_t   nEnDis;
   DXS_DTMF_AT_ADD_t    nAdd;
};

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
extern IFX_int32_t DXS_DTMF_SH (DXS_CHANNEL_t *pCh, DXS_DTMF_EVT_t evt);

IFX_void_t DXS_DTMF_GetTblIndex (IFX_char_t nChar, IFX_uint8_t *pDtmfCode);

extern IFX_uint8_t   DXS_DTMF_encode_fw2tapi   (IFX_uint8_t fwDtmfCode);
extern IFX_char_t    DXS_DTMF_encode_fw2ascii  (IFX_uint8_t fwDtmfCode);

extern IFX_return_t DXS_TAPI_LL_DTMF_RX_CFG(IFX_TAPI_LL_CH_t *pLLChannel,
                                                IFX_boolean_t bRW,
                                                IFX_TAPI_DTMF_RX_CFG_t *pCoeff);

extern IFX_int32_t  DXS_DTMF_AT_Allocate_Ch_Structures (DXS_CHANNEL_t *pCh);

extern IFX_void_t   DXS_DTMF_AT_Free_Ch_Structures (DXS_CHANNEL_t *pCh);

extern IFX_void_t   DXS_DTMF_AT_InitCh (DXS_CHANNEL_t *pCh);

extern IFX_int32_t DXS_DTMF_AT_CTRL(DXS_CHANNEL_t *pCh,
                                    const struct DXS_DTMF_AT_CFG *pDtmfAt);

extern IFX_int32_t DXS_DTMF_REC_CTRL (DXS_CHANNEL_t *pCh, IFX_boolean_t bEn);

extern IFX_void_t DXS_TCB_DTMF (Timer_ID Timer, IFX_ulong_t arg);
#endif /* _DRV_DXS_DTMF_H */
