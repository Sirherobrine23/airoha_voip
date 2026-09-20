#ifndef DRV_TAPI_IO_H
#define DRV_TAPI_IO_H
/******************************************************************************

  Copyright (c) 2006-2009 Infineon Technologies AG
  Copyright (c) 2009-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017     Intel Corporation.
  Copyright 2023-2024     MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_io.h
   Contains TAPI I/O defines, enums and structures according to LTAPI
   specification.
   TAPI ioctl defines
      The file is divided in sections ioctl commands, constants, enumerations,
      structures for the following groups :
      - TAPI_INTERFACE_INIT
      - TAPI_INTERFACE_OP
      - TAPI_INTERFACE_METER
      - TAPI_INTERFACE_TONE
      - TAPI_INTERFACE_SIGNAL
      - TAPI_INTERFACE_CID
      - TAPI_INTERFACE_MISC
      - TAPI_INTERFACE_EVENT
      - TAPI_INTERFACE_RINGING
      - TAPI_INTERFACE_CALIBRATION
      - TAPI_INTERFACE_PCM
      - TAPI_INTERFACE_MWL
*/

#ifdef __cplusplus
   extern "C" {
#endif

#include "drv_tapi_if_version.h"

/* Use this macro to check TAPI_VERSION_CODE against a specified version. */
#define TAPI_API_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

#ifdef TAPI_VERSION3
   #define TAPI_VERSION_CODE TAPI_API_VERSION(3,2,14)
   #undef TAPI_ONE_DEVNODE
   #ifdef TAPI_DXY_DOC
      #define TAPI3_DXY_DOC
   #endif /* TAPI_DXY_DOC */
#endif /* TAPI_VERSION3 */

#ifdef TAPI_VERSION4
   #define TAPI_VERSION_CODE TAPI_API_VERSION(4,1,12)
   #ifndef TAPI_ONE_DEVNODE
      #define TAPI_ONE_DEVNODE
   #endif /* TAPI_ONE_DEVNODE */
   #ifdef TAPI_DXY_DOC
      #define TAPI4_DXY_DOC
   #endif /* TAPI_DXY_DOC */
#endif /* TAPI_VERSION4 */

#if defined (TAPI_VERSION3) && defined (TAPI_VERSION4)
   #error Only one TAPI API version can be specifed
#endif /* TAPI_VERSION3 */

#if !defined (TAPI_VERSION3) && !defined (TAPI_VERSION4)
   #error !!! Please specify TAPI API version!!! \
   To define the TAPI API version the file >drv_tapi_if_version.h< must be \
   created. Either configure drv_tapi with autotools or create it manually. \
   To generate the file manually, navigate to the include subdirectory of \
   drv_tapi, which also contains the file drv_tapi_io.h. There you will find \
   two template files. Use >drv_tapi_if_version.v3< for CPE products and \
   >drv_tapi_if_version.v4< for AN products. Copy one of these files to the \
   filename >drv_tapi_if_version.h< in the same folder and compile again.
#endif /* !defined (TAPI_VERSION3) && !defined (TAPI_VERSION4) */

#ifndef __PACKED__
   #if defined (__GNUC__) || defined (__GNUG__)
      /* GNU C or C++ compiler */
      #define __PACKED__ __attribute__ ((packed))
   #elif !defined (__PACKED__)
      #define __PACKED__      /* nothing */
   #endif
#endif


/* Dummy variable to make structure size compatible with normal TAPI driver */
#define TAPI_COMPATIBILITY_FILL(bytes) IFX_uint8_t dummy[(bytes)]


/** \defgroup TAPI_INTERFACE TAPI Ioctl and Functions Reference
    This chapter describes all the services that can be used via the TAPI. The ioctl
    commands are described by means of the return values for each function.
    The chapter is organized as follows: */
/**@{*/

/** \defgroup TAPI_INTERFACE_CONTMEASUREMENT Analog Line Continuous Measurement
    Contains services to perform continuous measurement on the analog line.
    These measurements do not influence normal telephone operation. */

/** \defgroup TAPI_INTERFACE_CALIBRATION Calibration Services
      Calibration interfaces. */

/** \defgroup TAPI_INTERFACE_CID CID Features Services
      Contains services for configuring, sending and receiving the caller ID. */

/** \defgroup TAPI_INTERFACE_EVENT Event Reporting Services
    Contains services for event reporting. This is applicable to device file
    descriptors unless otherwise stated. */

#ifndef TAPI4_DXY_DOC
/** \defgroup TAPI_INTERFACE_PHONE_DETECTION FXS Phone Detection
    Contains services to detect a connected telephone on an FXS line. */
#endif /* #ifndef TAPI4_DXY_DOC */

#ifndef TAPI4_DXY_DOC
/** \defgroup TAPI_INTERFACE_GR909 GR-909 Services
   Contains services and mechanisms to perform measurements according
   to the GR-909 standard. */
#endif /* #ifndef TAPI4_DXY_DOC */

/** \defgroup TAPI_INTERFACE_INIT Initialization Services
   These services set the default initialization of the device and hardware.*/

/** \defgroup TAPI_INTERFACE_MWL Message Waiting Lamp Services
    Contains services for the message waiting lamp. */

/** \defgroup TAPI_INTERFACE_METER Metering Services
    Contains services for metering.
    All metering services apply to phone channels unless otherwise stated. */

/** \defgroup TAPI_INTERFACE_MISC Miscellaneous Services
    Contains services for status and version information. */

/** \defgroup TAPI_INTERFACE_NLT Network Line Testing Services
    Contains services to perform network line testing. These services are
    used in conjunction with the TAPI NLT library provided by Lantiq. */

/** \defgroup TAPI_INTERFACE_OP Operation Control Services
    Modifies the operation of the device.
    All operation control services apply to phone channels unless otherwise
    stated.  */

/** \defgroup TAPI_INTERFACE_PCM PCM Services
    Contains services for PCM configuration.
    Applies to phone channels unless otherwise stated. */

/** \defgroup TAPI_INTERFACE_RINGING Power Ringing Services
      Ringing on FXS interfaces. */

/** \defgroup TAPI_INTERFACE_SIGNAL Signal Detection Services
    Contains services for the detection of tones.
   The application handles the different states of the detection status. */

/** \defgroup TAPI_INTERFACE_TEST Testing Services
    Contains services for system tests such as hook generation and loops. */

/** \defgroup TAPI_INTERFACE_TONE Tone Control Services
    Contains services for tone generation and playout.
    All tone services apply to phone channels unless otherwise stated. */

/**@}*/

/* ========================================================================== */
/*                     TAPI Interface Ioctl Commands                          */
/* ========================================================================== */

/* Magic number for ioctls.*/
#define IFX_TAPI_IOC_MAGIC 'q'

/* include automatically generated TAPI ioctl command indexes */
#include "drv_tapi_io_indexes.h"
/* Include type definitons */
#include "ifx_types.h"

#ifdef WIN32
   #ifndef _IOWR
      #define _IOWR(x,y,t)  (IOC_INOUT|(((long)sizeof (t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))
   #endif
#endif /* WIN32 */

/* ======================================================================== */
/* TAPI Initialization Services, ioctl commands (Group TAPI_INTERFACE_INIT) */
/* ======================================================================== */
/** \addtogroup TAPI_INTERFACE_INIT */
/**@{*/


/** This service sets the default initialization of the device and of the
      specific channel. This command applies to any channel file descriptor.

   \note Deprecated service, use \ref IFX_TAPI_DEV_START instead.

   \param IFX_TAPI_CH_INIT_t* Pointer to an \ref IFX_TAPI_CH_INIT_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \tapiv3
   \code
   IFX_TAPI_CH_INIT_t param;

   memset (&param, 0, sizeof (param));

   param.nMode = 0;
   param.pProc = 0;

   if (ioctl (fd, IFX_TAPI_CH_INIT, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;
   return IFX_SUCCESS;
   \endcode
   \endtapiv3
   \tapiv4
   \code
   IFX_TAPI_CH_INIT_t param;

   memset (&param, 0, sizeof (param));

   param.nMode = 0;
   param.pProc = 0;

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_CH_INIT, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;
   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_CH_INIT                    _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CH_INIT_IDX, IFX_TAPI_CH_INIT_t)


/**
   This service starts the TAPI for the device file descriptor on which it is given.
   It reserves all resources needed by the TAPI for this specific device.
   This command applies to the device file descriptor.

   \param IFX_TAPI_DEV_START_CFG_t* Pointer to an \ref IFX_TAPI_DEV_START_CFG_t
          structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \code
   IFX_TAPI_DEV_START_CFG_t param;

   memset (&param, 0, sizeof (param));

   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_DEV_START, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;
   return IFX_SUCCESS;
   \endcode
*/
#define  IFX_TAPI_DEV_START                  _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_DEV_START_IDX, IFX_TAPI_DEV_START_CFG_t)


/**
   This service stops the TAPI for the device file descriptor on which it is given.
   It frees all resources allocated by the TAPI for this specific device.
   This command applies to the device file descriptor.

   \tapiv4
   \param IFX_TAPI_DEV_START_CFG_t* Pointer to an \ref IFX_TAPI_DEV_START_CFG_t
      structure.
   \endtapiv4
   \tapiv3
   \param int This interface expects no parameters. It should be set to 0.
   \endtapiv3

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \code
   IFX_TAPI_DEV_START_CFG_t param;

   memset (&param, 0, sizeof (param));

   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_DEV_STOP, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;
   return IFX_SUCCESS;
   \endcode
*/
#ifdef TAPI_ONE_DEVNODE
   #define  IFX_TAPI_DEV_STOP                _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_DEV_STOP_IDX, IFX_TAPI_DEV_START_CFG_t)
#else /* TAPI_ONE_DEVNODE */
   #define  IFX_TAPI_DEV_STOP                _IO   (IFX_TAPI_IOC_MAGIC, IFX_TAPI_DEV_STOP_IDX)
#endif /* TAPI_ONE_DEVNODE */

/**@}*/ /* TAPI_INTERFACE_INIT */

/* ========================================================================= */
/* TAPI Operation Control Services, ioctl commands (Group TAPI_INTERFACE_OP) */
/* ========================================================================= */
/** \addtogroup TAPI_INTERFACE_OP */
/**@{*/

/** This service sets the line feeding mode.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \param IFX_TAPI_LINE_FEED_t* Pointer to an \ref IFX_TAPI_LINE_FEED_t structure.

   \remarks The hardware must be able to support battery-switching modes,
   for example by programming coefficients.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error

   \tapiv3
   \code
   IFX_TAPI_LINE_FEED_t param;

   memset (&param, 0, sizeof (param));

   param = IFX_TAPI_LINE_FEED_DISABLED;

   if (ioctl (fd, IFX_TAPI_LINE_FEED_SET, (IFX_int32_t) param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_LINE_FEED_t param;

   memset (&param, 0, sizeof (param));

   // set line mode of ch 0 on dev 0 to power down
   param.lineMode = IFX_TAPI_LINE_FEED_DISABLED;

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_LINE_FEED_SET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_LINE_FEED_SET              _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_LINE_FEED_SET_IDX, IFX_TAPI_LINE_FEED_t)


/** This service reads back the line feeding mode.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \param IFX_TAPI_LINE_FEED_t* Pointer to an \ref IFX_TAPI_LINE_FEED_t structure.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error

   \tapiv3
   \code
   IFX_TAPI_LINE_FEED_t param;

   if (ioctl (fd, IFX_TAPI_LINE_FEED_GET, &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_LINE_FEED_t param;

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_LINE_FEED_GET, &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_LINE_FEED_GET              _IOR  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_LINE_FEED_GET_IDX, IFX_TAPI_LINE_FEED_t)


/** This service sets the line type (FXS, FXO) of an analog channel.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \param IFX_TAPI_LINE_TYPE_CFG_t* Pointer to an \ref IFX_TAPI_LINE_TYPE_CFG_t structure.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error

   \tapiv3
   \code
   IFX_TAPI_LINE_TYPE_CFG_t param;

   memset (&param, 0, sizeof (param));

   param.lineType = IFX_TAPI_LINE_TYPE_FXS;

   if (ioctl (fd, IFX_TAPI_LINE_TYPE_SET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_LINE_TYPE_CFG_t param;

   memset (&param, 0, sizeof (param));

   param.lineType = IFX_TAPI_LINE_TYPE_FXS;

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_LINE_TYPE_SET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_LINE_TYPE_SET              _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_LINE_TYPE_SET_IDX, IFX_TAPI_LINE_TYPE_CFG_t)


/** This service control PCM channel muting.
    This command applies to any channel file descriptor
    that includes a PCM module resource.

   \param IFX_TAPI_PCM_MUTE_CFG_t* Pointer to an \ref IFX_TAPI_PCM_MUTE_CFG_t structure.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \code
   IFX_TAPI_PCM_MUTE_CFG_t Pcm;

   memset (&Pcm, 0, sizeof(IFX_TAPI_PCM_MUTE_CFG_t));
   ioctl(fd, IFX_TAPI_PCM_MUTE_CFG_SET, &Pcm);
   \endcode   */
#define  IFX_TAPI_PCM_MUTE_CFG_SET           _IOW(IFX_TAPI_IOC_MAGIC, IFX_TAPI_PCM_MUTE_CFG_SET_IDX, IFX_TAPI_PCM_MUTE_CFG_t)


/** Specifies the timing for hook, pulse digit and hook flash validation.
    This command applies to phone channel file descriptors which contain
    an analog (ALM) module resource.

   \param IFX_TAPI_LINE_HOOK_VT_t* Pointer to an \ref IFX_TAPI_LINE_HOOK_VT_t structure.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error

   \remarks
   The following conditions must be met:
      - IFX_TAPI_LINE_HOOK_VT_DIGITLOW_TIME
        min. and max. < IFX_TAPI_LINE_HOOK_VT_HOOKFLASH_TIME min. and max.
      - IFX_TAPI_LINE_HOOK_VT_HOOKFLASH_TIME
        min. and max. < IFX_TAPI_LINE_HOOK_VT_HOOKON_TIME min. and max.

   \tapiv3
   \code
   IFX_TAPI_LINE_HOOK_VT_t param;

   memset (&param, 0, sizeof (param));

   // Set pulse dialing
   param.nType = IFX_TAPI_LINE_HOOK_VT_DIGITLOW_TIME;
   param.nMinTime = 40;
   param.nMaxTime = 60;

   if (ioctl (fd, IFX_TAPI_LINE_HOOK_VT_SET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_LINE_HOOK_VT_t param;

   memset (&param, 0, sizeof (param));

   // Set pulse dialing
   param.nType = IFX_TAPI_LINE_HOOK_VT_DIGITLOW_TIME;
   param.nMinTime = 40;
   param.nMaxTime = 60;

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_LINE_HOOK_VT_SET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_LINE_HOOK_VT_SET           _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_LINE_HOOK_VT_SET_IDX, IFX_TAPI_LINE_HOOK_VT_t)


/** Sets the voice volume of the analog line module for the incoming and
    outgoing voice path. This is used for speaker phone and microphone
    volume settings.
    This command applies to phone channel file descriptors which contain
    an analog (ALM) module resource.

   \param IFX_TAPI_LINE_VOLUME_t* Pointer to an \ref IFX_TAPI_LINE_VOLUME_t structure.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error

   \tapiv3
   \code
   IFX_TAPI_LINE_VOLUME_t param;

   memset (&param, 0, sizeof (param));
   param.nGainTx = 24;
   param.nGainRx = 0;

   if (ioctl (fd, IFX_TAPI_PHONE_VOLUME_SET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_LINE_VOLUME_t param;

   memset (&param, 0, sizeof (param));
   param.nGainTx = 24;
   param.nGainRx = 0;

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_PHONE_VOLUME_SET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_PHONE_VOLUME_SET           _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_PHONE_VOLUME_SET_IDX, IFX_TAPI_LINE_VOLUME_t)


/** This service enables or disables a high-level path of a phone channel.
   The high-level path might be required to play howler tones.
    This command applies to phone channel file descriptors which contain
    an analog (ALM) module resource.

   \param IFX_TAPI_LINE_LEVEL_CFG_t* Pointer to an \ref IFX_TAPI_LINE_LEVEL_CFG_t structure.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error

   \remarks This service is intended for use with phone channels only and must be used
   in combination with \ref IFX_TAPI_PHONE_VOLUME_SET to set the maximum level
   (\ref IFX_TAPI_LINE_VOLUME_HIGH) or to restore the level.
    Only the order of calls with parameters
    IFX_TAPI_LINE_LEVEL_ENABLE and then IFX_TAPI_LINE_LEVEL_DISABLE
    is supported.

   \tapiv3
   \code
   IFX_TAPI_LINE_LEVEL_CFG_t param;

   memset (&param, 0, sizeof (param));

   if (ioctl (fd, IFX_TAPI_LINE_LEVEL_SET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return param;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_LINE_LEVEL_CFG_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_LINE_LEVEL_SET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return param.level;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_LINE_LEVEL_SET             _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_LINE_LEVEL_SET_IDX, IFX_TAPI_LINE_LEVEL_CFG_t)


/**  This service reads the hook status from the driver.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \param IFX_TAPI_LINE_HOOK_STATUS_GET_t* Pointer to an \ref IFX_TAPI_LINE_HOOK_STATUS_GET_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \note 'Off-hook' corresponds to 'ground start', depending on the line feed mode.

   \tapiv3
   \code
   IFX_TAPI_LINE_HOOK_STATUS_GET_t param;

   memset (&param, 0, sizeof (param));

   if (ioctl (fd, IFX_TAPI_LINE_HOOK_STATUS_GET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   switch(param)
   {
      case IFX_TAPI_LINE_ONHOOK:
      // on hook
      break;

      case IFX_TAPI_LINE_OFFHOOK:
      // off hook
      break;

      default:
      // unknown state
      break;
   }

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_LINE_HOOK_STATUS_GET_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_LINE_HOOK_STATUS_GET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   switch(param.hookMode)
   {
      case IFX_TAPI_LINE_ONHOOK:
      // on hook
      break;

      case IFX_TAPI_LINE_OFFHOOK:
      // off hook
      break;

      default:
      // unknown state
      break;
   }

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_LINE_HOOK_STATUS_GET       _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_LINE_HOOK_STATUS_GET_IDX, IFX_TAPI_LINE_HOOK_STATUS_GET_t)


/**@}*/ /* TAPI_INTERFACE_OP */

/* =================================================================== */
/* TAPI Metering Services, ioctl commands (Group TAPI_INTERFACE_METER) */
/* =================================================================== */
/** \addtogroup TAPI_INTERFACE_METER */
/**@{*/

/** This service sends one burst of metering.
    This command applies to phone channel file descriptors which contain
    an analog (ALM) module resource.

   \param IFX_TAPI_METER_CFG_t* Pointer to an \ref IFX_TAPI_METER_CFG_t structure.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \remarks Bursts are sent out until an \ref IFX_TAPI_METER_STOP ioctl
            is called.

   \tapiv3
   \code
   IFX_TAPI_METER_CFG_t param;

   memset (&param, 0, sizeof (param));

   // set param characteristic
   // param mode is already set to 0
   // 100 ms pulse length
   param.nPulseLen = 100;
   // pause between two metering pulses
   // 100 ms length
   param.nPauseLen = 100;

   if (ioctl (fd, IFX_TAPI_METER_CFG_SET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_METER_CFG_t param;

   memset (&param, 0, sizeof (param));

   // set param characteristic
   // param mode is already set to 0
   // 100 ms pulse length
   param.nPulseLen = 100;
   // pause between two metering pulses
   // 100 ms length
   param.nPauseLen = 100;

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_METER_CFG_SET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_METER_CFG_SET              _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_METER_CFG_SET_IDX, IFX_TAPI_METER_CFG_t)


/** This service starts the metering.
    This command applies to phone channel file descriptors which contain
    an analog (ALM) module resource.

   \param IFX_TAPI_METER_START_t* Pointer to an \ref IFX_TAPI_METER_START_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \remarks Before this service can be used, the metering characteristic must
    be set (\ref IFX_TAPI_METER_CFG_SET) and the line mode must be set
    to normal (\ref IFX_TAPI_LINE_FEED_SET).

   \remarks To use transparent activation, set nPulseDist and nPulses to ZERO.
    For this mode, it is not necessary to set \ref IFX_TAPI_METER_CFG_SET.

   \tapiv3
   \code
   IFX_TAPI_METER_START_t param;

   memset (&param, 0, sizeof (param));

   if (ioctl (fd, IFX_TAPI_METER_START, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_METER_START_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_METER_START, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_METER_START                _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_METER_START_IDX, IFX_TAPI_METER_START_t)


/** This service stops the metering.
    This command applies to phone channel file descriptors which contain
    an analog (ALM) module resource.

   \tapiv4
   \param IFX_TAPI_METER_STOP_t* Pointer to an \ref IFX_TAPI_METER_STOP_t structure.
   \endtapiv4
   \tapiv3
   \param int This interface expects no parameters. It should be set to 0.
   \endtapiv3

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \remarks If the metering has not been previously started, this service returns
    an error.

   \tapiv3
   \code
   if (ioctl (fd, IFX_TAPI_METER_STOP, 0) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_METER_STOP_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_METER_STOP, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#ifdef TAPI_ONE_DEVNODE
   #define  IFX_TAPI_METER_STOP              _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_METER_STOP_IDX, IFX_TAPI_METER_STOP_t)
#else /* TAPI_ONE_DEVNODE */
   #define  IFX_TAPI_METER_STOP              _IO   (IFX_TAPI_IOC_MAGIC, IFX_TAPI_METER_STOP_IDX)
#endif /* TAPI_ONE_DEVNODE */


/** This service sends one burst of metering.
    This command applies to phone channel file descriptors which contain
    an analog (ALM) module resource.

   \param IFX_TAPI_METER_BURST_t* Pointer to an \ref IFX_TAPI_METER_BURST_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \remarks If the metering has not been previously started, this service returns
    an error.

   \tapiv3
   \code
   IFX_TAPI_METER_BURST_t param;

   memset (&param, 0, sizeof (param));

   if (ioctl (fd, IFX_TAPI_METER_BURST, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_METER_BURST_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_METER_BURST, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_METER_BURST                _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_METER_BURST_IDX, IFX_TAPI_METER_BURST_t)


/** This service reads the metering statistic.
    This command applies to phone channel file descriptors which contain
    an analog (ALM) module resource.

   \param IFX_TAPI_METER_STATISTICS_t* Pointer to an \ref IFX_TAPI_METER_STATISTICS_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \remarks If the metering has not been previously started, this service returns
    an error.

   \tapiv3
   \code
   IFX_TAPI_METER_STATISTICS_t param;

   memset (&param, 0, sizeof (param));

   if (ioctl (fd, IFX_TAPI_METER_STATISTICS_GET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_METER_STATISTICS_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_METER_STATISTICS_GET, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_METER_STATISTICS_GET       _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_METER_STATISTICS_GET_IDX, IFX_TAPI_METER_STATISTICS_t)

/**@}*/ /* TAPI_INTERFACE_METER */


/* ======================================================================= */
/* TAPI Tone Control Services, ioctl commands (Group TAPI_INTERFACE_TONE)  */
/* ======================================================================= */
/** \addtogroup TAPI_INTERFACE_TONE */
/**@{*/


/** Configures a tone based on simple or composed tones. The tone is also added
    to the tone table.
    This command applies to the device file descriptor.

   \param IFX_TAPI_TONE_t* Pointer to an \ref IFX_TAPI_TONE_t structure.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \remarks
   A simple tone specifies a tone sequence composed of several single frequency
   tones or dual frequency tones. The sequence can be transmitted only once,
   several times or until transmission is stopped by the client.
   This interface can add a simple tone to the internal table with a maximum of
   222 entries, starting from IFX_TAPI_TONE_INDEX_MIN (32).
   At least one cadence must be defined, otherwise this interface returns an
   error. The tone table provides all tone frequencies in steps of 5 Hz and with a 5%
   tolerance, as defined in RFC 2833.
   For composed tones, the loop count of each simple tone must have a value other than 0.

   \tapiv4
   \code

   IFX_TAPI_TONE_t tone;
   IFX_TAPI_TONE_PLAY_t param;

   memset (&tone,0, sizeof (tone));
   tone.simple.format = IFX_TAPI_TONE_TYPE_SIMPLE;
   tone.simple.index = 71;
   tone.simple.freqA = 480;
   tone.simple.freqB = 620;
   tone.simple.levelA = -300;
   tone.simple.cadence[0] = 2000;
   tone.simple.cadence[1] = 2000;
   tone.simple.frequencies[0] = IFX_TAPI_TONE_FREQA | IFX_TAPI_TONE_FREQB;
   tone.simple.loop = 2;
   tone.simple.pause = 200;
   if (ioctl (fd, IFX_TAPI_TONE_TABLE_CFG_SET, (IFX_uintptr_t) &tone) != IFX_SUCCESS)
      return IFX_ERROR;

   memset (&tone,0, sizeof (tone));
   tone.composed.format = IFX_TAPI_TONE_TYPE_COMPOSED;
   tone.composed.index = 100;
   tone.composed.count = 2;
   tone.composed.tones[0] = 71;
   tone.composed.tones[1] = 71;
   if (ioctl (fd, IFX_TAPI_TONE_TABLE_CFG_SET, (IFX_uintptr_t) &tone) != IFX_SUCCESS)
      return IFX_ERROR;

   memset (&param,0, sizeof (param));
   // Play tone on analog line module towards the telephone
   param.module = IFX_TAPI_MODULE_TYPE_ALM;
   // Play tone index 100
   param.index = 100;
   param.external = 1;
   param.internal = 0;

   // Play tone on the third device
   param.dev = 2;
   // Play tone on the second channel
   param.ch = 1;
   // Start to play
   if (ioctl (fd, IFX_TAPI_TONE_PLAY, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   sleep(5);

   //Stop playing tone
   if (ioctl (fd, IFX_TAPI_TONE_STOP, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_TONE_TABLE_CFG_SET         _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_TONE_TABLE_CFG_SET_IDX, IFX_TAPI_TONE_t)


/** Start/stop generation of a tone towards TDM, analog line, conf-bridge or
    RTP. The 'index' parameter of \ref IFX_TAPI_TONE_PLAY_t (if greater than 0)
     gives the tone table index of the tone to be played.
    If the parameter is equal to zero, stop the current tone generation.
    This command applies to ALM, PCM and COD modules.

   \param IFX_TAPI_TONE_PLAY_t* Pointer to an \ref IFX_TAPI_TONE_PLAY_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \remarks
   This can be a pre-defined simple or a composed tone.
   The tone codes are assigned previously on system start.
   Index 1 - 31 is pre-defined by the driver and covers the
   original TAPI.
   All other indices can be custom-defined by
   \ref IFX_TAPI_TONE_TABLE_CFG_SET.

   \tapiv4
   \code
   IFX_TAPI_TONE_PLAY_t param;

   memset (&param, 0, sizeof (param));

   // Play tone on analog line module towards the telephone
   param.module = IFX_TAPI_MODULE_TYPE_ALM;
   // Play tone index 34
   param.index = 34;
   param.external = 1;
   param.internal = 0;

   // Play tone on the third device
   param.dev = 2;
   // Play tone on the second channel
   param.ch = 1;
   if (ioctl (fd, IFX_TAPI_TONE_PLAY, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_TONE_PLAY                  _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_TONE_PLAY_IDX, IFX_TAPI_TONE_PLAY_t)


#ifndef TAPI4_DXY_DOC
   /** Starts/stops generation of a tone towards the local port; the parameter
   (if greater than 0) gives the tone table index of the tone to be played.
   If the parameter is equal to zero, the current tone generation stops.
    This command applies to any channel file descriptor
    that includes a data channel (COD+SIG) module resource.

   \param IFX_TAPI_TONE_IDX_t* Pointer to an \ref IFX_TAPI_TONE_IDX_t structure.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \remarks
   This can be a pre-defined simple or a composed tone. The tone codes are
   assigned previously upon system start.
   Index 1 - 31 is pre-defined by the driver and covers the original TAPI.

   \tapiv3
   \code
   IFX_TAPI_TONE_IDX_t param;

   memset (&param, 0, sizeof (param));

   // play tone index 34
   param = 34;

   if (ioctl (fd, IFX_TAPI_TONE_LOCAL_PLAY, (IFX_int32_t)param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_TONE_IDX_t param;

   memset (&param, 0, sizeof (param));

   // play tone index 34
   param.nToneIndex = 34;

   // Play tone on the third device
   param.dev = 2;
   // Play tone on the second channel
   param.ch = 1;

   if (ioctl (fd, IFX_TAPI_TONE_LOCAL_PLAY, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_TONE_LOCAL_PLAY            _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_TONE_LOCAL_PLAY_IDX, IFX_TAPI_TONE_IDX_t)


/** Pre-defined tone services for busy tone.
    This command applies to any channel file descriptor
    that includes a data channel (COD+SIG) module resource.

   \tapiv4
   \param IFX_TAPI_TONE_BUSY_t* Pointer to an \ref IFX_TAPI_TONE_BUSY_t structure.
   \endtapiv4
   \tapiv3
   \param int This interface expects no parameters. It should be set to 0.
   \endtapiv3

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \remarks These tones are defined by frequencies inside the TAPI for the USA.

   \tapiv3
   \code
   if (ioctl (fd, IFX_TAPI_TONE_BUSY_PLAY, 0) != IFX_SUCCESS)
      return IFX_ERROR;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_TONE_BUSY_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_TONE_BUSY_PLAY, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;
   \endcode
   \endtapiv4
*/
#ifdef TAPI_ONE_DEVNODE
   #define  IFX_TAPI_TONE_BUSY_PLAY          _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_TONE_BUSY_PLAY_IDX, IFX_TAPI_TONE_BUSY_t)
#else /* TAPI_ONE_DEVNODE */
   #define  IFX_TAPI_TONE_BUSY_PLAY          _IO   (IFX_TAPI_IOC_MAGIC, IFX_TAPI_TONE_BUSY_PLAY_IDX)
#endif /* TAPI_ONE_DEVNODE */

/** Pre-defined tone services for the ring back tone.
    This command applies to any channel file descriptor
    that includes a data channel (COD+SIG) module resource.

   \tapiv4
   \param IFX_TAPI_TONE_RINGBACK_t* Pointer to an \ref IFX_TAPI_TONE_RINGBACK_t structure.
   \endtapiv4
   \tapiv3
   \param int This interface expects no parameters. It should be set to 0.
   \endtapiv3

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \remarks These tones are defined by frequencies inside the TAPI for the USA.

   \tapiv3
   \code
   if (ioctl (fd, IFX_TAPI_TONE_RINGBACK_PLAY, 0) != IFX_SUCCESS)
      return IFX_ERROR;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_TONE_RINGBACK_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_TONE_RINGBACK_PLAY, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;
   \endcode
   \endtapiv4
*/
#ifdef TAPI_ONE_DEVNODE
   #define  IFX_TAPI_TONE_RINGBACK_PLAY      _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_TONE_RINGBACK_PLAY_IDX, IFX_TAPI_TONE_RINGBACK_t)
#else /* TAPI_ONE_DEVNODE */
   #define  IFX_TAPI_TONE_RINGBACK_PLAY      _IO   (IFX_TAPI_IOC_MAGIC, IFX_TAPI_TONE_RINGBACK_PLAY_IDX)
#endif /* TAPI_ONE_DEVNODE */

/**
   Pre-defined tone services for the dial tone.
   This command applies to any channel file descriptor that
   includes a data channel (COD+SIG) module resource.

   \tapiv4
   \param IFX_TAPI_TONE_DIALTONE_t* Pointer to an \ref IFX_TAPI_TONE_DIALTONE_t structure.
   \endtapiv4
   \tapiv3
   \param int This interface expects no parameters. It should be set to 0.
   \endtapiv3

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \remarks These tones are defined by frequencies inside the TAPI for the USA.

   \tapiv3
   \code
   if (ioctl (fd, IFX_TAPI_TONE_DIALTONE_PLAY, 0) != IFX_SUCCESS)
      return IFX_ERROR;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_TONE_DIALTONE_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_TONE_DIALTONE_PLAY, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;
   \endcode
   \endtapiv4
*/
#ifdef TAPI_ONE_DEVNODE
   #define  IFX_TAPI_TONE_DIALTONE_PLAY      _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_TONE_DIALTONE_PLAY_IDX, IFX_TAPI_TONE_DIALTONE_t)
#else /* TAPI_ONE_DEVNODE */
   #define  IFX_TAPI_TONE_DIALTONE_PLAY      _IO   (IFX_TAPI_IOC_MAGIC, IFX_TAPI_TONE_DIALTONE_PLAY_IDX)
#endif /* TAPI_ONE_DEVNODE */

/** Stops playback of a specified tone.
    This command applies to any channel file descriptor
    that includes a data channel (COD+SIG) module resource.

   \param IFX_TAPI_TONE_IDX_t* Pointer to an \ref IFX_TAPI_TONE_IDX_t structure.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \remarks Stopping of all tones played in this direction can be done with
   index 0. Passing index 0 to \ref IFX_TAPI_TONE_LOCAL_PLAY has the same effect.

   \tapiv3
   \code
   IFX_TAPI_TONE_IDX_t param;

   memset (&param, 0, sizeof (param));

   // stop playing tone index 34 in local direction
   param = 34;

   if (ioctl (fd, IFX_TAPI_TONE_LOCAL_STOP, (IFX_int32_t)param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_TONE_IDX_t param;

   memset (&param, 0, sizeof (param));

   // stop playing tone index 34 in local direction
   param.nToneIndex = 34;

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;

   if (ioctl (fd, IFX_TAPI_TONE_LOCAL_STOP, (IFX_uintptr_t)&param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_TONE_LOCAL_STOP            _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_TONE_LOCAL_STOP_IDX, IFX_TAPI_TONE_IDX_t)

#endif /* #ifndef TAPI4_DXY_DOC */


/** Stops tone generation.
    This command applies to any channel file descriptor
    that includes a data channel (COD+SIG) module resource.

   \param IFX_TAPI_TONE_PLAY_t* Pointer to an \ref IFX_TAPI_TONE_PLAY_t structure.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \remarks Assigning an 'index' of 0 will stop playback of all tones in all
   directions. Tones can also be stopped with \ref IFX_TAPI_TONE_PLAY
   and 'index' set to zero.
   The function returns an error in case there is no tone to stop.

   \tapiv3
   \code
   IFX_TAPI_TONE_PLAY_t param;

   memset (&param, 0, sizeof (param));

   // Stop tone on analog line module towards the telephone
   param.module = IFX_TAPI_MODULE_TYPE_ALM;
   // Set to zero but the TAPI ignores this parameter
   param.index = 0;

   if (ioctl (fd, IFX_TAPI_TONE_STOP, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_TONE_PLAY_t param;

   memset (&param, 0, sizeof (param));

   // Stop tone on analog line module towards the telephone
   param.module = IFX_TAPI_MODULE_TYPE_ALM;
   // Stop to play
   param.external = 1;
   param.internal = 0;
   // Set to zero but the TAPI ignores this parameter
   param.index = 0;

   // Second channel on the device
   param.ch = 1;
   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_TONE_STOP, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_TONE_STOP                  _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_TONE_STOP_IDX, IFX_TAPI_TONE_PLAY_t)

/**@}*/ /* TAPI_INTERFACE_TONE */

/* ======================================================================= */
/* TAPI Signal Detection Services, ioctl commands                          */
/* (Group TAPI_INTERFACE_SIGNAL)                                           */
/* ======================================================================= */
/** \addtogroup TAPI_INTERFACE_SIGNAL */
/**@{*/

/** This service is used to set DTMF receiver coefficients.
    This command applies to any channel file descriptor
    that includes a data channel (COD+SIG) module resource.

   \param IFX_TAPI_DTMF_RX_CFG_t* Pointer to an \ref IFX_TAPI_DTMF_RX_CFG_t structure.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \remarks If enabled, the DTMF receiver will be temporarily disabled during
    the writing of the coefficients.

   \tapiv3
   \code
   IFX_TAPI_DTMF_RX_CFG_t param;

   memset (&param, 0, sizeof (param));

   param.nLevel   = -56;   // dB
   param.nTwist   = 9;     // dB
   param.nGain    = 0;     // dB

   if (ioctl (fd, IFX_TAPI_DTMF_RX_CFG_SET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_DTMF_RX_CFG_t param;

   memset (&param, 0, sizeof (param));

   param.nLevel   = -56;   // dB
   param.nTwist   = 9;     // dB
   param.nGain    = 0;     // dB

   // Second channel on the second device
   param.ch = 1;
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_DTMF_RX_CFG_SET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_DTMF_RX_CFG_SET            _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_DTMF_RX_CFG_SET_IDX, IFX_TAPI_DTMF_RX_CFG_t)


/** Retrieves DTMF receiver coefficients.
    This command applies to any channel file descriptor
    that includes a data channel (COD+SIG) module resource.

   \param IFX_TAPI_DTMF_RX_CFG_t* Pointer to an \ref IFX_TAPI_DTMF_RX_CFG_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \tapiv3
   \code
   IFX_TAPI_DTMF_RX_CFG_t param;

   memset (&param, 0, sizeof (param));

   if (ioctl (fd, IFX_TAPI_DTMF_RX_CFG_GET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_DTMF_RX_CFG_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the second device
   param.ch = 1;
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_DTMF_RX_CFG_GET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_DTMF_RX_CFG_GET            _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_DTMF_RX_CFG_GET_IDX, IFX_TAPI_DTMF_RX_CFG_t)

/**@}*/ /* TAPI_INTERFACE_SIGNAL */


/* ======================================================================= */
/* TAPI CID Features Service, ioctl commands  (Group TAPI_INTERFACE_CID)   */
/* ======================================================================= */
/** \addtogroup TAPI_INTERFACE_CID */
/**@{*/

/** This interface transmits CID messages.
    This command applies to any channel file descriptor
    that includes a data channel (COD+SIG) module resource.

   \param  IFX_TAPI_CID_MSG_t*  Pointer to an \ref IFX_TAPI_CID_MSG_t structure,
   containing the CID/MWI information to be transmitted.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \remarks Before issuing this service, the CID engine must be configured with
   IFX_TAPI_CID_CFG_SET at least once after boot. This is required to
   configure country-specific settings.

   \code
   IFX_TAPI_CID_MSG_t Cid_Info;
   IFX_TAPI_CID_MSG_ELEMENT_t Msg_El[2];
   IFX_char_t* number = "12345";

   memset(&Cid_Info, 0, sizeof(Cid_Info));
   memset(&Msg_El, 0, sizeof(Msg_El));

   Cid_Info.txMode = IFX_TAPI_CID_HM_ONHOOK;

   // Message Waiting
   Cid_Info.messageType = IFX_TAPI_CID_MT_MWI;
   Cid_Info.nMsgElements = 2;
   Cid_Info.message = Msg_El;

   // Mandatory for Message Waiting: set Visual Indicator on
   Msg_El[0].value.elementType = IFX_TAPI_CID_ST_VISINDIC;
   Msg_El[0].value.element = IFX_TAPI_CID_VMWI_EN;

   // Add optional CLI (number) element
   Msg_El[1].string.elementType = IFX_TAPI_CID_ST_CLI;
   Msg_El[1].string.len = strlen(number);
   strncpy(Msg_El[1].string.element, number, sizeof(Msg_El[1].string.element));

   // First channel on the first device
   Cid_Info.ch = 0;
   Cid_Info.dev = 0;
   // Transmit the caller id
   if (ioctl (fd, IFX_TAPI_CID_TX_INFO_START, (IFX_uintptr_t) &Cid_Info) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
*/
#define  IFX_TAPI_CID_TX_INFO_START          _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CID_TX_INFO_START_IDX, IFX_TAPI_CID_MSG_t)


/** This service stops the ongoing transmission of a CID message that was
   started with \ref IFX_TAPI_CID_TX_INFO_START.
   This command applies to any channel file descriptor that
   includes a data channel (COD+SIG) module resource.

   \tapiv4
   \param IFX_TAPI_CID_TX_INFO_STOP_t* Pointer to an \ref IFX_TAPI_CID_TX_INFO_STOP_t structure.
   \endtapiv4
   \tapiv3
   \param int This interface expects no parameters. It should be set to 0.
   \endtapiv3

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \tapiv3
   \code
   if (ioctl (fd, IFX_TAPI_CID_TX_INFO_STOP, 0) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_CID_TX_INFO_STOP_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the second device
   param.ch = 1;
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_CID_TX_INFO_STOP, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#ifdef TAPI_ONE_DEVNODE
   #define  IFX_TAPI_CID_TX_INFO_STOP        _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CID_TX_INFO_STOP_IDX, IFX_TAPI_CID_TX_INFO_STOP_t)
#else /* TAPI_ONE_DEVNODE */
   #define  IFX_TAPI_CID_TX_INFO_STOP        _IO   (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CID_TX_INFO_STOP_IDX)
#endif /* TAPI_ONE_DEVNODE */


/** This service starts a pre-programmed CID sequence driven by the TAPI. This is a
   non-blocking service; the driver will signal the end of the CID sequence with an event.
   Before issuing this service, the CID engine must be configured with
   IFX_TAPI_CID_CFG_SET at least once after boot. This is required to
   configure country-specific settings.
   This command applies to any channel file descriptor that
   includes a data channel (COD+SIG) module resource.

   \param  IFX_TAPI_CID_MSG_t*  Pointer to an \ref IFX_TAPI_CID_MSG_t structure
   defining the CID/MWI type and containing the information to be transmitted.

   \remarks For FSK transmission, the decision of seizure and mark length is based
    on the configured standard and CID transmission type.
*/
#define  IFX_TAPI_CID_TX_SEQ_START           _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CID_TX_SEQ_START_IDX, IFX_TAPI_CID_MSG_t)

/** Configures the CID transmitter.
    This command applies to any channel file descriptor
    that includes a data channel (COD+SIG) module resource.

   \param  IFX_TAPI_CID_CFG_t*  Pointer to an \ref IFX_TAPI_CID_CFG_t structure
   containing CID / MWI configuration information.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

  \remarks The delay must be programmed so that the CID data would still fit
   between the ring burst in case of appearance mode 1 and 2, otherwise the
   ioctl \ref IFX_TAPI_RING_START may return an error. CID transmission is stopped when the
   ringing is stopped or the phone goes off-hook.

   \code
   IFX_TAPI_CID_CFG_t param;

   memset (&param, 0, sizeof (param));

   // Set CID standard to Telcordia/Bellcore default values
   param.nStandard = IFX_TAPI_CID_STD_TELCORDIA;

   // Second channel on the second device
   param.ch = 1;
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_CID_CFG_SET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
*/
#define  IFX_TAPI_CID_CFG_SET                _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CID_CFG_SET_IDX, IFX_TAPI_CID_CFG_t)


/**@}*/ /* TAPI_INTERFACE_CID */

/* ======================================================================== */
/* TAPI Miscellaneous Services, ioctl commands (Group TAPI_INTERFACE_MISC)  */
/* ======================================================================== */
/** \addtogroup TAPI_INTERFACE_MISC */
/**@{*/

/** Retrieves the TAPI version string.
    This command applies to the device file descriptor.

   \param IFX_char_t* Pointer to version character string.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \code
   IFX_char_t Version[IFX_TAPI_VERSION_LEN] = {0};

   if (ioctl(fd, IFX_TAPI_VERSION_GET, (IFX_uintptr_t) &Version[0]) != IFX_SUCCESS)
      return IFX_ERROR;

   printf("Version:%s\\n", Version);
   return IFX_SUCCESS;
   \endcode
*/
#define  IFX_TAPI_VERSION_GET                _IOR  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_VERSION_GET_IDX, IFX_char_t[IFX_TAPI_VERSION_LEN])


/** Sets the report levels if the driver is compiled with ENABLE_TRACE.
    It is applicable to device file descriptors. The trace
    level can be set to 'low', 'medium' or 'high'. All traces with a lower
    trace level than enabled are not reported. The trace information can vary
    between different driver implementations and driver versions. In general,
    traces of type 'high' indicate problems that could result in a system
    failure and system crash. These problems can be solved during the
    integration phase.
    This command applies to the device file descriptor.

   \param IFX_TAPI_DEBUG_REPORT_t* Pointer to an \ref IFX_TAPI_DEBUG_REPORT_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error
*/
#define  IFX_TAPI_DEBUG_REPORT_SET           _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_DEBUG_REPORT_SET_IDX, IFX_TAPI_DEBUG_REPORT_t)


/** Checks the supported TAPI interface version.
    This command applies to the device file descriptor.

   \param IFX_TAPI_VERSION_t Pointer to an
   \ref IFX_TAPI_VERSION_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \remarks
   Since an application is always built for one specific TAPI interface
   version, it should check whether this is supported. If not, the application should
   abort. This interface checks if the current TAPI version supports a
   particular version. For example, TAPI version 2.1 will support TAPI 2.0,
   but version 3.0 might not support 2.0.

   \code
   IFX_TAPI_VERSION_t param = {0};

   param.majorNumber = 2;
   param.minorNumber = 1;

   if (ioctl (fd, IFX_TAPI_VERSION_CHECK, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   printf("Version 2.1 supported\\n");
   return IFX_SUCCESS;
   \endcode
*/
#define  IFX_TAPI_VERSION_CHECK              _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_VERSION_CHECK_IDX, IFX_TAPI_VERSION_t)


/** This service returns the number of capabilities. The file descriptor is
    applicable to device file descriptors. These capabilities are of the
    same type as listed by \ref IFX_TAPI_CAP_LIST. They include
    supported features such as coder modules, analog line modules, PCM modules,
    tone generator, and tone detectors.
    This command applies to the device file descriptor.

   \param IFX_TAPI_CAP_NR_t* Pointer to an \ref IFX_TAPI_CAP_NR_t structure.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \tapiv3
   \code
   IFX_TAPI_CAP_NR_t param;

   memset (&param, 0, sizeof (param));

   // Get the cap list size
   if (ioctl (fd, IFX_TAPI_CAP_NR, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return param;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_CAP_NR_t param;

   memset (&param, 0, sizeof (param));

   // Second device
   param.dev = 1;

   // Get the cap list size
   if (ioctl (fd, IFX_TAPI_CAP_NR, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return param.nCap;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_CAP_NR                     _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CAP_NR_IDX, IFX_TAPI_CAP_NR_t)


/**
   Note: The use of this interface is deprecated. Use \ref IFX_TAPI_CAP_NLIST
   for new developments.
   This service returns the capability lists. The capability list contains
   the number of supported features such as coder modules, analog line modules,
   PCM modules, tone generator, and tone detectors.
   This command applies to the device file descriptor.

   \remarks A memory block of sufficient size needs to be passed for the
   return parameter. This ioctl cannot check the size and will always copy the
   full list it has into the given memory area. To avoid a potential
   memory overwrite the ioctl \ref IFX_TAPI_CAP_NLIST should be used instead
   of this one.

   \param int Pointer to an \ref IFX_TAPI_CAP_t structure array.
      The number of array entries can be retrieved by \ref IFX_TAPI_CAP_NR.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \code
   IFX_TAPI_CAP_t *pList;
   IFX_int32_t i, nCap;

   // retrieve nCap value via IFX_TAPI_CAP_NR
   pList = malloc (nCap * sizeof (IFX_TAPI_CAP_t));

   // Get the cap list
   if (ioctl (fd, IFX_TAPI_CAP_LIST, (IFX_uintptr_t) pList) == IFX_SUCCESS)
   {
      for (i = 0; i < nCap; i++)
      {
         IFX_TAPI_CAP_t *pCap = &pList[i];

         switch (pCap->captype)
         {
              case IFX_TAPI_CAP_TYPE_CODEC:
            printf ("Codec: %s\\n", pCap->desc);
                  break;
              case IFX_TAPI_CAP_TYPE_PCM:
            printf ("PCM: %d\\n", pCap->cap);
                 break;
              default:
                  break;
         }
      }
   }

   // Free the allocated memory
   free(pList);

   return IFX_SUCCESS;
   \endcode
*/
#define  IFX_TAPI_CAP_LIST                _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CAP_LIST_IDX, IFX_TAPI_CAP_t*)


/**
   This service returns the capability lists. The capability list contains
   the number of supported features such as coder modules, analog line modules,
   PCM modules, tone generator, and tone detectors.
   This command applies to the device file descriptor.

   \param int Pointer to an \ref IFX_TAPI_CAP_LIST_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \tapiv3
   \code
   IFX_TAPI_CAP_LIST_t param;
   IFX_int32_t i, nCap;

   memset (&param, 0, sizeof (param));

   // retrieve nCap value via IFX_TAPI_CAP_NR

   param.nCap = nCap;
   param.pList = malloc (nCap * sizeof (IFX_TAPI_CAP_t));

   // Get the cap list
   if (ioctl (fd, IFX_TAPI_CAP_NLIST, (IFX_uintptr_t) &param) == IFX_SUCCESS)
   {
      for (i = 0; i < nCap; i++)
      {
         IFX_TAPI_CAP_t *pCap = &param.pList[i];

         switch (pCap->captype)
         {
            case IFX_TAPI_CAP_TYPE_CODEC:
               printf ("Codec: %s\\n", pCap->desc);
            break;
            case IFX_TAPI_CAP_TYPE_PCM:
               printf ("PCM: %d\\n", pCap->cap);
               break;
            default:
            break;
         }
      }
   }

   // Free the allocated memory
   free(param.pList);

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_CAP_LIST_t param;
   IFX_int32_t i, nCap;

   memset (&param, 0, sizeof (param));

   // retrieve nCap value via IFX_TAPI_CAP_NR

   param.nCap = nCap;
   param.pList = malloc (nCap * sizeof (IFX_TAPI_CAP_t));

   // Second device
   param.dev = 1;
   // Get the cap list
   if (ioctl (fd, IFX_TAPI_CAP_NLIST, (IFX_uintptr_t) &param) == IFX_SUCCESS)
   {
      for (i = 0; i < nCap; i++)
      {
         IFX_TAPI_CAP_t *pCap = &param.pList[i];

         switch (pCap->captype)
         {
            case IFX_TAPI_CAP_TYPE_CODEC:
               printf ("Codec: %s\\n", pCap->desc);
            break;
            case IFX_TAPI_CAP_TYPE_PCM:
               printf ("PCM: %d\\n", pCap->cap);
               break;
            default:
            break;
         }
      }
   }

   // Free the allocated memory
   free(param.pList);

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_CAP_NLIST               _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CAP_NLIST_IDX, IFX_TAPI_CAP_LIST_t)


/** This service checks whether a specific capability is supported.
   This command applies to the device file descriptor.

   \param IFX_TAPI_CAP_t* Pointer to an
   \ref IFX_TAPI_CAP_t structure.

   \return Returns the following values:
      - 0: Capability not supported
      - 1:  Capability supported
      - IFX_ERROR: in case of an error

   \tapiv3
   \code
   IFX_TAPI_CAP_t param;
   IFX_int32_t ret = IFX_ERROR;

   memset (&param, 0, sizeof (param));

   // Check if MLAW is supported
   param.captype = IFX_TAPI_CAP_TYPE_CODEC;
   param.cap = IFX_TAPI_COD_TYPE_MLAW;

   ret = ioctl (fd, IFX_TAPI_CAP_CHECK, (IFX_uintptr_t) &param);

   if (ret == 0)
   {
      printf( "MLAW not supported\\n" );
   }
   else if(ret == 1)
   {
      printf( "MLAW supported\\n" );
   }
   else
   {
       return IFX_ERROR;
   }

   // Check how many data channels are supported
   param.captype = IFX_TAPI_CAP_TYPE_CODECS;

   ret = ioctl (fd, IFX_TAPI_CAP_CHECK, (IFX_uintptr_t) &param);

   if (ret == 0)
   {
      printf( "data channels not supported\\n");
   }
   else if(ret == 1)
   {
      printf( "%d data channels supported\\n", param.cap);
   }
   else
   {
       return IFX_ERROR;
   }

   // Check if POTS port is available
   param.captype = IFX_TAPI_CAP_TYPE_PORT;
   param.cap = IFX_TAPI_CAP_PORT_POTS;

   ret = ioctl (fd, IFX_TAPI_CAP_CHECK, (IFX_uintptr_t) &param);

   if (ret == 0)
   {
      printf("POTS port not supported\\n");
   }
   else if(ret == 1)
   {
      printf("POTS port supported\\n");
   }
   else
   {
       return IFX_ERROR;
   }

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_CAP_t param = {0};

   // Second device
   param.dev = 1;

   // Check if MLAW is supported
   param.captype = IFX_TAPI_CAP_TYPE_CODEC;
   param.cap = IFX_TAPI_COD_TYPE_MLAW;

   ret = ioctl (fd, IFX_TAPI_CAP_CHECK, (IFX_uintptr_t) &param);

   if (ret == 0)
   {
      printf("MLAW not supported\\n");
   }
   else if (ret == 1)
   {
      printf("MLAW supported\\n");
   }
   else
   {
       return IFX_ERROR;
   }

   // Check if POTS port is available
   param.captype = IFX_TAPI_CAP_TYPE_PORT;
   param.cap = IFX_TAPI_CAP_PORT_POTS;

   ret = ioctl (fd, IFX_TAPI_CAP_CHECK, (IFX_uintptr_t) &param);

   if (ret == 0)
   {
      printf("POTS port not supported\\n");
   }
   else if (ret == 1)
   {
      printf("POTS port supported\\n");
   }
   else
   {
       return IFX_ERROR;
   }

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_CAP_CHECK                  _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CAP_CHECK_IDX, IFX_TAPI_CAP_t)


/** This service returns the last error code occurring in the
    TAPI driver or the low-level driver. It also contains an error
    stack for tracking down the origin of the error source.
    This command applies to the device file descriptor.
    After calling this service the stack is reset.

   \param IFX_TAPI_Error_t* Pointer to an
   \ref IFX_TAPI_Error_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \tapiv3
   \code
   int ret, i;

   // call some ioctl
   // ret = ioctl (fd, <some ioctl> , (IFX_uintptr_t) &<some ioctl param>);
   if (ret != IFX_SUCCESS)
   {
      IFX_TAPI_Error_t error;

      memset (&error, 0, sizeof (error));

      ioctl (fd, IFX_TAPI_LASTERR, (IFX_uintptr_t)&error);
      if (error.nCode != -1)
      {
         // we have additional information
         printf ("Error Code 0x%X occurred\\n", error.nCode);
         for (i = 0; i < TAPI_ERRNO_CNT; ++i)
         {
            if (TAPI_drvErrnos[i] == ((error.nCode >> 16) & 0xffff))
            {
               printf("%s\\n", TAPI_drvErrStrings[i]);
            }
         }
         for (i = 0; i < error.nCnt; ++i)
         {
            printf ("%s:%d Code 0x%4X%4X\\n", error.stack[i].sFile,
            error.stack[i].nLine,
            error.stack[i].nHlCode,
            error.stack[i].nLlCode);
         }
      }
   }
   \endcode
   \endtapiv3

   \tapiv4
   \code
   int ret, i;

   // call some ioctl
   // ret = ioctl (fd, <some ioctl> , (IFX_uintptr_t) &<some ioctl param>);
   if (ret != IFX_SUCCESS)
   {
      IFX_TAPI_Error_t error;

      memset (&error, 0, sizeof (error));

      // use the correct device and channel
      error.nDev = param.dev;
      error.nCh = param.ch;

      ioctl (fd, IFX_TAPI_LASTERR, (IFX_uintptr_t)&error);
      if (error.nCode != -1)
      {
         // we have additional information
         printf ("Error Code 0x%X occurred\\n", error.nCode);
         for (i = 0; i < TAPI_ERRNO_CNT; ++i)
         {
            if (TAPI_drvErrnos[i] == ((error.nCode>>16) & 0xffff))
            {
               printf("%s\\n", TAPI_drvErrStrings[i]);
            }
         }
         for (i = 0; i < error.nCnt; ++i)
         {
            printf ("%s:%d Code 0x%4X%4X\\n", error.stack[i].sFile,
            error.stack[i].nLine,
            error.stack[i].nHlCode,
            error.stack[i].nLlCode);
         }
      }
   }
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_LASTERR                    _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_LASTERR_IDX, IFX_TAPI_Error_t)


/**@}*/ /* TAPI_INTERFACE_MISC */

/* ========================================================================== */
/* TAPI Power Ringing Services, ioctl commands (Group TAPI_INTERFACE_RINGING) */
/* ========================================================================== */
/** \addtogroup TAPI_INTERFACE_RINGING */
/**@{*/


/** This service sets the high resolution ring cadence for the power ringing
   services. The cadence value has to be set before ringing is started with
   \ref IFX_TAPI_RING_START or \ref IFX_TAPI_RING.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \param IFX_TAPI_RING_CADENCE_t* Pointer to an \ref IFX_TAPI_RING_CADENCE_t structure.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \remarks
   \tapiv4
   The number of ring bursts can be obtained by counting the events
    \ref IFX_TAPI_EVENT_FXS_RING.
   \endtapiv4
   The initial cadence pattern can be of zero length, in which case the initial
   pattern will not be played. If a length for the initial pattern is given,
   the initial pattern may not consist of all-zero bits, but of all bits set to one.
   The periodic pattern must contain at least one bit set to one. This implies
   that the length of the periodic pattern must be at least one. The setting of all
   bits for the defined length to one is allowed, and is a way of starting an infinite
   ringing.

   \tapiv3
   \code
   IFX_TAPI_RING_CADENCE_t ringCadence;
   IFX_char_t data[15] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00 };

   memset (&ringCadence, 0, sizeof (ringCadence));

   // Program the cadence
   memcpy(&ringCadence.data, data, sizeof (data));
   ringCadence.nr = (int)(sizeof (data) * 8);

   if (ioctl (fd, IFX_TAPI_RING_CADENCE_HR_SET, (IFX_uintptr_t) &ringCadence)
      != IFX_SUCCESS)
   return IFX_ERROR;

   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_RING_CADENCE_t ringCadence;
   IFX_TAPI_RING_t ring;
   IFX_char_t data[15] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
   0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00 };

   memset (&ringCadence, 0, sizeof (ringCadence));

   // Program the cadence
   memcpy(&ringCadence.data, data, sizeof (data));
   ringCadence.nr = (int)(sizeof (data) * 8);

   // Ringing should be done on the second analog line of the third device
   ringCadence.dev = 2;
   ringCadence.ch = 1;
   if (ioctl (fd, IFX_TAPI_RING_CADENCE_HR_SET, (IFX_uintptr_t) &ringCadence)
      != IFX_SUCCESS)
   return IFX_ERROR;

   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_RING_CADENCE_HR_SET        _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_RING_CADENCE_HR_SET_IDX, IFX_TAPI_RING_CADENCE_t)


/** This service sets the maximum number of cadences after which ringing stops
    automatically.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \param int The parameter defines the number of cadences to be played.
              A value of 0 means infinity.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \tapiv3
   \code
   IFX_TAPI_RING_MAX_t param;

   memset (&param, 0, sizeof (param));

   // set the maximal repeats of cadence
   param = 3;

   if (ioctl (fd, IFX_TAPI_RING_MAX_SET, (IFX_int32_t) param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_RING_MAX_t param;

   memset (&param, 0, sizeof (param));

   // set the maximal repeats of cadence
   param.nMaxRings = 3;

   // Second channel on the second device
   param.ch = 1;
   param.dev = 1;

   if (ioctl (fd, IFX_TAPI_RING_MAX_SET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_RING_MAX_SET               _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_RING_MAX_SET_IDX, IFX_TAPI_RING_MAX_t)


/** This service starts the non-blocking ringing on the phone line using the
    pre-configured ring cadence.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \tapiv4
   \param IFX_TAPI_RING_t* Pointer to an \ref IFX_TAPI_RING_t structure.
   \endtapiv4
   \tapiv3
   \param int This interface expects no parameters. It should be set to 0.
   \endtapiv3

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \remarks
   This interface does not not provide caller ID services.
   If ringing with caller ID is desired, \ref IFX_TAPI_CID_TX_SEQ_START should be
   used! The ringing must be stopped with \ref IFX_TAPI_RING_STOP. A second call
   to \ref IFX_TAPI_RING_START while the phone is ringing returns an error.
   The ringing can be configured with the interface
   \ref IFX_TAPI_RING_CADENCE_HR_SET, before this interface is called.

   \tapiv3
   \code
   if (ioctl (fd, IFX_TAPI_RING_START, 0) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_RING_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the second device
   param.ch = 1;
   param.dev = 1;
   // start the ringing
   if (ioctl (fd, IFX_TAPI_RING_START, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#ifdef TAPI_ONE_DEVNODE
   #define  IFX_TAPI_RING_START              _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_RING_START_IDX, IFX_TAPI_RING_t)
#else /* TAPI_ONE_DEVNODE */
   #define  IFX_TAPI_RING_START              _IO   (IFX_TAPI_IOC_MAGIC, IFX_TAPI_RING_START_IDX)
#endif /* TAPI_ONE_DEVNODE */


/** This service rings the phone. The service is blocking and will not return
    until the phone is off-hook or the maximum number of ring cadences as
    previously set by \ref IFX_TAPI_RING_MAX_SET have been played.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \param int This interface expects no parameters. It should be set to 0.

   \return The execution status can be:
      - 0: number of rings has been reached
      - 1: phone was unhooked
      - -1: in case of an error

   \code
   int nMaxRing = 3, ret;
   // set the maximum rings
   ioctl(fd, IFX_TAPI_RING_MAX_SET, nMaxRing);
   // ring the phone
   ret = ioctl(fd, IFX_TAPI_RING, 0);
   if (ret == 0)
   {
      // no answer, maximum number of rings reached
    }
    else if (ret == 1)
    {
      // phone unhooked
    }
   \endcode
*/
#define  IFX_TAPI_RING                       _IO (IFX_TAPI_IOC_MAGIC, IFX_TAPI_RING_IDX)


/** This service stops non-blocking ringing on the phone line that was previously started
   with the \ref IFX_TAPI_RING_START or \ref IFX_TAPI_CID_TX_SEQ_START services.
   This command applies to any channel file descriptor that
   includes an analog (ALM) module resource.

   \tapiv4
   \param IFX_TAPI_RING_t* Pointer to an \ref IFX_TAPI_RING_t structure.
   \endtapiv4
   \tapiv3
   \param int This interface expects no parameters. It should be set to 0.
   \endtapiv3

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \tapiv3
   \code
   if (ioctl (fd, IFX_TAPI_RING_STOP, 0) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_RING_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the second device
   param.ch = 1;
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_RING_STOP, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#ifdef TAPI_ONE_DEVNODE
   #define  IFX_TAPI_RING_STOP               _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_RING_STOP_IDX, IFX_TAPI_RING_t)
#else /* TAPI_ONE_DEVNODE */
   #define  IFX_TAPI_RING_STOP               _IO   (IFX_TAPI_IOC_MAGIC, IFX_TAPI_RING_STOP_IDX)
#endif /* TAPI_ONE_DEVNODE */

/**@}*/ /* TAPI_INTERFACE_RINGING */


/* ==================================================================== */
/* TAPI PCM Services, ioctl commands (Group TAPI_INTERFACE_PCM)         */
/* ==================================================================== */
/** \addtogroup TAPI_INTERFACE_PCM */
/**@{*/

/** This service sets the configuration of the PCM interface; it must be
    called after \ref IFX_TAPI_DEV_START but before activating a PCM channel.
    After activating a PCM channel, the PCM interface settings cannot be
    modified anymore.
    If a PCM channel is activated without calling this ioctl first the
    PCM interface is automatically configured with default settings.
    This command applies to any channel file descriptor
    that includes a PCM module resource.

    \param IFX_TAPI_PCM_IF_CFG_t* Pointer to an \ref IFX_TAPI_PCM_IF_CFG_t structure
   \tapiv3
   \code
   IFX_TAPI_PCM_IF_CFG_t param;

   memset (&param, 0, sizeof (param));

   param.nOpMode       = IFX_TAPI_PCM_IF_MODE_SLAVE;
   param.nDCLFreq      = IFX_TAPI_PCM_IF_DCLFREQ_2048;
   param.nDoubleClk    = IFX_DISABLE;
   param.nSlopeTX      = IFX_TAPI_PCM_IF_SLOPE_RISE;
   param.nSlopeRX      = IFX_TAPI_PCM_IF_SLOPE_FALL;
   param.nOffsetTX     = IFX_TAPI_PCM_IF_OFFSET_NONE;
   param.nOffsetRX     = IFX_TAPI_PCM_IF_OFFSET_NONE;
   param.nDrive        = IFX_TAPI_PCM_IF_DRIVE_ENTIRE;
   param.nShift        = IFX_DISABLE;
   param.nMCTS         = 0x00;
   param.nTsSync       = IFX_TAPI_PCM_IF_TS_SYNC_DEFAULT;
   param.nOffsetSlicTs = 0;

   // Configure the second PCM highway of the device
   param.nHighway = 0;

   if (ioctl (fd, IFX_TAPI_PCM_IF_CFG_SET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_PCM_IF_CFG_t param;

   memset (&param, 0, sizeof (param));

   param.nOpMode       = IFX_TAPI_PCM_IF_MODE_SLAVE;
   param.nDCLFreq      = IFX_TAPI_PCM_IF_DCLFREQ_2048;
   param.nDoubleClk    = IFX_DISABLE;
   param.nSlopeTX      = IFX_TAPI_PCM_IF_SLOPE_RISE;
   param.nSlopeRX      = IFX_TAPI_PCM_IF_SLOPE_FALL;
   param.nOffsetTX     = IFX_TAPI_PCM_IF_OFFSET_NONE;
   param.nOffsetRX     = IFX_TAPI_PCM_IF_OFFSET_NONE;
   param.nDrive        = IFX_TAPI_PCM_IF_DRIVE_ENTIRE;
   param.nShift        = IFX_DISABLE;
   param.nMCTS         = 0x00;
   param.nTsSync       = IFX_TAPI_PCM_IF_TS_SYNC_DEFAULT;
   param.nOffsetSlicTs = 0;

   // Configure the second PCM highway of the device
   param.nHighway = 0;

   // Second device
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_PCM_IF_CFG_SET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_PCM_IF_CFG_SET             _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_PCM_IF_CFG_SET_IDX, IFX_TAPI_PCM_IF_CFG_t)


/** This service sets the configuration of a PCM channel.
    This command applies to any channel file descriptor
    that includes a PCM module resource.

   \param IFX_TAPI_PCM_CFG_t* Pointer to an \ref IFX_TAPI_PCM_CFG_t structure.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \remarks The parameter rate must be set to the PCM rate that is applied to the
   device, otherwise an error is returned.

   \tapiv3
   \code
   IFX_TAPI_PCM_CFG_t param;

   memset (&param, 0, sizeof (param));

   param.nTimeslotRX = 5;
   param.nTimeslotTX = 5;
   param.nHighway = 1;

   // 16-bit resolution
   param.nResolution = IFX_TAPI_PCM_RES_NB_LINEAR_16BIT;

   if (ioctl (fd, IFX_TAPI_PCM_CFG_SET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_PCM_CFG_t param;

   memset (&param, 0, sizeof (param));

   param.nTimeslotRX = 5;
   param.nTimeslotTX = 5;
   param.nHighway = 1;

   // 16-bit resolution
   param.nResolution = IFX_TAPI_PCM_RES_NB_LINEAR_16BIT;

   // Second channel on the second device
   param.ch = 1;
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_PCM_CFG_SET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_PCM_CFG_SET                _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_PCM_CFG_SET_IDX, IFX_TAPI_PCM_CFG_t)


/** This service gets the configuration of the PCM channel.
    This command applies to any channel file descriptor
    that includes a PCM module resource.

   \param IFX_TAPI_PCM_CFG_t* Pointer to an \ref IFX_TAPI_PCM_CFG_t structure.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \tapiv3
   \code
   IFX_TAPI_PCM_CFG_t param;

   memset (&param, 0, sizeof (param));

   if (ioctl (fd, IFX_TAPI_PCM_CFG_GET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_PCM_CFG_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the second device
   param.ch = 1;
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_PCM_CFG_GET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_PCM_CFG_GET                _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_PCM_CFG_GET_IDX, IFX_TAPI_PCM_CFG_t)


/** This service activates/deactivates the PCM time slots configured for this
    channel.
    This command applies to any channel file descriptor that includes a
    PCM module resource.

   \param IFX_TAPI_PCM_ACTIVATION_t* Pointer to an \ref IFX_TAPI_PCM_ACTIVATION_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \tapiv3
   \code
   IFX_TAPI_PCM_ACTIVATION_t param;

   memset (&param, 0, sizeof (param));

   // activate the PCM time slot
   param = IFX_ENABLE;

   if (ioctl (fd, IFX_TAPI_PCM_ACTIVATION_SET, (IFX_int32_t) param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
\endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_PCM_ACTIVATION_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the second device
   param.ch = 1;
   param.dev = 1;
   // activate the PCM time slot
   param.mode = IFX_ENABLE;

   if (ioctl (fd, IFX_TAPI_PCM_ACTIVATION_SET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
\endcode
   \endtapiv4
*/
#define  IFX_TAPI_PCM_ACTIVATION_SET         _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_PCM_ACTIVATION_SET_IDX, IFX_TAPI_PCM_ACTIVATION_t)


/** This service receives the activation status from the PCM time slots configured
   for this channel.
   This command applies to any channel file descriptor that
   includes a PCM module resource.

   \param IFX_TAPI_PCM_ACTIVATION_t* Pointer to an \ref IFX_TAPI_PCM_ACTIVATION_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \tapiv3
   \code
   IFX_TAPI_PCM_ACTIVATION_t param;

   memset (&param, 0, sizeof (param));

   if (ioctl (fd, IFX_TAPI_PCM_ACTIVATION_GET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   if (IFX_ENABLE == param)
   {
      printf("Activated");
   } else {
      printf("Deactivated");
   }

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_PCM_ACTIVATION_t param;

   memset (&param, 0, sizeof (param));

   // Second channel on the second device
   param.ch = 1;
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_PCM_ACTIVATION_GET, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   if (IFX_ENABLE == param.mode)
   {
      printf("Activated");
   } else {
      printf("Deactivated");
   }

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_PCM_ACTIVATION_GET         _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_PCM_ACTIVATION_GET_IDX, IFX_TAPI_PCM_ACTIVATION_t)

/**@}*/ /* TAPI_INTERFACE_PCM */


/* ======================================================================= */
/* TAPI Test Services, ioctl commands (Group TAPI_INTERFACE_TEST)          */
/* ======================================================================= */

/** \addtogroup TAPI_INTERFACE_TEST */
/**@{*/
/** Forces generation of on-/off-hook.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \param IFX_TAPI_TEST_HOOKGEN_t* Pointer to an \ref IFX_TAPI_TEST_HOOKGEN_t structure.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \remarks After switching the hook state, the hook event gets to the hook
    state machine for validation. Depending on the timing of the interface call,
    hook flash and pulse dialing can be verified. The example
   shows the generation of a flash hook with a timing of 100 ms. The flash hook
   can be queried afterwards using the \ref IFX_TAPI_EVENT_GET ioctl.

   \tapiv3
   \code
   IFX_TAPI_TEST_HOOKGEN_t param;

   memset (&param, 0, sizeof (param));

   param = IFX_TAPI_HOOKGEN_ONHOOK;

   if (ioctl (fd, IFX_TAPI_TEST_HOOKGEN, (IFX_int32_t) param) != IFX_SUCCESS)
      return IFX_ERROR;

   param = IFX_TAPI_HOOKGEN_OFFHOOK;

   // generate off hook for 100 ms
   if (ioctl (fd, IFX_TAPI_TEST_HOOKGEN, (IFX_int32_t) param) != IFX_SUCCESS)
      return IFX_ERROR;

   sleep (100);

   param = IFX_TAPI_HOOKGEN_ONHOOK;

   if (ioctl (fd, IFX_TAPI_TEST_HOOKGEN, (IFX_int32_t) param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_TEST_HOOKGEN_t param;

   memset (&param, 0, sizeof (param));

   param.hookMode = IFX_TAPI_HOOKGEN_ONHOOK;

   // Second channel on the second device
   param.ch = 1;
   param.dev = 1;

   if (ioctl (fd, IFX_TAPI_TEST_HOOKGEN, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   param.hookMode = IFX_TAPI_HOOKGEN_OFFHOOK;

   // generate off hook for 100 ms
   if (ioctl (fd, IFX_TAPI_TEST_HOOKGEN, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   sleep (100);

   param.hookMode = IFX_TAPI_HOOKGEN_ONHOOK;

   if (ioctl (fd, IFX_TAPI_TEST_HOOKGEN, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_TEST_HOOKGEN               _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_TEST_HOOKGEN_IDX, IFX_TAPI_TEST_HOOKGEN_t)


/** Enables a local test loop in the analog part. The digital voice data is
   transparently looped back to the network without affecting downstream
   transmission. Local voice reception (upstream) is disabled,
   This means that voice applied to the local phone is not being processed,
   but data sent to the phone can still be heard.
   This command applies to any channel file descriptor that
   includes an analog (ALM) module resource.

   \param IFX_TAPI_TEST_LOOP_t* Pointer to an \ref IFX_TAPI_TEST_LOOP_t structure.

   \return Returns the following values:
     - IFX_SUCCESS: if successful
     - IFX_ERROR: in case of an error

   \tapiv3
   \code
   IFX_TAPI_TEST_LOOP_t param;

   memset (&param, 0, sizeof (param));
   param.bAnalog = 1;

   if (ioctl (fd, IFX_TAPI_TEST_LOOP, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   IFX_TAPI_TEST_LOOP_t param;

   memset (&param, 0, sizeof (param));
   param.bAnalog = 1;

   // Second channel on the second device
   param.ch = 1;
   param.dev = 1;
   if (ioctl (fd, IFX_TAPI_TEST_LOOP, (IFX_uintptr_t) &param) != IFX_SUCCESS)
      return IFX_ERROR;

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
*/
#define  IFX_TAPI_TEST_LOOP                  _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_TEST_LOOP_IDX, IFX_TAPI_TEST_LOOP_t)

/**@}*/ /* TAPI_INTERFACE_TEST */


/* ===================================================================== */
/* TAPI Event Services, ioctl commands (Group TAPI_INTERFACE_EVENT)      */
/* ===================================================================== */

/** \addtogroup TAPI_INTERFACE_EVENT */
/**@{*/

/** IFX_TAPI_EVENT_GET always returns IFX_SUCCESS as long as the channel
   parameter is not out of range, or if no event was available.
   If the parameter is out of range then IFX_ERROR is returned.
   As the event data is not valid in this case, the value
   \ref IFX_TAPI_EVENT_NONE is returned in the 'id' field of the event.
   This ioctl indicates that additional events are ready to be retrieved.
   The information is provided in the 'more' field of the returned
    \ref IFX_TAPI_EVENT_t structure.
    This command applies to the device file descriptor.

   \param IFX_TAPI_EVENT_t* Pointer to an \ref IFX_TAPI_EVENT_t structure.

   \return Returns the following values:
    - IFX_SUCCESS: if successful
    - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_EVENT_GET                   _IOR  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_EVENT_GET_IDX, IFX_TAPI_EVENT_t)


/** Enable detection of a single event.
    This command applies to the device file descriptor.

   \param IFX_TAPI_EVENT_t* Pointer to an \ref IFX_TAPI_EVENT_t structure.

   \return Returns the following values:
    - IFX_SUCCESS: if successful
    - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_EVENT_ENABLE                _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_EVENT_ENABLE_IDX, IFX_TAPI_EVENT_t)


/** Disable detection of a single event.
    This command applies to the device file descriptor.

   \param IFX_TAPI_EVENT_t* Pointer to an \ref IFX_TAPI_EVENT_t structure.

   \return Returns the following values:
    - IFX_SUCCESS: if successful
    - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_EVENT_DISABLE               _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_EVENT_DISABLE_IDX, IFX_TAPI_EVENT_t)


/** Enable detection of multiple events. All events are enabled on
    a single target module on one specific device. The number of events
    stored in 'pEvent' for enabling has to be set in 'nCount' of
    \ref IFX_TAPI_EVENT_MULTI_t.
    This command applies to the device file descriptor.

   \param IFX_TAPI_EVENT_MULTI_t* Pointer to an \ref IFX_TAPI_EVENT_MULTI_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \remarks Memory allocated for 'pEvent' should be equal
      to (sizeof (IFX_TAPI_EVENT_ENTRY_t) * nCont)
*/
#define IFX_TAPI_EVENT_MULTI_ENABLE          _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_EVENT_MULTI_ENABLE_IDX, IFX_TAPI_EVENT_MULTI_t)


/** Disable detection of multiple events. All events are disabled on
    a single target module on one specific device. The number of events
    stored in 'pEvent' for disabling has to be set in 'nCount' of
    \ref IFX_TAPI_EVENT_MULTI_t.
    This command applies to the device file descriptor.

   \param IFX_TAPI_EVENT_MULTI_t* Pointer to an \ref IFX_TAPI_EVENT_MULTI_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \remarks Memory allocated for 'pEvent' should be equal
      to (sizeof (IFX_TAPI_EVENT_ENTRY_t) * nCont)
*/
#define IFX_TAPI_EVENT_MULTI_DISABLE         _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_EVENT_MULTI_DISABLE_IDX, IFX_TAPI_EVENT_MULTI_t)
/**@}*/ /* TAPI_INTERFACE_EVENT */


/* ======================================================================= */
/* TAPI Message Waiting Lamp Services, ioctl commands                      */
/* (Group TAPI_INTERFACE_MWL)                                              */
/* ======================================================================= */
/** \addtogroup TAPI_INTERFACE_MWL */
/**@{*/
/** This service activates/deactivates the message waiting lamp using configurable
    analog signals on the line.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

    \remarks
    Every change of the line mode via \ref IFX_TAPI_LINE_FEED_SET disables
    the MWL service. It is up to the application to restart the MWL service
    (if required), after switching back to standby mode.

   \param IFX_TAPI_MWL_ACTIVATION_t* Pointer to an \ref IFX_TAPI_MWL_ACTIVATION_t structure containing the
   activation status.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_MWL_ACTIVATION_SET          _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_MWL_ACTIVATION_SET_IDX, IFX_TAPI_MWL_ACTIVATION_t)


/** This service gets the activation status of the message waiting lamp.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \param IFX_TAPI_MWL_ACTIVATION_t* Pointer to an \ref IFX_TAPI_MWL_ACTIVATION_t structure containing the
   activation status.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_MWL_ACTIVATION_GET          _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_MWL_ACTIVATION_GET_IDX, IFX_TAPI_MWL_ACTIVATION_t)
/**@}*/ /* TAPI_INTERFACE_MWL */


/* ========================================================================== */
/* TAPI Calibration Services, ioctl commands                                  */
/* (Group TAPI_INTERFACE_CALIBRATION)                                         */
/* ========================================================================== */
/** \addtogroup TAPI_INTERFACE_CALIBRATION */
/**@{*/

/** This function starts the analog line calibration process. It returns while
    the calibration process is still running (unblocking call). Calibration is
    done on the analog line.
    The calibration process stops automatically when finished or when an error
    occurs. The termination, including the reason, is reported autonomously with
    the event \ref IFX_TAPI_EVENT_CALIBRATION_END.
    The TAPI compares the calibration results, stored in the TAPI low-level driver,
    with pre-defined SLIC value ranges. It overwrites the results with default
    values in case the results are out of range.
    All calibration result parameters can be read by
    \ref IFX_TAPI_CALIBRATION_RESULTS_GET. The currently used analog line
    coefficients can be read by \ref IFX_TAPI_CALIBRATION_CFG_GET.
    \ref IFX_TAPI_CALIBRATION_CFG_SET allows new coefficients to be set
    if required.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \tapiv4
   \param IFX_TAPI_CALIBRATION_t* Pointer to an \ref IFX_TAPI_CALIBRATION_t structure.
   \endtapiv4
   \tapiv3
   \param int This interface expects no parameters. It should be set to 0.
   \endtapiv3

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error
*/
#ifdef TAPI_ONE_DEVNODE
   #define IFX_TAPI_CALIBRATION_START        _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CALIBRATION_START_IDX, IFX_TAPI_CALIBRATION_t)
#else /* TAPI_ONE_DEVNODE */
   #define IFX_TAPI_CALIBRATION_START        _IO   (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CALIBRATION_START_IDX)
#endif /* TAPI_ONE_DEVNODE */


/** This function stops a currently running analog line calibration process.
    A calibration can be started by IFX_TAPI_CALIBRATION_START.
    This function is called by the application in case the calibration did
    not finish successfully after a dedicated time-out (around 2 seconds).
    Stopping calibration means that the analog line coefficients are not
    well configured. The analog line has to be calibrated again using
    IFX_TAPI_CALIBRATION_START, or pre-defined values have to be programmed
    with IFX_TAPI_CALIBRATION_CFG_SET.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \tapiv4
   \param IFX_TAPI_CALIBRATION_t* Pointer to an \ref IFX_TAPI_CALIBRATION_t structure.
   \endtapiv4
   \tapiv3
   \param int This interface expects no parameters. It should be set to 0.
   \endtapiv3

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error
*/
#ifdef TAPI_ONE_DEVNODE
   #define IFX_TAPI_CALIBRATION_STOP         _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CALIBRATION_STOP_IDX, IFX_TAPI_CALIBRATION_t)
#else /* TAPI_ONE_DEVNODE */
   #define IFX_TAPI_CALIBRATION_STOP         _IO   (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CALIBRATION_STOP_IDX)
#endif /* TAPI_ONE_DEVNODE */


/** This function reads out the analog line coefficient results that
    are generated by an automatic calibration process.
    These values might differ from the currently used coefficients.
    IFX_TAPI_CALIBRATION_START starts the automatic calibration and
    IFX_TAPI_CALIBRATION_CFG_SET allows the coefficients to be written.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \param IFX_TAPI_CALIBRATION_CFG_t* Pointer to an
      \ref IFX_TAPI_CALIBRATION_CFG_t structure.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_CALIBRATION_RESULTS_GET     _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CALIBRATION_RESULTS_GET_IDX, IFX_TAPI_CALIBRATION_CFG_t)


/** This function sets the analog line coefficients.
    This command writes the coefficients for the addressed analog line.
    These coefficients could also be generated by an automatic calibration
    process (\ref IFX_TAPI_CALIBRATION_START).
    All calibration process result parameters can be read by
    \ref IFX_TAPI_CALIBRATION_RESULTS_GET. The currently used analog line
    coefficients can be read by \ref IFX_TAPI_CALIBRATION_CFG_GET.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \param IFX_TAPI_CALIBRATION_CFG_t* Pointer to an
      \ref IFX_TAPI_CALIBRATION_CFG_t structure.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_CALIBRATION_CFG_SET         _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CALIBRATION_CFG_SET_IDX, IFX_TAPI_CALIBRATION_CFG_t)


/** This function reads out the analog line coefficients. It reads the
    currently used coefficients for the addressed analog line.
    \ref IFX_TAPI_CALIBRATION_CFG_SET allows the coefficients to be modified.
    \ref IFX_TAPI_CALIBRATION_START could be used to start an automatic
    calibration process for these coefficients. All calibration process
    result parameters can be read by \ref IFX_TAPI_CALIBRATION_RESULTS_GET.
    This command applies to any channel file descriptor
    that includes an analog (ALM) module resource.

   \param IFX_TAPI_CALIBRATION_CFG_t* Pointer to an
      \ref IFX_TAPI_CALIBRATION_CFG_t structure.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_CALIBRATION_CFG_GET         _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CALIBRATION_CFG_GET_IDX, IFX_TAPI_CALIBRATION_CFG_t)

/**@}*/ /* TAPI_INTERFACE_CALIBRATION */


/* ========================================================================== */
/* TAPI Analog Line Continuous Measurement Services, ioctl commands           */
/* (Group TAPI_INTERFACE_CONTMEASUREMENT)                                               */
/* ========================================================================== */
/** \addtogroup TAPI_INTERFACE_CONTMEASUREMENT */
/**@{*/

/** This function triggers the request to read out the results of the
    continuous analog line measurement process. The TAPI generates an event when
    the requested measurement results are available. The application then
    calls \ref IFX_TAPI_CONTMEASUREMENT_GET to read out the results.
    This command applies to phone channel file descriptors which contain
    an analog (ALM) module resource.

   \tapiv4
   \param IFX_TAPI_CONTMEASUREMENT_t* Pointer to an \ref IFX_TAPI_CONTMEASUREMENT_t structure.
   \endtapiv4
   \tapiv3
   \param int This interface expects no parameters. It should be set to 0.
   \endtapiv3

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error

   \note This function is non-blocking and returns immediately.
*/
#ifdef TAPI_ONE_DEVNODE
   #define IFX_TAPI_CONTMEASUREMENT_REQ      _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CONTMEASUREMENT_REQ_IDX, IFX_TAPI_CONTMEASUREMENT_t)
#else /* TAPI_ONE_DEVNODE */
   #define IFX_TAPI_CONTMEASUREMENT_REQ      _IO   (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CONTMEASUREMENT_REQ_IDX)
#endif /* TAPI_ONE_DEVNODE */


/** This function reads results of the continuous analog line measurement
    process. \ref IFX_TAPI_CONTMEASUREMENT_REQ is used to request new
    measurement values from the firmware. An event is generated by the TAPI
    when new measurement results are available.
    This command applies to phone channel file descriptors which contain
    an analog (ALM) module resource.

   \param IFX_TAPI_CONTMEASUREMENT_GET_t* Pointer to an \ref IFX_TAPI_CONTMEASUREMENT_GET_t structure.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error

   \note This function returns with an error in case there are no results to be
   read out, or if the results have already been read before.
*/
#define IFX_TAPI_CONTMEASUREMENT_GET         _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_CONTMEASUREMENT_GET_IDX, IFX_TAPI_CONTMEASUREMENT_GET_t)

/**@}*/ /* TAPI_INTERFACE_CONTMEASUREMENT */


/* ======================================================================= */
/* TAPI Network Line Testing services, ioctl commands                       */
/* (Group TAPI_INTERFACE_NLT)                                              */
/* ======================================================================= */
/** \addtogroup TAPI_INTERFACE_NLT */
/**@{*/


/** This service starts an NLT test.
    This command applies to phone channel file descriptors which contain
    an analog (ALM) module resource.

   \param IFX_TAPI_NLT_TEST_START_t Value from \ref IFX_TAPI_NLT_TEST_START_t; specifies the target device and channel
   on which the test is to be performed

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_NLT_TEST_START              _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_NLT_TEST_START_IDX, IFX_TAPI_NLT_TEST_START_t)


/** This service reads test results of the specified NLT test.
    This command applies to phone channel file descriptors which contain
    an analog (ALM) module resource.

   \param IFX_TAPI_NLT_RESULT_GET_t Value from \ref IFX_TAPI_NLT_RESULT_GET_t; specifies the target
   device and channel on which the test was performed, and consequently test
   results are to be read.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_NLT_RESULT_GET              _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_NLT_RESULT_GET_IDX, IFX_TAPI_NLT_RESULT_GET_t)


#ifndef TAPI_DXY_DOC
/** This function is used to configure the open loop calibration factors
    of the measurement path for line testing.

    \param IFX_TAPI_NLT_CONFIGURATION_OL_t* Pointer to an
      \ref IFX_TAPI_NLT_CONFIGURATION_OL_t structure.

    \return Returns the following values:
    - IFX_SUCCESS: if successful
    - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_NLT_CONFIGURATION_OL_SET  _IOW (IFX_TAPI_IOC_MAGIC, IFX_TAPI_NLT_CONFIGURATION_OL_SET_IDX, IFX_TAPI_NLT_CONFIGURATION_OL_t)


/** This function is used to read the open loop calibration factors
    of the measurement path for line testing.

    \param IFX_TAPI_NLT_CONFIGURATION_OL_t* Pointer to an
      \ref IFX_TAPI_NLT_CONFIGURATION_OL_t structure.

    \return Returns the following values:
    - IFX_SUCCESS: if successful
    - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_NLT_CONFIGURATION_OL_GET  _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_NLT_CONFIGURATION_OL_GET_IDX, IFX_TAPI_NLT_CONFIGURATION_OL_t)


/** This function is used to define the configuration of the measurement path
    for line testing according to Rmes resitor.

    \param IFX_TAPI_NLT_CONFIGURATION_RMES_t* Pointer to an
      \ref IFX_TAPI_NLT_CONFIGURATION_RMES_t structure.

    \return Returns the following values:
    - IFX_SUCCESS: if successful
    - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_NLT_CONFIGURATION_RMES_SET  _IOW (IFX_TAPI_IOC_MAGIC, IFX_TAPI_NLT_CONFIGURATION_RMES_SET_IDX, IFX_TAPI_NLT_CONFIGURATION_RMES_t)


/** This function starts an analog line capacitance measurement cycle.
    The TAPI generates an \ref IFX_TAPI_EVENT_NLT_END event when the
    measurement is completed. This measurement can be stopped at any time
    by calling \ref IFX_TAPI_NLT_CAPACITANCE_STOP.

   \param IFX_TAPI_NLT_CAPACITANCE_START_t* Pointer to an
      \ref IFX_TAPI_NLT_CAPACITANCE_START_t structure.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_NLT_CAPACITANCE_START _IOW (IFX_TAPI_IOC_MAGIC, IFX_TAPI_NLT_CAPACITANCE_START_IDX, IFX_TAPI_NLT_CAPACITANCE_START_t)


/** This function stops any currently running analog line capacitance
    measurement session.

   \param IFX_TAPI_NLT_CAPACITANCE_STOP_t* Pointer to an
      \ref IFX_TAPI_NLT_CAPACITANCE_STOP_t structure.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_NLT_CAPACITANCE_STOP  _IOW (IFX_TAPI_IOC_MAGIC, IFX_TAPI_NLT_CAPACITANCE_STOP_IDX, IFX_TAPI_NLT_CAPACITANCE_STOP_t)


/** This function reads the results of the capacitance measurement.

    \param IFX_TAPI_NLT_CAPACITANCE_RESULT_t* Pointer to an
      \ref IFX_TAPI_NLT_CAPACITANCE_RESULT_t structure.

    \return Returns the following values:
    - IFX_SUCCESS: if successful
    - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_NLT_CAPACITANCE_RESULT_GET  _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_NLT_CAPACITANCE_RESULT_GET_IDX, IFX_TAPI_NLT_CAPACITANCE_RESULT_t)

#endif /* #ifndef TAPI_DXY_DOC */
/**@}*/ /* TAPI_INTERFACE_NLT */


#ifndef TAPI4_DXY_DOC
/* ========================================================================== */
/* TAPI GR909 Services, ioctl commands                                        */
/* This is not a public interface of the driver. It is intended to be used    */
/* by the linetesting library code.                                           */
/* (Group TAPI_INTERFACE_GR909)                                               */
/* ========================================================================== */
/** \addtogroup TAPI_INTERFACE_GR909 */
/**@{*/

/** This function starts an GR909 measurement.

   \param IFX_TAPI_GR909_START_t* Pointer to an \ref IFX_TAPI_GR909_START_t structure.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_GR909_START                 _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_GR909_START_IDX, IFX_TAPI_GR909_START_t)


/** This function stops an GR909 measurement.

   \tapiv4
   \param IFX_TAPI_GR909_STOP_t* Pointer to an \ref IFX_TAPI_GR909_STOP_t structure.
   \endtapiv4
   \tapiv3
   \param int This interface expects no parameters. It should be set to 0.
   \endtapiv3

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error
*/
#ifdef TAPI_ONE_DEVNODE
   #define IFX_TAPI_GR909_STOP               _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_GR909_STOP_IDX, IFX_TAPI_GR909_STOP_t)
#else /* TAPI_ONE_DEVNODE */
   #define IFX_TAPI_GR909_STOP               _IO   (IFX_TAPI_IOC_MAGIC, IFX_TAPI_GR909_STOP_IDX)
#endif /* TAPI_ONE_DEVNODE */


/** This function reads an returns the results of a GR909 measurement.

   \param IFX_TAPI_GR909_RESULT_t* Pointer to an \ref IFX_TAPI_GR909_RESULT_t structure.

   \return Returns the following values:
   - IFX_SUCCESS: if successful
   - IFX_ERROR: in case of an error
*/
#define IFX_TAPI_GR909_RESULT                _IOWR (IFX_TAPI_IOC_MAGIC, IFX_TAPI_GR909_RESULT_IDX, IFX_TAPI_GR909_RESULT_t)

/**@}*/ /* TAPI_INTERFACE_GR909 */
#endif /* #ifndef TAPI4_DXY_DOC */


#ifndef TAPI4_DXY_DOC
/* ========================================================================== */
/* TAPI FXS Phone Detection Services, ioctl commands                          */
/* (Group TAPI_INTERFACE_PHONE_DETECTION)                                     */
/* ========================================================================== */
/** \addtogroup TAPI_INTERFACE_PHONE_DETECTION */
/**@{*/

/** Important: The use of this interface is deprecated. Use the linetesting
    library instead.
    This function starts an analog line capacitance measurement session.
    The TAPI generates an \ref IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY event
    containing the results when the measurement is completed.
    This measurement can be stopped at any time by calling
    \ref IFX_TAPI_LINE_MEASURE_CAPACITANCE_STOP or selecting another line
    feeding mode. This command does not take any parameters.

   \tapiv4
   \param IFX_TAPI_LINE_MEASURE_CAPACITANCE_t* Pointer to an \ref IFX_TAPI_LINE_MEASURE_CAPACITANCE_t structure.
   \endtapiv4
   \tapiv3
   \param int This interface expects no parameters. It should be set to 0.
   \endtapiv3

   \return Returns the following values:
    - IFX_SUCCESS: if successful
    - IFX_ERROR: in case of an error
*/
#ifdef TAPI_ONE_DEVNODE
   #define IFX_TAPI_LINE_MEASURE_CAPACITANCE_START _IOW (IFX_TAPI_IOC_MAGIC, IFX_TAPI_LINE_MEASURE_CAPACITANCE_START_IDX, IFX_TAPI_LINE_MEASURE_CAPACITANCE_t)
#else /* TAPI_ONE_DEVNODE */
   #define IFX_TAPI_LINE_MEASURE_CAPACITANCE_START _IO  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_LINE_MEASURE_CAPACITANCE_START_IDX)
#endif /* TAPI_ONE_DEVNODE */


/** Important: The use of this interface is deprecated. Use the linetesting
    library instead.
    This function stops any currently running analog line capacitance measurement session.
    A measurement can be started by calling
    \ref IFX_TAPI_LINE_MEASURE_CAPACITANCE_START.
    This command does not take any parameters.

   \tapiv4
   \param IFX_TAPI_LINE_MEASURE_CAPACITANCE_t* Pointer to an \ref IFX_TAPI_LINE_MEASURE_CAPACITANCE_t structure.
   \endtapiv4
   \tapiv3
   \param int This interface expects no parameters. It should be set to 0.
   \endtapiv3

   \return Returns the following values:
    - IFX_SUCCESS: if successful
    - IFX_ERROR: in case of an error
*/
#ifdef TAPI_ONE_DEVNODE
   #define IFX_TAPI_LINE_MEASURE_CAPACITANCE_STOP  _IOW (IFX_TAPI_IOC_MAGIC, IFX_TAPI_LINE_MEASURE_CAPACITANCE_STOP_IDX, IFX_TAPI_LINE_MEASURE_CAPACITANCE_t)
#else /* TAPI_ONE_DEVNODE */
   #define IFX_TAPI_LINE_MEASURE_CAPACITANCE_STOP  _IO  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_LINE_MEASURE_CAPACITANCE_STOP_IDX)
#endif /* TAPI_ONE_DEVNODE */


/** This function reads out the current FXS phone detection state machine
    parameters. These parameters can be modified by calling
    \ref IFX_TAPI_LINE_PHONE_DETECT_CFG_SET.
    This command applies to any channel file descriptor for an analog (ALM)
    module resource.

   \param IFX_TAPI_LINE_PHONE_DETECT_CFG_t* Pointer to an \ref IFX_TAPI_LINE_PHONE_DETECT_CFG_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error
*/
#define  IFX_TAPI_LINE_PHONE_DETECT_CFG_GET  _IOR  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_LINE_PHONE_DETECT_CFG_GET_IDX, IFX_TAPI_LINE_PHONE_DETECT_CFG_t)


/** This function configures the FXS phone detection state machine parameters.
    These parameters specify, among others, time periods for off-hook detection
    and disabled line-state phases. The configured capacitance value specifies
    the threshold that indicates the detection of a telephone.
    \ref IFX_TAPI_LINE_PHONE_DETECT_CFG_GET allows for the current
    configuration to be read out.
    This command applies to any channel file descriptor for an analog (ALM)
    module resource.

   \param IFX_TAPI_LINE_PHONE_DETECT_CFG_t* Pointer to an \ref IFX_TAPI_LINE_PHONE_DETECT_CFG_t structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error
*/
#define  IFX_TAPI_LINE_PHONE_DETECT_CFG_SET  _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_LINE_PHONE_DETECT_CFG_SET_IDX, IFX_TAPI_LINE_PHONE_DETECT_CFG_t)

/**@}*/ /* TAPI_INTERFACE_PHONE_DETECTION */
#endif /* #ifndef TAPI4_DXY_DOC */


/** This function gets content of TAPI debug buffer as a copy.
    Size of debug buffer is determined by configured number of entries which by
    default is set to 1024 but can be changed using configure option:
       --with-tapi-debug-buf-entries[=VAL]
   The size of the debug buffer in bytes can be calculated with:
       sizeof(struct TapiDebugBufferEntry) * {configured number of entries}

   \remarks Only entires that were used are copied from kernel to user space.
         Number of copied entries can be checked in struct TapiDebugBufferContent
         copied_entries member.

   \param struct TapiDebugBufferContent* Pointer to a \ref TapiDebugBufferContent structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error

   \tapiv3
   \code
   struct TapiDebugBufferContent debug_buffer = {0};

   debug_buffer.data_length = TAPI_DEBUG_BUFFER_NUM_OF_ENTRIES * sizeof(struct TapiDebugBufferEntry);
   debug_buffer.data = (IFX_uint8_t *) malloc(debug_buffer.data_length);
   memset(debug_buffer.data, 0, debug_buffer.data_length);

   if (ioctl (fd, IFX_TAPI_DEBUG_BUFFER_GET, (IFX_uintptr_t) &debug_buffer) != IFX_SUCCESS)
      return IFX_ERROR;

   free(debug_buffer.data);

   return IFX_SUCCESS;
   \endcode
   \endtapiv3

   \tapiv4
   \code
   struct TapiDebugBufferContent debug_buffer = {0};

   debug_buffer.data_length = TAPI_DEBUG_BUFFER_NUM_OF_ENTRIES * sizeof(struct TapiDebugBufferEntry);
   debug_buffer.data = (IFX_uint8_t *) malloc(debug_buffer.data_length);
   memset(debug_buffer.data, 0, debug_buffer.data_length);

   if (ioctl (fd, IFX_TAPI_DEBUG_BUFFER_GET, (IFX_uintptr_t) &debug_buffer) != IFX_SUCCESS)
      return IFX_ERROR;

   free(debug_buffer.data);

   return IFX_SUCCESS;
   \endcode
   \endtapiv4
 */
#define  IFX_TAPI_DEBUG_BUFFER_GET      _IOWR  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_DEBUG_BUFFER_GET_IDX, struct TapiDebugBufferContent)


/** This function gets the configuration of the TAPI debug buffer.
    The caller can get the debug buffer current behaviour like: print_to_console,
    overwrite, paused.

   \param struct TapiDebugBufferConfig* Pointer to a \ref TapiDebugBufferConfig structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error
 */
#define  IFX_TAPI_DEBUG_BUFFER_CFG_GET  _IOR  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_DEBUG_BUFFER_CFG_GET_IDX, struct TapiDebugBufferConfig)


/** This function sets the configuration of the TAPI debug buffer.
    The caller can set the debug buffer behaviour like: print_to_console,
    overwrite, paused.

   \param struct TapiDebugBufferConfig* Pointer to a \ref TapiDebugBufferConfig structure.

   \return Returns the following values:
      - IFX_SUCCESS: if successful
      - IFX_ERROR: in case of an error
 */
#define  IFX_TAPI_DEBUG_BUFFER_CFG_SET  _IOW  (IFX_TAPI_IOC_MAGIC, IFX_TAPI_DEBUG_BUFFER_CFG_SET_IDX, struct TapiDebugBufferConfig)

/* ========================================================================= */
/*                     TAPI Interface Constants                              */
/* ========================================================================= */


/* ======================================================================= */
/* TAPI Operation Control Services, constants (Group TAPI_INTERFACE_OP)    */
/* ======================================================================= */
/** \addtogroup TAPI_INTERFACE_OP */
/**@{*/

/* TAPI phone volume control */

/** Switches the volume to low: -12 dB. */
#define IFX_TAPI_LINE_VOLUME_LOW                (-12)
/** Switches the volume to medium: -6 dB. */
#define IFX_TAPI_LINE_VOLUME_MEDIUM              (-6)
/** Switches the volume to high: 0 dB */
#define IFX_TAPI_LINE_VOLUME_HIGH                 (0)
/** Switches the volume to minimum gain: -24 dB; note that DTMF detection etc.
    might not work properly with such low signals. */
#define IFX_TAPI_LINE_VOLUME_MIN_GAIN           (-24)
/** Switches the volume to maximum gain: +24 dB. */
#define IFX_TAPI_LINE_VOLUME_MAX_GAIN            (24)


/**@}*/ /* TAPI_INTERFACE_OP */


/* ====================================================================== */
/* TAPI Tone Services, constants (Group TAPI_INTERFACE_TONE)              */
/* ====================================================================== */
/** \addtogroup TAPI_INTERFACE_TONE */
/**@{*/

/** Maximum number of simple tones that can be played at one go. */
#define IFX_TAPI_TONE_SIMPLE_MAX                  (7)

/** Maximum tone generation steps, also called cadences. */
#define IFX_TAPI_TONE_STEPS_MAX                   (6)

/** Tone minimum index that can be configured by the user. */
#define IFX_TAPI_TONE_INDEX_MIN                  (32)

/** Tone maximum index that can be configured by the user. */
#define IFX_TAPI_TONE_INDEX_MAX                 (255)

/**@}*/ /* TAPI_INTERFACE_TONE */

/* ===================================================================== */
/* TAPI Misc Services, constants (Group TAPI_INTERFACE_MISC)             */
/* ===================================================================== */
/** \addtogroup TAPI_INTERFACE_MISC */
/**@{*/

/** Used to report events from any channel in the device. This constant is also
   used to report events that cannot be associated with a particular channel.*/
#define IFX_TAPI_EVENT_ALL_CHANNELS               0xffff

/** Used to report events from any device. This constant is also
   used to report events that cannot be associated with a particular device. */
#define IFX_TAPI_EVENT_ALL_DEVICES                0xffff

/**@}*/ /* TAPI_INTERFACE_MISC */

/* =================================================================== */
/* TAPI Signal Detection Services, constants                           */
/* (Group TAPI_INTERFACE_SIGNAL)                                       */
/* =================================================================== */
/** \addtogroup TAPI_INTERFACE_SIGNAL */
/**@{*/

/**@}*/ /* TAPI_INTERFACE_SIGNAL */

/* ===================================================================== */
/* TAPI CID Features Service, constants (Group TAPI_INTERFACE_CID)       */
/* ===================================================================== */
/** \addtogroup TAPI_INTERFACE_CID */
/**@{*/

/**
   CID TX maximum buffer size.

   \remarks
   -  ETSI  :
      call setup cmd : 2,  cli : 22, date/time : 10, name : 52,
      redir num : 22, checksum : 1 => 109 Bytes max in CID buffer
   -  NTT :
      DLE : 3, SOH : 1, Header : 1, STX : 1, ETX : 1, DATA: 119, CRC : 2
      => 128 Bytes max in CID Buffer
   - ETSI SMS (protocol 2):
      type 1, length 1, data 1-255, crc 1 => 258 byte
*/
#define IFX_TAPI_CID_TX_SIZE_MAX                (258)

/** Maximum allowed length of one CID message element (in characters). */
#define IFX_TAPI_CID_MSG_LEN_MAX                (253)

/**@}*/ /* TAPI_INTERFACE_CID */

/* ======================================================================= */
/* TAPI Power Ringing Services, constants (Group TAPI_INTERFACE_RINGING)         */
/* ======================================================================= */
/** \addtogroup TAPI_INTERFACE_RINGING */
/**@{*/

/** Maximum number of cadence bytes. */
#define IFX_TAPI_RING_CADENCE_MAX_BYTES                 (40)

/**@}*/ /* TAPI_INTERFACE_RINGING */

/** Maximum version information string length. */
#define  IFX_TAPI_VERSION_LEN                            (64)

/* ========================================================================= */
/*                      TAPI Interface Enumerations                          */
/* ========================================================================= */

/* ======================================================================== */
/* TAPI Initialization Services, enumerations (Group TAPI_INTERFACE_INIT)   */
/* ======================================================================== */
/** \addtogroup TAPI_INTERFACE_INIT */
/**@{*/

/** TAPI initialization modes; controls pre-configuration of the driver for
    different target systems. This parameter specifies for which operation
    the driver should be set up. This pre-configures the driver with typical
    settings for the given operation mode. Not all modes are supported by all
    LL drivers, and the meaning of the mode is dependent on the implementation.*/
typedef enum
{
   /** Default initialization. The meaning of the default
       might vary between platforms. */
   IFX_TAPI_INIT_MODE_DEFAULT = 0,
   /** Phone to PCM not using DSP features for signal detection. */
   IFX_TAPI_INIT_MODE_PCM_PHONE = 3
} IFX_TAPI_INIT_MODE_t;

/**@}*/ /* TAPI_INTERFACE_INIT */

/* ======================================================================== */
/* TAPI Operation Control Services, enumerations (Group TAPI_INTERFACE_OP)  */
/* ======================================================================== */
/** \addtogroup TAPI_INTERFACE_OP */
/**@{*/

/** Definitions for line feeding. */
typedef enum
{
   /** Normal feeding mode for phone off-hook. */
   IFX_TAPI_LINE_FEED_ACTIVE = 0,
   /** Normal feeding mode for phone off-hook reversed. */
   IFX_TAPI_LINE_FEED_ACTIVE_REV = 1,
   /** Power-down resistance = on-hook with hook detection. */
   IFX_TAPI_LINE_FEED_STANDBY = 2,
   /** Switches off the line, but the device is able to test the line. */
   IFX_TAPI_LINE_FEED_HIGH_IMPEDANCE = 3,
   /** Switches off the line and the device. */
   IFX_TAPI_LINE_FEED_DISABLED = 4,
   /* Obsolete */
   /* IFX_TAPI_LINE_FEED_GROUND_START = 5, */
   /** Thresholds for automatic battery switch are set via coefficient settings. */
   IFX_TAPI_LINE_FEED_NORMAL_AUTO = 6,
   /** Thresholds for automatic battery switch are set via coefficient
   settings reversed. */
   IFX_TAPI_LINE_FEED_REVERSED_AUTO = 7,
   /** Feeding mode for phone off-hook with low battery to save power. */
   IFX_TAPI_LINE_FEED_NORMAL_LOW = 8,
   /** Feeding mode for phone off-hook with low battery to save power
       and reserved polarity. */
   IFX_TAPI_LINE_FEED_REVERSED_LOW = 9,
   /** Reserved; needed for ring call-back function. */
   IFX_TAPI_LINE_FEED_RING_BURST = 10,
   /** Reserved; needed for ring call-back function. */
   IFX_TAPI_LINE_FEED_RING_PAUSE = 11,
   /** Reserved; needed for internal function.  */
   IFX_TAPI_LINE_FEED_METER = 12,
   /** Reserved; special test line mode.*/
   IFX_TAPI_LINE_FEED_ACTIVE_LOW = 13,
   /** Reserved; special test line mode. */
   IFX_TAPI_LINE_FEED_ACTIVE_BOOSTED = 14,
   /** Reserved; special line mode for GEMINAX-S MAX SLIC. */
   IFX_TAPI_LINE_FEED_ACT_TESTIN = 15,
   /** Reserved; special line mode for GEMINAX-S MAX SLIC. */
   IFX_TAPI_LINE_FEED_DISABLED_RESISTIVE_SWITCH = 16,
   /** Power-down resistance = on-hook with hook detection. */
   IFX_TAPI_LINE_FEED_PARKED_REVERSED = 17,
   /** Active feeding mode with off-hook sensing ability, normal polarity and
       5 kOhm resistors activated. */
   IFX_TAPI_LINE_FEED_ACTIVE_RES_NORMAL = 18,
   /** Active feeding mode with off-hook sensing ability, reversed polarity and
       5 kOhm resistors activated. */
   IFX_TAPI_LINE_FEED_ACTIVE_RES_REVERSED = 19,
   /** Reserved; special line mode for SLIC-LCP. */
   IFX_TAPI_LINE_FEED_ACT_TEST = 20,
   /** Network line testing operating mode. */
   IFX_TAPI_LINE_FEED_NLT = 21,
   /** Power-down resistance = on-hook with hook detection and 5 kOhm
       resistors activated. */
   IFX_TAPI_LINE_FEED_STANDBY_RES = 22,
   /** Active mode with TIP wire in high-impedance state. */
   IFX_TAPI_LINE_FEED_ACTIVE_HIT = 23,
   /** Active mode with RING wire in high-impedance state. */
   IFX_TAPI_LINE_FEED_ACTIVE_HIR = 24,
   /** Starts the FXS Phone Detection state machine. */
   IFX_TAPI_LINE_FEED_PHONE_DETECT = 25,
   /* Ground start mode 1: ring line is fed, tip line is high-impedance. */
   IFX_TAPI_LINE_FEED_GROUND_START_TIP_OPEN = 26,
   /* Ground start mode 2: ring line is fed, tip line is grounded. */
   IFX_TAPI_LINE_FEED_GROUND_START_TIP2GND = 27
} IFX_TAPI_LINE_MODE_t;

   /** Line feed mode configuration \ref IFX_TAPI_LINE_FEED_SET */
#ifdef TAPI_ONE_DEVNODE
   typedef struct
   {
      /** Device index */
      IFX_uint16_t dev;
      /** Channel index */
      IFX_uint16_t ch;
      /** Apply the line mode of this analog channel. */
      IFX_TAPI_LINE_MODE_t lineMode;
   } IFX_TAPI_LINE_FEED_t;
#else /* TAPI_ONE_DEVNODE */
   typedef IFX_TAPI_LINE_MODE_t IFX_TAPI_LINE_FEED_t;
#endif /* TAPI_ONE_DEVNODE */

/** Definitions for line types. */
typedef enum
{
   /** Wrong line-mode type for analog channel. */
   IFX_TAPI_LINE_TYPE_UNKNOWN = -1,
   /** Line-mode type FXS narrowband sampling for analog channel. */
   IFX_TAPI_LINE_TYPE_FXS_NB = 0,
   /** Line-mode type FXS wideband sampling for analog channel. */
   IFX_TAPI_LINE_TYPE_FXS_WB = 1,
   /** Line-mode type FXS automatic NB/WB switching for analog channel. */
   IFX_TAPI_LINE_TYPE_FXS_AUTO = 2
} IFX_TAPI_LINE_TYPE_t;

/* Map the old names to the NB names */
#define IFX_TAPI_LINE_TYPE_FXS  IFX_TAPI_LINE_TYPE_FXS_NB

/** Line type configuration used for \ref IFX_TAPI_LINE_TYPE_SET. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Configures the line type of this analog channel. */
   IFX_TAPI_LINE_TYPE_t    lineType;

   TAPI_COMPATIBILITY_FILL(1);
} IFX_TAPI_LINE_TYPE_CFG_t;


