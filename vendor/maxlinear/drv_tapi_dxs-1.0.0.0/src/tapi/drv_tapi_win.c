/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_win.c
   Date: 2006-01-10
   This file contains the implementation of High-Level TAPI Driver,
   Windows simulation part

   The implementation mainly includes the registration part, using which
   the low-level drivers register themselves. During the registration
   the data structures are initialised,appropriate device nodes are
   created and the registered with the kernel.
*/

#ifdef WIN32

/* ============================= */
/* Includes                      */
/* ============================= */
#include <process.h>    /* _beginthread, _endthread */
#include "drv_tapi_config.h"
#include "drv_tapi.h"
#include "drv_tapi_api.h"
#include "drv_tapi_errno.h"
#include "drv_tapi_ioctl.h"
#include "drv_tapi_cid.h"

/* ============================================================================
   Local Macros & Definitions
   ========================================================================= */

#ifdef TAPI_STATIC
#undef TAPI_STATIC
#endif

#ifdef TAPI_DEBUG
#define TAPI_STATIC
#else
#define TAPI_STATIC   static
#endif

#define MAX_TAPI_FDS 50

/* ============================= */
/* Global Declarations           */
/* ============================= */

struct tq_struct
{
   void (*routine)(void *);   /* function to call */
   void *data;       /* argument to function */
};

typedef struct _filep {
   int major;
   int minor;
   TAPI_DEV* pTapiDev;
   TAPI_CHANNEL* pTapiCh;
}filep;

static filep* fps[MAX_TAPI_FDS];

/** Version of the HL-LL Interface API */
char tapiIfVersion[10] = "1.0.0.1";
unsigned int g_nMajor = 0;
static TAPI_OS_mutex_t semOpen;
static int bTickTimer = 0;

/* ============================= */
/* Local Functions               */
/* ============================= */
int ifx_tapi_open (const IFX_char_t *pName, IFX_int32_t *filp);
int ifx_tapi_ioctl (IFX_int32_t *filp, unsigned int nCmd, unsigned long nArgument);
int ifx_tapi_release (IFX_int32_t *filp);

#ifdef ENABLE_TRACE
extern IFX_uint32_t TAPI_debug_level;
#endif /* ENABLE_TRACE */

static BUFFERPOOL *pIFX_TAPI_Tq = IFX_NULL;

/* ============================= */
/* Local function definition     */
/* ============================= */

int TAPI_contextGet (IFX_TAPI_DRV_CTX_t* pDrvCtx, const char* pName,
                     int *dev, int *ch, int* nMinor)
{
   int ret;

   ret = sscanf(pName, "/dev/svip%d", nMinor);
   if (ret != -1)
   {
      *dev = (*nMinor / pDrvCtx->minorBase) - 1;
      *ch   = *nMinor % pDrvCtx->minorBase;
   }
   else
   {
      *dev = 0;
      *ch   = 0;
      *nMinor = 0;
   }
   return IFX_SUCCESS;
}

