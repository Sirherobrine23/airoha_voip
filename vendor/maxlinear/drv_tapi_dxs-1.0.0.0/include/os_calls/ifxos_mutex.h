#ifndef _IFXOS_MUTEX_H
#define _IFXOS_MUTEX_H
/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/** \file
   This file contains definitions for Mutex handling for driver and 
   user (application) space.
*/

/** \defgroup IFXOS_IF_MUTEX Mutex / Protection.

   This Group contains the Mutex and Protection definitions and function. 

   The current implementation is based on a binary semaphore and does 
   not allow recursive calls.

\attention
   A call to get the MUTEX is not interuptible.

\attention
   Do not use get MUTEX on interrupt level.

\ingroup IFXOS_IF_SYNC
*/

#ifdef __cplusplus
   extern "C" {
#endif

/* ============================================================================
   IFX OS adaptation - Includes
   ========================================================================= */
#include "linux/ifxos_linux_mutex.h"
#include <ifx_types.h>


/* ============================================================================
   IFX OS adaptation - MUTEX handling, functions
   ========================================================================= */

/** \addtogroup IFXOS_IF_MUTEX
@{ */

#if ( defined(IFXOS_HAVE_MUTEX) && (IFXOS_HAVE_MUTEX == 1) )

/**
   Check the init status of the given mutex object
*/
#define IFXOS_MUTEX_INIT_VALID(P_MUTEX_ID)\
   (((P_MUTEX_ID)) ? (((P_MUTEX_ID)->bValid == IFX_TRUE) ? IFX_TRUE : IFX_FALSE) : IFX_FALSE)

/**
   IFX OS adaptation - MUTEX Init

\param
   mutexId   Pointer to the Mutex Object.
   
*/
IFX_int32_t LINUX_MutexInit(
               IFXOS_mutex_t *mutexId);

/**
   Delete the Mutex.

\param
   mutexId   Pointer to the Mutex Object.

\return
   IFX_SUCCESS on success.
   IFX_ERROR   on error or timeout.
*/
IFX_int32_t LINUX_MutexDelete(
               IFXOS_mutex_t *mutexId);

/**
   Get the Mutex (not interruptible).

\param
   mutexId   Pointer to the Mutex Object.

\return
   IFX_SUCCESS on success.
   IFX_ERROR   on error.
*/
IFX_int32_t LINUX_MutexGet(
               IFXOS_mutex_t *mutexId);

/**
   Release the Mutex.

\param
   mutexId   Pointer to the Mutex Object.

\return
   IFX_SUCCESS on success.
   IFX_ERROR   on error or timeout.
*/
IFX_int32_t LINUX_MutexRelease(
               IFXOS_mutex_t *mutexId);


#endif      /* #if ( defined(IFXOS_HAVE_MUTEX) && (IFXOS_HAVE_MUTEX == 1) ) */

/** @} */

#ifdef __cplusplus
}
#endif

#endif      /* #ifndef _IFXOS_MUTEX_H */

