/******************************************************************************

  Copyright (c) 2006-2013 Lantiq Deutschland GmbH
  Copyright (c) 2015      Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2022          Maxlinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/


/**
   \file drv_tapi_event.c
   Contains TAPI Event Handling.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "drv_tapi.h"
#include "drv_tapi_errno.h"
#include "drv_tapi_event.h"
#include "drv_tapi_cid.h"

/*lint -save
  -esym(529,lock) */

/* ============================= */
/* Local Macros  Definitions    */
/* ============================= */

/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Global functions declaration  */
/* ============================= */

/* ============================= */
/* Type declarations             */
/* ============================= */

/* These structures are used for masking events. */
struct IFX_TAPI_EVENT_FXS_BITS
{
   IFX_uint32_t ring_abort:1;
   IFX_uint32_t ringpausecidtx:1;
   IFX_uint32_t linemode:1;
   IFX_uint32_t flash:1;
   IFX_uint32_t offhook:1;
   IFX_uint32_t onhook:1;
   IFX_uint32_t ringing_end:1;
   IFX_uint32_t ringburst_end:1;
   IFX_uint32_t ring:1;
   IFX_uint32_t contMeas:1;
   IFX_uint32_t rawoffhook:1;
   IFX_uint32_t rawonhook:1;
};

struct IFX_TAPI_EVENT_LT_BITS
{
   IFX_uint32_t gr909_rdy:1;
   IFX_uint32_t nlt_end:1;
};

struct IFX_TAPI_EVENT_PULSE_BITS
{
   IFX_uint32_t pulseStart:1;
   IFX_uint32_t digit:1;
};

struct IFX_TAPI_EVENT_DTMF_BITS
{
   IFX_uint32_t end_internal:1;
   IFX_uint32_t end_external:1;
   IFX_uint32_t digit_internal:1;
   IFX_uint32_t digit_external:1;
   IFX_uint32_t firstime_flag:1; /* not a mask but acutally a flag */
};

struct IFX_TAPI_EVENT_CID_BITS
{
   IFX_uint32_t sm_end:1;
   IFX_uint32_t tx_ringcad_err:1;
   IFX_uint32_t tx_noack_err:1;
   IFX_uint32_t tx_info_end:1;
   IFX_uint32_t tx_info_start:1;
   IFX_uint32_t tx_seq_end:1;
   IFX_uint32_t tx_seq_start:1;
};

struct IFX_TAPI_EVENT_TONE_GEN_BITS
{
   IFX_uint32_t end_internal:1;
   IFX_uint32_t end_external:1;
   IFX_uint32_t busy:1;
};

struct IFX_TAPI_EVENT_TONE_DET_BITS
{
   IFX_uint32_t receive:1;
   IFX_uint32_t transmit:1;
};

struct IFX_TAPI_EVENT_DEBUG_BITS
{
   IFX_uint32_t cerr:1;
};

struct IFX_TAPI_EVENT_FAULT_LINE_BITS
{
   /* IFX_uint32_t overtemp_end:1; - unmaskable */
   IFX_uint32_t gk_high_end:1;
   IFX_uint32_t gk_low_end:1;
   /* IFX_uint32_t gk_high_int:1; - internal */
   /* IFX_uint32_t gk_low_int:1; - internal */
   IFX_uint32_t overcurrent:1;
   /* IFX_uint32_t overtemp:1; - unmaskable */
   IFX_uint32_t gk_high:1;
   IFX_uint32_t gk_low:1;
   IFX_uint32_t gk_neg:1;
   IFX_uint32_t gk_pos:1;
};

/*lint -save -esym(754,spi_access) */
struct IFX_TAPI_EVENT_FAULT_HW_BITS
{
   /* IFX_uint32_t ssi_err_end : 1; - unmaskable */
   /* IFX_uint32_t ssi_err : 1; - unmaskable */
   /* IFX_uint32_t hw_reset:1; - unmaskable */
   /* IFX_uint32_t sync:1; - unmaskable */
   /* IFX_uint32_t hw_fault:1; - unmaskable */
   IFX_uint32_t clock_fail_end:1;
   IFX_uint32_t clock_fail:1;
   IFX_uint32_t spi_access:1;
};
/*lint -restore */

struct IFX_TAPI_EVENT_FAULT_FW_BITS
{
   IFX_uint32_t fw_ebo_uf:1;
   /* IFX_uint32_t fw_ebo_of:1; - unmaskable */
   IFX_uint32_t fw_cbo_uf:1;
   IFX_uint32_t fw_cbo_of:1;
   IFX_uint32_t fw_cbi_of:1;
   /* IFX_uint32_t fw_watchdog:1; - unmaskable */
};

struct IFX_TAPI_EVENT_METERING_BITS
{
   IFX_uint32_t metering_end:1;
};

struct IFX_TAPI_EVENT_CALIBRATION_BITS
{
   IFX_uint32_t calibration_end:1;
};

struct IFX_TAPI_EVENT_LCAP_BITS
{
   IFX_uint32_t cap2gnd:1;
   IFX_uint32_t linecap:1;
};


typedef struct
{
   struct IFX_TAPI_EVENT_FXS_BITS fxs;
   struct IFX_TAPI_EVENT_LT_BITS lt;
   struct IFX_TAPI_EVENT_PULSE_BITS pulse;
   struct IFX_TAPI_EVENT_DTMF_BITS dtmf[IFX_TAPI_MODULE_TYPE_PCM+1];
   struct IFX_TAPI_EVENT_CID_BITS cid;
   struct IFX_TAPI_EVENT_TONE_GEN_BITS tone_gen;
   struct IFX_TAPI_EVENT_TONE_DET_BITS tone_det[IFX_TAPI_MODULE_TYPE_PCM+1];
   struct IFX_TAPI_EVENT_DEBUG_BITS debug;
   struct IFX_TAPI_EVENT_FAULT_LINE_BITS fault_line;
   struct IFX_TAPI_EVENT_FAULT_HW_BITS fault_hw;
   struct IFX_TAPI_EVENT_FAULT_FW_BITS fault_fw;
   struct IFX_TAPI_EVENT_METERING_BITS metering;
   struct IFX_TAPI_EVENT_CALIBRATION_BITS calibration;
   struct IFX_TAPI_EVENT_LCAP_BITS lcap;
} IFX_TAPI_EVENT_MASK_t;

struct IFX_TAPI_EVENT_HANDLER_DATA
{
   /* Tapi event fifo -- High priority event */
   FIFO_ID              *pTapiEventFifoHi;
   /* Tapi event fifo -- Low priority event */
   FIFO_ID              *pTapiEventFifoLo;
   /* Mask for Tapi events (enable/disable single events) */
   IFX_TAPI_EVENT_MASK_t eventMask;
   /* Fifo concurrent access protection mutex. */
   TAPI_OS_mutex_t       fifoAcc;
   /* control struct for SMP spinlock handing */
   struct TAPI_OS_spin_lock_s slProtectFifoAcc;
};


/* ============================= */
/* Local variable definition     */
/* ============================= */

/* Memory pool for event dispatcher (global for all channels) */
static BUFFERPOOL *pIFX_TAPI_BP_Deferred_Event = IFX_NULL;

#ifdef TAPI_FEAT_LINUX_SMP
   /* Buffer pool access protection */
   static DEFINE_SPINLOCK(lockEventBufferPoolAcc);
#else
   /* Buffer pool access protection */
   static TAPI_OS_mutex_t lockEventBufferPoolAcc;
#endif


/* ============================= */
/* Local function declaration    */
/* ============================= */

static IFX_void_t tapi_WakeUp (
                        TAPI_DEV *pTapiDev);

static IFX_void_t tapi_DiscardEvent (
                        IFX_TAPI_EXT_EVENT_PARAM_t *pEvent);

static IFX_int32_t tapi_EventFifoGet (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_EVENT_t *pTapiEvent);

static IFX_int32_t tapi_PhoneEventGet (
                        TAPI_DEV *pTapiDev,
                        IFX_TAPI_EVENT_t *pEvent);

static IFX_int32_t tapi_EventStateSet (
                        IFX_TAPI_DRV_CTX_t *pDrvCtx,
                        IFX_TAPI_EVENT_t *pEvent,
                        IFX_uint32_t nEvtState);

/* ============================= */
/* Local function definition     */
/* ============================= */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Enable or disable an event on the specified channel(s).

   \param  pChannel     Pointer to TAPI_CHANNEL structure.
   \param  pEvent       The event which mask is to be set.
   \param  value        -IFX_EVENT_ENABLE to allow the event to the application
                        -IFX_EVENT_DISABLE to discard the event

   \return
   TAPI_statusOk - success
   TAPI_statusEvtNoHandle - event not known
