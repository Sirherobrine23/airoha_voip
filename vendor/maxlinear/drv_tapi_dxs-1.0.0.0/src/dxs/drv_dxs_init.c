/******************************************************************************

  Copyright 2014-2015 Lantiq Deutschland GmbH
  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017, 2020 Intel Corporation.
  Copyright 2021-2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_init.c
   This file implements functions for the Initialization of the device.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_access.h"
#include "drv_dxs_init.h"
#include "drv_dxs_dwld.h"
#include "drv_dxs_bbd.h"
#include "drv_dxs_irq.h"
#include "drv_dxs_version.h"
#include "drv_dxs_cid.h"
#include "drv_dxs_dtmf.h"
#include "drv_dxs_fifo.h"
#include "drv_dxs_mbx.h"
#include "drv_dxs_fw_cmd_sdd.h"

#include "../tapi/drv_tapi_api.h"
#include "../../drv_tapi_dxs_version.h"
#include "../tapi/drv_tapi_debug_buffer.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* maximum number of capabilities, used for allocating the array */
#define DXS_MAX_CAPS             12

#ifndef _MKSTR_1
#define _MKSTR_1(x)    #x
#define _MKSTR(x)      _MKSTR_1(x)
#endif

/** device driver version string */
#define DXS_DRV_VER_STR          _MKSTR(DXS_MAJORSTEP)    "."   \
                                 _MKSTR(DXS_MINORSTEP)    "."   \
                                 _MKSTR(DXS_VERSIONSTEP)  "."   \
                                 _MKSTR(DXS_VERS_TYPE)
/** TAPI LL interface version string */
#define DRV_LL_INTERFACE_VER_STR _MKSTR(LL_IF_MAJORSTEP)    "."      \
                                 _MKSTR(LL_IF_MINORSTEP)    "."      \
                                 _MKSTR(LL_IF_VERSIONSTEP)  "."      \
                                 _MKSTR(LL_IF_VERS_TYPE)

/** what compatible driver version */
#define DXS_DRV_WHAT_STR \
        "@(#)DXS device driver, version " DXS_DRV_VER_STR



/** Standalone TAPI for DXS device driver version string */
#define TAPI_DXS_DRV_VER_STR     _MKSTR(TAPI_DXS_MAJORSTEP)    "."   \
                                 _MKSTR(TAPI_DXS_MINORSTEP)    "."   \
                                 _MKSTR(TAPI_DXS_VERSIONSTEP)  "."   \
                                 _MKSTR(TAPI_DXS_VERS_TYPE)

/** what compatible driver version */
#define TAPI_DXS_DRV_WHAT_STR \
        "@(#)Standalone TAPI for DXS, version " TAPI_DXS_DRV_VER_STR

/* ========================================================================== */
/*                                 Defines                                    */
/* ========================================================================== */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,2,62))
   #ifndef SIZE_MAX
      #define SIZE_MAX ULONG_MAX
   #endif
#endif


/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* Variable that is used to set the initial debug trace level.
   In some OS implementations it can be overwritten when starting the driver
   by setting debug_level module parameter. */
#ifdef ENABLE_TRACE
   #ifdef DEBUG
      #define DXS_DBG_LEVEL_DEFAULT  DBG_LEVEL_LOW
   #else
      #define DXS_DBG_LEVEL_DEFAULT  DBG_LEVEL_HIGH
   #endif

   #ifdef DXS_USE_DEV_IO
      IFX_uint32_t DEVIO_debug_level = DXS_DBG_LEVEL_DEFAULT;
   #else
      /* Debug level variable to use when setting debug level via module parameter */
      IFX_uint32_t debug_level = DXS_DBG_LEVEL_DEFAULT;
   #endif

   /**
      Define trace output for whole Standalone TAPI for DXS driver
      (4) OFF:    no output
      (3) HIGH:   only important traces, as errors or some warnings
      (2) NORMAL: including traces from high and general proceedings and
                  possible problems
      (1) LOW:    all traces and low level traces as basic chip access
                  and interrupts, command data
      Traces can be completely switched off with the compiler switch
      ENABLE_TRACE set to 0
   */
   TAPI_CREATE_TRACE_GROUP(TAPI_DXS, DXS_DBG_LEVEL_DEFAULT);
#endif /* ENABLE_TRACE */

/** what string support, DXS driver version string */
const IFX_char_t DXS_DRV_WHATVERSION[] = DXS_DRV_WHAT_STR;
/** what string support, standalone TAPI for DXS driver version string */
const IFX_char_t TAPI_DXS_DRV_WHATVERSION[] = TAPI_DXS_DRV_WHAT_STR;
#ifdef HAVE_CONFIG_H
   /** which configure options were set */
   const IFX_char_t DRV_DXS_WHICHCONFIG[] = DRV_TAPI_CONFIGURE_STR;
#endif /* HAVE_CONFIG_H */

extern IFX_uint16_t major;
extern IFX_uint16_t minorBase;

/* Allowed type of DC/DC converter */
enum DXS_DcDcType nAllowedDcDcType;
/* String from commandline - overwritten by OS specific code. */
IFX_char_t *dcdc_type = "";


/* ========================================================================== */
/*                             Local  variables                               */
/* ========================================================================== */

/* static variable of the driver context struct */
static IFX_TAPI_DRV_CTX_t DrvCtx;
/* access to driver context from outside this file */
IFX_TAPI_DRV_CTX_t *pDrvCtx = &DrvCtx;
/* static array of device structs */
static DXS_DEVICE_t DxsDevices[DXS_MAX_DEVICES];


/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_void_t          dxs_ChipAccessExit (
                              DXS_DEVICE_t *pDev);

static IFX_TAPI_LL_DEV_t*  DXS_TAPI_LL_DevicePrepare (
                              TAPI_DEV *pTapiDev,
                              IFX_uint32_t devNum);

static IFX_int32_t         DXS_TAPI_LL_DeviceInit (
                              IFX_TAPI_LL_DEV_t* pLLDev);

static IFX_void_t          dxs_DeviceExit (
                              DXS_DEVICE_t *pDev,
                              IFX_boolean_t bChipAccess);

static IFX_void_t          DXS_TAPI_LL_DeviceExit (
                              IFX_TAPI_LL_DEV_t *pLLDev,
                              IFX_boolean_t bChipAccess);

static IFX_TAPI_LL_CH_t*   DXS_TAPI_LL_ChannelPrepare (
                              TAPI_CHANNEL *pTapiCh,
                              IFX_TAPI_LL_DEV_t *pLLDev,
                              IFX_uint32_t chNum);

static IFX_int32_t         DXS_TAPI_LL_ChannelInit (
                              IFX_TAPI_LL_CH_t *pLLCh);

static IFX_void_t          dxs_ChannelExit (
                              DXS_CHANNEL_t *pCh,
                              IFX_boolean_t bChipAccess);

static IFX_int32_t         DXS_TAPI_LL_FW_Start (
                              IFX_TAPI_LL_DEV_t *pLLDev,
                              IFX_void_t const *pProc);

static IFX_int32_t         DXS_TAPI_LL_FW_Init (
                              IFX_TAPI_LL_DEV_t *pLLDev,
                              IFX_uint8_t nMode);

static IFX_int32_t         DXS_TAPI_LL_BBD_Dnld (
                              IFX_TAPI_LL_DEV_t *pLLDev,
                              IFX_void_t const *pProc);

static IFX_void_t          dxs_DwldPtrSet (
                              const DXS_IO_Init_t *pInit,
                              DXS_FW_Download_t *pEdspDwld);

static IFX_void_t          dxs_DwldPtrUnset (
                              DXS_FW_Download_t *pEdspDwld);

static IFX_int32_t         dxs_DefaultRegInit (
                              DXS_DEVICE_t *pDev);

static IFX_int32_t         dxs_FW_DevDataInit (
                              DXS_DEVICE_t *pDev);

static IFX_int32_t         dxs_SetDevCaps (
                              DXS_DEVICE_t *pDev);

static IFX_int32_t         dxs_AddCaps (
                              DXS_DEVICE_t *pDev);

static IFX_void_t          AddCapability (
                              IFX_TAPI_CAP_t* CapList,
                              IFX_uint32_t *pnCap,
                              IFX_char_t const * description,
                              IFX_int32_t type,
                              IFX_int32_t value);

static IFX_int32_t dxs_PrintCaps(
                              const IFX_TAPI_CAP_t *const pCapList,
                              IFX_uint32_t nCap);

static IFX_int32_t         DXS_TAPI_LL_Phone_Check_Capability (
                              IFX_TAPI_LL_DEV_t *pLLDev,
                              IFX_TAPI_CAP_t    *pCapList);

static IFX_int32_t DXS_TAPI_LL_Phone_Get_Capability_List (
                              IFX_TAPI_LL_DEV_t *pLLDev,
                              IFX_TAPI_CAP_LIST_t *pCapList);

static IFX_int32_t DXS_TAPI_LL_Phone_Get_Capabilities (
                              IFX_TAPI_LL_DEV_t *pLLDev);


/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

