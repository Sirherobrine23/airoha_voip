/******************************************************************************

  Copyright 2001-2009 Infineon Technologies AG
  Copyright 2009-2015 Lantiq Deutschland GmbH
  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2019 Intel Corporation.
  Copyright 2021-2023 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_linux.c
   This file contains the implementation of High-Level TAPI Driver,
   Linux specific part.

   The implementation includes the following parts:
    -Registration part by which the low-level drivers register themselves.
    -Device node operations (open, close, ioctl, read, write, select)
     are done here.
    -Linux module support.
    -Linux /proc filesystem handlers.
    -Timer abstraction layer.
    -Deferring of a function call for later execution.
    -Export of High-Level TAPI function symbols.
*/

#ifdef LINUX

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_tapi_config.h"

#ifdef __KERNEL__
   #include <linux/kernel.h>
   #include <linux/version.h>
   #include <linux/compat.h>

   #ifdef TAPI_FEAT_PROCFS
      #include <linux/proc_fs.h>       /*proc-file system*/
      #include <linux/seq_file.h>
   #endif
   #include "linux/hrtimer.h"
   #include <linux/init.h>
   #include <linux/errno.h>
   #include <asm/byteorder.h>
   #include <asm/io.h>
   #include <linux/device.h>
#endif /* __KERNEL__ */

#ifdef MODULE
   #include <linux/module.h>
#endif /* MODULE */

#include "drv_tapi.h"
#include "drv_tapi_api.h"
#include "drv_tapi_errno.h"
#include "drv_tapi_ioctl.h"
#include "drv_tapi_ppd.h"
#include "drv_tapi_linux.h"
#include "drv_tapi_linux_procfs.h"


#ifndef IFXOS_USE_DEV_IO
   #include "drv_tapi_cid.h"
#endif /* IFXOS_USE_DEV_IO */

/* ================================== */
/* channel specific wrapper structure */
/* ================================== */
#ifndef IFXOS_USE_DEV_IO
struct TAPI_FD_PRIV_DATA
{
   /* ptr to tapi channel or device specific data */
   IFX_void_t *pTapiCtx;
   /* channel fifo number */
   IFX_TAPI_STREAM_t fifo_idx;
};

/* ============================= */
/* Local Functions               */
/* ============================= */


/* ============================= */
/* Extern variable declarations  */
/* ============================= */

/* TAPI's timers workqueue */
extern struct workqueue_struct *pTAPItimersWq;

/* ============================= */
/* Extern functions declarations  */
/* ============================= */

extern IFX_void_t proc_EntriesRemove(void);

/* ============================= */
/* Local variable definition     */
/* ============================= */

/* TAPI's deferred tasks (events) workqueue. Events and timers should use
 * separate workqueues, because events might be expecting triggering of
 * timers, which will not happen if a single queue is used for both. */
struct workqueue_struct *pTAPIeventsWq;

/* ============================= */
/* Local variables               */
/* ============================= */

struct class *pTAPI_Class;

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3,8,0))
   /* init struct for the RT workqueue to set the scheduling policy
      via TAPI_DeferWork */
   static IFX_TAPI_EXT_EVENT_PARAM_t tapi_wq_setscheduler_param;
#endif

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Open the device.

   At the first time:
   - Initialize the high-level TAPI device structure
   - Call the low-level function to initialise the low-level device structure
   - Initialize the high-level TAPI channel structure
   - Call the low-level function to initialise the low-level channel structure

   \param  inode        Pointer to the inode.
   \param  filp         Pointer to the file descriptor.

   \return
   0 - if no error,
   otherwise error code
