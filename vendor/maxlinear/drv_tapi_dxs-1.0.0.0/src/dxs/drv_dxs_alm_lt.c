/******************************************************************************

                            Copyright (c) 2014, 2015
                        Lantiq Beteiligungs-GmbH & Co.KG
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_alm_lt.c
   This file contains the implementation of functions for GR909 linetesting
   and capacitance measurement.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include <drv_tapi_config.h>
#include "drv_dxs_api.h"
#include "drv_dxs_mbx.h"
#include "drv_dxs_alm_priv.h"
#include "drv_dxs_errno.h"
#include "drv_dxs_init.h"

#include "drv_dxs_alm_lt.h"

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

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

#ifdef DXS_FEAT_GR909
/**
   Start selected subset (or all) GR909 tests

   \param  pLLChannel   Pointer to TAPI low level channel structure.
   \param  p_start      Pointer to DXS_IO_GR909_Start_t structure.

   \return
      DXS_statusOk or error code

   \remarks
   GR909 tests can be started only on disabled lines.
*/
IFX_int32_t DXS_TAPI_LL_ALM_LT_GR909_Start (IFX_TAPI_LL_CH_t *pLLChannel,
                                        IFX_TAPI_GR909_START_t const *p_start)
{
   DXS_CHANNEL_t         *pCh  = (DXS_CHANNEL_t *) pLLChannel;
   DXS_DEVICE_t          *pDev;
   DXS_SDD_GR909Config_t *p_ctrl;
   IFX_int32_t            ret     = IFX_SUCCESS;

   if((pCh == IFX_NULL) || (pCh->pParent == IFX_NULL))
      return DXS_statusErr;

   pDev = pCh->pParent;

   if(pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   p_ctrl = &pCh->pALM->sdd_gr909_config;

   if(p_start == IFX_NULL)
      RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);

   /* Nothing to test ? */
   if (p_start->test_mask == 0)
      goto error;

   /* Until the DC/DC converter type is set no linefeed change is allowed. */
   if (pCh->pALM->nDcDcType == DXS_DCDC_TYPE_NOTSET)
   {
      /* errmsg: Operation blocked until BBD containing DC/DC type setting is
                 downloaded  */
      RETURN_STATUS (DXS_statusBlockedNoDcDcType, IFX_NULL);
   }
   /* sanity check */
   if (pDev->caps.bfw_GR909 == 0)
   {
      RETURN_STATUS (DXS_statusNotSupported, IFX_NULL);
   }

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   /* implicit PDH opmode */
   ret = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Disabled);
   if (!DXS_SUCCESS (ret))
      goto error;
   /* workaround: when the SLIC is in sleep the first opmode change command
      will only wake it up but actual setting of the opmode will take until
      the dup counters have expired. To speed up the process the opmode
      change can be repeated after the SLIC is awake. So do a delay here
      to allow enough time for the wake up. */
   TAPI_OS_MSecSleep(3);
   /* end of workaround */
   ret = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Disabled);
   if (!DXS_SUCCESS (ret))
      goto error;
   /* workaround: Opmode Change event is not available on DXS.
      Wait 1 ms instead of polling the opmode */
   TAPI_OS_MSecSleep(1);
   /* end of workaround */

   ret = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)p_ctrl);
   if (!DXS_SUCCESS (ret))
   {
      TAPI_OS_MutexRelease (&pCh->mtxChAcc);
      RETURN_STATUS (DXS_statusCmdMbWrErr, IFX_NULL);
   }

   /* start GR909 measurement */
   ret = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_GR909);
error:
   /* release channel */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);
   return ret;
}