#if defined(DXS_DCDC_AUTODETECT_TID) || defined (DXS_DCDC_AUTODETECT_EASY32002_EXT)
/**
   Autodetection of a DC/DC variant.

   \param  pDev Pointer to a device structure.
   \param  pRet Return value (output parameter):
                DXS_statusOk if ok.
                DXS_statusDcDcTypeUnknown if unknown DC/DC type was detected.
                DXS_statusChipAccFailed if command read or command write failed.
   \return
   none
   \remarks
   This function is called only for platforms that support autodetection.
*/
static IFX_void_t dxs_dcdc_autodetect (DXS_DEVICE_t *pDev, IFX_int32_t *pRet)
{
   IFX_int32_t ret;

   tapi_debug_buffer_add_user_entry("dxs #%d dcdc autodetect setup", pDev->nDevNr);

   if (nAllowedDcDcType == DXS_DCDC_TYPE_AUTO)
   {
#if defined(DXS_DCDC_AUTODETECT_TID)
      union dxs_gpio_ctrl
      {
         IFX_uint32_t     data[2];
         DXS_GPIO_CTRL_t  cmd;
      };

      union dxs_gpio_data
      {
         IFX_uint32_t        data[2];
         DXS_GPIO_RW_DATA_t  cmd;
      };

      union dxs_gpio_ctrl  gpio_ctrl;
      union dxs_gpio_data  gpio_data;
      IFX_uint32_t         bkp_gpio_ctrl;

      gpio_ctrl.data[0] = 0x86000204;
      gpio_data.data[0] = 0x86000304;
      gpio_ctrl.data[1] = gpio_data.data[1] = 0;

      /* read GPIO_CTRL */
      ret = DXS_CmdRead(pDev, (IFX_uint32_t *)&gpio_ctrl,
                              (IFX_uint32_t *)&gpio_ctrl);
      if (DXS_statusOk == ret)
      {
         bkp_gpio_ctrl = gpio_ctrl.data[1];

         /* enable internal PU */
         gpio_ctrl.cmd.GPIO3_PULL_UP = 1;
         gpio_ctrl.cmd.GPIO2_PULL_UP = 1;
         gpio_ctrl.cmd.GPIO1_PULL_UP = 1;
         gpio_ctrl.cmd.GPIO0_PULL_UP = 1;
         gpio_ctrl.cmd.GPIO3_DIR = 1;
         gpio_ctrl.cmd.GPIO2_DIR = 1;
         gpio_ctrl.cmd.GPIO1_DIR = 1;
         gpio_ctrl.cmd.GPIO0_DIR = 1;
         /* Mask all bits but MSB */
         gpio_ctrl.data[0] &= 0x7FFFFFFF;

         ret = DXS_CmdWrite(pDev, (IFX_uint32_t *)&gpio_ctrl);
         if (DXS_statusOk == ret)
         {
            /* read the pin status */
            ret = DXS_CmdRead(pDev, (IFX_uint32_t *)&gpio_data,
                                    (IFX_uint32_t *)&gpio_data);
            if (DXS_statusOk == ret)
            {
               /* restore original GPIO_CTRL */
               gpio_ctrl.data[1] = bkp_gpio_ctrl;
               ret = DXS_CmdWrite(pDev, (IFX_uint32_t *)&gpio_ctrl);
            }
         }
      }
#endif /* defined(DXS_DCDC_AUTODETECT_TID) */

      if (DXS_statusOk == ret)
      {
         IFX_uint32_t gpio_status_word;
#if defined(DXS_DCDC_AUTODETECT_TID)
         gpio_status_word = ((gpio_data.cmd.GPIO3_VAL << 3) |
                             (gpio_data.cmd.GPIO2_VAL << 2) |
                             (gpio_data.cmd.GPIO1_VAL << 1) |
                             (gpio_data.cmd.GPIO0_VAL << 0)) & 0xF;

         /* GPIO3   GPIO2   GPIO1   GPIO0
            1       0       0       1        IFB12
            1       0       0       0        CIFB12
            1       1       1       0        CIBB12
            1       1       1       1        IBB12
         */
#elif defined(DXS_DCDC_AUTODETECT_EASY32002_EXT)
         gpio_status_word = drv_board_dcdc_get();
#endif /* defined(DXS_DCDC_AUTODETECT_TID) */

         switch (gpio_status_word)
         {
            case 0x9:
               nAllowedDcDcType = DXS_DCDC_TYPE_IFB;
               break;

            case 0x8:
               nAllowedDcDcType = DXS_DCDC_TYPE_CIFB;
               break;

            case 0xE:
               nAllowedDcDcType = DXS_DCDC_TYPE_CIBB;
               break;

            case 0xF:
               nAllowedDcDcType = DXS_DCDC_TYPE_IBB;
               break;

            default:
               nAllowedDcDcType = DXS_DCDC_TYPE_NOTSET;
               *pRet = DXS_statusDcDcTypeUnknown;
               return;
         }
      }
      else
      {
         /* chip access error */
         nAllowedDcDcType = DXS_DCDC_TYPE_NOTSET;
         *pRet = DXS_statusChipAccFailed;
         return;
      }
   }

   *pRet = DXS_statusOk;
}
#endif /* defined(DXS_DCDC_AUTODETECT_TID) || defined (DXS_DCDC_AUTODETECT_EASY32002_EXT) */


/**
   Get a pointer to the device struct.

   \param  nr           Number of the device. Counting starts with zero.
   \param  pDev         Returns pointer to a device structure.

   \return
   DXS_statusOk         if ok.
   DXS_statusErr        if device number is out of supported range.
*/
IFX_int32_t DXS_GetDevice (IFX_uint16_t nr, DXS_DEVICE_t** pDev)
{
   if (pDev == IFX_NULL)
   {
      return DXS_statusErr;
   }

   if (nr >= DXS_MAX_DEVICES)
   {
      *pDev = IFX_NULL;
      return DXS_statusErr;
   }

   *pDev = &DxsDevices[nr];
   return DXS_statusOk;
}


/**
   Get a pointer to the neighbour channel struct of the given DXS channel.

   The DXS only has two channels by HW design and so if channel 0 is passed
   channel 1 is returned and vice versa.

   \param  pCh          Pointer to the channel.

   \return
   Pointer to the DXS channel struct of the neighbour channel. Also for
   1 channel devices there is a neighbouring channel although it will not
   carry any resources.
*/
DXS_CHANNEL_t *DXS_GetNeighbourChannel (DXS_CHANNEL_t *pCh)
{
   IFX_uint8_t nCh;

   /* Note that the channel number stored in the channel is index + 1. */
   nCh = (pCh->nChannel - 1) == 0 ? 1 : 0;

   return &pCh->pParent->pChannel[nCh];
}


/**
   Initialise the access to the chip.

   This function allocates all resources needed for access to the chip.

   \param  pDev         Pointer to the device structure.
   \param  pBasicDeviceInit  Pointer to the chip access init parameters.

   \return
   - IFX_SUCCESS        if successful
   - IFX_ERROR          in case of an error

   \remarks
   Must be the first action after installing the driver and
   before any access to the hardware (corresponding DUSLIC XS chip).
*/
IFX_int32_t DXS_ChipAccessInit(DXS_DEVICE_t *pDev,
                               const DXS_BasicDeviceInit_t *pBasicDeviceInit)
{
   IFX_int32_t ret = DXS_statusErr;

   /* make sure that we really got a device structure */
   if ((pDev == IFX_NULL) || (pDev->nChannel != 0))
   {
      return DXS_statusErr;
   }

   if (pDev->nDevState & DS_BASIC_INIT)
   {
      /* Chip access was already initialised. So this is a reconfiguration.
         Reset all driver internal states and do a chip access exit. */
      IFX_TAPI_DeviceReset (pDev->pTapiDev);
   }

   /* initialize firmware download protection semaphore */
   TAPI_OS_MutexInit (&pDev->mtxFwDlAcc);

   /* initialize mailbox protection semaphore */
   TAPI_OS_MutexInit (&pDev->mtxMbxAcc);

   tapi_debug_buffer_add_user_entry("dxs #%d spi init", pDev->nDevNr);

   /* Initialise SPI access to the device. */
   dxs_init_spi (pDev);

   tapi_debug_buffer_add_user_entry("dxs #%d reg access test", pDev->nDevNr);

   /* check if device is connected and accessible */
   ret = DXS_reg_access_test(pDev);

   if (ret != DXS_statusOk)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("DXS ERROR: Dev %d SPI access test failed!\n", pDev->nDevNr));
      RETURN_DEVSTATUS(DXS_statusTestChipAccErr, IFX_NULL);
   }

   tapi_debug_buffer_add_user_entry("dxs #%d reboot", pDev->nDevNr);

   /* execute a controller reset */
   ret = DXS_ResetController(pDev, DXS_BOOT_ROM);
   if (ret != DXS_statusOk)
      RETURN_DEVSTATUS(DXS_statusCtrlResErr, IFX_NULL);

   /*lint -save -esym(613,pBasicDeviceInit) possible use of NULL pointer
      -> within macro ON_IOCTL_FRUSR the parameter is already checked against
      possible NULL pointers with TAPI_ASSERT */
   if (pBasicDeviceInit->nIrqNum >= 0)
#ifdef DXS_HAVE_INTERRUPTS
   {
      /* Set flag that events should be retrieved after an interrupt. */
      pDev->nEventHandlingMode = DXS_EVENT_INTERRUPT;

#ifdef __LINUX_SPI_H
      /* Check if IRQ number was provided inside of DXS SPI DT node.
         If yes, ignore nIrqNum param from pBasicDeviceInit */
      if ((pDev->pSpiDev) && (pDev->pSpiDev->irq >= 0))
      {
         tapi_debug_buffer_add_user_entry("dxs #%d install irq #%d handler", pDev->nDevNr, pDev->pSpiDev->irq);
         OS_Install_DxsIRQHandler(pDev, pDev->pSpiDev->irq);
      }
      /* If IRQ number is not existing / invalid inside of DXS SPI DT node,
         use IRQ number provided in IOCTL parameter (nIrqNum) */
      else
#endif /* __LINUX_SPI_H */
      {
         tapi_debug_buffer_add_user_entry("dxs #%d install irq handler", pDev->nDevNr);
         OS_Install_DxsIRQHandler(pDev, pBasicDeviceInit->nIrqNum);
      }

      if (pDev->pIrq == IFX_NULL)
      {
         /* Don't start event handling for the device. */
         pDev->nEventHandlingMode = DXS_EVENT_STOP;
         ret = IFX_ERROR;
      }
#ifndef DXS_FEAT_LINUX_THREADED_IRQ
      else
      {
#ifdef IRQLINE_INIT
         /* Configure and enable the interrupt line on the Port and
            IRQ controller. */
         IRQLINE_INIT(pDev);
#ifdef VXWORKS
         pInterruptCounters[pDev->nDevNr] = 1;
#endif /* VXWORKS */
#endif /*DXS_ENABLE_EXINT*/

         DXS_IrqUnlockDevice (pDev);
      }
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */
   }
   else

#else /* DXS_HAVE_INTERRUPTS */
   {
      /* interrupt handling not available, but the IRQ number was set,
         print warning and use the polling mode */
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("DXS WARNING: Interrupt mode support not available, "
             "only polling mode supported.\n"
             "DXS WARNING: Polling mode will be used for for handling events.\n"));
   }
#endif /* DXS_HAVE_INTERRUPTS */

   {
      /* Set flag that events should be retrieved by polling.
         It is first needed in dxs_init_spi() to check
         if resources for power save mode should be acquired. */
      pDev->nEventHandlingMode = DXS_EVENT_POLLING;
      DXS_PollingModeTimerStart();
   }
   /*lint -restore */

   if (ret == IFX_SUCCESS)
   {
      /* Device state: basic init is done */
      pDev->nDevState |= DS_BASIC_INIT;
   }
   else
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("DXS ERROR: Reserving the resources for chip access failed\n"));
   }

   tapi_debug_buffer_add_user_entry("dxs #%d irq masks setting", pDev->nDevNr);

   if (ret == IFX_SUCCESS /*lint --e(774)*/)
   {
      /* In IRQ as well as polling mode set the interrupt masks. */
      ret = dxs_DefaultRegInit(pDev);
   }

   if (ret == IFX_SUCCESS)
   {
      /* platform specific DC/DC type autodetection function */
      DXS_DCDC_AUTODETECT_FUNC(pDev, &ret);
      if (ret != IFX_SUCCESS)
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("DXS ERROR: DC/DC type autodetection failed, code 0x%08X\n", ret));
      }
   }

   return ret;
}


