#ifndef _DRV_DXS_IRQ_H
#define _DRV_DXS_IRQ_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_irq.h
   This file contains the declaration of all interrupt specific messages.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
extern enum IFX_irqreturn_t irq_DXS_interrupt_routine(DXS_DEVICE_t *pDev);
extern IFX_return_t DXS_interrupt_init(void);
extern IFX_return_t DXS_interrupt_exit(void);
extern IFX_void_t DXS_PollingModeTimerStart(void);
extern irqreturn_t DXS_irq_handler(int irq, void *pDev);
extern irqreturn_t DXS_irq_thread_handler(int irq, void *pDev);

#endif /* _DRV_DXS_IRQ_H */
