/****************************************************************************

  Copyright 2014 Lantiq Deutschland GmbH
  Copyright 2022 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

/**
   \file drv_dxs_cid_sm.c
   This file contains the implementation of the functions for the CID
   state machine.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include <drv_tapi_config.h>
#include "drv_dxs_api.h"

#ifdef DXS_FEAT_CID
#include "drv_dxs_cid_priv.h"
#include "drv_dxs_cid.h"
#include "drv_dxs_mbx.h"
#include "drv_dxs_access.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
typedef IFX_int32_t (*DXS_CID_STATE_HANDLER) (DXS_CHANNEL_t *pCh,
                                              DXS_CID_EVT_t evt);

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_int32_t dxs_CID_SH_Setup (DXS_CHANNEL_t *pCh, DXS_CID_EVT_t evt);
static IFX_int32_t dxs_CID_SH_Transmit (DXS_CHANNEL_t *pCh, DXS_CID_EVT_t evt);
static IFX_int32_t dxs_CID_SH_TransmitEnd (DXS_CHANNEL_t *pCh,
                                           DXS_CID_EVT_t evt);

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */
static DXS_CID_STATE_HANDLER  gDxsCidStates[DXS_CID_STATE_NO_STATES] = {
                                 dxs_CID_SH_Setup,
                                 dxs_CID_SH_Transmit,
                                 dxs_CID_SH_TransmitEnd};

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */
/**
   CID state machine handler for the state DXS_CID_STATE_SETUP

   \param pCh     pointer to a DXS channel structure

   \param evt     event triggering the state machine

   \return
   - DXS_statusOk
   - DXS_statusCidShSetupErr
   - DXS_statusErr in case of unknown event
*/
static IFX_int32_t dxs_CID_SH_Setup (DXS_CHANNEL_t *pCh, DXS_CID_EVT_t evt)
{
   IFX_int32_t    err = DXS_statusOk;

   switch (evt)
   {
      case DXS_CID_EVT_START:
         pCh->pCID->cidSend.state_cid = DXS_CID_STATE_TRANSMIT;
         /* Reset the Tx data position */
         pCh->pCID->cidSend.nPos     = 0;
         /*
            disable the autodeactivation bit, which might have been set in
            a previous CID transmission;
         */
         pCh->pCID->cid_gen_ctrl.AD = CID_GEN_CTRL_AD_OFF;
         /*
            now activate the CID generator, the state machine is triggered
            the next time with IRQ CIS_REQ;
         */
         err = DXS_CID_CidGenCtrl (pCh, IFX_TRUE);

         if (DXS_statusOk != err)
         {
            /* allow CID setup on the next run */
            pCh->pCID->cidSend.state_cid = DXS_CID_STATE_SETUP;
            pCh->pCID->cidSend.nCidCnt = 0;
         }
         break;

      case DXS_CID_EVT_OFFHOOK:
         /* do nothing */
         break;

      default:
         err = DXS_statusErr;
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("DXS ERROR: Illegal event type for CID_SH_Setup\n"));
         break;
   }

   if (!DXS_SUCCESS (err))
   {
      /* errmsg: CID state handler error from state CID setup.*/
      RETURN_STATUS(DXS_statusCidShSetupErr, IFX_NULL);
   }
   else
      return DXS_statusOk;
}