/**
   Release the access to the chip.

   This function releases all resources needed for access to the chip.

   \param  pDev         Pointer to the device structure.
*/
static IFX_void_t dxs_ChipAccessExit(DXS_DEVICE_t *pDev)
{
   /* make sure that we really got a device structure */
   if((pDev == IFX_NULL) || (pDev->nChannel != 0))
   {
      return;
   }

   if (pDev->nDevState & DS_BASIC_INIT)
   {
      /* Device state: basic init is not done */
      pDev->nDevState &= ~DS_BASIC_INIT;

      /* Stop event handling for the device. */
      pDev->nEventHandlingMode = DXS_EVENT_STOP;
      /* free the interrupt */
      if (pDev->pIrq != IFX_NULL)
      {
#ifdef IRQLINE_EXIT
         /* Disable the interrupt line on the Port and IRQ controller. */
         IRQLINE_EXIT(pDev)
#endif /*DXS_ENABLE_EXINT*/

#ifdef DXS_HAVE_INTERRUPTS
         OS_UnInstall_DxsIRQHandler(pDev);
#endif /* DXS_HAVE_INTERRUPTS */
      }
   }

   /* delete the mailbox protection semaphore */
   TAPI_OS_MutexDelete (&pDev->mtxMbxAcc);

   /* Release the  SPI access to the device. */
   pDev->nDevState &= ~DS_SPI_ACTIVE;
   dxs_exit_spi (pDev);
   TAPI_OS_MutexDelete (&pDev->mtxSpiAcc);
   TAPI_OS_MutexDelete (&pDev->mtxFwDlAcc);
}


/**
   Prepare the low level device struct.

   This function clears the device struct, sets the device number and links it
   with the high-level device.

   \param  pTapiDev     Pointer to the high-level device struct.
   \param  devNum       Device number.

   \return
   Pointer to DXS Device or IFX_NULL.
*/
static IFX_TAPI_LL_DEV_t* DXS_TAPI_LL_DevicePrepare(TAPI_DEV *pTapiDev,
                                                    IFX_uint32_t devNum)
{
   DXS_DEVICE_t* pDev;

   /* make sure a valid context is given */
   if (devNum >= DXS_MAX_DEVICES)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("%s: DXS device number out of range\n", __FUNCTION__));
      return IFX_NULL;
   }
   if (pTapiDev == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("%s: pTapiDev is NULL\n", __FUNCTION__));
      return IFX_NULL;
   }

   pDev = &DxsDevices[devNum];

   /* Clear the device struct (including the channels) */
   memset(pDev, 0, sizeof(DXS_DEVICE_t));

   pDev->nDevNr = devNum;

   /* Store the corresponding HL pointer */
   pDev->pTapiDev = pTapiDev;

   /* Event handling is stopped until it is configured. */
   pDev->nEventHandlingMode = DXS_EVENT_STOP;

   /* The version command is special as it can be used before the device is
      initialised. */
   memset (&pDev->fw_vers, 0, sizeof(pDev->fw_vers));
   pDev->fw_vers.CMD = DXS_CMD_CMD_EOP;
   pDev->fw_vers.MOD = DXS_CMD_MOD_SYS;
   pDev->fw_vers.ECMD = DXS_FW_SYS_VERS_ECMD;
   pDev->fw_vers.LENGTH = DXS_FW_SYS_VERS_LENGTH;

   dxs_AddCaps (pDev);

   /* Return the pDev pointer which is stored in the HL device */
   return pDev;
}


/**
   Initialise the low level device struct.

   Initialise the member variables of the device structure.

   \param  pLLDev       Pointer to the device structure.

   \return
   - DXS_statusOk       If successful.
   - DXS_statusErr      If channel pointer is null
*/
static IFX_int32_t DXS_TAPI_LL_DeviceInit(IFX_TAPI_LL_DEV_t* pLLDev)
{
   DXS_DEVICE_t *pDev = (DXS_DEVICE_t *)pLLDev;

   if(pLLDev == IFX_NULL)
      return DXS_statusErr;

   tapi_debug_buffer_add_user_entry("dxs #%d device init", pDev->nDevNr);

   /* OS and board independent initializations, resets all values */

   /* intially there is no error */
   pDev->nErr = DXS_statusOk;

   /* initialize share variables protection semaphore */
   TAPI_OS_MutexInit(&pDev->mtxMemberAcc);

   /* Easy to check flag for checking dedicated / combined DC/DC converter. */
   if (nAllowedDcDcType == DXS_DCDC_TYPE_CIBB ||
      nAllowedDcDcType == DXS_DCDC_TYPE_CIB ||
      nAllowedDcDcType == DXS_DCDC_TYPE_CBB ||
      nAllowedDcDcType == DXS_DCDC_TYPE_CIFB ||
      nAllowedDcDcType == DXS_DCDC_TYPE_CIBGD ||
      nAllowedDcDcType == DXS_DCDC_TYPE_CIBVD)
   {
      pDev->bDcDcHwCombined = IFX_TRUE;
   }
   else
   {
      pDev->bDcDcHwCombined = IFX_FALSE;
   }

   memset (&pDev->fw_caps, 0, sizeof(pDev->fw_caps));
   pDev->fw_caps.CMD = DXS_CMD_CMD_EOP;
   pDev->fw_caps.MOD = DXS_CMD_MOD_SYS;
   pDev->fw_caps.ECMD = DXS_FW_SYS_CAPS_ECMD;
   pDev->fw_caps.LENGTH =  DXS_FW_SYS_CAPS_LENGTH;

   memset (&pDev->fw_sys_ctrl, 0, sizeof(pDev->fw_sys_ctrl));
   pDev->fw_sys_ctrl.CMD = DXS_CMD_CMD_EOP;
   pDev->fw_sys_ctrl.MOD = DXS_CMD_MOD_SYS;
   pDev->fw_sys_ctrl.ECMD = DXS_FW_SYS_Control_ECMD;
   pDev->fw_sys_ctrl.LENGTH = DXS_FW_SYS_Control_LENGTH;

   return DXS_statusOk;
}


/**
   Frees all resources of the device.

   This calls also exit on all channels because they are members of the device.
   Called when the device is released.

   \param  pDev         Pointer to the device structure.
   \param  bChipAccess  Allow or deny chip access in this function.
*/
static IFX_void_t dxs_DeviceExit(DXS_DEVICE_t *pDev,
                                 IFX_boolean_t bChipAccess)
{
   IFX_uint16_t i;

   tapi_debug_buffer_add_user_entry("dxs #%d device exit", pDev->nDevNr);

   /* call exit on all channels of the device */
   for (i = 0; i < DXS_MAX_CH_NR; i++)
   {
      dxs_ChannelExit(&pDev->pChannel[i], bChipAccess);
   }

   if (bChipAccess != IFX_FALSE)
   {
      /* The PCM interface is device global and can only be stopped after all
         PCM channels are deactivated. */
      DXS_PCM_IF_Stop(pDev);
   }

   /* reset PCM timeslot management flags */
   memset(pDev->PcmRxTs, 0, sizeof(pDev->PcmRxTs));
   memset(pDev->PcmTxTs, 0, sizeof(pDev->PcmTxTs));

   /* Fill the capabilities again with initial values.
      To have capabilities available during cleanup this is done last. */
   pDev->bCapsRead = IFX_FALSE;
   dxs_AddCaps (pDev);

   /* delete device mutex */
   TAPI_OS_MutexDelete (&pDev->mtxMemberAcc);

   /* release chip access */
   dxs_ChipAccessExit(pDev);

   /* reset the states variable */
   pDev->nDevState = 0;
}


/**
   Stop the DXS device and free all allocated resources.

   \param  pLLDev       Pointer to the device structure.
   \param  bChipAccess  Allow or deny chip access in this function.
*/
static IFX_void_t DXS_TAPI_LL_DeviceExit (IFX_TAPI_LL_DEV_t *pLLDev,
                                          IFX_boolean_t bChipAccess)
{
   DXS_DEVICE_t  *pDev = (DXS_DEVICE_t *)pLLDev;

   if(pLLDev == IFX_NULL)
      return;

   /* resource cleanup */
   dxs_DeviceExit(pDev, bChipAccess);
}


/**
   Prepare the low level channel struct.

   \param  pTapiCh      High level channel pointer.
   \param  pLLDev       Pointer to the device structure.
   \param  chNum        Channel number.

   \return
   Pointer to DXS Channel or IFX_NULL.
*/
static IFX_TAPI_LL_CH_t* DXS_TAPI_LL_ChannelPrepare(TAPI_CHANNEL *pTapiCh,
                                                    IFX_TAPI_LL_DEV_t *pLLDev,
                                                    IFX_uint32_t chNum)
{
   DXS_DEVICE_t  *pDev = (DXS_DEVICE_t *)pLLDev;
   DXS_CHANNEL_t *pCh;

   if(pDev == IFX_NULL)
   {
      return IFX_NULL;
   }
   /* Make sure we have a DXS channel that we can bind to the TAPI channel. */
   if (chNum >= DXS_MAX_CH_NR)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("%s: No DXS channel available to bind to TAPI channel %d\n",
            __FUNCTION__, chNum));
      return IFX_NULL;
   }
   if (pTapiCh == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("%s: pTapiCh is NULL\n", __FUNCTION__));
      return IFX_NULL;
   }

   /* TAPI channel number is identical to the channel index in this driver. */
   pCh  = &(pDev->pChannel[chNum]);

   pCh->nChannel = (IFX_uint8_t)chNum + 1;
   pCh->pParent  = pDev;

   /* Store the corresponding HL TAPI channel pointer */
   pCh->pTapiCh = pTapiCh;

   /* Should return the pCh which should be stored by the HL */
   return pCh;
}


/**
   Initialise the low level channel struct.

   \param  pLLCh       Pointer to the channel structure.

   \return
   IFX_SUCCESS
*/
static IFX_int32_t DXS_TAPI_LL_ChannelInit (IFX_TAPI_LL_CH_t *pLLCh)
{
   DXS_CHANNEL_t *pCh = (DXS_CHANNEL_t *)pLLCh;

   TAPI_ASSERT(pCh != IFX_NULL);

   /* init channel lock */
   TAPI_OS_MutexInit (&pCh->mtxChAcc);

   return IFX_SUCCESS;
}


/**
   Frees all resources of the channel.

   Called when the channel is released.

   \param  pCh          Pointer to the channel.
   \param  bChipAccess  Allow or deny chip access in this function.
*/
static IFX_void_t dxs_ChannelExit(DXS_CHANNEL_t *pCh,
                                  IFX_boolean_t bChipAccess)
{
   /* Deactivate all still running algorithms in the FW */
   DXS_ALM_ChStop(pCh, bChipAccess);

   if (bChipAccess != IFX_FALSE)
   {
      DXS_PCM_ChStop(pCh);
   }

   /* delete channel mutex */
   TAPI_OS_MutexDelete (&pCh->mtxChAcc);

   /* free the modules */
   DXS_ALM_Free_Ch_Structures (pCh);
   DXS_PCM_Free_Ch_Structures (pCh);
   DXS_DTMF_AT_Free_Ch_Structures(pCh);
#ifdef DXS_FEAT_CID
   DXS_CID_Free_Ch_Structures(pCh);
#endif /* DXS_FEAT_CID */
}


