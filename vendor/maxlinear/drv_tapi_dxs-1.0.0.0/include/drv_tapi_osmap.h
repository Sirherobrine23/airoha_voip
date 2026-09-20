#ifndef _DRV_TAPI_OSMAP_H
#define _DRV_TAPI_OSMAP_H

/******************************************************************************

  Copyright 2015      Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2020      Intel Corporation.
  Copyright 2022-2023 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_osmap.h
   This file contains the includes and the defines specific to the OS.
*/

#ifdef __cplusplus
   extern "C" {
#endif

#include "drv_tapi_config.h"

#if !defined(LINUX) && !defined(VXWORKS) && \
    !defined(WINDOWS) && !defined(WIN32) && \
    !(defined(GENERIC_OS) && defined(GREENHILLS_CHECK))
      #error TAPI driver - no OS specified. Please define your operating system!
#endif

#if defined(LINUX) && \
   (!defined(TAPI_LINUX_KERNEL_SPACE) && !defined(TAPI_LINUX_USER_SPACE))

   #if defined(__KERNEL__)
      /* TAPI (and other low level drivers) running in kernel space */
      #define TAPI_LINUX_KERNEL_SPACE
   #else
       /* TAPI (and other low level drivers) running in user space */
      #define TAPI_LINUX_USER_SPACE
   #endif
#endif

/* define prototypes for VxWorks 5 */
#if (defined(VXWORKS)) && (!defined(__PROTOTYPE_5_0))
   #define __PROTOTYPE_5_0
#endif

#ifdef IFXOS_ENABLED
   #include <ifxos_memory_alloc.h>
   #include <ifxos_copy_user_space.h>
   #include <ifxos_debug.h>
   #include <ifxos_event.h>
   #include <ifxos_select.h>
   #include <ifxos_mutex.h>
   #include <ifxos_lock.h>
   #include <ifxos_time.h>
   #include <ifxos_thread.h>
   #include <ifxos_interrupt.h>
#else
   /* Local ifxos files can handle only Linux,
      other systems needs external ifxos (or files need to be copied to local
      directories) */
   #include "os_calls/ifxos_memory_alloc.h"
   #include "os_calls/ifxos_copy_user_space.h"
   #include "os_calls/ifxos_debug.h"
   #include "os_calls/ifxos_event.h"
   #include "os_calls/ifxos_select.h"
   #include "os_calls/ifxos_mutex.h"
   #include "os_calls/ifxos_lock.h"
   #include "os_calls/ifxos_time.h"
   #include "os_calls/ifxos_thread.h"
   #include "os_calls/ifxos_interrupt.h"
#endif

#include "ifx_types.h"

#ifdef GREENHILLS_CHECK
   #include "drv_tapi_ghs.h"
#endif

#ifndef TAPI_LIBRARY
   /* Linux kernel space */
   #if defined(LINUX)
      #if defined(__KERNEL__)
         #include <linux/irq.h>
         #include <linux/threads.h>
         #include <linux/interrupt.h> /* in_interrupt()  */
         #include <asm/poll.h>        /* POLLIN, POLLOUT */
         #if defined(TAPI_FEAT_LINUX_SMP)
            #include <linux/spinlock.h>
         #endif /* TAPI_FEAT_LINUX_SMP */
      #else
         #include <os_calls/ifxos_std_defs.h>
         #include <netinet/in.h> /* ntohs(), etc. */
      #endif
   #endif

   #ifdef VXWORKS
      #include <intLib.h>
   #endif
   
   #ifdef WINDOWS
      #include <sys_tickedtimer.h>
   #endif /* WINDOWS */  
#endif /* TAPI_LIBRARY */

/* ==========================================================================*/
/* Compiler dependent macros                                                 */
/* ========================================================================= */

#ifndef __PACKED__
   #if defined (__GNUC__) || defined (__GNUG__)
      /* GNU C or C++ compiler */
      #define __PACKED__ __attribute__ ((packed))
   #elif !defined (__PACKED__)
      #define __PACKED__ /* nothing */
   #endif
#endif

/* ==========================================================================*/
/* Debug features                                                            */
/* ========================================================================= */

/** Define trace output
   \remark Traces can be completely switched off with the compiler switch 
           ENABLE_TRACE to 0
*/

/** No trace output */
#define DBG_LEVEL_OFF    4
/** Only important traces, as errors or some warnings */
#define DBG_LEVEL_HIGH   3
/** Including traces from high and general proceedings and possible problems */
#define DBG_LEVEL_NORMAL 2
/** All traces and low level traces as basic chip access and interrupts, 
    command data */
#define DBG_LEVEL_LOW    1

/** Define the used CR/LF sequence */
#define TAPI_CRLF        IFXOS_CRLF

#define TAPI_CREATE_TRACE_GROUP(module_name, default_level) \
         IFXOS_PRN_USR_MODULE_CREATE(module_name, default_level)
#define TAPI_DECLARE_TRACE_GROUP(module_name) \
         IFXOS_PRN_USR_MODULE_DECL(module_name)
#define TAPI_GET_TRACE_GROUP_VARIABLE(module_name) \
         IFXOS_PrnUsrModule_##module_name

#ifdef ENABLE_TRACE
   #define TRACE(name, dbg_level, print_message) \
            IFXOS_PRN_USR_DBG_NL(name, dbg_level, print_message)
   #define TAPI_TRACE_LEVEL_SET(name, new_level) \
            IFXOS_PRN_USR_LEVEL_SET(name, new_level)
#else
   /* no operation - traces disabled */
   #define TRACE(name, dbg_level, print_message)
   /* no operation - traces disabled */
   #define TAPI_TRACE_LEVEL_SET(name, new_level)
#endif /* ENABLE_TRACE */

/** Assert in debug code
   \param expr - expression to be evaluated. If expr != TRUE assert is printed
                 out with line number */
#define TAPI_ASSERT_BODY(expr)                                                  \
   do {                                                                         \
      /*lint -save -e{506, 774} */                                              \
      if (!(expr)) {                                                            \
         IFXOS_DBG_PRINT_USR("\n\r" __FILE__ ":%d: Assertion %s "               \
                             "failed!\n\r", __LINE__, #expr);                   \
      }                                                                         \
      /*lint -restore */                                                        \
   } while (0)

#ifdef DEBUG
   #define TAPI_ASSERT(expr) TAPI_ASSERT_BODY(expr)
#else
   #define TAPI_ASSERT(expr) /* no operation in non-debug code */
#endif

/** Helper for dumping/tracing memory contents */
#if defined(ENABLE_TRACE) && defined(TAPI_LINUX_KERNEL_SPACE)
   /* Currently on Linux kernel space is supported */
   #define TAPI_TRACE_BYTES_FROM_MEMORY(prefix_str, buf, len)                  \
            print_hex_dump_debug(prefix_str, DUMP_PREFIX_NONE, 16, 1,          \
                                 buf, len, false)
#else
   /* Tracing bytes from memory not implemented */
   #define TAPI_TRACE_BYTES_FROM_MEMORY(prefix_str, buf, len)
#endif /* ENABLE_TRACE */

#define TAPI_LITTLE_ENDIAN              IFXOS_LITTLE_ENDIAN
#define TAPI_BIG_ENDIAN                 IFXOS_BIG_ENDIAN
#define TAPI_BYTE_ORDER                 IFXOS_BYTE_ORDER

/* ==========================================================================*/
/* Dynamic memory handling                                                   */
/* ========================================================================= */
#ifdef IFXOS_ENABLED
   #if defined(LINUX) && defined(__KERNEL__)
      #define TAPI_OS_Malloc                 IFXOS_BlockAlloc
      #define TAPI_OS_Free                   IFXOS_BlockFree
   #else
      #define TAPI_OS_Malloc                 IFXOS_MemAlloc
      #define TAPI_OS_Free                   IFXOS_MemFree
   #endif
#else /* external IFXOS disabled */
   #if defined(LINUX) && defined(__KERNEL__)
      #define TAPI_OS_Malloc                 LINUX_BlockAlloc
      #define TAPI_OS_Free                   LINUX_BlockFree
   #else
      #define TAPI_OS_Malloc                 LINUX_MemAlloc
      #define TAPI_OS_Free                   LINUX_MemFree
   #endif
#endif /* IFXOS_ENABLED */

/*
   Mapping table - Kernel-space / User-space data exchange.
*/
#ifdef TAPI_LINUX_USER_SPACE
   #define TAPI_OS_CpyKern2Usr(pTo, pFrom, size) \
         (memcpy(pTo, pFrom, size) != IFX_NULL)
   #define TAPI_OS_CpyUsr2Kern(pTo, pFrom, size) \
         (memcpy(pTo, pFrom, size) != IFX_NULL)
#else
   #ifdef IFXOS_ENABLED
      #define TAPI_OS_CpyKern2Usr            IFXOS_CpyToUser
      #define TAPI_OS_CpyUsr2Kern            IFXOS_CpyFromUser
   #else
      #define TAPI_OS_CpyKern2Usr            LINUX_CpyToUser
      #define TAPI_OS_CpyUsr2Kern            LINUX_CpyFromUser
   #endif /* IFXOS_ENABLED */
#endif /* TAPI_LINUX_USER_SPACE */


/* ==========================================================================*/
/* Mutex handling                                                            */
/* ========================================================================= */

#ifdef LINUX
   #define TAPI_OS_MutexForInterrupts_t TAPI_OS_mutex_t
#elif defined(VXWORKS)
   #define TAPI_OS_MutexForInterrupts_t SEM_ID
#endif

#define TAPI_OS_mutex_t                      IFXOS_mutex_t

#ifdef IFXOS_ENABLED
   #define TAPI_OS_MutexInit                 IFXOS_MutexInit
   #define TAPI_OS_MutexDelete               IFXOS_MutexDelete
   #define TAPI_OS_MutexGet                  IFXOS_MutexGet
   #define TAPI_OS_MutexRelease              IFXOS_MutexRelease
#else
   #define TAPI_OS_MutexInit                 LINUX_MutexInit
   #define TAPI_OS_MutexDelete               LINUX_MutexDelete
   #define TAPI_OS_MutexGet                  LINUX_MutexGet
   #define TAPI_OS_MutexRelease              LINUX_MutexRelease
#endif /* IFXOS_ENABLED */


/* ==========================================================================*/
/* Lock handling                                                             */
/* \remark Linux implementation for kernel space is based on kernel          */
/*         semaphores (based on spinlocks internally), for user space it is  */
/*         based on pthread semaphores.                                      */
/* ========================================================================= */

#define TAPI_OS_lock_t                       IFXOS_lock_t

#ifdef IFXOS_ENABLED
   #define TAPI_OS_LockInit                  IFXOS_LockInit
   #define TAPI_OS_LockGet                   IFXOS_LockGet
   #define TAPI_OS_LockRelease               IFXOS_LockRelease
   #define TAPI_OS_LockDelete                IFXOS_LockDelete
   #define TAPI_OS_LockTimedGet              IFXOS_LockTimedGet
#else
   #define TAPI_OS_LockInit                  LINUX_LockInit
   #define TAPI_OS_LockGet                   LINUX_LockGet
   #define TAPI_OS_LockRelease               LINUX_LockRelease
   #define TAPI_OS_LockDelete                LINUX_LockDelete
   #define TAPI_OS_LockTimedGet              LINUX_LockTimedGet
#endif /* IFXOS_ENABLED */

/* Lock (take the mutex) but be interruptable by signals.
   Difference to TAPI_OS_LockGet is that the sleep can be interrupted
   when a signal is received by the sleeping thread. */
#ifdef TAPI_LINUX_KERNEL_SPACE
   #define TAPI_OS_LOCK_GET_INTERRUPTIBLE(lockId) \
      (down_interruptible(&(lockId)->object))

   #define TAPI_OS_MutexLockInterruptible TAPI_OS_LOCK_GET_INTERRUPTIBLE

#elif defined(TAPI_LINUX_USER_SPACE)
   #define TAPI_OS_MutexLockInterruptible TAPI_OS_MutexGet
#endif /* LINUX */

#ifdef VXWORKS
   #define TAPI_OS_LOCK_GET_INTERRUPTIBLE(pmtx) \
           semTake((pmtx)->object, WAIT_FOREVER)

   #define TAPI_OS_MutexLockInterruptible TAPI_OS_LOCK_GET_INTERRUPTIBLE
#endif /* VXWORKS */

#ifdef WINDOWS
   #define TAPI_OS_LOCK_GET_INTERRUPTIBLE TAPI_OS_LockGet
#endif /* WINDOWS */


#if !defined(TAPI_LIBRARY)
   #if defined(TAPI_FEAT_LINUX_SMP)
      #define TAPI_OS_PROTECT_IRQLOCK(spinlock, lockId) \
              spin_lock_irqsave((spinlock), (lockId))

      #define TAPI_OS_UNPROTECT_IRQLOCK(spinlock, lockId) \
              spin_unlock_irqrestore((spinlock), (lockId))

   #else /* TAPI_FEAT_LINUX_SMP */
      #define TAPI_OS_PROTECT_IRQLOCK(semaphore, lockId) \
              if (!TAPI_OS_IN_INTERRUPT())               \
              {                                          \
                 TAPI_OS_MutexGet(semaphore);            \
              }                                          \
              TAPI_OS_LOCKINT(lockId)

      #define TAPI_OS_UNPROTECT_IRQLOCK(semaphore, lockId) \
              TAPI_OS_UNLOCKINT(lockId);                   \
              if (!TAPI_OS_IN_INTERRUPT())                 \
              {                                            \
                 TAPI_OS_MutexRelease(semaphore);          \
              }
   #endif /* TAPI_FEAT_LINUX_SMP */
#endif


/* ==========================================================================*/
/* Interrupts handling                                                       */
/* ========================================================================= */

/* Types declaration */

#define TAPI_OS_INTSTAT IFXOS_INTSTAT

/** return value of the interrupt service routine telling that ... */
enum IFX_irqreturn_t
{
   /** no interrupt has been handled for this device */
   IFX_IRQ_NONE = 0,
   /** an interrupt has been handled for this device */
   IFX_IRQ_HANDLED = 1
};

#if !defined(TAPI_LIBRARY)
   #ifdef TAPI_LINUX_KERNEL_SPACE
      /** Determine if the current state is in interrupt or task context. */
      #define TAPI_OS_IN_INTERRUPT()  (in_interrupt() ? IFX_TRUE : IFX_FALSE)
      #define TAPI_OS_LOCKINT(var)    local_irq_save(var)
      #define TAPI_OS_UNLOCKINT(var)  local_irq_restore(var)
   #endif /* LINUX */

   #ifdef VXWORKS
      /** Determine if the current state is in interrupt or task context. */
      #define TAPI_OS_IN_INTERRUPT() ((intContext() == TRUE) ? IFX_TRUE : IFX_FALSE)
      #define TAPI_OS_LOCKINT(var)   var = intLock()
      #define TAPI_OS_UNLOCKINT(var) intUnlock(var)
   #endif

   #ifdef WINDOWS
      /** Determine if the current state is in interrupt or task context. */
      #define TAPI_OS_IN_INTERRUPT() IFX_FALSE
   #endif
#endif

/**
   Macro to map the system function which takes care of interrupt handling
   registration.
   \param irq  irq number
   \param func interrupt handler callback function
   \param arg  argument of interrupt handler callback function
   \remarks
   The macro is by default mapped to the operating system method. For systems
   integrating different routines, this macro must be adapted in the user con-
   figuration header file.
*/
#ifdef LINUX
   #define TAPI_SYS_REGISTER_INT_HANDLER(irq, flags, func, arg) \
           request_irq((irq), (func), (flags), DXS_DEV_NAME, (IFX_void_t*)(arg))

   #define TAPI_SYS_REGISTER_INT_HANDLER_THREAD(irq, flags, fun_irq, fun_th, arg) \
           request_threaded_irq((irq), (fun_irq), (fun_th), (flags),              \
                                DXS_DEV_NAME, (IFX_void_t*)(arg));
#elif defined(VXWORKS)
   #define TAPI_SYS_REGISTER_INT_HANDLER(irq, func, arg)                 \
            sysGpioIntConnect(((pDev)->nDevNr == 1) ?                    \
               DXS_EXINT_DEV1 : DXS_EXINT_DEV0, (VOIDFUNCPTR)(func),     \
               (IFX_int32_t)(arg))
#endif

/**
   Macro to map the system function which takes care of unregistration of
   the interrupt handler.
   \param irq irq number
   \remarks
   The macro is by default mapped to the operating system method. For systems
   integrating different routines, this macro must be adapted in the user con-
   figuration header file.
*/
#if defined(LINUX)
   #define TAPI_SYS_UNREGISTER_INT_HANDLER(irq,arg) \
            free_irq((irq), (IFX_void_t*)(arg))
#elif defined(VXWORKS)
   #define TAPI_SYS_UNREGISTER_INT_HANDLER(irq, arg) \
            TAPI_SYS_REGISTER_INT_HANDLER((irq), OS_IRQHandler_Dummy, (irq))
#endif

/**
   Enables the global interrupt
   \param var  local mask that has to be stored for TAPI_ENABLE_IRQGLOBAL
   \remarks
   This macro is system specific. In case the Operating system methods can not
   be used for this purpose, define the correct macro in your user configuration
   file.
   This macro is used when no device specific interrupt handler or interrupt
   line is configured (nIrg < 0).
*/
#define TAPI_ENABLE_IRQGLOBAL(var)      IFXOS_UNLOCKINT(var)

/**
   Disables the global interrupt
   \param var  local mask that has to be stored for TAPI_ENABLE_IRQGLOBAL
   \remarks
   This macro is system specific. In case the Operating system methods can not
   be used for this purpose, define the correct macro in your user configuration
   file.
*/
#define TAPI_DISABLE_IRQGLOBAL(var)    IFXOS_LOCKINT(var)

/**
   Enables the interrupt line
   \param irq  irq number
   \remarks
   This macro is system specific. In case the Operating system methods can not
   be used for this purpose, define the correct macro in your user configuration
   file.
*/
#ifdef LINUX
   #define TAPI_ENABLE_IRQLINE(irq) IFXOS_IRQ_ENABLE(irq)

#elif defined(VXWORKS)
   #define TAPI_ENABLE_IRQLINE(irq)             \
      if (irq == 252) {                         \
         if (pInterruptCounters[0]++ == 0)      \
            sysIntEnableICU(irq);               \
      }                                         \
      else {                                    \
         if (pInterruptCounters[1]++ == 0)      \
            sysIntEnableICU(irq);               \
      }
#endif /* LINUX */

/**
   Disables the interrupt line
   \param irq  irq number
   \remarks
   This macro is system specific. In case the Operating system methods can not
   be used for this purpose, define the correct macro in your user configuration
   file.
*/
#ifdef LINUX
   #define TAPI_DISABLE_IRQLINE(irq)        IFXOS_IRQ_DISABLE(irq)
   #define TAPI_DISABLE_IRQLINE_NOSYNC(irq) disable_irq_nosync(irq)

#elif defined(VXWORKS)
   #define TAPI_DISABLE_IRQLINE(irq)                  \
      if (irq == 252) {                               \
         if (--pInterruptCounters[0] == 0) {          \
            sysIntDisableICU(irq);                    \
         }                                            \
      } else {                                        \
         if (--pInterruptCounters[1] == 0) {          \
            sysIntDisableICU(irq);                    \
         }                                            \
      }
   #define TAPI_DISABLE_IRQLINE_NOSYNC(irq) TAPI_DISABLE_IRQLINE(irq)
#endif /* LINUX */


/* ==========================================================================*/
/* Spinlocks handling                                                        */
/* ========================================================================= */

#if defined(TAPI_FEAT_LINUX_SMP) && defined(LINUX)
   #define IFXOS_spinlock_t spinlock_t

   #define TAPI_OS_SPIN_LOCK_INIT(P_TAPI_SPIN_LOCK)\
      do { spin_lock_init(&(P_TAPI_SPIN_LOCK)->sl_handle); } while(0)

   #define TAPI_OS_SPIN_LOCK(P_TAPI_SPIN_LOCK) \
      do { spin_lock(&(P_TAPI_SPIN_LOCK)->sl_handle); } while(0)
      
   #define TAPI_OS_SPIN_UNLOCK(P_TAPI_SPIN_LOCK) \
      do { spin_unlock(&(P_TAPI_SPIN_LOCK)->sl_handle); } while(0)

   #define TAPI_OS_SPIN_LOCK_IRQ(P_TAPI_SPIN_LOCK) \
      do { spin_lock_irq(&(P_TAPI_SPIN_LOCK)->sl_handle); } while(0)
      
   #define TAPI_OS_SPIN_UNLOCK_IRQ(P_TAPI_SPIN_LOCK) \
      do { spin_unlock_irq(&(P_TAPI_SPIN_LOCK)->sl_handle); } while(0)

   #define TAPI_OS_SPIN_LOCK_IRQSAVE(P_TAPI_SPIN_LOCK) \
      do { spin_lock_irqsave(&(P_TAPI_SPIN_LOCK)->sl_handle, \
                              (P_TAPI_SPIN_LOCK)->irq_flags); \
      } while(0)

   #define TAPI_OS_SPIN_UNLOCK_IRQRESTORE(P_TAPI_SPIN_LOCK) \
      do { spin_unlock_irqrestore(&(P_TAPI_SPIN_LOCK)->sl_handle, \
                                   (P_TAPI_SPIN_LOCK)->irq_flags); \
      } while(0)
#else
   #define IFXOS_spinlock_t unsigned int
   #define TAPI_OS_SPIN_LOCK_INIT(P_TAPI_SPIN_LOCK)
   #define TAPI_OS_SPIN_LOCK(P_TAPI_SPIN_LOCK)
   #define TAPI_OS_SPIN_UNLOCK(P_TAPI_SPIN_LOCK)
   #define TAPI_OS_SPIN_LOCK_IRQSAVE(P_TAPI_SPIN_LOCK)
   #define TAPI_OS_SPIN_UNLOCK_IRQRESTORE(P_TAPI_SPIN_LOCK)
   #define TAPI_OS_SPIN_LOCK_IRQ(P_TAPI_SPIN_LOCK)
   #define TAPI_OS_SPIN_UNLOCK_IRQ(P_TAPI_SPIN_LOCK)
#endif /* TAPI_FEAT_LINUX_SMP */

struct TAPI_OS_spin_lock_s
{
   IFXOS_spinlock_t sl_handle;
   TAPI_OS_INTSTAT irq_flags;
};

/*
   Mapping table - Event signalling.
*/
/* ==========================================================================*/
/* Mapping table - Event signalling.                                         */
/* ========================================================================= */

#define TAPI_OS_event_t                      IFXOS_event_t

#ifdef IFXOS_ENABLED
   #define TAPI_OS_EventInit                 IFXOS_EventInit
   #define TAPI_OS_EventWakeUp               IFXOS_EventWakeUp
   #define TAPI_OS_EventWait                 IFXOS_EventWait
   #define TAPI_OS_EventDelete               IFXOS_EventDelete
#else
   #define TAPI_OS_EventInit                 LINUX_EventInit
   #define TAPI_OS_EventWakeUp               LINUX_EventWakeUp
   #define TAPI_OS_EventWait                 LINUX_EventWait
   #define TAPI_OS_EventDelete               LINUX_EventDelete
#endif /* IFXOS_ENABLED */

#define TAPI_OS_WAIT_FOREVER                 IFXOS_WAIT_FOREVER


/*
   Mapping table - Select handling.
*/
#define TAPI_OS_drvSelectQueue_t             IFXOS_drvSelectQueue_t
#define TAPI_OS_drvSelectTable_t             IFXOS_drvSelectTable_t
#define TAPI_OS_drvSelectOSArg_t             IFXOS_drvSelectOSArg_t

#ifndef TAPI_LIBRARY
   #ifdef IFXOS_ENABLED
      #define TAPI_OS_DrvSelectQueueInit     IFXOS_DrvSelectQueueInit
      #define TAPI_OS_DrvSelectQueueWakeUp   IFXOS_DrvSelectQueueWakeUp
   #else
      #define TAPI_OS_DrvSelectQueueInit     LINUX_DrvSelectQueueInit
      #define TAPI_OS_DrvSelectQueueWakeUp   LINUX_DrvSelectQueueWakeUp
   #endif /* IFXOS_ENABLED */
#endif /* TAPI_LIBRARY */

#define TAPI_OS_DRV_SEL_WAKEUP_TYPE_RD       IFXOS_DRV_SEL_WAKEUP_TYPE_RD
#define TAPI_OS_DRV_SEL_WAKEUP_TYPE_WR       IFXOS_DRV_SEL_WAKEUP_TYPE_WR


/* ==========================================================================*/
/* Mapping table - Queues.                                                   */
/* ========================================================================= */

#ifdef IFXOS_ENABLED
   #define TAPI_OS_DrvSelectQueueAddTask     IFXOS_DrvSelectQueueAddTask
#else
   #define TAPI_OS_DrvSelectQueueAddTask     LINUX_DrvSelectQueueAddTask
#endif

#define TAPI_OS_DrvSelectQueueDelete(param) /* does not exist */

/** Definitions for select queues. */
/* TAPI_OS_SYSWRITE is returned by select() to indicate that the fd is ready
   for writing.
   TAPI_OS_SYSREAD is returned by select() to indicate that the fd is ready
   for reading. */
#ifdef TAPI_LINUX_KERNEL_SPACE
   #define TAPI_OS_SYSWRITE     POLLOUT
   #define TAPI_OS_SYSREAD      POLLIN
#endif

#ifdef VXWORKS
   #define TAPI_OS_SYSWRITE     0x00000002
   #define TAPI_OS_SYSREAD      0x00000001
#endif

#ifdef WINDOWS
   #define  TAPI_OS_SYSWRITE    0x00000002
   #define  TAPI_OS_SYSREAD     0x00000001
#endif


/* ==========================================================================*/
/* Mapping table - Thread handling.                                          */
/* ========================================================================= */
#define TAPI_OS_THREAD_PRIO_HIGH            IFXOS_THREAD_PRIO_HIGH
#define TAPI_OS_THREAD_PRIO_HIGHEST         IFXOS_THREAD_PRIO_HIGHEST
#define TAPI_OS_ThreadCtrl_t                IFXOS_ThreadCtrl_t
#define TAPI_OS_ThreadParams_t              IFXOS_ThreadParams_t
#define TAPI_OS_ThreadFunction_t            IFXOS_ThreadFunction_t

#ifdef IFXOS_ENABLED
   #define TAPI_OS_ThreadInit               IFXOS_ThreadInit
   #define TAPI_OS_ThreadDelete             IFXOS_ThreadDelete
#else
   #define TAPI_OS_ThreadInit               LINUX_ThreadInit
   #define TAPI_OS_ThreadDelete             LINUX_ThreadDelete
#endif /* IFXOS_ENABLED */

#ifdef TAPI_LINUX_KERNEL_SPACE
   #define TAPI_OS_THREAD_KILL              TAPI_OS_ThreadKill
   #define TAPI_OS_THREAD_PRIORITY_MODIFY   TAPI_OS_ThreadPriorityModify
   #define TAPI_OS_THREAD_ID                (current->pid)
#elif defined(TAPI_LINUX_USER_SPACE)
   /* TODO: Find ThreadKill for Linux User space */
   #define TAPI_OS_THREAD_KILL              TAPI_OS_ThreadKill
   #define TAPI_OS_THREAD_PRIORITY_MODIFY   IFXOS_ThreadPriorityModify
   #define TAPI_OS_THREAD_ID                (0)
#endif /* LINUX && __KERNEL */

#ifdef VXWORKS
   #define TAPI_OS_THREAD_KILL(pThrCntrl, pLock) /* empty */
   #define TAPI_OS_THREAD_PRIORITY_MODIFY   TAPI_OS_ThreadPriorityModify
   #define TAPI_OS_THREAD_ID                (0)
#endif

#ifdef WINDOWS
   #define TAPI_OS_THREAD_KILL(pTGet_tichrCntrl, pLock) /* empty */
   #define TAPI_OS_THREAD_PRIORITY_MODIFY   IFXOS_ThreadPriorityModify
   #define TAPI_OS_THREAD_ID                (0)
#endif

#ifdef __KERNEL__
   extern IFX_int32_t TAPI_OS_ThreadPriorityModify(IFX_uint32_t newPriority);
   extern IFX_void_t  TAPI_OS_ThreadKill(IFXOS_ThreadCtrl_t *pThrCntrl,
                                         TAPI_OS_lock_t *pLock);
#endif


/* ==========================================================================*/
/* Mapping table - Time and Waiting.                                         */
/* ========================================================================= */
#ifdef TAPI_LINUX_USER_SPACE
   /* IFXOS_USecSleep is missing for user space in IFXOS library,
      so a separate implementation is needed */
   #include <time.h>
   #include <sys/time.h>

   #define TAPI_OS_USecSleep(sleepTime_us)                                    \
      do                                                                      \
      {                                                                       \
         struct timespec tv = {0};                                            \
         tv.tv_sec = sleepTime_us/1000000;                                    \
         tv.tv_nsec = (long) ((sleepTime_us - (tv.tv_sec * 1000000)) * 1000); \
         while (1)                                                            \
         {                                                                    \
            int rval = nanosleep(&tv, &tv);                                   \
            if (rval == 0)                                                    \
            {                                                                 \
               break;                                                         \
            }                                                                 \
            else                                                              \
            {                                                                 \
               if (errno == EINTR)                                            \
                  continue;                                                   \
               else                                                           \
                  break;                                                      \
            }                                                                 \
         }                                                                    \
      } while(0)
#else /* !TAPI_LINUX_USER_SPACE (= TAPI_LINUX_KERNEL_SPACE) */
   #ifdef IFXOS_ENABLED
      #define TAPI_OS_USecSleep             IFXOS_USecSleep
   #else
      #define TAPI_OS_USecSleep             LINUX_USecSleep
   #endif /* IFXOS_ENABLED */
#endif /* TAPI_LINUX_USER_SPACE */

#ifdef IFXOS_ENABLED
   #define TAPI_OS_MSecSleep                IFXOS_MSecSleep
#else
   #define TAPI_OS_MSecSleep                LINUX_MSecSleep
#endif /* IFXOS_ENABLED */

#ifdef TAPI_LINUX_KERNEL_SPACE
   #include <linux/timekeeping.h>

   /* Get system timestamp */
   #define TAPI_OS_GetTimestamp()           ktime_get_ns()

   /* Nanoseconds per second */
   #define TAPI_OS_TIME_NSEC_IN_SEC         NSEC_PER_SEC
#endif

/* ==========================================================================*/
/* IOCTL handling                                                       */
/* ========================================================================= */

/** Returns the magic number of IOCTL command
\param iocmd - ioctl command of which magic number is decoded */
#define TAPI_IOC_MAGIC(cmd)   (_IOC_TYPE(cmd))

/** Returns the write request of IOCTL command
\param iocmd - ioctl command */
#define TAPI_IOC_WRITE(cmd)   ((_IOC_DIR(cmd) & _IOC_WRITE) ? \
                              IFX_TRUE : IFX_FALSE)

/** Returns the read request of IOCTL command
\param iocmd - ioctl command */
#define TAPI_IOC_READ(cmd)    ((_IOC_DIR(cmd) & _IOC_READ) ? \
                              IFX_TRUE : IFX_FALSE)

