/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.
  Copyright 2022 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_irq.c
   This file contains the implementation of the interrupt handler.

   \remarks
   The implementation assumes that multiple instances of this interrupt handler
   cannot preempt each other, i.e. if there is more than one DUSLIC XS in your
   design all DUSLIC XS are expected to raise interrupts at the same priority
   level.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_access.h"
#include "drv_dxs_init.h"
#include "drv_dxs_cid.h"
#include "drv_dxs_dtmf.h"

#include "drv_dxs_irq.h"
#include "drv_dxs_outbox.h"

#include <drv_dxs_errno.h>

#include <drv_tapi_osmap.h>

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/* The define should be set in drv_config_user.h
   If it was not set there set a default of 10 ms here. */
#ifndef DXS_POLL_CYCLE_MS
   #define DXS_POLL_CYCLE_MS 10 /*ms*/
#endif

/** 
 * Macro added for code clarity - used in line event irq handling for checking
 * hook events when pCh->nDiscardHookEvent value differs from default. Second
 * hook event for given channel is omitted. Event discarding procedure is
 * started by setting DXS_HOOK_EV_TST_CHNG to pCh->nDiscardHookEvent in
 * DXS_TAPI_LL_ALM_TestHookGen function.
 * \param pEvent pointer to line event structure
 * \param pCh pointer to particular DXS channel structure
 * \param nDev device absolute number
 * \param bDiscardEvent flag set when event has to be discarded
 */
#define DXS_HOOK_EVENT_TEST(pEvent, pCh, nDev, bDiscardEvent)                  \
   do                                                                          \
   {                                                                           \
      if ((pEvent->ONH) || (pEvent->OFFH))                                     \
      {                                                                        \
         if( DXS_HOOK_EV_TST_DFLT != pCh->nDiscardHookEvent )                  \
         {                                                                     \
            if( DXS_HOOK_EV_TST_CHNG == pCh->nDiscardHookEvent )               \
            {                                                                  \
               pCh->nDiscardHookEvent = DXS_HOOK_EV_TST_DISC;                  \
            }                                                                  \
            else if ( DXS_HOOK_EV_TST_DISC == pCh->nDiscardHookEvent )         \
            {                                                                  \
               bDiscardEvent = IFX_TRUE;                                       \
               pCh->nDiscardHookEvent = DXS_HOOK_EV_TST_DFLT;                  \
               TRACE(TAPI_DXS, DBG_LEVEL_LOW,                                       \
                     ("Discarding DXS event hook %s, dev:%d, ch:%d.\n",        \
                      (pEvent->ONH)? "make" : "break",                         \
                      nDev, pEvent->CHAN));                                    \
            }                                                                  \
            else                                                               \
            {                                                                  \
               TRACE(TAPI_DXS, DBG_LEVEL_LOW,                                       \
                   ("WARNING: Wrong nDiscardHookEvent value %d (DXS_CHANNEL)," \
                    "dev:%d, ch:%d.\n",                                        \
                    pCh->nDiscardHookEvent, nDev, pEvent->CHAN));              \
            }                                                                  \
         }                                                                     \
      }                                                                        \
   } while(0)

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

#ifndef DXS_FEAT_LINUX_THREADED_IRQ
   /** mutex semaphore blocking the real time interrupt handler thread */
   static TAPI_OS_MutexForInterrupts_t s_mtxInterruptHandler;

   /** interrupt thread handling structure */
   static TAPI_OS_ThreadCtrl_t s_intThread;
   #ifdef VXWORKS
      static TAPI_OS_ThreadCtrl_t s_pollTask;
      IFX_void_t PollTask(IFX_void_t);
   #endif
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */

/** polling timer */
static Timer_ID s_pollTimerId;
static IFX_boolean_t bPollTimerStarted;

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

enum IFX_irqreturn_t DXS_interrupt_handler(DXS_DEVICE_t *pDev);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

/**
   DUSLIC XS device interrupt service handler.

   \param pDev   reference to device context

   \return
   - IFX_IRQ_NONE if interrupt was not handled
   - IFX_IRQ_HANDLED if interrupt was handled
*/
enum IFX_irqreturn_t DXS_interrupt_handler(DXS_DEVICE_t *pDev)
{
   enum IFX_irqreturn_t iret      = IFX_IRQ_NONE;
   IFX_int32_t       ret            = DXS_statusOk,
                     ret2           = DXS_statusOk;
   IFX_uint16_t      nRegHostIrq[2];
   IFX_TAPI_EVENT_t  tapiEvent;

