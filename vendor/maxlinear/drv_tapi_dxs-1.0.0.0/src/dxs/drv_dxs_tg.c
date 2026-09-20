/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016, Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_tg.c
   This file contains the implementation of dtmf/tone generator related
   functions of DUSLIC XS.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include <drv_tapi_config.h>
#include "drv_dxs_api.h"

#ifdef DXS_FEAT_TONE_GENERATOR

#include "drv_dxs_tg.h"
#include "drv_dxs_dtmf.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_void_t DXS_TG_SetParam (const DXS_CHANNEL_t *pCh,
                                   const IFX_TAPI_TONE_SIMPLE_t *pToneSimple,
                                   struct DXS_DTMF_AT_CFG *pDtmfAtCfg);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

/**
   get paramters from simple tone defintion and store them for FW message

   \param pCh  handle to a DuSLIC channel structure

   \param pToneSimple   pointer to simple tone defintion

   \parma pDtmfAtCfg    pointer to structure for FW message

   \return
   None.
*/
static IFX_void_t DXS_TG_SetParam(const DXS_CHANNEL_t *pCh,
                                  const IFX_TAPI_TONE_SIMPLE_t *pToneSimple,
                                  struct DXS_DTMF_AT_CFG *pDtmfAtCfg)
{
   IFX_uint8_t i, nFreqCnt = 0;

   if (pCh->nToneStep >= IFX_TAPI_TONE_STEPS_MAX)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
          ("DXS_TG_SetParam detected out of range tone step\n"));
      return;
   }

   /*
      TAPI Tone API allows up to four frequencies in each cadence, but
      DUSLIC XS is limited to play only two frequencies simultaneously.
      So this code will use in each step the first two active frequency
      definition it finds. Additionally set frequencies will be silently
      ignored.
   */
   for (i = 0; i < 4; i++)
   {
      /*
         check in a loop for all four possible frequencies, if they have been
         setup to be active in this cadence of the simple tone
      */
      if ((pToneSimple->frequencies[pCh->nToneStep] & (0x1 << i)))
      {
         /* save frequency and level settings */
         switch (0x1 << i)
         {
            case IFX_TAPI_TONE_FREQA:
               /*
                  if frequency A from the simple tone definition is set to be
                  active in this cadence, it is always saved in
                  dtmfAtCfg.nFreq1/nLevel1
               */
               pDtmfAtCfg->nFreq1 = pToneSimple->freqA;
               pDtmfAtCfg->nLevel1 = pToneSimple->levelA / 10;
               break;
            case IFX_TAPI_TONE_FREQB:
               /*
                  frequency B from the simple tone definition is set to be
                  active in this cadence; depending on nFreqCnt it is saved at
                  dtmfAtCfg.nFreqX/nLevelX
               */
               if (nFreqCnt == 0)
               {
                  pDtmfAtCfg->nFreq1 = pToneSimple->freqB;
                  pDtmfAtCfg->nLevel1 = pToneSimple->levelB / 10;
               }
               else if (nFreqCnt == 1)
               {
                  pDtmfAtCfg->nFreq2 = pToneSimple->freqB;
                  pDtmfAtCfg->nLevel2 = pToneSimple->levelB / 10;
               }
               break;
            case IFX_TAPI_TONE_FREQC:
               /*
                  frequency C from the simple tone definition is set to be
                  active in this cadence; depending on nFreqCnt it is saved at
                  dtmfAtCfg.nFreqX/nLevelX
               */
               if (nFreqCnt == 0)
               {
                  pDtmfAtCfg->nFreq1 = pToneSimple->freqC;
                  pDtmfAtCfg->nLevel1 = pToneSimple->levelC / 10;
               }
               else if (nFreqCnt == 1)
               {
                  pDtmfAtCfg->nFreq2 = pToneSimple->freqC;
                  pDtmfAtCfg->nLevel2 = pToneSimple->levelC / 10;
               }
               break;
            case IFX_TAPI_TONE_FREQD:
               /*
                  frequency D from the simple tone definition is set to be
                  active in this cadence; depending on nFreqCnt it is saved at
                  dtmfAtCfg.nFreqX/nLevelX
               */
               if (nFreqCnt == 0)
               {
                  pDtmfAtCfg->nFreq1 = pToneSimple->freqD;
                  pDtmfAtCfg->nLevel1 = pToneSimple->levelD / 10;
               }
               else if (nFreqCnt == 1)
               {
                  pDtmfAtCfg->nFreq2 = pToneSimple->freqD;
                  pDtmfAtCfg->nLevel2 = pToneSimple->levelD / 10;
               }
               break;
            default:
               /* unsupported frequency */
               TAPI_ASSERT (0);
               break;
         }
         /* increment the count of active frequencies for this cadence */
         nFreqCnt++;
         /*
            a maximum of two frequencies can be active simultaneously, if this
            count is exceeded, the loop is exited
         */
         if (nFreqCnt >= DXS_TG_MAX_FREQ)
            break;
      }
   }
   return;
}


