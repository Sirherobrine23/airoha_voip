/******************************************************************************

   Copyright 2006-2009 Infineon Technologies AG
   Copyright 2009-2015 Lantiq Deutschland GmbH
   Copyright 2015      Lantiq Beteiligungs-GmbH & Co.KG
   Copyright 2022      MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_opcontrol.c
   TAPI Operation Control Services.

   \remarks
   All operations done by functions in this module are phone
   related and assumes a phone channel file descriptor.
   Caller of anyone of the functions must make sure that a phone
   channel is used. In case data channel functions are invoked here,
   an instance of the data channel must be passed.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "drv_tapi.h"
#include "drv_tapi_errno.h"
#include "drv_tapi_ppd.h"

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/* ============================= */
/* Local function declaration    */
/* ============================= */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Reads the hook status from the device

   \param pChannel   - handle to TAPI_CHANNEL structure
   \param pHookMode  - storage variable for hook state

   \return TAPI_statusOk

   \remarks
      pHookMode require different memory size for different driver modes.
      For single device node driver mode require handle to
      the \ref IFX_TAPI_LINE_HOOK_STATUS_GET_t structure.
      For multiple device node driver mode require handle to
      the \ref IFX_int32_t.
*/
IFX_int32_t TAPI_Phone_HookstateGet (TAPI_CHANNEL *pChannel, IFX_TAPI_LINE_HOOK_STATUS_GET_t *pHookMode)
{
   if (pChannel->TapiOpControlData.bHookState)
   {
#ifdef TAPI_ONE_DEVNODE
      pHookMode->hookMode = IFX_TAPI_LINE_OFFHOOK;
#else
      *pHookMode = IFX_TRUE;
#endif /* TAPI_ONE_DEVNODE*/
   }
   else
   {
#ifdef TAPI_ONE_DEVNODE
      pHookMode->hookMode = IFX_TAPI_LINE_ONHOOK;
#else
      *pHookMode = IFX_FALSE;
#endif /* TAPI_ONE_DEVNODE*/
   }

   return (TAPI_statusOk);
}

