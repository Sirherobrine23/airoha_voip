/******************************************************************************

  Copyright 2014      Lantiq Deutschland GmbH
  Copyright 2022      MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/** \file drv_tapi_ring.c
    Contains TAPI Ringing Services.
    Implementation of the TAPI ring state machine which controls ringing on
    analog lines.
    \remarks
    All operations done by functions in this module are operating on analog
    phone lines and require a file descriptor with an ALM module. */

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_tapi.h"

#ifdef TAPI_FEAT_RINGENGINE

#include "drv_tapi_api.h"
#include "drv_tapi_errno.h"
#include "drv_tapi_cid.h"
#include "drv_tapi_ppd.h"

/* ============================= */
/* Local macros and definitions  */
/* ============================= */

/* Time by which the stop command is sent early. */
#define TAPI_RING_BURST_EARLY_STOP 10 /* ms */

/* ============================= */
/* Type declarations             */
/* ============================= */

enum TAPI_RING_PENDING_EVENT
{
   TAPI_RING_PENDING_EVENT_NONE,
   TAPI_RING_PENDING_EVENT_CID,
   TAPI_RING_PENDING_EVENT_FINAL,
};

struct TAPI_RING_DATA
{
   /* timer id for ringing service */
   Timer_ID                   RingTimerID;
   /* is channel in ringing mode? */
   volatile IFX_boolean_t     bRingingMode;
   /* is channel in ring_burst mode? */
   IFX_boolean_t              bIsRingBurstState;
   /* should the FXS_RINGPAUSE_CIDTX be sent */
   IFX_boolean_t              bSendRingPauseCidTx;
   /* still need to trigger CID? */
   IFX_boolean_t              bTriggerCid;
   /* cadence data */
   IFX_TAPI_RING_CADENCE_t    RingCadence;
   /* counter to keep the current position in the cadence */
   IFX_uint16_t               nBitPos;
   /* pointer to the cadence we currently use */
   IFX_uint8_t                *pCurrentCadence;
   /* copy of the number of bits in the cadence we currently use */
   IFX_uint16_t               BitsInCurrentCadence;
   /* indicates that this is the final sequence of the last ring */
   IFX_boolean_t              bFinal;
   /* ringing will stop automatically after the maximum ring was reached */
   IFX_uint32_t               nMaxRings;
   /* keep remaining rings until nMaxRings */
   IFX_uint32_t               nRingsLeft;
   /* time in ms remaining from last delay - needs to be added to next delay */
   IFX_uint32_t               nTimerRemainder;
   /* indicates whether sending a TAPI event is pending */
   enum TAPI_RING_PENDING_EVENT nPendingEvent;
   /* Duration of one bit of the ring cadence in milliseconds. */
   IFX_uint32_t               nRingGranularity;
   /* event to wait on for blocking ringing */
   TAPI_OS_event_t            TapiRingEvent;
   /* locking the access to the ring event for blocking ringing is needed
      because the event is deleted and initialised to clear the event */
   TAPI_OS_lock_t             LockRingEvent;
};


/* ============================= */
/* Local function declarations   */
/* ============================= */
static IFX_uint32_t ifx_tapi_ring_get_next_time (
                        TAPI_RING_DATA_t *pRingData,
                        IFX_boolean_t *bIsBurst,
                        IFX_boolean_t *bLast);

static IFX_int32_t  ifx_tapi_ring_cadence_play (
                        TAPI_CHANNEL *pChannel);

static IFX_void_t   ifx_tapi_ring_OnTimer (
                        Timer_ID Timer,
                        IFX_ulong_t nArg);

static IFX_int32_t ifx_tapi_cid_tx_stop(
                        TAPI_CHANNEL *pChannel);

static IFX_int32_t ifx_tapi_ring_stop(
                        TAPI_CHANNEL *pChannel,
                        IFX_boolean_t bRestoreLineFeed);

static IFX_int32_t ifx_tapi_LineFeedSetByHookState (
                        TAPI_CHANNEL *pChannel);

/* ============================= */
/* Local function definitions    */
/* ============================= */