/**
   Stop GR909 tests

   \param  pLLChannel   Pointer to TAPI low level channel structure.

   \return
      DXS_statusOk or error code
*/
IFX_int32_t DXS_TAPI_LL_ALM_LT_GR909_Stop (IFX_TAPI_LL_CH_t *pLLChannel)
{
   DXS_CHANNEL_t         *pCh  = (DXS_CHANNEL_t *) pLLChannel;
   IFX_int32_t            ret;
   IFX_uint8_t            curr_opmode;

   if(pCh == IFX_NULL)
      return DXS_statusErr;

   if(pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   /* Until the DC/DC converter type is set no linefeed change is allowed. */
   if (pCh->pALM->nDcDcType == DXS_DCDC_TYPE_NOTSET)
   {
      /* errmsg: Operation blocked until BBD containing DC/DC type setting is
                 downloaded  */
      RETURN_STATUS (DXS_statusBlockedNoDcDcType, IFX_NULL);
   }

   /* Get the current opmode - wait while a opmode change is ongoing. */
   ret = DXS_ALM_OpmodeGet (pCh, &curr_opmode);
   /* Only exit when interrupted by a signal. When waiting was aborted by the
      timeout assume that no linemode change is pending and try to continue. */
   if (ret == DXS_statusSddEvtWaitInterrupt)
   {
      RETURN_STATUS (ret, IFX_NULL);
   }
   /* check if transition is valid */
   if (curr_opmode != DXS_SDD_Opmode_GR909)
   {
      /* GR909 is not running. So stopping is not needed.
         This is what was intended by the user and so not an error. */
      return DXS_statusOk;
   }

   /* Stop GR909 by changing operating mode to PDH */
   ret = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Disabled);
   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Read GR909 results

   \param  pLLChannel   Pointer to TAPI low level channel structure.
   \param  pResults     Pointer to result structure.

   \return
      DXS_statusOk or error code

   \remarks
   GR909 results are polled by the application after reception of the
   IFX_TAPI_GR909_RDY event
*/
IFX_int32_t DXS_TAPI_LL_ALM_LT_GR909_Result (IFX_TAPI_LL_CH_t *pLLChannel,
                                        IFX_TAPI_GR909_RESULT_t *pResults)
{
   DXS_CHANNEL_t           *pCh    = (DXS_CHANNEL_t *) pLLChannel;
   IFX_int32_t             ret;
   DXS_SDD_GR909Result_t   sdd_res;

   if((pCh == IFX_NULL) || (pCh->pParent == IFX_NULL))
      return DXS_statusErr;

   if(pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   memset(&sdd_res, 0, sizeof(sdd_res));
   sdd_res.CMD    = DXS_CMD_CMD_SDD;
   sdd_res.CHAN   = pCh->nChannel-1;
   sdd_res.ECMD   = DXS_SDD_GR909Result_ECMD;
   sdd_res.MOD    = DXS_CMD_MOD_SDD;
   sdd_res.LENGTH = DXS_SDD_GR909Result_LENGTH;
   ret = DXS_CmdRead(pCh->pParent,
      (IFX_uint32_t *)(IFX_void_t *)&sdd_res,
      (IFX_uint32_t *)(IFX_void_t *)&sdd_res);
   if (!DXS_SUCCESS (ret))
   {
      RETURN_STATUS(ret, IFX_NULL);
   }

   /* set validity information */
   pResults->valid  = sdd_res.HptValid  ? IFX_TAPI_GR909_HPT_VALID : 0;
   pResults->valid |= sdd_res.FemfValid ? IFX_TAPI_GR909_FEMF_VALID : 0;
   pResults->valid |= sdd_res.RftValid  ? IFX_TAPI_GR909_RFT_EXT_VALID : 0;
   pResults->valid |= sdd_res.RohValid  ? IFX_TAPI_GR909_ROH_VALID : 0;
   pResults->valid |= sdd_res.RitValid  ? IFX_TAPI_GR909_RIT_VALID : 0;

   /* set pass/fail information */
   pResults->passed  = sdd_res.HptPass  ? IFX_TAPI_GR909_HPT : 0;
   pResults->passed |= sdd_res.FemfPass ? IFX_TAPI_GR909_FEMF : 0;
   pResults->passed |= sdd_res.RftPass  ? IFX_TAPI_GR909_RFT : 0;
   pResults->passed |= sdd_res.RohPass  ? IFX_TAPI_GR909_ROH : 0;
   pResults->passed |= sdd_res.RitPass  ? IFX_TAPI_GR909_RIT : 0;

   /* extract results */
   if (sdd_res.HptValid  || sdd_res.FemfValid)
   {  /* copy results to to HPT and FEMF (not measured separately) */
      pResults->HPT_AC_R2G = pResults->FEMF_AC_R2G = sdd_res.HptAcR2g;
      pResults->HPT_AC_T2G = pResults->FEMF_AC_T2G = sdd_res.HptAcT2g;
      pResults->HPT_AC_T2R = pResults->FEMF_AC_T2R = sdd_res.HptAcT2r;
      pResults->HPT_DC_R2G = pResults->FEMF_DC_R2G = sdd_res.HptDcR2g;
      pResults->HPT_DC_T2G = pResults->FEMF_DC_T2G = sdd_res.HptDcT2g;
      pResults->HPT_DC_T2R = pResults->FEMF_DC_T2R = sdd_res.HptDcT2r;
   }
   if (sdd_res.RftValid)
   {
      pResults->RFT_R2G = sdd_res.RftR2g;
      pResults->RFT_T2G = sdd_res.RftT2g;
      pResults->RFT_T2R = sdd_res.RftT2r;
   }
   if (sdd_res.RohValid)
   {
      pResults->ROH_T2R_L = sdd_res.RohLow;
      pResults->ROH_T2R_H = sdd_res.RohHigh;
   }
   if (sdd_res.RitValid)
   {
      pResults->RIT_RES = sdd_res.RitRes;
   }

   pResults->dev_type = IFX_TAPI_GR909_DEV_DXS;

   pResults->OLR_T2R = pCh->pALM->nlt_ResistanceConfig.fOlResTip2Ring;
   pResults->OLR_T2G = pCh->pALM->nlt_ResistanceConfig.fOlResTip2Gnd;
   pResults->OLR_R2G = pCh->pALM->nlt_ResistanceConfig.fOlResRing2Gnd;

   TRACE(TAPI_DXS, DBG_LEVEL_LOW,
         ("GR909 results:\n"
         "valid:  0x%08X\n"
         "passed: 0x%08X\n"
         "HPT_AC_R2G: 0x%04X (dec %d), HPT_AC_T2G: 0x%04X (dec %d), HPT_AC_T2R: 0x%04X (dec %d)\n"
         "HPT_DC_R2G: 0x%04X (dec %d), HPT_DC_T2G: 0x%04X (dec %d), HPT_DC_T2R: 0x%04X (dec %d)\n"
         "FEMF_AC_R2G: 0x%04X (dec %d), FEMF_AC_T2G: 0x%04X (dec %d), FEMF_AC_T2R: 0x%04X (dec %d)\n"
         "FEMF_DC_R2G: 0x%04X (dec %d), FEMF_DC_T2G: 0x%04X (dec %d), FEMF_DC_T2R: 0x%04X (dec %d)\n"
         "RFT_R2G: 0x%04X (dec %d), RFT_T2G: 0x%04X (dec %d), RFT_T2R: 0x%04X (dec %d)\n"
         "ROH_T2R_L: 0x%04X (dec %d), ROH_T2R_H: 0x%04X (dec %d)\n"
         "RIT_RES: 0x%04X (dec %d)\n"
         "OLR_T2R: 0x%08X (dec %d), OLR_T2G: 0x%08X (dec %d), OLR_R2G: 0x%08X (dec %d)\n",
         pResults->valid,
         pResults->passed,
         pResults->HPT_AC_R2G, pResults->HPT_AC_R2G, pResults->HPT_AC_T2G, pResults->HPT_AC_T2G, pResults->HPT_AC_T2R, pResults->HPT_AC_T2R,
         pResults->HPT_DC_R2G, pResults->HPT_DC_R2G, pResults->HPT_DC_T2G, pResults->HPT_DC_T2G, pResults->HPT_DC_T2R, pResults->HPT_DC_T2R,
         pResults->FEMF_AC_R2G, pResults->FEMF_AC_R2G, pResults->FEMF_AC_T2G, pResults->FEMF_AC_T2G, pResults->FEMF_AC_T2R, pResults->FEMF_AC_T2R,
         pResults->FEMF_DC_R2G, pResults->FEMF_DC_R2G, pResults->FEMF_DC_T2G, pResults->FEMF_DC_T2G, pResults->FEMF_DC_T2R, pResults->FEMF_DC_T2R,
         pResults->RFT_R2G, pResults->RFT_R2G, pResults->RFT_T2G, pResults->RFT_T2G, pResults->RFT_T2R, pResults->RFT_T2R,
         pResults->ROH_T2R_L, pResults->ROH_T2R_L, pResults->ROH_T2R_H, pResults->ROH_T2R_H,
         pResults->RIT_RES, pResults->RIT_RES,
         pResults->OLR_T2R, pResults->OLR_T2R, pResults->OLR_T2G, pResults->OLR_T2G, pResults->OLR_R2G, pResults->OLR_R2G
         ));

   RETURN_STATUS(ret, IFX_NULL);
}
#endif /* DXS_FEAT_GR909 */


#ifdef DXS_FEAT_CAPACITANCE_MEASUREMENT
/**
   Check capacitance measurement support

   This function is used by phone detection to find if the chip supports the
   tip to ring capacitance measurement.

   \param  pLLChannel   Pointer to TAPI low level channel structure.
   \param  pSupported   Returns information whether capacitance measurement
                        is supported (IFX_TRUE) or not (IFX_FALSE).

   \return
   - DXS_statusOk
   - DXS_statusInvalCh
*/
IFX_int32_t DXS_TAPI_LL_ALM_LT_CheckCapMeasSup (IFX_TAPI_LL_CH_t *pLLChannel,
                                                IFX_boolean_t *pSupported)
{
   DXS_CHANNEL_t         *pCh  = (DXS_CHANNEL_t *) pLLChannel;
   DXS_DEVICE_t          *pDev;

   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pParent == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      RETURN_STATUS (DXS_statusInvalCh, IFX_NULL);
   }

   pDev = pCh->pParent;

   if (pSupported == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("%s: pSupported is NULL\n", __FUNCTION__));
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }

   *pSupported = (pDev->caps.bfw_CMeas == IFX_TRUE) ? IFX_TRUE : IFX_FALSE;

   return DXS_statusOk;
}


