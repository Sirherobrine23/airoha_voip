/******************************************************************************

  Copyright 2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#ifdef LINUX
#ifdef __KERNEL__

#include <ifx_types.h>
#include <drv_tapi.h> /* IFX_TAPI_HL_DRV_CTX_t */
#include "drv_tapi_ll_interface.h" /* IFX_TAPI_DRV_CTX_t */

#include <drv_tapi_linux.h>
#include <drv_tapi_linux_ioctl.h>

#include <linux/device.h>


extern struct class *pTAPI_Class;

extern int ifx_tapi_open(struct inode *inode, struct file *filp);
extern int ifx_tapi_release(struct inode *inode, struct file *filp);
extern long ifx_tapi_ioctl(struct file *filp, unsigned int nCmd,
                           unsigned long nArgument);

/** The driver callbacks which will be registered with the kernel*/
static struct file_operations tapi_fops;

/**
   Register the low level driver with the operating system.

   This function will be called when the low-level driver is added to the
   system. Any OS specific registration or set-up should be done here.

   For Linux kernels with devfs support device nodes will be created here.
   The number of nodes created depend on the "one device node" or "multiple
   device node" configuration.
   For Linux kernels without devfs support the character driver is registered.

   \param  pLLDrvCtx    Pointer to device driver context created by LL-driver.
   \param  pHLDrvCtx    Pointer to high-level driver context.

   \return
   TAPI_statusOk

   \remarks
   For Linux the device nodes need to be registered and then the driver
   itself is registered with the kernel.
*/
IFX_return_t TAPI_OS_RegisterLLDrv(IFX_TAPI_DRV_CTX_t *pLLDrvCtx,
                                   IFX_TAPI_HL_DRV_CTX_t *pHLDrvCtx)
{
   IFX_int32_t ret = 0;
   IFX_uint16_t majorNumber;
   IFX_uint16_t minorNumber;
   IFX_uint16_t maxDevices,
                maxChannels;
   IFX_uint16_t nDevIdx,
                nChIdx;
   IFX_char_t sDevName[15];
   IFX_uint16_t nTapiFdIdx = 0;
   dev_t       dev;

   if (tapi_fops.unlocked_ioctl == IFX_NULL)
   {
#ifdef MODULE
      tapi_fops.owner = THIS_MODULE;
#endif
      tapi_fops.poll = ifx_tapi_poll;
      tapi_fops.unlocked_ioctl = ifx_tapi_ioctl;
#ifdef CONFIG_COMPAT
      tapi_fops.compat_ioctl = ifx_tapi_compat_ioctl,
#endif
      tapi_fops.open =     ifx_tapi_open;
      tapi_fops.release =  ifx_tapi_release;
   }

   /* copy registration info from Low level driver */
   majorNumber = pLLDrvCtx->majorNumber;

#ifdef TAPI_ONE_DEVNODE
   /* Single device node interface. */
   maxDevices = 1;
   maxChannels = 0;
   /* First minor number. */
   minorNumber = 0;
   /* Number of consecutive minor numbers needed. */
   pHLDrvCtx->nDevNrCount = 1;

#else /* !defined (TAPI_ONE_DEVNODE) */
   /* Multiple device node interface. */
   maxDevices  = pLLDrvCtx->maxDevs;
   maxChannels = pLLDrvCtx->maxChannels;

   /* Allow more than 10 channels only for a single supported device. When
      supporting multiple devices limit the channel number to one digit.
      More than 10 is not possible with our numbering plan in which the decade
      indicates the device number. */
   if ((maxDevices > 1) && (maxChannels > 9))
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
         ("TAPI_DRV (major:%d): The channel resources limited up to 9, "
         "due to device entry naming limitation.\n",
         majorNumber));
      maxChannels = 9;
   }

   /* Number of consecutive minor numbers needed. */
   if (maxDevices > 1)
   {
      pHLDrvCtx->nDevNrCount = maxDevices * 10;
   }
   else
   {
      /* Number of consecutive minor numbers needed. */
      pHLDrvCtx->nDevNrCount = 1 /*device*/ + maxChannels;
   }