*/
int ifx_tapi_open (struct inode *inode, struct file *filp)
{
   struct TAPI_FD_PRIV_DATA *pTapiPriv = IFX_NULL;
   TAPI_DEV *pTapiDev = IFX_NULL;
   IFX_TAPI_DRV_CTX_t *pDrvCtx = IFX_NULL;
#if !defined(TAPI_ONE_DEVNODE)
   TAPI_CHANNEL            *pTapiCh    = IFX_NULL;
#endif /* not TAPI_ONE_DEVNODE */
   IFX_uint32_t            nDev = 0,
                           nCh = 0;
   IFX_uint32_t            majorNum = 0;
   IFX_uint32_t            minorNum = 0;

   majorNum = MAJOR(inode->i_rdev);
   minorNum = MINOR(inode->i_rdev);
   TRACE(TAPI_DXS, DBG_LEVEL_LOW,
         ("ifxTAPI open %d/%d\n", majorNum, minorNum));

   /* Get the pointer to the device driver context based on the major number */
   pDrvCtx = IFX_TAPI_DeviceDriverContextGet(majorNum);

   if (pDrvCtx == IFX_NULL)
   {
      /* This should actually never happen because the file descriptors are
         registered in TAPI_OS_RegisterLLDrv after a driver context is known. */
      printk(KERN_INFO "tapi_open: error finding DrvCtx\n");
      return -ENODEV;
   }

#if !defined(TAPI_ONE_DEVNODE)
   if (pDrvCtx->maxDevs == 1)
   {
      /* Extended fd numbering scheme for single device only. */
      /* If only one device is supported allow more than 9 channel fd. */
      nDev = 0;
      nCh = (IFX_uint32_t)minorNum - pDrvCtx->minorBase;
   }
   else
   {
      /* Regular fd numbering scheme for multiple devices. */
      /* calculate the device number and channel number */
      nDev = ((IFX_uint32_t)minorNum / pDrvCtx->minorBase) - 1;
      nCh   = ((IFX_uint32_t)minorNum % pDrvCtx->minorBase);

      /* check the device number */
      if (nDev >= pDrvCtx->maxDevs)
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
              ("TAPI DXS: max. device number exceed\n"));
         return -ENODEV;
      }
   }

   /* check the channel number */
   if (nCh > pDrvCtx->maxChannels)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
          ("TAPI DXS: max. channel number exceed\n"));
      return -ENODEV;
   }
#endif /* not TAPI_ONE_DEVNODE */

   pTapiDev = pDrvCtx->pTapiDev + nDev;

   /* Allocate memory for a new TAPI context wrapper structure */
   pTapiPriv = (struct TAPI_FD_PRIV_DATA* )
               TAPI_OS_Malloc (sizeof (struct TAPI_FD_PRIV_DATA));

   if (pTapiPriv == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("Unable to allocate memory for TAPI private data. "
             "[dev:%d ch:%d]\n", nDev, nCh));
      return -ENOMEM;
   }

#if !defined(TAPI_ONE_DEVNODE)
   if (nCh == 0)
#endif /* not TAPI_ONE_DEVNODE */
   {
      pTapiPriv->pTapiCtx = pTapiDev;
   }
#if !defined(TAPI_ONE_DEVNODE)
   else
   {
      pTapiCh = pTapiDev->pChannel + nCh - 1;

      pTapiPriv->pTapiCtx = pTapiCh;
   }
#endif /* not TAPI_ONE_DEVNODE */

   pTapiPriv->fifo_idx = IFX_TAPI_STREAM_COD;
   filp->private_data = pTapiPriv;

   /* Call the Low level Device specific open routine */
   if (IFX_TAPI_PtrChk (pDrvCtx->Open))
   {
      IFX_int32_t retLL = pDrvCtx->Open (pTapiPriv->pTapiCtx);

      if (!TAPI_SUCCESS(retLL))
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("Open LL channel failed. [dev:%d ch:%d]\n", nDev, nCh));

         TAPI_OS_Free (pTapiPriv);

         filp->private_data = IFX_NULL;
         return -ENODEV;
      }
   }

   /* increment the use counter */
   pTapiDev->nInUse++;

#if !defined(TAPI_ONE_DEVNODE)
   /* increment the use counters */
   if (IFX_NULL != pTapiCh)
   {
      pTapiCh->nInUse++;
   }