/**
   Capacitance measurement start

   \param  pLLChannel   Pointer to TAPI low level channel structure.
   \param  bTip2RingOnly  IFX_TRUE - measure only tip to ring capacitance

   \return
   - DXS_statusErr if channel pointer is null
   - DXS_statusFuncParam if at least one parameter in function is wrong
   - DXS_statusOk
   - DXS_statusInvalCh
   - status error from DXS_CmdWrite
*/
IFX_int32_t DXS_TAPI_LL_ALM_LT_CapMeasStart (IFX_TAPI_LL_CH_t *pLLChannel,
                                             IFX_boolean_t bTip2RingOnly)
{
   DXS_CHANNEL_t           *pCh  = (DXS_CHANNEL_t *) pLLChannel;
   DXS_DEVICE_t            *pDev;
   DXS_SDD_GR909Config_t   *p_ctrl;
   IFX_int32_t             ret = IFX_SUCCESS;

   if ((pCh == IFX_NULL) || (pCh->pParent == IFX_NULL))
      return DXS_statusErr;

   pDev = pCh->pParent;

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   p_ctrl = &pCh->pALM->sdd_gr909_config;

   /* Until the DC/DC converter type is set no linefeed change is allowed. */
   if (pCh->pALM->nDcDcType == DXS_DCDC_TYPE_NOTSET)
   {
      /* errmsg: Operation blocked until BBD containing DC/DC type setting is
                 downloaded  */
      RETURN_STATUS (DXS_statusBlockedNoDcDcType, IFX_NULL);
   }

   /* sanity check */
   if (pDev->caps.bfw_CMeas == 0)
   {
      RETURN_STATUS (DXS_statusNotSupported, IFX_NULL);
   }

   if (pCh->pALM->bCapMeasInProgress == IFX_TRUE)
   {
      /* errmsg: Capacitance measurement already in progress */
      RETURN_STATUS (DXS_statusCapMeasStartWhileActive, IFX_NULL);
   }
   /* clear previous results */
   memset (&(pCh->pALM->t2r_cap_result), 0,
           sizeof(IFX_TAPI_NLT_T2R_CAPACITANCE_RESULT_t));
   memset (&(pCh->pALM->l2g_cap_result), 0,
           sizeof(IFX_TAPI_NLT_L2GND_CAPACITANCE_RESULT_t));

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   /* implicit PDH opmode */
   ret = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Disabled);
   if (!DXS_SUCCESS (ret))
      goto exit;
   /* workaround: when the SLIC is in sleep the first opmode change command
      will only wake it up but actual setting of the opmode will take until
      the dup counters have expired. To speed up the process the opmode
      change can be repeated after the SLIC is awake. So do a delay here
      to allow enough time for the wake up. */
   TAPI_OS_MSecSleep(3);
   /* end of workaround */
   ret = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Disabled);
   if (!DXS_SUCCESS (ret))
      goto exit;
   /* workaround: Opmode Change event is not available on DXS.
      Wait 1 ms instead of polling the opmode */
   TAPI_OS_MSecSleep(1);
   /* end of workaround */

   ret = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)p_ctrl);
   if (!DXS_SUCCESS (ret))
      goto exit;

   /* set progress indicator */
   pCh->pALM->bCapMeasInProgress = IFX_TRUE;
   pCh->pALM->bCapMeasTip2RingOnly = bTip2RingOnly;

   /* start capacitance measurement */
   ret = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_CapMeas);
   if (!DXS_SUCCESS (ret))
   {
      pCh->pALM->bCapMeasInProgress = IFX_FALSE;
      pCh->pALM->bCapMeasTip2RingOnly = IFX_FALSE;
   }

