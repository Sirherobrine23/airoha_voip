/******************************************************************************

   Copyright 2006-2009 Infineon Technologies AG
   Copyright 2009-2015 Lantiq Deutschland GmbH
   Copyright 2015      Lantiq Beteiligungs-GmbH & Co.KG
   Copyright 2018-2019 Intel Corporation.
   Copyright 2021-2022 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_init.c
   Implements the creation and initialisation of the TAPI driver framework.

   This driver is loaded before all others low-level drivers are loaded.
   The low-level drivers which are loaded register with this driver and pass
   a so called driver context structure. This structure contains function
   pointers and variables for the services that the low-level driver offers.
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_tapi.h"
#include "drv_tapi_errno.h"
#include "drv_tapi_version.h"
#include "drv_tapi_cid.h"
#include "drv_tapi_ppd.h"
#include "drv_tapi_linux_procfs_phone_detection.h"

#include <drv_config_defines.h>

#include "drv_tapi_debug_buffer.h"

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

#ifndef _MKSTR_1
#define _MKSTR_1(x)    #x
#define _MKSTR(x)      _MKSTR_1(x)
#endif

/** driver version as string */
#define DRV_TAPI_VER_STR         _MKSTR(DRV_TAPI_VER_MAJOR) "." \
                                 _MKSTR(DRV_TAPI_VER_MINOR) "." \
                                 _MKSTR(DRV_TAPI_VER_STEP)  "." \
                                 _MKSTR(DRV_TAPI_VER_TYPE)

/** low-level API version string */
#define DRV_TAPI_LL_IF_VER_STR   _MKSTR(LL_IF_MAJORSTEP)    "."   \
                                 _MKSTR(LL_IF_MINORSTEP)    "."   \
                                 _MKSTR(LL_IF_VERSIONSTEP)  "."   \
                                 _MKSTR(LL_IF_VERS_TYPE)

/** driver version, what string */
#define DRV_TAPI_WHAT_STR "@(#)TAPI, Version " DRV_TAPI_VER_STR

/* ============================= */
/* Global variable definition    */
/* ============================= */

/* Array to register the driver context of the low-level drivers. */
IFX_TAPI_HL_DRV_CTX_t gHLDrvCtx[TAPI_MAX_LL_DRIVERS];

/** what string support, driver version string */
const IFX_char_t TAPI_DRV_WHATVERSION[] = DRV_TAPI_WHAT_STR;

#ifdef HAVE_CONFIG_H
   /** which configure options were set */
   const IFX_char_t DRV_TAPI_WHICHCONFIG[] = DRV_TAPI_CONFIGURE_STR;
#endif /* HAVE_CONFIG_H */

/* ============================= */
/* Local variable definition     */
/* ============================= */

static IFX_uint32_t nGlobalTapiID = 0;

/* ============================= */
/* Local function declaration    */
/* ============================= */

static IFX_int32_t ifx_tapi_DeviceCreate (IFX_TAPI_DRV_CTX_t *pDrvCtx);
static IFX_int32_t ifx_tapi_DevicePrepare (TAPI_DEV* pTapiDev,
                                           IFX_uint32_t dev_num,
                                           IFX_TAPI_DRV_CTX_t *pDrvCtx);
static IFX_int32_t ifx_tapi_DeviceInit    (TAPI_DEV* pTapiDev);
static IFX_int32_t ifx_tapi_DeviceExit    (TAPI_DEV* pTapiDev);
static IFX_int32_t ifx_tapi_DeviceDelete  (IFX_TAPI_DRV_CTX_t *pDrvCtx);

static IFX_int32_t ifx_tapi_ChannelCreate (TAPI_DEV *pTapiDev);
static IFX_int32_t ifx_tapi_ChannelPrepare(TAPI_CHANNEL *pTapiCh,
                                           IFX_uint32_t ch_num,
                                           TAPI_DEV* pTapiDev);
static IFX_int32_t ifx_tapi_ChannelInit   (TAPI_CHANNEL *pTapiCh);
static IFX_int32_t ifx_tapi_ChannelExit   (TAPI_CHANNEL *pTapiCh);

static IFX_int32_t ifx_tapi_InitCh_Unprot (TAPI_CHANNEL *pChannel);
static IFX_void_t  ifx_tapi_ExitCh        (TAPI_CHANNEL *pChannel);

/* ============================= */
/* Local function definition     */
/* ============================= */


/* ============================= */
/* Global function definition    */
/* ============================= */

#ifdef EVENT_LOGGER_DEBUG
/**
   get TAPI device pointer by global TAPI device ID.

   \param nDevID  unique TAPI device ID [0,1,...]

   \return
      Pointer in to the TAPI device, otherwise returns IFX_NULL.

   \remarks
      temporary solution for event logger, pending new concept
*/
TAPI_DEV *TAPI_DeviceGetByID(IFX_uint32_t nDevID)
{
   IFX_uint16_t i = 0, j = 0;

   if (nDevID >= nGlobalTapiID)
      return IFX_NULL;

   for (i=0; i < TAPI_MAX_LL_DRIVERS; i++)
   {
      if (IFX_NULL == gHLDrvCtx[i].pDrvCtx)
         continue;

      if (IFX_NULL == gHLDrvCtx[i].pDrvCtx->pTapiDev)
         continue;

      for (j=0; j < gHLDrvCtx[i].pDrvCtx->maxDevs; j++)
      {
         if (nDevID == gHLDrvCtx[i].pDrvCtx->pTapiDev[j].nDevID)
            return (gHLDrvCtx[i].pDrvCtx->pTapiDev + j);
      }
   }

   return IFX_NULL;

}
#endif /* EVENT_LOGGER_DEBUG */

