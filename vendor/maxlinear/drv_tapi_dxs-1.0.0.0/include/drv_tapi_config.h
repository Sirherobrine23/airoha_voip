#ifndef _DRV_TAPI_CONFIG_H
#define _DRV_TAPI_CONFIG_H
/******************************************************************************
  Copyright 2014      Lantiq Deutschland GmbH
  Copyright 2022-2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/


/**
   \file drv_tapi_config.h

   This file configures the features depending on user controlled defines and
   other defines.

   This is the main configuration file for the project.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#if defined(HAVE_CONFIG_H)
   #include "drv_config_defines.h"
   /*lint -save -e(19)  we need this ; to stop runaway arguments from breaking
     out of drv_tapi_autoconf.h */
   ;
   /*lint -restore */
#endif

#ifdef VXWORKS
   #include "prjParams.h"
#endif

#if defined(LINUX) && defined(__KERNEL__)
   #include <linux/kernel.h>
#endif

#include "drv_tapi_if_version.h"
#include "drv_config_tapi_compatibility.h"

/* ========================================================================== */
/*                    Global Configuration Definitions                        */
/* ========================================================================== */

#if defined(LINUX)
   #if defined(__KERNEL__)
      /* Standalone TAPI for DXS driver running in kernel space */
      #define TAPI_LINUX_KERNEL_SPACE
   #else
       /* Standalone TAPI for DXS driver running in user space */
      #define TAPI_LINUX_USER_SPACE
   #endif
#endif

#ifdef TAPI_USE_PROC
   #if defined(TAPI_LINUX_KERNEL_SPACE) && defined(CONFIG_PROC_FS)
      /* Support for the Linux procfs. */
      #define TAPI_FEAT_PROCFS
   #else
      #warning Not compiling for Linux kernel-space: procfs support disabled.
   #endif
#endif

#ifdef LINUX_SMP_SUPPORT
   #ifdef TAPI_LINUX_KERNEL_SPACE
      #define TAPI_FEAT_LINUX_SMP
   #else
      #warning Not compiling for Linux kernel-space: SMP support disabled.
   #endif
#endif

#ifdef TAPI_KERNEL_API_SUPPORT
   #ifdef TAPI_LINUX_KERNEL_SPACE
      /* Support for API to execute IOCTLs from Linux kernel-space. */
      #define TAPI_FEAT_KIOCTL
   #else
      #warning Not compiling for Linux kernel-space: Kernel API disabled.
   #endif
#endif

#if defined(TAPI_LINUX_KERNEL_SPACE) && defined(CONFIG_COMPAT)
   /* Support for translating IOCTLs on Linux in between 32-bit arch in user-space
   and 64-bit arch in kernel-space. */
   #define TAPI_FEAT_LX_COMPAT
#endif

#if defined(ENABLE_TAPI_DEBUG_BUFFER)
   /* Tapi debug buffer feature */
   #define TAPI_FEAT_DEBUG_BUFFER

   #ifndef TAPI_DEBUG_BUFFER_NUM_OF_ENTRIES
      /* Default number of TAPI debug buffer entries if don't set by the user */
      #define TAPI_DEBUG_BUFFER_NUM_OF_ENTRIES 1024
   #endif
#endif


/*
   TAPI basic POTS features switches.
   These should always be defined to provide basic driver and chip functionalities.
*/

#ifdef TAPI_DIAL
   /* Support for Pulse dial detection. */
   #define TAPI_FEAT_DIAL
#endif

/* Support for Pulse dial detection with the Hookstate Decoding Statemachine. */
#ifdef TAPI_HOOKSTATE
   #define TAPI_FEAT_DIAL
   #define TAPI_FEAT_DIALENGINE
#endif

#ifdef TAPI_PCM_SUPPORT
   /* Support for the PCM interface. */
   #define TAPI_FEAT_PCM
#endif

#ifdef TAPI_CALIBRATION
   /* Support for the Analog Line Calibration feature. */
   #define TAPI_FEAT_CALIBRATION
#endif


/*
   TAPI extended POTS features switches.
   These can be enabled (as a group or separately) to make use of advanced POTS
   features.
*/

