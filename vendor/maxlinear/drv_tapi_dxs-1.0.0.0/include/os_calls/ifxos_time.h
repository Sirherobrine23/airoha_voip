#ifndef _IFXOS_TIME_H
#define _IFXOS_TIME_H
/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/** \file
   This file contains definitions for timer and wait handling within driver and 
   user (application) space.
*/

/** \defgroup IFXOS_TIME Time and Wait.

   This Group contains the time and wait definitions and function. 

   Depending on the environment dedicated functions are available for driver
   and / or user (application) code.

\ingroup IFXOS_INTERFACE
*/

#ifdef __cplusplus
   extern "C" {
#endif

/* ============================================================================
   IFX OS adaptation - Includes
   ========================================================================= */
#include "linux/ifxos_linux_time.h"
#include <ifx_types.h>

/* ============================================================================
   IFX OS adaptation - time handling, typedefs
   ========================================================================= */
/** \addtogroup IFXOS_TIME
@{ */

/**
   This is the time datatype [ms]
*/
typedef IFX_uint32_t    IFX_timeMS_t;


/* ============================================================================
   IFX OS adaptation - time handling, functions
   ========================================================================= */

#if (defined(IFXOS_HAVE_TIME_SLEEP_US) && (IFXOS_HAVE_TIME_SLEEP_US == 1))
/**
   Sleep a given time in [us].

\param
   sleepTime_us   Time to sleep [us]

\return
   None.

\remarks
   Available in Driver and Application Space space
*/
IFX_void_t LINUX_USecSleep(
               IFX_time_t sleepTime_us);
#endif

#if (defined(IFXOS_HAVE_TIME_SLEEP_MS) && (IFXOS_HAVE_TIME_SLEEP_MS == 1))
/**
   Sleep at least the given time in [ms].

\param
   sleepTime_ms   Time to sleep [ms]

\return
   None.

\remarks
   Available in Driver and Application Space space

   Note that depending on the system tick setting the actual sleep time can be 
   equal to or longer then the specified one, but never be shorter.
*/
IFX_void_t LINUX_MSecSleep(
               IFX_time_t sleepTime_ms);
#endif

#if (defined(IFXOS_HAVE_TIME_SLEEP_SEC) && (IFXOS_HAVE_TIME_SLEEP_SEC == 1))
/**
   Sleep a given time in [seconds].

\param
   sleepTime_sec  Time to sleep [sec]

\return
   None.

\remarks
   Available in Application Space.
*/
IFX_void_t LINUX_SecSleep(
               IFX_time_t sleepTime_sec);
#endif

#if (defined(IFXOS_HAVE_TIME_ELAPSED_TIME_GET_MS) && (IFXOS_HAVE_TIME_ELAPSED_TIME_GET_MS == 1))
/**
   Get the elapsed time since startup in [ms].

\param
   refTime_ms  Reference time to calculate the elapsed time in [ms].

\return 
   Elapsed time in [ms] based on the given reference time

\remark
   Provide refTime_ms = 0 to get the current elapsed time. For messurement provide
   the current time as reference.

\remarks
   Available in Driver and Application Space space
*/
IFX_time_t LINUX_ElapsedTimeMSecGet(
               IFX_time_t refTime_ms);
#endif

#if (defined(IFXOS_HAVE_TIME_ELAPSED_TIME_GET_SEC) && (IFXOS_HAVE_TIME_ELAPSED_TIME_GET_SEC == 1))
/**
   Get the elapsed time since startup in [seconds]

\param
   refTime_sec Reference time to calculate the elapsed time in [sec].

\return 
   Elapsed time in [sec] based on the given reference time

\remark
   Provide refTime_sec = 0 to get the elapsed time since startup.

\remarks
   Available in Application Space.
*/
IFX_time_t LINUX_ElapsedTimeSecGet(
               IFX_time_t refTime_sec);
#endif

#if (defined(IFXOS_HAVE_TIME_SYS_TIME_GET) && (IFXOS_HAVE_TIME_SYS_TIME_GET == 1))
/**
   Reads the actual system time.

\return
   Returns the actual system time in seconds.
*/
IFX_time_t LINUX_SysTimeGet(void);
#endif

/** @} */

#ifdef __cplusplus
}
#endif

  
#endif      /* #ifndef _IFXOS_TIME_H */