   if(pDev == IFX_NULL)
   {
      return iret;
   }

#ifdef SPI_EXIT
   DXS_SPI_PROTECT(pDev);
#endif
   /* No event handling when SPI access has not been verified yet or
      the event handling flag indicates STOP. */
   if (!(pDev->nDevState & DS_SPI_ACTIVE) ||
       (pDev->nEventHandlingMode == DXS_EVENT_STOP))
   {
#ifdef SPI_EXIT
      DXS_SPI_RELEASE(pDev);
#endif
      return iret;
   }

#ifndef DXS_SPI_8BIT_ACCESS
   /* read both interrupt registers (HOST_INT1, HOST_INT2) */
   ret = DXS_RegReadMulti(pDev, DXS_HOST_INT1, nRegHostIrq, 2);
#else
   ret = DXS_RegRead(pDev, DXS_HOST_INT1, &nRegHostIrq[0]);
#endif

   /* if err bit is set in "interrupt register 1" read
      the "interrupt register 2" (Host_INT2) for details */
   if ((ret == DXS_statusOk) && (nRegHostIrq[0] & DXS_REG_IEN1_ERR))
   {
#ifdef DXS_SPI_8BIT_ACCESS
      ret = DXS_RegRead(pDev, DXS_HOST_INT2, &nRegHostIrq[1]);
#endif
      /* Out-Box Underflow */
      if (nRegHostIrq[1] & DXS_REG_IEN2_OBX_UFL)
      {
         memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_FW_CBO_UF;
         DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_NONE);
         IFX_TAPI_Event_Dispatch(pDev->pChannel[0].pTapiCh,&tapiEvent);
      }

      /* In-Box Overflow */
      if (nRegHostIrq[1] & DXS_REG_IEN2_IBX_OFL)
      {
         memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
         tapiEvent.id = IFX_TAPI_EVENT_FAULT_FW_CBI_OF;
         DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_NONE);
         IFX_TAPI_Event_Dispatch(pDev->pChannel[0].pTapiCh,&tapiEvent);
      }

      /* Clear the irq by writing to the irq register.
         Note that the self clearing mode has no effect on this register. */
      ret2 = DXS_RegWrite(pDev, DXS_HOST_INT2, nRegHostIrq[1]);
   } /* DXS_REG_IEN1_ERR set */

   /* handle the interrupts from "interrupt register 1" */
   if ((ret == DXS_statusOk) && (nRegHostIrq[0] & DXS_REG_IEN1_OBX_RDY))
   {
      ret = DXS_outbox_handler(pDev);
      iret = IFX_IRQ_HANDLED;
   }
#ifdef SPI_EXIT
   DXS_SPI_RELEASE(pDev);
#endif

   if ((ret != DXS_statusOk) || (ret2 != DXS_statusOk))
   {
      /* throw tapi-event informing about register access error */
      memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
      tapiEvent.id = IFX_TAPI_EVENT_FAULT_HW_SPI_ACCESS;
      DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_NONE);

      IFX_TAPI_Event_Dispatch(pDev->pChannel[0].pTapiCh, &tapiEvent);
   }

   return iret;
}

#ifndef DXS_FEAT_LINUX_THREADED_IRQ
/**
   DUSLIC XS device interrupt service routine

   This is actually the small interrupt routine which is executed in the
   interrupt context. It just disables the interrupt and signals the irq
   event to the interrupt handling thread.

   \param pDev    Reference to device context.
   \return        IFX_IRQ_HANDLED
*/
enum IFX_irqreturn_t irq_DXS_interrupt_routine(DXS_DEVICE_t *pDev)
{
   if(pDev == IFX_NULL)
   {
      return IFX_IRQ_NONE;
   }
   /* Because some implementations of locking the IRQ line or the mutex may
      have problems to be called more than once the need-flag is used
      to make sure that these are not called again before the release. */
   if (pDev->bNeedIrqHandling == IFX_FALSE)
   {
      if(pDev->pIrq == IFX_NULL)
      {
         return IFX_IRQ_NONE;
      }

      /* lock irq */
      TAPI_DISABLE_IRQLINE_NOSYNC((IFX_uint32_t)pDev->pIrq->nIrq);

      /* set flag in pDev context */
      pDev->bNeedIrqHandling = IFX_TRUE;

      /* wakeup the interrupt handler */
#ifdef VXWORKS
      semGive(s_mtxInterruptHandler);
#else
      TAPI_OS_MutexRelease(&s_mtxInterruptHandler);
#endif
   }

   return IFX_IRQ_HANDLED;
}
#endif /* ! DXS_FEAT_LINUX_THREADED_IRQ */


