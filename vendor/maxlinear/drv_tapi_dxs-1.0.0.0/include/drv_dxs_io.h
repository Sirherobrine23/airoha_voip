#ifndef _DRV_DXS_IO_H
#define _DRV_DXS_IO_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_io.h
   This file contains the defines specific to the DUSLIC XS driver interface
   and is used by applications.
*/

/** @defgroup DXS_DRIVER_INTERFACE DUSLIC XS Driver Interface
    Lists the entire interface to the DUSLIC XS Driver. */
/*@{*/

/** \defgroup DXS_DRIVER_INTERFACE_BASIC Basic Interface
    Basic DUSLIC XS access routines as command read/write and initialisation. */

/** \defgroup DXS_DRIVER_INTERFACE_INIT Driver Initialization Interface
    Interface needed to initialise the driver and the devices. */

/*@}*/ /* DXS_DRIVER_INTERFACE */


/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_io_types.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/** DUSLIC XS maximum channel number */
#define DXS_MAX_CH_NR            2
/** DUSLIC XS maximum analog channel number */
#define DXS_ANA_CH_NR            2

/** \addtogroup DXS_DRIVER_INTERFACE_INIT */
/*@{*/
/** Flag for \ref DXS_IO_Init_t to avoid firmware download */
#define DXS_NO_FW_DWLD          0x00000001
/** Flag for \ref DXS_IO_Init_t to avoid ASDSP download */
#define DXS_NO_ASDSP_DWLD       0x00000002
/*@}*/ /* TAPI_INTERFACE_CONTMEASUREMENT */


/* ============================= */
/* DUSLIC XS ioctl Defines       */
/* ============================= */

/* magic number */
#define DXS_IOC_MAGIC 'D'

/* IOCMD global defines */

/** \addtogroup DXS_DRIVER_INTERFACE_BASIC */
/*@{*/

/**
   Read relevant version information.
   The parameter points to a \ref DXS_IO_Version_t structure */
#define FIO_DXS_VERS                         _IOR (DXS_IOC_MAGIC,  4, DXS_IO_Version_t)

#ifndef TAPI_ONE_DEVNODE
#define FIO_DXS_DRVVERS                      _IOR (DXS_IOC_MAGIC,  5, DXS_IO_Version_t)
#endif /* TAPI_ONE_DEVNODE */


/**
   Set the driver report levels if the driver is compiled with
   ENABLE_TRACE

   \remarks valid arguments are:

   \arg 0: off
   \arg 1: low, high output
   \arg 2: normal, general information and warnings
   \arg 3: high, only errors are reported
*/
#define FIO_DXS_REPORT_SET                   _IOW (DXS_IOC_MAGIC, 6, IFX_uint32_t)


#ifdef DEBUG
/**
   Write a DUSLIC XS command.
   The parameter points to a \ref DXS_IO_MB_CMD_t structure.

   \note
   This interface is only for debugging and testing purposes.
   The use of this interface may disturb the driver operation
*/
#define FIO_DXS_WCMD                         _IOW (DXS_IOC_MAGIC, 7, DXS_IO_MB_CMD_t)


/**
   Read command.
   The parameter points to a \ref DXS_IO_MB_CMD_t structure

   \note
   This interface is only for debugging and testing purposes.
   The use of this interface may disturb the driver operation

   \code
   DXS_IO_MB_CMD_t ioCmd;
   int err;

   ioCmd.cmd1 = 0x8300 | ch;
   // read OPMODE_CUR
   ioCmd.cmd2 = 0x2101;

   err = ioctl (fd, FIO_DXS_RCMD, (INT) &ioCmd);
   \endcode
*/
#define FIO_DXS_RCMD                         _IOWR (DXS_IOC_MAGIC, 8, DXS_IO_MB_CMD_t)


/*@}*/ /* DXS_DRIVER_INTERFACE_BASIC */

/**
   Read host register (DUSLIC XS).
   The parameter points to a \ref DXS_IO_RegAccess_t structure

   \note
   This interface is only for debugging and testing purposes.
   Using of this interface may disturb the driver operation

   \code
   DXS_IO_REG_ACCESS_t ioCmd;
   int err;

   ioCmd.offset = 0x0C; // register STAT_INT

   err = ioctl (fd, FIO_DXS_RDREG, (INT) &ioCmd);
   \endcode
*/
#define FIO_DXS_RDREG                        _IOWR (DXS_IOC_MAGIC, 38, DXS_IO_RegAccess_t)


/**
   Write ro host register (DUSLIC XS)
   The parameter points to a \ref DXS_IO_RegAccess_t structure

   \note
   This interface is provided for debugging and testing purposes.
   Using of this interface may disturb the driver operation
   */
#define FIO_DXS_WRREG                        _IOW (DXS_IOC_MAGIC, 39, DXS_IO_RegAccess_t)

#endif /* DEBUG */

/**
   Important: The use of this interface is deprecated. The chip reset is not
   a function of the DXS driver. The controller should provide means to
   manipulate chip reset line. This interface will be removed in future
   versions.
   Board specific reset of the DUSLIC XS chip
   \tapiv3
   The parameter is either 0 (deactivate reset) or 1 (activate reset)
   \endtapiv3
   \tapiv4
   The reset can be is either deactivated or activated.
   Look at parameter description for more details.
   \endtapiv4

   \note
   This interface calls the CHIP_RESET macro defined in drv_config_user.h
*/
#define FIO_DXS_CHIP_RESET                   _IOW (DXS_IOC_MAGIC, 41, DXS_ChipReset_t)

/*@}*/


/** \addtogroup DXS_DRIVER_INTERFACE_INIT */
/*@{*/

/** Initialize DUSLIC XS Device driver information for one chip.
    Does some required settings, which must be done before any
    other chip access will work!
    Parameter is a pointer to a \ref DXS_BasicDeviceInit_t structure.
*/
#define FIO_DXS_BASICDEV_INIT                _IOW (DXS_IOC_MAGIC, 200, DXS_BasicDeviceInit_t)

/**
   Reset Vinetic Device driver internal structure for one chip.
   This ioctl must be called after each vinetic hard reset not leading
   to a vinetic basic device initialization.
*/
#ifdef TAPI_ONE_DEVNODE
   #define FIO_DXS_DEV_RESET                    _IOW (DXS_IOC_MAGIC, 201, DXS_DevReset_t)
#else /* TAPI_ONE_DEVNODE */
   #define FIO_DXS_DEV_RESET                    _IO (DXS_IOC_MAGIC, 201)
#endif /* TAPI_ONE_DEVNODE */

/**
   Does a download according to bbd format.
   Parameter is a pointer to a \ref DXS_BBD_Download_t structure.
   \tapiv3
   This structure is a generic bbd library structure.
   \endtapiv3
*/
#define FIO_DXS_BBD_DOWNLOAD                 _IOW (DXS_IOC_MAGIC, 202, DXS_BBD_Download_t)

/**
   Does a download of firmware.
   Parameter is a pointer to a \ref DXS_FW_Download_t structure.
*/
#define FIO_DXS_FW_DOWNLOAD                  _IOW (DXS_IOC_MAGIC, 203, DXS_FW_Download_t)
/*@}*/ /* DXS_DRIVER_INTERFACE_INIT */


/* =================================== */
/*     DUSLIC XS ioctl access          */
/* =================================== */

/* general mode settings (set or get information) */
#define  IOSET    0
#define  IOGET    1
#define  IOMODIFY 2

#endif /* _DRV_DXS_IO_H */