/**
   Play out a tone with the given tone definition

   This function handles all necessary steps to play out the first tone of the
   tone sequence. The event handler is called to initiate a timer before the
   next tone step can be played out.

   \param  pLLChannel   Pointer to DXS channel structure.

   \param  res          Resource number to be used for playing the tone.
                        There is just on TG and so this parameter is ignored.

   \param pToneSimple   Handle to the tone definition to play.

   \param dst           Destination where to play the tone: local or network.
                        Must be local. Network is not supported.

   \return
   - DXS_statusOk
   - DXS_statusToneDirErr
   - DXS_statusToneCfgErr
   - DXS_statusTonePlayErr
   - DXS_statusErr if channel pointer is null
   - DXS_statusFuncParam if at least one parameter in function is wrong
*/
IFX_int32_t DXS_TAPI_LL_ALM_TG_Play (IFX_TAPI_LL_CH_t *pLLChannel,
                                     IFX_uint8_t res,
                                     IFX_TAPI_TONE_SIMPLE_t const *pToneSimple)
{
   IFX_int32_t             err               = DXS_statusOk;
   DXS_CHANNEL_t           *pCh              = (DXS_CHANNEL_t *)pLLChannel;
   struct DXS_DTMF_AT_CFG  dtmfAtCfg;

   TAPI_UNUSED (res);

   if (pLLChannel == IFX_NULL)
      return DXS_statusErr;

   memset(&dtmfAtCfg, 0, sizeof(dtmfAtCfg));

   if (pToneSimple == IFX_NULL)
   {
      /* errmsg: Tone not configured. */
      RETURN_STATUS(DXS_statusToneCfgErr,IFX_NULL);
   }

   if((pToneSimple->loop > IFX_TAPI_TONE_LOOP_MAX) ||
      (pToneSimple->levelA < IFX_TAPI_TONE_POWER_LEVEL_MIN) ||
      (pToneSimple->levelA > IFX_TAPI_TONE_POWER_LEVEL_MAX) ||
      (pToneSimple->levelB < IFX_TAPI_TONE_POWER_LEVEL_MIN) ||
      (pToneSimple->levelB > IFX_TAPI_TONE_POWER_LEVEL_MAX) ||
      (pToneSimple->levelC < IFX_TAPI_TONE_POWER_LEVEL_MIN) ||
      (pToneSimple->levelC > IFX_TAPI_TONE_POWER_LEVEL_MAX) ||
      (pToneSimple->levelD < IFX_TAPI_TONE_POWER_LEVEL_MIN) ||
      (pToneSimple->levelD > IFX_TAPI_TONE_POWER_LEVEL_MAX) ||
      (pToneSimple->freqA > IFX_TAPI_TONE_FREQ_MAX) ||
      (pToneSimple->freqB > IFX_TAPI_TONE_FREQ_MAX) ||
      (pToneSimple->freqC > IFX_TAPI_TONE_FREQ_MAX) ||
      (pToneSimple->freqD > IFX_TAPI_TONE_FREQ_MAX))
   {
      RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);
   }

   /*
      the first tone of the sequence is started -> initialize tone step
      variable
   */
   pCh->nToneStep = 0;
   pCh->nTone_Cnt = pToneSimple->loop;