/**
   Firmware start function.

   \param  pLLDev       Pointer to the device structure.
   \param  pProc        Pointer to low-level device initialization structure.

   \return
   - DXS_statusOk             if successful
   - DXS_statusParam          no valid pointer to initialization provided
   - DXS_statusSpiAccErr
   - DXS_statusReadErr
   - DXS_statusFwDwldFail     firmware download failed.
   - DXS_statusRegInitErr
   - DXS_statusBbdErr         download of BBD buffer failed.
   - DXS_statusNoBbdBuf       no valid BBD buffer received
*/
static IFX_int32_t DXS_TAPI_LL_FW_Start(IFX_TAPI_LL_DEV_t *pLLDev,
                                        IFX_void_t const *pProc)
{
   DXS_DEVICE_t *pDev = (DXS_DEVICE_t*)pLLDev;
   DXS_IO_Init_t IoInit = {0};
   IFX_int32_t ret = DXS_statusErr;

   /* blocked until the chip access is initialised */
   if (!(pDev->nDevState & DS_BASIC_INIT))
   {
      /* errmsg: Chip access is not setup. */
      RETURN_DEVSTATUS (DXS_statusChipAccNotSetup, IFX_NULL);
   }

   /* Do not start the firmware again after it was initialised. */
   if (pDev->nDevState & DS_DEV_INIT)
   {
      return DXS_statusOk;
   }

   /* check if io init ptr valid */
   if (pProc == IFX_NULL)
   {
      /* errmsg: At least one parameter is wrong. */
      RETURN_DEVSTATUS (DXS_statusParam, IFX_NULL);
   }
   else
   {
      /* The init struct is specific for the LL driver and only known here.
         Because HL does not know the struct it cannot copy it and so it is
         copied here in the LL driver. */
      TAPI_OS_CpyUsr2Kern(&IoInit, pProc, sizeof(IoInit));
   }

   if (IoInit.pram_size > DXS_MAX_FIRMWARE_SIZE)
      RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);

   /* Call the chip specific part */
   ret = DXS_FW_Start(pDev, &IoInit);

   RETURN_DEVSTATUS (ret, IFX_NULL);
}


/**
   Download and start the firmware of the DUSLIC XS chip.

   \param  pDev         Pointer to the device structure.
   \param  pInit        Pointer to the initialization structure.

   \return
   - DXS_statusOk             if successful
   - DXS_statusSpiAccErr
   - DXS_statusReadErr
   - DXS_statusFwDwldFail     firmware download failed.
   - DXS_statusRegInitErr
   - DXS_statusBbdErr         download of BBD buffer failed.
   - DXS_statusNoBbdBuf       no valid BBD buffer received
*/
IFX_int32_t DXS_FW_Start(DXS_DEVICE_t *pDev, const DXS_IO_Init_t *pInit)
{
   IFX_int32_t ret = DXS_statusErr;
   DXS_FW_Download_t edspDwld = {0};

   /* Setting the cached mailbox length to zero will force a read from chip
      register upon the next access. */
   pDev->nMbxCachedCbiLen = 0;

   /* Check success of the download by reading the boot info register. */
   ret = DXS_Wait4BootFinished(pDev, 10);
   if (ret != DXS_statusOk)
   {
      /* errmsg: Firmware download timeout. */
      RETURN_DEVSTATUS(DXS_statusFwDwldTimeout, IFX_NULL);
   }

   if (pInit->pram_size > DXS_MAX_FIRMWARE_SIZE)
      RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);

   /* set download pointers */
   dxs_DwldPtrSet(pInit, &edspDwld);

   /* download firmware */
   ret = DXS_DwldFirmwareSelect(pDev, &edspDwld);

   /* now that downloads are finished, release memory */
   dxs_DwldPtrUnset(&edspDwld);

   if (ret != DXS_statusOk)
   {
      /* errmsg: Firmware download failed. */
      RETURN_DEVSTATUS(DXS_statusFwDwldFail, IFX_NULL);
   }

   /* Update FW version information after FW download */
   ret = DXS_ReadFwVersion(pDev);
   if (ret != DXS_statusOk)
   {
      /* errmsg: Firmware download failed. */
      RETURN_DEVSTATUS(DXS_statusFwDwldFail, IFX_NULL);
   }

   /* Warn if the version of the FW is older than a defined minimum version.
      The minimum version depends on the silicon. */
   {
      IFX_uint8_t min_version[3] = {0};
      IFX_uint8_t i, min_vers_len = 0;

      switch (pDev->fw_vers.MAJ & 0x7F)  /* ignore test-bit in major nr. */
      {
      case 1:
         /* ROM version 1.x.x */
         min_version[0] = DXS_V11_MIN_FW_DIGIT1;
         min_version[1] = DXS_V11_MIN_FW_DIGIT2;
         min_version[2] = DXS_V11_MIN_FW_DIGIT3;
         min_vers_len = 3;
         break;

      case 2:
         /* ROM version 2.x.x */
         if (pDev->fw_vers.CH == 0)
         {
            /* DXS 2-channel device */
            min_version[0] = DXS2_V12_MIN_FW_DIGIT1;
            min_version[1] = DXS2_V12_MIN_FW_DIGIT2;
            min_version[2] = DXS2_V12_MIN_FW_DIGIT3;
            min_vers_len = 3;
         }
         else
         {
            /* DXS 1-channel device */
            min_version[0] = DXS1_V12_MIN_FW_DIGIT1;
            min_version[1] = DXS1_V12_MIN_FW_DIGIT2;
            min_version[2] = DXS1_V12_MIN_FW_DIGIT3;
            min_vers_len = 3;
         }
         break;

      default:
         /* unknown ROM version - force a warning */
            min_version[0] = 0xFF;
            min_vers_len = 1;
         break;
      }

      /* Iterate over all digits of the version number starting with the
         major. For each digit first check if it is greater than the same
         digit in the required minimum version. If so no further checks are
         needed. Second check if this digit is smaller and if so do a
         warning and end the check. If the digits are equal continue with
         the next digit until all has been checked. */
      for (i = 0; (i < min_vers_len) && (i < 3); i++)
      {
         IFX_uint8_t nCurVersDigit = (IFX_uint8_t)((pDev->nFwRev >> ((3 - i) * 8)) & 0xFF);

         if (nCurVersDigit > min_version[i])
         {
            break;
         }
         if (nCurVersDigit < min_version[i])
         {
            TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                     ("\nDXS WARNING: FW version %u.%u.%u is too old."
                     " Minimum required FW version is %u.%u.%u"
                     "\n\n",
                     (IFX_uint8_t)((pDev->nFwRev >> 24) & 0xFF),
                     (IFX_uint8_t)((pDev->nFwRev >> 16) & 0xFF),
                     (IFX_uint8_t)((pDev->nFwRev >>  8) & 0xFF),
                     min_version[0], min_version[1], min_version[2]));
            break;
         }
      }
   }

   {
      /*
         set self-clearing mode - interrupts CBI_DATA, EBI_DATA, EVENTS in the
         HOST_INT1 register are cleared-on-read. This reduces communication
         overhead, but requires that all interrupt events have to be processed
         immediately or remembered for later processing in the controller.
      */
      const IFX_uint16_t nRegWrite = (DXS_REG_CFG_SC_MD_SCON | DXS_REG_CFG_8BIT_EN);

      tapi_debug_buffer_add_user_entry("dxs #%d configure irq", pDev->nDevNr);

      ret = DXS_RegWrite(pDev, DXS_HOST_CFG, nRegWrite);
   }

   RETURN_DEVSTATUS(ret, IFX_NULL);
}


#ifdef TAPI_FEAT_LX_COMPAT
/**
   Linux compat function for DXS_FW_Start

   \param  pDev         Pointer to the device structure.
   \param  pInit        Pointer to the initialization structure.

   \return
   - DXS_statusOk             if successful
   - DXS_statusSpiAccErr
   - DXS_statusReadErr
   - DXS_statusFwDwldFail     firmware download failed.
   - DXS_statusRegInitErr
   - DXS_statusBbdErr         download of BBD buffer failed.
   - DXS_statusNoBbdBuf       no valid BBD buffer received
*/
IFX_int32_t DXS_FW_Start_32(DXS_DEVICE_t *pDev, DXS_IO_Init_32_t *pInit)
{
   DXS_IO_Init_t init_64 = {0};
   init_64.dev = pInit->dev ;
   init_64.pPRAMfw = compat_ptr(pInit->pPRAMfw) ;
   init_64.pram_size = pInit->pram_size;
   init_64.pBBDbuf = compat_ptr(pInit->pBBDbuf);
   init_64.bbd_size = pInit->bbd_size;
   init_64.nFlags = pInit->nFlags;

   return DXS_FW_Start (pDev, &init_64);
}
#endif /* TAPI_FEAT_LX_COMPAT */