exit:
   /* release channel */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Capacitance measurement stop

   \param  pLLChannel   Pointer to TAPI low level channel structure.

   \return
   - DXS_statusErr if channel pointer is null
   - DXS_statusOk
   - DXS_statusInvalCh
   - status error from DXS_CmdWrite
*/
IFX_int32_t DXS_TAPI_LL_ALM_LT_CapMeasStop (IFX_TAPI_LL_CH_t *pLLChannel)
{
   DXS_CHANNEL_t         *pCh  = (DXS_CHANNEL_t *) pLLChannel;
   IFX_int32_t            ret;
   IFX_uint8_t            curr_opmode;

   if(pCh == IFX_NULL)
      return DXS_statusErr;

   if(pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   /* Get the current opmode - wait while a opmode change is ongoing. */
   ret = DXS_ALM_OpmodeGet (pCh, &curr_opmode);
   /* Only exit when interrupted by a signal. When waiting was aborted by the
      timeout assume that no linemode change is pending and try to continue. */
   if (ret == DXS_statusSddEvtWaitInterrupt)
   {
      RETURN_STATUS (ret, IFX_NULL);
   }
   /* check if transition is valid */
   if (curr_opmode != DXS_SDD_Opmode_CapMeas)
   {
      /* Capacitance measurement is not running. So stopping is not needed.
         This is what was intended by the user and so not an error. */
      return DXS_statusOk;
   }

   /* Stop capacitance measurement by changing operating mode to PDH */
   ret = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Disabled);

   if (DXS_SUCCESS (ret))
   {
      pCh->pALM->bCapMeasInProgress = IFX_FALSE;
      pCh->pALM->bCapMeasTip2RingOnly = IFX_FALSE;
   }

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Get capacitance measurement result

   \param  pLLChannel   Pointer to the TAPI LL channel structure.

   \return
   - DXS_statusOk or status error from DXS_CmdRead
*/
IFX_int32_t DXS_TAPI_LL_ALM_LT_CapMeasResult (IFX_TAPI_LL_CH_t *pLLChannel)
{
   DXS_CHANNEL_t         *pCh  = (DXS_CHANNEL_t *) pLLChannel;
   IFX_TAPI_EVENT_t      tapiEvent;
   IFX_int32_t           ret;
   DXS_SDD_CapMeasRead_t *pCapMeas;

   pCapMeas = &pCh->pALM->fw_sdd_capacitance_meas;

   memset(&tapiEvent, 0, sizeof(tapiEvent));

   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   tapiEvent.ch = pCh->nChannel - 1;
   tapiEvent.id = IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY;
   DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_ALM);

   ret = DXS_CmdRead (pCh->pParent, (IFX_uint32_t *)(IFX_void_t *)pCapMeas,
      (IFX_uint32_t *)(IFX_void_t *)pCapMeas);

   if (DXS_SUCCESS (ret))
   {
      pCh->pALM->bCapMeasInProgress = IFX_FALSE;

      /* zero capacitance is an error */
      tapiEvent.data.lcap.nReturnCode =
            (pCapMeas->CapT2R == 0) ? IFX_ERROR : ret;
      tapiEvent.data.lcap.nCapacitance = pCapMeas->CapT2R;

      IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);

      /* Store result from measurement.
         Needed for IFX_TAPI_NLT_CAPACITANCE_RESULT_GET. */
      pCh->pALM->t2r_cap_result.bValidTip2Ring =
            (pCapMeas->CapT2R == 0) ? IFX_FALSE : IFX_TRUE;
      pCh->pALM->t2r_cap_result.nCapTip2Ring = pCapMeas->CapT2R;

      pCh->pALM->l2g_cap_result.bValidLine2Gnd =
            (pCapMeas->CapR2G == 0) || (pCapMeas->CapT2G == 0) ?
            IFX_FALSE : IFX_TRUE;
      pCh->pALM->l2g_cap_result.nCapRing2Gnd = pCapMeas->CapR2G;
      pCh->pALM->l2g_cap_result.nCapTip2Gnd = pCapMeas->CapT2G;

      if (pCh->pALM->bCapMeasTip2RingOnly == IFX_FALSE)
      {
         /* the event for the user interface */
         tapiEvent.id = IFX_TAPI_EVENT_NLT_END;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
      }
   }

   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Reads the results of the capacitance measurement. Used by the
   IFX_TAPI_NLT_CAPACITANCE_RESULT_GET command.

   \param  pLLChannel   Pointer to the TAPI LL channel structure.
   \param  pResult      Pointer to the IFX_TAPI_NLT_CAPACITANCE_RESULT_t.

   \return
   - DXS_statusOk
   - DXS_statusInvalCh
   - DXS_statusErr if channel pointer is null
   - DXS_statusFuncParam if at least one parameter in function is wrong
