#ifndef _DRV_DXS_DEV_IO_H
#define _DRV_DXS_DEV_IO_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
   Description : This file contains the includes and the defines for running
                 driver in the user space.
   ========================================================================= */

/* ============================================================================
   Includes
   ========================================================================= */
#include <ifx_types.h>

/* ========================================================================== */
/*                           Global variables                                 */
/* ========================================================================== */

extern IFX_uint32_t DEVIO_debug_level;
extern IFX_char_t *dcdc_type;

/* ============================================================================
   Global Function (DEVIO) - declarations
   ========================================================================= */

extern int DXS_ModuleCreate(void);
extern int DXS_ModuleDelete(void);

#ifdef __cplusplus
}
#endif

#endif /* _DRV_DXS_DEV_IO_H */
