/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/* ============================================================================
   Description : IFX Linux adaptation - event handling (Kernel Space)
   Remark: ...
   ========================================================================= */

#ifdef LINUX
#ifdef __KERNEL__

/** \file
   This file contains the IFXOS Layer implementation for LINUX Kernel 
   Syncronistation Event.
*/

/* ============================================================================
   IFX Linux adaptation - Global Includes - Kernel
   ========================================================================= */

#include <linux/kernel.h>
#include <linux/version.h>
#ifdef MODULE
   #include <linux/module.h>
#endif

#include <linux/sched.h>
#include <linux/wait.h>

#include "../include/ifx_types.h"
#include "../include/os_calls/ifxos_common.h"
#include "../include/os_calls/ifxos_time.h"
#include "../include/os_calls/ifxos_event.h"

/* ============================================================================
   IFX Linux adaptation - Kernel EVENT handling
   ========================================================================= */
/** \addtogroup IFXOS_EVENT_LINUX_KERNEL
@{ */

#if ( defined(IFXOS_HAVE_EVENT) && (IFXOS_HAVE_EVENT == 1) )

/**
   LINUX Kernel - Create an Event Object for synchronisation.

\par Implementation
   - setup a LINUX wait queue (see "init_waitqueue_head").

\param
   pEventId    Prointer to the Event Object.

\return
   IFX_SUCCESS if the creation was successful, else
   IFX_ERROR in case of error.
*/
IFX_int_t LINUX_EventInit(
               IFXOS_event_t  *pEventId)
{
   TAPI_OS_CHECKPOINT
   if(pEventId)
   {
      if (IFXOS_EVENT_INIT_VALID(pEventId) == IFX_FALSE)
      {
         init_waitqueue_head (&pEventId->object);
         pEventId->bValid         = IFX_TRUE;
         pEventId->bConditionFlag = 0;

         return IFX_SUCCESS;
      }
   }

   return IFX_ERROR;
}

/**
   LINUX Kernel - Delete the given Event Object.

\par Implementation
   - nothing to do under LINUX.

\param
   pEventId    Prointer to the Event Object.

\return
   IFX_SUCCESS if delete was successful, else
   IFX_ERROR if something was wrong
*/
IFX_int_t LINUX_EventDelete(
               IFXOS_event_t  *pEventId)
{
   TAPI_OS_CHECKPOINT
   if (pEventId)
   {
      if (IFXOS_EVENT_INIT_VALID(pEventId) == IFX_TRUE)
      {
         /*remove_wait_queue(&pEventId->object); fixme */
         pEventId->bValid         = IFX_FALSE;
         pEventId->bConditionFlag = 0;
         
         return IFX_SUCCESS;
      }
   }

   return IFX_ERROR;
}

/**
   LINUX Kernel - Wakeup a Event Object to signal the occurance of 
   the "event" to the waiting processes.

\par Implementation
   - signal a wakeup for the given wait queue (see "wake_up_interruptible").

\param
   pEventId    Prointer to the Event Object.

\return
   IFX_SUCCESS on success.
   IFX_ERROR   on error
*/
IFX_int_t LINUX_EventWakeUp(
               IFXOS_event_t  *pEventId)
{
   TAPI_OS_CHECKPOINT
   if (pEventId)
   {
      if (IFXOS_EVENT_INIT_VALID(pEventId) == IFX_TRUE)
      {
         pEventId->bConditionFlag = 1;
         wake_up_interruptible(&pEventId->object);

         return IFX_SUCCESS;
      }
   }

   return IFX_ERROR;
}

/**
   LINUX Kernel - Wait for the occurance of an "event" with timeout.

\par Implementation
   - sleep on the given wait queue with timeout [ms] (see "interruptible_sleep_on_timeout").
   - timeout signaling currently not supported.

\param
   pEventId       Prointer to the Event Object.
\param
   waitTime_ms    Max time to wait [ms].
\param
   pRetCode    Points to the return code variable. [O]
               - If the pointer is NULL the return code will be ignored, else
                 the corresponding return code will be set
               - For timeout the return code is set to 1.

\return
   IFX_SUCCESS on success.
   IFX_ERROR   on error or timeout.
*/
IFX_int_t LINUX_EventWait(
               IFXOS_event_t  *pEventId,
               IFX_uint32_t   waitTime_ms,
               IFX_int32_t    *pRetCode)
{
   TAPI_OS_CHECKPOINT
   if (pEventId)
   {
      if (IFXOS_EVENT_INIT_VALID(pEventId) == IFX_TRUE)
      {
         int ret;
         ret = wait_event_interruptible_timeout(
                     pEventId->object, 
                     (pEventId->bConditionFlag == 1), 
                     ((HZ * (waitTime_ms)) / 1000));
         if (ret <= 0)
         {
            if (pRetCode && ret == 0)
               *pRetCode = 1;

            pEventId->bConditionFlag = 0;
            return IFX_ERROR;
         }

         pEventId->bConditionFlag = 0;
         return IFX_SUCCESS;
      }
   }

   return IFX_ERROR;
}


#endif      /* #if ( defined(IFXOS_HAVE_EVENT) && (IFXOS_HAVE_EVENT == 1) ) */

/** @} */

#endif      /* #ifdef __KERNEL__ */
#endif      /* #ifdef LINUX */