#ifndef TAPI_ONE_DEVNODE
   /* TAPI_ONE_DEVNODE check is done here because there was difference
      in TG interface between DXS and VXT.
      As VXT was used wit TAPI V4 API and DXS was used with TAPI V3 API,
      in the TAPI HL there is a check for TAPI API version when using
      TG_Play(). Now when DXS can be used with TAPI V4 API code below cannot
      be executed, and cadences can be played only with TG_toneStep,
      like it is done in VXT driver.  */
   /* check simple tone definition and set parameters for FW message */
   DXS_TG_SetParam(pCh, pToneSimple, &dtmfAtCfg);

   /* check if level is below device limitation */
   if (dtmfAtCfg.nLevel1 < DXS_DTMF_AT_LEVEL_MIN)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
      ("DXS_INFO: Level for frequency A = %d too low, set to %idB\n",
        dtmfAtCfg.nLevel1, DXS_DTMF_AT_LEVEL_MIN));
      dtmfAtCfg.nLevel1 = DXS_DTMF_AT_LEVEL_MIN;
   }
   if (dtmfAtCfg.nLevel2 < DXS_DTMF_AT_LEVEL_MIN)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
      ("DXS_INFO: Level for frequency B = %d too low, set to %idB\n",
        dtmfAtCfg.nLevel2, DXS_DTMF_AT_LEVEL_MIN));
      dtmfAtCfg.nLevel2 = DXS_DTMF_AT_LEVEL_MIN;
   }

   dtmfAtCfg.nAdd    = DXS_DTMF_AT_ADD_DTMF_AT_MUTE;
   dtmfAtCfg.nEnDis  = DXS_DTMF_AT_CTRL_EN;

   err = DXS_DTMF_AT_CTRL (pCh, &dtmfAtCfg);
   pCh->nToneStep++;

   if (DXS_statusOk != err)
   {
      /* errmsg: Error playing out a tone.*/
      RETURN_STATUS(DXS_statusTonePlayErr, IFX_NULL);
   }
#endif /* TAPI_ONE_DEVNODE */
   RETURN_STATUS(err, IFX_NULL);
}