/**
   Firmware initialisation function.

   \param  pLLDev       Pointer to the device structure.
   \param  nMode        Unused - enum from IFX_TAPI_INIT_MODE_t specifying
                        the setup.

   \return
   - DXS_statusOk             if successful
   - DXS_statusParam          no valid pointer to initialization provided
   - DXS_statusSpiAccErr
   - DXS_statusReadErr
   - DXS_statusFwDwldFail     firmware download failed.
   - DXS_statusRegInitErr
   - DXS_statusBbdErr         download of BBD buffer failed.
   - DXS_statusNoBbdBuf       no valid BBD buffer received
   - DXS_statusDtmfRcvInitErr

   \remarks
   Initialization flow of DUSLIC XS :
   - SDD Rev Read, store DevId 3xyz : x=fxs ch y=fxo ch z=PCM i/f,
     store FW Revision
   - set self clearing mode via register HOST_CFG
   - FW Patch download
   - unmask default irqs (ERR, EBO_DATA, ...)
   - SDD Rev Read, check for changes
   - BBD download (SDD Basic Config, SDD Ring Config, SDD Coefs)
   - allocate and init selected FW messages, e.g. set PCM I/F configuration for
     default configuration (if PCM_CH_CFG is issued before PCM_IF_CFG
   - SDD Opmode Disabled (reset anyhow)
   - DUSLIC XS Setup init (automodes)
   - optional unmask further irqs (GndKey on FXS ch, O-Temp on FXS ch)
*/
/*lint -esym(715, nMode) */
static IFX_int32_t DXS_TAPI_LL_FW_Init(IFX_TAPI_LL_DEV_t *pLLDev,
                                       IFX_uint8_t nMode)
{
   DXS_DEVICE_t         *pDev = (DXS_DEVICE_t*)pLLDev;
   IFX_uint8_t          nMaxRes;
   IFX_TAPI_RESOURCE    nResource;
   IFX_int32_t          ret;

   TAPI_UNUSED (nMode);

   /* do nothing if device initialization was already done */
   if (pDev->nDevState & DS_DEV_INIT)
   {
      return DXS_statusOk;
   }

   /* blocked until the chip access is initialised */
   if (!(pDev->nDevState & DS_BASIC_INIT))
   {
      /* errmsg: Chip access is not setup. */
      RETURN_DEVSTATUS (DXS_statusChipAccNotSetup, IFX_NULL);
   }

   /* Fill the internal capabilities struct. */
   ret = dxs_SetDevCaps (pDev);
   if (!DXS_SUCCESS(ret))
      RETURN_DEVSTATUS (ret, IFX_NULL);

   TRACE(TAPI_DXS, DBG_LEVEL_LOW,
         ("DXS FWCAP: nPCM:%2u nALM:%2u nDTMFR:%2u nDTMFG:%2u nCIDS:%2u\n",
          pDev->caps.nPCM, pDev->caps.nALI, pDev->caps.nDTMFR,
          pDev->caps.nDTMFG, pDev->caps.nCIDS));

   /* Maximum number of resources, is the maximum of: nALI, nPCM. */
   nMaxRes = (pDev->caps.nPCM > pDev->caps.nALI) ?
              pDev->caps.nPCM : pDev->caps.nALI;

   if (nMaxRes > DXS_MAX_CH_NR)
   {
      /* Warn about incorrect definition then limit the counters so that we
         do not crash when trying to initialise non existing resources. */
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("DXS WARNING: Detected maximum resources %d but defined only %d. "
             "Some resources will not be usable\n",
             nMaxRes, DXS_MAX_CH_NR));

      if (pDev->caps.nALI > DXS_MAX_CH_NR)
         pDev->caps.nALI = DXS_MAX_CH_NR;
      if (pDev->caps.nPCM > DXS_MAX_CH_NR)
         pDev->caps.nPCM = DXS_MAX_CH_NR;
   }

   /* report the resource counts to HL TAPI */
   memset (&nResource, 0x00, sizeof(nResource));
   nResource.AlmCount   = pDev->caps.nALI;
   nResource.PcmCount   = pDev->caps.nPCM;
   nResource.DTMFGCount = pDev->caps.nDTMFG;
   nResource.DTMFRCount = pDev->caps.nDTMFR;
   nResource.FSKGCount  = pDev->caps.nCIDS;
   nResource.ToneCount  = pDev->caps.nALI;
   IFX_TAPI_ReportResources (pDev->pTapiDev, &nResource);

   /* set default firmware cache values now, required to allow storage of
      coefficients from BBD Download... */
   ret = dxs_FW_DevDataInit(pDev);

   if (ret == DXS_statusOk)
   {
      ret = dxs_DefaultRegInit(pDev);
      if (ret != DXS_statusOk)
      {
         /* errmsg: Initialzing registers with default values failed. */
         ret = DXS_statusRegInitErr;
      }
   }

   if (ret == DXS_statusOk)
   {
      pDev->nDevState |= DS_FW_DLD;
      pDev->nDevState |= DS_DEV_INIT;
      /* Update the list now that we consolidated the chip capabilities. */
      ret = dxs_AddCaps(pDev);
   }

   RETURN_DEVSTATUS (ret, IFX_NULL);
}
/*lint +esym(715, nMode) */


/**
   Download a BBD file.

   \param  pLLDev       Pointer to the device structure.
   \param  pProc        Pointer to low-level device initialization structure.

   \return
   - DXS_statusOk if successful
*/
static IFX_int32_t DXS_TAPI_LL_BBD_Dnld(IFX_TAPI_LL_DEV_t *pLLDev,
                                        IFX_void_t const *pProc)
{
   DXS_DEVICE_t *pDev = (DXS_DEVICE_t *)pLLDev;
   DXS_IO_Init_t IoInit = {0};
   DXS_BBD_Download_t bbdDwld = {0};
   IFX_int32_t ret = DXS_statusErr;

   /* check if io init ptr valid */
   if (pProc == IFX_NULL)
   {
      /* errmsg: At least one parameter is wrong. */
      RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);
   }

   /* The init struct is specific for the LL driver and only known here.
      Because HL does not know the struct it cannot copy it and so it is
      copied here in the LL driver. */
   TAPI_OS_CpyUsr2Kern(&IoInit, pProc, sizeof(IoInit));

   /* BBD Download */
   if (IoInit.pBBDbuf == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("DXS INFO: No BBD Buffer provided.\n"));
      RETURN_DEVSTATUS(DXS_statusNoBbdBuf, IFX_NULL);
   }

   bbdDwld.buf  = IoInit.pBBDbuf;
   bbdDwld.size = IoInit.bbd_size;

   if (bbdDwld.size > DXS_MAX_BBD_FIRMWARE_SIZE)
   {
      /* Invalid bbdDwld.size */
      RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);
   }

   ret = DXS_BBD_Download((DXS_CHANNEL_t *)(IFX_void_t *)pDev, &bbdDwld);

   if (!DXS_SUCCESS(ret))
   {
      /* errmsg: BBD download failed. */
      RETURN_DEVSTATUS(DXS_statusBbdErr, IFX_NULL);
   }

   RETURN_DEVSTATUS(DXS_statusOk, IFX_NULL);
}


/**
   Maps the FW patch binary into memory usable by this driver

   This function reserves dynamic memory for download data and copies data
   from user space.

   \param  pInit        Pointer to DXS_IO_Init_t with user download data.
   \param  pEdspDwld    Pointer to struct that is filled with the pointers
                        to the mapped FW binary.
*/
static IFX_void_t dxs_DwldPtrSet(const DXS_IO_Init_t *pInit,
                                 DXS_FW_Download_t *pEdspDwld)
{
   /* do some initializations ... */
   memset(pEdspDwld, 0, sizeof(*pEdspDwld));
   pEdspDwld->pPRAMfw = IFX_NULL;
   pEdspDwld->nEdspFlags = DXS_NO_FW_DWLD;

   if (pInit->pPRAMfw != IFX_NULL && pInit->pPRAMfw <= (IFX_uint8_t*) SIZE_MAX)
   {
      pEdspDwld->pPRAMfw = TAPI_OS_Malloc(pInit->pram_size);
      if (pEdspDwld->pPRAMfw == IFX_NULL)
      {
         TRACE(TAPI_DXS, DBG_LEVEL_LOW, ("DXS ERR: No memory to copy FW\n"));
         return;
      }

      if (TAPI_OS_CpyUsr2Kern(pEdspDwld->pPRAMfw, pInit->pPRAMfw,
                              pInit->pram_size) == IFX_NULL)
      {
         TAPI_OS_Free(pEdspDwld->pPRAMfw);
         TRACE(TAPI_DXS, DBG_LEVEL_LOW, ("DXS ERR: User to kernel FW copy failed!\n"));
         return;
      }

      if (pEdspDwld->pPRAMfw != NULL)
      {
         pEdspDwld->pram_size = pInit->pram_size;
         pEdspDwld->nEdspFlags = pInit->nFlags & DXS_NO_FW_DWLD;
      }
   }
   else
   {
      /* User didn't provide FW patch binary. */
      TRACE(TAPI_DXS, DBG_LEVEL_LOW, ("DXS INFO: No firmware binaries for PRAM => "
                                 "default ROM firmware will be used\n"));
   }
}


/**
   Releases the download data pointers

   This function frees dynamic memory with download data.

   \param  pEdspDwld    Pointer to struct that is filled with the pointers
                        to the mapped FW binary.
*/
static IFX_void_t dxs_DwldPtrUnset(DXS_FW_Download_t *pEdspDwld)
{
   if (pEdspDwld->pPRAMfw)
   {
      TAPI_OS_Free(pEdspDwld->pPRAMfw);
   }
   memset(pEdspDwld, 0, sizeof(*pEdspDwld));
}


/**
   Write info for bootloader, how to boot.

   \param  pDev         Pointer to the device structure.
   \param  bootInfo     Enum describing the boot mode.

   \return
   -DXS_statusOk
   -DXS_statusSpiAccErr
*/
IFX_int32_t DXS_SetBootConfig (DXS_DEVICE_t *pDev, DXS_BOOT_INFO_t bootInfo)
{
   IFX_int32_t ret;
   IFX_uint16_t nHostReg = 0;

   switch (bootInfo)
   {
      case DXS_BOOT_ROM:
         nHostReg = DXS_REG_BCFG_ASC_ROM;
         break;
      case DXS_BOOT_SPI:
         nHostReg = DXS_REG_BCFG_ASC_SPI;
         break;
      case DXS_BOOT_UART:
      default:
         /* do nothing */
         break;
   }
   /* write the boot info to boot from SPI */
   ret = DXS_RegWrite(pDev, DXS_HOST_BCFG, nHostReg);

   RETURN_DEVSTATUS(ret, IFX_NULL);
}


/**
   Initiate a controller reset with the selected bootmode.

   \param  pDev         Pointer to the device structure.
   \param  bootInfo     Enum describing the boot mode.

   \return
   -DXS_statusOk
   -DXS_statusSpiAccErr
*/
IFX_int32_t DXS_ResetController (DXS_DEVICE_t *pDev, DXS_BOOT_INFO_t bootInfo)
{
   IFX_int32_t      ret = DXS_statusOk;
   IFX_uint16_t     nRegHostCfg = 0x0;
   IFX_int32_t      nCh = 0;
   DXS_SDD_Opmode_t pOpmode;
   memset(&pOpmode, 0, sizeof(DXS_SDD_Opmode_t));

   /* Channels need to be disabled before software reset */
   pOpmode.CMD         = DXS_CMD_CMD_SDD;
   pOpmode.MOD         = DXS_CMD_MOD_SDD;
   pOpmode.ECMD        = DXS_SDD_Opmode_ECMD;
   pOpmode.LENGTH      = DXS_SDD_Opmode_LENGTH;
   pOpmode.OpMode      = DXS_SDD_Opmode_Disabled;
   for(nCh = 0; nCh < DXS_MAX_CH_NR; nCh++)
   {
      pOpmode.CHAN = nCh;
      ret = DXS_CmdWrite(pDev, (IFX_uint32_t *) &pOpmode);
      if (ret != DXS_statusOk)
         RETURN_DEVSTATUS(ret, IFX_NULL);
   }

   TAPI_OS_MSecSleep(100);

   /* Set the boot mode. */
   ret = DXS_SetBootConfig(pDev, bootInfo);
   if (ret != DXS_statusOk)
   {
      /* errmsg: Setting of boot configuration register failed. */
      RETURN_DEVSTATUS(DXS_statusSetBootCfgErr, IFX_NULL);
   }

   /* Reset flag until alive event is reported. */
   pDev->nDevState &= ~DS_DEV_UP;

   /* Initiate the controller reset. */
   nRegHostCfg |= (DXS_REG_CFG_RST_RSTCORE | DXS_REG_CFG_8BIT_EN);
   ret = DXS_RegWrite(pDev, DXS_HOST_CFG, nRegHostCfg);

   if (ret != DXS_statusOk)
      RETURN_DEVSTATUS(ret, IFX_NULL);

   /* flush command mailbox fifo */
   fifo_flush (pDev->cmd_obx_queue);

   switch (bootInfo)
   {
      case DXS_BOOT_ROM:
         if (DXS_Wait4BootFinished(pDev, 500) != DXS_statusOk)
         {
            /* errmsg: Chip boot failed. */
            ret = DXS_statusBootFailed;
         }
         break;

      case DXS_BOOT_SPI:
         /* do nothing, first the binary must be downloaded */
         break;

      default:
         /* do nothing */
         break;
   }

   RETURN_DEVSTATUS(ret, IFX_NULL);
}