/**
   Gets the time of the next sequence.

   Each cadence consists of multiple sequences of bits of the same value 1/0.
   This function counts the number of bits in the sequence that starts at the
   current position. The minimum count is in all cases at least one bit.
   Each bit represents 50 ms time.
   When the entire cadence consists only of set or cleared bits the time
   will be set to 0 to indicate that the sequence is infinite. To detect
   such a condition the algorithm aborts on the second reload of a cadence.

   \param   pRingData   Pointer to the ring data used for this call.
   \param   bIsBurst    Pointer to return if this sequence is a burst.
   \param   bLast       Pointer to return if this is the last sequence in this
                        cadence.

   \return  Time of the sequence in ms.
            The value 0 is special and indicates an infinite sequence.
*/
static IFX_uint32_t ifx_tapi_ring_get_next_time(TAPI_RING_DATA_t *pRingData,
                                                IFX_boolean_t *bIsBurst,
                                                IFX_boolean_t *bLast)
{
   /* Cadence has a maximum number of 320 bits so we use 16-bit uint. */
   register IFX_uint16_t nBitCount = 0,
                         nBitPos = pRingData->nBitPos,
                         nBitsInCadence,
                         nReloadCount = 0;
   IFX_uint8_t           nPattern,
                         nCurrent,
                         *pBuf;

   /* Set to false until proven otherwise */
   *bLast = IFX_FALSE;

   /* Get the current cadence bit pattern */
   pBuf = pRingData->pCurrentCadence;
   /* nBitsInCadence has a range from 1 to n */
   nBitsInCadence = pRingData->BitsInCurrentCadence;

   /* Get the first byte of the current sequence
      and position on the first bit of the sequence */
   nCurrent = (pBuf[nBitPos >> 3] << (nBitPos & 0x07));
   /* Determine the first bit of the sequence */
   nPattern = nCurrent & 0x80;

   /* loop while the current bit matches the first bit of the sequence */
   /* abort if we reloaded the cadence pattern for the second time - this
      indicates that there was no change in the pattern and also the reloaded
      pattern will not contain a change meaning the sequence is infinite */
   while ( ((nCurrent & 0x80) == nPattern) && (nReloadCount < 2) )
   {
      /* check if we can skip whole bytes in one step */
      if (((nBitPos + 8) < nBitsInCadence) &&
          ((nBitPos & 0x07) == 0x00) &&
          ((nCurrent == 0xFF) || (nCurrent == 0x00)))
      {
         /* skip whole byte if all bits are set / unset */
         nBitPos += 8;
         nBitCount += 8;
      }
      else
      {
         /* otherwise advance bit by bit */
         nBitPos++;
         nBitCount++;
      }

      /* nBitsInCadence has a range from 1 to n while nBitPos counts from 0
         to n-1 so we use a check of greater equal */
      if (nBitPos >= nBitsInCadence)
      {
         /* if we want to support max ring abort with periodic cadences that
            start with pause abort here if the repeat count is zero */

         /* end of this cadence reload the periodic cadence pattern */
         pBuf = pRingData->pCurrentCadence = (IFX_uint8_t *)pRingData->RingCadence.data;
         nBitsInCadence =
            pRingData->BitsInCurrentCadence = pRingData->RingCadence.nr;

         /* return that this was the last time in this sequence */
         *bLast = IFX_TRUE;

         /* reset the bit counter within the cadence */
         nBitPos = 0;

         /* count this cadence pattern reload */
         nReloadCount++;
      }

      /* load the next position in the current cadence */
      nCurrent = (pBuf[nBitPos >> 3] << (nBitPos & 0x07));
   }

   /* Store the bit counter */
   pRingData->nBitPos = nBitPos;

   /* Return whether this is a ring-burst or ring-pause sequence */
   *bIsBurst = nPattern ? IFX_TRUE : IFX_FALSE;

   /* Return the time of this sequence.
      - if infinite sequence is detected 0 is returned.
      - for finite sequence returns time in milliseconds. Each bit is 50ms */
   return (nReloadCount < 2) ?
          (IFX_uint32_t)nBitCount * pRingData->nRingGranularity : 0;
}


