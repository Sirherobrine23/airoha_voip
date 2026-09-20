/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

/**
   \file drv_tapi_dial.c
   Implements the hook state machine for the analog line.
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_tapi.h"

#ifdef TAPI_FEAT_DIAL

#include "drv_tapi_errno.h"

/* ============================= */
/* Configuration                 */
/* ============================= */
/* Scatter the times used for timeout over the given period with the given
   increment between each timer start. This can help to distribute the load
   in systems with multiple devices when hook events happen on all lines in
   parallel. The increment must be smaller than half the maximum time.
   To use this option set TAPI_DIAL_SCATTER_MAX to a positive time in ms and
   TAPI_DIAL_SCATTER_INC to a positive time in ms of maximum half the max time.
   Using this option degrades the precision of the timeouts for the on-hook,
   off-hook,  flash-holdoff and interdigit. The precision of the times for
   flash and pulse detection will stay unaffected. */
#define TAPI_DIAL_SCATTER_MAX  0 /* ms */
#define TAPI_DIAL_SCATTER_INC  0 /* ms */
#if (TAPI_DIAL_SCATTER_MAX / 2) < TAPI_DIAL_SCATTER_INC
#error Configuration of DIAL_SCATTER incorrect
#endif

/* ============================= */
/* Local macros and definitions  */
/* ============================= */
#if (TAPI_DIAL_SCATTER_MAX > 0)
   #define TAPI_DIAL_SCATTER_INCREASE           \
      do {                                      \
         nScatter += TAPI_DIAL_SCATTER_INC;     \
         if (nScatter > TAPI_DIAL_SCATTER_MAX)  \
            nScatter = 0;                       \
      } while (0)
#else
   #define TAPI_DIAL_SCATTER_INCREASE
#endif

/* ============================= */
/* Type declarations             */
/* ============================= */

#ifdef TAPI_FEAT_DIALENGINE
/** states of the hookstate detection finite state machine */
enum HOOK_STATE
{
   /** phone is onhook */
   HOOK_STATE_ONHOOK,
   /** make detected - wait for off-hook timeout */
   HOOK_STATE_VALIDATE_OFFHOOK,
   /** phone is offhook */
   HOOK_STATE_OFFHOOK,
   /** break detected - wait for on-hook timeout */
   HOOK_STATE_VALIDATE_ONHOOK
};

/** states of the pulse dialing detection finite state machine */
enum DIGIT_STATE
{
   /** phone is onhook */
   DIGIT_STATE_ONHOOK,
   /** phone is offhook */
   DIGIT_STATE_OFFHOOK,
   /** wait for make to validate the digit break time */
   DIGIT_STATE_VALIDATE_DIGIT_BREAK,
   /** wait for break to validate the digit make time or interdigit timeout */
   DIGIT_STATE_VALIDATE_DIGIT_MAKE
};

/** states of the flash hook detection finite state machine */
enum FLASH_STATE
{
   /** phone is onhook */
   FLASH_STATE_ONHOOK,
   /** phone is offhook */
   FLASH_STATE_OFFHOOK,
   /** wait for make to validate the flash break time */
   FLASH_STATE_VALIDATE_FLASH_BREAK,
   /** wait for flash holdoff timeout before reporting the flash event */
   FLASH_STATE_VALIDATE_FLASH_HOLDOFF
};

/** actions to all state machines */
enum ACTION
{
   /** make reported from analog line */
   ACTION_MAKE,
   /** break reported from analog line */
   ACTION_BREAK,
   /** timer expired */
   ACTION_TIMEOUT_HOOK,
   /** timer expired */
   ACTION_TIMEOUT_INTERDIGIT,
   /** timer expired */
   ACTION_TIMEOUT_FLASHHOLDOFF,
   /** linefeed was set to disable */
   ACTION_LINEFEED_DISABLE
};
#endif /* TAPI_FEAT_DIALENGINE */

/** Data of the hook state machine. */
struct TAPI_DIAL_DATA
{
#ifdef TAPI_FEAT_DIALENGINE
   /** state of the hookstate detection finite state machine */
   enum HOOK_STATE         nHookState;
   /** state of the pulse dialing detection finite state machine */
   enum DIGIT_STATE        nDigitState;
   /** state of the flash hook detection finite state machine */
   enum FLASH_STATE        nFlashState;
   /** timer for hook state machine timeouts */
   Timer_ID                HookTimerID;
   /** timer for the interdigit timeout */
   Timer_ID                InterdigitTimerID;
   /** timer for the flash holdoff timeout */
   Timer_ID                FlashHoldoffTimerID;
   /** counter for the pulses of the pulse digit */
   IFX_uint8_t             nPulseDigit;
   /* indicates that pulse start event was sent, reset by onhook or flashhook */
   IFX_boolean_t           bPulseStartSent;
#endif /* TAPI_FEAT_DIALENGINE */