*/
IFX_int32_t DXS_TAPI_LL_ALM_NLT_Cap_Result(IFX_TAPI_LL_CH_t *pLLChannel,
                                     IFX_TAPI_NLT_CAPACITANCE_RESULT_t *pResult)
{
   DXS_CHANNEL_t                      *pCh  = (DXS_CHANNEL_t *) pLLChannel;

   if(pCh == IFX_NULL)
      return DXS_statusErr;

   if(pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   if(pResult == IFX_NULL)
      RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);

   /* Copy measurement results */
   pResult->bValidTip2Ring = pCh->pALM->t2r_cap_result.bValidTip2Ring;
   pResult->nCapTip2Ring = pCh->pALM->t2r_cap_result.nCapTip2Ring;
   pResult->bValidLine2Gnd = pCh->pALM->l2g_cap_result.bValidLine2Gnd ;
   pResult->nCapTip2Gnd = pCh->pALM->l2g_cap_result.nCapTip2Gnd;
   pResult->nCapRing2Gnd = pCh->pALM->l2g_cap_result.nCapRing2Gnd;
   pResult->fOlCapTip2Ring = pCh->pALM->nlt_CapacitanceConfig.fOlCapTip2Ring;
   pResult->fOlCapTip2Gnd = pCh->pALM->nlt_CapacitanceConfig.fOlCapTip2Gnd;
   pResult->fOlCapRing2Gnd = pCh->pALM->nlt_CapacitanceConfig.fOlCapRing2Gnd;

   return DXS_statusOk;
}
#endif /* DXS_FEAT_CAPACITANCE_MEASUREMENT */


