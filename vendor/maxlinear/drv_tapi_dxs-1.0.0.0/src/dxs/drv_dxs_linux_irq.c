#ifdef LINUX

#include <drv_tapi_config.h>

#ifdef DXS_HAVE_INTERRUPTS

#include "drv_dxs_irq.h"

#include <drv_tapi_osmap.h>
#include <ifx_types.h>


#ifndef DXS_FEAT_LINUX_THREADED_IRQ
   /* operating system callback for polling/interrupt */
   static irqreturn_t OS_IRQ_Wrapper(IFX_int32_t irq, IFX_void_t *pDev);
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */


/* list of DUSLIC XS interrupts, necessary for shared interrupt handling */
static DXS_IRQ_t *DXS_irq_head = IFX_NULL;


#ifndef DXS_FEAT_LINUX_THREADED_IRQ
/**
   IRQ handler

   \param  irq          IRQ number. (unused)
   \param  pDev         Pointer to the device structure.

   \return IFX_IRQ_HANDLED
*/
static irqreturn_t OS_IRQ_Wrapper(IFX_int32_t irq, IFX_void_t *pDev)
{
   TAPI_UNUSED(irq);

   return irq_DXS_interrupt_routine((DXS_DEVICE_t*)pDev);
}
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */


/**
   Install the Interrupt Handler.

   \param  pDev         Pointer to DUSLIC XS device structure.
   \param  nIrq         Interrupt number.
*/
IFX_void_t OS_Install_DxsIRQHandler(DXS_DEVICE_t *pDev,
                                    IFX_int32_t nIrq)
{
/*lint -save -esym(529,intstatus) -esym(530,intstatus) */
   DXS_IRQ_t         *pIrq,
                     *pIrqPrev = IFX_NULL;
   DXS_DEVICE_t      *tmp_pDev;
   TAPI_OS_INTSTAT    intstatus;
   IFX_int32_t        err;

   /* Install interrupt handler for irq */
   if (pDev->pIrq != IFX_NULL)
      OS_UnInstall_DxsIRQHandler(pDev);

   pIrq = DXS_irq_head;

   /* try to find existing irq with same number */
   while (pIrq != IFX_NULL)
   {
      pIrqPrev = pIrq;
      if (pIrq->nIrq == nIrq)
         break;
      pIrq = pIrq->next_irq;
   }

   if (pIrq == IFX_NULL)
   {
      /* IRQ number not yet registered */

      pIrq = TAPI_OS_Malloc(sizeof(*pIrq));
      if (pIrq == IFX_NULL)
         return;

      memset(pIrq, 0, sizeof(*pIrq));
      TAPI_OS_MutexInit(&pIrq->mtxIrqAcc);
      TAPI_OS_MutexGet(&pIrq->mtxIrqAcc);

      TAPI_DISABLE_IRQGLOBAL(intstatus);

      /* Install the Interrupt routine */
#ifdef DXS_FEAT_LINUX_THREADED_IRQ
      err = TAPI_SYS_REGISTER_INT_HANDLER_THREAD(
                        (IFX_uint32_t)nIrq,
                        (IRQF_TRIGGER_LOW | IRQF_SHARED),
                        DXS_irq_handler,
                        DXS_irq_thread_handler,
                        pIrq);
#else
      err = TAPI_SYS_REGISTER_INT_HANDLER(
                        (IFX_uint32_t)nIrq,
                        (IRQF_DISABLED | IRQF_SHARED),
                        OS_IRQ_Wrapper,
                        pDev);
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */

      if (err == 0)
      {
#ifndef DXS_FEAT_LINUX_THREADED_IRQ
         TAPI_DISABLE_IRQLINE_NOSYNC((IFX_uint32_t)nIrq);
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */

         /* New dedicated or first shared interrupt. */
         pIrq->pdev_head = pDev;
         pIrq->next_irq = IFX_NULL;
         pIrq->nIrq = nIrq;
         pIrq->bRegistered = IFX_TRUE;

         pDev->pIrq = pIrq;
         pDev->pInt_NextDev = IFX_NULL;

         /* Add to the global list of interrupts. */
         /* this list is not used by the interrupt handler itself,
            so no additional intLock is necessary! */
         if (DXS_irq_head == IFX_NULL)
         {
            DXS_irq_head = pIrq;
         }
         else
         {
            pIrq->next_irq = pIrqPrev->next_irq;
            pIrqPrev->next_irq = pIrq;
         }
      }

      TAPI_ENABLE_IRQGLOBAL(intstatus);

      if (err != 0)
      {
         pDev->pIrq = IFX_NULL;
         TAPI_OS_MutexDelete(&pIrq->mtxIrqAcc);
         TAPI_OS_Free(pIrq);

         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("DXS ERROR: request interrupt failed, 0x%04X\n", (-err)));
      }
   }
   else
   {
      TAPI_DISABLE_IRQGLOBAL(intstatus);

      /* Install the Interrupt routine (add to shared irq list) */
#ifndef DXS_FEAT_LINUX_THREADED_IRQ
      err = TAPI_SYS_REGISTER_INT_HANDLER (
                        (IFX_uint32_t)nIrq,
                        (IRQF_DISABLED | IRQF_SHARED),
                        OS_IRQ_Wrapper,
                        pDev);
      if (err == 0)
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */
      {
         /* add into the list of an existing shared interrupt */
         pDev->pIrq = pIrq;
         pDev->pInt_NextDev = IFX_NULL;
         tmp_pDev = pIrq->pdev_head;
         while (tmp_pDev->pInt_NextDev != IFX_NULL)
         {
            tmp_pDev = tmp_pDev->pInt_NextDev;
         }
         tmp_pDev->pInt_NextDev = pDev;
      }

      TAPI_ENABLE_IRQGLOBAL(intstatus);

#ifndef DXS_FEAT_LINUX_THREADED_IRQ
      if (err != 0)
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("DXS ERROR: request shared interrupt failed, 0x%04X\n", (-err)));
      }
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */
   }