/**
   TAPI initialisation upon starting the driver.

   \return
   Returns IFX_ERROR in case of an error, otherwise returns IFX_SUCCESS.
*/
IFX_int32_t IFX_TAPI_Driver_Start(void)
{
   IFX_uint8_t i = 0;
   IFX_int32_t ret = IFX_SUCCESS;

   /* Mark all slots as free in the global driver context array. */
   for (i=0; i < TAPI_MAX_LL_DRIVERS; i++)
   {
      /* Setting the pointer to NULL marks this slot as free. */
      gHLDrvCtx[i].pDrvCtx = IFX_NULL;
   }

   /* The bufferpool for voice packets is only created when a LL-driver
      registers that needs it. So when none of the registered LL drivers
      needs the pool it is not created. */

#ifdef TAPI_FEAT_TONETABLE
   /* Configure the tonetable with predefined tones. */
   ret = TAPI_Phone_Tone_Predef_Config ();
#endif /* TAPI_FEAT_TONETABLE */

   if (TAPI_SUCCESS(ret))
   {
      /* Initialise the TAPI event handler */
      ret = IFX_TAPI_Event_On_Driver_Start();
   }

   return ret;
}

/**
   TAPI cleanup upon stopping the driver.
*/
IFX_void_t IFX_TAPI_Driver_Stop(void)
{
   IFX_uint8_t i;

   /* Actually all LL drivers should have unregistered here. Being careful
      we force unregister of any drivers which may still be registered. */
   for (i = 0; i < TAPI_MAX_LL_DRIVERS; i++)
   {
      if (gHLDrvCtx[i].pDrvCtx != IFX_NULL)
      {
         IFX_TAPI_Unregister_LL_Drv (gHLDrvCtx[i].pDrvCtx->majorNumber);
      }
   }

   IFX_TAPI_Event_On_Driver_Stop();
}

/**
   Create all TAPI devices and link them with the LL-devices.

   Allocate all TAPI device and channel structs if not already existing and
   prepare them by linking the structs to each other and the LL-driver
   equivalents.
   - Allocate all high-level TAPI device structures and store them in the
     device driver context.
   - Call prepare to link the structures to each other and the LL structs.
   - Allocate the high-level TAPI channel structures and store them in the
     high-level TAPI device structure.

   \param  pDrvCtx      Pointer to device driver context.

   \return
   Returns TAPI_statusOk in case of success, otherwise error code.
*/
static IFX_int32_t ifx_tapi_DeviceCreate (IFX_TAPI_DRV_CTX_t *pDrvCtx)
{
   /* Allocate an array of TAPI device structures and store it in the low-level
      driver context. The array is created only the first time and kept until
      the corresponding LL-driver is unregistered. Only the devices that are
      addressed will be initialised. All others will stay uninitialized. */
   if (pDrvCtx->pTapiDev == IFX_NULL)
   {
      TAPI_DEV *pTapiDev = IFX_NULL;
      IFX_uint16_t nDev;

      /* This is the number of devices that the LL driver supports. */
      IFX_uint16_t maxDevices = pDrvCtx->maxDevs;

      pDrvCtx->pTapiDev = (TAPI_DEV *)
                          TAPI_OS_Malloc(sizeof(TAPI_DEV) * maxDevices);
      if (pDrvCtx->pTapiDev == IFX_NULL)
      {
         /* Just a return since we do not have a device or channel yet where
          * an error could be stored. */
         /* errmsg: Device structure allocation failed */
         return TAPI_statusDevAlloc;
      }
      /* the device structs are set to zero in the prepare function */

      /* prepare all tapi devices just created */
      for (nDev=0; nDev < maxDevices; nDev++)
      {
         pTapiDev = &(pDrvCtx->pTapiDev[nDev]);

         /* Prepare the TAPI device */
         if (ifx_tapi_DevicePrepare (pTapiDev, nDev, pDrvCtx) != TAPI_statusOk)
         {
            RETURN_DEVSTATUS (TAPI_statusInitFail, 0);
         }

         /* Create and prepare the TAPI channels of this device */
         if (ifx_tapi_ChannelCreate (pTapiDev) != TAPI_statusOk)
         {
            RETURN_DEVSTATUS (TAPI_statusInitFail, 0);
         }

         TAPI_LogInit(pTapiDev);
      }
   }

   return TAPI_statusOk;
}


/**
   TAPI device structure preparation

   One time preparation of the TAPI device structure. This sets all members
   needed for file descriptor handling and links the HL- to the LL-struct.

   \param  pTapiDev     Pointer to TAPI device structure.
   \param  dev_num      Number of the device in the driver.
   \param  pDrvCtx      Pointer to device driver context.

   \return
   Returns IFX_SUCCESS or IFX_ERROR.
*/
static IFX_int32_t ifx_tapi_DevicePrepare (TAPI_DEV* pTapiDev,
                                           IFX_uint32_t dev_num,
                                           IFX_TAPI_DRV_CTX_t *pDrvCtx)
{
   /* set the device structure to zero */
   memset (pTapiDev, 0x00, sizeof(TAPI_DEV));

   /* set the channel-number to the magic number that indicates a device */
   pTapiDev->nChannel = IFX_TAPI_DEVICE_CH_NUMBER;

   /* set the device-number */
   pTapiDev->nDev = dev_num;

   /* assign a unique TAPI device ID */
   pTapiDev->nDevID = nGlobalTapiID++;

   /* store the device driver context */
   pTapiDev->pDevDrvCtx = pDrvCtx;

   /* Clear the init-status flags. */
   pTapiDev->nInitStatusFlags = TAPI_INITSTATUS_UNINITIALISED;

   /* Mutex to block concurrent IOCTLs on the device */
   TAPI_OS_MutexInit (&pTapiDev->semTapiDevSingleIoctlAccess);

   /* needed for select on the device fds */
   TAPI_OS_DrvSelectQueueInit (&pTapiDev->wqEvent);
   pTapiDev->bNeedWakeup = IFX_TRUE;
   TAPI_OS_MutexInit (&pTapiDev->semTapiDevSingleIoctlAccess);

   /* call the LL driver prepare function */
   if (pDrvCtx->Prepare_Dev != IFX_NULL)
   {
      /* prepare the LL device and store the device pointer */
      pTapiDev->pLLDev = pDrvCtx->Prepare_Dev (pTapiDev, dev_num);
   }

   pTapiDev->error.nDev = dev_num;

   /* if LL function is not available or LL init failed return an error */
   if (pTapiDev->pLLDev == IFX_NULL)
   {
      /* cleanup the device driver context */
      pTapiDev->pDevDrvCtx = IFX_NULL;
      RETURN_DEVSTATUS (TAPI_statusInitFail, 0);
   }

   return TAPI_statusOk;
}


