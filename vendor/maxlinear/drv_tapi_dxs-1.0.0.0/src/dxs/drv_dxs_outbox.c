/******************************************************************************

  Copyright 2014-2015 Lantiq Deutschland GmbH
  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.
  Copyright 2021-2023 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_outbox.c
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_mbx.h"
#include "drv_dxs_errno.h"
#include "drv_dxs_fw_cmd_sdd.h"
#include "drv_dxs_cid.h"
#include "drv_dxs_dtmf.h"
#include "drv_dxs_access.h"
#include "drv_dxs_outbox.h"

#ifdef DXS_FEAT_NLT
   #include "drv_dxs_alm.h"
   #include "drv_dxs_alm_priv.h"
#endif
/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* Event message header CMD values */
#define EVT_CMD_EVT        9

/* Event message header MOD values */
#define EVT_MOD_ALI        1
#define EVT_MOD_SYS        7

/* Event message header ECMD values for ALI events */
#define EVT_ECMD_SDD       4
#define EVT_ECMD_SIG       5

/* Event message header ECMD values for SYS events */
#define EVT_ECMD_BOOTFIN   0
#define EVT_ECMD_CMDERR    2
#define EVT_ECMD_INTERR   11

/* Internal Error events */
/**  Internal event queue overflow */
#define EVT_INTERR_EQO           1
/**  Clock fail */
#define EVT_INTERR_CLK_FAIL      2
/**  Synchronisation error */
#define EVT_INTERR_SYNC_FAIL     3

/* SDD events */
/** Over temperature detected */
#define EVT_SDD_OTEMP            0
/** Line testing finished */
#define EVT_SDD_LT_FIN           1
/** Ground fault detected */
#define EVT_SDD_GF               3
/** Ground key detected */
#define EVT_SDD_GK               4
/** Opmode changed */
#define EVT_SDD_OPC              5
/** Error in command SDD_CoeffReadConfig */
#define EVT_SDD_CORCE            6
/** Error in command SDD_Coeff */
#define EVT_SDD_COEFE            7
/** On-hook */
#define EVT_SDD_ONH              9
/** Off-hook */
#define EVT_SDD_OFFH             10
/** Ground Fault Finished */
#define EVT_SDD_GF_FIN           12
/** Ground Key Finished */
#define EVT_SDD_GK_FIN           13
/** Overtemp finished which means that
   SLIC temperature is back in normal range */
#define EVT_SDD_OTEMP_FIN        14
/** Operating Mode Ignored */
#define EVT_SDD_OMI              17
/** Operating Mode discarded */
#define EVT_SDD_OPM_DIS          18
/** Line testing aborted */
#define EVT_SDD_LT_ABORT         19

/* SIG events */
/** DTMF detector */
#define EVT_SIG_DTMF_DET       1
/** Caller ID sender request */
#define EVT_SIG_CIS_REQ        2
/** Caller ID sender buffer underflow */
#define EVT_SIG_CIS_BUF        3
/** Caller ID sender has finished sending data */
#define EVT_SIG_CIS_FIN        4
/** Start of tone detected */
#define EVT_SIG_UTD_START      5
/** End of tone detected */
#define EVT_SIG_UTD_END        6
/** Metering pulse sent */
#define EVT_SIG_TTX_FIN        9
/** AC level metering finished */
#define EVT_SIG_AC_LM_FIN      10

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
struct __fw_evt_header
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   /* Reserved */
   uint32_t Res00 : 3;
   /* Command Type */
   uint32_t CMD : 5;
   /* Reserved */
   uint32_t Res01 : 4;
   /* Channel */
   uint32_t CHAN : 4;
   /* Command Mode */
   uint32_t MOD : 3;
   /* Command Sub-Mode */
   uint32_t ECMD : 5;
   /* Length of Command Payload */
   uint32_t LENGTH : 8;
#else
   /* Length of Command Payload */
   uint32_t LENGTH : 8;
   /* Command Sub-Mode */
   uint32_t ECMD : 5;
   /* Command Mode */
   uint32_t MOD : 3;
   /* Channel */
   uint32_t CHAN : 4;
   /* Reserved */
   uint32_t Res01 : 4;
   /* Command Type */
   uint32_t CMD : 5;
   /* Reserved */
   uint32_t Res00 : 3;
#endif
} __attribute__ ((packed));

