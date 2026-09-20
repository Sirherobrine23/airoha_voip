/****************************************************************************

  Copyright 2014 Lantiq Deutschland GmbH
  Copyright 2022,2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

/**
   \file drv_dxs_cid.c
   This file contains the implementation of the functions for CID operations.
*/

/* ========================================================================= */
/*                                 Includes                                  */
/* ========================================================================= */

#include <drv_tapi_config.h>
#include "drv_dxs_api.h"

#ifdef DXS_FEAT_CID
#include "drv_dxs_cid_priv.h"
#include "drv_dxs_cid.h"
#include "drv_dxs_dtmf_priv.h"
#include "drv_dxs_dtmf.h"
#include "drv_dxs_pcm.h"
#include "drv_dxs_mbx.h"
#include "drv_dxs_alm_priv.h"

/* ========================================================================= */
/*                             Macro definitions                             */
/* ========================================================================= */

/* ========================================================================= */
/*                             Type definitions                              */
/* ========================================================================= */

/* ========================================================================= */
/*                             Global variables                              */
/* ========================================================================= */

/* ========================================================================= */
/*                           Function prototypes                             */
/* ========================================================================= */
static IFX_int32_t DXS_CID_SetCidCoeff (DXS_CHANNEL_t *pCh,
                                        IFX_TAPI_CID_HOOK_MODE_t cidHookMode,
                                        const IFX_TAPI_CID_FSK_CFG_t *pFskConf);


/* ========================================================================= */
/*                         Function implementation                           */
/* ========================================================================= */
/**
   Disables or Enables CID Sender according to bEn

   \param   pCh  - pointer to DXS channel structure

   \param   bEn  - IFX_TRUE : enable / IFX_FALSE : disable

   \return
   - DXS_statusOk
   - DXS_statusCidCtrlErr
*/
IFX_int32_t DXS_CID_CidGenCtrl (DXS_CHANNEL_t *pCh, IFX_boolean_t bEn)
{
   IFX_int32_t          err   = IFX_SUCCESS;
   DXS_DEVICE_t         *pDev = pCh->pParent;
   DXS_CID_GEN_CTRL_t   *pCidGenCtrl = &pCh->pCID->cid_gen_ctrl;

   if ((bEn == IFX_TRUE) && (pCidGenCtrl->EN == CID_GEN_CTRL_EN_DIS))
   {
      pCidGenCtrl->EN = CID_GEN_CTRL_EN_EN;

      /* only when cid is activated, bellcore or ITU spec is set */
      if (pCh->pCID->cidSend.nCidDataType == IFX_TAPI_CID_DATA_TYPE_FSK_BEL202)
         pCh->pCID->cid_gen_ctrl.V23 = CID_GEN_CTRL_V23_V23_BEL202;
      else
         pCh->pCID->cid_gen_ctrl.V23 = CID_GEN_CTRL_V23_V23_ITU_T;

      /* mute the voice path */
      err = DXS_PCM_ChRxMute (pCh, IFX_ENABLE);
   }
   else if ((bEn == IFX_FALSE) && (pCidGenCtrl->EN == CID_GEN_CTRL_EN_EN))
   {
      pCidGenCtrl->EN = CID_GEN_CTRL_EN_DIS;

      /* unmute the voice path */
      err = DXS_PCM_ChRxMute (pCh, IFX_DISABLE);
   }

   if (DXS_SUCCESS (err))
   {
      err = DXS_CmdWrite (pDev, (IFX_uint32_t *)(IFX_void_t *)pCidGenCtrl);
   }
   if (DXS_statusOk != err)
   {
      /* errmsg: Changing the state of the CID generator failed.*/
      RETURN_STATUS(DXS_statusCidCtrlErr, IFX_NULL);
   }
   else
   {
      RETURN_STATUS(DXS_statusOk, IFX_NULL);
   }
}