*/
IFX_int32_t IFX_TAPI_Event_SetMask (TAPI_CHANNEL *pChannel,
                                    IFX_TAPI_EVENT_t *pEvent,
                                    IFX_uint32_t const value)
{
   IFX_TAPI_EVENT_MASK_t *pEventMask;
   IFX_TAPI_MODULE_TYPE_t nModule, nModuleLoop;
   IFX_boolean_t bExitLoop;

   TAPI_ASSERT (pChannel);

   if (pEvent->module < IFX_TAPI_MODULE_TYPE_NONE ||
       pEvent->module > IFX_TAPI_MODULE_TYPE_ALL)
   {
      RETURN_STATUS (TAPI_statusParam, 0);
   }

   if (IFX_NULL == pChannel->pEventHandler)
      RETURN_STATUS (TAPI_statusEvtNoHandle, 0);

   /* set mask for event filtering */
   pEventMask = &(pChannel->pEventHandler->eventMask);

#ifdef TAPI_VERSION3
   nModule = IFX_TAPI_MODULE_TYPE_ALM;
#else
   nModule = pEvent->module;
#endif

#ifdef TAPI_FEAT_DTMF
#ifdef TAPI_VERSION3
   if (((pEvent->id == IFX_TAPI_EVENT_DTMF_DIGIT) ||
        (pEvent->id == IFX_TAPI_EVENT_DTMF_END)) &&
       (pEventMask->dtmf[nModule].firstime_flag == IFX_EVENT_ENABLE))
   {
      /* This is the first enable/disable command on this channel that changes
         the DTMF detection. Clear the mask to prevent the error that the DTMF
         detector cannot be enabled in both directions. */
      pEventMask->dtmf[nModule].firstime_flag = IFX_EVENT_DISABLE;
      pEventMask->dtmf[nModule].digit_external = IFX_EVENT_DISABLE;
      pEventMask->dtmf[nModule].digit_internal = IFX_EVENT_DISABLE;
   }
#endif /* TAPI_VERSION3 */
#endif /* TAPI_FEAT_DTMF */

   /* In case of IFX_TAPI_MODULE_TYPE_ALL the mask of all module needs to
      be set. This is done with a loop below. The loop is ended when either
      all modules were iterated or when the exit variable is set. The second
      is done in case the event of only one specific module should be enabled.
      But also in all cases where the event is independent from the module. */
   if (nModule == IFX_TAPI_MODULE_TYPE_ALL)
   {
      nModuleLoop = IFX_TAPI_MODULE_TYPE_NONE;
      bExitLoop = IFX_FALSE;
   }
   else
   {
      nModuleLoop = nModule;
      bExitLoop = IFX_TRUE;
   }

   do
   {
      switch(pEvent->id)
      {

#ifdef TAPI_FEAT_RINGENGINE
         case IFX_TAPI_EVENT_FXS_RING:
            pEventMask->fxs.ring = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FXS_RINGBURST_END:
            pEventMask->fxs.ringburst_end = value;
            bExitLoop = IFX_TRUE;
            break;
#endif /* TAPI_FEAT_RINGENGINE */

         case IFX_TAPI_EVENT_FXS_RINGING_END:
            pEventMask->fxs.ringing_end = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FXS_ONHOOK:
            pEventMask->fxs.onhook = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FXS_OFFHOOK:
            pEventMask->fxs.offhook = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FXS_FLASH:
            pEventMask->fxs.flash = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FXS_RAW_ONHOOK:
            pEventMask->fxs.rawonhook = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FXS_RAW_OFFHOOK:
            pEventMask->fxs.rawoffhook = value;
            bExitLoop = IFX_TRUE;
            break;

#ifdef TAPI_FEAT_CONT_MEAS
         case IFX_TAPI_EVENT_CONTMEASUREMENT:
            pEventMask->fxs.contMeas = value;
            bExitLoop = IFX_TRUE;
            break;
#endif /* TAPI_FEAT_CONT_MEAS */

         case IFX_TAPI_EVENT_FXS_LINE_MODE:
            pEventMask->fxs.linemode = value;
            bExitLoop = IFX_TRUE;
            break;

#ifdef TAPI_FEAT_CID
         case IFX_TAPI_EVENT_FXS_RINGPAUSE_CIDTX:
            pEventMask->fxs.ringpausecidtx = value;
            bExitLoop = IFX_TRUE;
            break;
#endif /* TAPI_FEAT_CID */

         case IFX_TAPI_EVENT_FXS_RING_ABORT:
            pEventMask->fxs.ring_abort = value;
            bExitLoop = IFX_TRUE;
            break;

#ifdef TAPI_FEAT_GR909
         case IFX_TAPI_EVENT_LT_GR909_RDY:
            pEventMask->lt.gr909_rdy = value;
            bExitLoop = IFX_TRUE;
            break;
#endif /* TAPI_FEAT_GR909 */

         case IFX_TAPI_EVENT_PULSE_DIGIT:
            pEventMask->pulse.digit = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_PULSE_START:
            pEventMask->pulse.pulseStart = value;
            bExitLoop = IFX_TRUE;
            break;

#ifdef TAPI_FEAT_DTMF
         case IFX_TAPI_EVENT_DTMF_DIGIT:
            if (TAPI_COD_EXTERNAL (pEvent->data.dtmf))
               pEventMask->dtmf[nModuleLoop].digit_external = value;
            if (TAPI_COD_INTERNAL (pEvent->data.dtmf))
               pEventMask->dtmf[nModuleLoop].digit_internal = value;
            break;
         case IFX_TAPI_EVENT_DTMF_END:
            if (TAPI_COD_EXTERNAL (pEvent->data.dtmf))
               pEventMask->dtmf[nModuleLoop].end_external = value;

            if (TAPI_COD_INTERNAL (pEvent->data.dtmf))
               pEventMask->dtmf[nModuleLoop].end_internal = value;
            break;
#endif /* TAPI_FEAT_DTMF */

#ifdef TAPI_FEAT_CID
         case IFX_TAPI_EVENT_CID_TX_SEQ_START:
            pEventMask->cid.tx_seq_start = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_CID_TX_SEQ_END:
            pEventMask->cid.tx_seq_end = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_CID_TX_INFO_START:
            pEventMask->cid.tx_info_start = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_CID_TX_INFO_END:
            pEventMask->cid.tx_info_end = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_CIDSM_END:
            pEventMask->cid.sm_end = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_CID_TX_RINGCAD_ERR:
            pEventMask->cid.tx_ringcad_err = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_CID_TX_NOACK_ERR:
         case IFX_TAPI_EVENT_CID_TX_NOACK2_ERR:
            pEventMask->cid.tx_noack_err = value;
            bExitLoop = IFX_TRUE;
            break;
#endif /* TAPI_FEAT_CID */

         case IFX_TAPI_EVENT_TONE_GEN_BUSY:
            pEventMask->tone_gen.busy = value;
            bExitLoop = IFX_TRUE;
            break;

#ifdef TAPI_FEAT_TONEENGINE
         case IFX_TAPI_EVENT_TONE_GEN_END:
            if ( TAPI_COD_EXTERNAL (pEvent->data.tone_gen))
               pEventMask->tone_gen.end_external = value;
            if ( TAPI_COD_INTERNAL (pEvent->data.tone_gen))
               pEventMask->tone_gen.end_internal = value;
            bExitLoop = IFX_TRUE;
            break;
#endif /* TAPI_FEAT_TONEENGINE */

         case IFX_TAPI_EVENT_TONE_DET_RECEIVE:
            pEventMask->tone_det[nModuleLoop].receive = value;
            break;
         case IFX_TAPI_EVENT_TONE_DET_TRANSMIT:
            pEventMask->tone_det[nModuleLoop].transmit = value;
            break;
      /* case IFX_TAPI_EVENT_FAULT_GENERAL:
         case IFX_TAPI_EVENT_FAULT_GENERAL_CHINFO:
         case IFX_TAPI_EVENT_FAULT_GENERAL_DEVINFO:
            General system fault(s) (reserved) - not handled - will return error
            break; */
         case IFX_TAPI_EVENT_FAULT_LINE_GK_POS:
            pEventMask->fault_line.gk_pos = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FAULT_LINE_GK_NEG:
            pEventMask->fault_line.gk_neg = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FAULT_LINE_GK_LOW:
            pEventMask->fault_line.gk_low = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_END:
            pEventMask->fault_line.gk_low_end = value;
            bExitLoop = IFX_TRUE;
            break;
         /* case IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_INT:
            internal event - not handled - will return error
            break; */
         case IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH:
            pEventMask->fault_line.gk_high = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH_END:
            pEventMask->fault_line.gk_high_end = value;
            bExitLoop = IFX_TRUE;
            break;
         /* case IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH_INT:
            internal event - not handled - will return error
            break; */
         case IFX_TAPI_EVENT_FAULT_LINE_OVERCURRENT:
            pEventMask->fault_line.overcurrent = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FAULT_HW_SPI_ACCESS:
            pEventMask->fault_hw.spi_access = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL:
            pEventMask->fault_hw.clock_fail = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL_END:
            pEventMask->fault_hw.clock_fail_end = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FAULT_FW_EBO_UF:
            pEventMask->fault_fw.fw_ebo_uf = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FAULT_FW_CBO_UF:
            pEventMask->fault_fw.fw_cbo_uf = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FAULT_FW_CBO_OF:
            pEventMask->fault_fw.fw_cbo_of = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FAULT_FW_CBI_OF:
            pEventMask->fault_fw.fw_cbi_of = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_FAULT_GENERAL_EVT_FIFO_OVERFLOW:
         case IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP:
         case IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP_END:
         case IFX_TAPI_EVENT_FAULT_HW_SYNC:
         case IFX_TAPI_EVENT_FAULT_HW_FAULT:
         case IFX_TAPI_EVENT_FAULT_HW_RESET:
         case IFX_TAPI_EVENT_FAULT_FW_EBO_OF:
         case IFX_TAPI_EVENT_FAULT_HW_SSI_ERR:
         case IFX_TAPI_EVENT_FAULT_HW_SSI_ERR_END:
            /* cannot be masked, always enabled */
            if (value != IFX_EVENT_ENABLE)
            {
               /* errmsg: Event cannot be disable, event always enabled */
               RETURN_STATUS (TAPI_statusEvtNotDisabled, 0);
            }
            bExitLoop = IFX_TRUE;
            break;

#ifdef TAPI_FEAT_METERING
         case IFX_TAPI_EVENT_METERING_END:
            pEventMask->metering.metering_end = value;
            bExitLoop = IFX_TRUE;
            break;
#endif /* TAPI_FEAT_METERING */

#ifdef TAPI_FEAT_CALIBRATION
         case IFX_TAPI_EVENT_CALIBRATION_END:
            pEventMask->calibration.calibration_end = value;
            bExitLoop = IFX_TRUE;
            break;
#endif /* TAPI_FEAT_CALIBRATION */

#ifdef TAPI_FEAT_CAP_MEAS
         case IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY:
            pEventMask->lcap.linecap = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_GND_RDY:
            pEventMask->lcap.cap2gnd = value;
            bExitLoop = IFX_TRUE;
            break;
         case IFX_TAPI_EVENT_NLT_END:
            pEventMask->lt.nlt_end = value;
            bExitLoop = IFX_TRUE;
            break;
#endif /* TAPI_FEAT_CAP_MEAS */

         case IFX_TAPI_EVENT_DEBUG_CERR:
            pEventMask->debug.cerr = value;
            bExitLoop = IFX_TRUE;
            break;
         default:
            RETURN_STATUS (TAPI_statusEvtNoHandle, 0);
      } /* switch(pEvent->id) */

      nModuleLoop++;
   }  while ((bExitLoop == IFX_FALSE) &&
             (nModuleLoop <= IFX_TAPI_MODULE_TYPE_PCM));


   /* call LL-driver to enable/disable event generation */
   switch(pEvent->id)
   {
#ifdef TAPI_FEAT_DTMF
      case IFX_TAPI_EVENT_DTMF_DIGIT:
      case IFX_TAPI_EVENT_DTMF_END:
         break;
#endif /* TAPI_FEAT_DTMF */

      case IFX_TAPI_EVENT_FXS_RAW_ONHOOK:
      case IFX_TAPI_EVENT_FXS_RAW_OFFHOOK:
         break;

      default :
         /* nothing to do */
         break;
   }

   return TAPI_statusOk;
}


/**
   Reports the event information for a channel

   \param  pTapiDev     Pointer to TAPI device structure.
   \param  pEvent       Pointer to a event structure. Must not be NULL.

   \return
   TAPI_statusOk - success
   TAPI_statusParam - channel out of range
*/
static IFX_int32_t tapi_PhoneEventGet (
   TAPI_DEV *pTapiDev,
   IFX_TAPI_EVENT_t *pEvent)
{
   IFX_int32_t moreEvents = 0;

   if (pEvent->ch == IFX_TAPI_EVENT_ALL_CHANNELS)
   {
      IFX_int16_t found_ch_nr;
      IFX_uint8_t i;

      /* Scan every channel to see if any of them has an event waiting.
         If an event is found return it to the application. So each call
         returns one event at one time. */

      /* For safety make sure we are within limits. */
      if (pTapiDev->nLastEventChannel >= pTapiDev->nMaxChannel)
      {
         pTapiDev->nLastEventChannel = 0;
      }

      /* We start scanning with the next channel after the last channel
         processed during the last call. We are called at least one time.
         Even if there is only one channel the loop will be aborted because
         of the wrap-around correction. The exit from the loop is done only
         with the break at the end of the loop. */
      for (i = pTapiDev->nLastEventChannel + 1, found_ch_nr = -1;
           /* no condition */; i++)
      {
         i = ((i >= pTapiDev->nMaxChannel) ? 0 : i);

         /* As long as we have not found any event we check all the channels
            for events. As soon as we got an event we still check if there
            are any more events in any of the remaining channels. */
         if (found_ch_nr < 0)
         {
            /* check this channel for events */
            moreEvents = tapi_EventFifoGet (pTapiDev->pChannel + i, pEvent);
            if (pEvent->id != IFX_TAPI_EVENT_NONE)
            {
               found_ch_nr = i;
            }
         }
         else
         {
            /* check only if events present but do not get them */
            moreEvents += (IFX_TAPI_EventFifoEmpty(
               pTapiDev->pChannel + i) == 0) ? 1 : 0;
         }

         /* abort loop if we either wrapped around or found that there are
            more than just one event which implies that we already got one */
         if ((i == pTapiDev->nLastEventChannel) || (moreEvents != 0))
            break;
      }

      /* remember the last channel number that was serviced */
      pTapiDev->nLastEventChannel = found_ch_nr < 0 ? i : found_ch_nr;
   }
   else /* Scanning event by channel index. */
   {
      if (pEvent->ch >= pTapiDev->nMaxChannel)
         RETURN_DEVSTATUS (TAPI_statusParam, 0);

      /* Still get one event at one time. */
      moreEvents = tapi_EventFifoGet (pTapiDev->pChannel + pEvent->ch, pEvent);
   }

   /* Set the more events flag inside the event message. The flag indicates
      if there are any more events in the channel that was given as a parameter.
      So for specific channels it returns if there are more events in the
      specific channel. For IFX_TAPI_EVENT_ALL_CHANNELS it returns if there
      are any more events in any of the channels. */
   pEvent->more = moreEvents;

   return TAPI_statusOk;
}