struct __fw_evt_int_err
{
   struct __fw_evt_header hdr;
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   /* Reserved */
   uint32_t Res01 : 27;
   /* Event Code */
   uint32_t ERREVT : 5;
#else
   /* Event Code */
   uint32_t ERREVT : 5;
   /* Reserved */
   uint32_t Res01 : 27;
#endif
} __attribute__ ((packed));

struct __fw_evt_cmd_err
{
   struct __fw_evt_header hdr;
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   /* Reserved */
   uint32_t Res01 : 18;
   /* Error Cause */
   uint32_t CMDERR : 14;
   /* Command Header of Command */
   uint32_t CMDHDR;
#else
   /* Error Cause */
   uint32_t CMDERR : 14;
   /* Reserved */
   uint32_t Res01 : 18;
   /* Command Header of Command */
   uint32_t CMDHDR;
#endif
} __attribute__ ((packed));

struct __fw_evt_sdd
{
   struct __fw_evt_header hdr;
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   /* Time Stamp */
   uint32_t TIME_STAMP : 16;
   /* Analog line operating mode */
   uint32_t OPMODE : 8;
   /* Reserved */
   uint32_t Res01 : 3;
   /* Event code */
   uint32_t EVT : 5;
#else
   /* Event code */
   uint32_t EVT : 5;
   /* Reserved */
   uint32_t Res01 : 3;
   /* Analog line operating mode */
   uint32_t OPMODE : 8;
   /* Time Stamp */
   uint32_t TIME_STAMP : 16;
#endif
} __attribute__ ((packed));

struct __fw_evt_sig
{
   struct __fw_evt_header hdr;
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   /* Time Stamp */
   uint32_t TIME_STAMP : 16;
   /* Reserved */
   uint32_t Res01 : 4;
   /*  DTMF Key */
   uint32_t DTMF_KEY : 4;
   /* Reserved */
   uint32_t Res02 : 3;
   /* Event code */
   uint32_t SIGEVT : 5;
#else
   /* Event code */
   uint32_t SIGEVT : 5;
   /* Reserved */
   uint32_t Res02 : 3;
   /*  DTMF Key */
   uint32_t DTMF_KEY : 4;
   /* Reserved */
   uint32_t Res01 : 4;
   /* Time Stamp */
   uint32_t TIME_STAMP : 16;
#endif
} __attribute__ ((packed));

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_void_t dxs_handle_event(
                        DXS_DEVICE_t *pDev,
                        struct __fw_evt_header *pHdr);

static IFX_void_t dxs_event_sys(
                        DXS_DEVICE_t *pDev,
                        struct __fw_evt_header *pHdr);

static IFX_void_t dxs_event_sdd(
                        DXS_DEVICE_t *pDev,
                        struct __fw_evt_sdd *pSddEvt);

static IFX_void_t dxs_event_sig(
                        DXS_DEVICE_t *pDev,
                        struct __fw_evt_sig *pSigEvt);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

/**
   Event handler switchboard

   \param  pDev         Pointer to the device structure.
   \param  pHdr         Pointer to the FW message.
*/
static IFX_void_t dxs_handle_event(
                        DXS_DEVICE_t *pDev,
                        struct __fw_evt_header *pHdr)
{
   LOG_RD_EVT(pDev->nDevNr, pDev->nChannel, pHdr, pHdr->LENGTH + 4, 0);

   if (pHdr->MOD == EVT_MOD_SYS)
   {
      /* System events */
      dxs_event_sys(pDev, pHdr);
   }
   else if (pHdr->MOD == EVT_MOD_ALI && pHdr->ECMD == EVT_ECMD_SDD)
   {
      /* SDD event */
      dxs_event_sdd(pDev, (struct __fw_evt_sdd *)pHdr);
   }
   else if (pHdr->MOD == EVT_MOD_ALI && pHdr->ECMD == EVT_ECMD_SIG)
   {
      /* SIG event */
      dxs_event_sig(pDev, (struct __fw_evt_sig *)pHdr);
   }
}


