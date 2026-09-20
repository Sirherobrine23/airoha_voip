#ifndef _DRV_TAPI_API_H
#define _DRV_TAPI_API_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_api.h
   Internal functional API of the driver.
*/

/* ============================= */
/* includes                      */
/* ============================= */

#ifdef TAPI_FEAT_DEBUG_BUFFER
   #include "drv_tapi_debug_buffer.h"
#endif

/* ============================= */
/* Global defs                   */
/* ============================= */
#ifndef DRV_TAPI_NAME
   #ifdef LINUX
      /** device name */
      #define DRV_TAPI_NAME          "tapi_dxs"
   #else
      /** device name */
      #define DRV_TAPI_NAME          "/dev/tapi_dxs"
   #endif
#else
   #error TAPI module name already specified
#endif

/** mark variable as unused, to suppress compilation warnings

   \remarks: (ANSI X3.159-1989)
      void is used, in any context where the value of an expression
      is to be discarded, to indicate explicitly that a value is
      ignored by writing the cast (void).
*/
#define TAPI_UNUSED(var) ((IFX_void_t)(var))

#endif