/**
   Sets the linefeeding mode of the device

   \param pChannel      Pointer to TAPI_CHANNEL structure.
   \param nMode         Line mode.

   \return
   IFX_SUCCESS or IFX_ERROR

   \remarks
   Line mode is ALWAYS set, also if it was set before.
*/
IFX_int32_t TAPI_Phone_Set_Linefeed(TAPI_CHANNEL *pChannel,
                                    IFX_int32_t nMode)
{
   IFX_TAPI_DRV_CTX_t* pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;
   IFX_int32_t         ret     = TAPI_statusOk;
   IFX_TAPI_LINE_MODE_t tmp_nLineMode = pChannel->TapiOpControlData.nLineMode;
   IFX_TAPI_LINE_MODE_t  nLineMode = nMode;

   /* Disallow Standby and Ringing when phone is off-hook. When the last line
      feeding was "disabled" the hook state is undefined so skip this check. */
   if ((tmp_nLineMode != IFX_TAPI_LINE_FEED_DISABLED) &&
       (pChannel->TapiOpControlData.bHookState == IFX_TRUE))
   {
      switch (nLineMode)
      {
         case IFX_TAPI_LINE_FEED_STANDBY:
         case IFX_TAPI_LINE_FEED_RING_BURST:
         case IFX_TAPI_LINE_FEED_RING_PAUSE:
            /* Unsuitable line mode while phone is off-hook */
            RETURN_STATUS (TAPI_statusPhoneOffHook, 0);
         default:
            break;
      }
   }

   /* In emergency shutdown mode only setting of disabled is allowed. */
   if (pChannel->TapiOpControlData.bEmergencyShutdown == IFX_TRUE)
   {
      if (nLineMode == IFX_TAPI_LINE_FEED_DISABLED)
      {
         pChannel->TapiOpControlData.bEmergencyShutdown = IFX_FALSE;
      }
      else
      {
         /* errmsg: Cannot set linefeeding different than disabled while a line
            fault is present. */
         RETURN_STATUS (TAPI_statusLineFault, 0);
      }
   }


#ifdef TAPI_VERSION3
#ifdef TAPI_FEAT_METERING
   if (TAPI_Phone_Meter_IsActive (pChannel))
   {
      /* NOTE: Recursive call
               _Meter_Stop will call _Set_Linefeed to restore
               the line mode before metering */

      /* stop metering before line mode changing */
      TAPI_Phone_Meter_Stop (pChannel);
   }
#endif /* TAPI_FEAT_METERING */
#endif /* TAPI_VERSION3 */

#ifndef TAPI_FEAT_CID
#ifdef TAPI_FEAT_RINGENGINE
   /* This code causes a recursive call ending in a lockup when TAPI CID
      tries to use OSI alerting. */
   if (nLineMode == IFX_TAPI_LINE_FEED_DISABLED)
   {
      /* NOTE:
           Possible recursive calls during linefeed restore procedure.
           No any conflicts expected, because of DISABLED mode never used.
      */
      IFX_TAPI_Ring_Stop (pChannel);
   }
#endif /* TAPI_FEAT_RINGENGINE */
#endif /* TAPI_FEAT_CID */

   /* check if auto battery switch have to enabled */
   if ((nLineMode == IFX_TAPI_LINE_FEED_NORMAL_AUTO    ||
        nLineMode == IFX_TAPI_LINE_FEED_REVERSED_AUTO) &&
       !pChannel->TapiOpControlData.nBatterySw)
   {
      pChannel->TapiOpControlData.nBatterySw = 0x01;
   }

   /* check if auto battery switch have to disabled */
   if (!(nLineMode == IFX_TAPI_LINE_FEED_NORMAL_AUTO    ||
         nLineMode == IFX_TAPI_LINE_FEED_REVERSED_AUTO) &&
       pChannel->TapiOpControlData.nBatterySw)
   {
      pChannel->TapiOpControlData.nBatterySw = 0x00;
   }

   /* check if polarity has to change */
   if ((nLineMode == IFX_TAPI_LINE_FEED_ACTIVE_REV        ||
        nLineMode == IFX_TAPI_LINE_FEED_REVERSED_AUTO     ||
        nLineMode == IFX_TAPI_LINE_FEED_REVERSED_LOW      ||
        nLineMode == IFX_TAPI_LINE_FEED_ACTIVE_RES_REVERSED) &&
       !pChannel->TapiOpControlData.nPolarity)
   {
      pChannel->TapiOpControlData.nPolarity = 0x01;
   }
   if (!(nLineMode == IFX_TAPI_LINE_FEED_ACTIVE_REV       ||
         nLineMode == IFX_TAPI_LINE_FEED_REVERSED_AUTO    ||
         nLineMode == IFX_TAPI_LINE_FEED_REVERSED_LOW     ||
         nLineMode == IFX_TAPI_LINE_FEED_ACTIVE_RES_REVERSED) &&
       pChannel->TapiOpControlData.nPolarity)
   {
      pChannel->TapiOpControlData.nPolarity = 0x00;
   }

#ifdef TAPI_FEAT_PHONE_DETECTION
   ret = IFX_TAPI_PPD_HandleLineFeeding(pChannel, &nLineMode);
   if (nLineMode != IFX_TAPI_LINE_FEED_PHONE_DETECT)
   {
      /* call low level function to change operation mode */
      if (IFX_TAPI_PtrChk (pDrvCtx->ALM.Line_Mode_Set))
      {
          ret = pDrvCtx->ALM.Line_Mode_Set(pChannel->pLLChannel,
                                 nLineMode, tmp_nLineMode);
      }
   }
#else
   if (nLineMode == IFX_TAPI_LINE_FEED_PHONE_DETECT)
   {
      nLineMode = IFX_TAPI_LINE_FEED_STANDBY;
      TRACE(TAPI_DXS, DBG_LEVEL_LOW,
           ("DRV_WARNING: Phone Detection is not supported. "
            "Line mode STANDBY used instead of DETECT (ch %d).\n",
            pChannel->nChannel));
   }

   /* call low level function to change operation mode */
   if (IFX_TAPI_PtrChk (pDrvCtx->ALM.Line_Mode_Set))
   {
       ret = pDrvCtx->ALM.Line_Mode_Set(pChannel->pLLChannel,
                              nLineMode, tmp_nLineMode);
   }
#endif /* TAPI_FEAT_PHONE_DETECTION */

   if (!TAPI_SUCCESS(ret))
   {
      /* restore the old value, because the configuration on the driver failed */
      pChannel->TapiOpControlData.nLineMode = tmp_nLineMode;
#ifdef TAPI_FEAT_PHONE_DETECTION
      (void)IFX_TAPI_PPD_HandleLineFeeding(pChannel, &tmp_nLineMode);
#endif /* TAPI_FEAT_PHONE_DETECTION */
      RETURN_STATUS (TAPI_statusLineModeFail, ret);
   }

#ifdef TAPI_FEAT_DIALENGINE
   /* After successfully setting disabled inform the dial state machine. */
   if ((tmp_nLineMode != IFX_TAPI_LINE_FEED_DISABLED) &&
       (nMode == IFX_TAPI_LINE_FEED_DISABLED))
   {
      IFX_TAPI_Dial_LineDisable (pChannel);
   }
#endif /* TAPI_FEAT_DIALENGINE */

   /* save current linemode in tapi structure */
   pChannel->TapiOpControlData.nLineMode = nLineMode;

   return ret;
}