/**
   TAPI device structure initialization

   Initializes all fifos, semaphores, memory and all other members.
   The TAPI channels are members of the device and also initialised here.

   \param  pTapiDev     Pointer to TAPI device structure.

   \return
   Returns IFX_SUCCESS or IFX_ERROR.
*/
IFX_int32_t ifx_tapi_DeviceInit (TAPI_DEV* pTapiDev)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pTapiDev->pDevDrvCtx;
   IFX_uint16_t nCh;
   IFX_int32_t ret = TAPI_statusErr;

   if (pTapiDev->bInitialized == IFX_TRUE)
   {
      /* already initialised - nothing to do */
      return TAPI_statusOk;
   }

   /* set the number of TAPI channels in this device */
   pTapiDev->nMaxChannel = pDrvCtx->maxChannels;

   /* set the resource count to maximum in case LL does not report anything */
   pTapiDev->nResource.AlmCount =
   pTapiDev->nResource.PcmCount =
   pTapiDev->nResource.DTMFGCount =
   pTapiDev->nResource.DTMFRCount =
   pTapiDev->nResource.FSKGCount =
   pTapiDev->nResource.ToneCount = pTapiDev->nMaxChannel;

   if (IFX_TAPI_PtrChk (pDrvCtx->Init_Dev))
   {
      ret = pDrvCtx->Init_Dev (pTapiDev->pLLDev);
   }
   /* Initialise all TAPI channels of this device */
   for (nCh=0; TAPI_SUCCESS(ret) && (nCh < pTapiDev->nMaxChannel); nCh++)
   {
      TAPI_CHANNEL *pTapiCh = &(pTapiDev->pChannel[nCh]);

      ret = ifx_tapi_ChannelInit (pTapiCh);
   }
   if (TAPI_SUCCESS(ret))
   {
      /* All the device members are now initialised */
      pTapiDev->bInitialized = IFX_TRUE;
   }

   RETURN_DEVSTATUS(ret, 0);
}


/**
   TAPI device structure shutdown

   Shutdown all resources in the TAPI device structure. Frees all needed
   fifos, semaphores, memory.
   The TAPI channel struct array and TAPI device struct array are kept until
   the LL driver context is unregistered.

   \param  pTapiDev     Pointer to TAPI device structure.

   \return
   Returns IFX_SUCCESS or IFX_ERROR.
*/
static IFX_int32_t ifx_tapi_DeviceExit (TAPI_DEV* pTapiDev)
{
   TAPI_CHANNEL *pTapiCh = IFX_NULL;
   IFX_uint16_t i;

   if (pTapiDev == IFX_NULL)
   {
      return TAPI_statusErr;
   }

   if (pTapiDev->bInitialized == IFX_FALSE)
   {
      /* already shutdown - nothing to do */
      return TAPI_statusOk;
   }

   /* Do channel specific cleanup */
   for (i = 0; i < pTapiDev->nMaxChannel; i++)
   {
      pTapiCh = &(pTapiDev->pChannel[i]);

      ifx_tapi_ExitCh(pTapiCh);
      ifx_tapi_ChannelExit(pTapiCh);
   }

#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
   /* The Phone Detection feature installs entries for each device dynamically.
      It is necessary to remove them here. */
   TAPI_ProcPpdDeviceEntryRemove(pTapiDev);
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE */

   /* placeholder - call a LL driver device exit function if needed */

   /* mark the device struct as unused and free */
   pTapiDev->bInitialized = IFX_FALSE;

   return TAPI_statusOk;
}


/**
   Delete all TAPI device with all their channels.

   Frees the TAPI device and channel resources. This function also frees the
   memory of the TAPI channel structs and all TAPI device structs.

   \param  pDrvCtx      Pointer to device driver context.

   \return
   Returns TAPI_statusOk in case of success, otherwise error code.

   \remarks
   CAUTION: this function deletes _all_ TAPI devices not just a single one.
*/
static IFX_int32_t ifx_tapi_DeviceDelete (IFX_TAPI_DRV_CTX_t *pDrvCtx)
{
   IFX_uint16_t nDev;

   if (pDrvCtx->pTapiDev == IFX_NULL)
   {
      return TAPI_statusOk;
   }

   for (nDev=0;  nDev < pDrvCtx->maxDevs; nDev++)
   {
      TAPI_DEV* pTapiDev = pDrvCtx->pTapiDev + nDev;

      TAPI_OS_MutexDelete (&pTapiDev->semTapiDevSingleIoctlAccess);

      /* cleanup the resources of the device (including all channels) */
      ifx_tapi_DeviceExit(pTapiDev);

      if (pTapiDev->pChannel != IFX_NULL)
      {
         /* this deletes all TAPI channels of the device at once */
         TAPI_OS_Free (pTapiDev->pChannel);
         pTapiDev->pChannel = IFX_NULL;
      }

      TAPI_OS_DrvSelectQueueDelete (&pTapiDev->wqEvent);
      TAPI_OS_MutexDelete (&pTapiDev->semTapiDevSingleIoctlAccess);

      /* to forgot the device driver context */
      pTapiDev->pDevDrvCtx = IFX_NULL;

      TAPI_LogClose(pTapiDev);
   }

   /* deallocate the entire array of TAPI devices stored in the LL-driver ctx */
   if (pDrvCtx->pTapiDev != IFX_NULL)
   {
      TAPI_OS_Free (pDrvCtx->pTapiDev);
      pDrvCtx->pTapiDev = IFX_NULL;
   }

   return TAPI_statusOk;
}