/*lint -restore */
}


/**
   Uninstall the Interrupt Handler

   \param  pDev         Pointer to DUSLIC XS device structure.
*/
IFX_void_t OS_UnInstall_DxsIRQHandler(DXS_DEVICE_t *pDev)
{
   DXS_IRQ_t         *pIrq,
                     *pIrqPrev;
   DXS_DEVICE_t      *tmp_pDev;

   if (pDev->pIrq == IFX_NULL)
      return;

   /* uninstall regular interrupt */
   pIrq = pDev->pIrq;

   /* remove this pDev from the list of this pIrq */
   if (pIrq->pdev_head == pDev)
   {
      pIrq->pdev_head = pDev->pInt_NextDev;
   }
   else
   {
      /* find the element which has this pDev as next */
      tmp_pDev = pIrq->pdev_head;
      while (tmp_pDev->pInt_NextDev != pDev)
      {
         if (tmp_pDev->pInt_NextDev == IFX_NULL)
         {
            TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("DXS ERROR: device not found in interrupt list, cannot uninstall!\n"));
            return;
         }
         tmp_pDev = tmp_pDev->pInt_NextDev;
      }
      tmp_pDev->pInt_NextDev = pDev->pInt_NextDev;
   }
   pDev->pInt_NextDev = IFX_NULL;
   pDev->pIrq = IFX_NULL;

   /* if this was the last device, so cleanup the irq */
   if (pIrq->pdev_head == IFX_NULL)
   {
      /* IFXOS_MutexLock(pIrq->mtxIrqAcc); */
#ifndef DXS_FEAT_LINUX_THREADED_IRQ
      if (pIrq->bIntEnabled == IFX_TRUE)
      {
         TAPI_DISABLE_IRQLINE_NOSYNC((IFX_uint32_t)pIrq->nIrq);
      }
      if (pIrq->bRegistered == IFX_TRUE)
      {
         TAPI_SYS_UNREGISTER_INT_HANDLER((IFX_uint32_t)pIrq->nIrq, pDev);
      }
#else /* DXS_FEAT_LINUX_THREADED_IRQ */
      if (pIrq->bRegistered == IFX_TRUE)
      {
         TAPI_SYS_UNREGISTER_INT_HANDLER((IFX_uint32_t)pIrq->nIrq, pIrq);
      }
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */

      /* remove this pIrq from irq-list */
      if (DXS_irq_head == pIrq)
      {
         DXS_irq_head = pIrq->next_irq;
      }
      else
      {
         pIrqPrev = DXS_irq_head;
         while (pIrqPrev->next_irq != IFX_NULL)
         {
            if (pIrqPrev->next_irq == pIrq)
               break;
            pIrqPrev = pIrqPrev->next_irq;
         }
         if (pIrqPrev->next_irq != IFX_NULL)
            pIrqPrev->next_irq = pIrq->next_irq;
      }
      TAPI_OS_MutexDelete(&pIrq->mtxIrqAcc);
      TAPI_OS_Free(pIrq);
   }