/** Validation types used for the \ref IFX_TAPI_LINE_HOOK_VT_t structure.

   \remarks
   The default values are as follows:

   - 80 ms  <= flash time      <= 200 ms
   - 30 ms  <= digit low time  <= 80 ms
   - 30 ms  <= digit high time <= 80 ms
   - interdigit time =     300 ms
   - off hook time   =      40 ms
   - on hook time    =     400 ms
   !!! open: only min time is validated and pre-initialized */
typedef enum
{
   /** Settings for off-hook validation;
       an expection is raised in case the time reaches the nMinTime parameter.
       The parameter nMaxTime is unused. */
   IFX_TAPI_LINE_HOOK_VT_HOOKOFF_TIME     = 0x0,
   /** Settings for on-hook validation;
       an expection is raised in case the time reaches the nMinTime parameter.
       The parameter nMaxTime is unused. */
   IFX_TAPI_LINE_HOOK_VT_HOOKON_TIME      = 0x1,
   /** Settings for hook flash validation - also known as register recall.
       If the time value lies between the times defined in the fields nMinTime
       and nMaxTime, an exception is raised. */
   IFX_TAPI_LINE_HOOK_VT_HOOKFLASH_TIME   = 0x2,
   /** Settings for pulse digit low, open loop and make validation.
       The time value must lie between the times defined in the fields nMinTime and
       nMaxTime, in order to recognize it as a pulse dialing event. */
   IFX_TAPI_LINE_HOOK_VT_DIGITLOW_TIME    = 0x4,
   /** Settings for pulse digit high, close loop and break validation.
       The time value must lie between the times defined in the fields nMinTime and
       nMaxTime, in order to recognize it as a pulse dialing event. */
   IFX_TAPI_LINE_HOOK_VT_DIGITHIGH_TIME   = 0x8,
   /** Settings for pulse digit pause; the time must reach the nMinTime parameter
       in order to recognize it as a pulse dialing event.
       The parameter nMaxTime is unused. */
   IFX_TAPI_LINE_HOOK_VT_INTERDIGIT_TIME  = 0x10,
   /** Setting for hook flash holdoff period.
       The nMinTime defines the period after a flash break during which no
       further hook event must occur in order that the flash is reported.
       The parameter nMaxTime is unused.
       \remarks Not supported for VINETIC-SVIP and VINETIC-xT16. */
   IFX_TAPI_LINE_HOOK_VT_FLASH_HOLDOFF_TIME  = 0x20
} IFX_TAPI_LINE_HOOK_VALIDATION_TYPE_t;