/**
   Play out next tone of given definition.

   \param  pLLChannel   Pointer to DXS channel structure.

   \param pTone

   \param res           Resource number which is used for playing the tone. The
                        available resources are device dependend.
                        There is just on TG and so this parameter is always 0.

   \param nToneStep

   \return
   - DXS_statusOk          successful
   - DXS_statusErr         if channel pointer is null
   - DXS_statusFuncParam   if at least one parameter in function is wrong
   - DXS_statusToneStopErr
*/
IFX_int32_t DXS_TAPI_LL_ALM_TG_Step (IFX_TAPI_LL_CH_t *pLLChannel,
                                     IFX_TAPI_TONE_SIMPLE_t const *pTone,
                                     IFX_uint8_t res,
                                     IFX_uint8_t *nToneStep)
{
   IFX_int32_t             err            = DXS_statusErr;
   DXS_CHANNEL_t           *pCh           = (DXS_CHANNEL_t *) pLLChannel;
   TAPI_CHANNEL            *pTapiCh;
   IFX_TAPI_EVENT_t        tapiEvent;
   struct DXS_DTMF_AT_CFG  dtmfAtCfg;

   if((pCh == IFX_NULL) || (pCh->pTapiCh == IFX_NULL))
      return DXS_statusErr;

   pTapiCh = pCh->pTapiCh;

   if((pTone == IFX_NULL) || (nToneStep == IFX_NULL))
      RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);

   memset (&dtmfAtCfg, 0, sizeof (dtmfAtCfg));

   /* Either maximum of possible tone steps has been reached or
      no more cadences have been configured. */
   if (pCh->nToneStep >= IFX_TAPI_TONE_STEPS_MAX ||
       pTone->cadence[pCh->nToneStep] == 0)
   {
      pCh->nToneStep = 0;
      /* to decide which timer has to be started, used by HL TAPI */
      *nToneStep = pCh->nToneStep;
      if (pTone->loop != 0)
      {
         /* nTone_Cnt is initially set up with the loop count. loop is
            the number of repetitions of the cadences, separated by pause.
            nToneCnt is decremented, because all cadences of one repetion
            have been played out.
         */
         pCh->nTone_Cnt--;
      }

      dtmfAtCfg.nAdd    = DXS_DTMF_AT_ADD_DTMF_AT;
      dtmfAtCfg.nEnDis  = DXS_DTMF_AT_CTRL_DIS;

      err = DXS_DTMF_AT_CTRL (pCh, &dtmfAtCfg);
      if (DXS_statusOk != err)
      {
         /* errmsg: Error stopping a tone. */
         RETURN_STATUS(DXS_statusToneStopErr, IFX_NULL);
      }

      /* TG resource is #1 */
      /* Fill event structure. */
      memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
      tapiEvent.id = IFX_TAPI_EVENT_TONE_GEN_END_RAW;
      DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_ALM);
      /* value stores the TG resource number */
      tapiEvent.data.value = res;

      err = IFX_TAPI_Event_Dispatch(pTapiCh,&tapiEvent);
      RETURN_STATUS(err, IFX_NULL);
   }

   /* check simple tone definition and get parameters for the next tone step */
   DXS_TG_SetParam(pCh, pTone, &dtmfAtCfg);

   /*
      enable the adding of dtmf/at signal to pcm rx signal, enable
      dtmf/at generator and mute voice path
   */
   dtmfAtCfg.nAdd    = DXS_DTMF_AT_ADD_DTMF_AT_MUTE;
   dtmfAtCfg.nEnDis  = DXS_DTMF_AT_CTRL_EN;

   err = DXS_DTMF_AT_CTRL (pCh, &dtmfAtCfg);
   /* If the current playout is the pause of the previous loop of the simple
      tone, nToneStep must not be incremented. Otherwise the first cadence
      in the next loop would be left out. */
   if (TAPI_ToneState (pTapiCh, res) == TAPI_CT_ACTIVE)
   {
      /* Increment to the next tone to be played out */
      pCh->nToneStep++;
   }
   /* to decide which timer has to be started, used by HL TAPI */
   *nToneStep = pCh->nToneStep;

   if (!DXS_SUCCESS(err))
   {
      /* errmsg: Playing out a tone failed. */
      RETURN_STATUS(DXS_statusTonePlayErr, IFX_NULL);
   }
   RETURN_STATUS(err, IFX_NULL);
}


/**
   Stops playing tones on the given resource immediately

   \param  pLLChannel   Pointer to DXS channel structure.

   \param  res          Resource number of the TG to be stopped. Since there is
                        only on TG this parameter is ignored.

   \return
   - DXS_statusOk
   - DXS_statusToneStopErr
   - DXS_statusErr if channel pointer is null
*/
IFX_int32_t DXS_TAPI_LL_ALM_TG_Stop (IFX_TAPI_LL_CH_t *pLLChannel,
                                     IFX_uint8_t res)
{
   IFX_int32_t             err               = DXS_statusOk;
   DXS_CHANNEL_t           *pCh              = (DXS_CHANNEL_t *) pLLChannel;
   struct DXS_DTMF_AT_CFG  dtmfAtCfg;

   TAPI_UNUSED (res);

   if(pLLChannel == IFX_NULL)
      return DXS_statusErr;

   memset (&dtmfAtCfg, 0, sizeof (dtmfAtCfg));
   /*
      disable the dtmf/at generator
   */
   dtmfAtCfg.nAdd    = DXS_DTMF_AT_ADD_DTMF_AT;
   dtmfAtCfg.nEnDis  = DXS_DTMF_AT_CTRL_DIS;

   err = DXS_DTMF_AT_CTRL (pCh, &dtmfAtCfg);
   if (DXS_statusOk != err)
   {
      /* errmsg: Error stopping a tone. */
      RETURN_STATUS(DXS_statusToneStopErr, IFX_NULL);
   }

   RETURN_STATUS(err, IFX_NULL);
}


/* ========================================================================== */
/*                         Function pointer exports                           */
/* ========================================================================== */
#endif /* DXS_FEAT_TONE_GENERATOR */