/**
   Handler for System Events

   \param  pDev         Pointer to the device structure.
   \param  pHdr         Pointer to the FW message.
*/
static IFX_void_t dxs_event_sys(
                        DXS_DEVICE_t *pDev,
                        struct __fw_evt_header *pHdr)
{
   if (pHdr->ECMD == EVT_ECMD_BOOTFIN)
   {
      /* Boot Finished event */
      pDev->nDevState |= DS_DEV_UP;
   }
   else
   if (pHdr->ECMD == EVT_ECMD_INTERR)
   {
      /* Internal Error event */
      struct __fw_evt_int_err *pIntErr = (struct __fw_evt_int_err *)pHdr;
      IFX_TAPI_EVENT_t  tapiEvent;

      if (pIntErr->ERREVT == EVT_INTERR_CLK_FAIL)
      {
         /* Send an Clock fail end event to TAPI. */
         memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL_END;
         DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_NONE);

         IFX_TAPI_Event_Dispatch(pDev->pChannel[0].pTapiCh, &tapiEvent);
      }
      else if(pIntErr->ERREVT == EVT_INTERR_SYNC_FAIL)
      {
         /* Send an Clock fail end event to TAPI. */
         memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_HW_SYNC;
         DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_NONE);

         IFX_TAPI_Event_Dispatch(pDev->pChannel[0].pTapiCh, &tapiEvent);
      }
      else
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
             ("DXS device %d unknown Internal Error Event code:%d\n",
             pDev->nDevNr, pIntErr->ERREVT));
      }
   }
   else
   if (pHdr->ECMD == EVT_ECMD_CMDERR)
   {
      /* Command Error event */
      const struct __fw_evt_cmd_err *pCerr = (struct __fw_evt_cmd_err *)pHdr;
      IFX_TAPI_EVENT_t tapiEvent = {0};
      IFX_uint32_t cerr_ack_cmd = 0x0600e000;

      /* acknowledge command error */
      (IFX_void_t) DXS_CmdWrite(pDev, &cerr_ack_cmd);

      /* send an CERR event including the details to TAPI */
      tapiEvent.id = IFX_TAPI_EVENT_DEBUG_CERR;
      DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_NONE);
      tapiEvent.data.cerr.fw_id = 0x0002; /* id = 2 identifies DuSLIC-XS */
      tapiEvent.data.cerr.reason = pCerr->CMDERR;
      tapiEvent.data.cerr.command = pCerr->CMDHDR;
      IFX_TAPI_Event_Dispatch(pDev->pChannel[0].pTapiCh, &tapiEvent);
   }
}


/**
   Handle the SDD event

   \param  pDev         Pointer to the device structure.
   \param  pSddEvent    Pointer to the SDD event structure.
*/
static IFX_void_t dxs_event_sdd(
                        DXS_DEVICE_t *pDev,
                        struct __fw_evt_sdd *pSddEvt)
{
   DXS_CHANNEL_t *pCh = &pDev->pChannel[pSddEvt->hdr.CHAN];
   IFX_TAPI_EVENT_t tapiEvent;