/**
   Wait until the Boot Info Register indicates that boot has finished.

   Poll the Boot Info Register until a value of at least 0x10 is indicated.

   \param  pDev         Pointer to the device structure.
   \param  nLoop        Repetitions of the poll loop.

   \return
   DXS_statusOk or error code.
*/
IFX_int32_t DXS_Wait4BootFinished (
                        DXS_DEVICE_t *pDev,
                        IFX_uint16_t nLoop)
{
   IFX_uint16_t nRegBootInfo = 0x0;

   tapi_debug_buffer_add_user_entry("dxs #%d waiting for boot", pDev->nDevNr);

   while (nLoop > 0)
   {
      /* read the boot state indication */
      IFX_int32_t ret = DXS_RegRead(pDev, DXS_HOST_BINF, &nRegBootInfo);

      /*  A value equal or greater than 0x10 signals that the boot sequence
          has finished successfully and the main routine has started. */
      if ((ret == DXS_statusOk) &&
          ((nRegBootInfo & DXS_REG_BINF_BOOTSTATE_MASK) >= 0x10))
      {
         return DXS_statusOk;
      }

      /*lint -save -e(62) 'udelay' incompatible types for operator ":" */
      /* wait 2ms between register accesses */
      TAPI_OS_MSecSleep(2);
      /* lint -restore */

      nLoop--;
   }

   /* errmsg: Chip boot failed. */
   return DXS_statusBootFailed;
}


/**
   Default initialization of DUSLIC XS registers.

   Enable the required interrupts in the register interface.

   \param pDev pointer to the device structure

   \return
   - DXS_statusOk
   - DXS_statusSpiAccErr
*/
static IFX_int32_t dxs_DefaultRegInit(DXS_DEVICE_t *pDev)
{
   IFX_int32_t ret = DXS_statusErr;
   IFX_uint16_t nRegHostIen1 = 0;
   IFX_uint16_t nRegHostIen2 = 0;

   /*
      Enable the interrupts OBX_RDY and ERR.
   */
   nRegHostIen1 =  DXS_REG_IEN1_RESET;
   nRegHostIen1 |= DXS_REG_IEN1_OBX_RDY;
   nRegHostIen1 |= DXS_REG_IEN1_ERR;
   ret = DXS_RegWrite(pDev, DXS_HOST_IEN1, nRegHostIen1);

   if (DXS_statusOk != ret)
      RETURN_DEVSTATUS(ret, IFX_NULL);

   /*
      Enable the interrupts OBX_UFL and IBX_OFL.
   */
   nRegHostIen2 =  DXS_REG_IEN2_RESET;
   nRegHostIen2 |= DXS_REG_IEN2_OBX_UFL;
   nRegHostIen2 |= DXS_REG_IEN2_IBX_OFL;
   ret = DXS_RegWrite(pDev, DXS_HOST_IEN2, nRegHostIen2);

   RETURN_DEVSTATUS(ret, IFX_NULL);
}


/**
   Create and initalise the structs handling the firmware modules.

   \param  pDev         Pointer to the device structure.

   \return
   - DXS_statusOk
   - DXS_statusInitFwMsgErr - if FW messages could not be initialized
*/
static IFX_int32_t dxs_FW_DevDataInit(DXS_DEVICE_t *pDev)
{
   IFX_uint8_t i = 0;
   IFX_int32_t ret = DXS_statusErr;
   /* System Control is the first message to the FW. */
   DXS_FW_SYS_Control_t *pFW_SysControl = &pDev->fw_sys_ctrl;

   pFW_SysControl->SE      = IFX_DISABLE;
   pFW_SysControl->CP_EN   = IFX_DISABLE;
   pFW_SysControl->CP_VOLT = DXS_FW_SYS_Control_CP_VOLT_5_0V;
   pFW_SysControl->PLIM    = IFX_DISABLE;
   pFW_SysControl->SYCLKE  = IFX_ENABLE;

   ret = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)pFW_SysControl);
   if (!DXS_SUCCESS (ret))
      RETURN_DEVSTATUS (ret, IFX_NULL);

   /* allocate and initialize ALM FW messages */
   for (i = 0; (ret == DXS_statusOk) && (i < pDev->caps.nALI); ++i)
   {
      DXS_CHANNEL_t *pCh = &pDev->pChannel[i];

      ret = DXS_ALM_Allocate_Ch_Structures (pCh);
      if (ret != DXS_statusOk)
      {
         pDev->nErr = DXS_statusInitFwMsgErr;
         /* errmsg: Initializing FW messages failed. */
         RETURN_DEVSTATUS(DXS_statusInitFwMsgErr, IFX_NULL);
      }
      else
      {
         DXS_ALM_InitCh (pCh);
      }
   }

   /* allocate and initialize DTMF/AT FW messages (for dtmf/at generator, DTMF
      dialling and dtmf receiver (if available)) */
   for (i = 0; (ret == DXS_statusOk) && (i < pDev->caps.nDTMFG); ++i)
   {
      DXS_CHANNEL_t *pCh = &pDev->pChannel[i];

      ret = DXS_DTMF_AT_Allocate_Ch_Structures (pCh);
      if (ret != DXS_statusOk)
      {
         pDev->nErr = DXS_statusInitFwMsgErr;
         /* errmsg: Initializing FW messages failed. */
         RETURN_DEVSTATUS(DXS_statusInitFwMsgErr, IFX_NULL);
      }
      else
      {
         DXS_DTMF_AT_InitCh (pCh);
      }
   }

#ifdef DXS_FEAT_CID
   /* allocate and initialize CID sender FW messages */
   for (i = 0; (ret == DXS_statusOk) && (i < pDev->caps.nCIDS); ++i)
   {
      DXS_CHANNEL_t *pCh = &pDev->pChannel[i];

      ret = DXS_CID_Allocate_Ch_Structures(pCh);
      if (ret != DXS_statusOk)
      {
         pDev->nErr = DXS_statusInitFwMsgErr;
         /* errmsg: Initializing FW messages failed. */
         RETURN_DEVSTATUS(DXS_statusInitFwMsgErr,IFX_NULL);
      }
      else
      {
         DXS_CID_InitCh (pCh);
      }
   }
#endif /* DXS_FEAT_CID */

   /* allocate and initialize PCM FW messages */
   for (i = 0;  (ret == DXS_statusOk) && (i < pDev->caps.nPCM); ++i)
   {
      DXS_CHANNEL_t *pCh = &pDev->pChannel[i];

      ret = DXS_PCM_Allocate_Ch_Structures (pCh);

      if (ret != DXS_statusOk)
      {
         pDev->nErr = DXS_statusInitFwMsgErr;
         /* errmsg: Initializing FW messages failed. */
         RETURN_DEVSTATUS(DXS_statusInitFwMsgErr,IFX_NULL);
      }
      else
      {
         DXS_PCM_InitCh (pCh);
      }
   }

   RETURN_DEVSTATUS (ret, IFX_NULL);
}


/**
   Read the firmware version.

   This function can be called as soon as the device struct is prepared and
   the chip access is set. It will store the results in the device structure.

   \param pDev handle to the device structure

   \return
   - DXS_statusOk
   - DXS_statusCmdIbNoSpace
   - DXS_statusCmdMbWrErr
*/
IFX_int32_t DXS_ReadFwVersion (DXS_DEVICE_t *pDev)
{
   IFX_int32_t ret = DXS_CmdRead(pDev, (IFX_uint32_t *)(IFX_void_t *)&pDev->fw_vers,
                            (IFX_uint32_t *)(IFX_void_t *)&pDev->fw_vers);

   if (!DXS_SUCCESS(ret))
   {
      /* errmsg: Failed to read version or capability from FW. */
      RETURN_DEVSTATUS(DXS_statusCapVersUpdateError, IFX_NULL);
   }

   /* Silicon revision are the lower 5 bits of the Device-ID */
   pDev->nChipRev = (IFX_uint8_t) pDev->fw_vers.DEV & 0x1F;
   /* Upper 3 bits of the Device-ID extended by the number of channels will
      identify the chip type. */
   pDev->nDevId = (IFX_uint16_t)
                  (((pDev->fw_vers.DEV >> 4) & 0xE) | pDev->fw_vers.CH);
   /* FW version number as 32-bit value - one digit per byte */
   pDev->nFwRev = (pDev->fw_vers.MAJ << 24) |
                  (pDev->fw_vers.MIN << 16) |
                  (pDev->fw_vers.HF  <<  8);

   return DXS_statusOk;
}