/**
   Plays the next sequence of a cadence.

   Sets the linemode according to the next sequence and starts a timer for
   the duration of the sequence.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.

   \return
     - TAPI_statusOk: if successful
     - IFX_ERROR: in case of an error
*/
static IFX_int32_t ifx_tapi_ring_cadence_play(TAPI_CHANNEL *pChannel)
{
   TAPI_RING_DATA_t *pRingData = pChannel->pTapiRingData;
   IFX_boolean_t bBurst, bLast;
   IFX_int32_t ret = TAPI_statusOk;

   if (pRingData->nPendingEvent != TAPI_RING_PENDING_EVENT_NONE)
   {
      if (pRingData->nPendingEvent == TAPI_RING_PENDING_EVENT_CID)
      {
         /* after first ring burst ends CID can be played,
            so inform application that CID can be played out */
         if (IFX_TRUE == pRingData->bSendRingPauseCidTx)
         {
            IFX_TAPI_EVENT_t tapiEvent = {0};
            /* send the event that the ringing phase has ended */
            tapiEvent.id = IFX_TAPI_EVENT_FXS_RINGPAUSE_CIDTX;
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
            /* Event should be sent only once */
            pRingData->bSendRingPauseCidTx = IFX_FALSE;
         }
#ifdef TAPI_FEAT_CID
         IFX_TAPI_CID_OnRingpause(pChannel);
#endif /* TAPI_FEAT_CID */

         /* timer for the duration of the pause */
         TAPI_SetTime_Timer (pRingData->RingTimerID,
                             pRingData->nTimerRemainder, IFX_FALSE, IFX_FALSE);
         pRingData->nTimerRemainder = 0;
      }

      if (pRingData->nPendingEvent == TAPI_RING_PENDING_EVENT_FINAL)
      {
         IFX_TAPI_EVENT_t tapiEvent = {0};

#ifdef TAPI_FEAT_PHONE_DETECTION
         /* restore previous status of the FXS Phone Detection
            state machine */
         ret = IFX_TAPI_PPD_ServiceStop(pChannel);
#endif /* TAPI_FEAT_PHONE_DETECTION */

         /* set flag that ringing is stopped */
         pRingData->bRingingMode = IFX_FALSE;

         TAPI_OS_LockGet (&pRingData->LockRingEvent);
         TAPI_OS_EventWakeUp (&pRingData->TapiRingEvent);
         TAPI_OS_LockRelease (&pRingData->LockRingEvent);

         /* send the event that the ringing phase has ended */
         tapiEvent.id = IFX_TAPI_EVENT_FXS_RINGING_END;
         tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
         IFX_TAPI_Event_Dispatch(pChannel,&tapiEvent);

         /* no further timer */
      }

      pRingData->nPendingEvent = TAPI_RING_PENDING_EVENT_NONE;
      return ret;
   }

   /* The final flag indicates if this call was done after the last sequence
      of the last cadence was played and so final actions need to be performed.
      If not the next sequence is determined the linemode set and the timer
      started for the duration of the sequence. */
   if (pRingData->bFinal == IFX_FALSE)
   {
      /* get the duration of the sequence and if this is a burst or a pause */
      IFX_uint32_t nTime = ifx_tapi_ring_get_next_time(pRingData, &bBurst, &bLast);

      /* shows the values of each step that is done
      TRACE(TAPI_DXS, DBG_LEVEL_LOW,
            ("RINGING: time:%d, burst:%d, last:%d\n", nTime, bBurst, bLast));
      */

      if (bBurst)
      {
         /* ring burst */
         pRingData->bIsRingBurstState = IFX_TRUE;
         ret = TAPI_Phone_Set_Linefeed (pChannel, IFX_TAPI_LINE_FEED_RING_BURST);
      }
      else
      {
         /* ring pause */
         pRingData->bIsRingBurstState = IFX_FALSE;
         ret = TAPI_Phone_Set_Linefeed (pChannel, IFX_TAPI_LINE_FEED_RING_PAUSE);
      }
      if (!TAPI_SUCCESS (ret))
      {
         /* On error set flag that this is the final sequence */
         pRingData->bFinal = IFX_TRUE;
      }

      /* if this is the last sequence of one cadence decrease ringing count */
      if ((bLast == IFX_TRUE) && (pRingData->nRingsLeft > 0))
      {
         /* decrement the number of rings still to play */
         pRingData->nRingsLeft--;
         if (pRingData->nRingsLeft == 0)
         {
            /* Set flag that this is the final sequence */
            pRingData->bFinal = IFX_TRUE;
         }
      }

      /* The FW can stop ringing only at full periods of the ring signal;
         with the 2nd zero crossing of the ring waveform. To ensure that
         the linefeed command for the ring pause gets to the FW before the
         next period starts the timer for a burst duration is shortened.
         To compensate for the shorter time the time of the next cadence
         step (pause) will be made longer by this time.
         Special handling is needed for the events generated by this code
         as they must occur at the nominal times and not occur early. */
      if (bBurst == IFX_TRUE)
      {
         /* ring burst */
         nTime -= TAPI_RING_BURST_EARLY_STOP;
         pRingData->nTimerRemainder = TAPI_RING_BURST_EARLY_STOP;
      }
      else
      {
         /* ring pause */
         if (bLast == IFX_FALSE || pRingData->bTriggerCid == IFX_FALSE)
         {
            nTime += pRingData->nTimerRemainder;
            pRingData->nTimerRemainder = 0;
         }
         else
         {
            /* last pause sequence of the first cadence -> notify CID */
            /* remember the pause duration */
            pRingData->nTimerRemainder = nTime;
            /* delay to compensate for the too early timeout */
            nTime = TAPI_RING_BURST_EARLY_STOP;
            pRingData->nPendingEvent = TAPI_RING_PENDING_EVENT_CID;
            /* do this only once */
            pRingData->bTriggerCid = IFX_FALSE;
         }
      }

      /* if the time is 0 this sequence is infinite so no timer is needed
         otherwise start the timer for the duration of the sequence */
      if (nTime > 0)
      {
         /* start the timer for the duration of this sequence */
         TAPI_SetTime_Timer(pRingData->RingTimerID,
                            nTime, IFX_FALSE, IFX_FALSE);
      }
   }
   else /* pRingData->bFinal == IFX_FALSE */
   {
      /* set linemode back to standby */
      ret = TAPI_Phone_Set_Linefeed(pChannel, IFX_TAPI_LINE_FEED_STANDBY);

      pRingData->nPendingEvent = TAPI_RING_PENDING_EVENT_FINAL;

      /* start the timer to compensate a possible early stop */
      TAPI_SetTime_Timer (pRingData->RingTimerID,
                           pRingData->nTimerRemainder, IFX_FALSE, IFX_FALSE);
      pRingData->nTimerRemainder = 0;
   }

   return ret;
}


/**
   Ring-timer callback function.

   \param   Timer       TimerID of timer that expired.
   \param   nArg        Argument of timer. This argument is a pointer to the
                        TAPI_CHANNEL structure.

   \return  none.
*/
static IFX_void_t ifx_tapi_ring_OnTimer(Timer_ID Timer, IFX_ulong_t nArg)
{
   TAPI_CHANNEL *pChannel  = (TAPI_CHANNEL *) nArg;

   TAPI_UNUSED (Timer);

   /* lock channel */
   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);

   /* Avoid calling the playout function after ringing was stopped. This might
      occur when the timer is not executed immediately by the framework. */
   if (pChannel->pTapiRingData->bRingingMode != IFX_FALSE)
   {
      /* set the line to the next state and start the timer for the duration
         of the cadence */
      (IFX_void_t) ifx_tapi_ring_cadence_play (pChannel);
   }

   /* unlock channel */
   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);
}


/* ============================= */
/* Global function definitions   */
/* ============================= */