   if (pSddEvt->hdr.CHAN >= DXS_MAX_CH_NR)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("DXS FW error, reporting SDD event on wrong channel %d\n",
            pSddEvt->hdr.CHAN));
      return;
   }

   if (!(pDev->nDevState & DS_DEV_INIT))
   {
      TRACE(TAPI_DXS, DBG_LEVEL_LOW,
          ("WARNING: Skipping DXS SDD Event on not initialized driver.\n"));
      return;
   }

   memset(&tapiEvent, 0, sizeof(tapiEvent));
   DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_ALM);

   switch (pSddEvt->EVT)
   {
      case EVT_SDD_OTEMP:
         pCh->flags |= DXS_CH_OVER_TEMPERATURE;
         if (pCh->pALM != IFX_NULL)
         {
            irq_DXS_ALM_UpdateOpModeAndWakeUp(pCh, DXS_SDD_Opmode_Disabled);
            DXS_ALM_OnLineModeChanged(pCh, DXS_SDD_Opmode_Disabled);
         }
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_OTEMP_FIN:
         pCh->flags &= ~DXS_CH_OVER_TEMPERATURE;
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP_END;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_LT_FIN:
         tapiEvent.id = (DXS_ALM_CapMeasInProgress(pCh) == IFX_TRUE) ?
            IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY_INT :
            IFX_TAPI_EVENT_LT_GR909_RDY;
         IFX_TAPI_Event_DeferredDispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_LT_ABORT:
         if (DXS_ALM_CapMeasInProgress(pCh) == IFX_TRUE)
         {
            tapiEvent.id = IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY_INT;
            IFX_TAPI_Event_DeferredDispatch(pCh->pTapiCh, &tapiEvent);
         }
         break;

      case EVT_SDD_GF:
         pCh->flags |= DXS_CH_GROUND_FAULT;
         if (pCh->pALM != IFX_NULL)
         {
            irq_DXS_ALM_UpdateOpModeAndWakeUp(pCh, DXS_SDD_Opmode_Disabled);
            DXS_ALM_OnLineModeChanged(pCh, DXS_SDD_Opmode_Disabled);
         }
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_GF_FIN:
         pCh->flags &= ~DXS_CH_GROUND_FAULT;
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH_END;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_GK:
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_GK_LOW;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_GK_FIN:
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_END;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_OPC:
         if (pCh->pALM != IFX_NULL)
         {
            irq_DXS_ALM_UpdateOpModeAndWakeUp(pCh, pSddEvt->OPMODE);
            DXS_ALM_OnLineModeChanged(pCh, pSddEvt->OPMODE);
         }
         break;

      case EVT_SDD_OMI:
         if (pCh->pALM != IFX_NULL)
         {
            irq_DXS_ALM_UpdateOpModeAndWakeUp(pCh, OPMODE_IGNORED);
         }
         break;

      case EVT_SDD_OPM_DIS:
         if (pCh->pALM != IFX_NULL)
         {
            irq_DXS_ALM_UpdateOpModeAndWakeUp(pCh, OPMODE_IGNORED);
         }
         break;

      case EVT_SDD_ONH:
         /* Deactivate the dtmf receiver. */
         DXS_DTMF_REC_CTRL(pCh, IFX_FALSE);
         /* TAPI event */
         tapiEvent.id = IFX_TAPI_EVENT_FXS_ONHOOK_INT;
         tapiEvent.data.hook_int.nTime =
            DXS_ALM_ElapsedTimeSinceLastHook (pCh, pSddEvt->TIME_STAMP);
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;

      case EVT_SDD_OFFH:
#ifdef DXS_FEAT_CID
         /* call the caller ID state handler if CID transmission is ongoing */
         if (TAPI_Cid_IsActive(pCh->pTapiCh))
         {
            DXS_CID_SH (pCh, DXS_CID_EVT_OFFHOOK);
         }
#endif /* DXS_FEAT_CID */
         /* if no transmission of DTMF digits is ongoing, DXS_DTMF_SH returns
            immediately */
         DXS_DTMF_SH (pCh, DXS_DTMF_EVT_STOP);
         /* Activate the dtmf receiver. */
         DXS_DTMF_REC_CTRL(pCh, IFX_TRUE);
         /* TAPI event */
         tapiEvent.id = IFX_TAPI_EVENT_FXS_OFFHOOK_INT;
         tapiEvent.data.hook_int.nTime =
            DXS_ALM_ElapsedTimeSinceLastHook (pCh, pSddEvt->TIME_STAMP);
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;
      case EVT_SDD_CORCE:
         break;

      case EVT_SDD_COEFE:
         break;

      default:
         break;
   }
}


/**
   Handle the SIG event

   \param  pDev         Pointer to the device structure.
   \param  pSddEvent    Pointer to the SIG event structure.
*/
static IFX_void_t dxs_event_sig(
                           DXS_DEVICE_t *pDev,
                           struct __fw_evt_sig *pSigEvt)
{
   DXS_CHANNEL_t *pCh = &pDev->pChannel[pSigEvt->hdr.CHAN];
   IFX_TAPI_EVENT_t  tapiEvent;