/**
   Read the revision information and capabilities via firmware message.

   \param  pDev         Pointer to the device structure.

   \return
   DXS_statusOk or error code on command read error.
*/
static IFX_int32_t dxs_SetDevCaps (DXS_DEVICE_t *pDev)
{
   IFX_int32_t err = DXS_ReadFwVersion(pDev);
   if (!DXS_SUCCESS(err))
      RETURN_DEVSTATUS(err, IFX_NULL);

   err = DXS_CmdRead(pDev,
                     (uint32_t *)&pDev->fw_caps, (uint32_t *)&pDev->fw_caps);
   if (DXS_statusOk != err)
      RETURN_DEVSTATUS(DXS_statusCapVersUpdateError, IFX_NULL);

   /* store FW capabilities */
   pDev->caps.bfw_MWI       = IFX_TRUE;
   pDev->caps.bfw_ASPwrSave = IFX_TRUE;
   pDev->caps.bfw_GR909     = DXS_BITFIELD_TO_IFX_BOOL(pDev->fw_caps.GR909);
   pDev->caps.bfw_CMeas     = DXS_BITFIELD_TO_IFX_BOOL(pDev->fw_caps.GR909);
   pDev->caps.bfw_ExtLT     = DXS_BITFIELD_TO_IFX_BOOL(pDev->fw_caps.GR909);
   pDev->caps.bfw_combDcDc  = IFX_TRUE;
   pDev->caps.bfw_WbTsSplit = IFX_TRUE;
   pDev->caps.bfw_BuckBoost = IFX_TRUE;
   pDev->caps.bfw_UTD       = IFX_TRUE;

   /* Set capabilities which depend on the device id. */
   if ((pDev->nDevId & 0x01) == 0)
   {
      /* 2-channel device */
      pDev->caps.nALI      = 2;
      pDev->caps.nPCM      = 2;
      pDev->caps.nDTMFG    = 2;
      pDev->caps.nCIDS     = 2;
      pDev->caps.nDTMFR    = 2;
      pDev->caps.nUTD      = 2;
   }
   else
   {
      /* 1-channel device */
      pDev->caps.nALI      = 1;
      pDev->caps.nPCM      = 1;
      pDev->caps.nDTMFG    = 1;
      pDev->caps.nCIDS     = 1;
      pDev->caps.nDTMFR    = 1;
      pDev->caps.nUTD      = 1;
   }


   /* Set flag that the capabilities struct is now filled with values. */
   pDev->bCapsRead = IFX_TRUE;

   TRACE(TAPI_DXS, DBG_LEVEL_LOW,
         ("DXS FW rev %u.%u.%u Capabilities: MWI %d, AS %d, "
          "CMeas %d, ExtLT %d, CombDcDc %d, WbTsSplit %d, Buck/Boost %d, "
          "UTD %d, GR909 %d\n",
         pDev->fw_vers.MAJ, pDev->fw_vers.MIN, pDev->fw_vers.HF,
         pDev->caps.bfw_MWI, pDev->caps.bfw_ASPwrSave,
         pDev->caps.bfw_CMeas, pDev->caps.bfw_ExtLT,
         pDev->caps.bfw_combDcDc, pDev->caps.bfw_WbTsSplit,
         pDev->caps.bfw_BuckBoost, pDev->caps.bfw_UTD,
         pDev->caps.bfw_GR909));

   return DXS_statusOk;
}


/**
   Set all capabilities.

   \param  pDev         Pointer to the device structure.

   \return
   - DXS_statusOk
   - DXS_statusNoMem    no memory could be allocated for capability list

   \remarks
   Macro DXS_MAX_CAPS must match with the capabilities number. So adapt this
   macro accordingly if new capabilities are added.
*/
static IFX_int32_t dxs_AddCaps(DXS_DEVICE_t *pDev)
{
   /* capability list */
   IFX_TAPI_CAP_t *CapList = IFX_NULL;

   /* count the number of entries */
   IFX_uint32_t nCap = 0;

   if (pDev->CapList == IFX_NULL)
   {
      pDev->CapList = (IFX_TAPI_CAP_t*)
                      TAPI_OS_Malloc(DXS_MAX_CAPS * sizeof(IFX_TAPI_CAP_t));
      if (pDev->CapList == IFX_NULL)
      {
         /* errmsg: No memory could be allocated */
         RETURN_DEVSTATUS (DXS_statusNoMem, IFX_NULL);
      }
   }

   CapList = pDev->CapList;

   AddCapability (CapList, &nCap, "INTEL",
                  IFX_TAPI_CAP_TYPE_VENDOR,0);
   AddCapability (CapList, &nCap, "DUSLIC XS",
                  IFX_TAPI_CAP_TYPE_DEVICE, 0);
   AddCapability (CapList, &nCap, "POTS",
                  IFX_TAPI_CAP_TYPE_PORT, IFX_TAPI_CAP_PORT_POTS);
   AddCapability (CapList, &nCap, "PSTN",
                  IFX_TAPI_CAP_TYPE_PORT, IFX_TAPI_CAP_PORT_PSTN);
   AddCapability (CapList, &nCap, "DEVICE TYPE",
                  IFX_TAPI_CAP_TYPE_DEVTYPE, DXS_DEV_TYPE);
   AddCapability (CapList, &nCap, "u-LAW",
                  IFX_TAPI_CAP_TYPE_CODEC, IFX_TAPI_COD_TYPE_MLAW);
   AddCapability (CapList, &nCap, "A-LAW",
                  IFX_TAPI_CAP_TYPE_CODEC, IFX_TAPI_COD_TYPE_ALAW);

   if (pDev->nDevState & DS_DEV_INIT)
   {
      switch (pDev->nChipRev)
      {
         case DXS_V11:
            AddCapability (CapList, &nCap, "DEVICE VERSION",
                           IFX_TAPI_CAP_TYPE_DEVVERS, 0x0101);
            break;

         case DXS1_V12:
            /*lint -fallthrough */
         case DXS2_V12:
            AddCapability (CapList, &nCap, "DEVICE VERSION",
                           IFX_TAPI_CAP_TYPE_DEVVERS, 0x0102);
            break;

         default:
            /* unknown */
            AddCapability (CapList, &nCap, "DEVICE VERSION",
                           IFX_TAPI_CAP_TYPE_DEVVERS, 0x0000);
            break;
      }
   }

   if (pDev->bCapsRead)
   {
      AddCapability (CapList, &nCap, "PCM",
                     IFX_TAPI_CAP_TYPE_PCM, pDev->caps.nPCM);
      AddCapability (CapList, &nCap, "Phones",
                     IFX_TAPI_CAP_TYPE_PHONES, pDev->caps.nALI);
   }
   else
   {
      AddCapability (CapList, &nCap, "PCM",
                     IFX_TAPI_CAP_TYPE_PCM, 2);
      AddCapability (CapList, &nCap, "Phones",
                     IFX_TAPI_CAP_TYPE_PHONES, 2);
   }

   /* check if the array is not out of its limit */
   TAPI_ASSERT(nCap <= DXS_MAX_CAPS);
   pDev->nMaxCaps = (IFX_uint8_t)nCap;

   dxs_PrintCaps(CapList, nCap);

   return DXS_statusOk;
}


/**
   Print all capabilities.

   \param  pCapList  Pointer to the capabilities list
   \param  nCap  Number of capabilities to printout

   \return
   - DXS_statusOk
   - DXS_statusParam invalid input parameter
*/
static IFX_int32_t dxs_PrintCaps(const IFX_TAPI_CAP_t *const pCapList, IFX_uint32_t nCap)
{
   #define CAPS_BUF_LENGTH 512
   IFX_uint16_t buf_length = CAPS_BUF_LENGTH;
   IFX_char_t buf_caps[CAPS_BUF_LENGTH] = {0};
   IFX_char_t *curr_ptr = &buf_caps[0];
   IFX_uint32_t i = 0;

   if (pCapList == IFX_NULL)
   {
      return DXS_statusParam;
   }

   if (nCap == 0)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("No Duslic caps for printout!\n"));
      return DXS_statusParam;
   }

   /* Generate string with all caps */
   for (i = 0; i < nCap; ++i)
   {
      IFX_uint32_t added_string_size;

      /* 10 characters added for "[%d:%d] , " assuming numbers are max 2 digits each */
      IFX_char_t one_cap_buff[sizeof(pCapList[0].desc) + 10] = {0};

      (IFX_void_t) snprintf(one_cap_buff, sizeof(one_cap_buff), "[%d:%d] %s, ",
                              pCapList[i].captype, pCapList[i].cap,
                              pCapList[i].desc);

      added_string_size = strlen(one_cap_buff);

      if (buf_length <= added_string_size)
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH, 
            ("No more space in buffer for Duslic cap printout!\n"));
         break;
      }

      strncpy(curr_ptr, one_cap_buff, added_string_size);
      curr_ptr += added_string_size;
      buf_length -= added_string_size;
   }

   /* Remove last comma */
   if (*(curr_ptr + strlen(curr_ptr) - 2) == ',')
      *(curr_ptr + strlen(curr_ptr) - 2) = '\0';

   TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("DXS Cap: %s\n", buf_caps));

   return DXS_statusOk;
}


/**
   Add capability to given list.

   Writes a capability to the capability list entry that is addressed with
   nCap and increments nCap afterwards. Previous data in this slot is
   overwritten.

   \param  CapList      Pointer to the capability list.
   \param  pnCap        Pointer to an integer used as index to the list.
                        The integer is automatically post-incremented by one.
   \param  description  C-string with a text describing the capability.
   \param  type         Type ID of the capability
   \param  value        Value (amount) of the capability.
*/
static IFX_void_t AddCapability (IFX_TAPI_CAP_t* CapList,
                                 IFX_uint32_t *pnCap,
                                 IFX_char_t const *description,
                                 IFX_int32_t type,
                                 IFX_int32_t value)
{
   IFX_uint32_t capnr;

   if (pnCap == IFX_NULL)
      return;

   if (*pnCap >= DXS_MAX_CAPS)
      return;

   capnr = (*pnCap);
   /* Note: strncpy will terminate the target string with \0 if the source
      string is longer than length parameter */
   strncpy (CapList[capnr].desc, description, sizeof(CapList[0].desc) - 1);
   CapList[capnr].captype = (IFX_TAPI_CAP_TYPE_t)type;
   CapList[capnr].cap = value;
   CapList[capnr].handle = (IFX_int32_t)capnr;
   (*pnCap) = capnr + 1;
}


/**
   Returns the number of entries in the capability list.

   \param pLLDev  pointer to LL device structure

   \return The number of capability entries.
*/
static IFX_int32_t  DXS_TAPI_LL_Phone_Get_Capabilities (
                                 IFX_TAPI_LL_DEV_t *pLLDev)
{
   const DXS_DEVICE_t *pDev = pLLDev;

   if (pLLDev == IFX_NULL)
      return 0;

   return (pDev->nMaxCaps);
}


/**
   Returns DUSLIC XS's capability lists.

   \param  pLLDev       Pointer to DXS_DEVICE_t structure.
   \param  pCapList     Pointer to IFX_TAPI_CAP_LIST_t structure with details
                        where to copy the data to. No more capabilities than
                        specified in the element nCap will be copied into the
                        memory given in this structure.

   \return
   - DXS_statusParam if at least one parameter in is wrong
   - DXS_statusErr   if channel pointer is null
   - DXS_statusOk    if successful
*/
static IFX_int32_t DXS_TAPI_LL_Phone_Get_Capability_List (
                        IFX_TAPI_LL_DEV_t *pLLDev,
                        IFX_TAPI_CAP_LIST_t *pCapList)
{
   const DXS_DEVICE_t *pDev = pLLDev;

   if (pLLDev == IFX_NULL)
      return DXS_statusErr;

   if (pCapList == IFX_NULL)
      return DXS_statusParam;

   /* The count of entries given in the parameter is the limit. Should the
      internal list have less entries than this reduce the number of entries
      to be copied to this smaller number. */
   if (pDev->nMaxCaps < pCapList->nCap)
   {
      pCapList->nCap = pDev->nMaxCaps;
   }

   TAPI_OS_CpyKern2Usr (
      pCapList->pList,
      pDev->CapList, /*lint !e64 */
      pCapList->nCap * sizeof (*pDev->CapList));

   return DXS_statusOk;
}


