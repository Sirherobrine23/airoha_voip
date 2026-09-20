/******************************************************************************

  Copyright 2014 Lantiq Deutschland GmbH
  Copyright 2021 Maxlinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_dtmf_sm.c
   This file contains the implementation of the functions for the DTMF
   state machine.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_dtmf_priv.h"
#include "drv_dxs_pcm_priv.h"
#include "drv_dxs_mbx.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
typedef IFX_int32_t (*DXS_DTMF_STATE_HANDLER) (DXS_CHANNEL_t *pCh,
                                               DXS_DTMF_EVT_t evt);

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_int32_t irq_DXS_DtmfAtDisable(DXS_CHANNEL_t *pCh);
static IFX_int32_t DXS_DTMF_SH_Setup (DXS_CHANNEL_t *pCh, DXS_DTMF_EVT_t evt);
static IFX_int32_t DXS_DTMF_SH_Transmit (DXS_CHANNEL_t *pCh, DXS_DTMF_EVT_t evt);
static IFX_int32_t DXS_DTMF_SH_Pause (DXS_CHANNEL_t *pCh,
                                      DXS_DTMF_EVT_t evt);

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */
static DXS_DTMF_STATE_HANDLER  gDxsDtmfStates[DXS_DTMF_STATE_NO_STATES] =
                              {DXS_DTMF_SH_Setup, DXS_DTMF_SH_Transmit,
                               DXS_DTMF_SH_Pause};


#define DXS_DTMF_MAX_DIGITS    16
static const IFX_int32_t DXS_DtmfDigits [DXS_DTMF_MAX_DIGITS][4] =
{
    /* Freq1/Freq2(Hz)/Level1/Level2(dB)    - Index - DTMF Digit */
    {941, 1336, -11, -9},   /*   941/1336 Hz, -11/-9 dB,   0,       0 */
    {697, 1209, -11, -9},   /*   697/1209 Hz, -11/-9 dB,   1,       1 */
    {697, 1336, -11, -9},   /*   697/1336 Hz, -11/-9 dB,   2,       2 */
    {697, 1477, -11, -9},   /*   697/1477 Hz, -11/-9 dB,   3,       3 */
    {770, 1209, -11, -9},   /*   770/1209 Hz, -11/-9 dB,   4,       4 */
    {770, 1336, -11, -9},   /*   770/1336 Hz, -11/-9 dB,   5,       5 */
    {770, 1477, -11, -9},   /*   770/1477 Hz, -11/-9 dB,   6,       6 */
    {852, 1209, -11, -9},   /*   852/1209 Hz, -11/-9 dB,   7,       7 */
    {852, 1336, -11, -9},   /*   852/1336 Hz, -11/-9 dB,   8,       8 */
    {852, 1477, -11, -9},   /*   852/1477 Hz, -11/-9 dB,   9,       9 */
    {697, 1633, -11, -9},   /*   697/1633 Hz, -11/-9 dB,   10,      A */
    {770, 1633, -11, -9},   /*   770/1633 Hz, -11/-9 dB,   11,      B */
    {852, 1633, -11, -9},   /*   852/1633 Hz, -11/-9 dB,   12,      C */
    {941, 1633, -11, -9},   /*   941/1633 Hz, -11/-9 dB,   13,      D */
    {941, 1209, -11, -9},   /*   941/1209 Hz, -11/-9 dB,   14,      * */
    {941, 1477, -11, -9},   /*   941/1477 Hz, -11/-9 dB,   15,      # */
};

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */
/**
   stop and disable DTMF generator

   \param pCh  channel pointer

   \return
   - DXS_statusOk
   - DXS_statusDtmfGenCtrlErr
*/
static IFX_int32_t irq_DXS_DtmfAtDisable(DXS_CHANNEL_t *pCh)
{
   IFX_int32_t             err = DXS_statusOk;
   DXS_DEVICE_t            *pDev             = (DXS_DEVICE_t *)(pCh->pParent);
   DXS_DTMF_AT_GEN_CTRL_t  *pDtmfAtGenCtrl   = IFX_NULL;
   DXS_PCM_CH_MUTE_t       *pPcmChMute       = IFX_NULL;

   pDtmfAtGenCtrl          = &pCh->pDTMF->dtmf_at_gen_ctrl;
   pPcmChMute              = &pCh->pPCM->pcm_ch_mute;

   /* disable dtmf/at generator and enable voice path */
   pDtmfAtGenCtrl->EN      = DTMF_AT_GEN_CTRL_EN_DIS;

   err = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)pDtmfAtGenCtrl);
   if (!DXS_SUCCESS (err))
   {
      /* errmsg: Activation or deactivation of DTMF generator failed.*/
      RETURN_STATUS (DXS_statusDtmfGenCtrlErr, IFX_NULL);
   }
   pPcmChMute->RX_MUTE = PCM_CH_MUTE_RX_MUTE_NO_MUTE;
   err = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)pPcmChMute);
   if (!DXS_SUCCESS (err))
   {
      /** errmsg: Muting/unmuting the PCM path failed. */
      RETURN_STATUS(DXS_statusPcmMuteErr, IFX_NULL);
   }
   return DXS_statusOk;
}