/**
   Create all TAPI channels in one device.

   Allocates the high-level TAPI channel structures and stores them in the
   high-level TAPI device structure. Then link the channels to the LL driver
   equivalents.

   \param  pTapiDev     Pointer to TAPI device structure.

   \return
   Returns TAPI_statusOk in case of success, otherwise error code.
*/
IFX_int32_t ifx_tapi_ChannelCreate(TAPI_DEV *pTapiDev)
{
   const IFX_TAPI_DRV_CTX_t *pDrvCtx = pTapiDev->pDevDrvCtx;

   /* This is the number of channels that the LL driver supports for each dev.*/
   IFX_uint16_t maxChannels = pDrvCtx->maxChannels;

   if (pTapiDev->pChannel == IFX_NULL)
   {
      IFX_uint16_t nCh = 0;

      /* Allocate an array of TAPI channel structures and store it in the
         device. */
      pTapiDev->pChannel =
         (TAPI_CHANNEL *) TAPI_OS_Malloc (sizeof(TAPI_CHANNEL) * maxChannels);
      if (pTapiDev->pChannel == IFX_NULL)
      {
         /* errmsg: Channel structure allocation failed */
         RETURN_DEVSTATUS (TAPI_statusChAlloc, 0);
      }
      /* the channel structs are set to zero in the prepare function */

      /* Prepare all TAPI channels of this device */
      for (nCh = 0; nCh < maxChannels; nCh++)
      {
         TAPI_CHANNEL *pTapiCh = &(pTapiDev->pChannel[nCh]);

         if (ifx_tapi_ChannelPrepare (pTapiCh, nCh, pTapiDev) != TAPI_statusOk)
         {
            /* On error deallocate the TAPI channel structures again */
            TAPI_OS_Free (pTapiDev->pChannel);
            pTapiDev->pChannel = IFX_NULL;
            RETURN_DEVSTATUS (TAPI_statusInitFail, 0);
         }
      }
   }

   return TAPI_statusOk;
}


/**
   TAPI channel structure preparation

   One time preparation of the TAPI channel structures. This sets all members
   needed for file descriptor handling.

   \param  pTapiCh      Pointer to TAPI channel structure to be initialised.
   \param  ch_num       Number of the channel in the device.
   \param  pTapiDev     Pointer to TAPI device structure.
*/
static IFX_int32_t ifx_tapi_ChannelPrepare (TAPI_CHANNEL *pTapiCh,
                                            IFX_uint32_t ch_num,
                                            TAPI_DEV* pTapiDev)
{
   IFX_TAPI_DRV_CTX_t      *pDrvCtx;

   /* set the channel structure to zero */
   memset (pTapiCh, 0x00, sizeof(TAPI_CHANNEL));

   /* set the channel-number */
   pTapiCh->nChannel = ch_num;

   /* store the pointer to the TAPI device structure */
   pTapiCh->pTapiDevice = pTapiDev;

   /* call the LL driver prepare function */
   pDrvCtx = pTapiDev->pDevDrvCtx;
   if (pDrvCtx->Prepare_Ch != IFX_NULL)
   {
      /* prepare the LL channel and store the channel pointer */
      pTapiCh->pLLChannel = pDrvCtx->Prepare_Ch (pTapiCh,
                                                 pTapiDev->pLLDev,
                                                 pTapiCh->nChannel);
   }
   /* if LL function is not available or LL init failed return an error */
   if (pTapiCh->pLLChannel == IFX_NULL)
   {
      RETURN_DEVSTATUS (TAPI_statusInitFail, 0);
   }

   /* select wait queue and semaphore for reading/writing data */
   TAPI_OS_EventInit (&pTapiCh->semReadBlock);

   TAPI_OS_DrvSelectQueueInit (&pTapiCh->wqRead);
   TAPI_OS_DrvSelectQueueInit (&pTapiCh->wqWrite);

   return TAPI_statusOk;
}


/**
   One time TAPI channel structure initialization

   Initializes all needed fifos, semaphores and sets all needed members.

   \param  pTapiCh      Pointer to TAPI channel structure.

   \return
   Returns IFX_ERROR in case of an error, otherwise returns IFX_SUCCESS.
*/
static IFX_int32_t ifx_tapi_ChannelInit (TAPI_CHANNEL *pTapiCh)
{
   IFX_TAPI_DRV_CTX_t      *pDrvCtx = pTapiCh->pTapiDevice->pDevDrvCtx;
   TAPI_DEV                *pTapiDev = pTapiCh->pTapiDevice;

   /* Initialise the high level channel access mutex */
   TAPI_OS_MutexInit (&pTapiCh->semTapiChSingleIoctlAccess);
   TAPI_OS_MutexInit (&pTapiCh->semTapiChDataLock);

   /* Initialise the event dispatcher */
   if (IFX_TAPI_EventDispatcher_Init(pTapiCh) != TAPI_statusOk)
   {
      RETURN_DEVSTATUS (TAPI_statusInitFail, 0);
   }

   if (pDrvCtx->Init_Ch != IFX_NULL)
   {
      /* intialise the LL channel */
      IFX_int32_t retLL = pDrvCtx->Init_Ch (pTapiCh->pLLChannel);
      if (!TAPI_SUCCESS(retLL))
      {
         /* LL init failed -> return an error */
         RETURN_DEVSTATUS (TAPI_statusInitFail, retLL);
      }
   }
   else
   {
      /* LL function is not available -> return an error */
      RETURN_DEVSTATUS (TAPI_statusLLNotSupp, 0);
   }

   pTapiCh->TapiOpControlData.nLineMode = IFX_TAPI_LINE_FEED_DISABLED;
   pTapiCh->TapiOpControlData.bHookState = IFX_FALSE;
   pTapiCh->TapiOpControlData.nPolarity = 0x00;
   pTapiCh->TapiOpControlData.nBatterySw = 0x00;
   pTapiCh->TapiOpControlData.bEmergencyShutdown = IFX_FALSE;

   /* this channel is now in use */
   pTapiCh->nInUse++;

   return TAPI_statusOk;
}

