#ifndef _IFXOS_THREAD_H
#define _IFXOS_THREAD_H
/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/** \file
   This file contains definitions for Thread / Task handling.
*/

/** \defgroup IFXOS_IF_THREAD Thread / Task.

   This Group contains the thread and task definitions and  
   function. 

\par IFXOS Thread / Task Concept

   The IFXOS thread / task interface hides the OS specific implementations. 
   Therefore the common OS independant user argument structure \ref IFXOS_ThreadParams_t
   is defined. 
   Furhter the user thread function must be from type \ref IFXOS_ThreadFunction_t.

   Thread / task create.\n
   Internal the thread create is wrapped to get an additional layer 
   for the IFXOS specific setup. 

   Therefore the thread create function allocates an internal OS specific data 
   structure and setup the given user informations.
   Then the thread will be created in a OS specific way with the 
   - internal OS specific data, and an
   - internal OS specific OS stub function.

   Within the internal IFXOS thread-stub function the IFXOS control setup
   is done before the user thread function is entered.
   - the run flag is set to "1"
   - the shutdown flag is set to "0"

   Thread / task delete.\n
   To terminate a thread / task the thread delete function will signal the 
   shutdown to the user thread function via the bShutDown flag within the 
   thread argument structure \ref IFXOS_ThreadFunction_t.
   This requires a cooperation of the user function and the IFXOS Thread control.

\note
   The OS specific data structures and also the OS specific stub function is 
   used only internal. So the user doesn't have to know anything about this handing.

\ingroup IFXOS_INTERFACE
*/