/**
   Open the device.

   At the first time:
   - Initialize the high-level TAPI device structure
   - Call the low-level function to initialise the low-level device structure
   - Initialize the high-level TAPI channel structure
   - Call the low-level function to initialise the low-level channel structure

   \param inode pointer to the inode
   \param filp pointer to the file descriptor

   \return
   0 - if no error,
   otherwise error code
*/
int ifx_tapi_open (const IFX_char_t *pName, IFX_int32_t *filp)
{
   IFX_TAPI_DRV_CTX_t      *pDrvCtx    = IFX_NULL;
   TAPI_DEV                *pTapiDev   = IFX_NULL;
   TAPI_CHANNEL            *pTapiCh    = IFX_NULL;
   IFX_TAPI_LL_CH_t        *pLLChDev   = IFX_NULL;
   IFX_int32_t             nDev, nCh;
   filep* fp;
   IFX_int32_t majorNum, nMinor;
   int i;

   majorNum = g_nMajor;
   TAPI_OS_MutexGet (&semOpen);
   /* Get the index for the device driver context based on major number */
   pDrvCtx = IFX_TAPI_DeviceDriverContextGet (majorNum);
   if (pDrvCtx == IFX_NULL)
      goto OPEN_ERROR;

   /**\todo protect against concurrent access */

   /* get device / channel number */
   TAPI_contextGet (pDrvCtx, pName, &nDev, &nCh, &nMinor);
   fp = TAPI_OS_Malloc (sizeof(filep));
   memset (fp, 0, sizeof(filep));
   fp->major = majorNum;
   i = 0;
   while ((fps[i]!= IFX_NULL) && (i < MAX_TAPI_FDS))
   {
      ++i;
   }
   fps[i] = fp;

#ifdef TAPI_ONE_DEVNODE
   if (nMinor > 255 || nMinor < 0)
      nMinor = 0;
#else
   /* for multi dev nodes the nDevNum starts at 0, otherwise there
    * is an offset of 1 */
   nDev -= 1;
#endif /* TAPI_ONE_DEVNODE */
   fp->minor = nMinor;
   /* check the device number */
   if (nDev >= pDrvCtx->maxDevs)
   {
      TRACE(TAPI_DRV, DBG_LEVEL_HIGH,
           ("TAPI_DRV: max. device number exceed\n"));
      goto OPEN_ERROR;
   }

   /* check the channel number */
   if (nCh > pDrvCtx->maxChannels)
   {
      TRACE(TAPI_DRV, DBG_LEVEL_HIGH,
          ("TAPI_DRV: max. channel number exceed\n"));
      goto OPEN_ERROR;
   }
#if 0
   moved  ???
   /* check if this is the first open on this device */
   if ((pDrvCtx->pTapiDev == IFX_NULL) ||
       (pDrvCtx->pTapiDev[dev].bInitialized == IFX_FALSE))
   {
      if (IFX_TAPI_Create_Device (pDrvCtx, dev) != TAPI_statusOk)
         goto OPEN_ERROR;
      if (pIFX_TAPI_Tq == IFX_NULL)
      {
         pIFX_TAPI_Tq = bufferPoolInit(sizeof(struct tq_struct),
                        IFX_TAPI_EVENT_FIFO_SIZE,0,0);
      }
   }
#endif
   /*  check for device node */
   if (nCh == 0)
      nDev = 0;

   pTapiDev = &(pDrvCtx->pTapiDev[nDev]);
   fp->pTapiDev = pTapiDev;

   if (nCh == 0)
   {
      /* Save the device pointer */
      fp->pTapiDev = pTapiDev;
      pLLChDev = pTapiDev->pLLDev;
   }
   else if (nCh <= pDrvCtx->maxChannels)
   {
      pTapiCh = pTapiDev->pChannel + nCh - 1;
      /* Save the channel pointer */
      fp->pTapiCh = &(pTapiDev->pChannel[nCh-1]);
      pLLChDev = pTapiCh->pLLChannel;
   }

   /* Call the Low level Device specific open routine */
   if (IFX_TAPI_PtrChk (pDrvCtx->Open))
   {
      IFX_int32_t retLL;

      retLL = pDrvCtx->Open (pLLChDev);

      if (!TAPI_SUCCESS(retLL))
      {
         TRACE(TAPI_DRV, DBG_LEVEL_HIGH,
               ("Open LL channel failed. [dev:%d ch:%d]\n", nDev, nCh));
         goto OPEN_ERROR;
      }
   }

   /* Increment the Usage counter */
   pTapiDev->nInUse++;

   if (IFX_NULL != pTapiCh)
      pTapiCh->nInUse++;

   TAPI_OS_MutexRelease (&semOpen);
   *filp = (IFX_int32_t)fp;

   return IFX_SUCCESS;

OPEN_ERROR:

   if (pDrvCtx != IFX_NULL && pDrvCtx->pTapiDev != IFX_NULL)
   {
      TAPI_OS_Free (pDrvCtx->pTapiDev);
   }
   TAPI_OS_MutexRelease (&semOpen);

   return (IFX_int32_t) -1;
} /* ifx_tapi_open() */

