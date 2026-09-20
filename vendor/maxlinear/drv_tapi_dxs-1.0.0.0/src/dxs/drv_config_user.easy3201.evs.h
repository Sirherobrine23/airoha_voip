#ifndef _DRV_CONFIG_USER_H
#define _DRV_CONFIG_USER_H
/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016, Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/** \file drv_config_user.easy3201.evs.h
   Configuration for EASY3201 EVS (Evaluation system).
   This file is intended to customize system specific settings of the driver.

   This file will only be used if the compiler switch ENABLE_USER_CONFIG
   is defined (e.g. by "configure --enable-user-config")
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#ifdef LINUX
   #define DXS_BROKEN_LX_MIPS_BSP_DEVICETREE
   #include <linux/device.h>
   #include <linux/platform_device.h>
   #include "linux/spi/spi.h"
   #include <asm/ifx/irq.h>
   #include <asm/ifxmips/ifxmips_gpio.h>
#endif /* LINUX */


/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

#undef DXS_DCDC_AUTODETECT_TID
#undef DXS_DCDC_AUTODETECT_EASY32002_EXT
#define DXS_DCDC_AUTODETECT_FUNC(dev,ret)

/* ============================== */
/*  SPI bus driver configuration  */
/* ============================== */

/* The SVIP SPI interface buffer has a size of 32 bytes. This is also the
   maximum number of bytes that the SVIP SPI driver accepts in each call of
   spi_write_then_read() below. */
#define SPI_MAXBYTES_SIZE  32

/* The DXS device irq is connected to these SVIP external interrupt numbers. */
#define DXS_EXINT_DEV0   16   /* DXS device 0 */
#define DXS_EXINT_DEV1   9    /* DXS device 1 */

#ifdef LINUX
/* SPI initialisation */
#define SPI_INIT(pDev) \
   DXS_SPI_drvRegister(pDev)

/* SPI uninitialisation */
#define SPI_EXIT(pDev) \
   DXS_SPI_drvUnregister(pDev);

/* SPI chip select set/unset. */
#define SPI_CS_SET(devNo, high_low)
   /* empty macro above - the HW will drive the CS line itself */

/* SPI driver returns the data always at offset 0 */
#define DXS_SPI_READ_HALF_DUPLEX

/* spi low level access function */
#define spi_ll_read_write(pDev,txptr,txsize,rxptr,rxsize)   \
            spi_write_then_read (pDev->pSpiDev,             \
                  (const u8 *)(const IFX_uint8_t*)(txptr),  \
                  (unsigned)(IFX_uint32_t)(txsize),         \
                  (u8 *)(IFX_uint8_t*)(rxptr),              \
                  (unsigned)(IFX_uint32_t)(rxsize))
#endif /* LINUX */

#ifdef VXWORKS

#include "ifx_reg_base.h"
#include "svip16Gpio.h"