   /* validation times for all state machines */
   IFX_uint32_t            nTimeOnhook,
                           nTimeOffhook,
                           nTimeOffhookBackup,
                           nTimeInterdigit,
                           nTimeFlashHoldoff,
                           nTimeDigitMakeMin,
                           nTimeDigitMakeMax,
                           nTimeDigitBreakMin,
                           nTimeDigitBreakMax,
                           nTimeFlashBreakMin,
                           nTimeFlashBreakMax;
};

/* ============================= */
/* Local function declarations   */
/* ============================= */
#ifdef TAPI_FEAT_DIALENGINE
static IFX_void_t ifx_tapi_dial_Action (
                        TAPI_CHANNEL * pChannel,
                        enum ACTION nAction,
                        IFX_uint16_t nTime);

static IFX_void_t ifx_tapi_dial_OnTimer(
                        Timer_ID Timer,
                        IFX_ulong_t nArg);
#endif /* TAPI_FEAT_DIALENGINE */

/* ============================= */
/* Local function definitions    */
/* ============================= */

#ifdef TAPI_FEAT_DIALENGINE
/**
   Process an action in the state machines.

   In this function the hook, digit and flash state machines are all handled
   one after the other. The hook state machine needs to be the first because it
   directly changes the on-hook/off-hook states of the other state machines.
   This reduces the complexity of code of the other state machines.

   \param  pTapiCh      Pointer to TAPI channel structure.
   \param  nAction      Action to be executed.
   \param  nTime        If action is make or break this parameter holds the
                        duration since the last hook event in milliseconds.
                        If the action is timeout this parameter is unused.
*/
static IFX_void_t ifx_tapi_dial_Action (
                        TAPI_CHANNEL * pChannel,
                        enum ACTION nAction,
                        IFX_uint16_t nTime)
{
   TAPI_DIAL_DATA_t *pTapiDialData = pChannel->pTapiDialData;
   IFX_TAPI_EVENT_t tapiEvent;
   IFX_boolean_t bSendPulseEvent;
   static IFX_uint32_t nScatter = 0; /* driver global */

   /* make sure dial is initialised on this channel */
   if (pTapiDialData == IFX_NULL)
   {
      /* silently ignore event - this case can happen during startup or
         shutdown when an interrupt occures but the structures do not exist. */
      return;
   }

#if 0 /* hook state machine tracing */
   TRACE(TAPI_DXS, DBG_LEVEL_LOW,
          ("  ACTION: %d  (hook %d, digit %d, flash %d) time %d\n", nAction,
          pTapiDialData->nHookState,
          pTapiDialData->nDigitState,
          pTapiDialData->nFlashState,
          nTime));
#endif

   /* Handle the hookstate detection finite state machine. */
   switch(pTapiDialData->nHookState)
   {
   case HOOK_STATE_ONHOOK:
      if (nAction == ACTION_MAKE)
      {
         pTapiDialData->nHookState = HOOK_STATE_VALIDATE_OFFHOOK;
         TAPI_SetTime_Timer (pTapiDialData->HookTimerID,
            pTapiDialData->nTimeOffhook + nScatter, IFX_FALSE, IFX_TRUE);
         TAPI_DIAL_SCATTER_INCREASE;
      }
      break;

   case HOOK_STATE_OFFHOOK:
      if (nAction == ACTION_BREAK)
      {
         pTapiDialData->nHookState = HOOK_STATE_VALIDATE_ONHOOK;
         TAPI_SetTime_Timer (pTapiDialData->HookTimerID,
            pTapiDialData->nTimeOnhook + nScatter, IFX_FALSE, IFX_TRUE);
         TAPI_DIAL_SCATTER_INCREASE;
      }
      else
      if (nAction == ACTION_LINEFEED_DISABLE)
      {
         pTapiDialData->nHookState = HOOK_STATE_ONHOOK;
         /* TAPI event: on-hook */
         memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
         tapiEvent.id = IFX_TAPI_EVENT_FXS_ONHOOK;
         tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
         IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
      }
      break;

   case HOOK_STATE_VALIDATE_OFFHOOK:
      if (nAction == ACTION_BREAK)
      {
         pTapiDialData->nHookState = HOOK_STATE_ONHOOK;
         TAPI_Stop_Timer (pTapiDialData->HookTimerID);
         /* The FW changes the linefeeding to active with every off-hook event
            it reports. Here the make period was too short to be reported to
            the application so this is unaware of the linefeed change. So the
            linefeeding is restored here in the driver. */
         TAPI_Phone_Linefeed_Restore(pChannel);
      }
      else
      if (nAction == ACTION_TIMEOUT_HOOK)
      {
         pTapiDialData->nHookState = HOOK_STATE_OFFHOOK;
         /* Attention: change of the digit and flash state machine data! */
         pTapiDialData->nDigitState = DIGIT_STATE_OFFHOOK;
         pTapiDialData->nFlashState = FLASH_STATE_OFFHOOK;
         pTapiDialData->nPulseDigit = 0;
         pTapiDialData->bPulseStartSent = IFX_FALSE;
         /* TAPI event: off-hook */
         memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
         tapiEvent.id = IFX_TAPI_EVENT_FXS_OFFHOOK;
         tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
         IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
      }
      else
      if (nAction == ACTION_LINEFEED_DISABLE)
      {
         pTapiDialData->nHookState = HOOK_STATE_ONHOOK;
         TAPI_Stop_Timer (pTapiDialData->HookTimerID);
         /* TAPI event: on-hook */
         memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
         tapiEvent.id = IFX_TAPI_EVENT_FXS_ONHOOK;
         tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
         IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
      }
      break;

   case HOOK_STATE_VALIDATE_ONHOOK:
      if (nAction == ACTION_MAKE)
      {
         pTapiDialData->nHookState = HOOK_STATE_OFFHOOK;
         TAPI_Stop_Timer (pTapiDialData->HookTimerID);
      }
      else
      if (nAction == ACTION_TIMEOUT_HOOK)
      {
         pTapiDialData->nHookState = HOOK_STATE_ONHOOK;
         /* Attention: change of the digit and flash state machine data! */
         pTapiDialData->nDigitState = DIGIT_STATE_ONHOOK;
         pTapiDialData->nFlashState = FLASH_STATE_ONHOOK;
         /* TAPI event: on-hook */
         memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
         tapiEvent.id = IFX_TAPI_EVENT_FXS_ONHOOK;
         tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
         IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
      }
      else
      if (nAction == ACTION_LINEFEED_DISABLE)
      {
         pTapiDialData->nHookState = HOOK_STATE_ONHOOK;
         TAPI_Stop_Timer (pTapiDialData->HookTimerID);
         /* TAPI event: on-hook */
         memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
         tapiEvent.id = IFX_TAPI_EVENT_FXS_ONHOOK;
         tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
         IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
      }
      break;
   }

   /* Handle the pulse dialing detection finite state machine. */
   /* Attention: the hook state machine above will do the transitions into and
      out of the off-hook state. */
   switch (pTapiDialData->nDigitState)
   {
   case DIGIT_STATE_ONHOOK:
      /* ignore all */
      break;

   case DIGIT_STATE_OFFHOOK:
      if (nAction == ACTION_BREAK)
         pTapiDialData->nDigitState = DIGIT_STATE_VALIDATE_DIGIT_BREAK;
      else
      if (nAction == ACTION_LINEFEED_DISABLE)
         pTapiDialData->nDigitState = DIGIT_STATE_ONHOOK;
      break;

   case DIGIT_STATE_VALIDATE_DIGIT_BREAK:
      if (nAction == ACTION_MAKE)
      {
         if ((nTime >= pTapiDialData->nTimeDigitBreakMin) &&
             (nTime <= pTapiDialData->nTimeDigitBreakMax))
         {
            pTapiDialData->nDigitState = DIGIT_STATE_VALIDATE_DIGIT_MAKE;
            pTapiDialData->nPulseDigit += 1;
            TAPI_SetTime_Timer (pTapiDialData->InterdigitTimerID,
               pTapiDialData->nTimeInterdigit + nScatter, IFX_FALSE, IFX_TRUE);
            TAPI_DIAL_SCATTER_INCREASE;
            if ((pTapiDialData->nPulseDigit == 1) &&
                (pTapiDialData->bPulseStartSent == IFX_FALSE))
            {
               /* report the first validated pulse dial break */
               /* Use this to stop a dialtone right after the first pulse. */
               memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
               tapiEvent.id = IFX_TAPI_EVENT_PULSE_START;
               tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
               IFX_TAPI_Event_Dispatch(pChannel,&tapiEvent);
               /* Do this only for the first pulse after offhook or flash. */
               pTapiDialData->bPulseStartSent = IFX_TRUE;
            }
         }
         else
         {
            pTapiDialData->nDigitState = DIGIT_STATE_OFFHOOK;
            pTapiDialData->nPulseDigit = 0;
         }
      }
      else
      if (nAction == ACTION_LINEFEED_DISABLE)
      {
         pTapiDialData->nDigitState = DIGIT_STATE_ONHOOK;
      }
      break;

   case DIGIT_STATE_VALIDATE_DIGIT_MAKE:
      bSendPulseEvent = IFX_FALSE;
      if (nAction == ACTION_BREAK)
      {
         pTapiDialData->nDigitState = DIGIT_STATE_VALIDATE_DIGIT_BREAK;
         TAPI_Stop_Timer (pTapiDialData->InterdigitTimerID);
         if (nTime >= pTapiDialData->nTimeInterdigit)
         {
            /* Accept also the time from the hook event for the interdigt
               timeout condition. */
            bSendPulseEvent = IFX_TRUE;
         }
         else
         if ((nTime < pTapiDialData->nTimeDigitMakeMin) ||
             (nTime > pTapiDialData->nTimeDigitMakeMax))
         {
            pTapiDialData->nPulseDigit = 0;
         }
      }
      else
      if (nAction == ACTION_TIMEOUT_INTERDIGIT)
      {
         pTapiDialData->nDigitState = DIGIT_STATE_OFFHOOK;
         bSendPulseEvent = IFX_TRUE;
      }
      /* _no_ else here */
      if (bSendPulseEvent)
      {
         if (pTapiDialData->nPulseDigit <= 10)
         {
            /* TAPI event: digit */
            memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
            tapiEvent.id = IFX_TAPI_EVENT_PULSE_DIGIT;
            tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
            tapiEvent.data.pulse.digit = pTapiDialData->nPulseDigit;
            /* 10 pulses represent digit 0 which has TAPI code 0xB */
            if (tapiEvent.data.pulse.digit == 10)
               tapiEvent.data.pulse.digit = 0x0B;
            IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
         }
         pTapiDialData->nPulseDigit = 0;
      }
      if (nAction == ACTION_LINEFEED_DISABLE)
      {
         pTapiDialData->nDigitState = DIGIT_STATE_ONHOOK;
         TAPI_Stop_Timer (pTapiDialData->InterdigitTimerID);
      }
      break;
   }

   /* Handle the flash hook detection finite state machine. */
   /* Attention: the hook state machine above will do the transitions into and
      out of the off-hook state. */
   switch (pTapiDialData->nFlashState)
   {
   case FLASH_STATE_ONHOOK:
      /* ignore all */
      break;

   case FLASH_STATE_OFFHOOK:
      if (nAction == ACTION_BREAK)
         pTapiDialData->nFlashState = FLASH_STATE_VALIDATE_FLASH_BREAK;
      else
      if (nAction == ACTION_LINEFEED_DISABLE)
         pTapiDialData->nDigitState = DIGIT_STATE_ONHOOK;
      break;

   case FLASH_STATE_VALIDATE_FLASH_BREAK:
      if (nAction == ACTION_MAKE)
      {
         if ((nTime >= pTapiDialData->nTimeFlashBreakMin) &&
             (nTime <= pTapiDialData->nTimeFlashBreakMax))
         {
            pTapiDialData->nFlashState = FLASH_STATE_VALIDATE_FLASH_HOLDOFF;
            TAPI_SetTime_Timer (pTapiDialData->FlashHoldoffTimerID,
               pTapiDialData->nTimeFlashHoldoff + nScatter,
               IFX_FALSE, IFX_TRUE);
            TAPI_DIAL_SCATTER_INCREASE;
         }
         else
         {
            pTapiDialData->nFlashState = FLASH_STATE_OFFHOOK;
         }
      }
      else
      if (nAction == ACTION_LINEFEED_DISABLE)
         pTapiDialData->nDigitState = DIGIT_STATE_ONHOOK;
      break;

   case FLASH_STATE_VALIDATE_FLASH_HOLDOFF:
      if (nAction == ACTION_BREAK)
      {
         pTapiDialData->nFlashState = FLASH_STATE_VALIDATE_FLASH_BREAK;
         TAPI_Stop_Timer (pTapiDialData->FlashHoldoffTimerID);
      }
      else
      if (nAction == ACTION_TIMEOUT_FLASHHOLDOFF)
      {
         pTapiDialData->nFlashState = FLASH_STATE_OFFHOOK;
         pTapiDialData->bPulseStartSent = IFX_FALSE;
         /* TAPI event: flash */
         memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
         tapiEvent.id = IFX_TAPI_EVENT_FXS_FLASH;
         tapiEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
         IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
      }
      else
      if (nAction == ACTION_LINEFEED_DISABLE)
      {
         pTapiDialData->nDigitState = DIGIT_STATE_ONHOOK;
         TAPI_Stop_Timer (pTapiDialData->FlashHoldoffTimerID);
      }
      break;
   }

#if 0 /* hook state machine tracing */
   TRACE(TAPI_DXS, DBG_LEVEL_LOW,
          ("             (hook %d, digit %d, flash %d) digit %d\n",
           pTapiDialData->nHookState,
           pTapiDialData->nDigitState,
           pTapiDialData->nFlashState,
           pTapiDialData->nPulseDigit ));
#endif
}
#endif /* TAPI_FEAT_DIALENGINE */