/**
   Initialise ringing on the given channel.

   Initialise the data structures and resources needed for the ringing
   state machine.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.

   \return
     TAPI_statusOk - if successful
     TAPI_statusTimerFail - timer creation failed
     TAPI_statusNoMem - not able to allocate memory

   \remarks This function is not protected. The global protection is done
   in the calling function with TAPI_OS_MutexGet (&pChannel->semTapiChDataLock)
*/
IFX_int32_t IFX_TAPI_Ring_Initialise_Unprot (TAPI_CHANNEL *pChannel)
{
   TAPI_RING_DATA_t *pTapiRingData = pChannel->pTapiRingData;
   IFX_int32_t ret = TAPI_statusOk;

   /* check if channel has the required analog module */
   if (pChannel->nChannel >=  pChannel->pTapiDevice->nResource.AlmCount)
   {
      /* no analog module -> nothing to do  --  this is not an error */
      return TAPI_statusOk;
   }

   /* allocate data storage on the channel if not already existing */
   if (pTapiRingData == IFX_NULL)
   {
      /* allocate data storage */
      if ((pTapiRingData = TAPI_OS_Malloc (sizeof(*pTapiRingData))) == IFX_NULL)
      {
         RETURN_STATUS (TAPI_statusNoMem, 0);
      }
      /* Store pointer to data in the channel before we lose it on exit. */
      pChannel->pTapiRingData = pTapiRingData;
      memset (pTapiRingData, 0x00, sizeof(*pTapiRingData));
   }
   /* from here on pTapiRingData and pChannel->pTapiRingData are always valid */

   /* create ring engine timer if not already existing */
   if (pTapiRingData->RingTimerID == 0)
   {
      /* initialize (create) ring timer */
      pTapiRingData->RingTimerID =
         TAPI_Create_Timer((TIMER_ENTRY)ifx_tapi_ring_OnTimer,
                           (IFX_uintptr_t)pChannel);
      if(pTapiRingData->RingTimerID == 0)
      {
         /* free the data storage on the channel */
         TAPI_OS_Free (pChannel->pTapiRingData);
         pChannel->pTapiRingData = IFX_NULL;
         /** errmsg: Timer creation failed */
         RETURN_STATUS (TAPI_statusTimerFail, 0);
      }
   }

   /* create event object that is used  for blocking ringing */
   TAPI_OS_EventInit (&pTapiRingData->TapiRingEvent);
   TAPI_OS_LockInit (&pTapiRingData->LockRingEvent);
   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);

   /* stop ringing if already running */
   ret = IFX_TAPI_Ring_Stop(pChannel);

   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);
   /* set default ringing cadence: 2 sec burst + 2 sec pause */
   memset(&pTapiRingData->RingCadence, 0x00, sizeof(pTapiRingData->RingCadence));
   pTapiRingData->RingCadence.initialNr = 0;  /* no initial cadence */
   pTapiRingData->RingCadence.nr        = 80; /* 10 bytes with 8 bit each */
   memset(pTapiRingData->RingCadence.data, 0xFF, 5);
   /* set no limits on ring repetition */
   pTapiRingData->nMaxRings = 0;
   /* data needed for sending the ring stop early */
   pTapiRingData->nTimerRemainder = 0;
   pTapiRingData->nPendingEvent = TAPI_RING_PENDING_EVENT_NONE;
   pTapiRingData->bTriggerCid = IFX_FALSE;

   /* duration of each bit in the bitfield */
   pTapiRingData->nRingGranularity = TAPI_RING_CADENCE_GRANULARITY;

   return ret;
}

/**
   Cleanup ringing on the given channel.

   Free the resources needed for the ringing state machine.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.
*/
IFX_void_t IFX_TAPI_Ring_Cleanup(TAPI_CHANNEL *pChannel)
{
   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);

   if (pChannel->pTapiRingData != IFX_NULL)
   {
      TAPI_RING_DATA_t *pTapiRingData = pChannel->pTapiRingData;

      /* unconditionally destruct the ring engine timer if existing */
      if (pTapiRingData->RingTimerID != 0)
      {
         TAPI_Delete_Timer (pTapiRingData->RingTimerID);
         pTapiRingData->RingTimerID = 0;
      }
      TAPI_OS_LockDelete (&pTapiRingData->LockRingEvent);

      /* free the data storage on the channel */
      TAPI_OS_Free (pChannel->pTapiRingData);
      pChannel->pTapiRingData = IFX_NULL;
   }

   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);
}


/**
   Starts the ring engine.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.
   \param   bStartWithInitial   Select whether to start initial cadence if
                        possible or if to start with periodic cadence.

   \return
     - TAPI_statusOk: if successful
     - IFX_ERROR: in case of an error
*/
IFX_int32_t IFX_TAPI_Ring_Engine_Start(TAPI_CHANNEL *pChannel,
                                       IFX_boolean_t bStartWithInitial)
{
   IFX_int32_t          ret = TAPI_statusOk;
   TAPI_RING_DATA_t     *pRingData = pChannel->pTapiRingData;

#ifdef TAPI_FEAT_RING_ADAPTIVE_BITTIME
   {
      IFX_TAPI_DRV_CTX_t *pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;

      /* If supported by LL-driver get the period of one ring waveform. */
      if (IFX_TAPI_PtrChk(pDrvCtx->ALM.Ring_Parameter_Get))
      {
         struct IFX_TAPI_RING_PARAM ring_parameters = {0};

         ret = pDrvCtx->ALM.Ring_Parameter_Get(
                                 pChannel->pLLChannel, &ring_parameters);
         if (TAPI_SUCCESS(ret) &&
            (ring_parameters.ring_period >= 10) /* 100 Hz */ &&
            (ring_parameters.ring_period <= 200)  /* 5 Hz */ )
         {
            pRingData->nRingGranularity = ring_parameters.ring_period;
         }
      }
   }
#endif /* TAPI_FEAT_RING_ADAPTIVE_BITTIME */

   /* set the first cadence to be played */
   if ((pRingData->RingCadence.initialNr != 0) &&
       (bStartWithInitial == IFX_TRUE))
   {
      /* first cadence is taken from initial cadence */
      pRingData->pCurrentCadence = (IFX_uint8_t *)pRingData->RingCadence.initial;
      pRingData->BitsInCurrentCadence = pRingData->RingCadence.initialNr;
   }
   else
   {
      /* use periodic cadence as first cadence */
      pRingData->pCurrentCadence = (IFX_uint8_t *)pRingData->RingCadence.data;
      pRingData->BitsInCurrentCadence = pRingData->RingCadence.nr;
   }

   /* start with bit 0 */
   pRingData->nBitPos = 0;
   /* reset flag that indicates the final sequence of the last cadence */
   pRingData->bFinal = IFX_FALSE;
   /* get the maximum number of rings from configuration (0 means forever) */
   pRingData->nRingsLeft = pRingData->nMaxRings;

#ifdef TAPI_FEAT_CID
   pRingData->bTriggerCid = IFX_TRUE;
#endif /* TAPI_FEAT_CID */

   /* set the line to the first state and start the timer for the duration
      of the cadence. */
   ret = ifx_tapi_ring_cadence_play(pChannel);

   if (TAPI_SUCCESS (ret))
   {
      /* set flag that ringing is running */
      pRingData->bRingingMode = IFX_TRUE;
   }

   return ret;
}


