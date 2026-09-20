#ifndef _DRV_CONFIG_USER_H
#define _DRV_CONFIG_USER_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/** \file drv_config_user.h
   This file is intended to customize system specific settings of the driver.

   In case of optional features it may also contain some required definitions
   where no default values are making sense.

   This file will only be used if the compiler switch ENABLE_USER_CONFIG
   is defined (e.g. by "configure --enable-user-config")

   \remark
   This file (drv_config_user.default.h) is intended as a template.
   All options are commented in detail, defined with their default values
   (where possible) and disabled with an "#if 0 / #endif" block.

   To use this file, make a copy with the name "drv_config_user.h" in your
   build- or source-directory and enable (#if 1) the options you want to change.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
/* add your includes here */

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/** Macro to signal and set an error.
   Useful to generate a trigger signal during hardware debugging! */
#if 0
#define SET_ERROR(no)                     \
   do {                                   \
      /* do not change following line! */ \
      pDev->nErr = (IFX_int32_t)(no);      \
      IFXOS_ASSERT(IFX_FALSE);            \
   } while(0)
#endif


/* ============================== */
/*  SPI bus driver configuration  */
/* ============================== */

/* Define the maximum number of bytes that the SPI interface can transfer in
   one one call of the spi_ll_read_write() macro below.
   The driver will fragment all data during read or write transfer into pieces
   no larger than the value given here. To avoid overhead due to fragmentation
   use the maximum value the SPI interface can support.
   The minimum possible value is 4 bytes. There is no upper limit but values
   larger than 66 bytes are never used by the DXS driver.
*/
#define SPI_MAXBYTES_SIZE  66

/* SPI initialisation */
#if 0
#define SPI_INIT(pDev) do {                                    \
   switch (pDev->nDevNr)                                       \
   {                                                           \
      case 0:                                                  \
         /* initialise SPI access to device 0 here */          \
         /* this means SPI driver and HW port pins */          \
         break;                                                \
      case 1:                                                  \
         /* initialise SPI access to device 1 here */          \
         /* this means SPI driver and HW port pins */          \
         break;                                                \
      default:                                                 \
         break;                                                \
   }                                                           \
} while(0)
#endif

/* SPI uninitialisation */
#if 0
#define SPI_EXIT(pDev) do {                                    \
   switch (pDev->nDevNr)                                       \
   {                                                           \
      case 0:                                                  \
         /* uninitialise SPI access to device 0 here */        \
         /* this means SPI driver and HW port pins */          \
         break;                                                \
      case 1:                                                  \
         /* uninitialise SPI access to device 1 here */        \
         /* this means SPI driver and HW port pins */          \
         break;                                                \
      default:                                                 \
         break;                                                \
   }                                                           \
} while(0)
#endif /* 0 */

/** SPI CS (chip select) set/unset before low-level SPI reads/writes.
   \param  high_low 0 = activate the DXS by setting chip select line
                    1 = deactivate the DXS by setting chip select line
   \remark The DXS chip select line CS_N is low active.

   \note This macro should be defined as empty if control of the CS pin is done
         by the SPI bus driver or SPI controller hardware.
   \note This macro can be use to handle the devices which cannot be handled by
         the SPI bus or hardware (e.g. due to limited number of CS pins). Macro
         can be written to drive GPIO pins only for particular DXS devices with
         certain "nDevNo"s.
*/
#define SPI_CS_SET(nDevNo, high_low)  \
   do                                 \
   {                                  \
      if ((high_low) == 0)            \
      {                               \
         /* set CS_N = LOW */         \
      }                               \
      else                            \
      {                               \
         /* set CS_N = HIGH */        \
      }                               \
   } while(0)

/* Enable this when the SPI driver returns the data in the receive buffer
   always at offset 0 regardless how much data was written.
   Disable this when the SPI driver returns the data in the receive buffer
   at the offset following the one where the write data ended.
*/
#if 0
#define DXS_SPI_READ_HALF_DUPLEX
#endif

/** spi low level access function

   \param pDev   - Pointer to dev struct
   \param txptr  - transmit buffer pointer
   \param txsize - transmit buffer size
   \param rxptr  - receive buffer pointer
   \param rxsize - receive buffer size

   \remark
      This macro must map your spi low level read/write
      access routine. It should return IFX_SUCCESS (0)
      if the SPI access was successful and IFX_ERROR (-1)
      in case of any error.
*/
#define spi_ll_read_write(pDev,txptr,txsize,rxptr,rxsize) \
   /*yourSPI_Send ((pDev->pSpiDev),(txptr),(txsize),(rxptr),(rxsize))*/ \
   0 /* return value - just for compile tests */