/** Specifies the enable/disable mode of the high level. */
typedef enum
{
   /** Disable line level (default). */
   IFX_TAPI_LINE_LEVEL_DISABLE = 0x0,
   /** Enable line level. */
   IFX_TAPI_LINE_LEVEL_ENABLE = 0x1
} IFX_TAPI_LINE_LEVEL_t;

/** Specifies the current on-hook or off-hook status of the line. */
typedef enum
{
   /** Line is in on-hook state. */
   IFX_TAPI_LINE_ONHOOK = 0,
   /** Line is in off-hook state. */
   IFX_TAPI_LINE_OFFHOOK = 1
} IFX_TAPI_LINE_HOOK_t;

/**@}*/ /* TAPI_INTERFACE_OP */

/* ======================================================================= */
/* TAPI Metering Services, enumerations (Group TAPI_INTERFACE_METER)       */
/* ======================================================================= */
/** \addtogroup TAPI_INTERFACE_METER */
/**@{*/

/** Metering modes. */
typedef enum
{
   /** Normal TTX mode */
   IFX_TAPI_METER_MODE_TTX = 0,
   /** Reverse polarity mode */
   IFX_TAPI_METER_MODE_REVPOL = 1
} IFX_TAPI_METER_MODE_t;

/**@}*/ /* TAPI_INTERFACE_METER */