#ifdef TAPI_FEAT_DIALENGINE
/**
   Hook state machine timer callback function.

   \param  Timer        TimerID of timer that expires.
   \param  nArg         Argument of timer. This argument is a pointer to the
                        TAPI_CHANNEL structure.
*/
static IFX_void_t ifx_tapi_dial_OnTimer(
                        Timer_ID Timer,
                        IFX_ulong_t nArg)
{
   TAPI_CHANNEL *pChannel = (TAPI_CHANNEL *)nArg;
   const TAPI_DIAL_DATA_t *pTapiDialData = pChannel->pTapiDialData;

   if (Timer == pTapiDialData->HookTimerID)
      ifx_tapi_dial_Action (pChannel, ACTION_TIMEOUT_HOOK, /*dummy*/0);

   if (Timer == pTapiDialData->InterdigitTimerID)
      ifx_tapi_dial_Action (pChannel, ACTION_TIMEOUT_INTERDIGIT, /*dummy*/0);

   if (Timer == pTapiDialData->FlashHoldoffTimerID)
      ifx_tapi_dial_Action (pChannel, ACTION_TIMEOUT_FLASHHOLDOFF, /*dummy*/0);

   return;
}
#endif /* TAPI_FEAT_DIALENGINE */


/* ============================= */
/* Global function definitions   */
/* ============================= */