/**
   Checks in the capability list if a specific capability is supported.

   \param  pLLDev       Pointer to DXS_DEVICE_t structure
   \param  pCapList     Pointer to IFX_TAPI_CAP_t structure

   \return Support status of the capability
   - 0 if not supported
   - 1 if supported

   \remarks
   This function compares only the captype and the cap members of the given
   IFX_TAPI_CAP_t structure with the ones of DUSLIC XS.
*/
static IFX_int32_t DXS_TAPI_LL_Phone_Check_Capability (
                                 IFX_TAPI_LL_DEV_t *pLLDev,
                                 IFX_TAPI_CAP_t    *pCapList)
{
   DXS_DEVICE_t *pDev = pLLDev;
   IFX_int32_t cnt;
   IFX_int32_t ret = 0;

   if((pDev == IFX_NULL) || (pCapList == IFX_NULL))
      return ret;

   /* do checks */
   for (cnt = 0; cnt < pDev->nMaxCaps; cnt++)
   {
      if (pCapList->captype == pDev->CapList[cnt].captype)
      {
         switch (pCapList->captype)
         {
         /* Handle number counters, cap is returned */
         case IFX_TAPI_CAP_TYPE_PCM:
         case IFX_TAPI_CAP_TYPE_CODECS:
         case IFX_TAPI_CAP_TYPE_PHONES:
         case IFX_TAPI_CAP_TYPE_DEVVERS:
         case IFX_TAPI_CAP_TYPE_DEVTYPE:
            pCapList->cap = pDev->CapList[cnt].cap;
            ret = 1;
            break;
         case IFX_TAPI_CAP_TYPE_DEVICE:
         case IFX_TAPI_CAP_TYPE_VENDOR:
            strncpy(pCapList->desc, pDev->CapList[cnt].desc, sizeof(pCapList->desc) - 1);
            ret = 1;
            break;
         default:
            /* default IFX_TRUE or IFX_FALSE capabilities */
            if (pCapList->cap == pDev->CapList[cnt].cap)
               ret = 1;
            break;
         }
      }
   }
   return ret;
}


/**
   DUSLIC XS device driver initialization.
   This is the device driver initialization function to call at the system
   startup prior any access to the DUSLIC XS device driver.
   After the initialization the device driver is ready to be accessed by
   the application. The global structure "DXS_dev_ctx" contains all the data
   handling the interface (open, close, ioctl,...).

   \return
   - IFX_SUCCESS        If successful.
   - IFX_ERROR          if registration with HL failed.
*/
IFX_int32_t DXS_DeviceDriverStart(void)
{
   IFX_int32_t result = 0;

   /* Set the default trace level */
   TAPI_TRACE_LEVEL_SET(TAPI_DXS, debug_level);

   /* Get the coding of the allowed DC/DC converter type. */
   nAllowedDcDcType = DXS_BBD_DcDcStringTranslate(DXS_DCDC_TYPE);
   if ((dcdc_type[0] != '\0') &&
       (nAllowedDcDcType != DXS_DCDC_TYPE_AUTO)
      )
   {
      /* Setting the type on cmdline is not allowed if not compiled for EVAL. */
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("DXS driver start: Setting DC/DC HW type on the cmdline not "
             "allowed when driver is already configured.\n"));
      return DXS_statusDrvInitFail;
   }

   if (nAllowedDcDcType == DXS_DCDC_TYPE_NOTSET)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("DXS driver start: Configuration of DC/DC HW type invalid.\n"));
      return DXS_statusDrvInitFail;
   }

   memset(pDrvCtx, 0, sizeof(*pDrvCtx));

   /* Initialize the function pointers structure and register
      with the High Level TAPI */
#ifdef TAPI_LINUX_USER_SPACE
   pDrvCtx->majorNumber       = DXS_MAJOR;
   pDrvCtx->minorBase         = DXS_MINOR_BASE;
#else /* TAPI_LINUX_USER_SPACE */
   pDrvCtx->majorNumber       = major;
   pDrvCtx->minorBase         = minorBase;
#endif /* TAPI_LINUX_USER_SPACE */

   pDrvCtx->devNodeName       = "dxs"; /* devName; */
   pDrvCtx->maxDevs           = DXS_MAX_DEVICES;
   pDrvCtx->maxChannels       = DXS_MAX_CH_NR;

   /* procfs info */
   pDrvCtx->drvName           = DXS_DEV_NAME;
   pDrvCtx->drvVersion        = DXS_DRV_VER_STR;
   pDrvCtx->hlLLInterfaceVersion = DRV_LL_INTERFACE_VER_STR;

   /* Generic functions  */
   pDrvCtx->Prepare_Dev       = DXS_TAPI_LL_DevicePrepare;
   pDrvCtx->Init_Dev          = DXS_TAPI_LL_DeviceInit;
   pDrvCtx->Exit_Dev          = DXS_TAPI_LL_DeviceExit;

   pDrvCtx->Prepare_Ch        = DXS_TAPI_LL_ChannelPrepare;
   pDrvCtx->Init_Ch           = DXS_TAPI_LL_ChannelInit;

   pDrvCtx->FW_Start          = DXS_TAPI_LL_FW_Start;
   pDrvCtx->FW_Init           = DXS_TAPI_LL_FW_Init;
   pDrvCtx->BBD_Dnld          = DXS_TAPI_LL_BBD_Dnld;

   pDrvCtx->Ioctl             = DXS_Dev_Spec_Ioctl;

#ifdef TAPI_FEAT_LX_COMPAT
   pDrvCtx->CompatIoctl       = DXS_Dev_Spec_Compat_Ioctl;
#endif
#if 0
   pDrvCtx->GetCmdMbxSize     = DXS_TAPI_LL_GetCmdMbxSize;
#endif

   pDrvCtx->CAP_Number_Get    = DXS_TAPI_LL_Phone_Get_Capabilities;
   pDrvCtx->CAP_List_Get      = DXS_TAPI_LL_Phone_Get_Capability_List;
   pDrvCtx->CAP_Check         = DXS_TAPI_LL_Phone_Check_Capability;

#ifdef DXS_HAVE_INTERRUPTS
   /* IRQ information */
   pDrvCtx->IRQ.LockDevice    = DXS_IrqLockDevice;
   pDrvCtx->IRQ.UnlockDevice  = DXS_IrqUnlockDevice;
   pDrvCtx->IRQ.IrqEnable     = DXS_IrqEnable;
   pDrvCtx->IRQ.IrqDisable    = DXS_IrqDisable;
#else /* DXS_HAVE_INTERRUPTS */
   pDrvCtx->IRQ.LockDevice    = IFX_NULL;
   pDrvCtx->IRQ.UnlockDevice  = IFX_NULL;
   pDrvCtx->IRQ.IrqEnable     = IFX_NULL;
   pDrvCtx->IRQ.IrqDisable    = IFX_NULL;
#endif /* DXS_HAVE_INTERRUPTS */

   /* PCM related */
   DXS_PCM_Func_Register (&pDrvCtx->PCM);

   /* ALM specific */
   DXS_ALM_Func_Register (&pDrvCtx->ALM);

#ifdef DXS_FEAT_NLT
   /* NLT specific */
   DXS_NLT_Func_Register (&pDrvCtx->NLT);
#endif

#ifdef DXS_FEAT_CID
   pDrvCtx->SIG.CID_TX_Start  = DXS_TAPI_LL_CID_TX_Start;
   pDrvCtx->SIG.CID_TX_Stop   = DXS_TAPI_LL_CID_TX_Stop;
#endif /* DXS_FEAT_CID */

   /* DTMF receiver, always running if supported */
   pDrvCtx->SIG.DTMF_RxCoeff  = DXS_TAPI_LL_DTMF_RX_CFG;

#if defined(TAPI_LINUX_USER_SPACE) || defined(VXWORKS)
   pDrvCtx->Open              = IFX_NULL;
   pDrvCtx->Release           = IFX_NULL;
#else
   pDrvCtx->Open              = IFX_NULL;
   pDrvCtx->Release           = IFX_NULL;
#endif

   /* Register this driver with the HL-TAPI driver
      this also registers the driver context for use by the HL-TAPI */
   result = IFX_TAPI_Register_LL_Drv(pDrvCtx);
   if (result != IFX_SUCCESS)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("DXS driver start: registration failed\n"));
      return result;
   }

   /* Start and initialize the realtime kernel thread and it's resources
      to handle the interrupt routine for all devices handled by this driver.
      Note: not to be done before the register where the device structs are
      prepared. */
   DXS_interrupt_init();

#ifdef TAPI_LINUX_KERNEL_SPACE
   printk(KERN_DEFAULT "%s, (c) 2023-2024, MaxLinear, Inc.\n",
          &TAPI_DXS_DRV_WHATVERSION[4]);
#else
   printf("%s, (c) 2023-2024, MaxLinear, Inc.\n\r",
          &TAPI_DXS_DRV_WHATVERSION[4]);
#endif

#ifdef EVENT_LOGGER_DEBUG
   {
      IFX_int32_t i = 0;

      /* Register driver with device name, type and number to the event logger */
      for (i = 0; i < pDrvCtx->maxDevs; i++)
      {
         EL_REG_Register(DXS_DEV_NAME, DEV_TYPE_DUSLIC_XS,
                         i /* dev num */, IFX_NULL /* cb func */);
      }
   }
#endif /* EVENT_LOGGER_DEBUG */

   return IFX_SUCCESS;
}


/**
   DUSLIC XS device driver shutdown.
*/
IFX_void_t DXS_DeviceDriverStop (void)
{
   IFX_uint8_t    i;

   for (i=0; i < DXS_MAX_DEVICES; i++)
   {
      /* resource cleanup */
      dxs_DeviceExit(&DxsDevices[i], IFX_TRUE);

      /* Free the capabilities list */
      if (DxsDevices[i].CapList != IFX_NULL)
      {
         TAPI_OS_Free (DxsDevices[i].CapList);
      }
   }

   /* terminate interrupt processing kernel thread */
   DXS_interrupt_exit();
   /* Unregister this driver from the HL-TAPI driver
      this also unregisters the device driver context */
   IFX_TAPI_Unregister_LL_Drv ((&DrvCtx)->majorNumber);

   /* Note: the device struct array is a static variable so no free is needed.*/

#ifdef EVENT_LOGGER_DEBUG
   /* Unregister the driver with device name, type and number
    * from the event logger */
   EL_REG_Unregister(DXS_DEV_NAME, DEV_TYPE_DUSLIC_XS, -1 /* dev num (-1 all) */);
#endif /* EVENT_LOGGER_DEBUG */
}