/**
   CID state machine handler for the state DXS_CID_STATE_TRANSMIT

   \param pCh     pointer to a DXS channel structure

   \param evt     event triggering the state machine

   \return
   - DXS_statusOk
   - DXS_statusCidShTransmitErr
   - DXS_statusErr in case of unknown event
*/
static IFX_int32_t dxs_CID_SH_Transmit (DXS_CHANNEL_t *pCh, DXS_CID_EVT_t evt)
{
   IFX_int32_t          err   = DXS_statusOk;
   DXS_DEVICE_t         *pDev = pCh->pParent;

   switch (evt)
   {
      case DXS_CID_EVT_CIS_REQ:
      {
         DXS_CID_GEN_DATA_t   *pCidGenData   = &pCh->pCID->cid_gen_data;
         IFX_uint16_t         nPos           = pCh->pCID->cidSend.nPos;
         IFX_uint16_t         nRemainingCIDBytes,
                              nBytesToSendNow;

         /* calculate the remaining number of bytes */
         nRemainingCIDBytes = pCh->pCID->cidSend.nCidCnt -
                              pCh->pCID->cidSend.nPos;

         /* define how much bytes can be sent, limits apply */
         if (nRemainingCIDBytes > DXS_CID_GEN_DATA_MAX)
            nBytesToSendNow = DXS_CID_GEN_DATA_MAX;
         else
            nBytesToSendNow = nRemainingCIDBytes;

         /* Copy the data from byte string into the 32-bit word message
            with an endianess safe function. */
         DXS_cpb2dw ((IFX_uint32_t *)pCidGenData, 4 /*header*/ + 1 /*NRDATA*/,
                     &(pCh->pCID->cidSend.pCid[nPos]), nBytesToSendNow);
         /* Set the number of CID bytes to be transmitted.
            Keep after the copy above because this zeros the NRDATA field. */
         pCidGenData->NRDATA = nBytesToSendNow;
         /* Set the command payload length. */
         pCidGenData->LENGTH = 4 + (nBytesToSendNow / 4) * 4;

         /* write command into the mailbox */
         err = DXS_CmdWrite (pDev, (IFX_uint32_t *)(IFX_void_t *)pCidGenData);

         if (DXS_statusOk == err)
         {
            /* advance the pos variable */
            pCh->pCID->cidSend.nPos += nBytesToSendNow;
            if (pCh->pCID->cidSend.nPos >= pCh->pCID->cidSend.nCidCnt)
            {
               /*
                  complete CID data has been sent to device ->
                  state machine transition to TRANSMIT_END, the cid generator
                  is disabled with the next CIS_REQ IRQ
               */
               pCh->pCID->cidSend.state_cid = DXS_CID_STATE_TRANSMIT_END;
            }
         }
         break;
      }

      /*
         for both events buffer underflow and offhook the cid transmission
         must be aborted; set state to DXS_CID_STATE_SETUP afterwards
      */
      case DXS_CID_EVT_OFFHOOK:
         /* make sure that autodeactivation bit is not set */
         pCh->pCID->cid_gen_ctrl.AD = CID_GEN_CTRL_AD_OFF;
         /*
            offhook during transmission -> deactivate the CID generator
         */
         err = DXS_CID_CidGenCtrl (pCh, IFX_FALSE);
         pCh->pCID->cidSend.state_cid = DXS_CID_STATE_SETUP;
         break;

      case DXS_CID_EVT_CIS_BUF:
         /* make sure that autodeactivation bit is not set */
         pCh->pCID->cid_gen_ctrl.AD = CID_GEN_CTRL_AD_OFF;
         /*
            error in transmission -> deactivate the CID generator
         */
         err = DXS_CID_CidGenCtrl (pCh, IFX_FALSE);
         pCh->pCID->cidSend.state_cid = DXS_CID_STATE_SETUP;
         break;

      default:
         err = DXS_statusErr;
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("DXS ERROR: Illegal event type for DXS_CID_SH_Transmit\n"));
         break;
   }

   if (!DXS_SUCCESS (err))
   {
      /* errmsg: CID state handler error from state CID transmit.*/
      RETURN_STATUS(DXS_statusCidShTransmitErr, IFX_NULL);
   }
   else
      return DXS_statusOk;
}

/**
   CID state machine handler for the state DXS_CID_STATE_TRANSMIT_END

   \param pCh     pointer to a DXS channel structure

   \param evt     event triggering the state machine

   \return
   - DXS_statusOk
   - DXS_statusCidShTransmitEndErr
   - DXS_statusErr in case of unknown event
*/
static IFX_int32_t dxs_CID_SH_TransmitEnd (DXS_CHANNEL_t *pCh,
                                           DXS_CID_EVT_t evt)
{
   IFX_int32_t          err = DXS_statusOk;

   switch (evt)
   {
      case DXS_CID_EVT_CIS_REQ:
         /*
            no more data is available -> do nothing here, we have to wait
            for CIS_BUF before deactivating the CID generataor
          */
         break;
      case DXS_CID_EVT_CIS_BUF:
         /*
            Buffer Underflow. This means that transmission is complete and
            the CID generator must be deactivated with autodeactivation enabled.
            Autodeactivation is required, because the current implementation
            in FW sets CIS_BUF after the last data bit has been sent.
            If the CID generator would be deactivated immediately, the so-called
            stop bit, which must be sent after each data byte, would not be
            transmitted and the CID transmission would be incomplete.
            The transmission of the stop bit requires 833us.
         */
         pCh->pCID->cid_gen_ctrl.AD = CID_GEN_CTRL_AD_ON;
         err = DXS_CID_CidGenCtrl (pCh, IFX_FALSE);
         pCh->pCID->cidSend.state_cid = DXS_CID_STATE_SETUP;
         break;

      case DXS_CID_EVT_OFFHOOK:
         /* just deactivate the CID generator */
         err = DXS_CID_CidGenCtrl (pCh, IFX_FALSE);
         pCh->pCID->cidSend.state_cid = DXS_CID_STATE_SETUP;
         break;

      default:
         err = DXS_statusErr;
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("DXS ERROR: Illegal event type for DXS_CID_SH_TransmitEnd\n"));
         break;
   }
   if (!DXS_SUCCESS (err))
   {
      /* errmsg: CID state handler error from state CID transmit end.*/
      RETURN_STATUS(DXS_statusCidShTransmitEndErr, IFX_NULL);
   }
   else
      return DXS_statusOk;
}

/**
   CID state machine handler

   \param pCh  pointer to a DXS channel structure

   \param evt  event triggering the state machine

   \return
   - DXS_statusOk
   - DXS_statusCidShErr
*/
IFX_int32_t DXS_CID_SH (DXS_CHANNEL_t *pCh, DXS_CID_EVT_t evt)
{
   IFX_int32_t    err = DXS_statusOk;

   /* protect channel data against concurrent tasks */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);
   err = gDxsCidStates[pCh->pCID->cidSend.state_cid](pCh, evt);
   /* unlock */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   if (!DXS_SUCCESS (err))
   {
      /* errmsg: CID state handler error.*/
      RETURN_STATUS (DXS_statusCidShErr, IFX_NULL);
   }
   else
      return DXS_statusOk;
}
#endif /* DXS_FEAT_CID */