#endif /* not TAPI_ONE_DEVNODE */

   /* limit devNodeName to 8 characters */
   sprintf (pHLDrvCtx->registeredDrvName, "mxl_tapi (%.8s)",
            pLLDrvCtx->devNodeName);

   /* Reserve the device number range. */
   if (majorNumber)
   {
      /* Register a major defined at module load time with a range of minors. */
      dev = MKDEV(majorNumber, 0);
      ret = register_chrdev_region(dev,
                        pHLDrvCtx->nDevNrCount,
                        pHLDrvCtx->registeredDrvName);
   }
   else
   {
      /* Request a dynamic major with a range of minors. */
      ret = alloc_chrdev_region(&dev,
                        pLLDrvCtx->minorBase,
                        pHLDrvCtx->nDevNrCount,
                        pHLDrvCtx->registeredDrvName);
      if (0 == ret)
      {
         pLLDrvCtx->majorNumber = majorNumber = MAJOR(dev);
      }
   }

   if (ret < 0)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
           ("TAPI_OS_RegisterLLDrv: unable to register chrdev number range"
            " using major number %d with %d minors\n\r",
            majorNumber, pHLDrvCtx->nDevNrCount));
      return TAPI_statusErr;
   }

   for (nDevIdx = 0; nDevIdx < maxDevices; nDevIdx++)
   {
#if defined (TAPI_ONE_DEVNODE)
      minorNumber = 0;
#else /* TAPI_ONE_DEVNODE */
      minorNumber = pLLDrvCtx->minorBase * (nDevIdx + 1);
#endif /* TAPI_ONE_DEVNODE */

      /* add character device to Linux */
      cdev_init(&(pLLDrvCtx->pTapiDev[nDevIdx].cdev), &tapi_fops);
      pLLDrvCtx->pTapiDev[nDevIdx].cdev.owner = THIS_MODULE;

      ret = cdev_add(&(pLLDrvCtx->pTapiDev[nDevIdx].cdev),
                     MKDEV(majorNumber, minorNumber),
                     1 /*device */ + maxChannels);
      if (ret != 0)
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("TAPI: unable to add chrdev %s\n\r", pLLDrvCtx->devNodeName));

         cdev_del (&(pLLDrvCtx->pTapiDev[nDevIdx].cdev));
         return TAPI_statusErr;
      }

      for (nChIdx = 0; nChIdx <= maxChannels; nChIdx++)
      {
#if defined(TAPI_ONE_DEVNODE)
         minorNumber = 0;
         /* limit devNodeName to 8 characters */
         snprintf (sDevName, sizeof(sDevName), "%.8s",
            pLLDrvCtx->devNodeName);
#else /* TAPI_ONE_DEVNODE */
         /* First device have index 1 */
         minorNumber = pLLDrvCtx->minorBase * (nDevIdx + 1);
         minorNumber += nChIdx;
         /* limit devNodeName to 8 characters */
         snprintf (sDevName, sizeof(sDevName), "%.8s%d",
            pLLDrvCtx->devNodeName, minorNumber);
#endif /* TAPI_ONE_DEVNODE */

         if (nTapiFdIdx >= TAPI_MAX_DEVFS_HANDLES)
         {
            TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                 ("TAPI DXS: Stopped character device creation on '%s'. "
                  "Reached maximal number(nTapiFdIdx=%d) of file "
                  "descriptors supported by TAPI\n",
                  sDevName, nTapiFdIdx));
            /* terminate node creation */
            nDevIdx = maxDevices;
            break;
         }

         pHLDrvCtx->pLL_Device[nTapiFdIdx] =
            device_create(pTAPI_Class, IFX_NULL,
                          MKDEV(majorNumber, minorNumber), IFX_NULL,
                          "%s", (IFX_char_t*)sDevName);

         if (IS_ERR(pHLDrvCtx->pLL_Device[nTapiFdIdx]))
         {
            TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                 ("IFX_TAPI_Register_LL_Drv: unable to create class device "
                  "'%s'\n\r", sDevName));
            return TAPI_statusErr;
         }

         nTapiFdIdx++;
      }
   }

   return TAPI_statusOk;
}


/**
   UnRegister the low level driver from the system.

   This function will be called when the low-level driver is removed from the
   system. Any OS specific deregistration should be done here.

   For Linux kernels with devfs support device nodes will be unregistered here.
   The number of nodes unregistered depend on the "one device node" or "multiple
   device node" configuration.
   For Linux kernels without devfs support the character driver is unregistered.

   \param  pLLDrvCtx    Pointer to device driver context created by LL-driver.
   \param  pHLDrvCtx    Pointer to high-level driver context.

   \return
   TAPI_statusErr on error, otherwise TAPI_statusOk
*/
IFX_return_t TAPI_OS_UnregisterLLDrv(const IFX_TAPI_DRV_CTX_t* pLLDrvCtx,
                                     IFX_TAPI_HL_DRV_CTX_t* pHLDrvCtx)
{
   int i;
   IFX_uint16_t maxDevices, nDevIdx;

   for (i=0; i < TAPI_MAX_DEVFS_HANDLES; ++i)
   {
      if (pHLDrvCtx->pLL_Device[i])
      {
         device_destroy(pTAPI_Class, pHLDrvCtx->pLL_Device[i]->devt);
         pHLDrvCtx->pLL_Device[i] = IFX_NULL;
      }
   }

#ifdef TAPI_ONE_DEVNODE
   /* Single device node interface. */
   maxDevices = 1;
#else /* !defined (TAPI_ONE_DEVNODE) */
   /* Multiple device node interface. */
   maxDevices  = pLLDrvCtx->maxDevs;
#endif /* not TAPI_ONE_DEVNODE */

   for (nDevIdx = 0; nDevIdx < maxDevices; nDevIdx++)
   {
      /* remove character device */
      cdev_del (&(pLLDrvCtx->pTapiDev[nDevIdx].cdev));
   } /* for all devices */

   unregister_chrdev_region (MKDEV(pLLDrvCtx->majorNumber, 0),
                             pHLDrvCtx->nDevNrCount);

   return TAPI_statusOk;
}

#endif /* __KERNEL__ */
#endif /* LINUX */