#endif /* not TAPI_ONE_DEVNODE */

   return 0;
}


/**
   Close a file.

   This function gets called when a close is called on the file descriptor.
   Both types device and channel struct TAPI_FD_PRIV_DATAfile descriptors are handled here. The
   function decrements the usage count, and calls the LL release routine.

   \param  inode        Pointer to the inode.
   \param  filp         Pointer to the file descriptor.

   \return
   0 - if no error,
   otherwise error code
*/
int ifx_tapi_release(struct inode *inode, struct file *filp)
{
   /* nCh is the first field in both TAPI_DEV and TAPI_CHANNEL */
   struct TAPI_FD_PRIV_DATA *pTapiPriv = IFX_NULL;
   IFX_uint8_t          nCh = 0;
   IFX_TAPI_DRV_CTX_t  *pDrvCtx = IFX_NULL;
   TAPI_DEV            *pTapiDev = IFX_NULL;
   TAPI_CHANNEL        *pTapiCh = IFX_NULL;
   IFX_TAPI_LL_CH_t    *pLLChDev = IFX_NULL;

   if (IFX_NULL == filp->private_data)
   {
      /* device was already released */
      return -ENODEV;
   }

   pTapiPriv = (struct TAPI_FD_PRIV_DATA* )filp->private_data;
   nCh = ((TAPI_DEV *) pTapiPriv->pTapiCtx)->nChannel;

   TRACE(TAPI_DXS, DBG_LEVEL_LOW, ("ifxTAPI close %d/%d tapi-ch %d\n",
         MAJOR(inode->i_rdev), MINOR(inode->i_rdev), nCh));

   if (nCh != IFX_TAPI_DEVICE_CH_NUMBER)
   {
      /* closing channel file descriptor */
      pTapiCh = (TAPI_CHANNEL *) pTapiPriv->pTapiCtx;
      pTapiDev = pTapiCh->pTapiDevice;
      pLLChDev = pTapiCh->pLLChannel;
   }
   else
   {
      /* closing device file descriptor */
      pTapiDev = (TAPI_DEV *) pTapiPriv->pTapiCtx;
      pLLChDev = pTapiDev->pLLDev;
   }

   if ((IFX_NULL == pTapiDev) || (IFX_NULL == pTapiDev->pDevDrvCtx))
   {
      /* resource already removed, nothing to do for release */
      /* FIXME: the driver should not be unloaded while file descriptor is opened */
      return 0;
   }

   pDrvCtx = pTapiDev->pDevDrvCtx;

   /* Call the Low-level Device specific release routine. */
   /* Not having such a function is not an error. */
   if (IFX_TAPI_PtrChk (pDrvCtx->Release))
   {
      IFX_int32_t retLL;

      retLL = pDrvCtx->Release (pLLChDev);

      if (!TAPI_SUCCESS (retLL))
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("Release LL channel failed for ch: %d\n", nCh));
         return -EBUSY;
      }
   }

   /* decrement the use counters */
   if ((IFX_NULL != pTapiCh) && (pTapiCh->nInUse > 0))
   {
      pTapiCh->nInUse--;
   }

   /* decrement the use counter */
   if (pTapiDev->nInUse > 0)
   {
      pTapiDev->nInUse--;
   }

   /* free private data */
   TAPI_OS_Free(pTapiPriv);

   filp->private_data = IFX_NULL;

   return 0;
}