#ifdef DXS_FEAT_CALIBRATION_STORAGE
/**
   This function is used to configure the open loop calibration factors
   of the measurement path for line testing. Used by the
   IFX_TAPI_NLT_CONFIGURATION_OL_SET command.

   \param  pLLChannel   Pointer to the TAPI LL channel structure.
   \param  pConfig      Pointer to the IFX_TAPI_NLT_CONFIGURATION_OL_t.

   \return
   - DXS_statusOk
   - DXS_statusInvalCh
   - DXS_statusErr if channel pointer is null
   - DXS_statusFuncParam if at least one parameter in function is wrong
 */
IFX_int32_t DXS_TAPI_LL_ALM_NLT_OLConfig_Set(
                              IFX_TAPI_LL_CH_t *pLLChannel,
                              const IFX_TAPI_NLT_CONFIGURATION_OL_t *pConfig)
{
   DXS_CHANNEL_t                      *pCh  = (DXS_CHANNEL_t *) pLLChannel;

   if(pCh == IFX_NULL)
      return DXS_statusErr;

   if(pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   if(pConfig == IFX_NULL)
   {
      /* errmsg: At least one parameter in function is wrong. */
      RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);
   }

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   /* Store configuration */
   pCh->pALM->nlt_ResistanceConfig.fOlResTip2Ring = pConfig->fOlResTip2Ring;
   pCh->pALM->nlt_ResistanceConfig.fOlResTip2Gnd = pConfig->fOlResTip2Gnd;
   pCh->pALM->nlt_ResistanceConfig.fOlResRing2Gnd = pConfig->fOlResRing2Gnd;
   pCh->pALM->nlt_CapacitanceConfig.fOlCapTip2Ring = pConfig->fOlCapTip2Ring;
   pCh->pALM->nlt_CapacitanceConfig.fOlCapTip2Gnd = pConfig->fOlCapTip2Gnd;
   pCh->pALM->nlt_CapacitanceConfig.fOlCapRing2Gnd = pConfig->fOlCapRing2Gnd;

   /* release channel */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);
   return DXS_statusOk;
}


