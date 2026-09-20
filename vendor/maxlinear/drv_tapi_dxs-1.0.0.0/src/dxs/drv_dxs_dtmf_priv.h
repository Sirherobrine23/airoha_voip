#ifndef _DRV_DXS_DTMF_PRIV_H
#define _DRV_DXS_DTMF_PRIV_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_dtmf_priv.h
   This file contains the defines, the structures declarations of dtmf/at
   module of DUSLIC XS.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_dtmf.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/* DTMF max digt/inter digit time */
#define DXS_DTMF_MAX_DIGIT_TIME        127 /* ms */
#define DXS_DTMF_MAX_INTERDIGIT_TIME   127 /* ms */

#define DXS_DTMF_STATE_NO_STATES       3

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
/**
   state handler states of DTMF transmission
*/
typedef enum
{
   /* DTMF generator idle */
   DXS_DTMF_STATE_SETUP,
   /* DTMF generator transmitting */
   DXS_DTMF_STATE_TRANSMIT,
   /* DTMF transmission pause (interdigit time) */
   DXS_DTMF_STATE_PAUSE
} DXS_DTMF_STATES_t;

/**
   structure for DTMF transmission
*/
struct DXS_DTMF_SEND
{
   /* DTMF transmission state */
   DXS_DTMF_STATES_t          state_dtmf;
   /* callback function called on DTMF status changes */
   IFX_void_t                 (*stateCb)(DXS_CHANNEL_t *pCh);
   /* array to store the table indices for DTMF digits */
   IFX_uint8_t                pDtmfDigTblIndex[DXS_DTMF_MAX_BYTES];
   /* number of DTMF bytes to send */
   IFX_uint8_t                nDtmfCnt;
   /* number of DTMF bytes that already have been transmitted */
   IFX_uint8_t                nSent;
   /* substate of DTMF_TRANSMIT: DTMF digit is played out pause between digits */
   IFX_boolean_t              bPause;
   /** time for DTMF digit duration. Default is 50 ms. */
   IFX_uint16_t               digitTime;
   /** time between DTMF digits in ms. Default is 50 ms. */
   IFX_uint16_t               interDigitTime;
   /* Timer for the controlled transmission of DTMF tones */
   Timer_ID                   dtmfTimerId;
};

/**
   structure for DTMF detection parameters
*/
struct DXS_DTMF_REC
{
   /* minimum signal level for dtmf detection */
   IFX_uint16_t   level;
   /* maximum allowed signal twist */
   IFX_uint16_t   twist;
};

/**
   structure for the DMTF/AT firmware messages cache
*/
struct DXS_DTMF_AT
{
   DXS_DTMF_AT_GEN_CTRL_t     dtmf_at_gen_ctrl;
   DXS_DTMF_AT_GEN_COEF_t     dtmf_at_gen_coef;
   DXS_DTMF_AT_GEN_COEF_t     dtmf_at_gen_coef_written;
   DXS_DTMF_REC_CTRL_t        dtmf_rec_ctrl;
   DXS_DTMF_REC_COEF_t        dtmf_rec_coef;
   /* DTMF sender structure */
   struct DXS_DTMF_SEND       dtmfSend;
   /* DTMF receiver structure */
   struct DXS_DTMF_REC        dtmfReceive;
};

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

#endif /* _DRV_DXS_DTMF_PRIV_H */
