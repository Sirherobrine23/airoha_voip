#ifndef _DRV_TAPI_DEV_IO_H
#define _DRV_TAPI_DEV_IO_H

/******************************************************************************

  Copyright 2014 Lantiq Deutschland GmbH
  Copyright 2023 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#ifdef __cplusplus
   extern "C" {
#endif

/* ============================================================================
   Description : This file contains the includes and the defines
                 specific to the vxWorks OS.
   Remarks     : Please use the compiler switches here if you have
                 more than one OS.
   ========================================================================= */

/* ============================================================================
   Global Function (DEVIO) - declarations
   ========================================================================= */

extern int TAPI_ModuleCreate(void);
extern int TAPI_ModuleDelete(void);

#ifdef __cplusplus
   }
#endif

#endif /* _DRV_TAPI_DEV_IO_H */