#ifndef DXS_FEAT_LINUX_THREADED_IRQ
/**
   DUSLIC XS event handler thread

   The event handler thread is a single real time thread which blocks on
   a device driver global event, till it's unblocked from either the interrupt
   context or the polling timer. Once unblocked it will execute the handler
   function to retrieve the events from the DUSLIC XS, process them and finally
   in case the device is triggered by interrupt reenable the interrupts at
   the controller afterwards again.

   \param pThread    Reference to the thread context.
*/
static IFX_int32_t DXS_interrupt_thread (TAPI_OS_ThreadParams_t *pThread)
{
   IFX_int32_t mtx_ret;

   /* acquire realtime priority */
   TAPI_OS_ThreadPriorityModify(TAPI_OS_THREAD_PRIO_HIGH);

   /* main loop is waiting for an event */
   while ((pThread->bShutDown == IFX_FALSE) && (
          #ifndef VXWORKS
          mtx_ret = TAPI_OS_MutexLockInterruptible(&s_mtxInterruptHandler),
          #else
          /* vxWorks */
          mtx_ret = semTake(s_mtxInterruptHandler, WAIT_FOREVER),
          #endif
          (mtx_ret == 0)) &&
          (pThread->bShutDown == IFX_FALSE))
   {
      DXS_DEVICE_t *pDev;
      IFX_int32_t i;

      /* loop over all devices */
      for (i=0; i < DXS_MAX_DEVICES; i++)
      {
         if ((DXS_GetDevice(i, &pDev) == DXS_statusOk) &&
             (pDev->bNeedIrqHandling))
         {
            /* execute the interrupt handler */
            DXS_interrupt_handler(pDev);
         }
      }

      if (DXS_GetDevice(0, &pDev) == DXS_statusOk)
      {
         /* unset flag in pDev context */
         pDev->bNeedIrqHandling = IFX_FALSE;

         if (pDev->nEventHandlingMode == DXS_EVENT_INTERRUPT)
         {
            /* enable IRQ line */
            if(pDev->pIrq != IFX_NULL)
            {
               TAPI_ENABLE_IRQLINE((IFX_uint32_t)pDev->pIrq->nIrq);
            }
         }
      }
   }

   return 0;
}
#endif /* ! DXS_FEAT_LINUX_THREADED_IRQ */


#ifdef DXS_FEAT_LINUX_THREADED_IRQ
irqreturn_t DXS_irq_handler(int irq, void* _pIrq)
{
   TAPI_DISABLE_IRQLINE_NOSYNC(irq);
   return IRQ_WAKE_THREAD;
}
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */


#ifdef DXS_FEAT_LINUX_THREADED_IRQ
irqreturn_t DXS_irq_thread_handler(int irq, void* _pIrq)
{
   DXS_IRQ_t* pIrq = (DXS_IRQ_t*) _pIrq;
   DXS_DEVICE_t      *tmp_pDev;

   if(pIrq != IFX_NULL)
   {
      tmp_pDev = pIrq->pdev_head;

      while (tmp_pDev != IFX_NULL)
      {
         if((tmp_pDev->nDevState & DS_BASIC_INIT) &&
            (tmp_pDev->nEventHandlingMode == DXS_EVENT_INTERRUPT))
         {
            DXS_interrupt_handler(tmp_pDev);
         }
         tmp_pDev = tmp_pDev->pInt_NextDev;
      }
   }

   TAPI_ENABLE_IRQLINE(irq);
   return IRQ_HANDLED;
}
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */


#ifdef LINUX
/**
   DUSLIC XS polling mode timer callback function

   This timer callback is called periodically. It checks all devices if any is
   configured to polling mode. Any device that is found is marked for event
   handling and the handler thread is then unblocked.

   \param timer_id   Timer context ID.
   \param arg        unused parameter.
*/
static IFX_void_t dxs_polling_mode_timer (Timer_ID timer_id,
                                          IFX_ulong_t arg)
{
   DXS_DEVICE_t *pDev;
   IFX_uint16_t i;

   TAPI_UNUSED (timer_id);
   TAPI_UNUSED (arg);

   if (TAPI_OS_isRebootOngoing())
      return;

   /* check all devices */
   for (i=0; i < DXS_MAX_DEVICES; i++)
   {
      if ((DXS_GetDevice(i, &pDev) == DXS_statusOk) &&
          (pDev->nEventHandlingMode == DXS_EVENT_POLLING) &&
          (pDev->nDevState & DS_BASIC_INIT))
      {
         /* execute the interupt handler */
         DXS_interrupt_handler(pDev);
      }
   }
}

#endif /* LINUX */