/**
   Set the CID sender coefficients

   \param pCh           pointer to DXS channel structure

   \param cidHookMode   cid hook mode as specified in IFX_TAPI_CID_HOOK_MODE_t

   \param pFskConf      handle to IFX_TAPI_CID_FSK_CFG_t structure

   \param nSize         size of CID data to send, used to set the BRS level.

   \return
   - DXS_statusOk
   - DXS_statusCidSetCoefErr

   \remark
   - The CID sender must be disabled before programming the coefficients, what
     is done in this function
*/
static IFX_int32_t DXS_CID_SetCidCoeff (DXS_CHANNEL_t *pCh,
                                        IFX_TAPI_CID_HOOK_MODE_t cidHookMode,
                                        const IFX_TAPI_CID_FSK_CFG_t *pFskConf)
{
   IFX_int32_t          err = DXS_statusOk;
   DXS_DEVICE_t         *pDev = pCh->pParent;
   IFX_uint16_t         ch = (pCh->nChannel - 1);
   DXS_CID_GEN_COEF_t   *pCidGenCoef = &pCh->pCID->cid_gen_coef;

   /* protect fwmsg against concurrent tasks */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   /* to set the coefficients the cid generator must be inactive */
   err = DXS_CID_CidGenCtrl (pCh, IFX_FALSE);

   if (err == DXS_statusOk)
   {
      /* set the new coefficients */
      pCidGenCoef->CHAN    = ch;
      /* write all three parameters */
      pCidGenCoef->LENGTH  = CID_GEN_COEF_LENGTH;

      /* Convert the level value given in 0.1dB steps to FW value. */
      pCidGenCoef->LEVEL =
         DXS_ALM_rx_level_convert(pCh, pFskConf->levelTX / 10);

      /* set seizure and mark according to CID type */
      switch (cidHookMode)
      {
         /* offhook CID, called CID type 2, NTT inclusive */
         case  IFX_TAPI_CID_HM_OFFHOOK:
            /* set Seizure - off hook CID has always 0 seizure bits */
            pCidGenCoef->SEIZURE = 0;
            /* set Mark */
            pCidGenCoef->MARK = pFskConf->markTXOffhook;
            break;
         /* onhook CID, called CID type 1, NTT inclusive */
         case IFX_TAPI_CID_HM_ONHOOK:
            /* set Seizure */
            pCidGenCoef->SEIZURE = pFskConf->seizureTX;
            /* set Mark */
            pCidGenCoef->MARK = pFskConf->markTXOnhook;
            break;
         default:
            break;
      }
      err = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)pCidGenCoef);
   }
   if (DXS_statusOk != err)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("CID Coefficients setting failed\n"));
   }

   /* remove protection of fwmsg against concurrent tasks */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   if (DXS_statusOk != err)
   {
      /* errmsg: Setting coefficients for CID failed.*/
      RETURN_STATUS(DXS_statusCidSetCoefErr,IFX_NULL);
   }
   else
   {
      RETURN_STATUS(DXS_statusOk,IFX_NULL);
   }
}