/**
   Changes the linefeeding mode of the device

   \param pChannel      Pointer to TAPI_CHANNEL structure.
   \param nMode         Line mode.

*/
IFX_void_t TAPI_Phone_Change_Linefeed(TAPI_CHANNEL *pChannel,
                                      IFX_int32_t nMode)
{
   switch (pChannel->TapiOpControlData.nLineMode)
   {
      case IFX_TAPI_LINE_FEED_ACTIVE:
      case IFX_TAPI_LINE_FEED_RING_PAUSE:
      case IFX_TAPI_LINE_FEED_NORMAL_AUTO:
      case IFX_TAPI_LINE_FEED_ACTIVE_LOW:
      case IFX_TAPI_LINE_FEED_ACTIVE_BOOSTED:
      case IFX_TAPI_LINE_FEED_ACTIVE_REV:
      case IFX_TAPI_LINE_FEED_REVERSED_AUTO:
         if (nMode != IFX_TAPI_LINE_FEED_ACTIVE)
         {
            pChannel->TapiOpControlData.nLineMode = nMode;
         }
         break;
      case IFX_TAPI_LINE_FEED_STANDBY:
      case IFX_TAPI_LINE_FEED_PARKED_REVERSED:
         if (nMode != IFX_TAPI_LINE_FEED_STANDBY)
         {
            pChannel->TapiOpControlData.nLineMode = nMode;
         }
         break;
      case IFX_TAPI_LINE_FEED_DISABLED:
         pChannel->TapiOpControlData.nLineMode = nMode;
         break;
      case IFX_TAPI_LINE_FEED_RING_BURST:
         pChannel->TapiOpControlData.nLineMode = nMode;
         break;
      case IFX_TAPI_LINE_FEED_NLT:
         pChannel->TapiOpControlData.nLineMode = nMode;
         break;
      default:
         break;
   }
}