   if (pSigEvt->hdr.CHAN >= DXS_MAX_CH_NR)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("DXS FW error, reporting SIG event on wrong channel %d\n",
            pSigEvt->hdr.CHAN));
      return;
   }

   if (!(pDev->nDevState & DS_DEV_INIT))
   {
      TRACE(TAPI_DXS, DBG_LEVEL_LOW,
          ("WARNING: Skipping DXS SIG Event on not initialized driver.\n"));
      return;
   }

   memset(&tapiEvent, 0, sizeof(tapiEvent));
   DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_ALM);

   switch (pSigEvt->SIGEVT)
   {
      case EVT_SIG_DTMF_DET:
         {
            IFX_uint8_t nKey = pSigEvt->DTMF_KEY;

            /* translate fw-coding to tapi enum coding */
            tapiEvent.data.dtmf.digit = DXS_DTMF_encode_fw2tapi(nKey);
            /* translate fw-coding to ascii charater */
            tapiEvent.data.dtmf.ascii =
               (IFX_uint8_t)DXS_DTMF_encode_fw2ascii(nKey);

            /* DTMF tone can only be received from ALM side */
#ifdef TAPI_VERSION3
            tapiEvent.data.dtmf.local = 1;
#endif /* TAPI_VERSION3 */
#ifdef TAPI_VERSION4
            tapiEvent.data.dtmf.external = 1;
#endif /* TAPI_VERSION4 */
            tapiEvent.id = IFX_TAPI_EVENT_DTMF_DIGIT;
            IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         }
         break;

#ifdef DXS_FEAT_CID
      case EVT_SIG_CIS_REQ:
         /* CIS data request: send one more byte of data  (CID_Sender_Data) */
         /* call the caller ID state handler */
         if (DXS_statusOk != DXS_CID_SH (pCh, DXS_CID_EVT_CIS_REQ))
         {
            TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                ("DXS driver error, DXS_CID_SH() failed\n"));
         }
         break;

      case EVT_SIG_CIS_BUF:
         {
            IFX_int8_t nRemainingCIDBytes;

            /* CIS buffer underflow: set CID transmit error and abort sending */
            /* Fill event structure. */
            nRemainingCIDBytes = DXS_CID_GetRemainingBytes(pCh);
            if(nRemainingCIDBytes == 0)
            {
               /* no more data to send */
               tapiEvent.id = IFX_TAPI_EVENT_CID_TX_END;
            }
            else if (nRemainingCIDBytes > 0)
            {
               /* there is more data to send */
               tapiEvent.id = IFX_TAPI_EVENT_CID_TX_UNDERRUN_ERR;
            }
            else
            {
               /* negative value should never happen */
               TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                   ("DXS driver error, negative remaining CID bytes %d\n",
                     nRemainingCIDBytes));
               return;
            }

            IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);

            /* call the caller ID state handler */
            DXS_CID_SH (pCh, DXS_CID_EVT_CIS_BUF);
         }
         break;

      case EVT_SIG_CIS_FIN:
         break;
#endif /* DXS_FEAT_CID */

#ifdef DXS_FEAT_METERING
      case EVT_SIG_TTX_FIN:
         tapiEvent.id = IFX_TAPI_EVENT_METERING_END;
         IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
         break;
#endif /* DXS_FEAT_METERING */

      case EVT_SIG_AC_LM_FIN:
#ifdef DXS_FEAT_NLT
         TAPI_OS_EventWakeUp (&pCh->pALM->aclm_event);
#endif
         break;

      default:
         break;
   }
}


/* Size of the out message box (Host data message box out) in 32-bit words. */
#define DXS_MBO_WORDS 8 /* 8 words @ 32-bit */