/**
   WakeUp the TAPI channel task.

   \param  pTapiDev     Pointer to TAPI device structure.
*/
static IFX_void_t tapi_WakeUp(TAPI_DEV *pTapiDev)
{
   if (pTapiDev->bNeedWakeup)
   {
      TAPI_OS_DrvSelectQueueWakeUp(&pTapiDev->wqEvent,
                                   TAPI_OS_DRV_SEL_WAKEUP_TYPE_RD);
   }
}


/**
  Check whether event fifo is empty or not (protected).
  \param pChannel - High level TAPI Channel

  \return
    If empty, return 1,else 0.
*/
IFX_uint8_t IFX_TAPI_EventFifoEmpty (TAPI_CHANNEL * pChannel)
{
   IFX_uint8_t ret = 0;

   if (pChannel->pEventHandler == IFX_NULL)
      return 1;
   /* Lock the fifo access protection. */
   TAPI_OS_MutexGet (&pChannel->pEventHandler->fifoAcc);
   TAPI_OS_SPIN_LOCK_IRQSAVE(&pChannel->pEventHandler->slProtectFifoAcc);

   if (fifoEmpty (pChannel->pEventHandler->pTapiEventFifoHi) &&
       fifoEmpty (pChannel->pEventHandler->pTapiEventFifoLo))
   {
      ret = 1;
   }
   else
   {
      ret = 0;
   }

   /* Unlock fifo access protection. */
   TAPI_OS_SPIN_UNLOCK_IRQRESTORE(&pChannel->pEventHandler->slProtectFifoAcc);
   TAPI_OS_MutexRelease (&pChannel->pEventHandler->fifoAcc);

   return ret;
}


/**
  Return counters from the event fifos of the given channel.

  This function cares about protecting the fifo and returns counters from the
  fifos of the channel. Currently only the number of events waiting in the
  fifos are reported. Possible extensions are counters on the size, the full
  state or the number of transactions.

  \param  pChannel     Pointer to tapi channel structure.
  \param  pCounters    Pointer to struct in which the counters are returned.
                       Content is valid only if return is TAPI_statusOk.

  \return
  - TAPI_statusOk on success.
  - TAPI_statusErr if the channel is uninitialised or parameter is IFX_NULL.
*/
IFX_int32_t IFX_TAPI_EventFifoCounters (TAPI_CHANNEL *pChannel,
                  IFX_TAPI_EVENT_FIFO_COUNTERS_t *pCounters)
{
   if ((pChannel->pEventHandler == IFX_NULL) || (pCounters == IFX_NULL))
      return TAPI_statusErr;

   /* Lock the fifo access protection. */
   TAPI_OS_MutexGet (&pChannel->pEventHandler->fifoAcc);
   TAPI_OS_SPIN_LOCK_IRQSAVE(&pChannel->pEventHandler->slProtectFifoAcc);

   pCounters->nHighWaiting =
                     fifoElements (pChannel->pEventHandler->pTapiEventFifoHi);
   pCounters->nLowWaiting  =
                     fifoElements (pChannel->pEventHandler->pTapiEventFifoLo);

   /* Unlock fifo access protection. */
   TAPI_OS_SPIN_UNLOCK_IRQRESTORE(&pChannel->pEventHandler->slProtectFifoAcc);
   TAPI_OS_MutexRelease (&pChannel->pEventHandler->fifoAcc);

   return TAPI_statusOk;
}


/**
   Get event stored in the event fifo.

   Gets the next event from the channel high priority or low priority fifo.
   If both fifos are empty or not initalised the id IFX_TAPI_EVENT_NONE is
   returned. This function uses interrupt locking and a semaphore to protect
   access to the fifos. The wrapper buffer holding the event is freed after
   copying of the event.

   \param  pChannel     Pointer to TAPI_CHANNEL structure.
   \param  pTapiEvent   Pointer to TAPI Event struct where to return the event.

   \return
   If there are more events stored in hi/lo fifo, return 1, otherwise return 0.
*/
static IFX_int32_t tapi_EventFifoGet (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_EVENT_t *pTapiEvent)
{
   IFX_TAPI_EXT_EVENT_PARAM_t *pTempEvt = IFX_NULL;
   IFX_int32_t moreEvent = 0;

   TAPI_ASSERT (pChannel);

   if (IFX_NULL == pChannel->pEventHandler)
   {
      pTapiEvent->id = IFX_TAPI_EVENT_NONE;
      return 0;
   }

   /* Lock the fifo access protection. */
   TAPI_OS_MutexGet (&pChannel->pEventHandler->fifoAcc);
   TAPI_OS_SPIN_LOCK_IRQSAVE(&pChannel->pEventHandler->slProtectFifoAcc);

   /* Get event stored in high prio fifo */
   if (fifoGet(pChannel->pEventHandler->pTapiEventFifoHi, &pTempEvt) ==
       IFX_SUCCESS)
   {
      /* copy event data into struct provided by the caller */
      memcpy (pTapiEvent, &pTempEvt->tapiEvent, sizeof (IFX_TAPI_EVENT_t));
      /* release the buffer back to the pool */
      tapi_DiscardEvent (pTempEvt);

   }
   else
   /* If high prio fifo is empty, get event stored in low prio fifo. */
   if (fifoGet(pChannel->pEventHandler->pTapiEventFifoLo, &pTempEvt) ==
       IFX_SUCCESS)
   {
      /* copy event data into struct provided by the caller */
      memcpy (pTapiEvent, &pTempEvt->tapiEvent, sizeof (IFX_TAPI_EVENT_t));
      /* release the buffer back to the pool */
      tapi_DiscardEvent (pTempEvt);
   }
   else
   {
      /* If high and low prio fifo are empty, set the event id to
         IFX_TAPI_EVENT_NONE. Do not change the channel parameter. */
      pTapiEvent->id = IFX_TAPI_EVENT_NONE;
   }

   /* Check whether there are any more events stored in hi or lo fifo. */
   if (fifoEmpty (pChannel->pEventHandler->pTapiEventFifoHi) &&
       fifoEmpty (pChannel->pEventHandler->pTapiEventFifoLo))
   {
      /* no events stored in high priority or low priority fifo */
      moreEvent = 0;
   }
   else
   {
      moreEvent = 1;
   }

   /* Unlock fifo access protection. */
   TAPI_OS_SPIN_UNLOCK_IRQRESTORE(&pChannel->pEventHandler->slProtectFifoAcc);
   TAPI_OS_MutexRelease (&pChannel->pEventHandler->fifoAcc);

   return moreEvent;
}


/********************NOTE *****************************************************

 1.IFX_TAPI_Event_Dispatch : Call this function to dispatch an event. The
 event is stored in an allocated buffer together with additional information
 such as the channel. This is then passed on to task context with the
 TAPI_DeferWork() function.

 2.IFX_TAPI_Event_Dispatch_ProcessCtx : This function is working in task
 context. It receives the events dispatched by IFX_TAPI_Event_Dispatch
 processes and filters them and finally put them into the queue towards the
 application.

*******************************************************************************/

/**
   High Level Event Dispatcher function
   (May be called from task or interrupt context.)

   \param pChannel     - handle to tapi channel structure
   \param pTapiEvent   - handle to event

   \return TAPI_statusOk on ok, otherwise TAPI_statusErr
 */
IFX_int32_t IFX_TAPI_Event_Dispatch (TAPI_CHANNEL *pChannel,
   IFX_TAPI_EVENT_t *pTapiEvent)
{
   return IFX_TAPI_Event_DispatchExt (pChannel, pTapiEvent,
                                      TAPI_OS_IN_INTERRUPT());
}


/**
   High Level Event Dispatcher function
   (May be called from task or interrupt context.)

   \param pChannel     - handle to TAPI channel structure
   \param pTapiEvent   - handle to TAPI event
   \param bDefer If set to IFX_TRUE the event will be deferred in
      a new task/thread. This is equal to the TAPI_OS_IN_INTERRUPT statement,
      which also leads to deffering the event.
      Typically most events can be handled directely, but some require follow-up
      handling and LL interaction.

   \return TAPI_statusOk on ok, otherwise TAPI_statusErr
 */
IFX_int32_t IFX_TAPI_Event_DispatchExt (TAPI_CHANNEL *pChannel,
   IFX_TAPI_EVENT_t *pTapiEvent, IFX_boolean_t bDefer)
{
   IFX_int32_t ret = TAPI_statusOk;
   IFX_TAPI_EXT_EVENT_PARAM_t *pDeferredEvent;
   TAPI_OS_INTSTAT lock;

   TAPI_ASSERT (pChannel);
   TAPI_ASSERT (pTapiEvent);

   if (pChannel == IFX_NULL)
      return TAPI_statusErr;

   if (pTapiEvent == IFX_NULL)
      RETURN_STATUS(TAPI_statusParam, 0);

   /* filter error message and just copy the error stack */
   if (pTapiEvent->id == IFX_TAPI_EVENT_FAULT_GENERAL_CHINFO ||
       pTapiEvent->id == IFX_TAPI_EVENT_FAULT_GENERAL_DEVINFO)
   {
      TAPI_DEV *pTapiDev = pChannel->pTapiDevice;
      if (pTapiDev->error.nCnt < IFX_TAPI_MAX_ERROR_ENTRIES - 1)
      {
         memcpy (&pTapiDev->error.stack[pTapiDev->error.nCnt++],
                 (IFX_TAPI_ErrorLine_t*)pTapiEvent->data.error,
                 sizeof (IFX_TAPI_ErrorLine_t));
         pTapiDev->error.nCh   = pChannel->nChannel;
         pTapiDev->error.nCode =
            (IFX_uint32_t)pTapiEvent->data.error->nLlCode;
      }
      return TAPI_statusOk;
   }

   /* in force set the responsible device index */
   pTapiEvent->dev = pChannel->pTapiDevice->nDev;

   /* global irq lock - multiple drivers may be loaded and all share this
      event dispatch function. Lock access to the shared buffer pool. */
   TAPI_OS_PROTECT_IRQLOCK(&lockEventBufferPoolAcc, lock);

   pDeferredEvent = (IFX_TAPI_EXT_EVENT_PARAM_t *)
                               bufferPoolGet (pIFX_TAPI_BP_Deferred_Event);

   TAPI_OS_UNPROTECT_IRQLOCK(&lockEventBufferPoolAcc, lock);

   if (pDeferredEvent == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("TAPI dev%d,ch%d: failure dispatching event 0x%08x "
            "(no free pIFX_TAPI_BP_Deferred_Event buffers)\n\r",
            pTapiEvent->dev, pTapiEvent->ch, pTapiEvent->id));

      /* if no deferred_event buffer is free - discard the event as well :-/ */
      RETURN_STATUS (TAPI_statusErr, 0);
   }

   memcpy (&pDeferredEvent->tapiEvent, pTapiEvent, sizeof (IFX_TAPI_EVENT_t));
   pDeferredEvent->pChannel = pChannel;

   /* decide if the event can be handled immediately or needs to be scheduled
      into the process context first (if called from interrupt context) */
   if (!bDefer)
   {
#ifndef TAPI_LIBRARY
      TAPI_ASSERT (!TAPI_OS_IN_INTERRUPT());
#endif /* TAPI_LIBRARY */
      /* immediately handle this event, no scheduling required */
      ret = IFX_TAPI_Event_Dispatch_ProcessCtx (pDeferredEvent);
   }
   else
   {
      /*lint -save -e611 \todo: FIXME
         (ANSI X3.159-1989)
         It is invalid to convert a function pointer to an object pointer
         or a pointer to void, or vice-versa.
      */
      ret = (IFX_int32_t)TAPI_DeferWork(
                           (IFX_void_t *) IFX_TAPI_Event_Dispatch_ProcessCtx,
                           (IFX_void_t *) pDeferredEvent);
      /*lint -restore */
      /* error handling */
      if (ret != TAPI_statusOk)
      {
         /* deferring failed - return the deferred_event data structure
            to the bufferpool */
         tapi_DiscardEvent (pDeferredEvent);

         RETURN_STATUS (ret, 0);
      }
   }

   return ret;
}