/* ======================================================================= */
/* TAPI Tone Control Services, enumerations (Group TAPI_INTERFACE_TONE)    */
/* ======================================================================= */
/** \addtogroup TAPI_INTERFACE_TONE */
/**@{*/

/** Tone sources. */
typedef enum
{
   /** Tone is played out on the default source. */
   IFX_TAPI_TONE_SRC_DEFAULT = 0,
   /** Tone is played out on the local tone generator in
      the analog part of the device; default if DSP is not available. */
   IFX_TAPI_TONE_SRC_TG      = 0x8000
} IFX_TAPI_TONE_SRC_t;

/** Used for selection of one or more frequencies belonging to a tone cadence. */
typedef enum
{
   /** All frequencies are inactive.*/
   IFX_TAPI_TONE_FREQNONE = 0,
   /** Plays frequency A. */
   IFX_TAPI_TONE_FREQA = 0x1,
   /** Plays frequency B.*/
   IFX_TAPI_TONE_FREQB = 0x2,
   /** Plays frequency C. */
   IFX_TAPI_TONE_FREQC = 0x4,
   /** Plays frequency D. */
   IFX_TAPI_TONE_FREQD = 0x8,
   /** Plays all frequencies. */
   IFX_TAPI_TONE_FREQALL = 0xF
} IFX_TAPI_TONE_FREQ_t;

/** Modulation setting for a cadence step. */
typedef enum
{
   /** Modulation is off for the cadence step.*/
   IFX_TAPI_TONE_MODULATION_OFF   = 0,
   /** Modulation is on for the cadence step. */
   IFX_TAPI_TONE_MODULATION_ON    = 1
} IFX_TAPI_TONE_MODULATION_t;

/** Modulation factor settings */
typedef enum
{
   /** Modulation factor 100% */
   IFX_TAPI_TONE_MODULATION_FACTOR_100 = 0,  /* for backward compatibility */
   /** Modulation factor 10% */
   IFX_TAPI_TONE_MODULATION_FACTOR_10  = 10,
   /** Modulation factor 20% */
   IFX_TAPI_TONE_MODULATION_FACTOR_20  = 20,
   /** Modulation factor 30% */
   IFX_TAPI_TONE_MODULATION_FACTOR_30  = 30,
   /** Modulation factor 40% */
   IFX_TAPI_TONE_MODULATION_FACTOR_40  = 40,
   /** Modulation factor 50% */
   IFX_TAPI_TONE_MODULATION_FACTOR_50  = 50,
   /** Modulation factor 55% */
   IFX_TAPI_TONE_MODULATION_FACTOR_55  = 55,
   /** Modulation factor 60% */
   IFX_TAPI_TONE_MODULATION_FACTOR_60  = 60,
   /** Modulation factor 65% */
   IFX_TAPI_TONE_MODULATION_FACTOR_65  = 65,
   /** Modulation factor 70% */
   IFX_TAPI_TONE_MODULATION_FACTOR_70  = 70,
   /** Modulation factor 75% */
   IFX_TAPI_TONE_MODULATION_FACTOR_75  = 75,
   /** Modulation factor 80% */
   IFX_TAPI_TONE_MODULATION_FACTOR_80  = 80,
   /** Modulation factor 85% */
   IFX_TAPI_TONE_MODULATION_FACTOR_85  = 85,
   /** Modulation factor 90% */
   IFX_TAPI_TONE_MODULATION_FACTOR_90  = 90,
   /** Modulation factor 95% */
   IFX_TAPI_TONE_MODULATION_FACTOR_95  = 95
} IFX_TAPI_TONE_MODULATION_FACTOR_t;

/** Tone types */
typedef enum
{
   /** Simple tone */
   IFX_TAPI_TONE_TYPE_SIMPLE = 1,
   /** Composed tone */
   IFX_TAPI_TONE_TYPE_COMPOSED = 2,
#ifndef TAPI_DXY_DOC
   /** Dual tone */
   IFX_TAPI_TONE_TYPE_DUAL = 3
#endif /* TAPI_DXY_DOC */
} IFX_TAPI_TONE_TYPE_t;

/**@}*/ /* TAPI_INTERFACE_TONE */

/* ==================================================================== */
/* TAPI Signal Detection Services, enumerations                         */
/* (Group TAPI_INTERFACE_SIGNAL)                                        */
/* ==================================================================== */
/** \addtogroup TAPI_INTERFACE_SIGNAL */
/**@{*/

/** Lists the tone detection options. Some applications may not be interested
    in whether the signal came from the receive or transmit path. Therefore,
    a mask exists for each signal, including the receive and transmit paths.
*/
typedef enum
{
   /** No signal detected. */
   IFX_TAPI_SIG_NONE        = 0x0,
   /** V.21 preamble fax tone, digital identification signal (DIS),
      receive path. */
   IFX_TAPI_SIG_DISRX       = 0x1,
   /** V.21 preamble fax tone, digital identification signal (DIS),
      transmit path. */
   IFX_TAPI_SIG_DISTX       = 0x2,
   /** V.21 preamble fax tone in all paths, digital identification
       signal (DIS).  */
   IFX_TAPI_SIG_DIS         = 0x4,
   /** V.25 2100 Hz (CED) modem/fax tone, receive path. */
   IFX_TAPI_SIG_CEDRX       = 0x8,
   /** V.25 2100 Hz (CED) modem/fax tone, transmit path. */
   IFX_TAPI_SIG_CEDTX       = 0x10,
   /** V.25 2100 Hz (CED) modem/fax tone in all paths. */
   IFX_TAPI_SIG_CED         = 0x20,
   /** CNG fax calling tone (1100 Hz) receive path. */
   IFX_TAPI_SIG_CNGFAXRX    = 0x40,
   /** CNG fax calling tone (1100 Hz) transmit path. */
   IFX_TAPI_SIG_CNGFAXTX    = 0x80,
   /** CNG fax calling tone (1100 Hz) in all paths. */
   IFX_TAPI_SIG_CNGFAX      = 0x100,
   /** CNG modem calling tone (1300 Hz) receive path. */
   IFX_TAPI_SIG_CNGMODRX    = 0x200,
   /** CNG modem calling tone (1300 Hz) transmit path.  */
   IFX_TAPI_SIG_CNGMODTX    = 0x400,
   /** CNG modem calling tone (1300 Hz) in all paths. */
   IFX_TAPI_SIG_CNGMOD      = 0x800,
   /** Phase reversal detection receive path.
       \remarks Not supported phase reversal uses the same
       paths as CED detection. The detector for CED must also be configured. */
   IFX_TAPI_SIG_PHASEREVRX  = 0x1000,
   /** Phase reversal detection transmit path.
      \remarks Not supported phase reversal uses the same
      paths as CED detection. The detector for CED must also be configured.  */
   IFX_TAPI_SIG_PHASEREVTX  = 0x2000,
   /** Phase reversal detection in all paths.
       \remarks Phase reversals are detected at the
       end of an CED. If this signal is enabled, CED end detection
       is also automatically enabled and reported if it occurs. */
   IFX_TAPI_SIG_PHASEREV    = 0x4000,
   /** Amplitude modulation receive path.
      \remarks Non-supported amplitude modulation uses the same
      paths as CED detection. The detector for CED must also be configured.  */
   IFX_TAPI_SIG_AMRX         = 0x8000,
   /** Amplitude modulation transmit path.
   \remarks Non-supported amplitude modulation uses the same
   paths as CED detection. The detector for CED must also be configured.  */
   IFX_TAPI_SIG_AMTX                 = 0x10000,
   /** Amplitude modulation.

    \remarks Amplitude modulation is detected at the
    end of an CED. If this signal is enabled, CED end detection
    is also automatically enabled and reported if it occurs.

    \note In case of AM detected, the driver automatically switches
    to modem coefficients after end of CED. */
   IFX_TAPI_SIG_AM                   = 0x20000,
   /** Modem tone holding signal stopped receive path.*/
   IFX_TAPI_SIG_TONEHOLDING_ENDRX    = 0x40000,
   /** Modem tone holding signal stopped transmit path. */
   IFX_TAPI_SIG_TONEHOLDING_ENDTX    = 0x80000,
   /** Modem tone holding signal stopped all paths. */
   IFX_TAPI_SIG_TONEHOLDING_END      = 0x100000,
   /** End of signal CED detection receive path.

      \remarks Not supported; CED end detection uses the same
      paths as CED detection. The detector for CED must be configured
      as well.  */
   IFX_TAPI_SIG_CEDENDRX          = 0x200000,
   /** End of signal CED detection transmit path.

      \remarks Not supported; CED end detection uses the same
      paths as CED detection. The detector for CED must be configured
      as well.  */
   IFX_TAPI_SIG_CEDENDTX          = 0x400000,
   /** End of signal CED detection; this signal also includes
      information about phase reversals and amplitude modulation,
      if enabled. */
   IFX_TAPI_SIG_CEDEND            = 0x800000,
   /** Signals V8bis detection on the receive path. */
   IFX_TAPI_SIG_V8BISRX           = 0x2000000,
   /** Signals V8bis detection on the transmit path. */
   IFX_TAPI_SIG_V8BISTX           = 0x4000000,
   /** Enables DTMF reception on locally connected analog line. */
   IFX_TAPI_SIG_DTMFTX            = 0x10000000,
   /** Enables DTMF reception on remote connected line. */
   IFX_TAPI_SIG_DTMFRX            = 0x20000000
} IFX_TAPI_SIG_t;

/** This service offers extended tone-detection options. */
typedef enum
{
   /** No signal detected. */
   IFX_TAPI_SIG_EXT_NONE          = 0x0,
   /** 980 Hz single tone (V.21L mark sequence) receive path. */
   IFX_TAPI_SIG_EXT_V21LRX        = 0x1,
   /** 980 Hz single tone (V.21L mark sequence) transmit path. */
   IFX_TAPI_SIG_EXT_V21LTX        = 0x2,
   /** 980 Hz single tone (V.21L mark sequence) all paths. */
   IFX_TAPI_SIG_EXT_V21L          = 0x4,
   /** 1400 Hz single tone (V.18A mark sequence) receive path. */
   IFX_TAPI_SIG_EXT_V18ARX        = 0x8,
   /** 1400 Hz single tone (V.18A mark sequence) transmit path. */
   IFX_TAPI_SIG_EXT_V18ATX        = 0x10,
   /** 1400 Hz single tone (V.18A mark sequence) all paths. */
   IFX_TAPI_SIG_EXT_V18A          = 0x20,
   /** 1800 Hz single tone (V.27, V.32 carrier) receive path. */
   IFX_TAPI_SIG_EXT_V27RX         = 0x40,
   /** 1800 Hz single tone (V.27, V.32 carrier) transmit path. */
   IFX_TAPI_SIG_EXT_V27TX         = 0x80,
   /** 1800 Hz single tone (V.27, V.32 carrier) all paths. */
   IFX_TAPI_SIG_EXT_V27           = 0x100,
   /** 2225 Hz single tone (Bell answering tone) receive path. */
   IFX_TAPI_SIG_EXT_BELLRX        = 0x200,
   /** 2225 Hz single tone (Bell answering tone) transmit path. */
   IFX_TAPI_SIG_EXT_BELLTX        = 0x400,
   /** 2225 Hz single tone (Bell answering tone) all paths. */
   IFX_TAPI_SIG_EXT_BELL          = 0x800,
   /** 2250 Hz single tone (V.22 unscrambled binary ones) receive path. */
   IFX_TAPI_SIG_EXT_V22RX         = 0x1000,
   /** 2250 Hz single tone (V.22 unscrambled binary ones) transmit path. */
   IFX_TAPI_SIG_EXT_V22TX         = 0x2000,
   /** 2250 Hz single tone (V.22 unscrambled binary ones) all paths. */
   IFX_TAPI_SIG_EXT_V22           = 0x4000,
   /** Reserved, obsolete. */
   IFX_TAPI_SIG_EXT_V22ORBELLRX   = 0x8000,
   /** Reserved, obsolete. */
   IFX_TAPI_SIG_EXT_V22ORBELLTX   = 0x10000,
   /** Reserved, obsolete. */
   IFX_TAPI_SIG_EXT_V22ORBELL     = 0x20000,
   /** 600 Hz + 300 Hz dual tone (V.32 AC) receive path. */
   IFX_TAPI_SIG_EXT_V32ACRX       = 0x40000,
   /** 600 Hz + 300 Hz dual tone (V.32 AC) transmit path. */
   IFX_TAPI_SIG_EXT_V32ACTX       = 0x80000,
   /** 600 Hz + 300 Hz dual tone (V.32 AC) all paths. */
   IFX_TAPI_SIG_EXT_V32AC         = 0x100000,
   /** 2130 + 2750 Hz dual tone (Bell caller ID type 2 alert tone) receive path. */
   IFX_TAPI_SIG_EXT_CASBELLRX     = 0x200000,
   /** 2130 + 2750 Hz dual tone (Bell caller ID type 2 alert tone) transmit path. */
   IFX_TAPI_SIG_EXT_CASBELLTX     = 0x400000,
   /** 2130 + 2750 Hz dual tone (Bell caller ID type 2 alert tone) all paths. */
   IFX_TAPI_SIG_EXT_CASBELL       = 0x600000,
   /** 1650 Hz single tone (V.21H mark sequence) receive path. */
   IFX_TAPI_SIG_EXT_V21HRX        = 0x800000,
   /** 1650 Hz single tone (V.21H mark sequence) transmit path. */
   IFX_TAPI_SIG_EXT_V21HTX        = 0x1000000,
   /** 1650 Hz single tone (V.21H mark sequence) all paths. */
   IFX_TAPI_SIG_EXT_V21H          = 0x1800000,
   /** Voice modem discriminator all paths. */
   IFX_TAPI_SIG_EXT_VMD           = 0x2000000
} IFX_TAPI_SIG_EXT_t;


/**@}*/ /* TAPI_INTERFACE_SIGNAL */

/* ======================================================================= */
/* TAPI CID Features Service, enumerations (Group TAPI_INTERFACE_CID)        */
/* ======================================================================= */
/** \addtogroup TAPI_INTERFACE_CID */
/**@{*/

/** List of ETSI alerts. */
typedef enum
{
   /** First ring burst
     \note This is defined only for CID transmission associated with ringing.*/
   IFX_TAPI_CID_ALERT_ETSI_FR       = 0x0,
   /** DTAS */
   IFX_TAPI_CID_ALERT_ETSI_DTAS     = 0x1,
   /** Ring pulse */
   IFX_TAPI_CID_ALERT_ETSI_RP       = 0x2,
   /** Line reversal (alias polarity reversal), followed by DTAS. */
   IFX_TAPI_CID_ALERT_ETSI_LRDTAS   = 0x3
} IFX_TAPI_CID_ALERT_ETSI_t;

/** List of CID standards.*/
typedef enum
{
   /** Bellcore/Telcordia GR-30-CORE; use Bell202 FSK coding of CID
    information.*/
   IFX_TAPI_CID_STD_TELCORDIA    = 0x0,
   /** ETSI 300-659-1/2/3 V1.3.1; use V.23 FSK coding to transmit CID
    information.*/
   IFX_TAPI_CID_STD_ETSI_FSK     = 0x1,
   /** ETSI 300-659-1/2/3 V1.3.1; use DTMF transmission of CID information.*/
   IFX_TAPI_CID_STD_ETSI_DTMF    = 0x2,
   /** SIN 227 Issue 3.4; use V.23 FSK coding of CID information.*/
   IFX_TAPI_CID_STD_SIN          = 0x3,
   /** NTT standard: TELEPHONE SERVICE INTERFACES, edition 5; use a modified
    V.23 FSK coding of CID information.*/
   IFX_TAPI_CID_STD_NTT          = 0x4,
   /** KPN; use DTMF transmission of CID information.*/
   IFX_TAPI_CID_STD_KPN_DTMF     = 0x5,
   /** KPN; use DTMF and FSK transmission of CID information.*/
   IFX_TAPI_CID_STD_KPN_DTMF_FSK = 0x6 ,
   /** Tele-Denmark TS 900 301-1; use DTMF transmission of CID information.*/
   IFX_TAPI_CID_STD_TDC          = 0x7 /* ,
*/
} IFX_TAPI_CID_STD_t;

/** Caller ID transmission modes.
 \remarks Information required especially for FSK framing.*/
typedef enum
{
   /** On-hook transmission; applicable to CID type 1 and MWI.*/
   IFX_TAPI_CID_HM_ONHOOK   = 0x00,
   /** Off-hook transmission; applicable to CID type 2 and MWI.*/
   IFX_TAPI_CID_HM_OFFHOOK  = 0x01
} IFX_TAPI_CID_HOOK_MODE_t;

/** Caller ID message type defined in ETSI EN 300 659-3.*/
typedef enum
{
   /** Call setup; corresponds to caller ID type 1 and type 2. */
   IFX_TAPI_CID_MT_CSUP  = 0x80,
   /** Message waiting indicator */
   IFX_TAPI_CID_MT_MWI   = 0x82,
   /** Advice of charge */
   IFX_TAPI_CID_MT_AOC   = 0x86,
   /** Short message service */
   IFX_TAPI_CID_MT_SMS   = 0x89,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES01 = 0xF1,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES02 = 0xF2,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES03 = 0xF3,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES04 = 0xF4,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES05 = 0xF5,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES06 = 0xF6,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES07 = 0xF7,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES08 = 0xF8,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES09 = 0xF9,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES0A = 0xFA,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES0B = 0xFB,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES0C = 0xFC,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES0D = 0xFD,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES0E = 0xFE,
   /** Reserved for network operator use.*/
   IFX_TAPI_CID_MT_RES0F = 0xFF
} IFX_TAPI_CID_MSG_TYPE_t;

/** Caller ID services (defined in ETSI EN 300 659-3).*/
typedef enum
{
   /** Date and time presentation */
   IFX_TAPI_CID_ST_DATE        = 0x01,
   /** Calling line identity (mandatory) */
   IFX_TAPI_CID_ST_CLI         = 0x02,
   /** Called line identity */
   IFX_TAPI_CID_ST_CDLI        = 0x03,
   /** Reason for absence of CLI */
   IFX_TAPI_CID_ST_ABSCLI      = 0x04,
   /** Calling line name */
   IFX_TAPI_CID_ST_NAME        = 0x07,
   /** Reason for absence of name */
   IFX_TAPI_CID_ST_ABSNAME     = 0x08,
   /** Visual indicator */
   IFX_TAPI_CID_ST_VISINDIC    = 0x0B,
   /** Message identification */
   IFX_TAPI_CID_ST_MSGIDENT    = 0x0D,
   /** Last message CLI */
   IFX_TAPI_CID_ST_LMSGCLI     = 0x0E,
   /** Complementary date and time */
   IFX_TAPI_CID_ST_CDATE       = 0x0F,
   /** Complementary calling line identity */
   IFX_TAPI_CID_ST_CCLI        = 0x10,
   /** Call type */
   IFX_TAPI_CID_ST_CT          = 0x11,
   /** First called line identity */
   IFX_TAPI_CID_ST_FIRSTCLI    = 0x12,
   /** Number of messages */
   IFX_TAPI_CID_ST_MSGNR       = 0x13,
   /** Type of forwarded call */
   IFX_TAPI_CID_ST_FWCT        = 0x15,
   /** Type of calling user */
   IFX_TAPI_CID_ST_USRT        = 0x16,
   /** Number re-direction */
   IFX_TAPI_CID_ST_REDIR       = 0x1A,
   /** Charge */
   IFX_TAPI_CID_ST_CHARGE      = 0x20,
   /** Additional charge */
   IFX_TAPI_CID_ST_ACHARGE     = 0x21,
   /** Duration of the call */
   IFX_TAPI_CID_ST_DURATION    = 0x23,
   /** Network provider ID */
   IFX_TAPI_CID_ST_NTID        = 0x30,
   /** Carrier identity */
   IFX_TAPI_CID_ST_CARID       = 0x31,
   /** Selection of terminal function */
   IFX_TAPI_CID_ST_TERMSEL     = 0x40,
   /** Display information, used as INFO for DTMF */
   IFX_TAPI_CID_ST_DISP        = 0x50,
   /** Service information */
   IFX_TAPI_CID_ST_SINFO       = 0x55,
   /** Extension for operator use */
   IFX_TAPI_CID_ST_XOPUSE      = 0xE0,
   /** Transparent mode */
   IFX_TAPI_CID_ST_TRANSPARENT = 0xFF
} IFX_TAPI_CID_SERVICE_TYPE_t;

/** List of VMWI settings.*/
typedef enum
{
   /** Disable VMWI on CPE.*/
   IFX_TAPI_CID_VMWI_DIS = 0x00,
   /** Enable VMWI on CPE.*/
   IFX_TAPI_CID_VMWI_EN  = 0xFF
} IFX_TAPI_CID_VMWI_t;

/** List of ABSCLI/ABSNAME settings.*/
typedef enum
{
   /** Unavailable/unknown */
   IFX_TAPI_CID_ABSREASON_UNAV = 0x4F,
   /** Private */
   IFX_TAPI_CID_ABSREASON_PRIV = 0x50
} IFX_TAPI_CID_ABSREASON_t;

/** List of NTT reasons for ABSCLI element.*/
typedef enum
{
   /** Private */
   IFX_TAPI_CID_NTT_ABSREASON_PRIV     = 'P',
   /** Unable to provide service */
   IFX_TAPI_CID_NTT_ABSREASON_UNAV     = 'O',
   /** Public telephone originated */
   IFX_TAPI_CID_NTT_ABSREASON_PUBPHONE = 'C',
   /** Service conflict */
   IFX_TAPI_CID_NTT_ABSREASON_CONFLICT = 'S'
} IFX_TAPI_CID_ABSREASON_NTT_t;

/**@}*/ /* TAPI_INTERFACE_CID */

/* ========================================================================= */
/* TAPI Connection Services, enumerations (Group TAPI_INTERFACE_CON)         */
/* ========================================================================= */
/** \addtogroup TAPI_INTERFACE_CON */
/**@{*/

/** Definition of codec algorithms. The enumerated elements should be used
    to select the encoding algorithm and as an array index for the RTP payload
    type configuration.*/
typedef enum
{
   /** Reserved */
   IFX_TAPI_COD_TYPE_UNKNOWN  = 0,
   /** G711 u-law, 64 kbit/s */
   IFX_TAPI_COD_TYPE_MLAW     = 8,
   /** G711 A-Law, 64 kbit/s */
   IFX_TAPI_COD_TYPE_ALAW     = 9
} IFX_TAPI_COD_TYPE_t;

/** Type channel for mapping. */
typedef enum
{
   /** Default; depends on the device and the best applicable is configured. */
   IFX_TAPI_MAP_TYPE_DEFAULT           = 0,
   /** Type is a PCM channel. */
   IFX_TAPI_MAP_TYPE_PCM               = 2,
   /** Type is a phone channel. */
   IFX_TAPI_MAP_TYPE_PHONE             = 3
} IFX_TAPI_MAP_TYPE_t;


/**@}*/ /* TAPI_INTERFACE_CON */

/* ======================================================================== */
/* TAPI Miscellaneous Services, enumerations (Group TAPI_INTERFACE_MISC)    */
/* ======================================================================== */
/** \addtogroup TAPI_INTERFACE_MISC */
/**@{*/

/** Debug trace levels. */
typedef enum
{
   /** Report off */
   IFX_TAPI_DEBUG_REPORT_SET_OFF    = 0,
   /** Low-level report; minor problems. It is recommended that you solve these
      problems. System failure or system crash is not expected. */
   IFX_TAPI_DEBUG_REPORT_SET_LOW    = 1,
   /** Normal level report; problem should be solved. Possible system failure
       or system crash. */
   IFX_TAPI_DEBUG_REPORT_SET_NORMAL = 2,
   /** High-level report; critical problems that should be solved during the
       software integration phase. System failure or system crash expected. */
   IFX_TAPI_DEBUG_REPORT_SET_HIGH   = 3
} IFX_TAPI_DEBUG_REPORT_SET_t;

/** Enumeration used for phone capability types. */
typedef enum
{
   /** Capability type: representation of the vendor. */
   IFX_TAPI_CAP_TYPE_VENDOR      = 0,
   /** Capability type: representation of the underlying device. */
   IFX_TAPI_CAP_TYPE_DEVICE      = 1,
   /** Capability type: information about available ports. */
   IFX_TAPI_CAP_TYPE_PORT        = 2,
   /** Capability type: vocoder type. */
   IFX_TAPI_CAP_TYPE_CODEC       = 3,
   /** Capability type: number of PCM modules.*/
   IFX_TAPI_CAP_TYPE_PCM         = 5,
   /** Capability type: number of coder modules. */
   IFX_TAPI_CAP_TYPE_CODECS      = 6,
   /** Capability type: number of analog interfaces. */
   IFX_TAPI_CAP_TYPE_PHONES      = 7,
   /** Device version; the version is returned in one integer, where the upper
       byte defines the major and the lower byte the minor version.*/
   IFX_TAPI_CAP_TYPE_DEVVERS     = 10,
   /** Capability type: device type. */
   IFX_TAPI_CAP_TYPE_DEVTYPE     = 11
} IFX_TAPI_CAP_TYPE_t;