/**
   Initialise the analog line state machines on the given channel.

   Initialise the data structures and resources needed for the
   hook state machine.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.

   \return
     TAPI_statusOk - if successful
     TAPI_statusTimerFail - timer creation failed
     TAPI_statusNoMem - not able to allocate memory

   \remarks This function is not protected. The global protection is done
   in the calling function with TAPI_OS_MutexGet (&pChannel->semTapiChDataLock)
*/
IFX_int32_t IFX_TAPI_Dial_Initialise_Unprot (TAPI_CHANNEL *pChannel)
{
   TAPI_DIAL_DATA_t  *pTapiDialData  = pChannel->pTapiDialData;

   /* check if channel has the required analog module */
   if (pChannel->nChannel >= pChannel->pTapiDevice->nResource.AlmCount)
   {
      /* no analog or audio module -> nothing to do  --  this is not an error */
      return TAPI_statusOk;
   }

   /* allocate data storage for the hook state machine on the channel
      if not already existing */
   if (pTapiDialData == IFX_NULL)
   {
      /* allocate data storage */
      if ((pTapiDialData = TAPI_OS_Malloc (sizeof(*pTapiDialData))) == IFX_NULL)
      {
         RETURN_STATUS (TAPI_statusNoMem, 0);
      }
      /* Store pointer to data in the channel or we lose it on exit. */
      pChannel->pTapiDialData = pTapiDialData;
      memset (pTapiDialData, 0x00, sizeof(*pTapiDialData));
   }
   /* from here on pTapiDialData and pChannel->pTapiDialData are valid */

   /* set default values for the validation times */
   pTapiDialData->nTimeOnhook          = TAPI_MIN_ON_HOOK;
   pTapiDialData->nTimeOffhook         = TAPI_MIN_OFF_HOOK;
   pTapiDialData->nTimeInterdigit      = TAPI_MIN_INTERDIGIT;
   pTapiDialData->nTimeFlashHoldoff    = TAPI_MIN_FLASH_MAKE;
   pTapiDialData->nTimeDigitMakeMin    = TAPI_MIN_DIGIT_HIGH;
   pTapiDialData->nTimeDigitMakeMax    = TAPI_MAX_DIGIT_HIGH;
   pTapiDialData->nTimeDigitBreakMin   = TAPI_MIN_DIGIT_LOW;
   pTapiDialData->nTimeDigitBreakMax   = TAPI_MAX_DIGIT_LOW;
   pTapiDialData->nTimeFlashBreakMin   = TAPI_MIN_FLASH;
   pTapiDialData->nTimeFlashBreakMax   = TAPI_MAX_FLASH;

#ifdef TAPI_FEAT_DIALENGINE
   /* start all three FSMs in onhook state */
   pTapiDialData->nHookState  = HOOK_STATE_ONHOOK;
   pTapiDialData->nDigitState = DIGIT_STATE_ONHOOK;
   pTapiDialData->nFlashState = FLASH_STATE_ONHOOK;
   pTapiDialData->nPulseDigit = 0;
   pTapiDialData->bPulseStartSent = IFX_FALSE;

   /* create hook-state-machine validation timer if not already existing */
   if (pTapiDialData->HookTimerID == 0)
   {
      pTapiDialData->HookTimerID =
         TAPI_Create_Timer((TIMER_ENTRY)ifx_tapi_dial_OnTimer,
                           (IFX_uintptr_t)pChannel);
   }
   /* create interdigit timer if not already existing */
   if (pTapiDialData->InterdigitTimerID == 0)
   {
      pTapiDialData->InterdigitTimerID =
         TAPI_Create_Timer((TIMER_ENTRY)ifx_tapi_dial_OnTimer,
                           (IFX_uintptr_t)pChannel);
   }
   /* create flash holdoff timer if not already existing */
   if (pTapiDialData->FlashHoldoffTimerID == 0)
   {
      pTapiDialData->FlashHoldoffTimerID =
         TAPI_Create_Timer((TIMER_ENTRY)ifx_tapi_dial_OnTimer,
                           (IFX_uintptr_t)pChannel);
   }
   if ((pTapiDialData->HookTimerID == 0) ||
         (pTapiDialData->InterdigitTimerID == 0) ||
         (pTapiDialData->FlashHoldoffTimerID == 0) )
   {
      IFX_TAPI_Dial_Cleanup(pChannel);
      RETURN_STATUS (TAPI_statusTimerFail, 0);
   }

#endif /* TAPI_FEAT_DIALENGINE */

   return IFX_SUCCESS;
}