/**
   This function is used to get the open loop calibration factors
   of the measurement path for line testing. Used by the
   IFX_TAPI_NLT_CONFIGURATION_OL_GET command.

   \param  pLLChannel   Pointer to the TAPI LL channel structure.
   \param  pConfig      Pointer to the IFX_TAPI_NLT_CONFIGURATION_OL_t.

   \return
   - DXS_statusOk
   - DXS_statusInvalCh
   - DXS_statusErr if channel pointer is null
   - DXS_statusFuncParam if at least one parameter in function is wrong
 */
IFX_int32_t DXS_TAPI_LL_ALM_NLT_OLConfig_Get(
                                       IFX_TAPI_LL_CH_t *pLLChannel,
                                       IFX_TAPI_NLT_CONFIGURATION_OL_t *pConfig)
{
   DXS_CHANNEL_t                      *pCh  = (DXS_CHANNEL_t *) pLLChannel;

   if(pCh == IFX_NULL)
      return DXS_statusErr;

   if(pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   if(pConfig == IFX_NULL)
      RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   /* read configuration */
   pConfig->fOlResTip2Ring = pCh->pALM->nlt_ResistanceConfig.fOlResTip2Ring;
   pConfig->fOlResTip2Gnd = pCh->pALM->nlt_ResistanceConfig.fOlResTip2Gnd;
   pConfig->fOlResRing2Gnd = pCh->pALM->nlt_ResistanceConfig.fOlResRing2Gnd;
   pConfig->fOlCapTip2Ring = pCh->pALM->nlt_CapacitanceConfig.fOlCapTip2Ring;
   pConfig->fOlCapTip2Gnd = pCh->pALM->nlt_CapacitanceConfig.fOlCapTip2Gnd;
   pConfig->fOlCapRing2Gnd = pCh->pALM->nlt_CapacitanceConfig.fOlCapRing2Gnd;
   pConfig->dev_type = IFX_TAPI_GR909_DEV_DXS;

   /* release channel */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);
   return DXS_statusOk;
}
#endif /* DXS_FEAT_CALIBRATION_STORAGE */


#ifdef DXS_FEAT_GR909
/**
   This function is used to configure the measurement path for line testing
   according to Rmes resistor.
   Used by the IFX_TAPI_NLT_CONFIGURATION_RMES_SET command.

   \param  pLLChannel   Pointer to the TAPI LL channel structure.
   \param  pConfig      Pointer to the IFX_TAPI_NLT_CONFIGURATION_OL_t.

   \return
   - DXS_statusOk or status error
 */