/**
   Start CID data transmission

   This function is non blocking. It handles all necessary steps to transmit
   CID data by either DTMF or FSK. It returns an error if a CID transmission
   is already running.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  pCidData     Pointer to CID TX configuration structure.

   \return Return value according to DXS_status_t
   - DXS_statusOk
   - DXS_statusInvalCh
   - DXS_statusDtmfAct
   - DXS_statusDtmfTimingErr
   - DXS_statusCidStdNotSupported
   - DXS_statusCidStartSeqErr
   - DXS_statusErr if channel pointer is null
   - DXS_statusFuncParam if at least one parameter in function is wrong

   \remarks
   This function is non blocking. It handles all required steps to transmit a
   CID. It returns an error if a CID transmission is already running.
*/
IFX_int32_t DXS_TAPI_LL_CID_TX_Start (IFX_TAPI_LL_CH_t *pLLChannel,
                                      IFX_TAPI_CID_TX_t const *pCidData)
{
   DXS_CHANNEL_t  *pCh  = (DXS_CHANNEL_t *) pLLChannel;
   IFX_int32_t    err   = DXS_statusOk;

   if(pCh == IFX_NULL)
      return DXS_statusErr;

   if(pCh->pCID == IFX_NULL)
      RETURN_STATUS(DXS_statusInvalCh, IFX_NULL);

   if(pCidData == IFX_NULL)
      RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);

   /* remember the data type for stopping the correct machine */
   pCh->pCID->cidSend.nCidDataType = pCidData->cidDataType;

   switch (pCidData->cidDataType)
   {
      case IFX_TAPI_CID_DATA_TYPE_FSK_BEL202:
      case IFX_TAPI_CID_DATA_TYPE_FSK_V23:
         if (pCh->pDTMF->dtmf_at_gen_ctrl.EN == DTMF_AT_GEN_CTRL_EN_EN)
         {
            TRACE(TAPI_DXS, DBG_LEVEL_LOW,
                  ("DXS ERROR: Cid Sender cannot be used while DTMF/AT is active!\n"));
            /* errmsg: A DTMF transmission is active.*/
            RETURN_STATUS(DXS_statusDtmfAct, IFX_NULL);
         }

         if (pCh->pCID->cidSend.state_cid == DXS_CID_STATE_SETUP)
         {
            /* prevent data buffer overrun */
            if ( pCidData->nCidParamLen > IFX_TAPI_CID_TX_SIZE_MAX )
            {
               TRACE(TAPI_DXS, DBG_LEVEL_LOW,
                     ("DXS ERROR: Too much data for CID sender buffer - aborting\n"));
               /* errmsg: Parameter is out of range*/
               RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);
            }
            /* length in bytes */
            pCh->pCID->cidSend.nCidCnt = pCidData->nCidParamLen;
            memcpy(pCh->pCID->cidSend.pCid,
                   pCidData->pCidParam, pCidData->nCidParamLen);

            /* set callerID coefficients (level, seizure, mark bits) */
            err = DXS_CID_SetCidCoeff (pCh, pCidData->txHookMode,
                                       pCidData->pFskConf);

            /* now start non blocking low level machine */
            if (DXS_statusOk == err)
            {
               if (pCidData->cidDataType == IFX_TAPI_CID_DATA_TYPE_FSK_BEL202)
               {
                  /* Bellcore specification shall be used */
                  pCh->pCID->cid_gen_ctrl.V23 = CID_GEN_CTRL_V23_V23_BEL202;
               }
               else
               {
                  /* ITU-T V.23 specification shall be used */
                  pCh->pCID->cid_gen_ctrl.V23 = CID_GEN_CTRL_V23_V23_ITU_T;
               }
               /* call the caller ID state handler */
               err = DXS_CID_SH (pCh, DXS_CID_EVT_START);
            }
         }
         else
         {
            /* errmsg: A CID transmission is already active.*/
            RETURN_STATUS(DXS_statusCidAct, IFX_NULL);
         }
         break;

      case IFX_TAPI_CID_DATA_TYPE_DTMF:
      {
         if (pCh->pDTMF->dtmfSend.state_dtmf == DXS_DTMF_STATE_SETUP)
         {
            IFX_uint8_t pData[DXS_DTMF_MAX_BYTES] = {0};
            IFX_uint16_t i = 0;

            /* check digit and interdigit times */
            if ((pCidData->pDtmfConf->digitTime > DXS_DTMF_MAX_DIGIT_TIME) ||
                (pCidData->pDtmfConf->interDigitTime > DXS_DTMF_MAX_INTERDIGIT_TIME))
            {
               TRACE(TAPI_DXS, DBG_LEVEL_LOW, 
                     ("DXS ERROR: Digit time or inter digit time exceeds limit of 127 ms\n"));
               /* errmsg: DTMF digit or interdigit timing invalid.*/
               RETURN_STATUS(DXS_statusDtmfTimingErr, IFX_NULL);
            }

            /* prevent data buffer overrun */
            if ( pCidData->nCidParamLen > DXS_DTMF_MAX_BYTES )
            {
               TRACE(TAPI_DXS, DBG_LEVEL_LOW,
                     ("DXS ERROR: Too much data for CID sender buffer - aborting\n"));
               /* errmsg: Parameter is out of range*/
               RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);
            }

            /* store DTMF data in channel structure */
            pCh->pDTMF->dtmfSend.nDtmfCnt = pCidData->nCidParamLen;

            memcpy(pData,
                   pCidData->pCidParam, (IFX_uint32_t)pCidData->nCidParamLen);
            pCh->pDTMF->dtmfSend.digitTime      = pCidData->pDtmfConf->digitTime;
            pCh->pDTMF->dtmfSend.interDigitTime = pCidData->pDtmfConf->interDigitTime;

            /*
               transcode DTMF digits to DUSLIC XS specific setting:
               for each digit a table index is retrieved, which can be used to get
               all required settings for the corresponding DTMF digit
            */
            for (i = 0; i < pCh->pDTMF->dtmfSend.nDtmfCnt; i++)
               DXS_DTMF_GetTblIndex (pData[i], pData + i);

            memcpy(pCh->pDTMF->dtmfSend.pDtmfDigTblIndex, pData,
                   (IFX_uint32_t)pCh->pDTMF->dtmfSend.nDtmfCnt);

            err = DXS_DTMF_SH(pCh, DXS_DTMF_EVT_START);
         }
         else
         {
            /* errmsg: A DTMF transmission is active.*/
            RETURN_STATUS(DXS_statusDtmfAct, IFX_NULL);
         }
         break;
      }
      default:
         /* errmsg: CID standard not supported.*/
         RETURN_STATUS(DXS_statusCidStdNotSupported, IFX_NULL);
   }

   if (DXS_SUCCESS (err))
   {
      return DXS_statusOk;
   }
   else
   {
      /* errmsg: Initiating a CID sequence failed */
      RETURN_STATUS(DXS_statusCidStartSeqErr, IFX_NULL);
   }
}