/**
   TAPI channel structure shutdown

   Shutdown all resources in the TAPI channel structure.
   Frees all needed fifos, event queues, memory.

   \param  pTapiCh      Pointer to TAPI channel structure.

   \return
   Returns IFX_ERROR in case of an error, otherwise returns IFX_SUCCESS.
*/
static IFX_int32_t ifx_tapi_ChannelExit (TAPI_CHANNEL *pTapiCh)
{
   TAPI_ASSERT(pTapiCh != IFX_NULL);

   if (pTapiCh == IFX_NULL)
      return TAPI_statusErr;

   /* placeholder - call the LL driver channel exit function */

   /* Exit the event dispatcher */
   if (IFX_TAPI_EventDispatcher_Exit(pTapiCh) != TAPI_statusOk)
   {
      return TAPI_statusErr;
   }

   /* Delete queues for waiting in select */
   TAPI_OS_DrvSelectQueueDelete (&pTapiCh->wqRead);
   TAPI_OS_DrvSelectQueueDelete (&pTapiCh->wqWrite);

   /* Delete the high level channel access mutex */
   TAPI_OS_MutexDelete (&pTapiCh->semTapiChSingleIoctlAccess);
   TAPI_OS_MutexDelete (&pTapiCh->semTapiChDataLock);

   return TAPI_statusOk;
}

/**
   Init one TAPI channel, unprotected version.

   \param  pChannel     Pointer to TAPI_CHANNEL structure.

   \return:
    returncode: IFX_SUCCESS  -> init successful
                IFX_ERROR    -> init failed

   \remarks This function is not protected. The global protection is done
   in the calling function with TAPI_OS_MutexGet (&pChannel->semTapiChDataLock)
*/
static IFX_int32_t ifx_tapi_InitCh_Unprot (TAPI_CHANNEL *pChannel)
{
   if (pChannel->bInitialized == IFX_TRUE)
   {
      /* channel already configured */
      return TAPI_statusOk;
   }

#ifdef TAPI_FEAT_DIAL
   /* check if channel has the required analog module */
   if (pChannel->nChannel < pChannel->pTapiDevice->nResource.AlmCount)
   {
      /* ask all TAPI modules to initialise themselves */
      if (IFX_TAPI_Dial_Initialise_Unprot (pChannel) != TAPI_statusOk)
      {
         RETURN_STATUS (TAPI_statusInitDialFail, 0);
      }
   }
#endif /* TAPI_FEAT_DIAL */

#ifdef TAPI_FEAT_TONEENGINE
   /* Initialise tone only on channels which include tone capabilities. */
   if (pChannel->nChannel < pChannel->pTapiDevice->nResource.ToneCount)
   {
      if (IFX_TAPI_Tone_Initialise_Unprot (pChannel) != TAPI_statusOk)
      {
         RETURN_STATUS (TAPI_statusInitToneFail, 0);
      }
   }
#endif /* TAPI_FEAT_TONEENGINE */

#ifdef TAPI_FEAT_CID
   if (IFX_TAPI_CID_Initialise_Unprot (pChannel) != TAPI_statusOk)
   {
      RETURN_STATUS (TAPI_statusInitCIDFail, 0);
   }
#endif /* TAPI_FEAT_CID */

   /* check if channel has the required analog module */
   if (pChannel->nChannel < pChannel->pTapiDevice->nResource.AlmCount)
   {
#ifdef TAPI_FEAT_RINGENGINE
      /* Init ring after CID because ring will ask for CID status during init */
      if (IFX_TAPI_Ring_Initialise_Unprot (pChannel) != TAPI_statusOk)
      {
         RETURN_STATUS (TAPI_statusInitRingFail, 0);
      }
#endif /* TAPI_FEAT_RINGENGINE */
#ifdef TAPI_FEAT_METERING
      if (IFX_TAPI_Meter_Initialise_Unprot (pChannel) != TAPI_statusOk)
      {
         RETURN_STATUS (TAPI_statusInitMeterFail, 0);
      }
#endif /* TAPI_FEAT_METERING */
   }

#ifdef TAPI_FEAT_PHONE_DETECTION
   if (IFX_TAPI_PPD_Initialise_Unprot (pChannel) != TAPI_statusOk)
   {
      RETURN_STATUS (TAPI_statusInitPpdFail, 0);
   }
#endif /* TAPI_FEAT_PHONE_DETECTION */

   pChannel->bInitialized = IFX_TRUE;

   return TAPI_statusOk;
}

/**
   Cleanup one TAPI channel

   \param  pChannel     Pointer to TAPI_CHANNEL structure.
*/
static IFX_void_t ifx_tapi_ExitCh (TAPI_CHANNEL *pChannel)
{
#ifdef TAPI_FEAT_PHONE_DETECTION
   IFX_TAPI_PPD_Cleanup(pChannel);
#endif /* TAPI_FEAT_PHONE_DETECTION */
#ifdef TAPI_FEAT_METERING
   IFX_TAPI_Meter_Cleanup(pChannel);
#endif /* TAPI_FEAT_METERING */
#ifdef TAPI_FEAT_RINGENGINE
   IFX_TAPI_Ring_Cleanup(pChannel);
#endif /* TAPI_FEAT_RINGENGINE */
#ifdef TAPI_FEAT_CID
   IFX_TAPI_CID_Cleanup(pChannel);
#endif /* TAPI_FEAT_CID */
#ifdef TAPI_FEAT_TONEENGINE
   IFX_TAPI_Tone_Cleanup(pChannel);
#endif /* TAPI_FEAT_TONEENGINE */
#ifdef TAPI_FEAT_DIAL
   IFX_TAPI_Dial_Cleanup(pChannel);
#endif /* TAPI_FEAT_DIAL */
   pChannel->bInitialized = IFX_FALSE;
}