/**
   Start polling mode timer

   There is just one driver global polling mode timer which serves all devices.
   This timer is started just once when the first device is set to polling mode.
*/
IFX_void_t DXS_PollingModeTimerStart (void)
{
   if (bPollTimerStarted != IFX_FALSE)
      return;

   bPollTimerStarted = IFX_TRUE;

#ifdef LINUX
   /* set the timer to an automatic reloading cycle */
   TAPI_SetTime_Timer (s_pollTimerId, DXS_POLL_CYCLE_MS, IFX_TRUE, IFX_FALSE);
#endif /* LINUX */

#ifdef VXWORKS
   TAPI_OS_ThreadInit (&s_pollTask, "DXS_poll",
            (TAPI_OS_ThreadFunction_t)PollTask,
            /* stack size */ 5000,
            /* nPriority */ TAPI_OS_THREAD_PRIO_HIGHEST,
            0, 0);

#endif /* VXWORKS */
}


/**
   Initialization of the realtime kernel interrupt handler thread and it's
   mutex semaphore.
   \return     IFX_SUCCESS
*/
IFX_return_t DXS_interrupt_init(void)
{
   DXS_DEVICE_t *pDev;
   IFX_int32_t i;

   /* loop over all devices */
   for (i=0; i < DXS_MAX_DEVICES; i++)
   {
      if (DXS_GetDevice(i, &pDev) == DXS_statusOk)
      {
         if (DXS_outbox_handler_init(pDev))
         {
            /* errmsg: Failed to start thread for outbox handling. */
            return DXS_statusObxHandlerStartError;
         }
      }
   }

#ifndef DXS_FEAT_LINUX_THREADED_IRQ
   /* initialize mutex (locked) */
   #ifdef LINUX
      TAPI_OS_MutexInit(&s_mtxInterruptHandler);
      TAPI_OS_MutexGet(&s_mtxInterruptHandler);
   #endif
   
   #ifdef VXWORKS
      s_mtxInterruptHandler = semBCreate(SEM_Q_PRIORITY, SEM_EMPTY);
   #endif

   /* start a thread to handle interrupts in task context */
   TAPI_OS_ThreadInit(&s_intThread, "TAPIdxs_int",
            (TAPI_OS_ThreadFunction_t)DXS_interrupt_thread,
            /* stack size */ 5000,
            /* nPriority */ TAPI_OS_THREAD_PRIO_HIGH,
            0, 0);
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */

   TAPI_OS_RebootNotifierRegister();

#ifdef LINUX
   /* Create polling timer */
   s_pollTimerId = TAPI_Create_Timer((TIMER_ENTRY)dxs_polling_mode_timer, 0);
   if (s_pollTimerId == 0)
      return IFX_ERROR;
#endif

   return IFX_SUCCESS;
}

/**
   Termination of the realtime kernel interrupt handler thread and
   deletion of its mutex semaphore.
   \return     IFX_SUCCESS
*/
IFX_return_t DXS_interrupt_exit(void)
{
   DXS_DEVICE_t *pDev;
   IFX_int32_t i;

   TAPI_OS_RebootNotifierUnRegister();

   if (s_pollTimerId != 0)
   {
      TAPI_Stop_Timer(s_pollTimerId);
      TAPI_Delete_Timer(s_pollTimerId);
   }

#ifndef DXS_FEAT_LINUX_THREADED_IRQ
   TAPI_OS_THREAD_KILL(&s_intThread, &s_mtxInterruptHandler);

   #ifdef VXWORKS
      semDelete(s_mtxInterruptHandler);
   #else
      TAPI_OS_MutexDelete(&s_mtxInterruptHandler);
   #endif

#endif /* DXS_FEAT_LINUX_THREADED_IRQ */

   /* loop over all devices */
   for (i=0; i < DXS_MAX_DEVICES; i++)
   {
      if (DXS_GetDevice(i, &pDev) == DXS_statusOk)
      {
         DXS_outbox_handler_exit(pDev);
      }
   }

   return IFX_SUCCESS;
}

#ifdef VXWORKS
IFX_void_t PollTask()
{
   DXS_DEVICE_t *pDev;
   IFX_uint16_t i;

   for (;;)
   {
      /* check all devices */
      for (i=0; i < DXS_MAX_DEVICES; i++)
      {
         if ((DXS_GetDevice(i, &pDev) == DXS_statusOk) &&
             (pDev->nEventHandlingMode == DXS_EVENT_POLLING))
         {
            /* set flag in pDev context */
            pDev->bNeedIrqHandling = IFX_TRUE;
         }
      }
      semGive(s_mtxInterruptHandler);
      TAPI_OS_MSecSleep (DXS_POLL_CYCLE_MS);
   }
}
#endif /* VXWORKS */


/* ========================================================================== */
/*                         Function pointer exports                           */
/* ========================================================================== */