/**
   This service sets the ring high resolution cadence for the ringing services.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.
   \param   pCadence    Pointer to struct with cadence definition.

   \return
     - TAPI_statusOk: cadence is set
     - IFX_ERROR: cadence contains errors and is not set

   \remarks
     - The initial cadence may have a zero length while the periodic cadence
       must have at least a length of one bit.
     - The initial as well as the periodic cadence may not consists of all
       0 bits but all bits set to 1 is allowed.
*/
IFX_int32_t  IFX_TAPI_Ring_SetCadenceHighRes(TAPI_CHANNEL *pChannel,
                                             IFX_TAPI_RING_CADENCE_t const *pCadence)
{
   IFX_int32_t  i, j;
   IFX_uint8_t nTestZero;

   /* make sure ringing is initialised on this channel */
   if (pChannel->pTapiRingData == IFX_NULL)
   {
      /* errmsg: Service not supported on called channel context */
      RETURN_STATUS (TAPI_statusInvalidCh, 0);
   }
   /* abort if the line is currently ringing */
   if (pChannel->pTapiRingData->bRingingMode != IFX_FALSE)
   {
      /* cannot set cadence while channel is ringing */
      RETURN_STATUS (TAPI_statusRingAct, 0);
   }
   /* abort if the given counter of initial bits or data bits is out of range */
   if ((pCadence->initialNr < 0) ||
       (pCadence->initialNr > (IFX_TAPI_RING_CADENCE_MAX_BYTES * 8)) ||
       (pCadence->nr < 1) ||
       (pCadence->nr > (IFX_TAPI_RING_CADENCE_MAX_BYTES * 8)))
   {
      /* count of initial or data bits out of range */
      RETURN_STATUS (TAPI_statusRingCad, 0);
   }
   /* abort if the initial count is set but the entire cadence is zero */
   if (pCadence->initialNr != 0)
   {
      for (i = 0, nTestZero = 0; i < (pCadence->initialNr / 8); i++)
      {
         nTestZero |= pCadence->initial[i];
      }
      for (j = 0; j < (pCadence->initialNr % 8); j++)
      {
         nTestZero |= (((IFX_uint8_t)pCadence->initial[i]) << j) & 0x80;
      }
      if (nTestZero == 0)
      {
         /* no bit is set in the initial cadence */
         RETURN_STATUS (TAPI_statusRingCad, 0);
      }
   }
   /* abort if the entire periodic cadence is zero or one */
   for (i = 0, nTestZero = 0; i < (pCadence->nr / 8); i++)
   {
      nTestZero |= pCadence->data[i];
   }
   for (j = 0; j < (pCadence->nr % 8); j++)
   {
      nTestZero |= ((IFX_uint8_t)pCadence->data[i] << j) & 0x80;
   }
   if (nTestZero == 0)
   {
      /* no bit is set in the periodic cadence */
      RETURN_STATUS (TAPI_statusRingCad, 0);
   }

   /* copy cadence data into TAPIstructure */
   /* to round up to the next byte seven is added to the bit counters */
   /* maybe copying the full buffer is better than the calculation below? */
   memcpy(pChannel->pTapiRingData->RingCadence.initial,
          pCadence->initial, (((IFX_size_t)pCadence->initialNr + 7) / 8));
   memcpy(pChannel->pTapiRingData->RingCadence.data,
          pCadence->data, (((IFX_size_t)pCadence->nr + 7) / 8));

   /* length of cadence data for the initial cadence */
   pChannel->pTapiRingData->RingCadence.initialNr = pCadence->initialNr;
   /* length of cadence data for the periodic cadence */
   pChannel->pTapiRingData->RingCadence.nr        = pCadence->nr;

   return TAPI_statusOk;
}