/**
   Set the resource count struct.

   This function is intended to be called by the LL driver as soon as it knows
   how many resources will be available. The values given will be stored and
   used by TAPI functions to check for needed resources before they try to
   use them.

   \param   pTapiDev    Pointer to a TAPI device context.
   \param   pResources  Pointer to struct with resource count.
*/
IFX_void_t IFX_TAPI_ReportResources (TAPI_DEV* pTapiDev,
                                     const IFX_TAPI_RESOURCE *pResources)
{

   if (!(IFX_TAPI_PtrChk (pTapiDev) && IFX_TAPI_PtrChk (pResources)))
      return ;

   pTapiDev->nResource = *pResources;
}

/**
   Returns the number of capabilities.

   \param   pTapiDev Handle to the TAPI device
   \param   pCap     Handle to the memory for result

   \return
      TAPI_statusOk - success
      TAPI_statusLLNotSupp - service not supported by low-level driver
*/
IFX_int32_t IFX_TAPI_Cap_Nr_Get (TAPI_DEV *pTapiDev,
   IFX_TAPI_CAP_NR_t *pCap)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pTapiDev->pDevDrvCtx;
   IFX_int32_t nCap;

   if (IFX_NULL == pCap)
      RETURN_DEVSTATUS (TAPI_statusParam, 0);

   if (!IFX_TAPI_PtrChk(pDrvCtx->CAP_Number_Get))
   {
      RETURN_DEVSTATUS (TAPI_statusLLNotSupp, 0);
   }

   nCap = pDrvCtx->CAP_Number_Get (pTapiDev->pLLDev);

#ifdef TAPI_ONE_DEVNODE
   pCap->nCap = nCap;
#else
   *pCap = nCap;
#endif /* TAPI_ONE_DEVNODE */

   return TAPI_statusOk;
}

/**
   Returns the list of capabilities.

   \param   pTapiDev Handle to the TAPI device
   \param   pCapList Pointer to struct with list and number of elements

   \return
      TAPI_statusOk - success
      TAPI_statusLLNotSupp - service not supported by low-level driver
*/
IFX_int32_t IFX_TAPI_Cap_List_Get (
                        TAPI_DEV *pTapiDev,
                        IFX_TAPI_CAP_LIST_t *pCapList)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pTapiDev->pDevDrvCtx;
   IFX_int32_t retLL;

   if (pCapList == IFX_NULL)
   {
      RETURN_DEVSTATUS (TAPI_statusParam, 0);
   }

   if (!IFX_TAPI_PtrChk(pDrvCtx->CAP_List_Get))
   {
      RETURN_DEVSTATUS (TAPI_statusLLNotSupp, 0);
   }

   retLL = pDrvCtx->CAP_List_Get(pTapiDev->pLLDev, pCapList);
   if (!TAPI_SUCCESS(retLL))
   {
      /* LL failed -> return an error */
      RETURN_DEVSTATUS (TAPI_statusLLFailed, retLL);
   }

   return TAPI_statusOk;
}

/**
   Returns the last error code occurred in the TAPI driver
   or the low level driver. It contains also a error stack
   for tracking down the origin of the error source.

   \param   pTapiDev Handle to the TAPI driver
   \param   pErr     Handle to the memory for result

   \return  TAPI_statusOk

   \remarks
      After calling this service the stack is reset.
*/
IFX_int32_t TAPI_Last_Err_Get (TAPI_DEV *pTapiDev,
   IFX_TAPI_Error_t *pErr)
{
   memcpy (pErr, &(pTapiDev->error), sizeof(IFX_TAPI_Error_t));

   /* clear the error now that we read it. */
   pTapiDev->error.nCnt = 0;
   pTapiDev->error.nCode = 0;

   return TAPI_statusOk;
}

/**
   Register the low-level driver.

   This function is called by the low-level driver when this is started.
   It does the following:
      - Checks that HL- and LL-driver versions are matching.
      - Stores the low-level device driver context in a global array
      - Call the OS specific driver registration function.

   \param  pLLDrvCtx    Pointer to device driver context which is provided by
                        the low-level driver. The driver context may only be
                        used from registration until it is unregistered. The
                        memory for the context is at all times owned by the
                        LL-driver who has to keep it available while it is
                        registered.
   \return
   - TAPI_statusOk - Success
   - TAPI_statusInitLL - Driver context pointer must not be NULL
   - TAPI_statusInitLLRegVersMismatch - Version mismatch of HL and LL driver
   - TAPI_statusRegLLExists - Major number already registered
   - TAPI_statusInitLLReg - No free slot to register the driver
*/
IFX_int32_t IFX_TAPI_Register_LL_Drv(IFX_TAPI_DRV_CTX_t *pLLDrvCtx)
{
   IFX_TAPI_HL_DRV_CTX_t* pHLDrvCtx = IFX_NULL;
   IFX_uint8_t i;
   IFX_size_t l;
   IFX_int32_t ret = TAPI_statusOk;

   /* Sanity check of the function parameter. */
   if (pLLDrvCtx == IFX_NULL)
   {
      return TAPI_statusInitLL;
   }

   l = strlen(pLLDrvCtx->hlLLInterfaceVersion);

   /* Make sure HL- and LL-driver interface versions are matching. */
   if ((l != strlen(DRV_TAPI_LL_IF_VER_STR)) ||
      strncmp (pLLDrvCtx->hlLLInterfaceVersion, DRV_TAPI_LL_IF_VER_STR, l))
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
         ("TAPI: ATTENTION - mismatch of HL-LL Interface\n"));
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
         ("TAPI: please check that drv_tapi and drv_%s driver match.\n",
            pLLDrvCtx->drvName));
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
         ("Version set in LL Driver = %s\n", pLLDrvCtx->hlLLInterfaceVersion));
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
         ("Version expected by HL   = %s\n", DRV_TAPI_LL_IF_VER_STR));
      return TAPI_statusInitLLRegVersMismatch;
   }

   /* Don't allow more than one device driver to use the same major number. */
   if (IFX_TAPI_DeviceDriverContextGet (pLLDrvCtx->majorNumber) != IFX_NULL)
   {
      /* Here somebody is already using the given major number. */
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("Cannot register LL-driver. "
            "LL-driver with this major number is already registered.\n"));
      return TAPI_statusRegLLExists;
   }

   /* Find a free index in gHLDrvCtx. */
   for (i = 0; i < TAPI_MAX_LL_DRIVERS; i++)
   {
      if (gHLDrvCtx[i].pDrvCtx == IFX_NULL)
      {
         /* use this index */
         pHLDrvCtx = &gHLDrvCtx[i];
         /* store the low-level driver context (now this index is in use) */
         pHLDrvCtx->pDrvCtx = pLLDrvCtx;
         break;
      }
   }

   /* Could not find free index. */
   if (pHLDrvCtx == IFX_NULL)
   {
      return TAPI_statusInitLLReg;
   }
   if (pLLDrvCtx->devNodeName == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("Wrong input arguments. "
            "(File: %s, line: %d)\n", __FILE__, __LINE__));
      return TAPI_statusLLDevNodeName;
   }

   /* Create TAPI devices for all devices that the LL driver supports. */
   ret = ifx_tapi_DeviceCreate (pLLDrvCtx);

   /* OS specific registration of the driver */
   if (TAPI_SUCCESS(ret))
   {
      ret = TAPI_OS_RegisterLLDrv (pLLDrvCtx, pHLDrvCtx);
   }

   return ret;
}