#ifndef DXS_FEAT_LINUX_THREADED_IRQ
   else
   {
      TAPI_SYS_UNREGISTER_INT_HANDLER((IFX_uint32_t)pIrq->nIrq, pDev);
   }
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */
}


/**
   Disables irq line if the driver is in interrupt mode and irq line is
   actually enabled according to device flag bIntEnabled.
   Disable the global interrupt if the device is not in polling mode and no
   interrupt line is connected.

   \param pLLDev pointer to DUSLIC XS device

   \return
   None.

   \remarks
   If the driver works in Polling mode, nothing is done.
   If the driver works in interrupt mode and the irq was already disabled
   (flag bIntEnabled is IFX_FALSE ), the os disable function will
   not be called.

   Very important: It is assumed that disable and enable irq are done
   subsequently and that no routine calling disable/enable is executed
   in-between as stated in following code example:

   \verbatim
   Allowed :

   DXS_IrqLockDevice;
   .. some instructions
   DXS_IrqUnlockDevice

   Not allowed:

   routineX (IFX_int32_t x)
   {
      DXS_IrqLockDevice;
      .. some instructions;
      DXS_IrqUnlockDevice
   }

   routineY (IFX_int32_t y)
   {
      DXS_IrqLockDevice;
      routine (x);    <---------------- routineX unlocks interrupts..
      be carefull!
      ... some more instructions;
      DXS_IrqUnlockDevice;
   }
   \endverbatim
*/
IFX_void_t DXS_IrqLockDevice (IFX_TAPI_LL_DEV_t *pLLDev)
{
   DXS_DEVICE_t      *pDev = (DXS_DEVICE_t *) pLLDev;

   if (pDev != IFX_NULL)
   {
      DXS_IRQ_t         *pIrq = pDev->pIrq;

      if ((pIrq != IFX_NULL) &&
          (pDev->nEventHandlingMode == DXS_EVENT_INTERRUPT))
      {
         /* device specific interrupt routine is initialized */
         /* interrupt line was disabled already: exit */
         if (pIrq->bIntEnabled == IFX_FALSE)
            return;
         /* it must be possible to take the mutex before disabling the irq */
         if (!in_interrupt())
            TAPI_OS_MutexGet(&pIrq->mtxIrqAcc);
         /* invoke board or os routine to disable irq */
         TAPI_DISABLE_IRQLINE_NOSYNC((IFX_uint32_t)pIrq->nIrq);
         /* reset enable flag to signalize that interrupt line is disabled */
         pIrq->bIntEnabled = IFX_FALSE;
      }
      else
      {
         /* there is no specific interrupt service routine configured for that
            device. Disable the global interrupts instead */
         if (pDev->nIrqMask == 0)
         {
            TAPI_DISABLE_IRQGLOBAL(pDev->nIrqMask);
         }
      }
   }
}