/**
   dxs dtmf state machine timer callback

   \param  Timer        Timer context.
   \param  arg          Pointer to a DXS channel structure.

   \return none
*/
IFX_void_t DXS_TCB_DTMF (Timer_ID Timer, IFX_ulong_t arg)
{
   TAPI_UNUSED (Timer);

   DXS_DTMF_SH ((DXS_CHANNEL_t *) arg, DXS_DTMF_EVT_TIMEOUT);
}


/**
   DTMF state machine handler for the state DXS_DTMF_STATE_SETUP

   \param pCh     pointer to a DXS channel structure

   \param evt     event triggering the state machine

   \return
   - DXS_statusOk
   - DXS_statusDtmfCreateTimerErr
   - DXS_statusDtmfShSetupErr
   - DXS_statusErr in case of unknown event
*/
static IFX_int32_t DXS_DTMF_SH_Setup (DXS_CHANNEL_t *pCh, DXS_DTMF_EVT_t evt)
{
   IFX_int32_t    err = DXS_statusOk;

   switch (evt)
   {
      case DXS_DTMF_EVT_STOP:
         /* do nothing */
         return DXS_statusOk;
      case DXS_DTMF_EVT_START:
      {
         IFX_uint8_t             tableIndexForDigit;
         struct DXS_DTMF_AT_CFG  dtmfAtCfg;

         if (pCh->pDTMF->dtmfSend.dtmfTimerId == 0)
         {
            /* errmsg: Creating a timer for DTMF transmission failed */
            RETURN_STATUS(DXS_statusDtmfCreateTimerErr, IFX_NULL);
         }

         memset (&dtmfAtCfg, 0, sizeof (dtmfAtCfg));
         /* now the first digit must be played out now; the frequency and level
            information is stored in table DXS_DtmfDigits for all 16 DTMF
            digits;
            the required index for table DXS_DtmfDigits is stored in
            pCh->pDTMF->dtmfSend.pDtmfDigTblIndex;
            depending on the number of the digit to be played
            (pCh->pDTMF->dtmfSend.nSent) the index can be retrieved
         */
         tableIndexForDigit = pCh->pDTMF->dtmfSend.pDtmfDigTblIndex[0];
         if (tableIndexForDigit >= DXS_DTMF_MAX_DIGITS)
         {
            /* errmsg: DTMF state handler error from state DTMF setup.*/
            RETURN_STATUS(DXS_statusDtmfShSetupErr, IFX_NULL);
         }
         dtmfAtCfg.nFreq1  = (IFX_uint32_t)DXS_DtmfDigits[tableIndexForDigit][0];
         dtmfAtCfg.nFreq2  = (IFX_uint32_t)DXS_DtmfDigits[tableIndexForDigit][1];
         dtmfAtCfg.nLevel1 = DXS_DtmfDigits[tableIndexForDigit][2];
         dtmfAtCfg.nLevel2 = DXS_DtmfDigits[tableIndexForDigit][3];
         dtmfAtCfg.nAdd    = DXS_DTMF_AT_ADD_DTMF_AT_MUTE;
         dtmfAtCfg.nEnDis  = DXS_DTMF_AT_CTRL_EN;
         err = DXS_DTMF_AT_CTRL (pCh, &dtmfAtCfg);
         if (DXS_statusOk == err)
         {
            pCh->pDTMF->dtmfSend.state_dtmf = DXS_DTMF_STATE_TRANSMIT;
            /* start timer, length of timer = digit time */
            TAPI_SetTime_Timer (pCh->pDTMF->dtmfSend.dtmfTimerId,
                                pCh->pDTMF->dtmfSend.digitTime,
                                IFX_FALSE, IFX_FALSE);
         }
         break;
      }
      default:
         err = DXS_statusErr;
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("DXS ERROR: Illegal event type for DXS_DTMF_SH_Setup\n"));
         break;
   }

   if (!DXS_SUCCESS (err))
   {
      /* errmsg: DTMF state handler error from state DTMF setup.*/
      RETURN_STATUS(DXS_statusDtmfShSetupErr, IFX_NULL);
   }
   else
      return DXS_statusOk;
}