/**
    Release the device.

   \param inode pointer to the inode
   \param filp pointer to the file descriptor

   \return
   0 - on success
   \remarks
      This function gets called when a close is called on the device.
      It decrements the usage count, free the FIFOs
*/
int ifx_tapi_release (IFX_int32_t *filp)
{
   TAPI_DEV *pTapiDev = (TAPI_DEV*) ((filep*)(filp))->pTapiDev;
   TAPI_CHANNEL *pTapiCh = IFX_NULL;
   IFX_TAPI_DRV_CTX_t *pDrvCtx;
   IFX_TAPI_LL_CH_t*    pLLChDev = IFX_NULL;
   filep* fp;

   TRACE(TAPI_DRV, DBG_LEVEL_LOW, ("TAPI_DRV: close\n\r"));

   if (pTapiDev->nChannel != IFX_TAPI_DEVICE_CH_NUMBER)
   {
      pTapiCh = (TAPI_CHANNEL *) filp;
      pDrvCtx = (IFX_TAPI_DRV_CTX_t*) pTapiCh->pTapiDevice->pDevDrvCtx;
      pTapiCh->nInUse--;
      pTapiCh->pTapiDevice->nInUse--;
      pLLChDev = pTapiCh->pLLChannel;
   }
   else
   {
      pDrvCtx = (IFX_TAPI_DRV_CTX_t*) pTapiDev->pDevDrvCtx;
      pLLChDev = pTapiDev->pLLDev;
      pTapiDev->nInUse--;
      /* memory will be released when module is removed */
   }
   /* Call the Low-level Device specific release routine. */
   /* Not having such a function is not an error. */
   if (IFX_TAPI_PtrChk (pDrvCtx->Release))
   {
      IFX_int32_t retLL;

      retLL = pDrvCtx->Release (pLLChDev);

      if (!TAPI_SUCCESS (retLL))
      {
         TRACE(TAPI_DRV, DBG_LEVEL_HIGH,
               ("Release LL channel failed for ch: %d\n", pTapiDev->nChannel));
         return -1;
      }
   }
   /* decrement the use counters */
   if (IFX_NULL != pTapiCh)
   {
      if (pTapiCh->nInUse > 0)
      {
         pTapiCh->nInUse--;
      }
   }
   /* decrement the use counter */
   if (pTapiDev->nInUse > 0)
   {
      pTapiDev->nInUse--;
   }
   fp = (filep*) (filp);
   free (fp);
   return 0;
}


/*=================*/
/* Tapi iocontrols */
/*=================*/
/**
   Configuration / Control for the device.

   \param inode pointer to the inode
   \param filp pointer to the file descriptor
   \param nCmd function id's
   \param nArgument optional argument

   \return
   0 and positive values - success,
   negative value - ioctl failed
   \remarks
   This function does the following functions:
      - If the ioctl command is device specific, low-level driver's ioctl function
      - If the ioctl command is TAPI specific, it is handled at this level
*/
int ifx_tapi_ioctl (IFX_int32_t *filp,
                            unsigned int nCmd, unsigned long nArg)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = IFX_NULL;
   IFX_int32_t ret = 0;
   IFX_uint32_t nMinor = ((filep*)(filp))->minor;
   int majorNumber = ((filep*)(filp))->major;
   IFX_TAPI_ioctlCtx_t ctx;

   pDrvCtx = IFX_TAPI_DeviceDriverContextGet (majorNumber);
   if (pDrvCtx == IFX_NULL)
      return IFX_ERROR;

   /* get the ioctl context: channel, device etc. */
   ret = TAPI_ioctlContextGet (pDrvCtx, nMinor, nCmd, nArg, IFX_FALSE, &ctx);
   if (IFX_SUCCESS == ret)
   {
      ret = TAPI_Ioctl (&ctx);
   }

   TAPI_ioctlContextPut (nArg, &ctx);

   return ret;
}

/* ============================= */
/* Timer abstraction             */
/* ============================= */
#ifdef TAPI_HAVE_TIMERS
struct Timer_ID_s
{
   TIMER_ENTRY pTimerEntry;
   IFX_boolean_t bPeriodical;
   IFX_uint32_t Periodical_Time;
   IFX_ulong_t nArgument;
   /* Timer thread */
   IFXOS_ThreadCtrl_t TimerThread;
   /* Event to wait timer stop call */
   IFXOS_event_t TapiTimerStopEvent;
   /* locking the access to the timer event is needed
      because the event is deleted and initialized to clear the event */
   IFXOS_lock_t LockTimerEvent;
};