/* Extended POTS features enabled as a group.
   User can also enable extended features selectively by defining
   particular symbol or enabling configure option. */
#ifdef TAPI_EXTENDED_POTS_FEATURES
   #define TAPI_DTMF
   #define TAPI_RING_ENGINE
   #define TAPI_TONE_GENERATOR_SUPPORT
   #define TAPI_CID
   #define TAPI_PHONE_DETECTION
   #define TAPI_RING_ADAPTIVE_BITTIME
#endif

#ifdef TAPI_DTMF
   /* Support for the DTMF features. */
   #define TAPI_FEAT_DTMF
#endif

#ifdef TAPI_RING_ENGINE
   /* Support for the Ring engine feature. */
   #define TAPI_FEAT_RINGENGINE
#endif

/* Support for tone generator features. */
#ifdef TAPI_TONE_GENERATOR_SUPPORT
   #define TAPI_FEAT_TONEGEN
   #define TAPI_FEAT_TONEENGINE
   #define TAPI_FEAT_TONETABLE
   #define DXS_FEAT_TONE_GENERATOR
#endif

/* Support for CID (Caller ID) features. */
#ifdef TAPI_CID
   #define TAPI_FEAT_CID
   #define TAPI_FEAT_TONEENGINE
   #define TAPI_FEAT_TONETABLE
   #define TAPI_FEAT_RINGENGINE
   /* Duslic XS support for CID */
   #define DXS_FEAT_CID
#endif

#ifdef TAPI_PHONE_DETECTION
   /* Support for the Phone Detection feature. */
   #define TAPI_FEAT_PHONE_DETECTION

   /* Enable capacity measurement features needed by PPD. */
   #define TAPI_FEAT_CAP_MEAS
   #define DXS_FEAT_CAPACITANCE_MEASUREMENT
   #define DXS_FEAT_CALIBRATION_STORAGE

   #ifdef TAPI_FEAT_PROCFS
      /* Support for the Phone Detection proc fs entries creation */
      #define TAPI_FEAT_PHONE_DETECTION_PROCFS
      #ifdef DEBUG
         #define TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
      #endif
   #endif
#endif

#ifdef TAPI_RING_ADAPTIVE_BITTIME
   /* Instead of fixed 50ms steps per bit of the ring cadence adapt the duration
   per step to the configured ring-period. */
   #define TAPI_FEAT_RING_ADAPTIVE_BITTIME
#endif


/*
   TAPI linetesting POTS features switches.
   These can be enabled (as a group or separately) to make use of
   POTS features for linetesting.
*/
#ifdef TAPI_LINETESTING_POTS_FEATURES
   #define TAPI_CONT_MEASUREMENT
   #define TAPI_CAPACITANCE_MEASUREMENT_SUPPORT
   #define TAPI_NLT
   #define TAPI_GR909
#endif

#ifdef TAPI_CONT_MEASUREMENT
   /* Support for the Analog Line Continuous Measurement feature. */
   #define TAPI_FEAT_CONT_MEAS
   #define DXS_FEAT_CONT_MEASUREMENT
#endif

#ifdef TAPI_CAPACITANCE_MEASUREMENT_SUPPORT
   /* Support for the Analog Line Capacitance Measurement feature. */
   #define TAPI_FEAT_CAP_MEAS
   /* Support handling of Analog Line Capacitance Measurement feature
      via IOCTLs. */
   #define TAPI_FEAT_CAP_MEAS_IOCTL
   #define DXS_FEAT_CAPACITANCE_MEASUREMENT
   #define DXS_FEAT_CALIBRATION_STORAGE
#endif

#ifdef TAPI_NLT
   /* Support for NLT (Network Line Testing) features. */
   #define TAPI_FEAT_NLT
   #define DXS_FEAT_NLT
#endif

#ifdef TAPI_GR909
   /* Support for the GR-909 line testing feature. */
   #define TAPI_FEAT_GR909
   #define DXS_FEAT_GR909
   #define DXS_FEAT_CALIBRATION_STORAGE
#endif


/*
   Rarely used TAPI extended POTS features switches.
*/