#ifdef TAPI_FEAT_CID
/**
   Calculate the times of the current cadence as needed by CID.

   For CID we need the time from the beginning of the first cadence to
   the end of the last burst in the first cadence and additonally
   the time of the last pause sequence in the first cadence.

   \param  pChannel     Pointer to TAPI_CHANNEL structure.
   \param  pCadenceRingBurst Returns the duration of the cadence up to the
                             beginning of the last pause.
   \param  pCadenceRingPause Returns the duration of the last pause in the
                             cadence.
   \return
   - TAPI_statusOk if successful
   - TAPI_statusRingInit Ringing was not initialized

   \remarks
   It is assumed that this function is called while holding the TapiDataLock!
*/
IFX_int32_t IFX_TAPI_Ring_CalculateRingTiming(TAPI_CHANNEL *pChannel,
                                              IFX_uint32_t *pCadenceRingBurst,
                                              IFX_uint32_t *pCadenceRingPause)
{
   TAPI_RING_DATA_t *pRingData = pChannel->pTapiRingData;
   IFX_boolean_t bBurst, bLast;
   IFX_uint32_t  nTime,
                 nBurstTime = 0;

   /* make sure ringing is initialised on this channel */
   if (pRingData == IFX_NULL)
   {
      /* errmsg: Ringing was not initialized */
      RETURN_STATUS (TAPI_statusRingInit, 0);
   }

   /* find the first cadence to be played */
   if ((pRingData->RingCadence.initialNr != 0))
   {
      /* first cadence is taken from initial cadence */
      pRingData->pCurrentCadence = pRingData->RingCadence.initial;
      pRingData->BitsInCurrentCadence = pRingData->RingCadence.initialNr;
   }
   else
   {
      /* use periodic cadence as first cadence */
      pRingData->pCurrentCadence = pRingData->RingCadence.data;
      pRingData->BitsInCurrentCadence = pRingData->RingCadence.nr;
   }

   /* start with bit 0 */
   pRingData->nBitPos = 0;
   /* Get first sequence. This cannot be the last pause of a cadence. */
   nTime = ifx_tapi_ring_get_next_time(pRingData, &bBurst, &bLast);

   /* find time without last pause and time of last pause */
   /* stop loop when we find the last sequence of a cadence
      (this is also implicitly true for an infinite sequence) */
   do
   {
      nBurstTime += nTime;
      nTime = ifx_tapi_ring_get_next_time(pRingData, &bBurst, &bLast);
   } while (bLast == IFX_FALSE);

   /* restore current cadence */
   if ((pRingData->RingCadence.initialNr != 0))
   {
      /* first cadence is taken from initial cadence */
      pRingData->pCurrentCadence = pRingData->RingCadence.initial;
      pRingData->BitsInCurrentCadence = pRingData->RingCadence.initialNr;
   }

   if (bBurst == IFX_TRUE)
   {
      nBurstTime += nTime;
      nTime = 0;
   }

   /* return the calculation result */
   *pCadenceRingBurst = nBurstTime;
   *pCadenceRingPause = nTime;


   return TAPI_statusOk;
}
#endif /* TAPI_FEAT_CID */


#ifdef TAPI_FEAT_CID
/**
   Prepare CID cadence configuration from ring configuration.

   \param  pChannel     Pointer to TAPI_CHANNEL structure.
   \param  pCidData     CID configuration data to be udated
*/
IFX_void_t IFX_TAPI_Ring_CidCadencePrepare (TAPI_CHANNEL *pChannel,
                                            IFX_TAPI_CID_SEQ_CONF_t *pCidData)
{
   TAPI_RING_DATA_t *pRingData = pChannel->pTapiRingData;

   /* save cadence information for CID */
   pCidData->pCurrentCadence = pRingData->pCurrentCadence;
   pCidData->BitsInCurrentCadence = pRingData->BitsInCurrentCadence;

   pCidData->pPeriodicCadence = pRingData->RingCadence.data;
   pCidData->BitsInPeriodicCadence = pRingData->RingCadence.nr;

   pCidData->nMaxRings = pRingData->nMaxRings;
}
#endif /* TAPI_FEAT_CID */


/**
   Prepare ringing by checking the current state.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.

   \return
     - TAPI_statusOk: prepare ok
     - IFX_ERROR:   prepare failed

   \remarks
     - It is assumed that this function is called with holding the TapiDataLock!
     - Operation is done on a phone channel but updates also CID in a connected
       data channel.
*/
IFX_int32_t IFX_TAPI_Ring_Prepare(TAPI_CHANNEL *pChannel)
{
   TAPI_RING_DATA_t *pRingData = pChannel->pTapiRingData;
   IFX_int32_t ret = TAPI_statusOk;

   /* make sure ringing is initialised on this channel */
   if (pRingData == IFX_NULL)
   {
      /* errmsg: Ringing was not initialized */
      RETURN_STATUS (TAPI_statusRingInit, 0);
   }
   /* do not send ring pause event */
   pRingData->bSendRingPauseCidTx = IFX_FALSE;
   /* check if ring timer is available */
   if (pRingData->RingTimerID == 0)
   {
      /* no ring timer available for this channel */
      RETURN_STATUS (TAPI_statusRingTimer, 0);
   }
   /* check if cadence is set */
   if (!pRingData->RingCadence.nr)
   {
      /* ring cadence is not set */
      RETURN_STATUS (TAPI_statusRingCad, 0);
   }
   /* abort if the line is currently ringing */
   if (pRingData->bRingingMode != IFX_FALSE)
   {
      /* channel is currently ringing */
      RETURN_STATUS (TAPI_statusRingAct, 0);
   }
#ifdef TAPI_FEAT_METERING
   /* check if metering is active */
   if (TAPI_Phone_Meter_IsActive(pChannel))
   {
      /* metering is active */
      RETURN_STATUS (TAPI_statusMeterAct, 0);
   }
#endif /* TAPI_FEAT_METERING */
#ifdef TAPI_FEAT_CID
   {
      TAPI_CHANNEL *pDataCh = IFX_NULL;

      /* Find all data channels to which this phone channel is connected
         and check if CID is already running on one of them. This tests
         for CID sequences that do not use ring as alert sequence. */
      while (IFX_TAPI_Module_Find_Connected_Data_Channel (
                pChannel, IFX_TAPI_MAP_TYPE_PHONE, &pDataCh),
             pDataCh != IFX_NULL)
      {
         if (TAPI_Cid_IsActive(pDataCh))
         {
            /* caller id is active on data ch associated with this phone ch */
            RETURN_STATUS (TAPI_statusCIDActive, 0);
         }
      }
   }
#endif /* TAPI_FEAT_CID */

   return ret;
}