/**
   Timer thread function for processing user space timers

   \param  pThread  thread handler

   \return

   \remarks
   Thread started for each timer, and can be stopped using
   timer stop event linked to this thread.
*/
static IFX_int32_t TAPI_Timer_thread (TAPI_OS_ThreadParams_t *pThread)
{
   struct Timer_ID_s *pTimerData = (struct Timer_ID_s *)pThread->nArg1;
   IFX_int32_t ret;

   if (pTimerData == IFX_NULL)
      return IFX_FALSE;

   if (pTimerData->Periodical_Time == 0)
   {
      pTimerData->pTimerEntry (pTimerData, pTimerData->nArgument);
      return IFX_SUCCESS;
   }
   while (IFX_TRUE)
   {
      TAPI_OS_EventWait (&pTimerData->TapiTimerStopEvent,
                         pTimerData->Periodical_Time, &ret);
      if (ret == 0)
      {
         /* stop timer event received, exit thread */
         break;
      }
      /* Enter call back function */
      pTimerData->pTimerEntry (pTimerData, pTimerData->nArgument);
      if (!pTimerData->bPeriodical)
      {
         /* Oneshot timer expired, exit thread */
         break;
      }
   }
   return IFX_SUCCESS;
}

/**
   Function create a timer.

   \param pTimerEntry - Function pointer to the call back function
   \param nArgument   - pointer to TAPI channel structure

   \return
   Timer_ID    - pointer to internal timer structure
   \remarks
   Initialize a task queue which will be scheduled once a timer interrupt occurs
   to execute the appropriate operation in a process context, process in which
   semaphores ... are allowed.
   Please notice that this task has to run under the keventd process, in which it
   can be executed thousands of times within a single timer tick.
*/
Timer_ID TAPI_Create_Timer(TIMER_ENTRY pTimerEntry, IFX_ulong_t nArgument)
{
   struct Timer_ID_s *pTimerData;

   pTimerData = (struct Timer_ID_s*)TAPI_OS_Malloc(sizeof (*pTimerData));
   if (pTimerData == IFX_NULL)
      return IFX_NULL;

   TAPI_OS_LockInit(&pTimerData->LockTimerEvent);
   TAPI_OS_LockGet (&pTimerData->LockTimerEvent);
   TAPI_OS_EventInit (&pTimerData->TapiTimerStopEvent);
   TAPI_OS_LockRelease (&pTimerData->LockTimerEvent);
   /* set function to be called after timer expires */
   pTimerData->pTimerEntry = pTimerEntry;
   pTimerData->nArgument = nArgument;

   return (Timer_ID)pTimerData;
}

/**
   Function set and starts a timer with a specific time. It can be choose if the
   timer starts periodically.

   \param Timer_ID      - pointer to internal timer structure
   \param nTime         - Time in ms
   \param bPeriodically - Starts the timer periodically or not
   \param bRestart      - Restart the timer or normal start

   \return
   Returns an error code: IFX_TRUE / IFX_FALSE
*/
IFX_boolean_t TAPI_SetTime_Timer(Timer_ID Timer, IFX_uint32_t nTime, IFX_boolean_t bPeriodically, IFX_boolean_t bRestart)
{
   struct Timer_ID_s *pTimerData = (struct Timer_ID_s *)Timer;

   if (pTimerData == IFX_NULL)
      return IFX_FALSE;

   pTimerData->TimerThread.bValid = IFX_FALSE;
   pTimerData->bPeriodical = bPeriodically;
   pTimerData->Periodical_Time = nTime;
   if (bRestart == IFX_TRUE)
   {
      TAPI_Stop_Timer (pTimerData);
   }

   TAPI_OS_LockGet (&pTimerData->LockTimerEvent);
   TAPI_OS_EventDelete (&pTimerData->TapiTimerStopEvent);
   TAPI_OS_EventInit (&pTimerData->TapiTimerStopEvent);
   TAPI_OS_LockRelease (&pTimerData->LockTimerEvent);

   TAPI_OS_ThreadInit (&pTimerData->TimerThread, "TAPItimer",
                       TAPI_Timer_thread, 0, TAPI_OS_THREAD_PRIO_HIGHEST,
                       (IFX_ulong_t)pTimerData, 0);
   return IFX_TRUE;
}