/**
   Discard a TAPI deferred event buffer

   Helper routine used to return an allocated deferred event buffer back
   to its buffer pool. The routine takes care about concurrency protection.

   \param  pEvent       Pointer to event wrapper buffer.

   \remarks The function always succeeds.
*/
static IFX_void_t tapi_DiscardEvent (
                        IFX_TAPI_EXT_EVENT_PARAM_t *pEvent)
{
   TAPI_OS_INTSTAT lock;

   TAPI_OS_PROTECT_IRQLOCK(&lockEventBufferPoolAcc, lock);
   bufferPoolPut (pEvent);
   TAPI_OS_UNPROTECT_IRQLOCK(&lockEventBufferPoolAcc, lock);
}


/**
   High Level Event Dispatcher task.

   \param pParam - parameters with task structure and detected event.

   \return TAPI_statusOk on ok, otherwise TAPI_statusErr

   \remarks
   This function is spawned as a task to dispatch all events from process
   context. It is the single resource of event processing. The buffers it
   uses are allocated in IFX_TAPI_Event_Dispatch above which is called
   from interrupt or task context. So locking needs to be done during
   all buffer operations. Multiple drivers may be loaded which all can
   generate events. Instead of locking each driver separately we lock all
   interrupts globally. Additionally semaphores are needed to lock the
   access to the buffers and fifos.
 */
