/******************************************************************************

  Copyright 2014-2015 Lantiq Deutschland GmbH
  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017, 2020 Intel Corporation.
  Copyright 2021-2023 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_linux.c
   This file contains the implementation of linux specific driver functions.
*/

#ifdef LINUX
/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include <drv_tapi_osmap.h>
#include "drv_dxs_api.h"
#include "drv_dxs_access.h"
#include "drv_dxs_init.h"
#include "drv_dxs_alm_priv.h"
#include "drv_dxs_linux.h"

#include "../tapi/drv_tapi_ll_interface.h"

#include <linux/module.h>
#include <linux/init.h>
#include <linux/of_gpio.h>

#include <linux/version.h>

#include <linux/delay.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0))
   #include <linux/sched.h>
#else
   #include <linux/sched/types.h>
   #include <linux/sched/signal.h>
#endif

#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/of.h>

#include "../tapi/drv_tapi_linux.h"
#include "../tapi/drv_tapi_debug_buffer.h"

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */
IFX_uint16_t major      = 0; /* default for dynamic allocation */
IFX_uint16_t minorBase  = DXS_MINOR_BASE;
IFX_char_t *devName     = "dxs";

#if defined(ENABLE_TRACE) && defined(TAPI_LINUX_KERNEL_SPACE)
   /* Debug level variable to use when setting debug level via module parameter */
   extern IFX_uint32_t debug_level;
#endif

extern IFX_char_t *dcdc_type;

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/*lint -save -e528 -e19 -e546 */
MODULE_DESCRIPTION("Standalone TAPI for DXS device driver");
MODULE_AUTHOR("MaxLinear, Inc.");
MODULE_LICENSE("Dual BSD/GPL");

module_param(major, ushort, 0);
module_param(minorBase, ushort, 0);
module_param(devName, charp, 0);
#ifdef ENABLE_TRACE
   module_param(debug_level, uint, 0);
#endif /* ENABLE_TRACE */
module_param(dcdc_type, charp, 0);

MODULE_PARM_DESC(major, "Device Major number");
MODULE_PARM_DESC(minorBase, "Minor number of the node for the first device");
MODULE_PARM_DESC(devName, "Basename of the devicenode");
#ifdef ENABLE_TRACE
   MODULE_PARM_DESC(debug_level, "Debug level: 1 (verbose) - 4 (no) trace output");
#endif /* ENABLE_TRACE */
MODULE_PARM_DESC(dcdc_type, "DC/DC HW type ID string");
/*lint restore*/


/**
   Race condition free implementation for IFXOS_WaitEvent_timeout
   \param  pDev         Reference to device context.

   \return
   - DXS_statusOk: Data available for reading.
   - DXS_statusReadInterrupted: Waiting interrupted by signal.
   - DXS_statusCmdObTimeout: Timeout occurred while waiting for data.

   \remarks
   Porting instructions

   Different OS implement the wait queue mechanism in different ways. The
   major difference is the way they handle a wakeup which occurs before
   the thread is completely sleeping. Linux is known not to handle this
   "early" wakeup properly, i.e. we'll sleep forever - or when we are lucky
   the same event occurs again. To prevent this case, we need to check a
   condition "half the way to sleep" - where we are still able to rollback
   our preparations to sleep.
   If your OS doesn't require this special handling, you can continue using
   IFXOS_WaitEvent_timeout - or your specific port of this macro - (and
   ignore the additional condition in pDev->bOutBoxData).
*/
IFX_int32_t DXS_WaitForCmdMbxData(DXS_DEVICE_t *pDev)
{
   int ret = wait_event_interruptible_timeout(pDev->obxDataEvt.object,
                                              pDev->bOutBoxData,
                                              msecs_to_jiffies(3000));

   if (ret == 0)
   {
      /* errmsg: Timeout while waiting for data in command outbox. */
      return DXS_statusCmdObTimeout;
   }
   else if (ret < 0)
   {
      /* errmsg: CmdRead interrupted by signal. */
      return DXS_statusReadInterrupted;
   }

   return DXS_statusOk;
}


/**
   Waiting for completion of SDD Operating Mode change or timeout.

   \param  pCh          Pointer to DXS channel context.

   \return  >0      wakeup occurred, remaining jiffies
             0      timeout
            <0      interrupted by a signal
*/
IFX_int32_t DXS_WaitForSddOpmodeChEvt(DXS_CHANNEL_t *pCh)
{
   return wait_event_interruptible_timeout(pCh->pALM->sdd_event.object,
                                           !pCh->pALM->bOpmodeChangePending,
                                           msecs_to_jiffies(SDD_EVT_TIMEOUT_MS));
}


IFX_void_t DXS_GPIO_Reset(DXS_DEVICE_t *pDev, int reset)
{
#ifdef TAPI_LINUX_KERNEL_SPACE
   if (!IS_ERR_OR_NULL(pDev->pResetGpio))
   {
      /* Logical reset: respect GPIO_ACTIVE_LOW/HIGH from Device Tree. */
      gpiod_set_value_cansleep(pDev->pResetGpio, !!reset);
      msleep(pDev->nResetInterval);
   }
#else
   TAPI_UNUSED(pDev);
   TAPI_UNUSED(reset);
#endif
}


/**
   Initialize the module.

   \return 0 if successful or an negative error code otherwise.

   \remarks
   Called by the kernel.
*/
static int __init dxs_module_init(void)
{
   IFX_int32_t ret;

   #ifdef TAPI_FEAT_DEBUG_BUFFER
      if (tapi_debug_buffer_init())
      {
         printk(KERN_ERR "TAPI debug buffer init failed!\n");
      }
   #endif

   /* Initialize TAPI HL driver before DXS to let DXS register in it. */
   if (ifx_tapi_module_init())
   {
      printk(KERN_ERR "TAPI (DXS) driver start failed!\n");
      return -1;
   }

   ret = DXS_DeviceDriverStart();
   if (ret != DXS_statusOk)
   {
      ifx_tapi_module_exit();
      return -EIO;
   }
   ret = DXS_SPI_ModuleInit();
   if (ret)
   {
      DXS_DeviceDriverStop();
      ifx_tapi_module_exit();
   }
   return ret;
}


/**
   Clean up the module when unloaded.

   \remarks
   Called by the kernel.
*/
static void __exit dxs_module_exit(void)
{
   DXS_SPI_ModuleExit();

   DXS_DeviceDriverStop();

   TRACE(TAPI_DXS, DBG_LEVEL_LOW, ("Cleaned up %s module.\n", DXS_DEV_NAME));

   ifx_tapi_module_exit();

   #ifdef TAPI_FEAT_DEBUG_BUFFER
      tapi_debug_buffer_free();
   #endif
}

/*lint -save -e19 -e546*/
module_init(dxs_module_init);
module_exit(dxs_module_exit);

/*lint -restore */
#endif /* LINUX */