/** Lists the ports for the capability list. */
typedef enum
{
   /** POTS port available. */
   IFX_TAPI_CAP_PORT_POTS    = 0,
   /** PSTN port available. */
   IFX_TAPI_CAP_PORT_PSTN    = 1,
   /** Handset port available. */
   IFX_TAPI_CAP_PORT_HANDSET = 2,
   /** Speaker port available. */
   IFX_TAPI_CAP_PORT_SPEAKER = 3
} IFX_TAPI_CAP_PORT_t;

/** Lists the possible DC/DC converters. */
typedef enum
{
   /** Inverting buck-boost converter is attached. */
   IFX_TAPI_CAP_DCDC_IBB     = 0,
   /** Combined inverting buck-boost converter is attached. */
   IFX_TAPI_CAP_DCDC_CIBB    = 1
} IFX_TAPI_CAP_DCDC_t;

/** Lists the signal detectors for the capability list. */
typedef enum
{
   /** Signal detection for CNG is available. */
   IFX_TAPI_CAP_SIG_DETECT_CNG   = 0,
   /** Signal detection for CED is available. */
   IFX_TAPI_CAP_SIG_DETECT_CED   = 1,
   /** Signal detection for DIS is available. */
   IFX_TAPI_CAP_SIG_DETECT_DIS   = 2,
   /** Signal detection for line power is available. */
   IFX_TAPI_CAP_SIG_DETECT_POWER = 3,
   /** Signal detection for V8.bis is available. */
   IFX_TAPI_CAP_SIG_DETECT_V8BIS = 5
} IFX_TAPI_CAP_SIG_DETECT_t;

/** Defines the device types. */
typedef enum
{
   /** Device not available */
   IFX_TAPI_DEV_TYPE_NONE = 0,
   /** Device of the XWAY DUSLIC XS family */
   IFX_TAPI_DEV_TYPE_DUSLIC_XS = 14
} IFX_TAPI_DEV_TYPE_t;

/** Firmware module type definition. */
typedef enum
{
   /** Reserved */
   IFX_TAPI_MODULE_TYPE_NONE = 0x0,
   /** ALM module */
   IFX_TAPI_MODULE_TYPE_ALM = 0x1,
   /** PCM module */
   IFX_TAPI_MODULE_TYPE_PCM = 0x2,
   /** Applies to all available module types. */
   IFX_TAPI_MODULE_TYPE_ALL = 0x6
} IFX_TAPI_MODULE_TYPE_t;

/**@}*/ /* TAPI_INTERFACE_MISC */

/* ==================================================================== */
/* TAPI PCM Services, enumerations (Group TAPI_INTERFACE_PCM)           */
/* ==================================================================== */
/** \addtogroup TAPI_INTERFACE_PCM */
/**@{*/

/** Defines the coding for the PCM channel. */
typedef enum
{
   /** G.711 A-law, 8 bits, narrowband.
       This resolution requires 1 PCM time slot. */
   IFX_TAPI_PCM_RES_NB_ALAW_8BIT    = 0,
   /** G.711 u-law, 8 bits, narrowband.
       This resolution requires 1 PCM time slot. */
   IFX_TAPI_PCM_RES_NB_ULAW_8BIT    = 1,
   /** Linear 16 bits, narrowband.
       This resolution requires 2 consecutive PCM time slots. */
   IFX_TAPI_PCM_RES_NB_LINEAR_16BIT = 2,
   /** G.711 A-law, 8 bits, wideband.
       This resolution requires 2 consecutive PCM time slots. */
   IFX_TAPI_PCM_RES_WB_ALAW_8BIT    = 3,
   /** G.711 u-law, 8 bits, wideband.
       This resolution requires 2 consecutive PCM time slots. */
   IFX_TAPI_PCM_RES_WB_ULAW_8BIT    = 4,
   /** Linear 16 bits, wideband.
       This resolution requires 4 consecutive PCM time slots. */
   IFX_TAPI_PCM_RES_WB_LINEAR_16BIT = 5,
   /** Linear 16 bits, wideband, split timeslots.
       This resolution requires 4 PCM time slots. These 4 PCM time slots are
       split in 2 groups of 2 time slots each. The first group starts at the
       specified time slots for TX and RX. The second group starts 1/16kHz
       later. */
   IFX_TAPI_PCM_RES_WB_LINEAR_SPLIT_16BIT = 11
} IFX_TAPI_PCM_RES_t;

/**@}*/ /* TAPI_INTERFACE_PCM */


/* ======================================================================== */
/* TAPI Test Services, enumerations (Group TAPI_INTERFACE_TEST)             */
/* ======================================================================== */
/** \addtogroup TAPI_INTERFACE_TEST */
/**@{*/

/** Specifies the hook event to generate. */
typedef enum
{
   /** Generate an on-hook event. */
   IFX_TAPI_HOOKGEN_ONHOOK = 0,
   /** Generate an off-hook event. */
   IFX_TAPI_HOOKGEN_OFFHOOK = 1
} IFX_TAPI_HOOKGEN_t;

/**@}*/ /* TAPI_INTERFACE_TEST */

/* ==================================================================== */
/* TAPI GR909 Services, enumerations (Group TAPI_INTERFACE_GR909)       */
/* ==================================================================== */
/** \addtogroup TAPI_INTERFACE_GR909 */
/**@{*/

#ifndef TAPI4_DXY_DOC
/** GR909 device type */
typedef enum
{
  /** Product using the voice engine, such as XWAY ARX188, XWAY GRX188. */
  IFX_TAPI_GR909_DEV_VMMC     = 0,
  /** Product of the XWAY VINETIC-CPE, XWAY VINETIC-ATA family. */
  IFX_TAPI_GR909_DEV_VINCPE   = 1,
  /** Product of the XWAY DUSLIC-xT family. */
  IFX_TAPI_GR909_DEV_DXT      = 2,
  /** Product of the XWAY DUSLIC-xS family. */
  IFX_TAPI_GR909_DEV_DXS      = 3,
  /** Product of the XWAY S220 family. */
  IFX_TAPI_GR909_DEV_S220     = 4
} IFX_TAPI_GR909_DEV_t;
#endif /* #ifndef TAPI4_DXY_DOC */

/** GR909 power-line frequency selection. */
typedef enum
{
   /** EU / Europe-like countries */
   IFX_TAPI_GR909_EU_50HZ = 0,
   /** US-like countries */
   IFX_TAPI_GR909_US_60HZ = 1
} IFX_TAPI_GR909_POWERLINE_FREQ_t;

/** GR909 test modes */
typedef enum
{
   /** Hazardous potential test */
   IFX_TAPI_GR909_HPT  = 0x0001,
   /** Foreign electromotive forces test */
   IFX_TAPI_GR909_FEMF = 0x0002,
   /** Resistive faults test */
   IFX_TAPI_GR909_RFT  = 0x0004,
   /** Receiver off-hook test */
   IFX_TAPI_GR909_ROH  = 0x0008,
   /** Ringer impedance test */
   IFX_TAPI_GR909_RIT  = 0x0010
} IFX_TAPI_GR909_TEST_t;

/** GR909 result validity flags */
typedef enum
{
   /** Hazardous potential test */
   IFX_TAPI_GR909_HPT_VALID  = IFX_TAPI_GR909_HPT,
   /** Foreign electromotive forces test */
   IFX_TAPI_GR909_FEMF_VALID = IFX_TAPI_GR909_FEMF,
   /** Resistive faults test */
   IFX_TAPI_GR909_RFT_VALID  = IFX_TAPI_GR909_RFT,
   /** Receiver off-hook test */
   IFX_TAPI_GR909_ROH_VALID  = IFX_TAPI_GR909_ROH,
   /** Ringer impedance test */
   IFX_TAPI_GR909_RIT_VALID  = IFX_TAPI_GR909_RIT,
   /** Hazardous potential test with extended measurement range. */
   IFX_TAPI_GR909_HPT_EXT_VALID  = 0x0100,
   /** Foreign electromotive forces test with extended measurement range.*/
   IFX_TAPI_GR909_FEMF_EXT_VALID = 0x0200,
   /** Resistive faults test with extended measurement range. */
   IFX_TAPI_GR909_RFT_EXT_VALID  = 0x0400,
   /** Receiver off-hook test with extended measurement range. */
   IFX_TAPI_GR909_ROH_EXT_VALID  = 0x0800,
   /** Ringer impedance test with extended measurement range. */
   IFX_TAPI_GR909_RIT_EXT_VALID  = 0x1000
} IFX_TAPI_GR909_VALID_t;

/**@}*/ /* TAPI_INTERFACE_GR909 */

/* ========================================================================= */
/*                      TAPI Interface Structures                            */
/* ========================================================================= */

/* ======================================================================== */
/* TAPI Initialization Services, structures (Group TAPI_INTERFACE_INIT)     */
/* ======================================================================== */
/** \addtogroup TAPI_INTERFACE_INIT */
/**@{*/

/** TAPI initialization structure used for \ref IFX_TAPI_CH_INIT. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Channel initialization mode, to be selected from \ref IFX_TAPI_INIT_MODE_t */
   IFX_uint8_t nMode;
   /** Reserved; country selection for future purposes. */
   IFX_uint8_t nCountry;
   /** Pointer to the low-level device initialization structure (for example
       VMMC_IO_INIT for XWAY INCA-IP2). For more details, refer to the device-specific
       driver documentation. */
   IFX_void_t *pProc;
} IFX_TAPI_CH_INIT_t;

/** Structure used to pass optional parameters with \ref IFX_TAPI_DEV_START. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** Reserved; initialization mode from \ref IFX_TAPI_INIT_MODE_t.
   This should always be set to \ref IFX_TAPI_INIT_MODE_DEFAULT. */
   IFX_uint8_t nMode;
} IFX_TAPI_DEV_START_CFG_t;

/**@}*/ /* TAPI_INTERFACE_INIT */

/* ======================================================================== */
/* TAPI Operation Control Services, structures (Group TAPI_INTERFACE_OP)    */
/* ======================================================================== */
/** \addtogroup TAPI_INTERFACE_OP */
/**@{*/

/** Structure used to configure volume settings.*/
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Gain setting for the receive path.
       The value is given in dB within the range (-24 dB ... 12 dB), in 1 dB steps.*/
   IFX_int32_t nGainRx;
   /** Gain setting for the transmit path.
       The value is given in dB within the range (-24 dB ... 12 dB), in 1 dB steps. */
   IFX_int32_t nGainTx;
} IFX_TAPI_LINE_VOLUME_t;


/** Structure used for validation times of hook, hook flash and pulse dialing.
    An example of typical timing:
       - 80 ms <= flash time <= 200 ms
       - 30 ms <= digit low time <= 80 ms
       - 30 ms <= digit high time <= 80 ms
       - Interdigit time = 300 ms
       - Off-hook time = 40 ms
       - On-hook time = 400 ms!!! open: only min. time is validated and pre-initialized.
*/
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Type of validation time setting. */
   IFX_TAPI_LINE_HOOK_VALIDATION_TYPE_t nType;
   /** Minimum time for validation in ms. */
   IFX_uint32_t         nMinTime;
   /** maximum time for validation in ms. */
   IFX_uint32_t         nMaxTime;
} IFX_TAPI_LINE_HOOK_VT_t;


#ifdef TAPI_ONE_DEVNODE
   /** Structure used by \ref IFX_TAPI_LINE_HOOK_STATUS_GET. */
   typedef struct
   {
      /** Device index */
      IFX_uint16_t dev;
      /** Channel index */
      IFX_uint16_t ch;
      /** This service reads the current hook state. */
      IFX_TAPI_LINE_HOOK_t hookMode;
   } IFX_TAPI_LINE_HOOK_STATUS_GET_t;
#else /* TAPI_ONE_DEVNODE */
   /** type used by \ref IFX_TAPI_LINE_HOOK_STATUS_GET. */
   typedef IFX_boolean_t IFX_TAPI_LINE_HOOK_STATUS_GET_t;
#endif /* TAPI_ONE_DEVNODE */

#ifdef TAPI_ONE_DEVNODE
   /** Structure used to enable or disable a high-level path of a phone channel.
       The high-level path might be required to play howler tones. The structure is
       used by \ref IFX_TAPI_LINE_LEVEL_SET.
*/
   typedef struct
   {
      /** Device index */
      IFX_uint16_t dev;
      /** Channel index */
      IFX_uint16_t ch;
      /** Specifies the line high-level path. */
      IFX_TAPI_LINE_LEVEL_t level;
   }IFX_TAPI_LINE_LEVEL_CFG_t;
#else /* TAPI_ONE_DEVNODE */
   /** Type used to enable or disable a high-level path of a phone channel.
       The high-level path might be required to play howler tones. The structure is
       used by \ref IFX_TAPI_LINE_LEVEL_SET.
*/
   typedef IFX_TAPI_LINE_LEVEL_t IFX_TAPI_LINE_LEVEL_CFG_t;
#endif /* TAPI_ONE_DEVNODE */

/**@}*/ /* TAPI_INTERFACE_OP */

/* ===================================================================== */
/* TAPI Metering Services, structures (Group TAPI_INTERFACE_METER)       */
/* ===================================================================== */
/** \addtogroup TAPI_INTERFACE_METER */
/**@{*/

/** Structure for configuration of metering
    used by \ref IFX_TAPI_METER_CFG_SET.*/
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Metering mode:
   - 0: TAPI_METER_MODE_TTX, TTX mode
   - 1: IFX_TAPI_METER_MODE_REVPOL, reverse polarity */
   IFX_TAPI_METER_MODE_t    bMode;
   /** Length of metering pulse (in ms).
       'nPulseLen' must be greater than zero.*/
   IFX_uint32_t             nPulseLen;
   /** Length of pause between two metering pulses (in ms).
       'nPauseLen' must be greater than zero.*/
   IFX_uint32_t             nPauseLen;
} IFX_TAPI_METER_CFG_t;

/** Structure used by \ref IFX_TAPI_METER_START. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Distance between the metering bursts (in seconds). */
   IFX_uint32_t nPulseDist;
   /** Defines the number of pulses. */
   IFX_uint32_t nPulses;
} IFX_TAPI_METER_START_t;

#ifdef TAPI_ONE_DEVNODE
/** Structure used by \ref IFX_TAPI_METER_STOP. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
} IFX_TAPI_METER_STOP_t;
#endif /* TAPI_ONE_DEVNODE */

/** Structure used by \ref IFX_TAPI_METER_BURST. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Defines the number of pulses in a burst. */
   IFX_uint32_t nPulses;
} IFX_TAPI_METER_BURST_t;

/** Structure used by \ref IFX_TAPI_METER_STATISTICS_GET. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Number of pulse bursts that are requested by the periodic timer or
       application intermediate burst and which are not generated yet.
       These will be generated by the firmware. */
   IFX_uint32_t nPulseOutstanding;
   /** Number of bursts that are transmitted since the last statistic call. */
   IFX_uint32_t nBurstTransmitted;
} IFX_TAPI_METER_STATISTICS_t;

/**@}*/ /* TAPI_INTERFACE_METER */

/* ==================================================================== */
/* TAPI Tone Control Services, structures (Group TAPI_INTERFACE_TONE)   */
/* ==================================================================== */
/** \addtogroup TAPI_INTERFACE_TONE */
/**@{*/

/** Min tone code ID */
#define IFX_TAPI_TONE_SIMPLE_INDEX_MIN 0

/** Max tone code ID */
#define IFX_TAPI_TONE_SIMPLE_INDEX_MAX 255

/** Min number of times to play the simple tone */
#define IFX_TAPI_TONE_LOOP_MIN 0

/** Max number of times to play the simple tone */
#define IFX_TAPI_TONE_LOOP_MAX 8

/** Min power level for frequency in 0.1 dB steps */
#define IFX_TAPI_TONE_POWER_LEVEL_MIN (-300)

/** Max power level for frequency in 0.1 dB steps */
#define IFX_TAPI_TONE_POWER_LEVEL_MAX 0

/** Max tone frequency in Hz */
#define IFX_TAPI_TONE_FREQ_MAX 4000

/** Structure used to define simple tone characteristics. */
typedef struct
{
    /** Indicates the type of the tone descriptor:

    - 1: IFX_TAPI_TONE_TYPE_SIMPLE, the tone descriptor describes a simple tone
    - 2: IFX_TAPI_TONE_TYPE_COMPOSED, the tone descriptor describes a
         composed tone */
    IFX_TAPI_TONE_TYPE_t    format;
    /** Name of the simple tone. */
    IFX_char_t    name[30];
    /** Tone code ID; \ref IFX_TAPI_TONE_SIMPLE_INDEX_MIN <= ID <= \ref IFX_TAPI_TONE_SIMPLE_INDEX_MAX. */
    IFX_uint32_t  index;
    /** Number of times to play the simple tone, \ref IFX_TAPI_TONE_LOOP_MIN < loop < \ref IFX_TAPI_TONE_LOOP_MAX.
        The loop count, if not equal to 0, defines the time of the entire sequence:
        tone_seq = loop * (onA + offA + onB + offB + onC + offC + pause). */
    IFX_uint32_t  loop;
    /** Power level for frequency A in 0.1 dB steps;
        \ref IFX_TAPI_TONE_POWER_LEVEL_MIN <= levelA <= \ref IFX_TAPI_TONE_POWER_LEVEL_MAX. */
    IFX_int32_t   levelA;
    /** Power level for frequency B in 0.1 dB steps;
        \ref IFX_TAPI_TONE_POWER_LEVEL_MIN <= levelB <= \ref IFX_TAPI_TONE_POWER_LEVEL_MAX. */
    IFX_int32_t   levelB;
    /** Power level for frequency C in 0.1 dB steps;
        \ref IFX_TAPI_TONE_POWER_LEVEL_MIN <= levelC <= \ref IFX_TAPI_TONE_POWER_LEVEL_MAX. */
    IFX_int32_t   levelC;
    /** Power level for frequency D in 0.1 dB steps;
        \ref IFX_TAPI_TONE_POWER_LEVEL_MIN <= levelD <= \ref IFX_TAPI_TONE_POWER_LEVEL_MAX. */
    IFX_int32_t   levelD;
    /** Tone frequency A in Hz; 0 <= Hz <= \ref IFX_TAPI_TONE_FREQ_MAX. */
    IFX_uint32_t  freqA;
    /** Tone frequency B in Hz; 0 <= Hz <= \ref IFX_TAPI_TONE_FREQ_MAX. */
    IFX_uint32_t  freqB;
    /** Tone frequency C in Hz; 0 <= Hz <= \ref IFX_TAPI_TONE_FREQ_MAX. */
    IFX_uint32_t  freqC;
    /** Tone frequency D in Hz; 0 <= Hz <= \ref IFX_TAPI_TONE_FREQ_MAX. */
    IFX_uint32_t  freqD;
    /**
      IFX_TAPI_TONE_STEPS_MAX array defining time duration
      for each cadence step, with 1 ms granularity.
      0 <= cadence <= 16383.
      The first cadence[X] = 0 (starting from X = 1) in the array indicates that
      X-1 cadences must be played.
      A tone with cadence[0]=0 cannot be processed! */
    IFX_uint32_t  cadence[IFX_TAPI_TONE_STEPS_MAX];
    /** Active frequencies for the cadence steps; more than one frequency
      can be active in the same cadence step. All active frequencies are
      summed together.

      - 0x0:  IFX_TAPI_TONE_FREQNONE, no frequencies
      - 0x01: IFX_TAPI_TONE_FREQA, frequency A is enabled
      - 0x02: IFX_TAPI_TONE_FREQB, frequency B is enabled
      - 0x04: IFX_TAPI_TONE_FREQC, frequency C is enabled
      - 0x08: IFX_TAPI_TONE_FREQD, frequency D is enabled
      - 0x0F: IFX_TAPI_TONE_FREQALL, all frequencies are enabled */
    IFX_uint32_t  frequencies[IFX_TAPI_TONE_STEPS_MAX];

    /** Array specifying, for each cadence step, whether to
      enable/disable the modulation of frequency A with frequency B.
      Refer to \ref IFX_TAPI_TONE_MODULATION_t enum values.
    */
    IFX_uint32_t  modulation[IFX_TAPI_TONE_STEPS_MAX];

    /** Modulation factor to be applied to cadence steps which activate
        modulation. Refer to \ref IFX_TAPI_TONE_MODULATION_FACTOR_t
        enum values.
    */
    IFX_uint32_t  modulation_factor;
    /** Some tones require an off-time at the end of the tone. The off-time
      is added to the last used cadence. Therefore, the off-time has a maximum value of
      32000 - 'last used cadence' and a granularity of 2 ms;
      0 < 32000 - 'last used cadence' < 32000.*/
    IFX_uint32_t  pause;
} IFX_TAPI_TONE_SIMPLE_t;

/** Structure used for definition of composed tones. */
typedef struct
{
    /** Indicate the type of the tone descriptor:

    - 1: IFX_TAPI_TONE_TYPE_SIMPLE, the tone descriptor describes a simple tone
    - 2: IFX_TAPI_TONE_TYPE_COMPOSED, the tone descriptor describes a composed
        tone */
    IFX_TAPI_TONE_TYPE_t    format;
    /** Name of the composed tone. */
    IFX_char_t    name[30];
    /** Tone code ID; 0< ID <255.*/
    IFX_uint32_t  index;
    /** Number of times to play the tone sequence: 0 for infinite,
        maximum 7. */
    IFX_uint32_t  loop;
    /** Indicate whether the voice path is active between the loops.*/
    IFX_uint32_t  alternatVoicePath;
    /** Number of simple tones used in the composed tone. */
    IFX_uint32_t  count;
    /** Tone table indexes of the simple tones to be used; the simple tones
        are played in the same order in which they are stored in the array.
        \note In order to create composed tones, only simple tones with
        a finite loop count can be used.*/
    IFX_uint32_t  tones [IFX_TAPI_TONE_SIMPLE_MAX];
} IFX_TAPI_TONE_COMPOSED_t;

#ifndef TAPI_DXY_DOC
/** Structure for definition of dual tones. */
typedef struct
{
   /** Device index */
   IFX_uint16_t dev;
   /** Channel index */
   IFX_uint16_t ch;
   IFX_TAPI_TONE_TYPE_t format;
   IFX_int32_t levelA;
   IFX_int32_t levelB;
   IFX_uint32_t freqA;
   IFX_uint32_t freqB;
} IFX_TAPI_TONE_DUAL_t;
#endif /* TAPI_DXY_DOC */

/** Tone descriptor. */
typedef union
{
   /** Pointer to an \ref IFX_TAPI_TONE_SIMPLE_t structure.*/
   IFX_TAPI_TONE_SIMPLE_t   simple;
   /**  Pointer to an \ref IFX_TAPI_TONE_COMPOSED_t structure.*/
   IFX_TAPI_TONE_COMPOSED_t composed;
#ifndef TAPI_DXY_DOC
   /** Descriptor for dual tone. */
   IFX_TAPI_TONE_DUAL_t     dual;
#endif /* TAPI_DXY_DOC */
} IFX_TAPI_TONE_t;

#ifndef TAPI4_DXY_DOC
#ifdef TAPI_ONE_DEVNODE
   /** Structure used by \ref IFX_TAPI_TONE_BUSY_PLAY,
       \ref IFX_TAPI_TONE_RINGBACK_PLAY and  \ref IFX_TAPI_TONE_DIALTONE_PLAY. */
   struct IFX_TAPI_PREDEFINED_TONE_s
   {
      /** Device index */
      IFX_uint16_t dev;
      /** Channel index */
      IFX_uint16_t ch;
   };

   /** Structure used by \ref IFX_TAPI_TONE_BUSY_PLAY. */
   typedef struct IFX_TAPI_PREDEFINED_TONE_s IFX_TAPI_TONE_BUSY_t;

   /** Structure used by \ref IFX_TAPI_TONE_RINGBACK_PLAY. */
   typedef struct IFX_TAPI_PREDEFINED_TONE_s IFX_TAPI_TONE_RINGBACK_t;

   /** Structure used by \ref IFX_TAPI_TONE_DIALTONE_PLAY. */
   typedef struct IFX_TAPI_PREDEFINED_TONE_s IFX_TAPI_TONE_DIALTONE_t;
#endif /* TAPI_ONE_DEVNODE */

#ifdef TAPI_ONE_DEVNODE
   /** Structure used by \ref IFX_TAPI_TONE_LOCAL_PLAY. */
   typedef struct
   {
      /** Device index */
      IFX_uint16_t dev;
      /** Channel index */
      IFX_uint16_t ch;
      /** The parameter is the index of the tone to the pre-defined tone
         table (range 1 - 31) or custom tones added previously
         (index 32 - 255). Index 0 means tone stop. Using the upper bits
         modifies the default tone playing source. */
      IFX_int32_t nToneIndex;
   }IFX_TAPI_TONE_IDX_t;
#else /* TAPI_ONE_DEVNODE */
   /** The parameter is the index of the tone to the pre-defined tone
      table (range 1 - 31) or custom tones added previously
      (index 32 - 255). Index 0 means tone stop. Using the upper bits
      modifies the default tone playing source. */
   typedef IFX_int32_t IFX_TAPI_TONE_IDX_t;
#endif /* TAPI_ONE_DEVNODE */
#endif /* ifndef TAPI4_DXY_DOC */

/**@}*/ /* TAPI_INTERFACE_TONE */


/* ======================================================================== */
/* TAPI Signal Detection Services, structures                               */
/* (Group TAPI_INTERFACE_SIGNAL)                                            */
/* ======================================================================== */
/** \addtogroup TAPI_INTERFACE_SIGNAL */
/**@{*/


/** DTMF receiver coefficients settings.
    The following DTMF receiver coefficients are to be directly programmed
    in the underlying device. Therefore, the passed values must be expressed
    in a format ready for programming, as no interpretation of these values is
    attempted.
*/
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
#ifdef TAPI_VERSION4
   /** Firmware module to play the configuration. */
   IFX_TAPI_MODULE_TYPE_t module;
#endif /* TAPI_VERSION4 */
   /** Minimal signal level in dB. */
   IFX_int32_t nLevel;
   /** Maximum allowed signal twist in dB. */
   IFX_int32_t nTwist;
   /** Gain adjustment of the input signal in dB. */
   IFX_int32_t nGain;
   /** Delay inserted in the voice path (in ms). It is possible to insert
       a delay of up to 20 ms; the default is 0 ms. A delay in the voice path
       facilitates a better/total suppression of the DTMF tone
       (if the DTMF auto-suppression feature is used).
       \note This option should be used with caution since the overall
       group delay will increase by the same quantity. */
   IFX_uint8_t nVoicePathDelay;
} IFX_TAPI_DTMF_RX_CFG_t;


/** This structure is used by the IFX_TAPI_TONE_PLAY and IFX_TAPI_TONE_STOP
    commands to enable/disable tone playout.*/
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Tone index to enable. To disable, the index is set to zero. */
   IFX_uint32_t index;
   /** Firmware module on which to play the tone */
   IFX_TAPI_MODULE_TYPE_t module;
   /** Tone generation towards external
       (e.g. analog line, PCM bus, RTP in-band, etc.). */
   IFX_uint16_t external:1;
   /** Tone generation towards internal (connected firmware module). */
   IFX_uint16_t internal:1;
   /** Set to IFX_TRUE to mute all other voice from connected module or conference. */
   IFX_uint16_t bMuteOtherVoice:1;
   /** Reserved */
   IFX_uint16_t reserved:13;
} IFX_TAPI_TONE_PLAY_t;

/**@}*/ /* TAPI_INTERFACE_SIGNAL */

/* ==================================================================== */
/* TAPI CID Features Service, structures (Group TAPI_INTERFACE_CID)       */
/* ==================================================================== */
/** \addtogroup TAPI_INTERFACE_CID */
/**@{*/

/** Structure containing the timing for CID transmission. */
typedef struct
{
   /** Time to wait before data transmission, in ms;
      default is 300 ms. */
   IFX_uint32_t beforeData;
   /** Time to wait after data transmission, in ms, for on-hook services;
      default is 300 ms. */
   IFX_uint32_t dataOut2restoreTimeOnhook;
   /** Time to wait after data transmission, in ms, for off-hook services;
      default is 60 ms. */
   IFX_uint32_t dataOut2restoreTimeOffhook;
   /** Time to wait after ACK detection, in ms;
      default is 55 ms. */
   IFX_uint32_t ack2dataOutTime;
   /** Time-out for ACK detection, in ms;
      default is 160 ms. */
   IFX_uint32_t cas2ackTime;
   /** Time to wait after ACK time-out, in ms;
      default is 0 ms. */
   IFX_uint32_t afterAckTimeout;
   /** Time to wait after the first ring, in ms, typically before data transmission;
      default is 600 ms. */
   IFX_uint32_t afterFirstRing;
   /** Time to wait after ring pulse, in ms, typically before data transmission;
      default is 500 ms. */
   IFX_uint32_t afterRingPulse;
   /** Time to wait after DTAS, in ms, typically before data transmission;
      default is 45 ms. */
   IFX_uint32_t afterDTASOnhook;
   /** Time to wait after line reversal in ms, typically before data transmission;
      default is 100 ms. */
   IFX_uint32_t afterLineReversal;
   /** Time to wait after OSI signal, in ms;
      default is 300 ms. */
   IFX_uint32_t afterOSI;
} IFX_TAPI_CID_TIMING_t;

/* Min signal level for FSK transmission in 0.1 dB steps (-96.5 dB) */
#define IFX_TAPI_CID_FSK_LEVEL_MIN (-965)

/* Max signal level for FSK transmission in 0.1 dB steps (0.5 dB) */
#define IFX_TAPI_CID_FSK_LEVEL_MAX 5

/** Structure containing the configuration information for the FSK transmitter and
   receiver. */
typedef struct
{
   /** Signal level for FSK transmission in 0.1 dB steps; default is -140
    (-14 dB).*/
   IFX_int32_t    levelTX;
   /** Minimum signal level for FSK reception in 0.1 dB steps; default is -150
    (-15 dB).*/
   IFX_int32_t    levelRX;
   /** Number of seizure bits for FSK transmission; relevant only
   for on-hook transmission. Default values are:
   - NTT standard: 0 bits
   - Other standards: 300 bits*/
   IFX_uint32_t   seizureTX;
   /** Minimum number of seizure bits for FSK reception; relevant
   only for on-hook transmission. Default values are:
   - NTT standard: 0 bits
   - Other standards: 200 bits*/
   IFX_uint32_t   seizureRX;
   /** Number of mark bits for on-hook FSK transmission; default values are:
   - NTT standard: 72 bits
   - Other standards: 180 bits */
   IFX_uint32_t   markTXOnhook;
   /** Number of mark bits for off-hook FSK transmission; default values are:
   - NTT standard: 72 bits
   - Other standards: 80 bits */
   IFX_uint32_t   markTXOffhook;
   /** Minimum number of mark bits for on-hook FSK reception; default values are:
   - NTT standard: 50 bits
   - Other standards: 150 bits */
   IFX_uint32_t   markRXOnhook;
   /** Minimum number of mark bits for off-hook FSK reception; default values are:
   - NTT standard: 50 bits
   - Other standards: 55 bits */
   IFX_uint32_t   markRXOffhook;
   /** Number of additional stop (mark) bits for on-hook FSK transmission.
   The ETSI standard states that one to ten stop bits shall be sent after the
   checksum to avoid corruption of the checksum.
   A value of 0 in this field defaults to 1 additional stop bit being sent. */
   IFX_uint32_t   stopTXOnhook;
   /** Number of additional stop (mark) bits for off-hook FSK transmission.
   The ETSI standard states that one to ten stop bits shall be sent after the
   checksum to avoid corruption of the checksum.
   A value of 0 in this field defaults to 1 additional stop bit being sent. */
   IFX_uint32_t   stopTXOffhook;
} IFX_TAPI_CID_FSK_CFG_t;

/** Structure containing the configuration information for the DTMF CID. */
typedef struct
{
   /** Tone ID for starting tone; default is DTMF A. */
   IFX_int8_t     startTone;
   /** Tone ID for stop tone; default is DTMF C. */
   IFX_int8_t     stopTone;
   /** Tone ID for starting information tone; default is DTMF B. */
   IFX_int8_t     infoStartTone;
   /** Tone ID for starting redirection tone; default is DTMF D. */
   IFX_int8_t     redirStartTone;
   /** Time for DTMF digit duration; default is 50 ms. */
   IFX_uint32_t   digitTime;
   /** Time between DTMF digits in ms; default is 50 ms. */
   IFX_uint32_t   interDigitTime;
} IFX_TAPI_CID_DTMF_CFG_t;

/** Structure containing CID configuration for the ETSI standard using DTMF
 transmission.*/
typedef struct
{
   /** Length of the coded strings.*/
   IFX_uint32_t   len;
   /** String representing code for unavailable/unknown CLI; default 00.*/
   IFX_uint8_t    unavailable[IFX_TAPI_CID_MSG_LEN_MAX];
   /** String representing code for private/withheld CLI; default 01.*/
   IFX_uint8_t    priv[IFX_TAPI_CID_MSG_LEN_MAX];
} IFX_TAPI_CID_ABS_REASON_t;

/** Structure containing CID configuration for the Telcordia standard. */
typedef struct
{
   /** Pointer to a structure containing timing information. If the parameter
    is not given, \ref IFX_TAPI_CID_TIMING_t default values will be used. */
   IFX_TAPI_CID_TIMING_t   *pCIDTiming;
   /** Pointer to a structure containing FSK configuration parameters. If the
    parameter is not given, \ref IFX_TAPI_CID_FSK_CFG_t default values will be used.*/
   IFX_TAPI_CID_FSK_CFG_t  *pFSKConf;
   /** Use of OSI for off-hook transmission; default IFX_FALSE.*/
   IFX_uint32_t            OSIoffhook;
   /** Length of the OSI signal in ms; default 200 ms.*/
   IFX_uint32_t            OSItime;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t            nAlertToneOnhook;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t            nAlertToneOffhook;
   /** DTMF ACK after CAS, used for off-hook transmission; default DTMF 'D'.
       This acknowledge tone can be set to the DTMF tones 'A', 'B', 'C' or 'D'.
       Setting this parameter to zero means that any one of the four tones is
       accepted. */
   IFX_char_t              ackTone;
   /** Tone table index for the subscriber alerting signal (SAS) to be used
       in off-hook transmissions. By default, the TAPI plays no SAS tone */
   IFX_uint32_t            nSAStone;
   /** Time to wait before a SAS tone, in ms, for off-hook services.
       The default is 20 ms when OSI is not used, and 100 ms when OSI is used. */
   IFX_uint32_t            beforeSAStime;
   /** Time to wait between generation of SAS and CAS tone, in ms;
       default 20 ms. */
   IFX_uint32_t            SAS2CAStime;
} IFX_TAPI_CID_STD_TELCORDIA_t;

/** Structure containing CID configuration for the ETSI standard using FSK
    transmission. */