/**
   Restore the linefeeding mode of the device to the last requested mode

   \param pChannel      Pointer to TAPI_CHANNEL structure.
*/
IFX_void_t TAPI_Phone_Linefeed_Restore(TAPI_CHANNEL *pChannel)
{
   IFX_TAPI_DRV_CTX_t* pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;
   IFX_int32_t         ret     = TAPI_statusOk;

   IFX_TAPI_LINE_MODE_t nLineMode = pChannel->TapiOpControlData.nLineMode;

   /* While in emergency shutdown do not try to restore the line feed. */
   if (pChannel->TapiOpControlData.bEmergencyShutdown == IFX_TRUE)
      return;

#ifdef TAPI_FEAT_PHONE_DETECTION
   ret = IFX_TAPI_PPD_HandleLineFeeding(pChannel, &nLineMode);
   if (nLineMode != IFX_TAPI_LINE_FEED_PHONE_DETECT)
#endif /* TAPI_FEAT_PHONE_DETECTION */
   {
      /* call low level function to change operation mode */
      if (IFX_TAPI_PtrChk (pDrvCtx->ALM.Line_Mode_Set))
      {
          ret = pDrvCtx->ALM.Line_Mode_Set(pChannel->pLLChannel,
                                           nLineMode, nLineMode);
      }
   }

   if (!TAPI_SUCCESS(ret))
   {
      TRACE(TAPI_DXS, DBG_LEVEL_LOW,
            ("TAPI ERROR: Linefeed restore failed %X\n", ret));
   }
}


/**
   Gets the linefeeding mode of the device

   \param pChannel      Pointer to TAPI_CHANNEL structure.
   \param nMode         Line mode.
*/
IFX_void_t TAPI_Phone_Linefeed_Get(TAPI_CHANNEL *pChannel,
                                   IFX_uint8_t *nLineMode)
{
   *nLineMode = pChannel->TapiOpControlData.nLineMode;
}


/**
   Sets the line type mode of the specific analog channel

   \param pChannel      Pointer to TAPI_CHANNEL structure.
   \param pCfg          Pointer to struct IFX_TAPI_LINE_TYPE_CFG_t.

   \return TAPI_statusOk or TAPI_statusErr

   \remarks Line mode is ALWAYS set, also if it was set before. By default
            FXS type is used.
*/
IFX_int32_t TAPI_Phone_Set_LineType(
      TAPI_CHANNEL *pChannel,
      IFX_TAPI_LINE_TYPE_CFG_t const *pCfg
   )
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;
   IFX_int32_t ret = TAPI_statusOk,
               retLL = TAPI_statusOk;

   /* call low level function to change operation mode */
   if (IFX_TAPI_PtrChk (pDrvCtx->ALM.Line_Type_Set))
   {
      retLL = pDrvCtx->ALM.Line_Type_Set(pChannel->pLLChannel, pCfg->lineType);
   }

   if (TAPI_SUCCESS(retLL))
   {
      pChannel->TapiOpControlData.nLineType = pCfg->lineType;
   }
   else
   {
      /* errmsg: LL driver returned an error */
      ret = TAPI_statusLLFailed;
   }

   RETURN_STATUS (ret, retLL);
}


#ifdef TAPI_FEAT_DTMF
/**
   Gets the DTMF Receiver Coefficients

   \param pChannel        - handle to TAPI_CHANNEL structure
   \param pCoeff          - handle to IFX_TAPI_DTMF_RX_CFG_t structure

   \return
   TAPI_statusOk or TAPI_statusErr
*/
IFX_int32_t TAPI_Phone_DTMFR_Cfg_Get (TAPI_CHANNEL *pChannel,
   IFX_TAPI_DTMF_RX_CFG_t *pDtmfRxCoeff)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;
   IFX_int32_t retLL;

   /* do not touch anything if LL device DTMF Rx coefficients
      not configurable */
   if (!IFX_TAPI_PtrChk (pDrvCtx->SIG.DTMF_RxCoeff))
   {
      /* DTMF Rx coefficients not configurable on LL device */
      RETURN_STATUS (TAPI_statusLLNotSupp, 0);
   }

   /* call low level routine to read the settings */
   retLL = pDrvCtx->SIG.DTMF_RxCoeff (pChannel->pLLChannel, IFX_TRUE, pDtmfRxCoeff);
   if (!(TAPI_SUCCESS (retLL)))
   {
      RETURN_STATUS (TAPI_statusDtmfRxCfg, retLL);
   }

   return TAPI_statusOk;
}
#endif /* TAPI_FEAT_DTMF */