#ifdef TAPI_MWL
   /* Support for the MWL (Message Waiting Lamp) feature. */
   #define TAPI_FEAT_MWL
#endif

#ifdef TAPI_METERING
   /* Support for the TTX (TeleTax/Metering) feature. */
   #define TAPI_FEAT_METERING
   /* Support for the TTX (TeleTax/Metering) feature. */
   #define DXS_FEAT_METERING
#endif

/* The level of tones played in local direction is attenuated by the analog
   line attenuation. This setting compensates the levels of all generated
   tones including the FSK signal of CID sequences. As result the levels
   of the tones becomes independent of the attenuation in the BBD setting. */
#ifdef DXS_TONE_GENERATOR_LEVEL_COMPENSATION
   #define DXS_FEAT_TG_LEVEL_COMPENSATION
#endif

#ifdef TAPI_STRICT_PERMISSIONS
   #define TAPI_FEAT_IOCTL_CAPABILITY_CHECK
   #define TAPI_FEAT_PROCFS_CAPABILITY_CHECK
   #define TAPI_FEAT_PROCFS_STRICT_MODES

   #undef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
#endif

/* ==================================== */
/* Safety checks for TAPI settings      */
/* ==================================== */

#if !defined(TAPI_HAVE_TIMERS)
   /* All these features make no sense without timers.
      Deactivate them completely when no timers are available. */
   #undef TAPI_FEAT_CID
   #undef TAPI_FEAT_TONEGEN
   #undef TAPI_FEAT_TONEENGINE
   #undef TAPI_FEAT_DIALENGINE
   #undef TAPI_FEAT_RINGENGINE
   #undef TAPI_FEAT_METERING

   #warning "TAPI without timers: deactivated TAPI features depending on timers."
#endif


/* ========================================================================== */
/*                    Duslic XS specific configuration                        */
/* ========================================================================== */


/* DUSLIC XS device name */
#define DXS_DEV_NAME "duslicxs"

/* Wait time in [us] for the next DUSLIC XS access in case of polling register */
#define DXS_WAIT_POLLTIME 50

/* Time to wait until OBXML indicates data:
   DXS_OBXML_POLL_LOOP * DXS_WAIT_POLLTIME = poll time in us 
   This number defines repeat count. */
#define DXS_OBXML_POLL_LOOP 100

/* Loops until free inbox space is available
   time = DXS_FIBXMS_POLL_LOOP * DXS_WAIT_POLLTIME = poll time in us 
   This number defines repeat count. */
#define DXS_FIBXMS_POLL_LOOP 1000

/* Timeout in [ms] for mailbox empty event in ms */
#define DXS_WAIT_CB_EMPTY 100

/* Major device number for OS registration. */
#define DXS_MAJOR 125
/* Minor device number for OS registration. */
#define DXS_MINOR_BASE 10

/*
   Duslic XS specific features
*/

#ifdef TAPI_LINUX_KERNEL_SPACE
   /* Linux IRQ handling will be done by thread */
   #define DXS_FEAT_LINUX_THREADED_IRQ
#endif

#ifdef DXS_DIRECT_CHIP_ACCESS_SUPPORT
   #ifdef DEBUG
      #define DXS_FEAT_IOCTL_RW_CMD
      #define DXS_FEAT_IOCTL_RW_REG
   #else
      #warning "DXS direct chip access is available only with DEBUG flag set!"
   #endif
#endif /* DXS_DIRECT_CHIP_ACCESS_SUPPORT */


/*
   Duslic XS dev io support (not supported at the moment)
*/
/* #if (defined(IFXOS_USE_DEV_IO) && (IFXOS_USE_DEV_IO == 1))        */
/*     using IFXOS solution to enable usage of driver in user space  */
/*    #define DXS_USE_DEV_IO                                         */
/* #endif                                                            */


/* ==================================== */
/* Safety checks for Duslic XS settings */
/* ==================================== */

#if defined(TAPI_LINUX_USER_SPACE) && defined(DXS_HAVE_INTERRUPTS)
   #error interrupt handling not supported for linux user space
#endif

#endif /* _DRV_TAPI_CONFIG_H */