/**
   UnRegister the low-level driver

   This function is called by the low-level driver when this is stopped.
   It does the following:
      - Call the OS specific driver deregistration function.
      - Free the high-level TAPI device structures
      - Forget the low-device driver context

   \param  majorNumber  Device major number.

   \return IFX_ERROR on error, otherwise TAPI_statusOk
   - TAPI_statusOk - Success
   - TAPI_statusInitLLReg - Unregister failed: major number is not registered
*/
IFX_int32_t IFX_TAPI_Unregister_LL_Drv (IFX_int32_t majorNumber)
{
   IFX_TAPI_HL_DRV_CTX_t *pHLDrvCtx = IFX_NULL;
   IFX_TAPI_DRV_CTX_t    *pLLDrvCtx;
   IFX_uint8_t i;
   IFX_int32_t ret = TAPI_statusOk;

   TRACE(TAPI_DXS, DBG_LEVEL_NORMAL,
      ("INFO: Low level driver unregistering with major number %d\n",
      majorNumber));

   /* find registered driver and get the high-level driver context */
   for (i = 0; i < TAPI_MAX_LL_DRIVERS; i++)
   {
      if (gHLDrvCtx[i].pDrvCtx != IFX_NULL)
      {
         if (majorNumber == gHLDrvCtx [i].pDrvCtx->majorNumber)
         {
            pHLDrvCtx = &gHLDrvCtx [i];
         }
      }
   }
   /* not registered or no high-level driver context */
   if (pHLDrvCtx == IFX_NULL)
   {
      return TAPI_statusInitLLReg;
   }

   /* this is the corresponding low-level driver context */
   pLLDrvCtx = pHLDrvCtx->pDrvCtx;

   ret = TAPI_OS_UnregisterLLDrv (pLLDrvCtx, pHLDrvCtx);

   /* delete all devices in this driver context */
   ifx_tapi_DeviceDelete (pLLDrvCtx);

   /* forget the low-level driver context (now this index is free again) */
   pHLDrvCtx->pDrvCtx = IFX_NULL;

   return ret;
}

/**
   Returns the pointer to the low-level context

   Compares the major number with all the device driver's major numbers and
   if it matches, returns the low-level driver context.

   \param  Major        Device major number.

   \return
   If found it returns the pointer to the low-level context, otherwise IFX_NULL.
*/
IFX_TAPI_DRV_CTX_t* IFX_TAPI_DeviceDriverContextGet (IFX_int32_t Major)
{
   IFX_uint8_t i;

   for (i = 0; i < TAPI_MAX_LL_DRIVERS; i++)
   {
      if (gHLDrvCtx[i].pDrvCtx != IFX_NULL)
      {
         if (Major == gHLDrvCtx[i].pDrvCtx->majorNumber)
         {
            return gHLDrvCtx[i].pDrvCtx;
         }
      }
   }
   return IFX_NULL;
}


/**
   Start the TAPI for a specified device.

   \param  pTapiDev     Pointer to TAPI device structure.
   \param  pDevStartCfg Pointer to IFX_TAPI_DEV_START_CFG_t or IFX_NULL.
                        If the parameter is not needed IFX_NULL must be passed.

   \return
   Returns TAPI_statusOk in case of success, otherwise error code.
*/
IFX_int32_t IFX_TAPI_DeviceStart (TAPI_DEV *pTapiDev,
                                  IFX_TAPI_DEV_START_CFG_t const *pDevStartCfg)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pTapiDev->pDevDrvCtx;
   IFX_uint8_t nMode = IFX_TAPI_INIT_MODE_DEFAULT;
   IFX_int32_t retLL = TAPI_statusErr;
   IFX_uint16_t i = 0;

   IFX_int32_t ret = ifx_tapi_DeviceInit(pTapiDev);

   if (IFX_TAPI_PtrChk(pDrvCtx->FW_Init))
   {
      if (pDevStartCfg != IFX_NULL)
      {
         /* If parameter is given overwrite the default that is set above. */
         nMode = pDevStartCfg->nMode;
      }
      retLL = pDrvCtx->FW_Init(pTapiDev->pLLDev, nMode);
      if (!TAPI_SUCCESS(retLL))
      {
         ret = TAPI_statusInitFail;
      }
   }
   else
   {
      ret = TAPI_statusInitFail;
   }

   /* Initialize all TAPI channel members, if not already done. */
   for (i = 0; (ret == TAPI_statusOk) && (i < pTapiDev->nMaxChannel); i++)
   {
      TAPI_CHANNEL *pChannel = pTapiDev->pChannel + i;

      TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);

      ret = ifx_tapi_InitCh_Unprot (pChannel);

      TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);
   }