/**
   Function dxs_outbox_handler

   \param  pDev         Pointer to the device structure.

   \return Always DXS_statusOk.
*/
IFX_int32_t DXS_outbox_handler (DXS_DEVICE_t *pDev)
{
   static IFX_uint32_t message[DXS_MBO_WORDS] = {0};
   struct __fw_evt_header *pHdr = (struct __fw_evt_header *)message;
   IFX_uint8_t length = 1;
   IFX_int32_t ret = DXS_statusErr;

   /* protection from concurrent tasks and interrupts */
   DXS_FW_DL_PROTECT (pDev);

   /* Read the message header from the outbox - just one word. */
   ret = DXS_ObxRead(pDev, &message[0], &length);

   /* If the requested one word was read there is a message to be processed. */
   while (DXS_SUCCESS(ret) && (length == 1))
   {
      IFX_uint8_t length_from_msghdr = 0;

      /* get the length of the message payload in 32-bit words */
      length = (pHdr->LENGTH) / 4;
      length_from_msghdr = length;

      /* validate that the length byte is not obviously incorrect */
      if (((pHdr->LENGTH % 4) != 0) || (length > (DXS_MBO_WORDS - 1)))
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("Mailbox out of sync.\n"));
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("Message header: 0x%08X\n", message[0]));
         ret = DXS_statusCmdLengthInvalid;
         break;
      }

      /* Read the payload of the message if there is any. */
      if (length > 0)
      {
         ret = DXS_ObxRead(pDev, &message[1], &length);
      }
      if (!DXS_SUCCESS(ret))
      {
         break;
      }
      if (length != length_from_msghdr)
      {
         /* Unable to read the full length of the payload. */
         ret = DXS_statusCmdLengthInvalid;
         break;
      }

      /* decode the message and process it */
      if (pHdr->CMD == EVT_CMD_EVT &&
          ((pHdr->MOD == EVT_MOD_ALI && pHdr->LENGTH == 4) ||
           (pHdr->MOD == EVT_MOD_SYS && (pHdr->LENGTH == 0 ||
                                         pHdr->LENGTH == 4 ||
                                         pHdr->LENGTH == 8))))
      {
         /* this is an event */
         dxs_handle_event(pDev, pHdr);
      }
      else
      {
         const struct DXS_FW_Cmd_Header *pCmdHdr = (struct DXS_FW_Cmd_Header *)message;

         if ((pCmdHdr->RW == 1) &&
             (((pCmdHdr->CMD == DXS_CMD_CMD_EOP) &&
               (pCmdHdr->MOD == DXS_CMD_MOD_SYS)) ||
              ((pCmdHdr->CMD == DXS_CMD_CMD_SDD) &&
               (pCmdHdr->MOD == DXS_CMD_MOD_SDD) &&
               ((pCmdHdr->CHAN == DXS_CMD_CHAN_A) ||
                (pCmdHdr->CHAN == DXS_CMD_CHAN_B))) ||
              (((pCmdHdr->CMD == DXS_CMD_CMD_EOP)) &&
               ((pCmdHdr->CHAN == DXS_CMD_CHAN_A) ||
                (pCmdHdr->CHAN == DXS_CMD_CHAN_B)) &&
               ((pCmdHdr->MOD == DXS_CMD_MOD_GPIO) ||
                (pCmdHdr->MOD == DXS_CMD_MOD_PCM) ||
                (pCmdHdr->MOD == DXS_CMD_MOD_SIG_GEN) ||
                (pCmdHdr->MOD == DXS_CMD_MOD_SIG_DET)))))
         {
            /* this is a command - copy it to fifo for CmdRead */
            IFX_uint8_t i;

            for (i = 0; i <= length; i++)
            {
               fifo_put(pDev->cmd_obx_queue, NULL, message[i]);
            }

            /* wake up sleeping CmdRead */
            pDev->bOutBoxData = IFX_TRUE;
            TAPI_OS_EventWakeUp (&pDev->obxDataEvt);
         }
         else
         {
            /* Send outbox header error event to TAPI. */
            IFX_TAPI_EVENT_t  tapiEvent = {0};
            tapiEvent.id = IFX_TAPI_EVENT_FAULT_FW_OBX_HDR_ERR;
            DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_NONE);
            IFX_TAPI_Event_Dispatch(pDev->pChannel[0].pTapiCh, &tapiEvent);
            /* errmsg: Header of message read from outbox is corrupted. */
            ret = DXS_statusObxFwMsgHdrCorrupt;
            break;
         }
      }

      /* If the IRQ is edge triggered we have to make sure that the
         mailbox is empty when we exit the loop. For edge triggered
         interrupts this works as well. As the exit condition is that
         there is no more data waiting in the mailbox another read
         attempt is done and if there is no data the length will be 0. */
      length = 1;
      ret = DXS_ObxRead(pDev, &message[0], &length);
   }

   /* release protection for concurrent tasks and interrupts */
   DXS_FW_DL_RELEASE (pDev);

   return ret;
}


/**
   Function dxs_outbox_handler_init

   \param  pDev         Pointer to the device structure.

   \return
   - DXS_status_t
*/
IFX_int32_t DXS_outbox_handler_init(DXS_DEVICE_t *pDev)
{
   /* Init event that indicates that a command packet arrived in mailbox. */
   TAPI_OS_EventInit (&pDev->obxDataEvt);
   pDev->bOutBoxData = IFX_FALSE;

   pDev->cmd_obx_queue = fifo_init (64);
   if (NULL == pDev->cmd_obx_queue)
   {
      /* errmsg: Failed to create fifo for outbox handling. */
      return DXS_statusMsgQueueCreatError;
   }

   return DXS_statusOk;
}


/**
   Function dxs_outbox_handler_exit

   \param  pDev         Pointer to the device structure.

   \return
   - DXS_status_t
*/
IFX_int32_t DXS_outbox_handler_exit(DXS_DEVICE_t *pDev)
{
   fifo_destroy(pDev->cmd_obx_queue);

   /* DXS_OS_EventDelete (&pDev->obxDataEvt); */

   return IFX_SUCCESS;
}
