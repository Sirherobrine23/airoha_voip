#ifndef _DRV_TAPI_OSMAP_LOCAL_H
#define _DRV_TAPI_OSMAP_LOCAL_H
/******************************************************************************

  Copyright 2022-2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_osmap_local.h
   This file contains the includes and the defines specific to the OS and only
   for TAPI driver (which cannot be made generic for other low level drivers
   those includes osmap defines from TAPI driver e.g. TAPI_LIBRARY switch is
   only applicable to TAPI driver)
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#ifdef TAPI_LIBRARY
    #include <ifxos_std_defs.h>
#endif

#include <drv_tapi_osmap.h>

#ifdef TAPI_LIBRARY /* TAPI_LIBRARY */
   #define TAPI_OS_PROTECT_IRQLOCK(semaphore, lock) \
         ((IFX_void_t)(lock)); /* variable unused */ \
         TAPI_OS_MutexGet (semaphore)
   #define TAPI_OS_UNPROTECT_IRQLOCK(semaphore, lock) \
         ((IFX_void_t)(lock)); /* variable unused */ \
         TAPI_OS_MutexRelease (semaphore)
#endif /* TAPI_LIBRARY */

#ifdef TAPI_LIBRARY
   /** Determine if the current state is in interrupt or task context. */
   #define TAPI_OS_IN_INTERRUPT()      IFX_FALSE
   #define TAPI_OS_INTSTAT             unsigned
   #define TAPI_OS_LOCKINT(var)        ((IFX_void_t)(var)) /* variable unused */
   #define TAPI_OS_UNLOCKINT(var)      ((IFX_void_t)(var)) /* variable unused */
#endif /* TAPI_LIBRARY */

#ifdef TAPI_LIBRARY
   #define TAPI_OS_DrvSelectQueueInit(pEventId) while(0)
   #define TAPI_OS_DrvSelectQueueWakeUp(pDrvSelectQueue,drvSelType) while(0)
#endif

/* IFXOS_GET_TICK() macro for Event Logger support */
#ifndef WIN32
   #ifdef IFXOS_ENABLED
      #define IFXOS_GET_TICK()             IFXOS_ElapsedTimeMSecGet(0)
   #else
      #define IFXOS_GET_TICK()             LINUX_ElapsedTimeMSecGet(0)
   #endif /* IFXOS_ENABLED */
#else
   /* IFXOS_ElapsedTimeMSecGet can't be used, because with correct time it 
   * renders overflow and returns a negative value. It calls time() and then 
   * multiplies it by 1000. GetCurrentTime() returns uptime in ms.
   */
   #define IFXOS_GET_TICK()                GetCurrentTime()
#endif

#endif /* _DRV_TAPI_OSMAP_LOCAL_H */