/**
   Stop CID data transmission

   \param pLLChannel      Handle to TAPI low level channel structure

   \return
   - DXS_statusCidStdNotSupported
   - DXS_statusCidTxStopErr
   - DXS_statusInvalCh
   - DXS_statusErr if channel pointer is null
   - DXS_statusOk if successful
*/
IFX_int32_t DXS_TAPI_LL_CID_TX_Stop (IFX_TAPI_LL_CH_t *pLLChannel)
{
   IFX_int32_t    err   = DXS_statusOk;
   DXS_CHANNEL_t  *pCh  = (DXS_CHANNEL_t *) pLLChannel;

   TRACE(TAPI_DXS, DBG_LEVEL_LOW, ("DXS_INFO: %s\n", __FUNCTION__));

   if(pCh == IFX_NULL)
      return DXS_statusErr;

   if(pCh->pCID == IFX_NULL)
      RETURN_STATUS(DXS_statusInvalCh, IFX_NULL);

   switch (pCh->pCID->cidSend.nCidDataType)
   {
      case IFX_TAPI_CID_DATA_TYPE_FSK_BEL202:
      case IFX_TAPI_CID_DATA_TYPE_FSK_V23:
         if (pCh->pCID->cidSend.state_cid == DXS_CID_STATE_TRANSMIT)
         {
            /* flag a fake offhook to stop the FSK state machine */
            err = DXS_CID_SH (pCh, DXS_CID_EVT_OFFHOOK);
         }
         break;
      case IFX_TAPI_CID_DATA_TYPE_DTMF:
         if(pCh->pDTMF == IFX_NULL)
         {
            RETURN_STATUS(DXS_statusInvalCh, IFX_NULL);
         }

         if ((pCh->pDTMF->dtmfSend.state_dtmf == DXS_DTMF_STATE_TRANSMIT) ||
             (pCh->pDTMF->dtmfSend.state_dtmf == DXS_DTMF_STATE_PAUSE))
         {
            /* stop dtmf transmission */
            err = DXS_DTMF_SH (pCh, DXS_DTMF_EVT_STOP);
         }
         break;
      default:
         /* errmsg: CID standard not supported.*/
         RETURN_STATUS(DXS_statusCidStdNotSupported,IFX_NULL);
   }

   if (DXS_SUCCESS (err))
      return err;
   else
      /* errmsg: CID Tx could not be stopped.*/
      RETURN_STATUS(DXS_statusCidTxStopErr, IFX_NULL);
}