/* ============================== */
/* Polling mode configuration     */
/* ============================== */
/* Polling mode allows to operate the DXS without an interrupt line. In this
   mode the DXS is polled at regular intervalls to retrieve events that are
   waiting in the DXS device. The time for these cycles is configured with
   the define below. Shorten this time in case that events get lost. Make
   it longer in order to reduce the traffic on the SPI bus and and reduce the
   load on the controller that is executing the diver. */
#define DXS_POLL_CYCLE_MS   10 /* ms */


/* ============================== */
/*  Interrupt line configuration  */
/* ============================== */

/** Configure the interrupt line on the IO port and/or IRQ controller.
    Interrupt generation from this line is also enabled by calling this. */
#if 0
#define IRQLINE_INIT(pDev)                                     \
   if (pDev->nDevNr == 0) {                                    \
      /* initialise and enable IRQ line for device 0 here */   \
   }                                                           \
   else if (pDev->nDevNr == 1) {                               \
      /* initialise and enable IRQ line for device 1 here */   \
   }
#endif /* 0 */

/** Disable the interrupt generation for this line and unconfigure the line
    on the IO port and IRQ controller. */
#if 0
#define IRQLINE_EXIT(pDev)                                     \
   if (pDev->nDevNr == 0) {                                    \
      /* disable and free the IRQ line for device 0 here */    \
   }                                                           \
   else if (pDev->nDevNr == 1) {                               \
      /* disable and free the IRQ line for device 1 here */    \
   }
#endif /* 0 */

/** Macro to disable the interrupt line of given irq number.
    \remark
       Define this macro in case the operating system methods
       as defined in sys_drv_ifxos.h aren't suitable for your system.
       (i.e FPGA controls interrupts)
*/
#if 0
#define TAPI_DISABLE_IRQLINE(irq)                IFXOS_IRQ_DISABLE(irq)
#endif

/** Macro to enable the interrupt line of given irq number.

    \remark
       Define this macro in case the operating system methods
       as defined in sys_drv_ifxos.h aren't suitable for your system.
       (i.e FPGA controls interrupts)
*/
#if 0
#define TAPI_ENABLE_IRQLINE(irq)                 IFXOS_IRQ_ENABLE(irq)
#endif

/** Macro to disable the globale interrupt.

    \remark
       Define this macro in case the operating system methods
       as defined in sys_drv_ifxos.h aren't suitable for your system.
       (i.e FPGA controls interrupts)
*/
#if 0
#define TAPI_DISABLE_IRQGLOBAL(var)              IFXOS_LOCKINT(var)
#endif

/** Macro to enable the globale interrupt.

    \remark
       Define this macro in case the operating system methods
       as defined in sys_drv_ifxos.h aren't suitable for your system.
       (i.e FPGA controls interrupts)
*/
#if 0
#define TAPI_ENABLE_IRQGLOBAL(var)               IFXOS_UNLOCKINT(var)
#endif

/** Macro to map the system function with takes care of interrupt handling
    registration.

    \param irq   -  irq number
    \param func  -  interrupt handler callback function
    \param arg   -  argument of interrupt handler callback function

    \remarks
      The macro is by default mapped to the operating system method. For systems
      integrating different routines, this macro must be adapted in the user
      configuration header file.

      This macro may have different arguments set according to the requirements
      of the system or operating system used.
*/
#if 0
#define TAPI_SYS_REGISTER_INT_HANDLER(irq,func,arg)             \
            intConnect(INUM_TO_IVEC(irq), (VOIDFUNCPTR)(func), \
            (IFX_int32_t)(arg))
#endif

/** Macro to map the system function with takes care of interrupt handling
    unregistration.

    \param irq - irq number

    \remarks
      The macro is by default mapped to the operating system method. For systems
      integrating different routines, this macro must be adapted in the user
      configuration header file.

      This macro may have different arguments set according to the requirements
      of the system or operating system used.
*/
#if 0
#define TAPI_SYS_UNREGISTER_INT_HANDLER(irq)  \
         TAPI_SYS_REGISTER_INT_HANDLER((irq), OS_IRQHandler_Dummy, (irq))
#endif

/* ============================== */
/*  Reset line configuration      */
/* ============================== */

/** Macro to set the reset line.

    \param pDev  - identifies which device to reset usually not needed
                   because there is only one chip.
    \param reset - transmit buffer size
*/
#define CHIP_RESET(pDev, reset) do {                      \
   if (!reset) /* 0 - deactivate reset */                 \
   {                                                      \
      /* code to deactivate the reset line */             \
   }                                                      \
   else /* 1 - activate reset */                          \
   {                                                      \
      /* code to activate the reset line */               \
   }                                                      \
} while(0)


/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */


#endif /* _DRV_CONFIG_USER_H */