/**
   Cleanup the analog line state machines on the given channel.

   Free the resources needed for the hook state machine.
   Free the resources needed for the hook state machine and
   the ground-key state machine.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.
*/
IFX_void_t IFX_TAPI_Dial_Cleanup(TAPI_CHANNEL *pChannel)
{
   TAPI_DIAL_DATA_t  *pTapiDialData  = pChannel->pTapiDialData;

   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);

   if (pTapiDialData != IFX_NULL)
   {
#ifdef TAPI_FEAT_DIALENGINE
      /* unconditionally destruct all timers if they exist */
      if (pTapiDialData->HookTimerID != 0)
      {
         TAPI_Delete_Timer (pTapiDialData->HookTimerID);
         pTapiDialData->HookTimerID = 0;
      }
      if (pTapiDialData->InterdigitTimerID != 0)
      {
         TAPI_Delete_Timer (pTapiDialData->InterdigitTimerID);
         pTapiDialData->InterdigitTimerID = 0;
      }
      if (pTapiDialData->FlashHoldoffTimerID != 0)
      {
         TAPI_Delete_Timer (pTapiDialData->FlashHoldoffTimerID);
         pTapiDialData->FlashHoldoffTimerID = 0;
      }
#endif /* TAPI_FEAT_DIALENGINE */

      /* free the data storage on the channel */
      TAPI_OS_Free (pTapiDialData);
      pChannel->pTapiDialData = IFX_NULL;
   }

   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);
}