/**
   Enables the irq line if the driver is in interrupt mode and irq line is
   actually disabled according to device flag bIntEnabled.
   Enable the global interrupt, if this was disabled by 'DXS_IrqDisable'

   \param pLLDev  pointer to DUSLIC XS device

   \return
   None.
*/
IFX_void_t DXS_IrqEnable (IFX_TAPI_LL_DEV_t *pLLDev)
{
   DXS_DEVICE_t   *pDev = (DXS_DEVICE_t *) pLLDev;

   if (pDev != IFX_NULL)
   {
      DXS_IRQ_t      *pIrq = pDev->pIrq;

      if ((pIrq != IFX_NULL) &&
          (pDev->nEventHandlingMode == DXS_EVENT_INTERRUPT))
      {
         /* invoke board or os routine to enable irq */
         TAPI_ENABLE_IRQLINE((IFX_uint32_t)pIrq->nIrq);
      }
   }
}

/**
   Disables irq line if the driver is in interrupt mode and irq line is
   actually enabled according to device flag bIntEnabled.
   Disable the global interrupt, if this was enabled by 'DXS_IrqEnable'

   \param pLLDev  pointer to DUSLIC XS device

   \return
   None.
*/
IFX_void_t DXS_IrqDisable (IFX_TAPI_LL_DEV_t *pLLDev)
{
   DXS_DEVICE_t   *pDev = (DXS_DEVICE_t *) pLLDev;

   if (pDev != IFX_NULL)
   {
      DXS_IRQ_t      *pIrq = pDev->pIrq;

      if ((pIrq != IFX_NULL) &&
          (pDev->nEventHandlingMode == DXS_EVENT_INTERRUPT))
      {
         /* invoke board or os routine to disable irq */
         TAPI_DISABLE_IRQLINE_NOSYNC((IFX_uint32_t)pIrq->nIrq);
      }
   }
}

/**
   Enables irq line if the driver is in interrupt mode and irq line is
   actually disabled according to device flag bIntEnabled.
   Enable the global interrupt, if this was disabled by 'DXS_IrqLockDevice'.

   \param pLLDev  pointer to DUSLIC XS device

   \return
   None.

   \remarks
   cf Remarks of DXS_IrqLockDevice () in this file.
*/
IFX_void_t DXS_IrqUnlockDevice (IFX_TAPI_LL_DEV_t *pLLDev)
{
   DXS_DEVICE_t      *pDev = (DXS_DEVICE_t *) pLLDev;

   if (pDev != IFX_NULL)
   {
      DXS_IRQ_t         *pIrq = pDev->pIrq;

      if ((pIrq != IFX_NULL) &&
          (pDev->nEventHandlingMode == DXS_EVENT_INTERRUPT))
      {
         /* device specific interrupt routine is initialized */
         /* interrupt line was enabled already: exit */
         if (pIrq->bIntEnabled == IFX_TRUE)
            return;
         /* set enable flag to signalize that interrupt line is enabled */
         pIrq->bIntEnabled = IFX_TRUE;
         /* invoke board or os routine to enable irq */
         TAPI_ENABLE_IRQLINE((IFX_uint32_t)pIrq->nIrq);
         TAPI_OS_MutexRelease(&pIrq->mtxIrqAcc);
      }
      else
      {
         /*
            there is no specific interrupt service routine configured for that
            device. Enable the global interrupts again
         */
         /*lint -save -esym(529,nMask) */
         if (pDev->nIrqMask != 0)
         {
            TAPI_OS_INTSTAT nMask = pDev->nIrqMask;
            pDev->nIrqMask = 0;
            TAPI_ENABLE_IRQGLOBAL(nMask);
         }
         /*lint -restore */
      }
   }
}
#endif /* DXS_HAVE_INTERRUPTS */

#endif /* LINUX */