#ifdef __cplusplus
   extern "C" {
#endif

/* ============================================================================
   IFX OS adaptation - Common Includes
   ========================================================================= */
#include <ifx_types.h>
#include "ifxos_common.h"

/* ============================================================================
   IFX OS adaptation - Common type and definitions for the IFX Thread/Task OS
   ========================================================================= */

/** \addtogroup IFXOS_IF_THREAD
@{ */

/**
   Max length of the Thread/Task name string.

\remark
   The name is kept as a copy in the OS specific thread struct.
\remark
   Depending on the used OS the name is additional set within the OS structures
   while startup of the thread.
*/
#define IFXOS_THREAD_NAME_LEN                16

/** IFXOS Thread Delete - for user thread end wait forever */
#define IFXOS_THREAD_DELETE_WAIT_FOREVER     0xFFFFFFFF

/**
   User argument structure for the user thread start routine.
   Here the user can provide its own thread/task arguments to the thread function.
*/
typedef struct IFXOS_ThreadParams_s
{
   /** user argument 1 */
   IFX_ulong_t   nArg1;
   /** user argument 2 */
   IFX_ulong_t   nArg2;
   /** name of the thread/task */
   IFX_char_t     pName[IFXOS_THREAD_NAME_LEN];

   /** control - signal the run state */
   volatile IFX_boolean_t  bRunning;
   /** control - set to shutdown the thread */
   volatile IFX_boolean_t  bShutDown;

   /** points to the internal system object - for debugging */
   IFX_void_t              *pSysObject;

} IFXOS_ThreadParams_t;

/**
   Function type of the user thread/task function.
*/
typedef IFX_int32_t (*IFXOS_ThreadFunction_t)(IFXOS_ThreadParams_t *);

/** @} */


/* ============================================================================
   IFX OS adaptation - OS Includes
   ========================================================================= */
#include "linux/ifxos_linux_thread.h"

/* ============================================================================
   IFX OS adaptation - Thread/Task handling, functions
   ========================================================================= */
/** \addtogroup IFXOS_IF_THREAD
@{ */

#if ( defined(IFXOS_HAVE_THREAD) && (IFXOS_HAVE_THREAD == 1) )

/**
   Check the init status of the given mutex object
*/
#define IFXOS_THREAD_INIT_VALID(P_THREAD_ID)\
   (((void *)(P_THREAD_ID) != NULL) ? (((P_THREAD_ID)->bValid == IFX_TRUE) ? IFX_TRUE : IFX_FALSE) : IFX_FALSE)

/**
   Creates a new thread / task.

\param
   pThrCntrl         - Pointer to thread control structure. This structure has to 
                       be allocated outside and will be initialized.
\param
   pName             - specifies the 8-char thread / task name
\param
   pThreadFunction   - specifies the user entry function of the thread / task
\param
   nStackSize        - specifies the size of the thread stack
\param
   nPriority         - specifies the thread priority, 0 will be ignored
\param
   nArg1             - first argument passed to thread / task entry function
\param
   nArg2             - second argument passed to thread / task entry function

\return
   - IFX_SUCCESS thread was successful started.
   - IFX_ERROR thread was not deleted
*/
IFX_int32_t LINUX_ThreadInit(
               IFXOS_ThreadCtrl_t *pThrCntrl,
               const IFX_char_t     *pName,
               IFXOS_ThreadFunction_t pThreadFunction,
               IFX_uint32_t   nStackSize,
               IFX_uint32_t   nPriority,
               IFX_ulong_t    nArg1,
               IFX_ulong_t    nArg2);

/**
   Shutdown and terminate a given thread.
   Therefore the thread delete functions triggers the user thread function 
   to shutdown. In case of not responce (timeout) the thread will be canceled.

   Therefore the thread delete functions triggers the user thread function 
   to shutdown.

\param
   pThrCntrl   - Thread control struct.
\param
   waitTime_ms - Time [ms] to wait for "self-shutdown" of the user thread.

\return
   - IFX_SUCCESS thread was successful deleted.
   - IFX_ERROR thread was not deleted
*/
IFX_int32_t LINUX_ThreadDelete(
               IFXOS_ThreadCtrl_t *pThrCntrl,
               IFX_uint32_t       waitTime_ms);

/**
   Shutdown a given thread.
   Therefore the thread delete functions triggers the user thread function 
   to shutdown.

\param
   pThrCntrl   - Thread control struct.
\param
   waitTime_ms - Time [ms] to wait for "self-shutdown" of the user thread.

\return
   - IFX_SUCCESS thread was successful deleted.
   - IFX_ERROR thread was not deleted
*/
IFX_int32_t LINUX_ThreadShutdown(
               IFXOS_ThreadCtrl_t *pThrCntrl,
               IFX_uint32_t       waitTime_ms);

/**
   Set own thread / task priority

\param
   newPriority - new thread priority.
                 Possible Values are:
                 - 0 - not used
                 - IFXOS_THREAD_PRIO_IDLE
                 - IFXOS_THREAD_PRIO_LOWEST
                 - IFXOS_THREAD_PRIO_LOW
                 - IFXOS_THREAD_PRIO_NORMAL
                 - IFXOS_THREAD_PRIO_HIGH
                 - IFXOS_THREAD_PRIO_HIGHEST
                 - IFXOS_THREAD_PRIO_TIME_CRITICAL
\attention
   The intention for the priority "TIME_CRITICAL" is for use within 
   driver space.

\return
   - IFX_SUCCESS priority changed.
   - IFX_ERROR priority not changed.
*/
IFX_int32_t LINUX_ThreadPriorityModify(
               IFX_uint32_t newPriority);



/**
   Return the own thread / task ID

\return
   - thread ID of the current thread
   - IFX_ERROR (-1) if something went wrong.
*/
IFXOS_thread_t LINUX_ThreadIdGet(void);

/**
   Return the own process ID

\return
   - process ID of the current process
   - IFX_ERROR (-1) if something went wrong.

\attention
   On non multi-process systems the thread/task ID is returned.
*/
IFXOS_process_t LINUX_ProcessIdGet(void);


#endif      /* #if ( defined(IFXOS_HAVE_THREAD) && (IFXOS_HAVE_THREAD == 1) ) */

/** @} */

#ifdef __cplusplus
}
#endif

#endif      /* #ifndef _IFXOS_THREAD_H */