#ifdef TAPI_FEAT_DIALENGINE
/**
   Notify the hook state machine that linefeeding was set to disabled.

   \param  pChannel     Pointer to TAPI channel structure.
*/
IFX_void_t IFX_TAPI_Dial_LineDisable (
                        TAPI_CHANNEL * pChannel)
{
   /* make sure dial is initialised on this channel */
   if (pChannel->pTapiDialData == IFX_NULL)
   {
      return;
   }

   if (pChannel->pTapiDialData->nHookState != HOOK_STATE_ONHOOK)
   {
      ifx_tapi_dial_Action(pChannel, ACTION_LINEFEED_DISABLE, /* dummy */0);
   }

   return;
}
#endif /* TAPI_FEAT_DIALENGINE */


#ifdef TAPI_FEAT_DIALENGINE
/**
   Process hook events in the state machines.

   \param  pChannel     Pointer to TAPI channel structure.
   \param  bHookState   - IFX_TRUE:  make  (off-hook),
                        - IFX_FALSE: break (on-hook)
   \param  nTime        Duration since the last hook event in units of 1/8 ms.
*/
IFX_void_t IFX_TAPI_Dial_HookEvent (
                        TAPI_CHANNEL *pChannel,
                        IFX_boolean_t bHookState,
                        IFX_uint16_t nTime)
{
   /* make sure dial is initialised on this channel */
   if (pChannel->pTapiDialData == IFX_NULL)
   {
      /* silently ignore event - this case can happen during startup or
         shutdown when an interrupt occures but the structures do not exist. */
      return;
   }

   /* Note: Currently the state machine uses internally a granularity of
      one millisecond while the external interface was defined to be
      eight times more precise. This was just a precaution and done in case
      it might be needed in the future. While we do not need it the precision
      is reduced here to the millisecond granularity that is currently used
      inside the state machine */
   /* Note: The hook events actually report a change between the make and break
      status of the line. The action reports the new line status. This implies
      that before this event the opposite line status was present and that the
      time that is reported is the duration that this status was present.
      Here make and break is used to make it clear that this is not the hook
      status of the phone but the line state which will also change from
      pulse dialing and flash hook. */
   ifx_tapi_dial_Action(pChannel,
                        bHookState ? ACTION_MAKE : ACTION_BREAK, nTime / 8);

   return;
}
#endif /* TAPI_FEAT_DIALENGINE */