/**
   DTMF state machine handler for the state DXS_DTMF_STATE_TRANSMIT

   \param pCh     pointer to a DXS channel structure

   \param evt     event triggering the state machine

   \return
   - DXS_statusOk
   - DXS_statusDtmfShTransmitErr
*/
static IFX_int32_t DXS_DTMF_SH_Transmit (DXS_CHANNEL_t *pCh, DXS_DTMF_EVT_t evt)
{
   IFX_int32_t          err   = DXS_statusOk;
   IFX_TAPI_EVENT_t     tapiEvent;

   switch (evt)
   {
      case DXS_DTMF_EVT_TIMEOUT:
      {
         /* stop digit currently played out for pause */
         /* increment counter of dtmf tones which already have been played out */
         pCh->pDTMF->dtmfSend.nSent++;

         /* if all digits have been played out, stop DTMF transmission */
         if (pCh->pDTMF->dtmfSend.nSent == pCh->pDTMF->dtmfSend.nDtmfCnt)
         {
            /* stop dtmf and reset status; */
            err = irq_DXS_DtmfAtDisable (pCh);
            if (!DXS_SUCCESS (err))
               break;

            /* now reset internal values */
            pCh->pDTMF->dtmfSend.nDtmfCnt   = 0;
            pCh->pDTMF->dtmfSend.nSent      = 0;
            pCh->pDTMF->dtmfSend.state_dtmf = DXS_DTMF_STATE_SETUP;

            TAPI_Stop_Timer (pCh->pDTMF->dtmfSend.dtmfTimerId);

            /* Send TAPI event that generator has stopped. */
            memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
            tapiEvent.id = IFX_TAPI_EVENT_CID_TX_END;
            DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_ALM);

            IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);

            break;
         }

         /* stop dtmf/at generator for pause time */
         err = irq_DXS_DtmfAtDisable (pCh);
         if (!DXS_SUCCESS (err))
            break;

         pCh->pDTMF->dtmfSend.state_dtmf = DXS_DTMF_STATE_PAUSE;
         TAPI_SetTime_Timer (pCh->pDTMF->dtmfSend.dtmfTimerId,
                             pCh->pDTMF->dtmfSend.interDigitTime,
                             IFX_FALSE, IFX_FALSE);
         break;
      }
      case DXS_DTMF_EVT_STOP:
         /*
            offhook detected -> stop Dtmf/At generator,
            this is interrupt context, therefore no additional
            protection required
         */
         err = irq_DXS_DtmfAtDisable (pCh);
         if (!DXS_SUCCESS (err))
            break;

         /* now reset internal values */
         pCh->pDTMF->dtmfSend.nDtmfCnt   = 0;
         pCh->pDTMF->dtmfSend.nSent      = 0;

         pCh->pDTMF->dtmfSend.state_dtmf = DXS_DTMF_STATE_SETUP;
         TAPI_Stop_Timer (pCh->pDTMF->dtmfSend.dtmfTimerId);
         break;
      default:
         break;
   }

   if (!DXS_SUCCESS (err))
   {
      /* errmsg: DTMF state handler error from state DTMF transmit.*/
      RETURN_STATUS (DXS_statusDtmfShTransmitErr, IFX_NULL);
   }

   return DXS_statusOk;
}