#define __GPIO_DIR_OUT(port,pin) \
   (*(SVIP16_P ## port ## _DIR) |= (1 << pin))
#define __GPIO_DIR_IN(port,pin) \
   (*(SVIP16_P ## port ## _DIR) &= ~(1 << pin))
#define __GPIO_ALTSEL0_SET(port,pin) \
   (*(SVIP16_P ## port ## _ALTSEL0) |= (1 << pin))
#define __GPIO_ALTSEL0_CLEAR(port,pin) \
   (*(SVIP16_P ## port ## _ALTSEL0) &= ~(1 << pin))
#define __GPIO_ALTSEL1_SET(port,pin) \
   (*(SVIP16_P ## port ##_ALTSEL1) |= (1 << pin))
#define __GPIO_ALTSEL1_CLEAR(port,pin) \
   (*(SVIP16_P ## port ##_ALTSEL1) &= ~(1 << pin))
#define __GPIO_EXINTCR0_SET(port,pin) \
   (*(SVIP16_P ## port ##_EXINTCR0) |= (1 << pin))
#define __GPIO_EXINTCR0_CLEAR(port,pin) \
   (*(SVIP16_P ## port ##_EXINTCR0) &= ~(1 << pin))
#define __GPIO_IRNEN_SET(port,pin) \
   (*(SVIP16_P ## port ##_IRNEN) |= (1 << pin))
#define __GPIO_IRNEN_CLEAR(port,pin) \
   (*(SVIP16_P ## port ##_IRNEN) &= ~(1 << pin))
#define __GPIO_IRNEN_SET(port,pin) \
   (*(SVIP16_P ## port ##_IRNEN) |= (1 << pin))
#define __GPIO_IRNICR_SET(port,pin) \
   (*(SVIP16_P ## port ##_IRNICR) |= (1 << pin))

/* SPI initialisation */
#define SPI_INIT(pDev)             \
   if (pDev->nDevNr == 1) {        \
      __GPIO_DIR_OUT(0, 11);       \
      __GPIO_ALTSEL0_CLEAR(0, 11); \
      __GPIO_ALTSEL1_CLEAR(0, 11); \
   }

/* SPI uninitialisation */
#define SPI_EXIT(pDev)

/* SPI chipselect set/unset */
#define SPI_CS_SET(devNo, high_low) IFX_SSC_Cs(devNo, high_low)

/* spi low level access function */
#define spi_ll_read_write(pDev, txptr,txsize,rxptr,rxsize)  \
                  IFX_SCC_SpiXfer(     \
                  (IFX_uint8_t*)(txptr),              \
                  (IFX_uint32_t)(txsize),             \
                  (IFX_uint8_t*)(rxptr),              \
                  (IFX_uint32_t)(rxsize))

/* setting SPI mode */
#define DXS_SPI_MODE_SET(pDev, mode) IFX_SSC_ModeSet0(mode)

/* setting SPI baudrate */
#define DXS_SPI_BAUDRATE_SET(pDev, baudrate) IFX_SSC_BaudrateSet0(baudrate)

#define IRQLINE_INIT(pDev)                  \
do {                                        \
   __GPIO_DIR_IN(0,19);        \
   __GPIO_ALTSEL0_SET(0,19);   \
   __GPIO_ALTSEL1_CLEAR(0,19); \
   __GPIO_DIR_IN(0,16);        \
   __GPIO_ALTSEL0_CLEAR(0,16); \
   __GPIO_ALTSEL1_CLEAR(0,16); \
   sysGpioIntConfig(((pDev)->nDevNr == 1) ? \
      DXS_EXINT_DEV1 : DXS_EXINT_DEV0, 1);  \
   sysGpioIntEnable(((pDev)->nDevNr == 1) ? \
      DXS_EXINT_DEV1 : DXS_EXINT_DEV0);     \
} while (0)

extern INT32 IFX_SSC_Cs (IFX_uint8_t devNr, INT32 nState);
extern IFX_void_t IFX_SSC_ModeSet0(IFX_int32_t mode);
extern STATUS IFX_SSC_BaudrateSet0(UINT32 baudrate);
extern INT32 IFX_SCC_SpiXfer (unsigned char *txptr, unsigned long txsize,
                          unsigned char *rxptr, unsigned long rxsize);
extern IFX_int32_t pInterruptCounters[];
#endif /* VXWORKS */

/* ============================== */
/* Polling mode configuration     */
/* ============================== */
/* Polling mode allows to operate the DXS without an interrupt line. In this
   mode the DXS is polled at regular intervals to retrieve events that are
   waiting in the DXS device. The time for these cycles is configured with
   the define below. Shorten this time in case that events get lost. Make
   it longer in order to reduce the traffic on the SPI bus and and reduce the
   load on the controller that is executing the diver. */
#define DXS_POLL_CYCLE_MS   10 /* ms */


/* ============================== */
/*  Interrupt line configuration  */
/* ============================== */

#ifdef LINUX
/* This defines to which IRQ generation mode the SVIP ports where the DXS
   IRQ line is connected are configured. */
/* Please check the function 'ifx_enable_external_int' and consult the SVIP
   HW guide before changing this value. */
#define DXS_EXINT_MODE 1 /* Interrupt on falling edge */

/* Configure and enable the interrupt line on the Port and IRQ controller. */
#define IRQLINE_INIT(pDev)                                     \
   if (pDev->nDevNr == 0) {                                    \
      ifx_enable_external_int(DXS_EXINT_DEV0, DXS_EXINT_MODE); \
   }                                                           \
   else if (pDev->nDevNr == 1) {                                     \
/* FIXME: the next line has no effect on P0.16 register bits */      \
      /* ifx_enable_external_int(DXS_EXINT_DEV1, DXS_EXINT_MODE); */ \
/* workaround: use ifxmips_port API */                               \
      ifxmips_port_reserve_pin(0, 16);                               \
      ifxmips_port_set_dir_in(0, 16);                                \
      ifxmips_port_clear_altsel0(0, 16);                             \
      ifxmips_port_clear_altsel1(0, 16);                             \
      ifxmips_port_set_exintcr0(0, 16);                              \
      ifxmips_port_set_irnen(0, 16);                                 \
/* end of workaround */                                              \
   }

/* Disable the interrupt line on the Port and IRQ controller. */
#define IRQLINE_EXIT(pDev)                                      \
   if (pDev->nDevNr == 0) {                                     \
      ifx_disable_external_int(DXS_EXINT_DEV0);                 \
   }                                                            \
   else if (pDev->nDevNr == 1) {                                \
/* FIXME: the next line has no effect on P0.16 register bits */ \
      /* ifx_disable_external_int(DXS_EXINT_DEV1); */           \
/* workaround: use ifxmips_port API */                          \
      ifxmips_port_clear_irnen(0, 16);                          \
      ifxmips_port_clear_exintcr0(0, 16);                       \
      ifxmips_port_free_pin(0, 16);                             \
/* end of workaround */                                         \
   }
#endif


/* ============================== */
/*  Reset line configuration      */
/* ============================== */

/* Not needed on the EASY3201 EVS because there is a board driver for this. */
/**
   Macro to set the reset line.

   \param  pDev         Identifies which device to reset.
   \param  reset        0 to deactivate the reset, <>0 to activate the reset.
*/
#define CHIP_RESET(pDev, reset)

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

#endif /* _DRV_CONFIG_USER_H */