/**
   Starts non-blocking ringing.

   \param pChannel      Pointer to TAPI_CHANNEL structure.

   \return
     - TAPI_statusOk: ring start successful
     - IFX_ERROR:   ring start failed

   \remarks
   To start ringing with caller id use the function \ref TAPI_Phone_CID_Seq_Tx.
*/
IFX_int32_t IFX_TAPI_Ring_Start (TAPI_CHANNEL *pChannel)
{
   IFX_int32_t ret = TAPI_statusOk;

   /* Abort if phone is currently off-hook */
   if (pChannel->TapiOpControlData.bHookState == IFX_TRUE)
   {
      /* Cannot start ringing while phone is off-hook */
      RETURN_STATUS (TAPI_statusPhoneOffHook, 0);
   }

#ifdef TAPI_FEAT_PHONE_DETECTION
   IFX_TAPI_PPD_ServiceStart(pChannel);
#endif /* TAPI_FEAT_PHONE_DETECTION */

   /* Ringing can only be started in certain line modes */
   switch (pChannel->TapiOpControlData.nLineMode)
   {
      case IFX_TAPI_LINE_FEED_ACTIVE:
      case IFX_TAPI_LINE_FEED_NORMAL_LOW:
      case IFX_TAPI_LINE_FEED_NORMAL_AUTO:
      case IFX_TAPI_LINE_FEED_ACTIVE_REV:
      case IFX_TAPI_LINE_FEED_REVERSED_LOW:
      case IFX_TAPI_LINE_FEED_REVERSED_AUTO:
      case IFX_TAPI_LINE_FEED_STANDBY:
      case IFX_TAPI_LINE_FEED_RING_BURST:
      case IFX_TAPI_LINE_FEED_RING_PAUSE:
         /* In all these modes ringing can be started */
         break;
      default:
         /* errmsg: Unsuitable line mode for ringing */
         RETURN_STATUS (TAPI_statusRingLineModeNotOk, 0);
   }

   /* begin of protected area */
   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);

   ret = IFX_TAPI_Ring_Prepare(pChannel);

   if (TAPI_SUCCESS (ret))
   {
      /* if CID not used send ring pause event */
      pChannel->pTapiRingData->bSendRingPauseCidTx = IFX_TRUE;
      /* start the timer */
      ret = IFX_TAPI_Ring_Engine_Start(pChannel, IFX_TRUE);
   }

   /* end of protected area */
   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);

   return ret;
}


/**
   Stops CID data transmition.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.

   \return
     - TAPI_statusOk: ring stop successful
     - IFX_ERROR:   ring stop failed
*/
static IFX_int32_t ifx_tapi_cid_tx_stop(TAPI_CHANNEL *pChannel)
{
   IFX_int32_t ret = TAPI_statusOk;

   /* make sure ringing is initialised on this channel */
   if (pChannel->pTapiRingData == IFX_NULL)
   {
      RETURN_STATUS (TAPI_statusRingInit, 0);
   }
#ifdef TAPI_FEAT_CID
   {
      TAPI_CHANNEL *pDataCh = IFX_NULL;

      /* find all data channels to which this phone channel is connected */
      while (IFX_TAPI_Module_Find_Connected_Data_Channel (
                pChannel, IFX_TAPI_MAP_TYPE_PHONE, &pDataCh),
             pDataCh != IFX_NULL)
      {
         /* in case CID tx is ongoing stop it now this will also prevent the
            start of any periodic ringing after CID has finished */
         if (TAPI_Cid_IsActive(pDataCh))
         {
            ret = TAPI_Phone_CID_Stop_Tx (pDataCh);
         }
      }
   }
#endif /* TAPI_FEAT_CID */

   return ret;
}

/**
   Update line feed mode by hook state.

      for OFFHOOK state use ACTIVE mode
      for ONHOOK state use STANDBY mode

   \param   pChannel    Pointer to TAPI_CHANNEL structure.

   \return
     - TAPI_statusOk
     - IFX_ERROR
*/
static IFX_int32_t ifx_tapi_LineFeedSetByHookState(TAPI_CHANNEL *pChannel)
{
   IFX_int32_t ret;

   if (IFX_TRUE == pChannel->TapiOpControlData.bHookState)
   { /* OFFhook */
      ret = TAPI_Phone_Set_Linefeed (pChannel, IFX_TAPI_LINE_FEED_ACTIVE);
   }
   else
   { /* ONhook */
      /* set linemode back to standby */
      ret = TAPI_Phone_Set_Linefeed(pChannel, IFX_TAPI_LINE_FEED_STANDBY);

#ifdef TAPI_FEAT_PHONE_DETECTION
      /* restore previous status of the FXS Phone Detection
         state machine */
      ret = IFX_TAPI_PPD_ServiceStop(pChannel);
#endif /* TAPI_FEAT_PHONE_DETECTION */
   }

   RETURN_STATUS(ret, 0);
}

/**
   Stops ringing.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.
   \param   bRestoreLineFeed - force restore line feed

   \return
     - TAPI_statusOk: ring stop successful
     - IFX_ERROR:   ring stop failed
*/
static IFX_int32_t ifx_tapi_ring_stop(TAPI_CHANNEL *pChannel, IFX_boolean_t bRestoreLineFeed)
{
   TAPI_RING_DATA_t *pRingData = pChannel->pTapiRingData;

   /* stop only when phone is ringing (even if CID stop above failed) */
   if (pRingData->bRingingMode != IFX_FALSE)
   {
      TAPI_Stop_Timer(pRingData->RingTimerID);
      bRestoreLineFeed = IFX_TRUE;

      /* reset ringing data */
      pRingData->bRingingMode        = IFX_FALSE;
      pRingData->bIsRingBurstState   = IFX_FALSE;
   }

   if (IFX_TRUE == bRestoreLineFeed)
      if (ifx_tapi_LineFeedSetByHookState(pChannel) != TAPI_statusOk)
         RETURN_STATUS(TAPI_statusRingStop, 0);

   return TAPI_statusOk;
}

