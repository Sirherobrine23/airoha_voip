#ifndef _IFXOS_INTERRUPT_H
#define _IFXOS_INTERRUPT_H
/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/** \file
   This file contains definitions for interupt handling within driver.
*/

/** \defgroup IFXOS_INTERRUPT Interrupt Sub System.

   This Group contains the interrupt sub system definitions and function. 

   The functions are available for driver code.

\ingroup IFXOS_INTERFACE
*/

#ifdef __cplusplus
   extern "C" {
#endif

/* ============================================================================
   IFX OS adaptation - Includes
   ========================================================================= */
#include "linux/ifxos_linux_interrupt.h"
#include <ifx_types.h>

/* ============================================================================
   IFX OS adaptation - interrupt handling, typedefs
   ========================================================================= */
/** \addtogroup IFXOS_INTERRUPT
@{ */

/* ============================================================================
   IFX OS adaptation - interrupt handling, functions
   ========================================================================= */


/** @} */

#ifdef __cplusplus
}
#endif

#endif      /* #ifndef _IFXOS_INTERRUPT_H */