typedef struct
{
   /** Pointer to a structure containing timing information. If the parameter
    is not given, \ref IFX_TAPI_CID_TIMING_t default values will be used.*/
   IFX_TAPI_CID_TIMING_t         *pCIDTiming;
   /** Pointer to a structure containing FSK configuration parameters. If the
    parameter is not given, \ref IFX_TAPI_CID_FSK_CFG_t default values will be used.*/
   IFX_TAPI_CID_FSK_CFG_t        *pFSKConf;
   /** Type of ETSI alert of on-hook services associated with ringing
    (enumerated in IFX_TAPI_CID_ALERT_ETSI_t); default
    IFX_TAPI_CID_ALERT_ETSI_FR.*/
   IFX_TAPI_CID_ALERT_ETSI_t     nETSIAlertRing;
   /** Type of ETSI alert of on-hook services not associated with ringing
    (enumerated in IFX_TAPI_CID_ALERT_ETSI_t); default
     IFX_TAPI_CID_ALERT_ETSI_RP.*/
   IFX_TAPI_CID_ALERT_ETSI_t     nETSIAlertNoRing;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t                  nAlertToneOnhook;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t                  nAlertToneOffhook;
   /** Duration of ring pulse, in ms; default 500 ms.*/
   IFX_uint32_t                  ringPulseTime;
   /** DTMF ACK after CAS, used for off-hook transmission; default DTMF is D.
       This acknowledge tone can be set to the DTMF tones 'A', 'B', 'C' or 'D'.
       Setting this parameter to zero means that any one of the four tones is
       accepted. */
   IFX_char_t                    ackTone;
} IFX_TAPI_CID_STD_ETSI_FSK_t;

/** Structure containing CID configuration for the ETSI standard using DTMF
 transmission. */
typedef struct
{
   /** Pointer to a structure containing timing information. If the parameter
    is not given, \ref IFX_TAPI_CID_TIMING_t default values will be used. */
   IFX_TAPI_CID_TIMING_t       *pCIDTiming;
   /** Pointer to a structure containing DTMF configuration parameters. If the
    parameter is not given, \ref IFX_TAPI_CID_DTMF_CFG_t default values will be
   used. */
   IFX_TAPI_CID_DTMF_CFG_t     *pDTMFConf;
   /** Pointer to a structure containing the coding for the
   absence reason of the calling number.*/
   IFX_TAPI_CID_ABS_REASON_t   *pABSCLICode;
   /** Type of ETSI alert of on-hook services associated with ringing (enumerated
    in \ref IFX_TAPI_CID_ALERT_ETSI_t); default \ref IFX_TAPI_CID_ALERT_ETSI_FR.*/
   IFX_TAPI_CID_ALERT_ETSI_t   nETSIAlertRing;
   /** Type of ETSI alert of on-hook services not associated with ringing
    (enumerated in \ref IFX_TAPI_CID_ALERT_ETSI_t); default \ref
    IFX_TAPI_CID_ALERT_ETSI_RP. */
   IFX_TAPI_CID_ALERT_ETSI_t   nETSIAlertNoRing;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t                nAlertToneOnhook;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition. */
   IFX_uint32_t                nAlertToneOffhook;
   /** Duration of ring pulse, in ms; default is 500 ms. */
   IFX_uint32_t                ringPulseTime;
   /** DTMF ACK after CAS, used for off-hook transmission; default DTMF D.
       This acknowledge tone can be set to the DTMF tones 'A', 'B', 'C' or 'D'.
       Setting this parameter to zero means that any one of the four tones is
       accepted. */
   IFX_char_t                  ackTone;
} IFX_TAPI_CID_STD_ETSI_DTMF_t;

/** Structure for the configuration of the SIN standard. */
typedef struct
{
   /** Pointer to a structure containing timing information.
      If the parameter is NULL, default values will be used. */
   IFX_TAPI_CID_TIMING_t   *pCIDTiming;
   /** Pointer to a structure containing FSK configuration parameters.
      If the parameter is NULL, default values will be used. */
   IFX_TAPI_CID_FSK_CFG_t  *pFSKConf;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t            nAlertToneOnhook;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t            nAlertToneOffhook;
   /** DTMF ACK after CAS, used for off-hook transmission; default DTMF 'D'.
       This acknowledge tone can be set to the DTMF tones 'A', 'B', 'C' or 'D'.
       Setting this parameter to zero means that any one of the four tones is
       accepted. */
   IFX_char_t              ackTone;
} IFX_TAPI_CID_STD_SIN_t;

/** Structure containing the CID configuration for the NTT standard.*/
typedef struct
{
   /** Pointer to a structure containing timing information. If the parameter
    is not given, \ref IFX_TAPI_CID_TIMING_t default values will be used.*/
   IFX_TAPI_CID_TIMING_t   *pCIDTiming;
   /** Pointer to a structure containing FSK configuration parameters. If the
    parameter is not given, \ref IFX_TAPI_CID_FSK_CFG_t default values will be used.*/
   IFX_TAPI_CID_FSK_CFG_t  *pFSKConf;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t            nAlertToneOnhook;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t            nAlertToneOffhook;
   /** Ring pulse on time (CAR signal), in ms; default 500 ms.*/
   IFX_uint32_t            ringPulseTime;
   /** Maximum number of ring pulses (CAR signals); default 5.*/
   IFX_uint32_t            ringPulseLoop;
   /** Ring pulse off-time (CAR signal), in ms; default 500 ms.*/
   IFX_uint32_t            ringPulseOffTime;
   /** Time-out for incoming successful signal to arrive after CID data
    transmission is completed; default 7000 ms.*/
   IFX_uint32_t            dataOut2incomingSuccessfulTimeout;
} IFX_TAPI_CID_STD_NTT_t;

/** Structure containing CID configuration for the KPN standard, with DTMF
    and FSK transmission */
typedef struct
{
   /** Pointer to a structure containing timing information. If the parameter
    is not given, IFX_TAPI_CID_TIMING_t default values will be used.*/
   IFX_TAPI_CID_TIMING_t         *pCIDTiming;
   /** Pointer to a structure containing DTMF configuration parameters. If the
    parameter is not given, \ref IFX_TAPI_CID_DTMF_CFG_t default values will be
   used. */
   IFX_TAPI_CID_DTMF_CFG_t       *pDTMFConf;
   /** Pointer to a structure containing the coding for the
   absence reason of the calling number.*/
   IFX_TAPI_CID_ABS_REASON_t     *pABSCLICode;
   /** Pointer to a structure containing FSK configuration parameters. If the
    parameter is not given, IFX_TAPI_CID_FSK_CFG_t default values will be used.*/
   IFX_TAPI_CID_FSK_CFG_t        *pFSKConf;
   /** Tone table index for the alert tone to be used. Required for automatic
    CID/MWI generation. By default, TAPI uses an internal tone definition.*/
   IFX_uint32_t                  nAlertToneOffhook;
   /** DTMF ACK after CAS, used for off-hook transmission; default DTMF is D.
       This acknowledge tone can be set to the DTMF tones 'A', 'B', 'C' or 'D'.
       Setting this parameter to zero means that any one of the four tones is
       accepted. */
   IFX_char_t                    ackTone;
} IFX_TAPI_CID_STD_KPN_DTMF_FSK_t;

/** Structure containing CID configuration for the KPN standard, with DTMF
    transmission */
typedef struct
{
   /** Pointer to a structure containing timing information. If the parameter
    is not given, IFX_TAPI_CID_TIMING_t default values will be used.*/
   IFX_TAPI_CID_TIMING_t         *pCIDTiming;
   /** Pointer to a structure containing DTMF configuration parameters. If the
    parameter is not given, \ref IFX_TAPI_CID_DTMF_CFG_t default values will be
   used. */
   IFX_TAPI_CID_DTMF_CFG_t       *pDTMFConf;
   /** Pointer to a structure containing the coding for the
   absence reason of the calling number.*/
   IFX_TAPI_CID_ABS_REASON_t     *pABSCLICode;
   /** Pointer to a structure containing FSK configuration parameters. If the
    parameter is not given, IFX_TAPI_CID_FSK_CFG_t default values will be used.*/
   IFX_TAPI_CID_FSK_CFG_t        *pFSKConf;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t                  nAlertToneOffhook;
   /** DTMF ACK after CAS, used for off-hook transmission; default DTMF is D.
       This acknowledge tone can be set to the DTMF tones 'A', 'B', 'C' or 'D'.
       Setting this parameter to zero means that any one of the four tones is
       accepted. */
   IFX_char_t                    ackTone;
} IFX_TAPI_CID_STD_KPN_DTMF_t;

/** Structure containing CID configuration for the TDC standard */
typedef struct
{
   /** Pointer to a structure containing timing information. If the parameter
    is not given, IFX_TAPI_CID_TIMING_t default values will be used.*/
   IFX_TAPI_CID_TIMING_t         *pCIDTiming;
   /** Pointer to a structure containing DTMF configuration parameters. If the
    parameter is not given, \ref IFX_TAPI_CID_DTMF_CFG_t default values will be
   used. */
   IFX_TAPI_CID_DTMF_CFG_t       *pDTMFConf;
   /** Pointer to a structure containing the coding for the
   absence reason of the calling number.*/
   IFX_TAPI_CID_ABS_REASON_t     *pABSCLICode;
} IFX_TAPI_CID_STD_TDC_t;

/** Union of the CID configuration structures for different standards.*/
typedef union
{
   /** Structure defining configuration parameters for the Telcordia standard. */
   IFX_TAPI_CID_STD_TELCORDIA_t     telcordia;
   /** Structure defining configuration parameters for the ETSI standard, with FSK
    transmission.*/
   IFX_TAPI_CID_STD_ETSI_FSK_t      etsiFSK;
   /** Structure defining configuration parameters for the ETSI standard, with DTMF
    transmission.*/
   IFX_TAPI_CID_STD_ETSI_DTMF_t     etsiDTMF;
   /** Structure defining configuration parameters for the BT SIN standard. */
   IFX_TAPI_CID_STD_SIN_t           sin;
   /** Structure defining configuration parameters for the NTT standard. */
   IFX_TAPI_CID_STD_NTT_t           ntt;
   /** Structure defining configuration parameters for the KPN standard, with DTMF
       transmission. */
   IFX_TAPI_CID_STD_KPN_DTMF_t      kpnDTMF;
   /** Structure defining configuration parameters for the KPN standard, with DTMF
       and FSK transmission. */
   IFX_TAPI_CID_STD_KPN_DTMF_FSK_t  kpnDTMF_FSK;
   /** Structure defining configuration parameters for the TDC standard */
   IFX_TAPI_CID_STD_TDC_t           tdc;
} IFX_TAPI_CID_STD_TYPE_t;

/** Structure containing CID configuration possibilities. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Standard used (enumerated in IFX_TAPI_CID_STD_t); default
    IFX_TAPI_CID_STD_TELCORDIA.*/
   IFX_TAPI_CID_STD_t       nStandard;
   /** Union of the different standards; default IFX_TAPI_CID_STD_TELCORDIA_t.*/
   IFX_TAPI_CID_STD_TYPE_t  *cfg;
} IFX_TAPI_CID_CFG_t;

/** Structure for element types (\ref IFX_TAPI_CID_SERVICE_TYPE_t) containing
   date and time information.*/
typedef struct
{
   /** Element type. */
   IFX_TAPI_CID_SERVICE_TYPE_t   elementType;
   /** Month. */
   IFX_uint32_t                  month;
   /** Day. */
   IFX_uint32_t                  day;
   /** Hour. */
   IFX_uint32_t                  hour;
   /** Minute. */
   IFX_uint32_t                  mn;
} IFX_TAPI_CID_MSG_DATE_t;

/** Structure for element types (\ref IFX_TAPI_CID_SERVICE_TYPE_t) with
 dynamic length (line numbers or names).*/
typedef struct
{
   /** Element type. */
   IFX_TAPI_CID_SERVICE_TYPE_t   elementType;
   /** Length of the message array. */
   IFX_uint32_t                  len;
   /** String containing the message element. */
   IFX_uint8_t                   element[IFX_TAPI_CID_MSG_LEN_MAX];
} IFX_TAPI_CID_MSG_STRING_t;

/** Structure for element types (\ref IFX_TAPI_CID_SERVICE_TYPE_t) with one
 value (length 1).
*/
typedef struct
{
   /** Element type. */
   IFX_TAPI_CID_SERVICE_TYPE_t   elementType;
   /** Value for the message element. */
   IFX_uint8_t                   element;
} IFX_TAPI_CID_MSG_VALUE_t;

/** Structure for service type transparent (\ref IFX_TAPI_CID_SERVICE_TYPE_t).*/
typedef struct
{
   /** Element type. */
   IFX_TAPI_CID_SERVICE_TYPE_t   elementType;
   /** Element length. */
   IFX_uint32_t                  len;
   /** Element buffer. */
   IFX_uint8_t                   *data;
} IFX_TAPI_CID_MSG_TRANSPARENT_t;

/** Union of element types. */
typedef union
{
   /** Message element including date and time information. */
   IFX_TAPI_CID_MSG_DATE_t          date;
   /** Message element formatted as a string.*/
   IFX_TAPI_CID_MSG_STRING_t        string;
   /** Message element formatted as a value.*/
   IFX_TAPI_CID_MSG_VALUE_t         value;
   /** Message element to be sent with transparent transmission. */
   IFX_TAPI_CID_MSG_TRANSPARENT_t   transparent;
} IFX_TAPI_CID_MSG_ELEMENT_t;

/** Structure containing the CID message type and content as well as
   information about transmission mode. This structure contains all information
   required by IFX_TAPI_CID_TX_INFO_START to start CID generation.*/
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Defines the transmission mode (enumerated in \ref IFX_TAPI_CID_HOOK_MODE_t).
    The default is \ref IFX_TAPI_CID_HM_ONHOOK.*/
   IFX_TAPI_CID_HOOK_MODE_t      txMode;
   /** Defines the message type to be displayed (enumerated in \ref
   IFX_TAPI_CID_MSG_TYPE_t).*/
   IFX_TAPI_CID_MSG_TYPE_t       messageType;
   /** Number of elements of the message array. */
   IFX_uint32_t                  nMsgElements;
   /** Message array. */
   IFX_TAPI_CID_MSG_ELEMENT_t    *message;
} IFX_TAPI_CID_MSG_t;


#ifdef TAPI_ONE_DEVNODE
/** Structure used by \ref IFX_TAPI_CID_TX_INFO_STOP. */
typedef struct
{
   /** Device index */
   IFX_uint16_t dev;
   /** Channel index */
   IFX_uint16_t ch;
}IFX_TAPI_CID_TX_INFO_STOP_t;
#endif /* TAPI_ONE_DEVNODE */

/**@}*/ /* TAPI_INTERFACE_CID */


/* ======================================================================== */
/* TAPI Miscellaneous Services, structures (Group TAPI_INTERFACE_MISC)      */
/* ======================================================================== */
/** \addtogroup TAPI_INTERFACE_MISC */
/**@{*/

/** Capability structure. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** Descriptive text of this capability. */
   IFX_char_t desc[80];
   /** Defines the capability type, see \ref IFX_TAPI_CAP_TYPE_t.*/
   IFX_TAPI_CAP_TYPE_t captype;
   /** Defines if, what or how many are available. The definition of cap
       depends on the type; see captype. */
   IFX_int32_t cap;
   /** The number of this capability. */
   IFX_int32_t handle;
} IFX_TAPI_CAP_t;

/** Capability structure. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** Count of allocated \ref IFX_TAPI_CAP_t elements. On return it contains
       the number of elements that were copied. */
   IFX_uint32_t nCap;
   /** Allocated memory for capability list;
       amount of memory should be equal to (sizeof (IFX_TAPI_CAP_t) * nCap). */
   IFX_TAPI_CAP_t *pList;
} IFX_TAPI_CAP_LIST_t;

/** Structure used for the TAPI version support check. */
typedef struct
{
   /** Major version number supported. */
   IFX_uint8_t majorNumber;
   /** Minor version number supported. */
   IFX_uint8_t minorNumber;
} IFX_TAPI_VERSION_t;


/** Defines the maximum number of stack entries. */
#define IFX_TAPI_MAX_ERROR_ENTRIES 5
/** Defines the maximum length of a filename in each error-stack entry. */
#define IFX_TAPI_MAX_FILENAME 20
/** Defines the space for additional data in each error-stack entry. */
#define IFX_TAPI_MAX_ERRMSG 16

/** Contains one line of an error source, including the error code, the
source code line and the file name (maximum 32 characters). */
typedef struct
{
   /** High-level error code, which is set at the detection of the error in the
   high-level TAPI driver part.
   The code may change in the flow of the error handling in the upper call stack. */
   IFX_uint16_t nHlCode;
   /** Low-level error code, which is set at the detection of the error in the
   low-level driver part. This code is device-driver specific.
   The code may change in the flow of the error handling in the upper call stack. */
   IFX_uint16_t nLlCode;
   /** Source code line number. */
   IFX_uint32_t nLine;
   /** Source code file name. */
   IFX_char_t   sFile[IFX_TAPI_MAX_FILENAME];
   /** Any additional information depending on the error, such as the last message
       sent to the device, state machine status, etc. */
   IFX_uint32_t msg[IFX_TAPI_MAX_ERRMSG];
} IFX_TAPI_ErrorLine_t;

/** Error information with the source of the error. It contains a maximum of
    IFX_TAPI_MAX_ERROR_ENTRIES stack entries.
    The item with index 0 is the first error detected. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t nDev;
   /** The channel that causes this error, if any. */
   IFX_uint16_t  nCh;
   /** Error code, which is set at the highest level where the
   error was detected.*/
   IFX_uint32_t         nCode;
   /** Error stack information. */
   IFX_TAPI_ErrorLine_t stack[IFX_TAPI_MAX_ERROR_ENTRIES];
   /** Number of stack entries. */
   IFX_uint8_t          nCnt;
} IFX_TAPI_Error_t;

   /** Structure used by \ref IFX_TAPI_CAP_NR. */
#ifdef TAPI_ONE_DEVNODE
   typedef struct
   {
      /** Device index */
      IFX_uint16_t dev;
      /** This service returns the number of capabilities. */
      IFX_int32_t nCap;
   }IFX_TAPI_CAP_NR_t;
#else /* TAPI_ONE_DEVNODE */
   typedef IFX_int32_t IFX_TAPI_CAP_NR_t;
#endif /* TAPI_ONE_DEVNODE */

#ifdef TAPI_ONE_DEVNODE
   /** Structure used by \ref IFX_TAPI_DEBUG_REPORT_SET. */
   typedef struct
   {
      /** Device index */
      IFX_uint16_t dev;
      /** Specifies the debug report level. */
      IFX_TAPI_DEBUG_REPORT_SET_t level;
   }IFX_TAPI_DEBUG_REPORT_t;
#else /* TAPI_ONE_DEVNODE */
   /** Specifies the debug report level;
      type used by \ref IFX_TAPI_DEBUG_REPORT_SET. */
   typedef IFX_TAPI_DEBUG_REPORT_SET_t IFX_TAPI_DEBUG_REPORT_t;
#endif /* TAPI_ONE_DEVNODE */

/**@}*/ /* TAPI_INTERFACE_MISC */

/* ===================================================================== */
/* TAPI Power Ringing Services, structures (Group TAPI_INTERFACE_RINGING)      */
/* ===================================================================== */
/** \addtogroup TAPI_INTERFACE_RINGING */
/**@{*/

/** Structure for ring cadence used in \ref IFX_TAPI_RING_CADENCE_HR_SET.  */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Array of data bytes containing the encoded periodic cadence sequence.
   One bit represents a ring cadence of 50 ms. A maximum pattern of 40 bytes
   (320 bits) is allowed. */
   IFX_uint8_t     data[IFX_TAPI_RING_CADENCE_MAX_BYTES];
   /** Number of valid data bits in the periodic cadence sequence. The periodic
   cadence may have any length between 1 and 320 bits. Each valid bit corresponds
   to a 50 ms cadence duration, so that the periodic cadence can have a
   duration of between 50 ms and 16 s. */
   IFX_int32_t    nr;
   /** Array of data bytes containing the encoded initial cadence sequence.
   One bit represents a ring cadence of 50 ms. A maximum pattern of 40 bytes
   (320 bits) is allowed.  */
   IFX_uint8_t     initial [IFX_TAPI_RING_CADENCE_MAX_BYTES];
   /** Number of valid data bits in the initial cadence sequence. The initial
   cadence may have any length between 0 and 320 bits. Each valid bit corresponds
   to a 50 ms cadence duration, so that the periodic cadence can have a
   duration of between 0 and 16 s. Set this to 0 if no initial cadence
   should be used. If a length is given, the initial pattern is played once
   when ringing is started. After this, the periodic pattern is played and will
   be repeated for as long as ringing is not stopped. */
   IFX_int32_t    initialNr;
} IFX_TAPI_RING_CADENCE_t;


#ifdef TAPI_ONE_DEVNODE
/** Structure used by \ref IFX_TAPI_RING_START and \ref IFX_TAPI_RING_STOP. */
typedef struct
{
   /** Device index */
   IFX_uint16_t dev;
   /** Channel index */
   IFX_uint16_t ch;
}IFX_TAPI_RING_t;
#endif /* TAPI_ONE_DEVNODE */

#ifdef TAPI_ONE_DEVNODE
   /** Structure used by \ref IFX_TAPI_RING_MAX_SET. */
   typedef struct
   {
      /** Device index */
      IFX_uint16_t dev;
      /** Channel index */
      IFX_uint16_t ch;
      /** Maximum number of cadences. */
      IFX_uint32_t nMaxRings;
   } IFX_TAPI_RING_MAX_t;
#else /* TAPI_ONE_DEVNODE */
   /** Maximum number of cadences.
      Type used by \ref IFX_TAPI_RING_MAX_SET. */
   typedef IFX_uint32_t IFX_TAPI_RING_MAX_t;
#endif /* TAPI_ONE_DEVNODE */

/**@}*/ /* TAPI_INTERFACE_RINGING */

/* ============================================================ */
/* TAPI Calibration Services, structures                        */
/* (Group TAPI_INTERFACE_CALIBRATION)                           */
/* ============================================================ */
/** \addtogroup TAPI_INTERFACE_CALIBRATION */
/**@{*/

#ifdef TAPI_ONE_DEVNODE
/** Structure used to start the analog line calibration process.
    The structure is used by \ref IFX_TAPI_CALIBRATION_START. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
} IFX_TAPI_CALIBRATION_t;
#endif /* TAPI_ONE_DEVNODE */

/** Used by \ref IFX_TAPI_CALIBRATION_CFG_t to set structure parameters to an
    unused and undefined value. */
#define IFX_TAPI_CALIBRATION_UNUSED (0x8000)

/** Defines the current state of the calibration and its results. */
typedef enum
{
   /** No calibration performed; all returned results are invalid. */
   IFX_TAPI_CALIBRATION_STATE_NO       = 0,
   /** Calibration performed successfully. */
   IFX_TAPI_CALIBRATION_STATE_DONE     = 1,
   /** Calibration process failed; the returned calibration results are
       overwritten with default values on the analog line. */
   IFX_TAPI_CALIBRATION_STATE_FAILED   = 2
}IFX_TAPI_CALIBRATION_STATE_t;

/** Structure used to set and get the analog line calibration coefficients;
    used by \ref IFX_TAPI_CALIBRATION_CFG_SET,
    \ref IFX_TAPI_CALIBRATION_CFG_GET and
    \ref IFX_TAPI_CALIBRATION_RESULTS_GET.
    Non-supported values are set to \ref IFX_TAPI_CALIBRATION_UNUSED. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Calibration already performed. This value is used when reading back the
       results or the configuration from the device. */
   IFX_TAPI_CALIBRATION_STATE_t  nState;
   /** TX path offset.
       Range: -1.5 mA ... +1.5 mA (10 uA steps). */
   IFX_int16_t nITransOffset;
   /** Measurement equipment offset.
       Range: -10 V ... +10 V (10 mV steps). */
   IFX_int16_t nMeOffset;
   /** RX path DC offset gain 30.
       Range: -1.5 V ... +1.5 V (10m V steps). */
   IFX_int16_t nUlimOffset30;
   /** RX path DC offset gain 60.
       Range: -3.0 V ... +3.0 V (10 mV steps). */
   IFX_int16_t nUlimOffset60;
   /** IDAC gain correction.
       Range: -10% ... +10% (0.1% steps).*/
   IFX_int16_t nIdacGain;
   /** Long current offset.
       Range: -3 mA ... +3 mA (10 uA steps). */
   IFX_int16_t nILongOffset;
   /** Ring current offset.
       Range: -3 mA ... +3 mA (10 uA steps). */
   IFX_int16_t nIRingOffset;
   /** Combined DC/DC voltage correction.
       Range: -15% ... +15% (0.1% steps).*/
   IFX_int16_t nVdcdc;
} IFX_TAPI_CALIBRATION_CFG_t;

/**@}*/ /* TAPI_INTERFACE_CALIBRATION */

/* ============================================================ */
/* TAPI PCM Services, structures (Group TAPI_INTERFACE_PCM)     */
/* ============================================================ */
/** \addtogroup TAPI_INTERFACE_PCM */
/**@{*/

/** PCM data clock frequency (alias DCL or PCLK) for the PCM interface.*/
typedef enum
{
   /** 512 kHz */
   IFX_TAPI_PCM_IF_DCLFREQ_512         = 0,
   /** 1024 kHz */
   IFX_TAPI_PCM_IF_DCLFREQ_1024        = 1,
   /** 1536 kHz */
   IFX_TAPI_PCM_IF_DCLFREQ_1536        = 2,
   /** 2048 kHz */
   IFX_TAPI_PCM_IF_DCLFREQ_2048        = 3,
   /** 4096 kHz */
   IFX_TAPI_PCM_IF_DCLFREQ_4096        = 4,
   /** 8192 kHz */
   IFX_TAPI_PCM_IF_DCLFREQ_8192        = 5,
   /** 16384 kHz */
   IFX_TAPI_PCM_IF_DCLFREQ_16384       = 6
} IFX_TAPI_PCM_IF_DCLFREQ_t;

/** Drive mode for bit 0, in single clocking mode.*/
typedef enum
{
   /** Bit 0 is driven for the entire clock period.*/
   IFX_TAPI_PCM_IF_DRIVE_ENTIRE        = 0,
   /** Bit 0 is driven for the first half of the clock period.*/
   IFX_TAPI_PCM_IF_DRIVE_HALF          = 1
} IFX_TAPI_PCM_IF_DRIVE_t;

 /** PCM interface mode (master/slave).*/
typedef enum
{
   /** Reserved */
   IFX_TAPI_PCM_IF_MODE_SLAVE_AUTOFREQ = 0,
   /** Slave mode; the DCL frequency is explicitly programmed.*/
   IFX_TAPI_PCM_IF_MODE_SLAVE          = 1,
   /** Master mode; the DCL frequency is explicitly programmed.*/
   IFX_TAPI_PCM_IF_MODE_MASTER         = 2
} IFX_TAPI_PCM_IF_MODE_t;

/** PCM interface mode transmit/receive offset.*/
typedef enum
{
   /** No offset.*/
   IFX_TAPI_PCM_IF_OFFSET_NONE         = 0,
   /** Offset: one data period is added.*/
   IFX_TAPI_PCM_IF_OFFSET_1            = 1,
   /** Offset: two data periods are added.*/
   IFX_TAPI_PCM_IF_OFFSET_2            = 2,
   /** Offset: three data periods are added.*/
   IFX_TAPI_PCM_IF_OFFSET_3            = 3,
   /** Offset: four data periods are added.*/
   IFX_TAPI_PCM_IF_OFFSET_4            = 4,
   /** Offset: five data periods are added.*/
   IFX_TAPI_PCM_IF_OFFSET_5            = 5,
   /** Offset: six data periods are added.*/
   IFX_TAPI_PCM_IF_OFFSET_6            = 6,
   /** Offset: seven data periods are added.*/
   IFX_TAPI_PCM_IF_OFFSET_7            = 7
} IFX_TAPI_PCM_IF_OFFSET_t;

/** Slope for the PCM interface transmit/receive.
    It is used by \ref IFX_TAPI_PCM_IF_CFG_t.*/
typedef enum
{
   /** Rising edge.*/
   IFX_TAPI_PCM_IF_SLOPE_RISE          = 0,
   /** Falling edge.*/
   IFX_TAPI_PCM_IF_SLOPE_FALL          = 1
} IFX_TAPI_PCM_IF_SLOPE_t;

/** Mode of synchronising read and write by the firmware to PCM time slots
    with the PCM frame clock. */
typedef enum
{
   /** Default; use the setting of the driver. */
   IFX_TAPI_PCM_IF_TS_SYNC_DEFAULT     = 0,
   /** Read and write of timeslots from firmware is synchronised with the
      PCM frame clock. This is the recommended mode.
      This mode is needed for data transmission or when using codecs which
      occupy more than one timeslot in one PCM frame like the wideband
      codecs. Using this mode can increase the group delay by up to 125us
      compared to the IFX_TAPI_PCM_IF_TS_SYNC_NONE mode. */
   IFX_TAPI_PCM_IF_TS_SYNC_FRAME       = 1,
   /** Read and write of timeslots from firmware is not synchronised with
      the PCM frame.
      Use this mode for minimal group delay. This mode is only meant for
      voice transmission on the local system. */
   IFX_TAPI_PCM_IF_TS_SYNC_NONE        = 2
} IFX_TAPI_PCM_IF_TS_SYNC_t;


/** ADPCM bit-packing in PCM time slots. */
typedef enum
{
   /** Default; ADPCM bits in least-significant bits of PCM time slot. */
   IFX_TAPI_PCM_BITPACK_LSB = 0,
   /** ADPCM bits in most-significant bits of PCM time slot. */
   IFX_TAPI_PCM_BITPACK_MSB = 1
} IFX_TAPI_PCM_BITPACK_t;

/** PCM sample order swap. */
typedef enum
{
   /** Default; the older sample is transmitted first (in the time slot with the
       lower number. */
   IFX_TAPI_PCM_SAMPLE_SWAP_DISABLED = 0,
   /** The newer sample is transmitted first (in the time slot with the lower
       number). It should be used only if the attached PCM device has a
       different sample/time-slot mapping. */
   IFX_TAPI_PCM_SAMPLE_SWAP_ENABLED = 1
} IFX_TAPI_PCM_SAMPLE_SWAP_t;

/** Bit order within PCM time slots. */
typedef enum
{
   /** Default; normal bit order. */
   IFX_TAPI_PCM_BITORDER_NORMAL = 0,
   /** Reversed bit order. */
   IFX_TAPI_PCM_BITORDER_REVERSED = 1
} IFX_TAPI_PCM_BITORDER_t;

/** Structure for PCM interface configuration.

   \remarks
   Attention: not all products support all features that can be configured
   using this structure (for example, master mode or slave mode without
   automatic clock detection). Refer to the product system release note
   for more information about the supported features.
*/
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t                  dev;
   /** PCM interface index (starts counting from zero).
       Some devices support multiple PCM interfaces, known as PCM highways.
       This value is always set to zero in case the device has only one
       PCM highway. */
   IFX_uint16_t                  nHighway;
   /** PCM interface mode (master or slave mode). */
   IFX_TAPI_PCM_IF_MODE_t        nOpMode;
   /** DCL frequency to be used in the master and/or slave mode. */
   IFX_TAPI_PCM_IF_DCLFREQ_t     nDCLFreq;
   /** Activation/deactivation of the double clock mode.

   - 0: IFX_DISABLE, (Default) single clocking is used.
   - 1: IFX_ENABLE, Double clocking is used. */
   IFX_operation_t               nDoubleClk;
   /** Slope to be considered for the PCM transmit direction.*/
   IFX_TAPI_PCM_IF_SLOPE_t       nSlopeTX;
   /** Slope to be considered for the PCM receive direction. */
   IFX_TAPI_PCM_IF_SLOPE_t       nSlopeRX;
   /** Bit offset for TX time slot.*/
   IFX_TAPI_PCM_IF_OFFSET_t      nOffsetTX;
   /** Bit offset for RX time slot.*/
   IFX_TAPI_PCM_IF_OFFSET_t      nOffsetRX;
   /** Drive mode for bit 0.*/
   IFX_TAPI_PCM_IF_DRIVE_t       nDrive;
   /** Enable/disable shift access edge; shift the access edges by one clock
      cycle.

   - 0: IFX_DISABLE, No shift takes place.
   - 1: IFX_ENABLE, Shift takes place.
   \note This setting is defined only in double clock mode.*/
   IFX_operation_t               nShift;
   /** Reserved; PCM chip-specific settings.
   Set to 0x01 for VINETIC-SVIP devices to enable PCM clock tracking.
   For other devices set to 0x00, if not advised otherwise by the Lantiq support team. */
   IFX_uint8_t                   nMCTS;
   /** Mode of synchronising read and write by the firmware to PCM time slots
       with the PCM frame clock. */
   IFX_TAPI_PCM_IF_TS_SYNC_t     nTsSync;
   /** Offset for timeslots which are reserved for SLIC connections.
       On system with VCODEC timeslots on the PCM bus are used to transport
       the voice to the DSP. By default the reservation starts at timeslot 0.
       When using the PCM bus also for external devices there can be a conflict
       with timeslot usage. This parameters allows to shift the reserved
       timeslots by the given offset. Please note that after shifting still
       all timeslots have to fit into the PCM frame or the configuration
       will fail.

       This parameter is ignored if there are no reserved timeslots on the
       device. */
   IFX_uint16_t                  nOffsetSlicTs;
} IFX_TAPI_PCM_IF_CFG_t;

/** Structure for PCM channel configuration.  */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** PCM time slot for the receive direction. */
   IFX_uint32_t               nTimeslotRX;
   /** PCM time slot for the transmit direction. */
   IFX_uint32_t               nTimeslotTX;
   /** Defines the PCM highway number which is connected to the channel. */
   IFX_uint32_t               nHighway;
   /** Defines the PCM interface coding; values defined in
   \ref IFX_TAPI_PCM_RES_t. */
   IFX_uint32_t               nResolution;
   /** Enable sample to time-slot swap; to be used if the
   attached PCM device has a different sample to time-slot mapping;
   valid values are defined in \ref IFX_TAPI_PCM_SAMPLE_SWAP_t. */
   IFX_TAPI_PCM_SAMPLE_SWAP_t nSampleSwap;
   /** Configure ADPCM bit-packing in PCM time slots.
   Valid values are defined in \ref IFX_TAPI_PCM_BITPACK_t. */
   IFX_TAPI_PCM_BITPACK_t     nBitPacking;
   /** Configures the bit order within PCM time slots.
   Valid values are defined in \ref IFX_TAPI_PCM_BITORDER_t. */
   IFX_TAPI_PCM_BITORDER_t    nBitOrder;
} IFX_TAPI_PCM_CFG_t;


/** Structure used to control PCM muting used by
    \ref IFX_TAPI_PCM_MUTE_CFG_SET */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Mute the PCM channel into TX direction */
   IFX_boolean_t bMuteTx;
   /** Mute the PCM channel into RX direction */
   IFX_boolean_t bMuteRx;
} IFX_TAPI_PCM_MUTE_CFG_t;

#ifdef TAPI_ONE_DEVNODE
   /** Structure used by \ref IFX_TAPI_PCM_ACTIVATION_GET and
      \ref IFX_TAPI_PCM_ACTIVATION_SET. */
   typedef struct
   {
      /** Device index */
      IFX_uint16_t dev;
      /** Channel index */
      IFX_uint16_t ch;
      /** PCM enable or disable. */
      IFX_operation_t mode;
   }IFX_TAPI_PCM_ACTIVATION_t;
