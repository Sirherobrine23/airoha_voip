#ifndef DRV_TAPI_LINUX_IOCTL_H
#define DRV_TAPI_LINUX_IOCTL_H

/******************************************************************************

  Copyright 2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#include "drv_tapi_config.h"

long ifx_tapi_ioctl(struct file *filp, unsigned int nCmd, unsigned long nArg);

#ifdef TAPI_FEAT_LX_COMPAT
   long ifx_tapi_compat_ioctl(struct file *filp, unsigned int nCmd,
                              unsigned long nArgument);
#endif /* TAPI_FEAT_LX_COMPAT */

#endif /* DRV_TAPI_LINUX_IOCTL_H */