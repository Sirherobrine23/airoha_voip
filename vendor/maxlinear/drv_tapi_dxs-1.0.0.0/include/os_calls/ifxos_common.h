#ifndef _IFXOS_COMMON_H
#define _IFXOS_COMMON_H
/******************************************************************************

  Copyright 2009       Lantiq Deutschland GmbH
  Copyright 2020       Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/** \file
   This file contains common OS independent definitions for the IFX OS.
*/

/** \defgroup IFXOS_INTERFACE IFXOS Interface Layer

   This Group contains the definitions and functions of the Infineon / Lantiq OS Layer.

   Therefore the IFX OS defines and provides wrapper functions to hide the
   OS specific implementations.

   Currently supported:
   - eCos
   - Linux
   - VxWorks
*/

/** \defgroup IFXOS_LAYER_LINUX IFXOS Implementation (Linux)

   This Group contains the mapping and implementation layer for the Linux OS.

   Therefore the Linux Layer is splitted in 2 parts:
   - Linux Kernel Layer
   - Linux User Layer
*/

/** \defgroup IFXOS_LAYER_VXWORKS IFXOS Implementation (VxWorks)

   This Group contains the mapping and implementation layer for the VxWorks OS.

   The VxWorks OS doesn't enforce a clear split between driver and
   application code, but for a clear structure and compatibility to Linux the
   IFX OS Layer keeps this border between driver and application code.

\note
   Concerning to this split, there are functions which are related to driver code
*/

/** \defgroup IFXOS_COMMON IFXOS Common Defines

   This Group contains the common OS independent definition.
*/

#ifdef __cplusplus
   extern "C" {
#endif

#define TAPI_OS_CHECKPOINT_BODY(name)                                           \
   do {                                                                         \
      printk(KERN_DEBUG #name"_OS_CHECKPOINT::Function:%s,Line:%d\n",           \
             __FUNCTION__, __LINE__);                                           \
   } while(0);

#ifdef TAPI_OS_CHECKPOINT_ENABLED
   #define TAPI_OS_CHECKPOINT TAPI_OS_CHECKPOINT_BODY(TAPI)
#else
   #define TAPI_OS_CHECKPOINT /* noop */
#endif

/* ==========================================================================
   IFXOS Version
   ========================================================================== */

/* get the IFXOS version definitons */
#include "ifxos_version.h"

/* ============================================================================
   IFX OS adaptation - Debug and Error Printout
   ========================================================================= */
/** \addtogroup IFXOS_COMMON
@{ */

#ifndef IFXOS_DEBUG_USR_PRINT
   /** defines the Debug Printout (User Level) */
#  define IFXOS_DEBUG_USR_PRINT                    1
#endif

#ifndef IFXOS_DEBUG_INT_PRINT
   /** defines the Debug Printout (Interrupt Level) */
#  define IFXOS_DEBUG_INT_PRINT                    1
#endif

#ifndef IFXOS_ERROR_USR_PRINT
   /** defines the Error Printout (User Level) */
#  define IFXOS_ERROR_USR_PRINT                    1
#endif

#ifndef IFXOS_ERROR_INT_PRINT
   /** defines the Error Printout (Interrupt Level) */
#  define IFXOS_ERROR_INT_PRINT                    1
#endif


/* ============================================================================
   IFX OS adaptation - Endianess
   ========================================================================= */

#ifndef IFXOS_LITTLE_ENDIAN
   /**
      Definition for the LITTLE endianess handling.
      To be OS independent set an own definition.
   */
#  define IFXOS_LITTLE_ENDIAN                            1234
#endif

#ifndef IFXOS_BIG_ENDIAN
   /**
      Definition for the BIG endianess handling.
      To be OS independent set an own definition.
   */
#  define IFXOS_BIG_ENDIAN                               4321
#endif

/* ============================================================================
   IFX OS adaptation - includes
   ========================================================================= */
#include "linux/ifxos_linux_common.h"


/* ============================================================================
   IFX OS adaptation - Misc
   ========================================================================= */

/**
   The function call will wait forever until the event occur.
*/
#define IFXOS_WAIT_FOREVER    0xFFFFFFFF

/**
   Don't wait at all. The function will check the condition and will return
   immediately.
*/
#define IFXOS_NO_WAIT         0x00000000
/** @} */

#ifdef __cplusplus
}
#endif

#endif      /* #ifndef _IFXOS_COMMON_H */