#else /* TAPI_ONE_DEVNODE */
   /** PCM enable or disable.
      Type used by \ref IFX_TAPI_PCM_ACTIVATION_GET and
      \ref IFX_TAPI_PCM_ACTIVATION_SET. */
   typedef IFX_operation_t IFX_TAPI_PCM_ACTIVATION_t;
#endif /* TAPI_ONE_DEVNODE */

/**@}*/ /* TAPI_INTERFACE_PCM */

/* ======================================================================== */
/* TAPI Test Services, structures (Group TAPI_INTERFACE_TEST)               */
/* ======================================================================== */
/** \addtogroup TAPI_INTERFACE_TEST */
/**@{*/

/** Structure used to switch loops for testing. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Switch an analog loop in the device. If switched on, signals that are
      played to the subscriber are looped back to the receiving side.

      - 0x0: Analog loop off
      - 0x1: Analog loop on */
   IFX_uint8_t bAnalog;
} IFX_TAPI_TEST_LOOP_t;

#ifdef TAPI_ONE_DEVNODE
   /** Structure used by \ref IFX_TAPI_TEST_HOOKGEN. */
   typedef struct
   {
      /** Device index */
      IFX_uint16_t dev;
      /** Channel index */
      IFX_uint16_t ch;
      /** Specifies the hook state event to generate. */
      IFX_TAPI_HOOKGEN_t hookMode;
   }IFX_TAPI_TEST_HOOKGEN_t;
#else /* TAPI_ONE_DEVNODE */
   /** Specifies the hook state event to generate.
       Type used by \ref IFX_TAPI_TEST_HOOKGEN. */
   typedef IFX_TAPI_HOOKGEN_t IFX_TAPI_TEST_HOOKGEN_t;
#endif /* TAPI_ONE_DEVNODE */

/**@}*/ /* TAPI_INTERFACE_TEST */

#ifndef TAPI_DXY_DOC
/** Return code classes for error handling. */
typedef enum
{
   /** Specifies a generic status result or error. */
   TAPI_statusClassSuccess    = 0x0000,
   /** Specifies a channel-related error in addition to the other classes. */
   TAPI_statusClassCh         = 0x1000,
   /** Specifies a warning or information that does not harm the system
       if handled correctly. The upper layers handle this result as an error and this
       may be signaled to the application. */
   TAPI_statusClassWarn       = 0x4000,
   /** Specifies a general error, which may lead to function failure of at least
       that channel or feature. */
   TAPI_statusClassErr        = 0x6000,
   /** Specifies a critical error; device or driver maybe out of function. */
   TAPI_statusClassCritical   = 0x8000
} TAPI_statusClass_t;
#endif /* TAPI_DXY_DOC */

/**
   This macro informs whether the given status code means success or failure.
   For success it returns IFX_TRUE and for fails IFX_FALSE.

   If the return value is IFX_ERROR, it returns IFX_FALSE. Otherwise the
   code is checked to see whether any of the classes Err, Warn or Critical
   are set and also returns IFX_FALSE if set, otherwise IFX_TRUE.

   At first the simple style return codes are tested and then the TAPI_status
   error codes. The expected value IFX_SUCCESS is tested first then the errors
   are checked to get results fast.

   Note that the shift of the error code can actually lead to a zero value
   depending on the datatype and value used. But the lower 16 bit will stay
   and determine the result in this case.
*/
#define TAPI_SUCCESS(code)                                                     \
/*lint -save -e{506, 572, 774, 778} */                                         \
(                                                                              \
    /* return IFX_TRUE (success) if code is IFX_SUCCESS */                     \
    (IFX_return_t)(code) == IFX_SUCCESS ? IFX_TRUE :                           \
    /* return IFX_FALSE (failure) if code is IFX_ERROR */                      \
   ((IFX_return_t)(code) == IFX_ERROR ? IFX_FALSE :                            \
   /* check if code higher or lower two bytes are from */                      \
   /* class Err or Warn or Critical */                                         \
      (((TAPI_statusClass_t)                                                   \
         ((IFX_uint32_t)(code) | ((IFX_uint32_t)(code) >> 16)) &               \
          (TAPI_statusClassErr |                                               \
           TAPI_statusClassWarn |                                              \
           TAPI_statusClassCritical)                                           \
   /* return IFX_FALSE (failure) if code matches class Err, Warn or Critical */\
   /* otherwise return IFX_TRUE (success) */                                   \
       ) ? IFX_FALSE : IFX_TRUE                                                \
      )                                                                        \
   )                                                                           \
)
/*lint -restore */


#ifndef TAPI4_DXY_DOC
/* ===================================================================== */
/* TAPI GR909 Services, structures (Group TAPI_INTERFACE_GR909)          */
/* ===================================================================== */
/** \addtogroup TAPI_INTERFACE_GR909 */
/**@{*/

/** GR909 test start */
typedef struct
{
   /** \tapiv3 Not used in TAPI for CPE Products. \endtapiv3
       \tapiv4 Device number.                \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used in TAPI for CPE Products. \endtapiv3
       \tapiv4 Channel number.               \endtapiv4 */
   IFX_uint16_t ch;
   /** GR909 powerline frequency to use.
       See \ref IFX_TAPI_GR909_POWERLINE_FREQ_t. */
   IFX_TAPI_GR909_POWERLINE_FREQ_t     pl_freq;
   /** GR909 test mask as value or combination of tests in
       \ref IFX_TAPI_GR909_TEST_t. */
   IFX_uint32_t                        test_mask;
} IFX_TAPI_GR909_START_t;

#ifdef TAPI_ONE_DEVNODE
   /** GR909 test stop. */
   typedef struct
   {
      /** Device number. */
      IFX_uint16_t dev;
      /** Channel number. */
      IFX_uint16_t ch;
   } IFX_TAPI_GR909_STOP_t;
#endif /* TAPI_ONE_DEVNODE */

/** GR909 results. */
typedef struct
{
#ifdef TAPI_ONE_DEVNODE
   /** Device number. */
   IFX_uint16_t dev;
   /** Channel number. */
   IFX_uint16_t ch;
#endif /* TAPI_ONE_DEVNODE */
   /** Device type, see \ref IFX_TAPI_GR909_DEV_t. */
   IFX_TAPI_GR909_DEV_t dev_type;
   /** Valid results flag, see \ref IFX_TAPI_GR909_VALID_t. */
   IFX_uint32_t         valid;
   /** Passed flag according to valid flag, see \ref IFX_TAPI_GR909_TEST_t. */
   IFX_uint32_t         passed;
   /** HPT AC RING wire to GND result. */
   IFX_int16_t          HPT_AC_R2G;
   /** HPT AC TIP wire to GND result. */
   IFX_int16_t          HPT_AC_T2G;
   /** HPT AC TIP wire to RING wire result. */
   IFX_int16_t          HPT_AC_T2R;
   /** HPT DC RING wire to GND result. */
   IFX_int16_t          HPT_DC_R2G;
   /** HPT DC TIP wire to GND result. */
   IFX_int16_t          HPT_DC_T2G;
   /** HPT DC TIP wire to RING wire result. */
   IFX_int16_t          HPT_DC_T2R;
   /** FEMF AC RING wire to GND result. */
   IFX_int16_t          FEMF_AC_R2G;
   /** FEMF AC TIP wire to GND result. */
   IFX_int16_t          FEMF_AC_T2G;
   /** FEMF AC TIP wire to RING wire result. */
   IFX_int16_t          FEMF_AC_T2R;
   /** FEMF DC RING wire to GND result. */
   IFX_int16_t          FEMF_DC_R2G;
   /** FEMF DC TIP wire to GND result. */
   IFX_int16_t          FEMF_DC_T2G;
   /** FEMF DC TIP wire to RING wire result. */
   IFX_int16_t          FEMF_DC_T2R;
   /** RFT RING wire to GND result. */
   IFX_int16_t          RFT_R2G;
   /** RFT TIP wire to GND result. */
   IFX_int16_t          RFT_T2G;
   /** RFT TIP wire to RING wire result. */
   IFX_int16_t          RFT_T2R;
   /** ROH TIP wire to RING wire result for low voltage. */
   IFX_int16_t          ROH_T2R_L;
   /** ROH TIP wire to RING wire result for high voltage. */
   IFX_int16_t          ROH_T2R_H;
   /** RIT result. */
   IFX_int16_t          RIT_RES;
   /** Stored open loop resistance tip to ring [Ohm].
       Float value stored in unsigned int bytes. */
   IFX_uint32_t         OLR_T2R;
   /** Stored open loop resistance tip to ground [Ohm].
       Float value stored in unsigned int bytes. */
   IFX_uint32_t         OLR_T2G;
   /** Stored open loop resistance ring to ground [Ohm].
       Float value stored in unsigned int bytes. */
   IFX_uint32_t         OLR_R2G;
} IFX_TAPI_GR909_RESULT_t;

/**@}*/ /* TAPI_INTERFACE_GR909 */
#endif /* #ifndef TAPI4_DXY_DOC */

#ifndef TAPI4_DXY_DOC
/* ========================================================================== */
/* TAPI FXS Phone Detection Services, structures                              */
/* (Group TAPI_INTERFACE_PHONE_DETECTION)                                     */
/* ========================================================================== */
/** \addtogroup TAPI_INTERFACE_PHONE_DETECTION */
/**@{*/

#ifdef TAPI_ONE_DEVNODE
/** Structure used during start of an analog line capacitance measurement.
    Used by \ref IFX_TAPI_LINE_MEASURE_CAPACITANCE_START
    \ref IFX_TAPI_LINE_MEASURE_CAPACITANCE_STOP. */
typedef struct {
   /** Device number. */
   IFX_uint16_t dev;
   /** Channel number. */
   IFX_uint16_t ch;
} IFX_TAPI_LINE_MEASURE_CAPACITANCE_t;
#endif /* TAPI_ONE_DEVNODE */

/** Structure used to configure the FXS phone detection state machine.*/
typedef struct {
   /** Telephone capacitance threshold [nF]. A telephone is detected in case
       the measured capacitance is above this given threshold.
       Default: 20 nF. */
   IFX_uint32_t nCapacitance;
   /** Off-hook detection timer (T3) value [ms]. The line feeding power is
       enabled for off-hook detection. It is reduced again in case no off-hook
       event detects a telephone within the given time period.
       Default: 200 ms. */
   IFX_uint32_t nOffHookTime;
   /** Find phone detection period (T2) [ms]. The phone detection period, off-hook detection,
       to find a newly connected phone.
       Default: 3000 ms. */
   IFX_uint32_t nFindPeriod;
   /** Lost phone detection period (T1) [s]. The phone detection period,
       capacitance measurement, to ensure that the telephone is still connected.
       Default: 3600 s. */
   IFX_uint32_t nLostPeriod;
} IFX_TAPI_LINE_PHONE_DETECT_CFG_t;

/**@}*/ /* TAPI_INTERFACE_PHONE_DETECTION */
#endif /* #ifndef TAPI4_DXY_DOC */

/* ===================================================================== */
/* TAPI Network Line-Testing Services, structures                          */
/* (Group TAPI_INTERFACE_NLT)                                            */
/* ===================================================================== */
/** \addtogroup TAPI_INTERFACE_NLT */
/**@{*/

/** List of NLT test identifiers. Used by the TAPI
   ioctl commands IFX_TAPI_NLT_START and IFX_TAPI_NLT_RESULT_GET. */
typedef enum
{
   /** NLT AC meter measurement ID */
   IFX_TAPI_NLT_AC_METER_ID         = 0,
   /** NLT AC transhybrid measurement ID */
   IFX_TAPI_NLT_AC_TRANSHYBRID_ID   = 1,
   /** NLT AC frequency response measurement ID */
   IFX_TAPI_NLT_AC_FREQRESPONSE_ID  = 2,
   /** NLT AC idle noise measurement ID */
   IFX_TAPI_NLT_AC_IDLENOISE_ID     = 3,
   /** NLT AC gain tracking measurement ID */
   IFX_TAPI_NLT_AC_GAINTRACKING_ID  = 4,
   /** NLT DC meter measurement ID */
   IFX_TAPI_NLT_DC_METER_ID         = 5,
   /** NLT current measurement ID */
   IFX_TAPI_NLT_CURRENT_ID          = 6,
   /** NLT voltage measurement ID */
   IFX_TAPI_NLT_VOLTAGE_ID          = 7,
   /** NLT to ground measurement ID */
   IFX_TAPI_NLT_TOGROUND_ID         = 8,
   /** NLT tip-ring measurement ID */
   IFX_TAPI_NLT_TIPRING_ID          = 9,
   /** NLT tip-ring measurement (results 2) ID */
   IFX_TAPI_NLT_TIPRING2_ID         = 10,
   /** NLT network (ultimate) measurement ID */
   IFX_TAPI_NLT_NETWORK_ID          = 11,
   /** NLT admittance measurement ID */
   IFX_TAPI_NLT_ADMITTANCE_ID       = 12,
   /** None */
   IFX_TAPI_NLT_NONE                = 13
} IFX_TAPI_NLT_TESTID_t;

#ifndef TAPI_DXY_DOC
/** Used by \ref IFX_TAPI_NLT_ACLM_Result_t to configure number of results. */
#define IFX_TAPI_ACLM_MAX_MP_RESULTS 40

/** NLT Result measurement structure, see \ref IFX_TAPI_NLT_ACLM_Result_t */
typedef struct
{
   /** Argument */
   IFX_int32_t arg;
   /** Tone frequency */
   IFX_int32_t freq;
   /** Tone Level */
   IFX_int32_t level;
   /** Tone Reference Level (Gain Tracking only) */
   IFX_int32_t ref_level;
   /** AC Integration Time [ms] */
   IFX_uint32_t Int;
    /** AC Levelmeter Inband Result Shift */
   IFX_int32_t AcInbSh;
   /** AC Levelmeter Inband Result */
   IFX_uint32_t AcInb;
   /** AC Levelmeter Outband Result Shift */
   IFX_int32_t AcOutbSh;
   /** AC Levelmeter Outband Result */
   IFX_uint32_t AcOutb;
} IFX_TAPI_NLT_ACLM_MP_Result_t;

/** Possible, not mandatory NLT AC Measurement results structure. */
typedef struct
{
  IFX_TAPI_NLT_TESTID_t type;
  IFX_uint8_t mp_count;
  IFX_TAPI_NLT_ACLM_MP_Result_t mp_val[IFX_TAPI_ACLM_MAX_MP_RESULTS];
} IFX_TAPI_NLT_ACLM_Result_t;
#endif /* #ifndef TAPI_DXY_DOC */

/** Structure used during start of an NLT test. Used by
    \ref IFX_TAPI_NLT_TEST_START. */
typedef struct
{
   /** Device number */
   IFX_uint16_t dev;
   /** Channel number */
   IFX_uint16_t ch;
   /** Test start status */
   IFX_int32_t nStatus;
   /** When set to IFX_TRUE, the calling process is put to sleep on test start.
       The process is woken up upon reception of the LTEST. */
   IFX_boolean_t bBlock;
   /** Identifier of test to be started. */
   IFX_TAPI_NLT_TESTID_t testID;
   /** Where required, configuration arguments relative to the test are passed.
       The TAPI does not have to know about the details, therefore the
       details are known by the LL driver and NLT Library, only. */
   IFX_void_t *pTestCfg;
} IFX_TAPI_NLT_TEST_START_t;

/** Structure used for reading NLT test results. Used by
    \ref IFX_TAPI_NLT_RESULT_GET. */
typedef struct
{
   /** Device number */
   IFX_uint16_t dev;
   /** Channel number */
   IFX_uint16_t ch;
   /** Result get status */
   IFX_int32_t nStatus;
   /** Identifier of test from which results are to be read. */
   IFX_TAPI_NLT_TESTID_t testID;
   /** When the respective NLT LIB API is called to retrieve the results
       from a test previously started (non-blocking call), it might be necessary,
       depending on the test, to retrieve the configuration parameters which were
       used during the test. These configuration parameters could be required for
       the calculation of the final results. For this reason, the caller
       where needed passes a pointer (else NULL) of the related type, so that the LL
       implementation copies in the cached configuration parameters used by the
       last test started. */
   IFX_void_t *pTestCfg;
   /** Pointer to a data structure large enough to accumulate the results
       related to the NLT test. The TAPI does not have to know about the details,
       therefore the details are known by the LL driver and NLT Library. */
   IFX_void_t *pTestResults;
} IFX_TAPI_NLT_RESULT_GET_t;

#ifndef TAPI_DXY_DOC
/** Structure used to start the NLT capacitance measurement. Used by
    \ref IFX_TAPI_NLT_CAPACITANCE_START. */
typedef struct
{
   /** Device number */
   IFX_uint16_t dev;
   /** Channel number */
   IFX_uint16_t ch;
} IFX_TAPI_NLT_CAPACITANCE_START_t;

/** Structure used to stop the NLT capacitance measurement. Used by
    \ref IFX_TAPI_NLT_CAPACITANCE_STOP. */
typedef struct
{
   /** Device number */
   IFX_uint16_t dev;
   /** Channel number */
   IFX_uint16_t ch;
} IFX_TAPI_NLT_CAPACITANCE_STOP_t;

/** Structure used for reading the NLT capacitance measurement results.
    Used by \ref IFX_TAPI_NLT_CAPACITANCE_RESULT_GET. */
typedef struct
{
   /** Device number */
   IFX_uint16_t dev;
   /** Channel number */
   IFX_uint16_t ch;
   /** Validity of measurement result tip to ring. */
   IFX_boolean_t  bValidTip2Ring;
   /** Measured capacitance tip to ring [nF]. */
   IFX_uint32_t   nCapTip2Ring;
   /** Validity of measurement results tip to ground and ring to ground.*/
   IFX_boolean_t  bValidLine2Gnd;
   /** Measured capacitance tip to ground [nF]. */
   IFX_uint32_t   nCapTip2Gnd;
   /** Measured capacitance ring to ground [nF]. */
   IFX_uint32_t   nCapRing2Gnd;
   /** Stored open loop capacitance tip to ring [nF].
       Float value stored in unsigned int bytes. */
   IFX_uint32_t   fOlCapTip2Ring;
   /** Stored open loop capacitance tip to ground [nF].
    * Float value stored in unsigned int bytes. */
   IFX_uint32_t   fOlCapTip2Gnd;
   /** Stored open loop capacitance ring to ground [nF].
       Float value stored in unsigned int bytes. */
   IFX_uint32_t   fOlCapRing2Gnd;
} IFX_TAPI_NLT_CAPACITANCE_RESULT_t;

/** List of Rmeasure configuration options. */
typedef enum
{
   /** Use the driver default for Rmeasure setting. */
   IFX_TAPI_NLT_RMEAS_DEFAULT = 0,
   /** Rmeasure of 1MOhm is used. */
   IFX_TAPI_NLT_RMEAS_1MOHM   = 1,
   /** Rmeasure of 1.5MOhm is used. */
   IFX_TAPI_NLT_RMEAS_1_5MOHM = 2,
} IFX_TAPI_NLT_RMEAS_CFG_t;

/** Structure used for storing the configuration of the Rmeas resistance
    variant connected to the chip. */
typedef struct
{
   /** Device number */
   IFX_uint16_t dev;
   /** Channel number */
   IFX_uint16_t ch;
   /** Configuration which Rmeas variant is connected to the chip. */
   IFX_TAPI_NLT_RMEAS_CFG_t nRmeas;
} IFX_TAPI_NLT_CONFIGURATION_RMES_t;
#endif /* #ifndef TAPI_DXY_DOC */

/** Structure used for storing the configuration of the measurement path
    for line testing.
    The float values in here are correction factors that are just stored in
    the driver and will be returned together with the associated measurement
    results. */
typedef struct
{
   /** Device number */
   IFX_uint16_t dev;
   /** Channel number */
   IFX_uint16_t ch;
   /** Device type,
     \tapiv3 see \ref IFX_TAPI_GR909_DEV_t. \endtapiv3
     \tapiv4 reserved. \endtapiv4 */
   IFX_TAPI_GR909_DEV_t dev_type;
   /** Open loop capacitance tip to ring [nF].
       Float value stored in unsigned int bytes. */
   IFX_uint32_t  fOlCapTip2Ring;
   /** Open loop capacitance tip to ground [nF].
       Float value stored in unsigned int bytes. */
   IFX_uint32_t  fOlCapTip2Gnd;
   /** Open loop capacitance ring to ground [nF].
       Float value stored in unsigned int bytes. */
   IFX_uint32_t  fOlCapRing2Gnd;
   /** Open loop resistance tip to ring [Ohm].
    *  Float value stored in unsigned int bytes. */
   IFX_uint32_t   fOlResTip2Ring;
   /** Open loop resistance tip to ground [Ohm].
    * Float value stored in unsigned int bytes. */
   IFX_uint32_t   fOlResTip2Gnd;
   /** Open loop resistance ring to ground [Ohm].
       Float value stored in unsigned int bytes. */
   IFX_uint32_t   fOlResRing2Gnd;
} IFX_TAPI_NLT_CONFIGURATION_OL_t;
/**@}*/ /* TAPI_INTERFACE_NLT */

/* ======================================================================= */
/* TAPI Message Waiting Lamp Services, structures                          */
/* (Group TAPI_INTERFACE_MWL)                                              */
/* ======================================================================= */
/** \addtogroup TAPI_INTERFACE_MWL */
/**@{*/

/** Structure for message waiting lamp configuration; used by
    \ref IFX_TAPI_MWL_ACTIVATION_SET and \ref IFX_TAPI_MWL_ACTIVATION_GET. */
typedef struct
{
   /** \tapiv3 Not used in TAPI for CPE Products. \endtapiv3
       \tapiv4 Device number.                \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used in TAPI for CPE Products. \endtapiv3
       \tapiv4 Channel number.               \endtapiv4 */
   IFX_uint16_t ch;
   /** Activation of the message waiting lamp. */
   IFX_enDis_t      nActivation;
} IFX_TAPI_MWL_ACTIVATION_t;

/**@}*/ /* TAPI_INTERFACE_MWL */


/* ===================================================================== */
/* TAPI Event Services, structures (Group TAPI_INTERFACE_EVENT)          */
/* ===================================================================== */

/** \addtogroup TAPI_INTERFACE_EVENT */
/**@{*/

/* =============================== */
/* Macros                          */
/* =============================== */

#define IFX_TAPI_EVENT_TYPE_MASK                              0xFFFF0000
#define IFX_TAPI_EVENT_SUBTYPE_MASK                           0x0000FFFF
#define IFX_TAPI_EVENT_TYPE_FAULT_MASK                        0xF0000000

/* =============================== */
/* enum                            */
/* =============================== */

/** List of event types */
typedef enum
{
   /** Reserved */
   IFX_TAPI_EVENT_TYPE_NONE                                 = 0x00000000,
   /** Event on GPIOs, channel IOs */
   IFX_TAPI_EVENT_TYPE_IO_GENERAL                           = 0x10000000,
   /** Reserved; external interrupt */
   IFX_TAPI_EVENT_TYPE_IO_INTERRUPT                         = 0x11000000,
   /** Ringing, hook events */
   IFX_TAPI_EVENT_TYPE_FXS                                  = 0x20000000,
   /** Ringing, polarity reversal*/
   IFX_TAPI_EVENT_TYPE_FXO                                  = 0x21000000,
   /** Line-testing events */
   IFX_TAPI_EVENT_TYPE_LT                                   = 0x29000000,
   /** Pulse digit detected */
   IFX_TAPI_EVENT_TYPE_PULSE                                = 0x30000000,
   /** DTMF digit detected */
   IFX_TAPI_EVENT_TYPE_DTMF                                 = 0x31000000,
   /** Caller ID events */
   IFX_TAPI_EVENT_TYPE_CID                                  = 0x32000000,
   /** Tone generation event, e.g. tone generation ended. */
   IFX_TAPI_EVENT_TYPE_TONE_GEN                             = 0x33000000,
   /** Tone detection event, e.g. call progress tones. */
   IFX_TAPI_EVENT_TYPE_TONE_DET                             = 0x34000000,
   /** Calibration events */
   IFX_TAPI_EVENT_TYPE_CALIBRATION                          = 0x36000000,
   /** Metering event */
   IFX_TAPI_EVENT_METERING                                  = 0x38000000,
   /** E.g. FW download finished, bad CRC */
   IFX_TAPI_EVENT_TYPE_DOWNLOAD                             = 0x70000000,
   /** Information about the system status. */
   IFX_TAPI_EVENT_TYPE_INFO                                 = 0xA0000000,
   /** Debug information, e.g. dump of some registers or memory areas. */
   IFX_TAPI_EVENT_TYPE_DEBUG                                = 0xD0000000,
   /** Events of the low-level driver. */
   IFX_TAPI_EVENT_TYPE_LL_DRIVER                            = 0xE0000000,
   /** Reserved */
   IFX_TAPI_EVENT_TYPE_FAULT_GENERAL                        = 0xF1000000,
   /** E.g. over-temperature, ground key detected. */
   IFX_TAPI_EVENT_TYPE_FAULT_LINE                           = 0xF2000000,
   /** (Reserved) e.g. watchdog, PLL. */
   IFX_TAPI_EVENT_TYPE_FAULT_HW                             = 0xF3000000,
   /** (Reserved) e.g. mailbox error. */
   IFX_TAPI_EVENT_TYPE_FAULT_FW                             = 0xF4000000,
} IFX_TAPI_EVENT_TYPE_t;