#ifdef TAPI_FEAT_DIALENGINE
/**
   Set the given hook state unconditionally.

   This is used after certain CID sequences which use hook for signalling
   purposes. There the hook is not given to this state machine and so the
   state needs to be corrected in case of an abort.

   \param  pChannel     Pointer to TAPI channel structure.
   \param  bHookState   - IFX_TRUE:  make  (off-hook),
                        - IFX_FALSE: break (on-hook)
*/
IFX_void_t IFX_TAPI_Dial_HookSet (
                        TAPI_CHANNEL *pChannel,
                        IFX_boolean_t bHookState)
{
   TAPI_DIAL_DATA_t *pTapiDialData = pChannel->pTapiDialData;

   /* make sure dial is initialised on this channel */
   if (pChannel->pTapiDialData == IFX_NULL)
   {
      /* silently ignore */
      return;
   }

   /* Handle the hookstate detection finite state machine. */
   switch(pTapiDialData->nHookState)
   {
   case HOOK_STATE_OFFHOOK:
      if (bHookState == IFX_FALSE)
      {
         pTapiDialData->nHookState = HOOK_STATE_ONHOOK;
         pTapiDialData->nDigitState = DIGIT_STATE_ONHOOK;
         pTapiDialData->nFlashState = FLASH_STATE_ONHOOK;
         /* Do not send a TAPI event here. */
      }
      break;

   case HOOK_STATE_ONHOOK:
      if (bHookState == IFX_TRUE)
      {
         pTapiDialData->nHookState = HOOK_STATE_OFFHOOK;
         pTapiDialData->nDigitState = DIGIT_STATE_OFFHOOK;
         pTapiDialData->nFlashState = FLASH_STATE_OFFHOOK;
         pTapiDialData->nPulseDigit = 0;
         pTapiDialData->bPulseStartSent = IFX_FALSE;
         /* Do not send a TAPI event here. */
      }
      break;

   default:
      /* Do nothing in the validation states. */
      break;
   }

   return;
}
#endif /* TAPI_FEAT_DIALENGINE */