IFX_int32_t DXS_TAPI_LL_ALM_NLT_RmesConfig_Set(
                              IFX_TAPI_LL_CH_t *pLLChannel,
                              const IFX_TAPI_NLT_CONFIGURATION_RMES_t *pConfig)
{
   DXS_CHANNEL_t                      *pCh  = (DXS_CHANNEL_t *) pLLChannel;
   IFX_int32_t                        ret = DXS_statusOk;

   /* calling function should ensure valid parameters */
   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   if (pConfig == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("%s: pConfig is NULL\n", __FUNCTION__));
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   /* Set limit values according to Rmeas resistor. */
   ret = DXS_ALM_GR909_SetLimits(pCh, pConfig->nRmeas);
   if (TAPI_SUCCESS(ret))
      pCh->pALM->nRmeas = pConfig->nRmeas;

   /* release channel */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);
   RETURN_STATUS (ret, IFX_NULL);
}
#endif /* DXS_FEAT_GR909 */


/**
   Check capacitance measurement progress

   \param  pCh          DXS channel context.

   \return
   IFX_TRUE or IFX_FALSE

   \remarks
   Called from ISR
*/
IFX_boolean_t  DXS_ALM_CapMeasInProgress (DXS_CHANNEL_t *pCh)
{
#ifdef DXS_FEAT_CAPACITANCE_MEASUREMENT
   if(pCh == IFX_NULL)
   {
      return IFX_FALSE;
   }

   if (pCh->pALM == IFX_NULL)
   {
      return IFX_FALSE;
   }

   return pCh->pALM->bCapMeasInProgress;
#else
   return IFX_FALSE;
#endif /* DXS_FEAT_CAPACITANCE_MEASUREMENT */
}


/**
   Set default limits.

   \param  pCh          DXS channel context.
   \param  nRmeas       Rmeas variant connected to the chip.

   \return
   DXS_statusOk or status error
*/
IFX_int32_t  DXS_ALM_GR909_SetLimits (DXS_CHANNEL_t *pCh,
                                      IFX_TAPI_NLT_RMEAS_CFG_t nRmeas)
{
   IFX_int32_t            ret = DXS_statusOk;
   DXS_SDD_GR909Config_t  *p_ctrl = IFX_NULL;

   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   p_ctrl = &pCh->pALM->sdd_gr909_config;

   switch (nRmeas)
   {
      case IFX_TAPI_NLT_RMEAS_1MOHM:
         /* reset values are used */
         p_ctrl->HptW2gAcLim  = 0x1B9E;
         p_ctrl->HptW2wAcLim  = 0x1B9E;
         p_ctrl->HptW2gDcLim  = 0x34BC;
         p_ctrl->HptW2wDcLim  = 0x34BC;
         p_ctrl->FemfW2gAcLim = 0x0586;
         p_ctrl->FemfW2wAcLim = 0x0586;
         p_ctrl->FemfW2gDcLim = 0x0258;
         p_ctrl->FemfW2wDcLim = 0x0258;
         p_ctrl->RftResLim    = 0x249F;
         p_ctrl->RohLinLim    = 0x000F;
         p_ctrl->RitLowLim    = 0x0057;
         p_ctrl->RitHighLim   = 0x09AB;
         break;

      case IFX_TAPI_NLT_RMEAS_1_5MOHM:
      case IFX_TAPI_NLT_RMEAS_DEFAULT:
         /* 50Vrms */
         p_ctrl->HptW2gAcLim  = 7070;
         /* 50Vrms */
         p_ctrl->HptW2wAcLim  = 7070;
         /* 135V */
         p_ctrl->HptW2gDcLim  = 13500;
         /* 135V */
         p_ctrl->HptW2wDcLim  = 13500;
         /* 10Vrms */
         p_ctrl->FemfW2gAcLim = 1414;
         /* 10Vrms */
         p_ctrl->FemfW2wAcLim = 1414;
         /* 6V */
         p_ctrl->FemfW2gDcLim = 600;
         /* 6V */
         p_ctrl->FemfW2wDcLim = 600;
         /* 136.36kOhm corresponds to 150kOhm limit to ground and
            140kOhm limit tip to ring  */
         p_ctrl->RftResLim    = 8523;
         /* 15% */
         p_ctrl->RohLinLim    = 0x000F;
         /* 5REN (6930Ohm + 8uF at 20Hz) */
         p_ctrl->RitLowLim    = 0x0057;
         /* 0.175REN (6930Ohm + 8uF at 20Hz) */
         p_ctrl->RitHighLim   = 0x09AB;
         break;

      default:
         /** errmsg: Invalid Rmes value */
         ret = DXS_statusInvalidRmes;
         break;
   }

   return ret;
}