/**
   Stops non-blocking ringing.

   This interface stops ringing both with or without caller id.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.

   \return
     - TAPI_statusOk: ring stop successful
     - IFX_ERROR:   ring stop failed
*/
IFX_int32_t IFX_TAPI_Ring_Stop (TAPI_CHANNEL *pChannel)
{
   return IFX_TAPI_Ring_Stop_Ext(pChannel, IFX_FALSE /*  Do not force restore line feed */);
}

/**
   Stops non-blocking ringing.

   This interface stops ringing both with or without caller id.

   \param   pChannel - Pointer to TAPI_CHANNEL structure.
   \param   bRestoreLineFeed - force restore line feed

   \return
     - TAPI_statusOk: ring stop successful
     - IFX_ERROR:   ring stop failed
*/
IFX_int32_t IFX_TAPI_Ring_Stop_Ext (TAPI_CHANNEL *pChannel, IFX_boolean_t bRestoreLineFeed)
{
   TAPI_RING_DATA_t *pRingData = pChannel->pTapiRingData;
   IFX_int32_t ret = TAPI_statusOk;

   /* make sure ringing is initialised on this channel */
   if (pRingData == IFX_NULL)
   {
      RETURN_STATUS (TAPI_statusRingInit, 0);
   }

   /* stop any cid transmition if ringing stopped */
   ret = ifx_tapi_cid_tx_stop(pChannel);
   if (!TAPI_SUCCESS (ret))
      return ret;

   /* lock channel */
   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);

   ret = ifx_tapi_ring_stop(pChannel, bRestoreLineFeed);

   /* Wake up blocking ringing
      We know that this stop has not been called by the thread that called
      blocking ringing because it is currently sleeping. So it must have been
      either called by the driver during off-hook or another thread. The
      blocking ringing does not care and will return as if woken by offhook. */
   TAPI_OS_LockGet (&pRingData->LockRingEvent);
   TAPI_OS_EventWakeUp (&pRingData->TapiRingEvent);
   TAPI_OS_LockRelease (&pRingData->LockRingEvent);

   /* unlock channel */
   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);

   return ret;
}

/**
   Tells if ringing is currently active or inactive.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.

   \return
     - \ref IFX_FALSE: Ringing is currently not active
     - \ref IFX_TRUE:  Ringing is currently active
*/
IFX_int32_t IFX_TAPI_Ring_IsActive(TAPI_CHANNEL *pChannel)
{

   if ((pChannel->pTapiRingData != IFX_NULL) &&
       (pChannel->pTapiRingData->bRingingMode != IFX_FALSE))
   {
      return IFX_TRUE;
   }

   return IFX_FALSE;
}


/**
   Set the number of cadences after which ringing stops automatically.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.
   \param   nMaxRings   Contain the number of ring periods to be played.
                        Value 0 means infinite.

   \return TAPI_statusOk or TAPI_statusInvalidCh

   \remarks pArg can be single integer or handle to the
      structure \ref IFX_TAPI_RING_MAX_t.
*/
IFX_int32_t IFX_TAPI_Ring_SetMaxRings(TAPI_CHANNEL *pChannel,
   IFX_uint32_t nMaxRings)
{
   /* make sure ringing is initialised on this channel */
   if (pChannel->pTapiRingData == IFX_NULL)
   {
      /* errmsg: Service not supported on called channel context */
      RETURN_STATUS (TAPI_statusInvalidCh, 0);
   }

   pChannel->pTapiRingData->nMaxRings = nMaxRings;

   return TAPI_statusOk;
}


/**
   Starts the ringing in blocking mode.

   This function starts ringing and then blocks until either the phone went
   off-hook or the number of cadences set with \ref IFX_TAPI_Ring_SetMaxRings
   have been played.

   \param pChannel      Pointer to TAPI_CHANNEL structure.

   \return
     - TAPI_statusOk  Stopped because the configured number of rings were reached.
     - +1           Stopped because phone hooked off.
     - IFX_ERROR    Cannot start because phone is off-hook or channel has no
                    analog resource.
*/
IFX_int32_t IFX_TAPI_Ring_DoBlocking(TAPI_CHANNEL *pChannel)
{
   TAPI_RING_DATA_t *pRingData = pChannel->pTapiRingData;
   IFX_int32_t ret;

   /* make sure ringing is initialised on this channel */
   if (pRingData == IFX_NULL)
   {
      /* For this function return values stay as defined above. */
      return IFX_ERROR;
   }
   /* Abort if phone is currently off-hook */
   if (pChannel->TapiOpControlData.bHookState == IFX_TRUE)
   {
      return IFX_ERROR;
   }

   IFX_TAPI_Ring_Start (pChannel);

   /* clear any previous events that occured */
   TAPI_OS_LockGet (&pRingData->LockRingEvent);
   TAPI_OS_EventDelete (&pRingData->TapiRingEvent);
   TAPI_OS_EventInit (&pRingData->TapiRingEvent);
   TAPI_OS_LockRelease (&pRingData->LockRingEvent);

   /* wait until wakeup -> maxrings have reached
                        -> hook off              */
   TAPI_OS_EventWait (&pRingData->TapiRingEvent,
                      TAPI_OS_WAIT_FOREVER, IFX_NULL);

   /* just a safety because off-hook already uses Ring_Stop() */
   if (pRingData->bRingingMode != IFX_FALSE)
   {
      IFX_TAPI_Ring_Stop(pChannel);
   }

   /* determine whether telephone went off-hook or max-rings where reached */
   if ((pRingData->nMaxRings != 0) &&
       (pRingData->nRingsLeft == 0))
   {
      /* For this function return values stay as defined above. */
      /* maxrings were reached */
      ret = TAPI_statusOk;
   }
   else
   {
      /* hook off */
      ret = 1;
   }

   return ret;
}

#endif /* TAPI_FEAT_RINGENGINE */