#ifdef TAPI_FEAT_DTMF
/**
   Sets the DTMF Receiver Coefficients

   \param pChannel        - handle to TAPI_CHANNEL structure
   \param pCoeff          - handle to IFX_TAPI_DTMF_RX_CFG_t structure

   \return
   TAPI_statusOk or TAPI_statusErr
*/
IFX_int32_t TAPI_Phone_DTMFR_Cfg_Set (TAPI_CHANNEL *pChannel,
   IFX_TAPI_DTMF_RX_CFG_t const *pDtmfRxCoeff)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;
   IFX_int32_t ret;
   IFX_TAPI_DTMF_RX_CFG_t coeff;

   /* do not touch anything if LL device DTMF Rx coefficients
      not configurable */
   if (!IFX_TAPI_PtrChk (pDrvCtx->SIG.DTMF_RxCoeff))
   {
      /* DTMF Rx coefficients not configurable on LL device */
      RETURN_STATUS (TAPI_statusLLNotSupp, 0);
   }

   memcpy (&coeff, pDtmfRxCoeff, sizeof (coeff));

   /* call low level routine to write the settings */
   ret = pDrvCtx->SIG.DTMF_RxCoeff (pChannel->pLLChannel, IFX_FALSE, &coeff);
   if (!(TAPI_SUCCESS (ret)))
   {
      RETURN_STATUS (TAPI_statusDtmfRxCfg, ret);
   }

   return ret;
}
#endif /* TAPI_FEAT_DTMF */


#ifdef TAPI_FEAT_NLT
/**
   Start line testing.

   \param   pChannel    Handle to the TAPI channel
   \param   pTestParam  Params for test to be run

   \return
      TAPI_statusOk - success
      TAPI_statusLLNotSupp - service not supported by low-level driver
      TAPI_statusLLFailed - failed low-level call
*/
IFX_int32_t IFX_TAPI_NLT_Test_Start (TAPI_CHANNEL *pChannel,
   IFX_TAPI_NLT_TEST_START_t *pTestParam)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;
   IFX_int32_t retLL;

   /* Check for LL driver functionality */
   if (!IFX_TAPI_PtrChk (pDrvCtx->NLT.NLT_test_start))
   {
      RETURN_STATUS (TAPI_statusLLNotSupp, 0);
   }

   retLL = pDrvCtx->NLT.NLT_test_start(pChannel->pLLChannel, pTestParam);

   if (!TAPI_SUCCESS(retLL))
   {
      RETURN_STATUS (TAPI_statusLLFailed, retLL);
   }

   return TAPI_statusOk;
}


/**
   Get results of line test.

   \param   pChannel      Handle to the TAPI channel
   \param   pTestResults  Test results

   \return
      TAPI_statusOk - success
      TAPI_statusLLNotSupp - service not supported by low-level driver
      TAPI_statusLLFailed - failed low-level call
*/
IFX_int32_t IFX_TAPI_NLT_Result_Get (TAPI_CHANNEL *pChannel,
   IFX_TAPI_NLT_RESULT_GET_t *pTestResult)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;
   IFX_int32_t retLL;

   /* Check for LL driver functionality */
   if (!IFX_TAPI_PtrChk (pDrvCtx->NLT.NLT_result_get))
   {
      RETURN_STATUS (TAPI_statusLLNotSupp, 0);
   }

   retLL = pDrvCtx->NLT.NLT_result_get(pChannel->pLLChannel, pTestResult);

   if (!TAPI_SUCCESS(retLL))
   {
      RETURN_STATUS (TAPI_statusLLFailed, retLL);
   }

   return TAPI_statusOk;
}

#endif /* TAPI_FEAT_NLT */