/**
   Executes the select for the channel fd

   \param  pTapiPriv    Pointer fd private data carrying the context.
   \param  pNode        node list.
   \param  pOpt         Optional argument, which contains needed information for
                        TAPI_OS_DrvSelectQueueAddTask.

   \return
   System event qualifier. Either 0 or TAPI_OS_SYSREAD

   \remarks
   This function needs operating system services, that are hidden by
   IFXOS macros.
*/
static IFX_int32_t TAPI_SelectCh (struct TAPI_FD_PRIV_DATA *pTapiPriv,
                                  TAPI_OS_drvSelectTable_t *pNode,
                                  TAPI_OS_drvSelectOSArg_t *pOpt)
{
   TAPI_CHANNEL *pTapiCh = (TAPI_CHANNEL* )pTapiPriv->pTapiCtx;
   IFX_int32_t   ret = 0;

   /* Register the voice channel waitqueues as wakeup source. */
   TAPI_OS_DrvSelectQueueAddTask(pOpt, &pTapiCh->semReadBlock.object, pNode);
   TAPI_OS_DrvSelectQueueAddTask(pOpt, &pTapiCh->wqRead, pNode);

   /* In Linux the wakeup is always to be used. */
   pTapiCh->nFlags |= CF_NEED_WAKEUP;

   return ret;
}


/**
   Poll implementation for the device.

   \param file pointer to the file descriptor
   \param wait pointer to the poll table

   \return status of the events occurred on the device which are
           0, TAPI_OS_SYSEXCEPT, TAPI_OS_SYSREAD, TAPI_OS_SYSWRITE
   \remarks
   This function does the following functions:
      - Put the event queue in the wait table and sleep if select
        is called on the device itself.
      - Put the read/write queue in the wait table and sleep if select
        is called on the channels
*/
IFX_uint32_t ifx_tapi_poll(struct file *filp, poll_table *wait)
{
   struct TAPI_FD_PRIV_DATA *pTapiPriv = (struct TAPI_FD_PRIV_DATA* )filp->private_data;
   TAPI_DEV *pTapiDev = IFX_NULL;
   IFX_uint16_t i;
#ifdef TAPI_ONE_DEVNODE
   IFX_uint16_t j;
   IFX_TAPI_DRV_CTX_t *pDrvCtx = IFX_NULL;
#endif /* TAPI_ONE_DEVNODE */
   IFX_int32_t ret = 0;

   /* check device ptr */
   if ((IFX_NULL == pTapiPriv) || (IFX_NULL == pTapiPriv->pTapiCtx))
   {
      /* resource removed */
      /* FIXME: the driver should not be unloaded while file descriptor is opened */
      return (IFX_uint32_t)-ENODEV;
   }

   pTapiDev  = (TAPI_DEV* )pTapiPriv->pTapiCtx;

   if (IFX_NULL == pTapiDev->pDevDrvCtx)
   {
      /* resource removed */
      /* FIXME: the driver should not be unloaded while file descriptor is opened */
      return (IFX_uint32_t)-ENODEV;
   }

#ifdef TAPI_ONE_DEVNODE
   /* get driver context in case select called on device file descriptor */
   if (pTapiDev->nChannel == IFX_TAPI_DEVICE_CH_NUMBER)
   {
      pDrvCtx = IFX_TAPI_DeviceDriverContextGet (
         pTapiDev->pDevDrvCtx->majorNumber);
   }

   if (pTapiDev->nChannel == IFX_TAPI_DEVICE_CH_NUMBER &&
       pDrvCtx != IFX_NULL)
   {
      for (j= 0;j < pDrvCtx->maxDevs; ++j)
      {
         pTapiDev = &pDrvCtx->pTapiDev[j];

         TAPI_OS_DrvSelectQueueAddTask (filp, &pTapiDev->wqEvent, wait);

         if (pTapiDev->pChannel == IFX_NULL)
            continue;

         for (i = 0; i < pTapiDev->nMaxChannel; i++)
         {
            TAPI_CHANNEL *pTapiCh = &(pTapiDev->pChannel[i]);

            TAPI_OS_DrvSelectQueueAddTask (filp, &pTapiCh->semReadBlock.object,
                                           wait);
            TAPI_OS_DrvSelectQueueAddTask (filp, &pTapiCh->wqRead, wait);

            if ((pTapiCh->bInitialized == IFX_TRUE) &&
                (IFX_TAPI_EventFifoEmpty(pTapiCh) == IFX_FALSE))
            {
               /* exception available so return action */
               ret |= TAPI_OS_SYSREAD;
               break;
            }
         }
      }
   }
   else
#endif /* TAPI_ONE_DEVNODE */
   {
      if (pTapiDev->nChannel == IFX_TAPI_DEVICE_CH_NUMBER)
      {
         /* Register the TAPI-event waitqueue as wakeup source. */
         TAPI_OS_DrvSelectQueueAddTask (filp, &pTapiDev->wqEvent, wait);

         /* Check if there is any TAPI-event on any of the TAPI channels and
            return if file operations are possible without blocking. */

         TAPI_ASSERT (pTapiDev->pChannel != IFX_NULL);

         for (i = 0; i < pTapiDev->nMaxChannel; i++)
         {
            TAPI_CHANNEL *pTapiCh = &(pTapiDev->pChannel[i]);

            TAPI_ASSERT(pTapiCh);

            if (pTapiCh != IFX_NULL &&
                (pTapiCh->bInitialized == IFX_TRUE) &&
                (IFX_TAPI_EventFifoEmpty(pTapiCh) == IFX_FALSE))
            {
               /* TAPI-event available so return action */
               ret |= TAPI_OS_SYSREAD;
               break;
            }
         }
      }
      else
      {
         TAPI_CHANNEL *pTapiCh = (TAPI_CHANNEL *)pTapiDev;
         pTapiDev  = pTapiCh->pTapiDevice;

         if (pTapiCh->nChannel < pTapiDev->nMaxChannel)
         {
            /* Check if any packet is available in any of the upstream fifos
               and report if file operations are possible without blocking. */
            ret |= TAPI_SelectCh (pTapiPriv,
                                  (TAPI_OS_drvSelectTable_t *)wait,
                                  (TAPI_OS_drvSelectOSArg_t *)filp);
         }
      }
   }

   return (IFX_uint32_t)ret;
}