/** Returns the IOCTL command number
\param iocmd - ioctl command */
#define TAPI_IOC_IDX(cmd)     (_IOC_NR(cmd))

/** Returns the argument size of IOCTL command
\param iocmd - ioctl command */
#define TAPI_IOC_SIZE(cmd)    (_IOC_SIZE(cmd))


/* ==========================================================================*/
/* Reboot notifier                                                           */
/* ========================================================================= */
#ifdef LINUX
   extern IFX_void_t TAPI_OS_RebootNotifierRegister(IFX_void_t);
   extern IFX_void_t TAPI_OS_RebootNotifierUnRegister(IFX_void_t);
   extern IFX_boolean_t TAPI_OS_isRebootOngoing(IFX_void_t);
#else
   #define TAPI_OS_RebootNotifierRegister()   /* empty */
   #define TAPI_OS_RebootNotifierUnRegister() /* empty */
   #define TAPI_OS_isRebootOngoing()          (IFX_FALSE)
#endif

/* ==========================================================================*/
/* Other OS specific parts which are not generalized                         */
/* ========================================================================= */
#ifdef VXWORKS
   #include "drv_tapi_vxworks.h"
#endif

#ifdef WINDOWS
   #include "drv_tapi_win.h"
#endif

#ifdef __cplusplus
   }
#endif

#endif /* _DRV_TAPI_OSMAP_H */