/**
   DTMF state machine handler for the state DXS_DTMF_STATE_ABORT

   \param pCh     pointer to a DXS channel structure

   \param evt     event triggering the state machine

   \return
   - DXS_statusOk
   - DXS_statusDtmfShPauseErr
*/
static IFX_int32_t DXS_DTMF_SH_Pause (DXS_CHANNEL_t *pCh,
                                      DXS_DTMF_EVT_t evt)
{
   IFX_int32_t          err = DXS_statusOk;

   switch (evt)
   {
      case DXS_DTMF_EVT_TIMEOUT:
      {
         struct DXS_DTMF_AT_CFG  dtmfAtCfg;
         IFX_uint8_t             tableIndexForDigit;

         /* now the next digit must be played out now; for this next digit the
            frequency and level information is required, which is stored in
            table DXS_DtmfDigits for all 16 DTMF digits;
            for each digit to be played the index for table DXS_DtmfDigits is
            stored in pCh->pDTMF->dtmfSend.pDtmfDigTblIndex;
            depending on the number of the digit to be played
            (pCh->pDTMF->dtmfSend.nSent) the index can be retrieved
         */
         if (pCh->pDTMF->dtmfSend.nSent >= DXS_DTMF_MAX_BYTES)
         {
            /* errmsg: DTMF state handler error from state DTMF pause. */
            RETURN_STATUS (DXS_statusDtmfShPauseErr, IFX_NULL);
         }
         tableIndexForDigit =
            pCh->pDTMF->dtmfSend.pDtmfDigTblIndex[pCh->pDTMF->dtmfSend.nSent];
         if (tableIndexForDigit >= DXS_DTMF_MAX_DIGITS)
         {
            /* errmsg: DTMF state handler error from state DTMF pause. */
            RETURN_STATUS (DXS_statusDtmfShPauseErr, IFX_NULL);
         }
         memset (&dtmfAtCfg, 0, sizeof (dtmfAtCfg));
         /*
            with the index to DXS_DtmfDigits the parameter information is
            available
         */
         dtmfAtCfg.nFreq1  = (IFX_uint32_t)DXS_DtmfDigits[tableIndexForDigit][0];
         dtmfAtCfg.nFreq2  = (IFX_uint32_t)DXS_DtmfDigits[tableIndexForDigit][1];
         dtmfAtCfg.nLevel1 = DXS_DtmfDigits[tableIndexForDigit][2];
         dtmfAtCfg.nLevel2 = DXS_DtmfDigits[tableIndexForDigit][3];
         dtmfAtCfg.nAdd    = DXS_DTMF_AT_ADD_DTMF_AT_MUTE;
         dtmfAtCfg.nEnDis  = DXS_DTMF_AT_CTRL_EN;
         err = DXS_DTMF_AT_CTRL (pCh, &dtmfAtCfg);
         if (DXS_SUCCESS (err))
         {
            pCh->pDTMF->dtmfSend.state_dtmf = DXS_DTMF_STATE_TRANSMIT;
            /* start timer for the duration of the next dtmf digit */
            TAPI_SetTime_Timer (pCh->pDTMF->dtmfSend.dtmfTimerId,
                                pCh->pDTMF->dtmfSend.digitTime,
                                IFX_FALSE, IFX_FALSE);
            /*
               new digit is set up, timer is set up -> everything is done
               -> break here
            */
            break;
         }

         /* the next digit could not be set up -> stop dtmf and reset status; */
         err = irq_DXS_DtmfAtDisable (pCh);
         if (!DXS_SUCCESS (err))
            break;

         /* reset internal values */
         pCh->pDTMF->dtmfSend.nDtmfCnt   = 0;
         pCh->pDTMF->dtmfSend.nSent      = 0;

         pCh->pDTMF->dtmfSend.state_dtmf = DXS_DTMF_STATE_SETUP;
         TAPI_Stop_Timer (pCh->pDTMF->dtmfSend.dtmfTimerId);
         break;
      }
      case DXS_DTMF_EVT_STOP:
         /* offhook detected -> stop Dtmf/At not required,
            voice path is already enabled */

         /* reset internal values */
         pCh->pDTMF->dtmfSend.nDtmfCnt   = 0;
         pCh->pDTMF->dtmfSend.nSent      = 0;

         pCh->pDTMF->dtmfSend.state_dtmf = DXS_DTMF_STATE_SETUP;
         TAPI_Stop_Timer (pCh->pDTMF->dtmfSend.dtmfTimerId);
         break;
      default:
         break;
   }

   if (!DXS_SUCCESS (err))
   {
      /* errmsg: DTMF state handler error from state DTMF pause.*/
      RETURN_STATUS (DXS_statusDtmfShPauseErr, IFX_NULL);
   }
   else
      return DXS_statusOk;
}

/**
   DTMF state machine handler

   \param pCh  pointer to a DXS channel structure

   \param evt  event triggering the state machine

   \return
   - DXS_statusOk
   - DXS_statusDtmfShErr
*/
IFX_int32_t DXS_DTMF_SH (DXS_CHANNEL_t *pCh, DXS_DTMF_EVT_t evt)
{
   IFX_int32_t    err = DXS_statusOk;

   err = gDxsDtmfStates[pCh->pDTMF->dtmfSend.state_dtmf](pCh, evt);
   if (!DXS_SUCCESS (err))
   {
      /* errmsg: DTMF state handler error.*/
      RETURN_STATUS (DXS_statusDtmfShErr, IFX_NULL);
   }

   return DXS_statusOk;
}