#if (LINUX_VERSION_CODE <= KERNEL_VERSION(3,8,0))
/**
   Set the scheduling policy for the TAPIevent workqueue to RT scheduling

   \param  foo          unused.
*/
static IFX_void_t tapi_wq_setscheduler (IFX_int32_t foo)
{
   struct sched_param sched_params;

   TAPI_UNUSED(foo);

   sched_params.sched_priority = TAPI_OS_THREAD_PRIO_HIGH;
   sched_setscheduler(current, SCHED_FIFO, &sched_params);
}
#endif /* LINUX_VERSION_CODE */


static IFX_int32_t tapiClassCreate(void)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(6,4,0))
   pTAPI_Class = class_create(THIS_MODULE, "tapi_dxs");
#else
   pTAPI_Class = class_create("tapi_dxs");
#endif
   if (IS_ERR(pTAPI_Class))
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
         ("tapiClassCreate: unable to create tapi_dxs class\n"));
      return TAPI_statusErr;
   }
   return 0;
}


static IFX_void_t tapiClassRemove(void)
{
   class_destroy(pTAPI_Class);
}


/**
   Initialize the module.

   \return
   Error code or 0 on success

   \remarks
   Called by the kernel.
*/
int ifx_tapi_module_init(void)
{
   if (IFX_TAPI_Driver_Start() != IFX_SUCCESS)
   {
      printk (KERN_ERR "Driver start failed\n");
      return -1;
   }

#ifdef TAPI_FEAT_PROCFS
   proc_EntriesInstall();
#endif /* TAPI_FEAT_PROCFS */

   if (tapiClassCreate() != IFX_SUCCESS)
      return -1;


#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,8,0))
   /*
      For "Concurrency Managed Workqueue" use related flags to increase the
      priority.

      WQ_MEM_RECLAIM

      All wq which might be used in the memory reclaim paths _MUST_
      have this flag set. The wq is guaranteed to have at least one
      execution context regardless of memory pressure.

      WQ_HIGHPRI

      Work items of a highpri wq are queued to the highpri
      thread-pool of the target gcwq. Highpri thread-pools are
      served by worker threads with elevated nice level.

      Note that normal and highpri thread-pools don't interact with
      each other. Each maintain its separate pool of workers and
      implements concurrency management among its workers.
   */
   pTAPItimersWq = alloc_workqueue("TAPItimers", WQ_MEM_RECLAIM | WQ_HIGHPRI, 0);
   pTAPIeventsWq = alloc_workqueue("TAPIevents",
      WQ_MEM_RECLAIM | WQ_HIGHPRI, 0);
#else
   pTAPItimersWq = create_workqueue("TAPItimers");
   pTAPIeventsWq = create_workqueue("TAPIevents");
   TAPI_DeferWork((IFX_void_t *) tapi_wq_setscheduler,
                  (IFX_void_t *)&tapi_wq_setscheduler_param);
#endif

   return 0;
}