/** List of event IDs */
typedef enum
{
   /* NONE (reserved) */
   /** Reserved */
   IFX_TAPI_EVENT_NONE                 = IFX_TAPI_EVENT_TYPE_NONE,

#ifndef TAPI_DXY_DOC
   /* FXS */
   /** No event (reserved). */
   IFX_TAPI_EVENT_FXS_NONE             = IFX_TAPI_EVENT_TYPE_FXS,
#endif
#ifndef TAPI3_DXY_DOC
   /** FXS line is ringing (reserved). */
   IFX_TAPI_EVENT_FXS_RING             = IFX_TAPI_EVENT_TYPE_FXS | 0x0001,
   /** End of a ring burst detected; this event may be used as a trigger to
       start an FSK transmission (\ref IFX_TAPI_CID_TX_INFO_START). */
   IFX_TAPI_EVENT_FXS_RINGBURST_END    = IFX_TAPI_EVENT_TYPE_FXS | 0x0002,
#endif /* #ifndef TAPI3_DXY_DOC */
   /** Indicates that ringing has ended; this event is only generated when
       the configured maximum ring cadences have been played. This event is
       not generated when ringing is stopped by either going off-hook or
       \ref IFX_TAPI_RING_STOP. This is because, for off-hook, there already is
       the off-hook event. Ring stop is initiated by the application and
       does not need an extra confirmation. */
   IFX_TAPI_EVENT_FXS_RINGING_END      = IFX_TAPI_EVENT_TYPE_FXS | 0x0003,
   /** Hook event: on-hook. */
   IFX_TAPI_EVENT_FXS_ONHOOK           = IFX_TAPI_EVENT_TYPE_FXS | 0x0004,
   /** Hook event: off-hook. */
   IFX_TAPI_EVENT_FXS_OFFHOOK          = IFX_TAPI_EVENT_TYPE_FXS | 0x0005,
   /** Hook event: flash hook. */
   IFX_TAPI_EVENT_FXS_FLASH            = IFX_TAPI_EVENT_TYPE_FXS | 0x0006,
#ifndef TAPI_DXY_DOC
   /** Hook event: on-hook detected by interrupt.
       - This (internal) event is not available to the application. */
   IFX_TAPI_EVENT_FXS_ONHOOK_INT       = IFX_TAPI_EVENT_TYPE_FXS | 0x0007,
   /** Hook event: off-hook detected by interrupt.
       - This (internal) event is not available to the application. */
   IFX_TAPI_EVENT_FXS_OFFHOOK_INT      = IFX_TAPI_EVENT_TYPE_FXS | 0x0008,
#endif
   /** Measurement results for continuous measurement are available
       to be read out. */
   IFX_TAPI_EVENT_CONTMEASUREMENT      = IFX_TAPI_EVENT_TYPE_FXS | 0x0009,
   /** Raw hook event: raw on-hook (break). */
   IFX_TAPI_EVENT_FXS_RAW_ONHOOK       = IFX_TAPI_EVENT_TYPE_FXS | 0x000a,
   /** Raw hook event: raw off-hook (make). */
   IFX_TAPI_EVENT_FXS_RAW_OFFHOOK      = IFX_TAPI_EVENT_TYPE_FXS | 0x000b,
#ifndef TAPI_DXY_DOC
   /** The line feeding mode has changed. The new line feeding mode is given as an argument.
       This event is used by the driver internally and may be used by the application for test purposes only.
       Reporting of the event is deactivated per default */
   IFX_TAPI_EVENT_FXS_LINE_MODE        = IFX_TAPI_EVENT_TYPE_FXS | 0x000c,
#endif /* #ifndef TAPI_DXY_DOC */
   /** First ring burst ended and CID can be played out. Can be used when CID
       is played out by external DSP. */
   IFX_TAPI_EVENT_FXS_RINGPAUSE_CIDTX  = IFX_TAPI_EVENT_TYPE_FXS | 0x000e,
   /** Ring abort condition reached. */
   IFX_TAPI_EVENT_FXS_RING_ABORT       = IFX_TAPI_EVENT_TYPE_FXS | 0x000f,

   /** GR-909 test results ready. */
   IFX_TAPI_EVENT_LT_GR909_RDY         = IFX_TAPI_EVENT_TYPE_LT  | 0x0001,

   /** Line testing finished (results are ready, where applicable). */
   IFX_TAPI_EVENT_NLT_END              = IFX_TAPI_EVENT_TYPE_LT  | 0x0002,
   /** End of tip to ring capacitance measurement. */
   IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY     = IFX_TAPI_EVENT_TYPE_LT  | 0x0003,
#ifndef TAPI_DXY_DOC
   /** Internal event: end of capacitance measurement.
       - This (internal) event is not available to the application. */
   IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_RDY_INT = IFX_TAPI_EVENT_TYPE_LT  | 0x0004,
   /** Reserved (obsolete). */
   IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_START_INT = IFX_TAPI_EVENT_TYPE_LT | 0x0005,
#endif
#ifndef TAPI3_DXY_DOC
   /** End of capacitance to ground measurement. */
   IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_GND_RDY = IFX_TAPI_EVENT_TYPE_LT  | 0x0006,
#endif /* #ifndef TAPI3_DXY_DOC */

#ifndef TAPI_DXY_DOC
   /* PULSE */
   /** No event (reserved). */
   IFX_TAPI_EVENT_PULSE_NONE           = IFX_TAPI_EVENT_TYPE_PULSE,
#endif
   /** Pulse digit detected. */
   IFX_TAPI_EVENT_PULSE_DIGIT          = IFX_TAPI_EVENT_TYPE_PULSE | 0x0001,
   /** Indicates start of pulse dialing. This event can be used to stop
       the dial tone. */
   IFX_TAPI_EVENT_PULSE_START          = IFX_TAPI_EVENT_TYPE_PULSE | 0x0002,

#ifndef TAPI_DXY_DOC
   /* DTMF */
   /** No event (reserved). */
   IFX_TAPI_EVENT_DTMF_NONE            = IFX_TAPI_EVENT_TYPE_DTMF,
#endif
   /** DTMF tone detected. */
   IFX_TAPI_EVENT_DTMF_DIGIT           = IFX_TAPI_EVENT_TYPE_DTMF | 0x0001,
   /** DTMF tone end detected. */
   IFX_TAPI_EVENT_DTMF_END             = IFX_TAPI_EVENT_TYPE_DTMF | 0x0002,

#ifndef TAPI_DXY_DOC
   /* Calibration */
   /** Reserved */
   IFX_TAPI_EVENT_CALIBRATION_NONE     = IFX_TAPI_EVENT_TYPE_CALIBRATION,
#endif
   /** End of calibration.
       The calibration process has finished or was stopped because of an error. */
   IFX_TAPI_EVENT_CALIBRATION_END      = IFX_TAPI_EVENT_TYPE_CALIBRATION | 0x0001,
#ifndef TAPI_DXY_DOC
   /** Internal event: end of calibration.
       - This (internal) event is not available to the application. */
   IFX_TAPI_EVENT_CALIBRATION_END_INT  = IFX_TAPI_EVENT_TYPE_CALIBRATION | 0x0002,
   /** Internal event: end of calibration.
       - This (internal) event is not available to the application. */
   IFX_TAPI_EVENT_CALIBRATION_END_SINT = IFX_TAPI_EVENT_TYPE_CALIBRATION | 0x0003,
#endif

#ifndef TAPI_DXY_DOC
   /* Metering */
   /** Reserved */
   IFX_TAPI_EVENT_METERING_NONE        = IFX_TAPI_EVENT_METERING,
#endif
   /** Metering event. */
   IFX_TAPI_EVENT_METERING_END         = IFX_TAPI_EVENT_METERING | 0x0001,

#ifndef TAPI_DXY_DOC
   /* CID */
   /* TX */
   /** TX no event (reserved). */
   IFX_TAPI_EVENT_CID_TX_NONE          = IFX_TAPI_EVENT_TYPE_CID,
#endif
   /** Reserved; start of CID TX sequence (reserved). */
   IFX_TAPI_EVENT_CID_TX_SEQ_START     = IFX_TAPI_EVENT_TYPE_CID | 0x0001,
   /** CID TX protocol sequence ended. The event indicates that the CID
       sequence that was started with \ref IFX_TAPI_CID_TX_SEQ_START has
       ended in a regular manner. The event is sent immediately after the CID
       data transmission phase (FSK or DTMF), including the programmed pause.
       At the time in which the event is received by application software,
       the attached telephone should already display the CID data. */
   IFX_TAPI_EVENT_CID_TX_SEQ_END       = IFX_TAPI_EVENT_TYPE_CID | 0x0002,
   /** Start of CID TX information (reserved). */
   IFX_TAPI_EVENT_CID_TX_INFO_START    = IFX_TAPI_EVENT_TYPE_CID | 0x0003,
   /** CID TX FSK message sent. This event indicates that the CID data
       transmission that was started with \ref IFX_TAPI_CID_TX_INFO_START
       has ended. At the time in which the event is received by the application software,
       the attached telephone should already display the CID data
       (in case of CID type 1 or type 2). */
   IFX_TAPI_EVENT_CID_TX_INFO_END      = IFX_TAPI_EVENT_TYPE_CID | 0x0004,
   /** CID TX sequence error: acknowledge not received. No ack was received
       from the CID receiver during the time-out phase. The sequence ended
       with an error. The ACK is required in most CID type 2 protocols
       (all standards but NTT) and in CID type 1 for NTT
       ('Primary Answer Signal Detected'). */
   IFX_TAPI_EVENT_CID_TX_NOACK_ERR     = IFX_TAPI_EVENT_TYPE_CID | 0x0005,
#ifndef TAPI3_DXY_DOC
   /** Error in ring cadence setting used for the CID type 1 TX sequence,
       in case of alert type 'first ring' (Telcordia or ETSI).
       The programmed time between the first and second ring bursts is too short
       to transmit the CID data (FSK or DTMF). Check ring cadence
       and CID timing settings and ensure that enough time is reserved for
       the CID data transmission. */
   IFX_TAPI_EVENT_CID_TX_RINGCAD_ERR   = IFX_TAPI_EVENT_TYPE_CID | 0x0006,
#endif /* #ifndef TAPI3_DXY_DOC */
   /** Buffer underrun on the CID FSK sender. This indicates that, after the
       request for more data, the host controller was too slow to refill data
       into the transmission buffer. This can happen if interrupts are
       not processed on the host controller for some time. */
   IFX_TAPI_EVENT_CID_TX_UNDERRUN_ERR  = IFX_TAPI_EVENT_TYPE_CID | 0x0007,
   /** CID TX sequence error: second acknowledge not received
       ('Incoming Successful Signal Detected', only for NTT CID type 1).
       No ACK was received during the time out, from an NTT CID receiver
       that acknowledges the FSK transmission. */
   IFX_TAPI_EVENT_CID_TX_NOACK2_ERR    = IFX_TAPI_EVENT_TYPE_CID | 0x0008,
#ifndef TAPI3_DXY_DOC
   /** CID transmission stopped: this event indicates that the CID state machine
       reached the end of the sequence. It can be used to clean up some data or
       restore previous states. */
   IFX_TAPI_EVENT_CIDSM_END            = IFX_TAPI_EVENT_TYPE_CID | 0x0009,
#endif /* #ifndef TAPI3_DXY_DOC */
   /** Internal event: CID data transmission ended.
       The event is sent immediately after data transmission and translated
       into either a ...SEQ_END or ...INFO_END event, depending on the current
       transmission mode.
       - This (internal) event is not available to the application. */
   IFX_TAPI_EVENT_CID_TX_END           = IFX_TAPI_EVENT_TYPE_CID | 0x0010,

   /* TONE_GEN */
#ifndef TAPI_DXY_DOC
   /** No event (reserved). */
   IFX_TAPI_EVENT_TONE_GEN_NONE        = IFX_TAPI_EVENT_TYPE_TONE_GEN,
#endif
#ifndef TAPI3_DXY_DOC
   /** Tone generator busy (reserved). */
   IFX_TAPI_EVENT_TONE_GEN_BUSY        = IFX_TAPI_EVENT_TYPE_TONE_GEN | 0x0001,
#endif /* #ifndef TAPI3_DXY_DOC */
   /** Tone generation ended. */
   IFX_TAPI_EVENT_TONE_GEN_END         = IFX_TAPI_EVENT_TYPE_TONE_GEN | 0x0002,
   /** Tone generation end event used internally to trigger the state machines.
       - This (internal) event is not available to the application. */
   IFX_TAPI_EVENT_TONE_GEN_END_RAW     = IFX_TAPI_EVENT_TYPE_TONE_GEN | 0x0003,

   /* TONE_DET */
#ifndef TAPI_DXY_DOC
   /** No event (reserved). */
   IFX_TAPI_EVENT_TONE_DET_NONE        = IFX_TAPI_EVENT_TYPE_TONE_DET,
#endif
#ifndef TAPI3_DXY_DOC
   /** Tone detect receive (reserved). */
   IFX_TAPI_EVENT_TONE_DET_RECEIVE     = IFX_TAPI_EVENT_TYPE_TONE_DET | 0x0001,
   /** Tone detect transmit (reserved). */
   IFX_TAPI_EVENT_TONE_DET_TRANSMIT    = IFX_TAPI_EVENT_TYPE_TONE_DET | 0x0002,
#endif /* #ifndef TAPI3_DXY_DOC */

   /* DEBUG */
#ifndef TAPI_DXY_DOC
   /** For debug purposes (reserved). */
   IFX_TAPI_EVENT_DEBUG_NONE           = IFX_TAPI_EVENT_TYPE_DEBUG,
#endif
   /** Debug command error event. */
   IFX_TAPI_EVENT_DEBUG_CERR           = IFX_TAPI_EVENT_TYPE_DEBUG | 0x0001,

   /* Low-level driver events. */
#ifndef TAPI_DXY_DOC
   /** Device-specific events (reserved). */
   IFX_TAPI_EVENT_LL_DRIVER_NONE       = IFX_TAPI_EVENT_TYPE_LL_DRIVER,
#endif
#ifndef TAPI_DXY_DOC
   /* FAULT_GENERAL */
   /** Generic fault, no event (reserved). */
   IFX_TAPI_EVENT_FAULT_GENERAL_NONE   = IFX_TAPI_EVENT_TYPE_FAULT_GENERAL,
#endif
#ifndef TAPI3_DXY_DOC
   /** General system fault (reserved). */
   IFX_TAPI_EVENT_FAULT_GENERAL        = IFX_TAPI_EVENT_TYPE_FAULT_GENERAL | 0x1,
   /** General system fault (reserved). */
   IFX_TAPI_EVENT_FAULT_GENERAL_CHINFO = IFX_TAPI_EVENT_TYPE_FAULT_GENERAL | 0x2,
   /** General device fault (reserved). */
   IFX_TAPI_EVENT_FAULT_GENERAL_DEVINFO = IFX_TAPI_EVENT_TYPE_FAULT_GENERAL | 0x3,
#endif /* #ifndef TAPI3_DXY_DOC */
   /** General fault, channel event FIFO overflow. */
   IFX_TAPI_EVENT_FAULT_GENERAL_EVT_FIFO_OVERFLOW =
                                         IFX_TAPI_EVENT_TYPE_FAULT_GENERAL | 0x4,

#ifndef TAPI_DXY_DOC
   /* FAULT_LINE */
   /** Reserved; line fault, no event. */
   IFX_TAPI_EVENT_FAULT_LINE_NONE      = IFX_TAPI_EVENT_TYPE_FAULT_LINE,
#endif
#ifndef TAPI3_DXY_DOC
   /** Ground key, positive polarity, currently used by DUSLIC only. */
   IFX_TAPI_EVENT_FAULT_LINE_GK_POS    = IFX_TAPI_EVENT_TYPE_FAULT_LINE | 0x0001,
   /** Ground key, negative polarity, currently used by DUSLIC only. */
   IFX_TAPI_EVENT_FAULT_LINE_GK_NEG    = IFX_TAPI_EVENT_TYPE_FAULT_LINE | 0x0002,
#endif /* #ifndef TAPI3_DXY_DOC */
   /** Ground key low. */
   IFX_TAPI_EVENT_FAULT_LINE_GK_LOW    = IFX_TAPI_EVENT_TYPE_FAULT_LINE | 0x0003,
   /** Ground key high. */
   IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH   = IFX_TAPI_EVENT_TYPE_FAULT_LINE | 0x0004,
   /** Over-temperature. */
   IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP  = IFX_TAPI_EVENT_TYPE_FAULT_LINE | 0x0005,
   /** Over-current; currently used by XWAY DUSLIC-xT only. */
   IFX_TAPI_EVENT_FAULT_LINE_OVERCURRENT  = IFX_TAPI_EVENT_TYPE_FAULT_LINE | 0x0006,
#ifndef TAPI_DXY_DOC
   /** Internal event: ground key low.
       - This (internal) event is not available to the application. */
   IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_INT = IFX_TAPI_EVENT_TYPE_FAULT_LINE | 0x0007,
   /** Internal event: ground key high.
       - This (internal) event is not available to the application. */
   IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH_INT = IFX_TAPI_EVENT_TYPE_FAULT_LINE | 0x0008,
#endif
   /** Ground key low end. */
   IFX_TAPI_EVENT_FAULT_LINE_GK_LOW_END = IFX_TAPI_EVENT_TYPE_FAULT_LINE | 0x0009,
   /** Ground key high end. */
   IFX_TAPI_EVENT_FAULT_LINE_GK_HIGH_END = IFX_TAPI_EVENT_TYPE_FAULT_LINE | 0x000a,
   /** Over-temperature end. */
   IFX_TAPI_EVENT_FAULT_LINE_OVERTEMP_END = IFX_TAPI_EVENT_TYPE_FAULT_LINE | 0x000b,
   /** Operating Mode Ignored. */
   IFX_TAPI_EVENT_FAULT_LINE_OMI = IFX_TAPI_EVENT_TYPE_FAULT_LINE | 0x000c,

#ifndef TAPI_DXY_DOC
   /* FAULT_HW */
   /** Reserved */
   IFX_TAPI_EVENT_FAULT_HW_NONE        = IFX_TAPI_EVENT_TYPE_FAULT_HW,
#endif
   /** SPI access error; currently used by XWAY DUSLIC-xT only. */
   IFX_TAPI_EVENT_FAULT_HW_SPI_ACCESS  = IFX_TAPI_EVENT_TYPE_FAULT_HW | 0x0001,
   /** Clock failure; currently used by XWAY DUSLIC-xT only. */
   IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL  = IFX_TAPI_EVENT_TYPE_FAULT_HW | 0x0002,
   /** Clock failure end; currently used by XWAY DUSLIC-xT only. */
   IFX_TAPI_EVENT_FAULT_HW_CLOCK_FAIL_END  = IFX_TAPI_EVENT_TYPE_FAULT_HW | 0x0003,
   /** Hardware failure; currently used by XWAY DUSLIC-xT only. */
   IFX_TAPI_EVENT_FAULT_HW_FAULT       = IFX_TAPI_EVENT_TYPE_FAULT_HW | 0x0004,
#ifndef TAPI3_DXY_DOC
   /** Hardware synchronization event; currently used by XWAY VINETIC-xT only. */
   IFX_TAPI_EVENT_FAULT_HW_SYNC        = IFX_TAPI_EVENT_TYPE_FAULT_HW | 0x0005,
   /** A HW reset occurred; currently used by XWAY VINETIC-xT only. */
   IFX_TAPI_EVENT_FAULT_HW_RESET       = IFX_TAPI_EVENT_TYPE_FAULT_HW | 0x0006,
#endif /* #ifndef TAPI3_DXY_DOC */
   /** A fatal communication error on the SSI bus happened. */
   IFX_TAPI_EVENT_FAULT_HW_SSI_ERR     = IFX_TAPI_EVENT_TYPE_FAULT_HW | 0x0007,
#ifndef TAPI3_DXY_DOC
   /** A communication error on the SSI is resolved. */
   IFX_TAPI_EVENT_FAULT_HW_SSI_ERR_END = IFX_TAPI_EVENT_TYPE_FAULT_HW | 0x0008,
#endif /* #ifndef TAPI3_DXY_DOC */
#ifndef TAPI_DXY_DOC
   /** Internal event: SSI bus fault.
       - This (internal) event is not available to the application. */
   IFX_TAPI_EVENT_FAULT_HW_SSI_ERROR_INT = IFX_TAPI_EVENT_TYPE_FAULT_HW | 0x0009,
   /** Internal event: SSI bus recovered.
       - This (internal) event is not available to the application. */
   IFX_TAPI_EVENT_FAULT_HW_SSI_FIXED_INT = IFX_TAPI_EVENT_TYPE_FAULT_HW | 0x000A,
#endif

#ifndef TAPI_DXY_DOC
   /* FAULT_FW */
   /** Reserved */
   IFX_TAPI_EVENT_FAULT_FW_NONE        = IFX_TAPI_EVENT_TYPE_FAULT_FW,
#endif
   /** Event mailbox out underflow; currently used by XWAY DUSLIC-xT only. */
   IFX_TAPI_EVENT_FAULT_FW_EBO_UF      = IFX_TAPI_EVENT_TYPE_FAULT_FW | 0x0001,
   /** Event mailbox out overflow; currently used by XWAY DUSLIC-xT only. */
   IFX_TAPI_EVENT_FAULT_FW_EBO_OF      = IFX_TAPI_EVENT_TYPE_FAULT_FW | 0x0002,
   /** Command mailbox out underflow. */
   IFX_TAPI_EVENT_FAULT_FW_CBO_UF      = IFX_TAPI_EVENT_TYPE_FAULT_FW | 0x0003,
   /** Command mailbox out overflow; currently used by XWAY DUSLIC-xT only. */
   IFX_TAPI_EVENT_FAULT_FW_CBO_OF      = IFX_TAPI_EVENT_TYPE_FAULT_FW | 0x0004,
   /** Command mailbox in overflow; currently used by XWAY DUSLIC-xT only. */
   IFX_TAPI_EVENT_FAULT_FW_CBI_OF      = IFX_TAPI_EVENT_TYPE_FAULT_FW | 0x0005,
   /** Outbox header error. */
   IFX_TAPI_EVENT_FAULT_FW_OBX_HDR_ERR = IFX_TAPI_EVENT_TYPE_FAULT_FW | 0x0008,

}IFX_TAPI_EVENT_ID_t;


/** This structure extends the SPI hardware access events
    of \ref IFX_TAPI_EVENT_FAULT_HW_SPI_ACCESS. */
typedef enum
{
   /** SPI communication synchronization failure detected between the TAPI and
       the device. */
   IFX_TAPI_EVENT_FAULT_HW_SYNC_SPI_FAILURE  = 0,
   /** The TAPI starts a recovery process to the device over SPI. */
   IFX_TAPI_EVENT_FAULT_HW_SYNC_SPI_RECOVERY = 1,
   /** The TAPI-to-device communication over SPI is synchronized (again). */
   IFX_TAPI_EVENT_FAULT_HW_SYNC_SPI_NORMAL   = 2,
   /** Based on an unexpected error, the TAPI could not re-establish communication to the device. */
   IFX_TAPI_EVENT_FAULT_HW_SYNC_SPI_ABORT    = 3
} IFX_TAPI_EVENT_FAULT_HW_SYNC_t;

/* =============================== */
/* type definition                 */
/* =============================== */
/** This structure contains data specific to the pulse dialing event. */
typedef struct
{
   /** Reserved*/
   IFX_uint16_t reserved:8;
   /** Pulse digit number information (0 - 9).

   - 1: Key_1, key '1' detected
   - 2: Key_2, key '2' detected
   - 3: Key_3, key '3' detected
   - 4: Key_4, key '4' detected
   - 5: Key_5, key '5' detected
   - 6: Key_6, key '6' detected
   - 7: Key_7, key '7' detected
   - 8: Key_8, key '8' detected
   - 9: Key_9, key '9' detected
   - 11: Key_0, key '0' detected
*/
   IFX_uint16_t digit:8;
} IFX_TAPI_EVENT_DATA_PULSE_t;

/** This structure contains data specific to the DTMF event. */
typedef struct
{
#ifdef TAPI_VERSION4
   /** Detected from external (e.g. analog line, PCM bus, RTP in-band, etc.).*/
   IFX_uint32_t external:1;
   /** Detected from internal. */
   IFX_uint32_t internal:1;
#endif /* TAPI_VERSION4 */
#ifdef TAPI_VERSION3
   /** Detected on the local side.*/
   IFX_uint32_t local:1;
   /** Detected on the network side. */
   IFX_uint32_t network:1;
#endif /* TAPI_VERSION3 */
   /** Reserved */
   IFX_uint32_t reserved:6;
   /** DTMF digit number information.

   - 0: No_Key, no key detected
   - 11: Key_0, DTMF key '0' detected
   - 1: Key_1, DTMF key '1' detected
   - 2: Key_2, DTMF key '2' detected
   - 3: Key_3, DTMF key '3' detected
   - 4: Key_4, DTMF key '4' detected
   - 5: Key_5, DTMF key '5' detected
   - 6: Key_6, DTMF key '6' detected
   - 7: Key_7, DTMF key '7' detected
   - 8: Key_8, DTMF key '8' detected
   - 9: Key_9, DTMF key '9' detected
   - 10: Key_*, DTMF key '*' detected
   - 12: Key_#, DTMF key '#' detected
   - 28: Key_A, DTMF key 'A' detected
   - 29: Key_B, DTMF key 'B' detected
   - 30: Key_C, DTMF key 'C' detected
   - 31: Key_D, DTMF key 'D' detected
*/
   IFX_uint32_t digit:8;
   /** DTMF digit in ASCII representation.

   - 0: No_Key, no key detected
   - 48: Key_0, DTMF key '0' detected
   - 49: Key_1, DTMF key '1' detected
   - 50: Key_2, DTMF key '2' detected
   - 51: Key_3, DTMF key '3' detected
   - 52: Key_4, DTMF key '4' detected
   - 53: Key_5, DTMF key '5' detected
   - 54: Key_6, DTMF key '6' detected
   - 55: Key_7, DTMF key '7' detected
   - 56: Key_8, DTMF key '8' detected
   - 57: Key_9, DTMF key '9' detected
   - 42: Key_*, DTMF key '*' detected
   - 35: Key_#, DTMF key '#' detected
   - 65: Key_A, DTMF key 'A' detected
   - 66: Key_B, DTMF key 'B' detected
   - 67: Key_C, DTMF key 'C' detected
   - 68: Key_D, DTMF key 'D' detected
*/
   IFX_uint32_t ascii:8;
} IFX_TAPI_EVENT_DATA_DTMF_t;

/** This structure contains data specific to the tone generation event.
    This event is generated when a tone generation is stopped or has stopped
    automatically. The 'index' field contains the tone table index of the
    tone that was played out. */
typedef struct
{
#ifdef TAPI_VERSION4
   /** Generation on the external (e.g. analog line, PCM bus, RTP in-band, etc.).*/
   IFX_uint32_t external:1;
   /** Generation on the internal. */
   IFX_uint32_t internal:1;
#endif /* TAPI_VERSION4 */
#ifdef TAPI_VERSION3
   /** Generation on the local side.*/
   IFX_uint32_t local:1;
   /** Generation on the network side. */
   IFX_uint32_t network:1;
#endif /* TAPI_VERSION3 */
   /** Reserved */
   IFX_uint32_t reserved:6;
   /** Tone table index of the tone that has stopped being played out. */
   IFX_uint32_t index:8;
} IFX_TAPI_EVENT_DATA_TONE_GEN_t;

/** This structure contains data specific to the tone detection event.
    It is used by \ref IFX_TAPI_EVENT_DATA_t. */
typedef struct
{
#ifdef TAPI_VERSION4
   /** Detected from external (e.g. analog line, PCM bus, RTP in-band, etc.).*/
   IFX_uint32_t external:1;
   /** Detected from internal. */
   IFX_uint32_t internal:1;
#endif /* TAPI_VERSION4 */
#ifdef TAPI_VERSION3
   /** Detected on the local side.*/
   IFX_uint32_t local:1;
   /** Detected on the network side. */
   IFX_uint32_t network:1;
#endif /* TAPI_VERSION3 */
   /** Reserved */
   IFX_uint32_t reserved:6;
   /** Tone table index of the tone that has been detected. */
   IFX_uint32_t index:8;
} IFX_TAPI_EVENT_DATA_TONE_DET_t;

/** This structure contains data specific to the fax event. It is used by
    \ref IFX_TAPI_EVENT_DATA_t. */
typedef struct
{
#ifdef TAPI_VERSION4
   /** Detected from external (e.g. analog line, PCM bus, RTP in-band, etc.).*/
   IFX_uint32_t external:1;
   /** Detected from internal. */
   IFX_uint32_t internal:1;
#endif /* TAPI_VERSION4 */
#ifdef TAPI_VERSION3
   /** Detected on the local side.*/
   IFX_uint32_t local:1;
   /** Detected on the network side. */
   IFX_uint32_t network:1;
#endif /* TAPI_VERSION3 */
   /** Last event or not; only to be used in ioctl.*/
   IFX_uint32_t reserved:6;
   /** Fax or modem signal. */
   IFX_uint32_t signal:8;
} IFX_TAPI_EVENT_DATA_FAX_SIG_t;


/** This structure contains data specific to the command error event. */
typedef struct
{
   /** Firmware family identifier used to decode the reason field. */
   IFX_uint16_t fw_id;
   /** Reason given by the firmware for the command error. */
   IFX_uint16_t reason;
   /** Header of error command. */
   IFX_uint32_t command;
} IFX_TAPI_EVENT_DATA_CERR_t;

/** Event generated when the automatic calibration stops for an analog line.
    This event provides an indication of whether the calibration process was completed
    successfully or with an error. The enumeration value provides failure
    details (defined in the future; for example, IFX_TAPI_EVENT_DATA_CALIBRATION_ERROR_xx).
*/
typedef enum
{
   /** Calibration completed successfully. The calibration results are applied
       to the analog line. */
   IFX_TAPI_EVENT_CALIBRATION_SUCCESS        = 0,
   /** The calibration process returns without an error. The results are out of
       range. The TAPI overwrites the results with default values on the analog line.*/
   IFX_TAPI_EVENT_CALIBRATION_ERROR_RANGE    = 1,
   /** The calibration process failed because of a time-out. */
   IFX_TAPI_EVENT_CALIBRATION_ERROR_TIMEOUT  = 2
} IFX_TAPI_EVENT_CALIBRATION_t;

/** This structure describes the number of lost events. */
typedef struct
{
   /** Number of events lost. */
   IFX_uint32_t lost;
} IFX_TAPI_EVENT_DATA_EVT_FIFO_t;

/** Event generated when the metering transmission was stopped by calling
   \ref IFX_TAPI_METER_STOP and the last metering pulse is transmitted on the
   analog line. It is also generated when \ref IFX_TAPI_METER_BURST is
   called and non-periodic metering is started by \ref IFX_TAPI_METER_BURST.
*/
typedef enum {
   /** Metering finished successfully. */
   IFX_TAPI_EVENT_DATA_METERING_SUCCESS = 0
} IFX_TAPI_EVENT_METERING_t;

/** Event generated when new continuous measurement results are available
    to be read out by IFX_TAPI_CONTMEASUREMENT_GET. */
typedef enum
{
   IFX_TAPI_EVENT_DATA_CONTMEASUREMENT_SUCCESS = 0
} IFX_TAPI_EVENT_CONTMEASUREMENT_t;

#ifndef TAPI_DXY_DOC
/** This enum lists the possible values reported in the TAPI event
    \ref IFX_TAPI_EVENT_FXO_POLARITY. */
typedef enum
{
   /** Reversed polarity (ring more positive than tip). */
   IFX_TAPI_EVENT_DATA_FXO_POLARITY_REVERSED = 0,
   /** Normal polarity (tip more positive than ring). */
   IFX_TAPI_EVENT_DATA_FXO_POLARITY_NORMAL = 1
} IFX_TAPI_EVENT_DATA_FXO_POLARITY_t;
#endif /* #ifndef TAPI_DXY_DOC */

/** Structure that reports analog line capacitance measurement results. */
typedef struct
{
   /** Measured capacitance tip to ring [nF]. */
   IFX_uint32_t nCapacitance;
   /** Return code of the measurement.
       The measurement result is invalid in case the return code
       is not set to IFX_SUCCESS. */
   IFX_int32_t nReturnCode;
} IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_t;

/** Structure that reports line to ground capacitance measurement results. */
typedef struct
{
   /** Return code of the measurement.
       The measurement results are invalid in case the return code
       is not set to IFX_SUCCESS. */
   IFX_int32_t nReturnCode;
   /** Measured capacitance tip to ground [nF]. */
   IFX_uint32_t nCapT2G;
   /** Measured capacitance ring to ground [nF]. */
   IFX_uint32_t nCapR2G;
} IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_GND_t;

/** Structure that reports details of the error that occured on the socket
    used by the QOS driver. */
typedef struct
{
   /** Error code returned by the socket operation. */
   IFX_int32_t                         error_code;
   /** KPI channel number on which the error occured. */
   IFX_uint16_t                        kpi_channel;
} IFX_TAPI_EVENT_DATA_KPI_SOCKET_t;

#ifndef TAPI_DXY_DOC
/** Structure that reports additional information for internal hook events. */
typedef struct
{
   /** Time since last hook event [1/8 ms]. Times exceeding the value range
       will be represented by the maximum value of 0xFFFF. */
   IFX_uint16_t nTime;
} IFX_TAPI_EVENT_HOOK_INT_t;
#endif /* #ifndef TAPI_DXY_DOC */

/** Union for events that can be reported. */
typedef union
{
   /** Pulse digit information.*/
   IFX_TAPI_EVENT_DATA_PULSE_t         pulse;
   /** DTMF digit information. */
   IFX_TAPI_EVENT_DATA_DTMF_t          dtmf;
   /** Tone generation index. */
   IFX_TAPI_EVENT_DATA_TONE_GEN_t      tone_gen;
   /** Tone detection information. */
   IFX_TAPI_EVENT_DATA_TONE_DET_t      tone_det;
   /** Command error event details. */
   IFX_TAPI_EVENT_DATA_CERR_t          cerr;
   /** Synchronization details. */
   IFX_TAPI_EVENT_FAULT_HW_SYNC_t      hw_sync;
   /** Event FIFO details.*/
   IFX_TAPI_EVENT_DATA_EVT_FIFO_t      event_fifo;
   /** Analog line calibration process. */
   IFX_TAPI_EVENT_CALIBRATION_t        calibration;
   /** Metering event. */
   IFX_TAPI_EVENT_METERING_t           metering;
   /** Event generated when new continuous measurement results are available
    to be read out by IFX_TAPI_CONTMEASUREMENT_GET. */
   IFX_TAPI_EVENT_CONTMEASUREMENT_t    contmeasurement;
    /** Error line. */
   IFX_TAPI_ErrorLine_t               *error;
   /** Line mode change event information. */
   IFX_TAPI_LINE_MODE_t                linemode;
#ifndef TAPI_DXY_DOC
   /** FXO line polarity event details. */
   IFX_TAPI_EVENT_DATA_FXO_POLARITY_t  fxo_polarity;
#endif /* #ifndef TAPI_DXY_DOC */
   /** Analog line capacitance measurement results. */
   IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_t lcap;
   /** Capacitance to ground measurement results. */
   IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_GND_t cap2gnd;
#ifndef TAPI_DXY_DOC
   IFX_TAPI_EVENT_HOOK_INT_t           hook_int;
#endif /* #ifndef TAPI_DXY_DOC */

   TAPI_COMPATIBILITY_FILL(56);

   /** Reserved */
   IFX_uint16_t value;
} IFX_TAPI_EVENT_DATA_t;

/** This structure is reported by an 'EVENT_GET' ioctl. For event masking
 'EVENT_MASK' re-using IFX_TAPI_EVENT_t should be used. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** Channel where the event occurred or \ref IFX_TAPI_EVENT_ALL_CHANNELS to
       indicate 'any channel' and for events that cannot be assigned to a
       specific channel (such as hardware errors). */
   IFX_uint16_t ch;
   /** Module type. */
   IFX_TAPI_MODULE_TYPE_t module;
   /** Indicates that more TAPI events are waiting to be read out
       with the \ref IFX_TAPI_EVENT_GET ioctl. It has to be set to zero
       for \ref IFX_TAPI_EVENT_ENABLE or \ref IFX_TAPI_EVENT_DISABLE.  */
   IFX_uint16_t more;
   /** Reserved */
   IFX_uint16_t reserved;
   /** Event type and sub-type. */
   IFX_TAPI_EVENT_ID_t id;
   /** Specific data of individual event. */
   IFX_TAPI_EVENT_DATA_t data;
} IFX_TAPI_EVENT_t;

/** List entry of events to mask (enable/disable). This structure is used in
    \ref IFX_TAPI_EVENT_MULTI_t to enable or disable a single or multiple
    events due to one single TAPI call. The event types are listed in
    \ref IFX_TAPI_EVENT_ID_t. */
typedef struct
{
   /** Event type and sub-type. */
   IFX_TAPI_EVENT_ID_t id;
   /** Specific data of individual event (for the event type/subtype). */
   IFX_TAPI_EVENT_DATA_t data;
}IFX_TAPI_EVENT_ENTRY_t;

/** This structure is used to mask one or multiple events of the TAPI using
    \ref IFX_TAPI_EVENT_ENABLE or \ref IFX_TAPI_EVENT_DISABLE. */
typedef struct
{
   /** Device index of event. If set to IFX_TAPI_EVENT_ALL_DEVICES this
       information is global. All masked events are related to this device. */
   IFX_uint16_t dev;
   /** Channel information of event.
       If set to IFX_TAPI_EVENT_ALL_CHANNELS this information is global.
       All masked events are related to this module channel. */
   IFX_uint16_t ch;
#ifdef TAPI_VERSION4
   /** Module type. All masked events are related to this module type.*/
   IFX_TAPI_MODULE_TYPE_t module;
#endif /* TAPI_VERSION4 */
   /** Number of events in the pEvent. */
   IFX_uint16_t nCount;
   /** List of events to enable or disable;
       the amount of memory should be equal
       to (sizeof (IFX_TAPI_EVENT_ENTRY_t) * nCount). */
   IFX_TAPI_EVENT_ENTRY_t *pEvent;
} IFX_TAPI_EVENT_MULTI_t;

/**@}*/ /* TAPI_INTERFACE_EVENT */

/* ======================================================================= */
/* TAPI Analog Line Continuous Measurement Services, structures            */
/* (Group TAPI_INTERFACE_CONTMEASUREMENT)                                  */
/* ======================================================================= */
/** \addtogroup TAPI_INTERFACE_CONTMEASUREMENT */
/**@{*/

#ifdef TAPI_ONE_DEVNODE
/** Structure used to request or reset the results of the continuous analog
    line measurement process; used by \ref IFX_TAPI_CONTMEASUREMENT_REQ. */
typedef struct
{
   /** Device index */
   IFX_uint16_t dev;
   /** Channel index */
   IFX_uint16_t ch;
} IFX_TAPI_CONTMEASUREMENT_t;
#endif /* TAPI_ONE_DEVNODE */

/** Structure used to read out the results of the continuous analog line
    measurement process.
    For some parameters, the physical value has to be multiplied by the steps to yield the parameter value.
    For example: Voltage = nVLineWireRing/Steps = 1000(IFX_int16_t)/10(mV) = 100(V);
    used by \ref IFX_TAPI_CONTMEASUREMENT_GET.

    The values for the ring current and ring voltage are updated at the end
    of every ring burst period. When the system is switched to any operating
    mode other than RINGING, the last measured values are kept. The value of
    the ring voltage is calculated from the output of the ring voltage
    regulation. Therefore, it is identical to the programmed ring voltage
    if no ring voltage regulation is used. The ring voltage cannot be
    reported for external ringing but the ring current will be updated. */
typedef struct
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index                             \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel index                            \endtapiv4 */
   IFX_uint16_t ch;
   /** Line voltage on RING wire.
       Value of the actual line voltage;
       range (10 mV steps):
       - Minimum: -144 V
       - Maximum: +144 V */
   IFX_int16_t nVLineWireRing;
   /** Line voltage on TIP wire.
       Value of the actual line voltage;
       range (10 mV steps):
       - Minimum: -144 V
       - Maximum: +144 V */
   IFX_int16_t nVLineWireTip;
   /** Desired line voltage.
       Output voltage of DC regulation;
       range (10 mV steps):
       - Minimum: -144 V
       - Maximum: +144 V */
   IFX_int16_t nVLineDesired;
   /** Line current.
       Value of the actual line current;
       range (10 uA steps):
       - Minimum: -100 mA
       - Maximum: +100 mA */
   IFX_int16_t nILine;
   /** Line longitudinal current.
       Value of the actual longitudinal current;
       range (10 uA steps):
       - Minimum: -100 mA
       - Maximum: +100 mA */
   IFX_int16_t nILineLong;
   /** Line ring current.
       Value of the last ring burst current;
       absolute peak value within the last ring period.
       Range (10 uA steps):
       - Minimum: -100 mA
       - Maximum: +100 mA */
   IFX_int16_t nILineRingPeak;
   /** Line ring voltage.
       Value of the last ring burst voltage; peak value of VDAC sine.
       Range (10 mV steps):
       - Minimum: -144 V
       - Maximum: +144 V */
   IFX_int16_t nVLineRingPeak;
   /** TTX metering adaptation coefficient real part.
       Value of the last TTX adaptation;
       value does not have a physical unit.
       Additional user space libraries are needed for further calculations.
       Range (steps of 1):
       - Minimum: -32768
       - Maximum: +32767 */
   IFX_int16_t nTtxMeterReal;
   /** TTX metering adaptation coefficient imaginary part.
       Value of the last TTX adaptation;
       value does not have a physical unit.
       Additional user-space libraries are needed for further calculations.
       Range (steps of 1):
       - Minimum: -32768
       - Maximum: +32767 */
   IFX_uint16_t nTtxMeterImag;
   /** TTX burst length.
       Length of the last TTX burst;
       Range (1 ms steps):
       - Minimum: 0 s
       - Maximum: 10 s */
   IFX_uint16_t nTtxMeterLen;
   /** TTX Current.
       Value of the last TTX current;
       value does not have a physical unit.
       Additional user space libraries are needed for further calculations.
      Range (steps of 1):
       - Minimum: -32768
       - Maximum: +32767 */
   IFX_int16_t nITtxMeter;
   /** TTX Voltage.
       Value of the last TTX voltage;
       value does not have a physical unit.
       Additional user space libraries are needed for further calculations.
       Range (steps of 1):
       - Minimum: -32768
       - Maximum: +32767 */
   IFX_int16_t nVTtxMeter;
   /** Standard battery voltage (Vbath). The allowed parameter range depends
       on the used SLIC hardware; proposed TAPI range (10 mV steps):
       - Minimum: -144 V
       - Maximum: 0 V */
   IFX_int16_t nVBat;
   /** SLIC temperature (in Celsius). */
   IFX_int16_t nSlicTemp;
} IFX_TAPI_CONTMEASUREMENT_GET_t;


/**@}*/ /* TAPI_INTERFACE_CONTMEASUREMENT */


/* Type of single debug buffer entry */
enum TapiDebugBufferEntryType
{
   TAPI_DBUF_SPI_REG_WRITE,
   TAPI_DBUF_SPI_REG_READ,
   TAPI_DBUF_USER_INFO,
   /* <- Here new entry types can be added */
   TAPI_DBUF_ENTRY_TYPE_LAST
};


/* Single debug buffer entry */
struct TapiDebugBufferEntry
{
   /* Entry timestamp given in nanoseconds (or other system dependent unit) */
   unsigned long long int timestamp;
   /* Device number */
   IFX_uint8_t dev_nr;
   /* Channel number */
   IFX_uint8_t ch_nr;
   /* Entry type (e.g. SPI write, read, user info) */
   enum TapiDebugBufferEntryType type;
   /* Length of stored data in single entry (in bytes) */
   IFX_uint16_t data_length;
   /* Register address in SPI read/write. Part of SPI header, not data. */
   IFX_uint8_t reg_number;

   /* Maximum length of stored data in single entry (in bytes) */
   #define TAPI_DEBUG_BUFFER_ENTRY_DATA_SIZE 128
   /* Placeholder for entry data bytes */
   IFX_uint8_t data[TAPI_DEBUG_BUFFER_ENTRY_DATA_SIZE];
};


/* Information about place in memory to copy TAPI debug buffer into */
struct TapiDebugBufferContent
{
   /* Pointer to buffer to which data will be copied */
   IFX_uint8_t *data;
   /* Size of allocated buffer for data */
   IFX_size_t data_length;
   /* Number of copied entries */
   IFX_size_t copied_entries;
};


/* Structure holding debug buffer settings which can be read/written using 
   helper functions */
struct TapiDebugBufferConfig
{
   /* If true - new entries will overwrite first ones when buffer becomes full
      (buffer will act as ring buffer) otherwise new entries will be discarded */
   IFX_boolean_t overwrite;
   /* Print new entries to console */
   IFX_boolean_t print_to_console;
   /* Buffering paused - if true, no new entries are added */
   IFX_boolean_t paused;
};


#ifdef __cplusplus
   }
#endif

#endif  /* DRV_TAPI_IO_H */