#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
   /* Create entry in proc fs - necessary to have possibility to enable/disable
      Phone Detection in channels. */
   if (TAPI_SUCCESS(ret))
   {
      ret = TAPI_ProcPpdDeviceEntryInstall(pTapiDev);
   }
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE */

   if (!TAPI_SUCCESS(ret))
   {
      /* In case of an error try to roll-back. */
      IFX_TAPI_DeviceStop(pTapiDev);
   }

   RETURN_DEVSTATUS(ret, retLL);
}


/**
   Stop the TAPI for a specified device.

   \param  pTapiDev     Pointer to TAPI device structure to be stopped.

   \return
   - TAPI_statusOk - Success
*/
IFX_int32_t IFX_TAPI_DeviceStop(TAPI_DEV *pTapiDev)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pTapiDev->pDevDrvCtx;
   IFX_int32_t ret = TAPI_statusOk;

   if (IFX_TAPI_PtrChk(pDrvCtx->Exit_Dev))
   {
      pDrvCtx->Exit_Dev(pTapiDev->pLLDev, IFX_TRUE /* with chip access */);

      /* if LL support is missing this is not executed */
      ret = ifx_tapi_DeviceExit(pTapiDev);
   }
   else
   {
      /* errmsg: Service is not supported by the low level driver*/
      ret = TAPI_statusLLNotSupp;
   }

   RETURN_DEVSTATUS(ret, 0);
}


/**
   Reset TAPI states

   This function is intended to be issued after an HW reset. It will reset
   all states that TAPI keeps about the device. During this function only
   SW structures will be reset; no HW access will be done.

   \param  pTapiDev     Pointer to TAPI device structure to be stopped.
*/
IFX_void_t IFX_TAPI_DeviceReset (TAPI_DEV *pTapiDev)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx;

   if (!IFX_TAPI_PtrChk (pTapiDev))
      return ;

   tapi_debug_buffer_add_user_entry("dxs #%d device reset", pTapiDev->nDev);

   pDrvCtx = pTapiDev->pDevDrvCtx;

   if (IFX_TAPI_PtrChk (pDrvCtx->Exit_Dev))
   {
      pDrvCtx->Exit_Dev (pTapiDev->pLLDev, IFX_FALSE /* without chip access */);

      /* if LL support is missing this is not executed */
      ifx_tapi_DeviceExit (pTapiDev);
   }
}


/**
   Legacy function to initialise the TAPI.

   This function is kept for the old IOCTL IFX_TAPI_CH_INIT. The IOCTL is
   channel specific but the init is for the entire device.

   \param  pTapiDev     Pointer to TAPI device structure.
   \param  pInit        Pointer to IFX_TAPI_CH_INIT_t structure.

   \return
   error code: IFX_SUCCESS  -> init successful
               IFX_ERROR    -> init not successful
*/
IFX_int32_t IFX_TAPI_Phone_Init(TAPI_DEV *pTapiDev,
   IFX_TAPI_CH_INIT_t const *pInit)
{
   IFX_TAPI_DRV_CTX_t  *pDrvCtx   = pTapiDev->pDevDrvCtx;
   IFX_int32_t          ret = TAPI_statusOk;

   if (IFX_TAPI_PtrChk (pDrvCtx->FW_Start))
   {
      IFX_void_t *pProc = (pInit == IFX_NULL) ? IFX_NULL : pInit->pProc;

      ret = pDrvCtx->FW_Start (pTapiDev->pLLDev, pProc);

      if (!TAPI_SUCCESS (ret))
      {
         /* errmsg: FW starting failed */
         RETURN_DEVSTATUS (TAPI_statusFWStart, ret);
      }
   }

   /* +++++ from here on the firmware is available +++++ */

   if (pTapiDev->bInitialized != IFX_TRUE)
   {
      IFX_TAPI_DEV_START_CFG_t nDevStartCfg;

      memset(&nDevStartCfg, 0x00, sizeof(nDevStartCfg));
      nDevStartCfg.nMode =
         (pInit == IFX_NULL) ? IFX_TAPI_INIT_MODE_DEFAULT : pInit->nMode;

      ret = IFX_TAPI_DeviceStart(pTapiDev, &nDevStartCfg);
   }

   if (TAPI_SUCCESS(ret) && IFX_TAPI_PtrChk (pDrvCtx->BBD_Dnld))
   {
      /* Do default BBD download for all channels*/
      IFX_void_t *pProc = IFX_NULL;  /* broadcast default BBD */
#ifdef TAPI_ONE_DEVNODE
      TAPI_CHANNEL *pChannel = pTapiDev->pChannel;
#endif /* TAPI_ONE_DEVNODE */

      if ((IFX_NULL != pInit) && (IFX_NULL != pInit->pProc))
      {
         if (pInit->ch >= pTapiDev->nMaxChannel)
         {
            RETURN_DEVSTATUS (TAPI_statusInvalidCh, 0);
         }

#ifdef TAPI_ONE_DEVNODE
         /* Do custom BBD download for a single channel */
         pChannel += pInit->ch;
#endif /* TAPI_ONE_DEVNODE */
         pProc = pInit->pProc;
      }

#ifdef TAPI_ONE_DEVNODE
      ret = pDrvCtx->BBD_Dnld (pChannel->pLLChannel, pProc);
#else
      ret = pDrvCtx->BBD_Dnld (pTapiDev->pLLDev, pProc);
#endif /* TAPI_ONE_DEVNODE */

      if (!TAPI_SUCCESS (ret))
      {
         /* errmsg: BBD downoading failed */
         RETURN_DEVSTATUS (TAPI_statusBbdDownload, ret);
      }
   }

   return ret;
}