/**
   Clean up the module.

   \remarks
   Called by the kernel.
*/
void ifx_tapi_module_exit(void)
{
   int i = 0;

   printk(KERN_INFO "Removing TAPI part of Duslic standalone driver\n");

   /* Actually all LL drivers should have unregistered here. Being careful
      we force unregister of any drivers which may still be registered. */
   for (i = 0; i < TAPI_MAX_LL_DRIVERS; i++)
   {
      if (gHLDrvCtx [i].pDrvCtx != IFX_NULL)
      {
         IFX_TAPI_Unregister_LL_Drv (gHLDrvCtx [i].pDrvCtx->majorNumber);
      }
   }

#ifdef TAPI_FEAT_PROCFS
   proc_EntriesRemove();
#endif /* TAPI_FEAT_PROCFS */

   tapiClassRemove();

   /* as we are using work queues to schedule events from the interrupt
      context to the process context, we use work queues in case of
      Linux 2.6 they must be flushed on driver unload... */
   flush_workqueue(pTAPItimersWq);
   destroy_workqueue(pTAPItimersWq);
   flush_workqueue(pTAPIeventsWq);
   destroy_workqueue(pTAPIeventsWq);

   IFX_TAPI_Driver_Stop();

   TRACE(TAPI_DXS, DBG_LEVEL_NORMAL, ("TAPI DXS: cleanup successful\n"));
}
#endif /* IFXOS_USE_DEV_IO */


/* ============================= */
/* Defer work to process context */
/* ============================= */

static IFX_void_t Deferred_Worker (struct work_struct *pWork)
{
   IFX_TAPI_EXT_EVENT_PARAM_t *pEvParam = (IFX_TAPI_EXT_EVENT_PARAM_t *) pWork;
   pEvParam->pFunc(pEvParam);
}

/**
   Defer work to process context

   \param  pFunc        Pointer to function to be called.
   \param  pParam       Parameter passed to the function.
                        Attention: this must be a valid structure of
                        type IFX_TAPI_EXT_EVENT_PARAM_t as we need to
                        do some casting for Linux.

   \return TAPI_statusOk or TAPI_statusErr in case of an error.
*/
IFX_int32_t TAPI_DeferWork (IFX_void_t *pFunc, IFX_void_t *pParam)
{
   IFX_int32_t ret = TAPI_statusOk;
   IFX_TAPI_EXT_EVENT_PARAM_t *pEvParam = (IFX_TAPI_EXT_EVENT_PARAM_t *) pParam;

   struct work_struct         *pTapiWs;

   pTapiWs = (struct work_struct *) &pEvParam->tapiWs;
   pEvParam->pFunc = (IFX_void_t *)pFunc;

   INIT_WORK(pTapiWs, Deferred_Worker);

   if (queue_work (pTAPIeventsWq, pTapiWs) == 0)
   {
      ret = TAPI_statusWorkFail;
   }

   return ret;
}

#endif /* LINUX */
