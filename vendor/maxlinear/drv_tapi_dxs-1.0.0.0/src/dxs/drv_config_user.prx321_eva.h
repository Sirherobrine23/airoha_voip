#ifndef _DRV_CONFIG_USER_H
#define _DRV_CONFIG_USER_H
/******************************************************************************

  Copyright 2014      Lantiq Deutschland GmbH
  Copyright 2019      Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_config_user.prx321_eva.h
   This file is intended to customize system specific settings of the driver.

   In case of optional features it may also contain some required definitions
   where no default values are making sense.

   This file will only be used if the compiler switch ENABLE_USER_CONFIG
   is defined (e.g. by "configure --enable-user-config")

   \remark
   This file (drv_config_user.prx321_eva.h) is specifically configured for
   the PRX321 EVA board.

   To use this file, make a copy or link with the name "drv_config_user.h"
   in your build-directory.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#ifdef LINUX
   #include <linux/spi/spi.h>
#endif /* LINUX */

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */


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
#define SPI_MAXBYTES_SIZE  16

/** SPI initialisation
    This macro is called during chip initialisation (if SPI is used) and
    can be extended to do board specific initialisation related to the
    DuSLIC-xS.
 */
#define SPI_INIT(pDev) do {                                                    \
         DXS_SPI_drvRegister(pDev);                                            \
} while(0);

#define SPI_EXIT(pDev) do {                                                    \
         DXS_SPI_drvUnregister(pDev);                                          \
} while(0);

#define DXS_SPI_READ_HALF_DUPLEX 1

/** SPI chip select set/unset.
 *  Define empty macro as CS pin is controlled by the PRX321 HW.
*/
#define SPI_CS_SET(nDevNr, high_low) do {                                      \
} while(0);

/* spi low level access function */
#define spi_ll_read_write(pDev,txptr,txsize,rxptr,rxsize)                      \
            spi_write_then_read (pDev->pSpiDev,                                \
                  (const u8 *)(const IFX_uint8_t*)(txptr),                     \
                  (unsigned)(IFX_uint32_t)(txsize),                            \
                  (u8 *)(IFX_uint8_t*)(rxptr),                                 \
                  (unsigned)(IFX_uint32_t)(rxsize))


/**
   Macro to map the system function with takes care of interrupt handling
   registration.

   \param irq  irq number

   \param func interrupt handler callback function

   \param arg  argument of interrupt handler callback function

   \remarks
   The macro is by default mapped to the operating system method. For systems
   integrating different routines, this macro must be adapted in the user con-
   figuration header file.
*/
#ifdef TAPI_SYS_REGISTER_INT_HANDLER
   #undef TAPI_SYS_REGISTER_INT_HANDLER
#endif /* DXT_SYS_REGISTER_INT_HANDLER */

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

#endif /* _DRV_CONFIG_USER_H */

#define DXS_DCDC_AUTODETECT_TID
#define DXS_DCDC_AUTODETECT_FUNC(dev,ret) \
               dxs_dcdc_autodetect((dev),(ret))