IFX_int32_t IFX_TAPI_Event_Dispatch_ProcessCtx(IFX_TAPI_EXT_EVENT_PARAM_t *pParam)
{
   IFX_int32_t ret = TAPI_statusOk;
   IFX_boolean_t bEventDisabled = IFX_FALSE;
   IFX_boolean_t bEventDiscard = IFX_TRUE;
   TAPI_CHANNEL* pChannel;
   IFX_TAPI_EVENT_t *pEvent, TapiEvent;
   IFX_TAPI_DRV_CTX_t *pDrvCtx = IFX_NULL;
   IFX_TAPI_EVENT_MASK_t *pEventMask;
   IFX_TAPI_MODULE_TYPE_t nModule;
   FIFO_ID *pDispatchFifo;

   TAPI_ASSERT(pParam);
   pChannel = pParam->pChannel;
   if ((IFX_NULL == pChannel) || (IFX_NULL == pChannel->pEventHandler))
   {
      tapi_DiscardEvent (pParam);
      /* no error for not yet initialized event handler */
      return TAPI_statusOk;
   }
   /* at this point pChannel is valid */

   pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;

   pEvent = &pParam->tapiEvent;
   pEvent->ch = (IFX_uint16_t) pChannel->nChannel;
   /**\todo log event with new log type:
    LOG_STR (pEvent->ch, "Evt ID 0x%X DATA 0x%X", pEvent->id, pEvent->data.value);
    */

#ifdef TAPI_VERSION3
   nModule = IFX_TAPI_MODULE_TYPE_ALM;
#else
   nModule = pEvent->module;
   if (nModule > IFX_TAPI_MODULE_TYPE_PCM)
   {
      tapi_DiscardEvent (pParam);
      RETURN_STATUS (TAPI_statusParam, 0);
   }
#endif

   pEventMask = &(pChannel->pEventHandler->eventMask);

   /* Errors are handled first and put into the high-priority fifo.
      Normal events are handled afterwards and go to the low-priority fifo. */
   if ((((IFX_uint32_t)pEvent->id) & IFX_TAPI_EVENT_TYPE_FAULT_MASK) ==
         IFX_TAPI_EVENT_TYPE_FAULT_MASK)
   {
      /*lint -save -e650 */
      /*
       * If any fault event occured, invoke the error state machine.
       *  Then, put this event into the high-priority FIFO.
       */
      pDispatchFifo = pChannel->pEventHandler->pTapiEventFifoHi;

      switch (((IFX_uint32_t)pEvent->id) & IFX_TAPI_EVENT_TYPE_MASK)
      {
      case IFX_TAPI_EVENT_TYPE_FAULT_GENERAL:
         {
            int src = pEvent->data.value & IFX_TAPI_ERRSRC_MASK;
            /* clear error clasification */
            pEvent->data.value &= ~(IFX_TAPI_ERRSRC_MASK);
            switch (src)
            {
               case  IFX_TAPI_ERRSRC_LL_DEV:
                  /**\todo put in device fifo */
                  pEvent->ch = IFX_TAPI_DEVICE_CH_NUMBER;
                  fallthrough;
                  /*lint -fallthrough */
               case  IFX_TAPI_ERRSRC_LL_CH:
                  pEvent->data.value |= IFX_TAPI_ERRSRC_LL;
                  break;
               case  IFX_TAPI_ERRSRC_TAPI_CH:
               case  IFX_TAPI_ERRSRC_TAPI_DEV:
               default:
               /* do nothing */
                  break;
            }
            bEventDisabled = IFX_FALSE;
         }
         break;
      case IFX_TAPI_EVENT_TYPE_FAULT_LINE:
         switch (pEvent->id)
         {
         case IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP:
#ifdef TAPI_FEAT_DIALENGINE
            IFX_TAPI_Dial_LineDisable(pChannel);
#endif /* TAPI_FEAT_DIALENGINE */
            pChannel->TapiOpControlData.nLineMode = IFX_TAPI_LINE_FEED_DISABLED;
            pChannel->TapiOpControlData.bEmergencyShutdown = IFX_TRUE;
            bEventDisabled = IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP_END:
            if (pChannel->TapiOpControlData.bEmergencyShutdown == IFX_TRUE)
            {
               TAPI_Phone_Set_Linefeed (pChannel, IFX_TAPI_LINE_FEED_DISABLED);
            }
            bEventDisabled = IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH:
            pChannel->TapiOpControlData.bEmergencyShutdown = IFX_TRUE;
            bEventDisabled = pEventMask->fault_line.gk_high ?
                            IFX_TRUE : IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH_INT:
            /* Internal GK high event - internal - unmaskable */
            pEvent->id = IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH;
            bEventDisabled = IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH_END:
            if (pChannel->TapiOpControlData.bEmergencyShutdown == IFX_TRUE)
            {
               TAPI_Phone_Set_Linefeed (pChannel, IFX_TAPI_LINE_FEED_DISABLED);
            }
            bEventDisabled = pEventMask->fault_line.gk_high_end ?
                            IFX_TRUE : IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_INT:
            /* Internal GK low event - internal - unmaskable */
            /* set the TAPI line mode to power down. The TAPI will not signal
               hook changes to the client application anymore till the mode is
               changed again */
            pChannel->TapiOpControlData.bFaulCond = IFX_TRUE;
            pEvent->id = IFX_TAPI_EVENT_FAULT_LINE_GK_LOW;
            bEventDisabled = IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_LINE_GK_LOW:
            bEventDisabled = pEventMask->fault_line.gk_low ?
                            IFX_TRUE : IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_END:
            bEventDisabled = pEventMask->fault_line.gk_low_end ?
                            IFX_TRUE : IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_LINE_OVERCURRENT:
            bEventDisabled = pEventMask->fault_line.overcurrent ?
                            IFX_TRUE : IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_LINE_GK_POS:
            bEventDisabled = pEventMask->fault_line.gk_pos ?
                            IFX_TRUE : IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_LINE_GK_NEG:
            bEventDisabled = pEventMask->fault_line.gk_neg ?
                            IFX_TRUE : IFX_FALSE;
            break;
         default:
            /* do nothing */
            break;
         }
#ifdef TAPI_FEAT_PHONE_DETECTION
         IFX_TAPI_PPD_HandleEvent(pChannel,pEvent);
#endif /* TAPI_FEAT_PHONE_DETECTION */
         break;
      case IFX_TAPI_EVENT_TYPE_FAULT_HW:
         switch (pEvent->id)
         {
         case IFX_TAPI_EVENT_FAULT_HW_SPI_ACCESS:
            bEventDisabled = pEventMask->fault_hw.spi_access ?
                            IFX_TRUE : IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL:
            bEventDisabled = pEventMask->fault_hw.clock_fail ?
                            IFX_TRUE : IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL_END:
            bEventDisabled = pEventMask->fault_hw.clock_fail_end ?
                            IFX_TRUE : IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_HW_FAULT:
            /* this fault event cannot be masked */
            bEventDisabled = IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_HW_SYNC:
            /* this fault event cannot be masked */
            bEventDisabled = IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_HW_RESET:
            /* this fault event cannot be masked */
            bEventDisabled = IFX_FALSE;
            break;
         case IFX_TAPI_EVENT_FAULT_HW_SSI_ERR:
         /*lint -fallthrough */
         case IFX_TAPI_EVENT_FAULT_HW_SSI_ERR_END:
            /* SSI fault events cannot be masked */
            bEventDisabled = IFX_FALSE;
            break;
         default:
         /* do nothing */
            break;
         }
         break;
      case IFX_TAPI_EVENT_TYPE_FAULT_FW:
         switch (pEvent->id)
         {
            case IFX_TAPI_EVENT_FAULT_FW_EBO_UF:
               bEventDisabled = pEventMask->fault_fw.fw_ebo_uf ?
                               IFX_TRUE : IFX_FALSE;
               break;
            case IFX_TAPI_EVENT_FAULT_FW_EBO_OF:
               /* Event mailbox out overflow event cannot be masked */
               bEventDisabled = IFX_FALSE;
               break;
            case IFX_TAPI_EVENT_FAULT_FW_CBO_UF:
               bEventDisabled = pEventMask->fault_fw.fw_cbo_uf ?
                               IFX_TRUE : IFX_FALSE;
               break;
            case IFX_TAPI_EVENT_FAULT_FW_CBO_OF:
               bEventDisabled = pEventMask->fault_fw.fw_cbo_of ?
                               IFX_TRUE : IFX_FALSE;
               break;
            case IFX_TAPI_EVENT_FAULT_FW_CBI_OF:
               bEventDisabled = pEventMask->fault_fw.fw_cbi_of ?
                               IFX_TRUE : IFX_FALSE;
               break;
            default:
            break;
         }
         break;
      default:
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                ("TAPI dev%d,ch%d: Unknown event ID: 0x%08x.\n",
                pEvent->dev, pEvent->ch, pEvent->id));
         break;
      }
      /*lint -restore */
   }
   else
   {
      /*
       * Normal events, put these into low-priority FIFO
       */
      pDispatchFifo = pChannel->pEventHandler->pTapiEventFifoLo;

      switch (pEvent->id)
      {
      /* EVENT TYPE: -- IFX_TAPI_EVENT_TYPE_FXS -- */

      case IFX_TAPI_EVENT_FXS_ONHOOK_INT:
      case IFX_TAPI_EVENT_FXS_OFFHOOK_INT:
         /* Internal hook events are generated by the CPE LL-drivers to report
            make or break changes on the analog line. The hook state machine
            integrates these events over time and generats new events from
            this. Depending on the timing on-hook, off-hook, flash-hook or
            pulse-dialing digits are reported. */
         {
#if defined(TAPI_FEAT_DIALENGINE) || defined(TAPI_FEAT_CID)
            IFX_boolean_t bSendHookEvent = IFX_TRUE;
#endif /* defined(TAPI_FEAT_DIALENGINE) || defined(TAPI_FEAT_CID) */
            IFX_boolean_t bOffhook =
                  (pEvent->id == IFX_TAPI_EVENT_FXS_OFFHOOK_INT) ?
                  IFX_TRUE : IFX_FALSE;

#ifdef TAPI_FEAT_CID
            IFX_TAPI_CID_OnHookEvent(pChannel, bOffhook, &bSendHookEvent);
#endif /* TAPI_FEAT_CID */

#ifdef TAPI_FEAT_DIALENGINE
            if (bSendHookEvent == IFX_TRUE)
            {
               /* trigger event in the hook state machine */
               IFX_TAPI_Dial_HookEvent (pChannel, bOffhook,
                                        pEvent->data.hook_int.nTime);
            }
#endif /* TAPI_FEAT_DIALENGINE */

            TRACE(TAPI_DXS, DBG_LEVEL_LOW,
                  ("TAPI d%d,c%d hook %s time %d %d/8 ms\n",
                   pEvent->dev, pEvent->ch,
                   bOffhook ? "make " : "break",
                   pEvent->data.hook_int.nTime / 8,
                   pEvent->data.hook_int.nTime % 8));

            /* The internal events are dropped unless they are relabeled
               to raw hook-events below. */
            bEventDisabled = IFX_TRUE;

            /* In case that the raw hook-events are not masked relabel the
               internal hook-events as raw hook-events and send them. */
            if ( !pEventMask->fxs.rawonhook && !bOffhook )
            {
               pEvent->id = IFX_TAPI_EVENT_FXS_RAW_ONHOOK;
               bEventDisabled = IFX_FALSE;
            }
            if ( !pEventMask->fxs.rawoffhook && bOffhook )
            {
               pEvent->id = IFX_TAPI_EVENT_FXS_RAW_OFFHOOK;
               bEventDisabled = IFX_FALSE;
            }
         }
         break;

      case IFX_TAPI_EVENT_FXS_ONHOOK:
         /* IFX_FALSE => nature of hook event is ONhook */
         pChannel->TapiOpControlData.bHookState = IFX_FALSE;
         bEventDisabled = pEventMask->fxs.onhook ? IFX_TRUE : IFX_FALSE;
         break;

      case IFX_TAPI_EVENT_FXS_OFFHOOK:
#ifdef TAPI_FEAT_RINGENGINE
         /* stop ringing (including an optional CID transmission) */
         IFX_TAPI_Ring_Stop(pChannel);
#endif /* TAPI_FEAT_RINGENGINE */
         /* IFX_TRUE => nature of hook event is OFFhook */
         pChannel->TapiOpControlData.bHookState = IFX_TRUE;
#ifdef TAPI_VERSION3
         /* In case of an validated off-hook line feeding is active. */
         pChannel->TapiOpControlData.nLineMode =
            (pChannel->TapiOpControlData.nPolarity == 0) ?
            IFX_TAPI_LINE_FEED_ACTIVE : IFX_TAPI_LINE_FEED_ACTIVE_REV;
#endif /* TAPI_VERSION3 */
         bEventDisabled = pEventMask->fxs.offhook ? IFX_TRUE : IFX_FALSE;
         break;

      case IFX_TAPI_EVENT_FXS_FLASH:
         bEventDisabled = pEventMask->fxs.flash ? IFX_TRUE : IFX_FALSE;
         break;

      case IFX_TAPI_EVENT_FXS_RAW_ONHOOK:
         /* IFX_FALSE => nature of hook event is ONhook */
         bEventDisabled = pEventMask->fxs.rawonhook ? IFX_TRUE : IFX_FALSE;
         break;

      case IFX_TAPI_EVENT_FXS_RAW_OFFHOOK:
         /* IFX_TRUE => nature of hook event is OFFhook */
         bEventDisabled = pEventMask->fxs.rawoffhook ? IFX_TRUE : IFX_FALSE;
         break;

      case IFX_TAPI_EVENT_FXS_RINGING_END:
         bEventDisabled = pEventMask->fxs.ringing_end ?
                         IFX_TRUE : IFX_FALSE;
         break;

#ifdef TAPI_FEAT_CONT_MEAS
      case IFX_TAPI_EVENT_CONTMEASUREMENT:
         bEventDisabled = pEventMask->fxs.contMeas ?
                         IFX_TRUE : IFX_FALSE;
         break;
#endif /* TAPI_FEAT_CONT_MEAS */

      /* line feed mode has changed */
      case IFX_TAPI_EVENT_FXS_LINE_MODE:
         TAPI_Phone_Change_Linefeed(pChannel, pEvent->data.linemode);
         bEventDisabled = pEventMask->fxs.linemode ? IFX_TRUE : IFX_FALSE;
         break;

#ifdef TAPI_FEAT_CID
      case IFX_TAPI_EVENT_FXS_RINGPAUSE_CIDTX:
         bEventDisabled = pEventMask->fxs.ringpausecidtx ? IFX_TRUE : IFX_FALSE;
         break;
#endif /* TAPI_FEAT_CID */

      case IFX_TAPI_EVENT_FXS_RING_ABORT:
         bEventDisabled = pEventMask->fxs.ring_abort ? IFX_TRUE : IFX_FALSE;
         break;

      /* EVENT TYPE: -- IFX_TAPI_EVENT_TYPE_LT -- */

#ifdef TAPI_FEAT_GR909
      case IFX_TAPI_EVENT_LT_GR909_RDY:
#ifdef TAPI_FEAT_DIALENGINE
         IFX_TAPI_Dial_LineDisable(pChannel);
#endif /* TAPI_FEAT_DIALENGINE */
         pChannel->TapiOpControlData.nLineMode = IFX_TAPI_LINE_FEED_DISABLED;
         bEventDisabled = pEventMask->lt.gr909_rdy ? IFX_TRUE : IFX_FALSE;
         break;
#endif /* TAPI_FEAT_GR909 */

      case IFX_TAPI_EVENT_NLT_END:
#ifdef TAPI_FEAT_PHONE_DETECTION
         if (IFX_TAPI_PPD_DisableEvent(pChannel) == IFX_TRUE)
         {
            bEventDisabled = IFX_TRUE;
         }
         else
#endif /* TAPI_FEAT_PHONE_DETECTION */
         {
#ifdef TAPI_FEAT_CAP_MEAS
            bEventDisabled = pEventMask->lt.nlt_end ?
                             IFX_TRUE : IFX_FALSE;
#endif /* TAPI_FEAT_CAP_MEAS */
         }
         break;

      case IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY:
#ifdef TAPI_FEAT_PHONE_DETECTION
         if (IFX_TAPI_PPD_DisableEvent(pChannel) == IFX_TRUE)
         {
            bEventDisabled = IFX_TRUE;
         }
         else
#endif /* TAPI_FEAT_PHONE_DETECTION */
         {
#ifdef TAPI_FEAT_CAP_MEAS
            bEventDisabled = pEventMask->lcap.linecap ?
                             IFX_TRUE : IFX_FALSE;
#endif /* TAPI_FEAT_CAP_MEAS */
         }
         break;

#ifdef TAPI_FEAT_CAP_MEAS
      case IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_GND_RDY:
         bEventDisabled = pEventMask->lcap.cap2gnd ?
                          IFX_TRUE : IFX_FALSE;
         break;
#endif /* TAPI_FEAT_CAP_MEAS */

#ifdef TAPI_FEAT_CAP_MEAS
      case IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY_INT:
         if (IFX_NULL != pDrvCtx->ALM.CapMeasResult)
         {
            (IFX_void_t)pDrvCtx->ALM.CapMeasResult(pChannel->pLLChannel);
         }
         bEventDisabled = IFX_TRUE;
         break;
#endif /* TAPI_FEAT_CAP_MEAS */

      /* EVENT TYPE: -- IFX_TAPI_EVENT_TYPE_PULSE -- */

      case IFX_TAPI_EVENT_PULSE_DIGIT:
         bEventDisabled = pEventMask->pulse.digit ? IFX_TRUE : IFX_FALSE;
         break;

      case IFX_TAPI_EVENT_PULSE_START:
         bEventDisabled = pEventMask->pulse.pulseStart ?
                         IFX_TRUE : IFX_FALSE;
         break;

      /* EVENT TYPE: -- IFX_TAPI_EVENT_TYPE_DTMF -- */

      case IFX_TAPI_EVENT_DTMF_DIGIT:
         {
#ifdef TAPI_FEAT_CID
            if (IFX_TAPI_CID_OnDtmfEvent(pChannel, pEvent->data.dtmf.ascii))
            {
               bEventDisabled = IFX_TRUE;
            }
            else
#endif /* TAPI_FEAT_CID */
            {
#ifdef TAPI_FEAT_DTMF
               if (TAPI_COD_EXTERNAL (pEvent->data.dtmf))
               {
                  bEventDisabled = pEventMask->dtmf[nModule].digit_external ?
                                  IFX_TRUE : IFX_FALSE;
               }
               else if (TAPI_COD_INTERNAL (pEvent->data.dtmf))
               {
                  bEventDisabled = pEventMask->dtmf[nModule].digit_internal ?
                                  IFX_TRUE : IFX_FALSE;
               }
#endif /* TAPI_FEAT_DTMF */
            }
         }
         break;

#ifdef TAPI_FEAT_DTMF
      case IFX_TAPI_EVENT_DTMF_END:
         if (TAPI_COD_INTERNAL (pEvent->data.dtmf))
         {
            bEventDisabled = pEventMask->dtmf[nModule].end_internal ?
                            IFX_TRUE : IFX_FALSE;
         }
         else if (TAPI_COD_EXTERNAL (pEvent->data.dtmf))
         {
            bEventDisabled = pEventMask->dtmf[nModule].end_external ?
                            IFX_TRUE : IFX_FALSE;
         }
         break;
#endif /* TAPI_FEAT_DTMF */

      /* EVENT TYPE: -- IFX_TAPI_EVENT_TYPE_CID -- */

#ifdef TAPI_FEAT_CID
      case IFX_TAPI_EVENT_CID_TX_END:
         /* This event is sent by the lower-layer after a data transmission
            has ended. This is here translated into the ...TX_INFO_END
            depending on the mode of the CID transmission. */
         if (TAPI_Cid_UseSequence (pChannel) == IFX_TRUE)
         {
            bEventDisabled = IFX_TRUE;
         }
         else
         {
            pEvent->id = IFX_TAPI_EVENT_CID_TX_INFO_END;
            bEventDisabled = pEventMask->cid.tx_info_end ?
                            IFX_TRUE : IFX_FALSE;
         }
         break;

      case IFX_TAPI_EVENT_CID_TX_SEQ_END:
         bEventDisabled = pEventMask->cid.tx_seq_end ?
                          IFX_TRUE : IFX_FALSE;
         break;

      case IFX_TAPI_EVENT_CIDSM_END:
#ifdef TAPI_FEAT_RINGENGINE
         /* stop ringing (including an optional CID transmission) */
         if (TAPI_Cid_UseSequence(pChannel) == IFX_TRUE)
         {
            IFX_TAPI_Ring_Stop_Ext(pChannel, IFX_TRUE);
         }
         else
         {
            /* Do not change line after CID info TX */
            IFX_TAPI_Ring_Stop_Ext(pChannel, IFX_FALSE);
         }
#endif /* TAPI_FEAT_RINGENGINE */
         bEventDisabled = pEventMask->cid.sm_end ?
                           IFX_TRUE : IFX_FALSE;
         break;

      case IFX_TAPI_EVENT_CID_TX_RINGCAD_ERR:
         bEventDisabled = pEventMask->cid.tx_ringcad_err ?
                         IFX_TRUE : IFX_FALSE;
         break;

      case IFX_TAPI_EVENT_CID_TX_NOACK_ERR:
         bEventDisabled = pEventMask->cid.tx_noack_err ?
                         IFX_TRUE : IFX_FALSE;
         break;

      case IFX_TAPI_EVENT_CID_TX_NOACK2_ERR:
#ifdef TAPI_FEAT_DIALENGINE
         /* This event is reported when the line is in make-state (off-hook)
            and fails to go to break-state (on-hook) within the correct
            time window of the CID sequence. Since the CID sequence consumed
            the hook events internally update now the hook state machine and
            the resulting hook state.*/
         IFX_TAPI_Dial_HookSet (pChannel, IFX_TRUE);
#endif /* TAPI_FEAT_DIALENGINE */
         pChannel->TapiOpControlData.bHookState = /* off-hook */IFX_TRUE;
         bEventDisabled = pEventMask->cid.tx_noack_err ?
                         IFX_TRUE : IFX_FALSE;
         break;

      case IFX_TAPI_EVENT_CID_TX_INFO_START:
         bEventDisabled = pEventMask->cid.tx_info_start ?
                         IFX_TRUE : IFX_FALSE;
         break;

      case IFX_TAPI_EVENT_CID_TX_SEQ_START:
         bEventDisabled = pEventMask->cid.tx_seq_start ?
                         IFX_TRUE : IFX_FALSE;
         break;
#endif /* TAPI_FEAT_CID */

      /* EVENT TYPE: -- IFX_TAPI_EVENT_TYPE_TONE_GEN -- */

#ifdef TAPI_FEAT_TONEENGINE
      case IFX_TAPI_EVENT_TONE_GEN_END:
         {
            bEventDisabled = IFX_TRUE;
            /* check event masks */
            if (TAPI_COD_EXTERNAL (pEvent->data.tone_gen) &&
                (!pEventMask->tone_gen.end_external))
               bEventDisabled = IFX_FALSE;

            if (TAPI_COD_INTERNAL (pEvent->data.tone_gen) &&
                (!pEventMask->tone_gen.end_internal))
               bEventDisabled = IFX_FALSE;
         }
         break;

      /* EVENT TYPE: -- IFX_TAPI_EVENT_TYPE_TONE_GEN_RAW -- */
      /*                not available to the application    */
      case IFX_TAPI_EVENT_TONE_GEN_END_RAW:
         {
            TAPI_Tone_Step_Completed (pChannel, (IFX_uint8_t)pEvent->data.value);
            bEventDisabled = IFX_TRUE;
         }
         break;
#endif /* TAPI_FEAT_TONEENGINE */

      /* EVENT TYPE: -- IFX_TAPI_EVENT_TYPE_TONE_DET -- */

      case IFX_TAPI_EVENT_TONE_DET_RECEIVE:
         bEventDisabled = pEventMask->tone_det[nModule].receive ?
                         IFX_TRUE : IFX_FALSE;
         break;
      case IFX_TAPI_EVENT_TONE_DET_TRANSMIT:
         bEventDisabled = pEventMask->tone_det[nModule].transmit ?
                         IFX_TRUE : IFX_FALSE;
         break;

      /* EVENT TYPE: -- IFX_TAPI_EVENT_TYPE_DEBUG -- */

      case IFX_TAPI_EVENT_DEBUG_CERR:
         /* print data if any data is available */
         if (pEvent->data.cerr.fw_id != 0x0000)
         {
            TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                  ("\n!!! cErrHdlr cause 0x%04X, cHdr 0x%08X\n",
                   pEvent->data.cerr.reason, pEvent->data.cerr.command));
         }
         bEventDisabled = pEventMask->debug.cerr ? IFX_TRUE : IFX_FALSE;
         break;

      /* EVENT TYPE: -- IFX_TAPI_EVENT_METERING -- */

#ifdef TAPI_FEAT_METERING
      case IFX_TAPI_EVENT_METERING_END:
         IFX_TAPI_Meter_EventServe (pChannel);
         bEventDisabled =
         pEventMask->metering.metering_end ? IFX_TRUE : IFX_FALSE;
         break;
#endif /* TAPI_FEAT_METERING */

      /* EVENT TYPE: -- IFX_TAPI_EVENT_TYPE_CALIBRATION -- */

#ifdef TAPI_FEAT_CALIBRATION
      case IFX_TAPI_EVENT_CALIBRATION_END:
#ifdef TAPI_FEAT_DIALENGINE
            IFX_TAPI_Dial_LineDisable(pChannel);
#endif /* TAPI_FEAT_DIALENGINE */
         pChannel->TapiOpControlData.nLineMode = IFX_TAPI_LINE_FEED_DISABLED;
         bEventDisabled =
            pEventMask->calibration.calibration_end ? IFX_TRUE : IFX_FALSE;
         break;

      case IFX_TAPI_EVENT_CALIBRATION_END_INT:
         if (IFX_TAPI_PtrChk (pDrvCtx->ALM.Calibration_Finish))
         {
            (IFX_void_t)pDrvCtx->ALM.Calibration_Finish(pChannel->pLLChannel);
         }
         bEventDisabled = IFX_TRUE;
         break;

      case IFX_TAPI_EVENT_CALIBRATION_END_SINT:
         if (IFX_TAPI_PtrChk (pDrvCtx->ALM.Calibration_Finish))
         {
            (IFX_void_t)pDrvCtx->ALM.Calibration_Finish(pChannel->pLLChannel);
         }
         bEventDisabled = IFX_TRUE;
         break;
#endif /* TAPI_FEAT_CALIBRATION */

      default:
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                ("TAPI dev%d,ch%d: Unknown event ID: 0x%08x.\n",
                pEvent->dev, pEvent->ch, pEvent->id));
         break;
      }

