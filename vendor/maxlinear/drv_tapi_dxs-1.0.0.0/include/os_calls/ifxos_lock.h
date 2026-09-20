#ifndef _IFXOS_LOCK_H
#define _IFXOS_LOCK_H
/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/** \file
   This file contains definitions for Lock / Protect handling for driver and 
   user (application) space.
*/

/** \defgroup IFXOS_IF_LOCK Lock / Protection.

   This Group contains the Lock and Protection definitions and function. 

   The current implementation is based on a binary semaphore and does 
   not allow recursive calls.

\attention
   A call to get the LOCK is not interruptible.
\attention
   Do not use create and get LOCK on interrupt level.

\ingroup IFXOS_IF_SYNC
*/

#ifdef __cplusplus
   extern "C" {
#endif

/* ============================================================================
   IFX OS adaptation - Includes
   ========================================================================= */
#include "linux/ifxos_linux_lock.h"
#include <ifx_types.h>


/* ============================================================================
   IFX OS adaptation - LOCK handling, functions
   ========================================================================= */

/** \addtogroup IFXOS_IF_LOCK
@{ */

#if ( defined(IFXOS_HAVE_LOCK) && (IFXOS_HAVE_LOCK == 1) )

/**
   Check the init status of the given lock object
*/
#define IFXOS_LOCK_INIT_VALID(P_LOCK_ID)\
   (((P_LOCK_ID)) ? (((P_LOCK_ID)->bValid == IFX_TRUE) ? IFX_TRUE : IFX_FALSE) : IFX_FALSE)

/**
   Initialize a Lock Object for protection and lock.
   The lock is based on binary semaphores, recursive calls are not allowed.

\param
   lockId   Provides the pointer to the Lock Object.

\return
   IFX_SUCCESS if initialization was successful, else
   IFX_ERROR if something was wrong

*/
IFX_int32_t LINUX_LockInit(
               IFXOS_lock_t *lockId);

/**
   Delete the given Lock Object.

\param
   lockId   Provides the pointer to the Lock Object.

\return
   IFX_SUCCESS if delete was successful, else
   IFX_ERROR if something was wrong
*/
IFX_int32_t LINUX_LockDelete(
               IFXOS_lock_t *lockId);

/**
   Get the Lock (not interruptable).

\param
   lockId   Provides the pointer to the Lock Object.

\return
   IFX_SUCCESS on success.
   IFX_ERROR   on error.
*/
IFX_int32_t LINUX_LockGet(
               IFXOS_lock_t *lockId);

/**
   Release the Lock.

\param
   lockId   Provides the pointer to the Lock Object.

\return
   IFX_SUCCESS on success.
   IFX_ERROR   on error or timeout.
*/
IFX_int32_t LINUX_LockRelease(
               IFXOS_lock_t *lockId);


#if ( defined(IFXOS_HAVE_NAMED_LOCK) && (IFXOS_HAVE_NAMED_LOCK == 1) )
/**
   Initialize a Lock Object for protection and lock.
   The lock is based on IFXOS_LockInit.

\param
   lockId   Provides the pointer to the Lock Object.
\param
   pLockName   Points to the LOCK name
\param
   lockIdx     additional index which is used to generate the lock name

\return
   IFX_SUCCESS if initialization was successful, else
   IFX_ERROR if something was wrong

*/
IFX_int32_t LINUX_NamedLockInit(
               IFXOS_lock_t      *lockId,
               const IFX_char_t  *pLockName,
               const IFX_int_t   lockIdx);

#endif      /* #if ( defined(IFXOS_HAVE_NAMED_LOCK) && (IFXOS_HAVE_NAMED_LOCK == 1) ) */
#endif      /* #if ( defined(IFXOS_HAVE_LOCK) && (IFXOS_HAVE_LOCK == 1) ) */

#if ( defined(IFXOS_HAVE_LOCK_TIMEOUT) && (IFXOS_HAVE_LOCK_TIMEOUT == 1) )

/**
   Get the Lock with timeout.

\param
   lockId   Provides the pointer to the Lock Object.

\param
   timeout_ms  Timeout value [ms]
               - 0: no wait
               - -1: wait forever
               - any other value: waiting for specified amount of milliseconds
\param
   pRetCode    Points to the return code variable. [O]
               - If the pointer is NULL the return code will be ignored, else
                 the corresponding return code will be set
               - For timeout the return code is set to 1.

\return
   IFX_SUCCESS on success.
   IFX_ERROR   on error or timeout.

\note
   To detect timeouts provide the return code variable, in case of timeout
   the return code is set to 1.
*/
IFX_int32_t LINUX_LockTimedGet(
               IFXOS_lock_t *lockId,
               IFX_uint32_t timeout_ms,
               IFX_int32_t  *pRetCode);

#endif      /* #if ( defined(IFXOS_HAVE_LOCK_TIMEOUT) && (IFXOS_HAVE_LOCK_TIMEOUT == 1) ) */

/** @} */

#ifdef __cplusplus
}
#endif

#endif      /* #ifndef _IFXOS_LOCK_H */

