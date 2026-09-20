#ifdef LINUX
#ifdef __KERNEL__

#include <ifx_types.h>

#include <linux/version.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0))
   #include <linux/sched.h>
#else
   #include <uapi/linux/sched/types.h>
   #include <linux/sched/types.h>
   #include <linux/sched/signal.h>
#endif

#include <drv_tapi_osmap.h>

/**
   Modify own thread priority.

   \param  newPriority  New thread priority.

   \return
   - IFX_SUCCESS priority changed.
   - IFX_ERROR priority not changed.
*/
IFX_int32_t TAPI_OS_ThreadPriorityModify(IFX_uint32_t newPriority)
{
   IFX_int32_t ret = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,9,0))
   struct sched_param sched_params = { .sched_priority = newPriority };
   ret = sched_setscheduler(current, SCHED_FIFO, &sched_params);
#else
   /*  Starting from kernel 5.9.0 the RT thread scheduling has been revised
    *  and the idea of setting specific 'nice' priorities for RT threads
    *  has been dropped in favour of the SCHED_DEADLINE concept. Anyway, even
    *  switching to RT policy SCHED_FIFO with fixed 'sched_prioirty=50' parameter,
    *  what this call does, increases thread scheduling policy over the SCHED_NORMAL.
    *  And this is good enough for all the voice use cases */
   sched_set_fifo(current);
#endif

   if (ret < 0)
      printk(KERN_ERR "Failed to set the thread priority to %d ret = %d\n", 
             newPriority, ret);

   return (ret < 0) ? IFX_ERROR : IFX_SUCCESS;
}


/**
   stop a kernel thread. Called by the removing instance

   \param  pThrCntrl    Pointer to a thread control struct.
   \param  pMutex       Pointer to mutex to unblock the thread to be killed.
*/
IFX_void_t TAPI_OS_ThreadKill(IFXOS_ThreadCtrl_t *pThrCntrl,
                              TAPI_OS_lock_t *pLock)
{
   if ((pThrCntrl) &&
       (IFXOS_THREAD_INIT_VALID(pThrCntrl) == IFX_TRUE) &&
       (pThrCntrl->thrParams.bRunning == 1))
   {
      /* signal the thread routine to shutdown */
      pThrCntrl->thrParams.bShutDown = IFX_TRUE;
      mb();

      /* Wake the process so that is able to see the shutdown flag and
         terminate itself. */
      if (pLock != IFX_NULL)
         TAPI_OS_LockRelease(pLock);

      wait_for_completion (&pThrCntrl->thrCompletion);

      pThrCntrl->bValid = IFX_FALSE;
   }
}

#endif /* __KERNEL__ */
#endif /* LINUX */