#ifdef TAPI_FEAT_PHONE_DETECTION
      IFX_TAPI_PPD_HandleEvent(pChannel,pEvent);
#endif /* TAPI_FEAT_PHONE_DETECTION */
   }

   /* Make a copy of the event for use after the next block in which the
      ownership of the event is transferred. */
   memcpy (&TapiEvent, pEvent, sizeof (TapiEvent));

   if (bEventDisabled == IFX_FALSE)
   {
      /* Event is enabled so we put it into its respective fifo
         and wake up the application sleeping on read file descriptor */

      /* Lock the fifo access protection */
      TAPI_OS_MutexGet (&pChannel->pEventHandler->fifoAcc);
      TAPI_OS_SPIN_LOCK_IRQSAVE(&pChannel->pEventHandler->slProtectFifoAcc);

      if (fifoPut(pDispatchFifo, (IFX_void_t *)&pParam) == IFX_SUCCESS)
      {
         tapi_WakeUp (pChannel->pTapiDevice);
         /* the event is now owned by the fifo - do not discard it here */
         bEventDiscard = IFX_FALSE;
      }
      else
      {
         IFX_TAPI_EXT_EVENT_PARAM_t *pLostDeferredEvent = IFX_NULL;

         /* Event fifo queue full. This is not a driver error. The fifo
            contains a limited depth and needs to be read out by the
            application to be emptied. When the fifo gets full every
            consequtive event gets discarded. */

         /* Retrieve the event that was written last into the fifo and
            overwrite it with an fifo overflow event. */
         if (fifoPeekTail (pDispatchFifo, &pLostDeferredEvent) == IFX_SUCCESS)
         {
            IFX_TAPI_EVENT_t *pLostEvent = &pLostDeferredEvent->tapiEvent;

            if (pLostEvent->id !=
                IFX_TAPI_EVENT_FAULT_GENERAL_EVT_FIFO_OVERFLOW)
            {
               TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                      ("TAPI dev%d,ch%d: Event 0x%08x lost (first).\n",
                      pLostEvent->dev, pLostEvent->ch, pLostEvent->id));

               /* Overwrite the Event-ID. */
               pLostEvent->id =
                  IFX_TAPI_EVENT_FAULT_GENERAL_EVT_FIFO_OVERFLOW;
               /* We just lost the event that we have overwritten. */
               pLostEvent->data.event_fifo.lost = 1;
            }
            pLostEvent->data.event_fifo.lost++;
         }

         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                ("TAPI dev%d,ch%d: Event 0x%08x lost.\n",
                pEvent->dev, pEvent->ch, pEvent->id));

         /* errmsg: Event fifo queue full */
         ret = TAPI_statusEvtFifoFull;
      }

      TAPI_OS_SPIN_UNLOCK_IRQRESTORE(&pChannel->pEventHandler->slProtectFifoAcc);
      TAPI_OS_MutexRelease (&pChannel->pEventHandler->fifoAcc);
   }

   if (bEventDiscard == IFX_TRUE)
   {
      tapi_DiscardEvent (pParam);
   }

   /* reset TAPI device on serious failure */
   /* This works on a copy of the event because the ownership of the event
      has been changed just above. */
   switch (TapiEvent.id)
   {
      case IFX_TAPI_EVENT_FAULT_HW_SYNC:
         if (IFX_TAPI_EVENT_FAULT_HW_SYNC_SPI_ABORT != TapiEvent.data.hw_sync)
            break;
         fallthrough;
         /*lint -fallthrough */
      case IFX_TAPI_EVENT_FAULT_HW_RESET:
      case IFX_TAPI_EVENT_FAULT_FW_EBO_OF:
         IFX_TAPI_DeviceReset (pChannel->pTapiDevice);
         /*lint -fallthrough */
      default:
         break;
   }

   RETURN_STATUS (ret, 0);
}