/**
   Sets the validation times for hook, pulse digit and flashhook.

   \param  pChannel     Pointer to TAPI channel structure.
   \param  pTime        Type of validation setting, min and max time in
                        milliseconds.

   \return
   - TAPI_statusOk: In case success.
   - TAPI_statusDialMaxMinParam: Max must be larger than min validation time.
   - TAPI_statusDialZeroParam: Value zero not allowed as validation time.
   - TAPI_statusDialTypeParam: Unknown hook state validation type parameter.
*/
IFX_int32_t IFX_TAPI_Dial_SetValidationTime(
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_LINE_HOOK_VT_t const *pTime)
{
   TAPI_DIAL_DATA_t *pTapiDialData = pChannel->pTapiDialData;
   IFX_int32_t ret = TAPI_statusOk;

   /* check if dial is initialised on this channel */
   if (pTapiDialData == IFX_NULL)
   {
      /* errmsg: Service not supported on called channel context */
      RETURN_STATUS (TAPI_statusInvalidCh, 0);
   }

   /* zero timer value is not allowed -- except for the flash holdoff time */
   if ((pTime->nMinTime == 0) &&
       (pTime->nType != IFX_TAPI_LINE_HOOK_VT_FLASH_HOLDOFF_TIME))
   {
      /* errmsg: Value zero not allowed as hook state validation time */
      RETURN_STATUS (TAPI_statusDialZeroParam, 0);
   }

   /* For the times hook-off, hook-on, interdigit and flash holdoff just
      the minimum time is relevant. So do not check the maximum time. */
   if ((pTime->nType != IFX_TAPI_LINE_HOOK_VT_HOOKOFF_TIME) &&
       (pTime->nType != IFX_TAPI_LINE_HOOK_VT_HOOKON_TIME) &&
       (pTime->nType != IFX_TAPI_LINE_HOOK_VT_INTERDIGIT_TIME) &&
       (pTime->nType != IFX_TAPI_LINE_HOOK_VT_FLASH_HOLDOFF_TIME))
   {
      /* check whether min timer value > max timer value */
      if (pTime->nMinTime > pTime->nMaxTime)
      {
         /* errmsg: Max must be larger than min hook state validation time */
         RETURN_STATUS (TAPI_statusDialMaxMinParam, 0);
      }
      /* Note: the max timer value cannot be 0 because min > 0 and
         min <= max entails that max > 0. */
   }

   /* set the validation times */
   switch (pTime->nType)
   {
      case IFX_TAPI_LINE_HOOK_VT_HOOKOFF_TIME:
         if (pTapiDialData->nTimeOffhookBackup == 0)
            pTapiDialData->nTimeOffhook      = pTime->nMinTime;
         else
            pTapiDialData->nTimeOffhookBackup= pTime->nMinTime;
         break;

      case IFX_TAPI_LINE_HOOK_VT_HOOKON_TIME:
         pTapiDialData->nTimeOnhook          = pTime->nMinTime;
         break;

      case IFX_TAPI_LINE_HOOK_VT_HOOKFLASH_TIME:
         pTapiDialData->nTimeFlashBreakMin   = pTime->nMinTime;
         pTapiDialData->nTimeFlashBreakMax   = pTime->nMaxTime;
         break;

      case IFX_TAPI_LINE_HOOK_VT_DIGITLOW_TIME:
         pTapiDialData->nTimeDigitBreakMin   = pTime->nMinTime;
         pTapiDialData->nTimeDigitBreakMax   = pTime->nMaxTime;
         break;

      case IFX_TAPI_LINE_HOOK_VT_DIGITHIGH_TIME:
         pTapiDialData->nTimeDigitMakeMin    = pTime->nMinTime;
         pTapiDialData->nTimeDigitMakeMax    = pTime->nMaxTime;
         break;

      case IFX_TAPI_LINE_HOOK_VT_INTERDIGIT_TIME:
         pTapiDialData->nTimeInterdigit      = pTime->nMinTime;
         break;

      case IFX_TAPI_LINE_HOOK_VT_FLASH_HOLDOFF_TIME:
         pTapiDialData->nTimeFlashHoldoff    = pTime->nMinTime;
         break;

      default:
         /* errmsg: Unknown hook state validation type parameter */
         RETURN_STATUS (TAPI_statusDialTypeParam, 0);
   }

   return ret;
}


/**
   Overrides the user configured off-hook time.

   This is used by the phone detection to set an off-hook time to suppress
   transient hook events that can occur in the off-hook detection state.

   \param  pChannel     Pointer to TAPI channel structure.
   \param  nTime        IFX_TAPI_DIAL_TIME_NORMAL for normal user configured
                        value. All other values are used as off-hook time
                        in milliseconds.
   \remarks
   The value 0 is invalid for the off-hook time. Here it is used as a marker.
*/
IFX_void_t IFX_TAPI_Dial_OffhookTime_Override(
                        TAPI_CHANNEL *pChannel,
                        IFX_uint32_t const nTime)
{
   TAPI_DIAL_DATA_t *pTapiDialData = pChannel->pTapiDialData;

   /* make sure that dial is initialised on this channel */
   if (pTapiDialData != IFX_NULL)
   {
      if (nTime == IFX_TAPI_DIAL_TIME_NORMAL)
      {
         if (pTapiDialData->nTimeOffhookBackup != 0)
         {
            /* Restore the backed up time value. */
            pTapiDialData->nTimeOffhook = pTapiDialData->nTimeOffhookBackup;
            pTapiDialData->nTimeOffhookBackup = 0;
         }
      }
      else
      {
         /* Switch to override - backup time value and set from parameter. */
         if (pTapiDialData->nTimeOffhookBackup == 0)
         {
            pTapiDialData->nTimeOffhookBackup = pTapiDialData->nTimeOffhook;
         }
         pTapiDialData->nTimeOffhook = nTime;
      }
   }
}

#endif /* TAPI_FEAT_DIAL */