/**
   Initialize the cached firmware message for the callerID generator.

   \param  pCh          Pointer to the channel structure.

   \return
   None.
*/
IFX_void_t DXS_CID_InitCh (DXS_CHANNEL_t *pCh)
{
   DXS_CID_GEN_DATA_t  *pCidGenData   = IFX_NULL;
   DXS_CID_GEN_COEF_t  *pCidGenCoef   = IFX_NULL;
   DXS_CID_GEN_CTRL_t  *pCidGenCtrl   = IFX_NULL;
   IFX_uint8_t          ch            = pCh->nChannel - 1;

   /* initialize FW message for activation/deactivation of dtmf/at generator */
   pCidGenCtrl = &pCh->pCID->cid_gen_ctrl;
   memset (pCidGenCtrl, 0, sizeof (DXS_CID_GEN_CTRL_t));

   pCidGenCtrl->CMD        = DXS_CMD_CMD_EOP;
   pCidGenCtrl->CHAN       = ch;
   pCidGenCtrl->MOD        = DXS_CMD_MOD_SIG_GEN;
   pCidGenCtrl->ECMD       = CID_GEN_CTRL_ECMD_EOP_CIDSEND;
   pCidGenCtrl->LENGTH     = CID_GEN_CTRL_LENGTH;
   pCidGenCtrl->EN         = CID_GEN_CTRL_EN_DIS;
   pCidGenCtrl->AD         = CID_GEN_CTRL_AD_OFF;
   pCidGenCtrl->HLEV       = CID_GEN_CTRL_HLEV_HLEV_HIGH;
   pCidGenCtrl->V23        = CID_GEN_CTRL_V23_V23_BEL202;

   /* initialize FW message for configuration of dtmf/at signal levels */
   pCidGenCoef = &pCh->pCID->cid_gen_coef;
   memset (pCidGenCoef, 0, sizeof (DXS_CID_GEN_COEF_t));

   pCidGenCoef->CMD        = DXS_CMD_CMD_EOP;
   pCidGenCoef->CHAN       = ch;
   pCidGenCoef->MOD        = DXS_CMD_MOD_SIG_GEN;
   pCidGenCoef->ECMD       = CID_GEN_COEF_ECMD_EOP_CIDS_COEFF;
   pCidGenCoef->LENGTH     = CID_GEN_COEF_LENGTH;
   pCidGenCoef->LEVEL      = CID_GEN_COEF_LEVEL_RESET;
   pCidGenCoef->SEIZURE    = CID_GEN_COEF_SEIZURE_RESET;
   pCidGenCoef->MARK       = CID_GEN_COEF_MARK_RESET;

   /* initialize FW message for configuration of dtmf/at frequencies */
   pCidGenData = &pCh->pCID->cid_gen_data;
   memset (pCidGenData, 0, sizeof (DXS_CID_GEN_DATA_t));

   pCidGenData->CMD     = DXS_CMD_CMD_EOP;
   pCidGenData->CHAN    = ch;
   pCidGenData->MOD     = DXS_CMD_MOD_SIG_GEN;
   pCidGenData->ECMD    = CID_GEN_DATA_ECMD_CIDS_DATA;
   pCidGenData->LENGTH  = 0;
}

/**
   Allocate data structures of the CID module for the given channel

   \param  pCh          Pointer to the channel structure.

   \return
   - DXS_statusOk
   - DXS_statusNoMem    in case the stucture could not be created

   \remarks The channel parameter is not checked because the calling
   function assures correct values.
*/
IFX_int32_t DXS_CID_Allocate_Ch_Structures (DXS_CHANNEL_t *pCh)
{
   DXS_CID_Free_Ch_Structures (pCh);

   pCh->pCID = TAPI_OS_Malloc(sizeof(*pCh->pCID));
   if (pCh->pCID == IFX_NULL)
   {
      /* errmsg: No memory could be allocated. */
      RETURN_STATUS(DXS_statusNoMem, IFX_NULL);
   }
   memset(pCh->pCID, 0, sizeof(*pCh->pCID));

   return DXS_statusOk;
}

/**
   Free data structure of the CID module in the given channel.

   \param  pCh          Pointer to the channel structure.
*/
IFX_void_t DXS_CID_Free_Ch_Structures (DXS_CHANNEL_t *pCh)
{
   if (pCh->pCID != IFX_NULL)
   {
      TAPI_OS_Free(pCh->pCID);
      pCh->pCID = IFX_NULL;
   }
}

/**
   Return the remaining byte count in the CID buffer.

   \param  pCh          Pointer to the channel structure.
   \return              Remaining byte count.
*/
IFX_int8_t DXS_CID_GetRemainingBytes (DXS_CHANNEL_t *pCh)
{
   IFX_int8_t remaining;

   /* protect fwmsg against concurrent tasks */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);
   remaining = (pCh->pCID->cidSend.nCidCnt - pCh->pCID->cidSend.nPos);
   TAPI_OS_MutexRelease(&pCh->mtxChAcc);

   return remaining;
}

/* ========================================================================= */
/*                         Function pointer exports                          */
/* ========================================================================= */
#endif /* DXS_FEAT_CID */