/**
   Resource allocation and initialisation upon loading the driver.

   This function allocates memory for the buffer pool of events that the
   event dispatcher transports. The buffers contain the event and additional
   fields needed to route and defer the event.
   The pool is shared between all devices of this driver.

   \return
   Returns IFX_ERROR in case of an error, otherwise returns IFX_SUCCESS.
*/
IFX_int32_t IFX_TAPI_Event_On_Driver_Start(void)
{
   if (pIFX_TAPI_BP_Deferred_Event == IFX_NULL)
   {
#if defined(LINUX) && defined(__KERNEL__)
   /* not needed in linux kernel, linux already protecting insmod */
#else
   TAPI_OS_INTSTAT lock;
#endif

#ifndef TAPI_FEAT_LINUX_SMP
      /* initialize buffer pool access protection semaphore */
      TAPI_OS_MutexInit (&lockEventBufferPoolAcc);
#endif /* TAPI_FEAT_LINUX_SMP */

      /* allocate memory pool for the deferred event structures */
#if defined(LINUX) && defined(__KERNEL__)
   /* not needed in linux kernel, linux already protecting insmod */
#else
      TAPI_OS_PROTECT_IRQLOCK(&lockEventBufferPoolAcc, lock);
#endif
      pIFX_TAPI_BP_Deferred_Event =
         bufferPoolInit(sizeof (IFX_TAPI_EXT_EVENT_PARAM_t),
                        IFX_TAPI_EVENT_POOL_INITIAL_SIZE,
                        IFX_TAPI_EVENT_POOL_GROW_SIZE,
                        IFX_TAPI_EVENT_POOL_GROW_LIMIT);
      bufferPoolIDSet(pIFX_TAPI_BP_Deferred_Event, 10);

#if defined(LINUX) && defined(__KERNEL__)
   /* not needed in linux kernel, linux already protecting insmod */
#else
      TAPI_OS_UNPROTECT_IRQLOCK(&lockEventBufferPoolAcc, lock);
#endif
   }

   return (pIFX_TAPI_BP_Deferred_Event != IFX_NULL) ? IFX_SUCCESS : IFX_ERROR;
}


/**
   Free resources upon unloading the driver.

   This function frees the memory for the buffer pool of events that the
   event dispatcher transports.
*/
IFX_void_t IFX_TAPI_Event_On_Driver_Stop(void)
{
   TAPI_OS_INTSTAT lock;

   TAPI_OS_PROTECT_IRQLOCK(&lockEventBufferPoolAcc, lock);

   /* free memory pool for the deferred event structures */
   if (pIFX_TAPI_BP_Deferred_Event != IFX_NULL)
   {
      if (bufferPoolFree (pIFX_TAPI_BP_Deferred_Event) != IFX_SUCCESS)
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                ("WARN: Free deferred-event buffer error.\n"));
      }
      pIFX_TAPI_BP_Deferred_Event = IFX_NULL;
   }
   TAPI_OS_UNPROTECT_IRQLOCK(&lockEventBufferPoolAcc, lock);

#ifndef TAPI_FEAT_LINUX_SMP
   /* delete the buffer pool access protection semaphore */
   TAPI_OS_MutexDelete (&lockEventBufferPoolAcc);
#endif /* TAPI_FEAT_LINUX_SMP */
}


/**
   High Level Event Dispatcher function init.

   \param  pChannel     Pointer to tapi channel structure.

   \return
   - TAPI_statusNoMem No memory for data storage
   - TAPI_statusErr   Initialisiation of FIFOs failed
   - TAPI_statusOk    Initialisation successful
*/
IFX_int32_t IFX_TAPI_EventDispatcher_Init (TAPI_CHANNEL * pChannel)
{
   IFX_TAPI_EVENT_HANDLER_DATA_t *pEventHandlerData;
   IFX_int32_t ret = TAPI_statusOk;

   TAPI_ASSERT(pChannel);

   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);

   /* allocate data storage on the channel if not already existing */
   if (pChannel->pEventHandler == IFX_NULL)
   {
      /* allocate data storage */
      if ((pChannel->pEventHandler =
           TAPI_OS_Malloc (sizeof(*pChannel->pEventHandler))) == IFX_NULL)
      {
         TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);
         /* errmsg: Memory not available */
         RETURN_STATUS (TAPI_statusNoMem, 0);
      }
      /* zero the just allocated data storage */
      memset(pChannel->pEventHandler, 0x00, sizeof(*pChannel->pEventHandler));
   }
   /* from here on pChannel->pEventHandler is valid */

   /* Get pointer to data in the channel. */
   pEventHandlerData = pChannel->pEventHandler;

   /* Initialize high and low priority FIFO struct */
   if (pEventHandlerData->pTapiEventFifoHi == IFX_NULL)
   {
      pEventHandlerData->pTapiEventFifoHi =
         fifoInit(IFX_TAPI_EVENT_FIFO_SIZE, sizeof(void *));
   }
   if (pEventHandlerData->pTapiEventFifoLo == IFX_NULL)
   {
      pEventHandlerData->pTapiEventFifoLo =
         fifoInit(IFX_TAPI_EVENT_FIFO_SIZE, sizeof(void *));
      /* initialize fifo access protection semaphore */
      TAPI_OS_MutexInit (&pEventHandlerData->fifoAcc);
      TAPI_OS_SPIN_LOCK_INIT (&pEventHandlerData->slProtectFifoAcc);
   }

   if ((pEventHandlerData->pTapiEventFifoHi == NULL) ||
       (pEventHandlerData->pTapiEventFifoLo == IFX_NULL))
   {
      /* cleanup previously allocated memory */
      if (pEventHandlerData->pTapiEventFifoHi != NULL)
      {
         fifoFree (pEventHandlerData->pTapiEventFifoHi);
      }
      TAPI_OS_MutexDelete (&pEventHandlerData->fifoAcc);

      TAPI_OS_Free (pChannel->pEventHandler);
      pChannel->pEventHandler = IFX_NULL;

      TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);
      /* errmsg: Memory not available */
      RETURN_STATUS (TAPI_statusNoMem, 0);
   }

#ifdef TAPI_VERSION4
   /* Set the mask to disable all events. */
   memset(&pEventHandlerData->eventMask, IFX_EVENT_DISABLE * 0xFF,
          sizeof (pEventHandlerData->eventMask));

   /* enable events */
   {

      /* enable all maskable fault events */
      /* memset(&pEventHandlerData->eventMask.fault_general,
             IFX_EVENT_ENABLE * 0xFF,
             sizeof (pEventHandlerData->eventMask.fault_general)); unused */

      memset(&pEventHandlerData->eventMask.fault_hw,
             IFX_EVENT_ENABLE * 0xFF,
             sizeof (pEventHandlerData->eventMask.fault_hw));

      memset(&pEventHandlerData->eventMask.fault_fw,
             IFX_EVENT_ENABLE * 0xFF,
             sizeof (pEventHandlerData->eventMask.fault_fw));

      /* memset(&pEventHandlerData->eventMask.fault_sw,
             IFX_EVENT_ENABLE * 0xFF,
             sizeof (pEventHandlerData->eventMask.fault_sw)); for future use */
   }
#endif /* TAPI_VERSION4 */

#ifdef TAPI_VERSION3
   /* Clear the mask to enable all events. */
   memset(&pEventHandlerData->eventMask, IFX_EVENT_ENABLE * 0xFF,
          sizeof (pEventHandlerData->eventMask));

   /* disable events */
   {
      /* disable Pulse-start event */
      pEventHandlerData->eventMask.pulse.pulseStart = IFX_EVENT_DISABLE;

      /* disable raw hook events */
      pEventHandlerData->eventMask.fxs.rawonhook  = IFX_EVENT_DISABLE;
      pEventHandlerData->eventMask.fxs.rawoffhook = IFX_EVENT_DISABLE;
#ifdef TAPI_FEAT_CID
      pEventHandlerData->eventMask.fxs.ringpausecidtx = IFX_EVENT_DISABLE;
#endif /* TAPI_FEAT_CID */
   }
#endif /* TAPI_VERSION3 */

   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);

   /* from here on pChannel->pEventHandler is valid */
   return ret;
}

/**
   High Level Event Dispatcher function exit.

   \param  pChannel     Pointer to tapi channel structure.

   \return IFX_SUCCESS on ok, otherwise IFX_ERROR
*/
IFX_int32_t IFX_TAPI_EventDispatcher_Exit (TAPI_CHANNEL * pChannel)
{
   IFX_TAPI_EVENT_HANDLER_DATA_t *pEventHandlerData;
   IFX_int32_t ret = TAPI_statusOk;
   IFX_void_t *pEvt = IFX_NULL;

   TAPI_ASSERT(pChannel);

   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);

   /* Get pointer to data in the channel. */
   pEventHandlerData = pChannel->pEventHandler;

   if (pEventHandlerData != IFX_NULL)
   {
      /* Lock the fifo access protection. */
      TAPI_OS_MutexGet (&pEventHandlerData->fifoAcc);
      TAPI_OS_SPIN_LOCK_IRQSAVE(&pEventHandlerData->slProtectFifoAcc);

      /* Silently drop all events stored in high-prio fifo */
      while (fifoGet(pEventHandlerData->pTapiEventFifoHi, &pEvt) == IFX_SUCCESS)
      {
         bufferPoolPut(pEvt);
      }
      /* Silently drop all events stored in low-prio fifo */
      while (fifoGet(pEventHandlerData->pTapiEventFifoLo, &pEvt) == IFX_SUCCESS)
      {
         bufferPoolPut(pEvt);
      }

      /* Free the low prio fifo itself */
      if (fifoFree (pEventHandlerData->pTapiEventFifoHi) != IFX_SUCCESS)
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                ("INFO: Free event hi fifo error. ch(%d)\n", pChannel->nChannel));
         ret = IFX_ERROR;
      }
      /* Free the high prio fifo itself */
      if (fifoFree (pEventHandlerData->pTapiEventFifoLo) != IFX_SUCCESS)
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                ("INFO: Free event lo fifo error. ch(%d)\n", pChannel->nChannel));
         ret = IFX_ERROR;
      }

      /* Unlock the protection and delete the semaphore */
      TAPI_OS_SPIN_UNLOCK_IRQRESTORE(&pEventHandlerData->slProtectFifoAcc);
      TAPI_OS_MutexRelease (&pEventHandlerData->fifoAcc);
      TAPI_OS_MutexDelete (&pEventHandlerData->fifoAcc);

      /* Free the data storage on the channel */
      TAPI_OS_Free (pChannel->pEventHandler);
      pChannel->pEventHandler = IFX_NULL;
   }

   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);

   return ret;
}


/**
   Retrieve the overall number of elements of the event wrapper bufferpool.

   \return the overall number of elements
*/
IFX_int32_t IFX_TAPI_EventWrpBufferPool_ElementCountGet(void)
{
   TAPI_OS_INTSTAT lock;
   IFX_int32_t  elements;

   TAPI_OS_PROTECT_IRQLOCK(&lockEventBufferPoolAcc, lock);
   elements = bufferPoolSize( pIFX_TAPI_BP_Deferred_Event );
   TAPI_OS_UNPROTECT_IRQLOCK(&lockEventBufferPoolAcc, lock);

   return elements;
}


/**
   Retrieve the available (free) number of elements of the
   event bufferpool.

   \return the number of available elements
*/
IFX_int32_t IFX_TAPI_EventWrpBufferPool_ElementAvailCountGet(void)
{
   TAPI_OS_INTSTAT lock;
   IFX_int32_t  elements;

   TAPI_OS_PROTECT_IRQLOCK(&lockEventBufferPoolAcc, lock);
   elements = bufferPoolAvail( pIFX_TAPI_BP_Deferred_Event );
   TAPI_OS_UNPROTECT_IRQLOCK(&lockEventBufferPoolAcc, lock);

   return elements;
}