/**
   Function stop a timer.

   \param Timer_ID      - pointer to internal timer structure

   \return
   Returns an error code: IFX_TRUE / IFX_FALSE
*/
IFX_boolean_t TAPI_Stop_Timer(Timer_ID Timer)
{
   struct Timer_ID_s *pTimerData = (struct Timer_ID_s *)Timer;

   if (pTimerData != IFX_NULL)
   {
      /* Just send stop event, and exit timer thread */
      TAPI_OS_LockGet (&pTimerData->LockTimerEvent);
      TAPI_OS_EventWakeUp (&pTimerData->TapiTimerStopEvent);
      TAPI_OS_LockRelease (&pTimerData->LockTimerEvent);
   }

   return IFX_TRUE;
}

/**
   Function delete a timer.

   \param Timer_ID      - pointer to internal timer structure

   \return
   Returns an error code: IFX_TRUE / IFX_FALSE
*/
IFX_boolean_t TAPI_Delete_Timer(Timer_ID Timer)
{
   struct Timer_ID_s *pTimerData = (struct Timer_ID_s *)Timer;

   if (Timer == IFX_NULL)
      return IFX_FALSE;

   TAPI_Stop_Timer(Timer);

   if (pTimerData != IFX_NULL)
      TAPI_OS_LockDelete(&pTimerData->LockTimerEvent);

   /* free memory */
   TAPI_OS_Free (Timer);

   return IFX_TRUE;
}
#endif /* TAPI_HAVE_TIMERS */

int ifx_tapi_module_init(void)
{
   int i;

   printf("%s,(c) 2001-2014, Lantiq Deutschland GmbH\n\r", &TAPI_WHATVERSION[4]);

   for (i = 0; i < MAX_TAPI_FDS; ++i)
      fps[i] = IFX_NULL;

   TAPI_OS_MutexInit (&semOpen);
   return IFX_TAPI_Driver_Start();
}

/**
   Clean up the module if unloaded.

   \remarks
   Called by the kernel.
*/
void ifx_tapi_module_exit(void)
{
   int i;
   IFX_int32_t ret;

   bTickTimer = 0;
   /* Free the device data block */
   for (i = 0; i < TAPI_MAX_LL_DRIVERS; i++)
   {
      IFX_TAPI_DRV_CTX_t *pDrvCtx = gHLDrvCtx[i].pDrvCtx;
      TAPI_CHANNEL *pChannel;
      int nCh;

      if (pDrvCtx != IFX_NULL)
      {
         for (nCh = 0; nCh < pDrvCtx->maxChannels; nCh++)
         {
            pChannel = &(pDrvCtx->pTapiDev->pChannel[nCh]);
            if (pChannel != IFX_NULL)
               IFX_TAPI_EventDispatcher_Exit(pChannel);

         }
         /*TAPI_OS_Free (gHLDrvCtx[i].pDrvCtx);*/
      }
   }
   IFX_TAPI_Driver_Stop();
   ret = bufferPoolFree(pIFX_TAPI_Tq);
   if (ret != IFX_SUCCESS)
      TRACE(TAPI_DRV,DBG_LEVEL_HIGH,
         ("INFO: Free tq buffer error. \n\r"));
   for (i = 0; i < MAX_TAPI_FDS; ++i)
   {
      if (fps[i] != IFX_NULL)
      {
         TRACE(TAPI_DRV,DBG_LEVEL_HIGH,
            ("INFO: fd not released\n\r"));
         TAPI_OS_Free (fps[i]);
      }
   }
   TRACE(TAPI_DRV,DBG_LEVEL_NORMAL,("TAPI_DRV: cleanup successful\n\r"));
}

#endif /* #ifdef WIN32 */