/**
   Generate hook event for testing purpose

   \param   pChannel    Handle to TAPI channel structure.
   \param   nHook       Hook state

   \return
      TAPI_statusOk - success
      TAPI_statusLLNotSupp - sefice not supported by low-level driver
      TAPI_statusLLFailed - failed low-level call
*/
IFX_int32_t TAPI_Test_Hook_Gen (TAPI_CHANNEL *pChannel, IFX_uint32_t nHook)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;
   IFX_int32_t retLL;

   /* Check for LL driver functionality */
   if (!IFX_TAPI_PtrChk (pDrvCtx->ALM.TestHookGen))
   {
      RETURN_STATUS (TAPI_statusLLNotSupp, 0);
   }

   retLL = pDrvCtx->ALM.TestHookGen (pChannel->pLLChannel,
      (IFX_boolean_t)nHook);
   if (!TAPI_SUCCESS(retLL))
   {
      RETURN_STATUS (TAPI_statusLLFailed, retLL);
   }

   RETURN_STATUS (TAPI_statusOk, 0);
}

/**
   Retrieve event from TAPI device(s)

   Multiple devices will be served in a round robin fashion. For this the
   device to be serviced next will be remembered in the device context.
   Please note that also within a device the channels are served in a
   round robin fashion.

   \param   pDrvCtx  Handle to the low-level driver context
   \param   pEvent   Handle to the memory for event

   \return
      TAPI_statusOk - success
      TAPI_statusParam - incorrect parameters are passed
*/
IFX_int32_t TAPI_Event_Get (IFX_TAPI_DRV_CTX_t *pDrvCtx,
                            IFX_TAPI_EVENT_t *pEvent)
{
   TAPI_DEV *pTapiDev = IFX_NULL;
   IFX_uint16_t nDevCnt, nDevIdx;
   IFX_int32_t ret = TAPI_statusOk;

   if (pEvent->dev == IFX_TAPI_EVENT_ALL_DEVICES)
   {
      /* Start with the device the last event get did not check. */
      nDevIdx = pDrvCtx->nLastEventDevice;
      /* Check all devices which this LL driver has. */
      nDevCnt = pDrvCtx->maxDevs;
   }
   else if (pEvent->dev >= pDrvCtx->maxDevs)
   {
      return TAPI_statusParam;
   }
   else
   {
      /* Check only the single requested device. */
      nDevIdx = pEvent->dev;
      nDevCnt = 1;
   }

   for (pEvent->id = IFX_TAPI_EVENT_NONE;
        (nDevCnt > 0) && TAPI_SUCCESS (ret) && (pEvent->id == IFX_TAPI_EVENT_NONE);
        nDevCnt--, nDevIdx++)
   {
      nDevIdx = (nDevIdx >= pDrvCtx->maxDevs) ? 0 : nDevIdx;
      pTapiDev = pDrvCtx->pTapiDev + nDevIdx;

      /* touch only initialized devices */
      if (pTapiDev->bInitialized == IFX_TRUE)
      {
         ret = tapi_PhoneEventGet (pTapiDev, pEvent);
      }
   }

   /* Remember the device for the next call. */
   pDrvCtx->nLastEventDevice = nDevIdx;

   return ret;
}


/**
   Enables or disables a event on a given channel. The parameter
   specifies which channel and also the details of the event.

   \param   pDrvCtx     Handle to the low-level driver context
   \param   pEvent      Handle to the event descriptor
   \param   nEvtState   Event reporting state
                        -IFX_EVENT_ENABLE to allow the event to the application
                        -IFX_EVENT_DISABLE to discard the event

   \return TAPI_statusOk on success
*/
static IFX_int32_t tapi_EventStateSet (
   IFX_TAPI_DRV_CTX_t *pDrvCtx,
   IFX_TAPI_EVENT_t *pEvent,
   IFX_uint32_t nEvtState)
{
   TAPI_DEV *pTapiDev = IFX_NULL;
   IFX_uint32_t nDevMax, nCh, nChMin, nChMax;
   IFX_int32_t ret = TAPI_statusOk;

   if (pEvent->dev == IFX_TAPI_EVENT_ALL_DEVICES)
   {
      /*  select first device */
      pTapiDev = pDrvCtx->pTapiDev;
      nDevMax = pDrvCtx->maxDevs;
   }
   else if (pEvent->dev >= pDrvCtx->maxDevs)
   {
      return TAPI_statusParam;
   }
   else
   {
      pTapiDev = pDrvCtx->pTapiDev + pEvent->dev;
      nDevMax = pEvent->dev;
   }

   do /* for all devices */
   {
      /* touch only initialized devices */
      if (pTapiDev->bInitialized)
      {
         if (pEvent->ch == IFX_TAPI_EVENT_ALL_CHANNELS)
         {
            nChMin = 0;
            switch (pEvent->module)
            {
               case IFX_TAPI_MODULE_TYPE_ALM:
                  nChMax = pTapiDev->nResource.AlmCount;
                  break;
               case IFX_TAPI_MODULE_TYPE_PCM:
                  nChMax = pTapiDev->nResource.PcmCount;
                  break;
               default:
                  nChMax = pTapiDev->nMaxChannel;
                  break;
            }
         }
         else if (pEvent->ch >= pTapiDev->nMaxChannel)
         {
            TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
              ("TAPI: IFX_TAPI_EVENT_EN-/DISABLE called with channel "
               "parameter '%d' out of range for device '%d'. Device parameter "
               "'%d'.\n",
               pEvent->ch, pTapiDev->nDev, pEvent->dev));
            continue;
         }
         else
         {
            nChMin = pEvent->ch;
            nChMax = nChMin + 1;
         }

         for (nCh = nChMin; nCh < nChMax; nCh++)
         {
            /* call function to set event reporting mask according to the
               ioctl on either one or all channels of a device */
            ret = IFX_TAPI_Event_SetMask (pTapiDev->pChannel + nCh,
               pEvent, nEvtState);

            if (!TAPI_SUCCESS (ret))
               RETURN_DEVSTATUS (ret, 0);
         }
      }
   }
   while (TAPI_SUCCESS (ret) &&
      ((++pTapiDev) < (pDrvCtx->pTapiDev + nDevMax)));

   return ret;
}

/**
   Enables or disables a event on a given channel. The parameter
   specifies which channel and also the details of the event.

   \param   pDrvCtx     Handle to the low-level driver context
   \param   pEvent      Handle to the event descriptor
   \param   nEvtState   Event reporting state
                        -IFX_EVENT_ENABLE to allow the event to the application
                        -IFX_EVENT_DISABLE to discard the event
   \param  bCompatIoctl Flag indicating compatibility mode

   \return TAPI_statusOk on success
*/
IFX_int32_t TAPI_EventMultiState_Set (IFX_TAPI_DRV_CTX_t *pDrvCtx,
   IFX_TAPI_EVENT_MULTI_t *pMulEvent, IFX_uint32_t nEvtState,
   IFX_boolean_t const *bCompatIoctl)
{
   IFX_TAPI_EVENT_t event;
   IFX_uint16_t idx;
   IFX_int32_t ret = TAPI_statusOk;
   IFX_TAPI_EVENT_ENTRY_t *pEvent = IFX_NULL;

   if (pMulEvent->nCount == 0) /* nCount is unsigned and cannot be negative */
   {
      /* nothing to do for empty list */
      return ret;
   }

#if defined (LINUX) && defined (__KERNEL__)
   /* for compat case  all CID data is already available in kernelspace */
   if(*bCompatIoctl)
      pEvent = pMulEvent->pEvent;
   else
   {
      pEvent = TAPI_OS_Malloc (pMulEvent->nCount * sizeof (IFX_TAPI_EVENT_ENTRY_t));
      if (!IFX_TAPI_PtrChk (pEvent))
         return TAPI_statusNoMem;

      if (!TAPI_OS_CpyUsr2Kern (pEvent, pMulEvent->pEvent,
            pMulEvent->nCount * sizeof (IFX_TAPI_EVENT_ENTRY_t)))
      {
         TAPI_OS_Free (pEvent);
         return TAPI_statusErrKernCpy;
      }
   }
#else
   pEvent = pMulEvent->pEvent;
#endif /* LINUX && __KERNEL__ */

   memset (&event, 0, sizeof (event));

   event.dev = pMulEvent->dev;
   event.ch = pMulEvent->ch;
#ifdef TAPI_VERSION4
   event.module = pMulEvent->module;
#endif /* TAPI_VERSION4 */

   for (idx = 0;idx < pMulEvent->nCount; idx++)
   {
      event.id = pEvent[idx].id;
      event.data = pEvent[idx].data;

      /* apply single event */
      ret = tapi_EventStateSet (pDrvCtx, &event, nEvtState);

      if (!TAPI_SUCCESS (ret))
      {
         if (pEvent != pMulEvent->pEvent)
            TAPI_OS_Free (pEvent);

         return ret;
      }
   }

   if (pEvent != pMulEvent->pEvent)
      TAPI_OS_Free (pEvent);

   return ret;
}

/**
   Enable a event on a given channel. The parameter
   specifies which channel and also the details of the event.

   \param   pDrvCtx     Handle to the low-level driver context
   \param   pEvent      Handle to the event descriptor

   \return TAPI_statusOk on success
*/
IFX_int32_t IOCTL_TAPI_EventEnable (IFX_TAPI_DRV_CTX_t *pDrvCtx, IFX_TAPI_EVENT_t *pEvent)
{
   return tapi_EventStateSet (pDrvCtx, pEvent, IFX_EVENT_ENABLE);
}

/**
   Disable a event on a given channel. The parameter
   specifies which channel and also the details of the event.

   \param   pDrvCtx     Handle to the low-level driver context
   \param   pEvent      Handle to the event descriptor

   \return TAPI_statusOk on success
*/
IFX_int32_t IOCTL_TAPI_EventDisable (IFX_TAPI_DRV_CTX_t *pDrvCtx, IFX_TAPI_EVENT_t *pEvent)
{
   return tapi_EventStateSet (pDrvCtx, pEvent, IFX_EVENT_DISABLE);
}

/**
   Enable a event on a given channel. The parameter
   specifies which channel and also the details of the event.

   \param   pDrvCtx     Handle to the low-level driver context
   \param   pEvent      Handle to the event descriptor

   \return TAPI_statusOk on success
*/
IFX_int32_t TAPI_EventMultiEnable (IFX_TAPI_DRV_CTX_t *pDrvCtx, IFX_TAPI_EVENT_MULTI_t *pMulEvent)
{
   IFX_boolean_t bCompatIoctl = IFX_FALSE;

   return TAPI_EventMultiState_Set (pDrvCtx, pMulEvent, IFX_EVENT_ENABLE,
                                    (IFX_boolean_t const *)&bCompatIoctl);
}

/**
   Disable a event on a given channel. The parameter
   specifies which channel and also the details of the event.

   \param   pDrvCtx     Handle to the low-level driver context
   \param   pEvent      Handle to the event descriptor

   \return TAPI_statusOk on success
*/
IFX_int32_t TAPI_EventMultiDisable (IFX_TAPI_DRV_CTX_t *pDrvCtx, IFX_TAPI_EVENT_MULTI_t *pMulEvent)
{
   IFX_boolean_t bCompatIoctl = IFX_FALSE;

   return TAPI_EventMultiState_Set (pDrvCtx, pMulEvent, IFX_EVENT_DISABLE,
                                    (IFX_boolean_t const *)&bCompatIoctl);
}

/*lint -restore*/
