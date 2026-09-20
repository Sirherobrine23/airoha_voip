#ifndef _DRV_TAPI_VERSION_H
#define _DRV_TAPI_VERSION_H
/******************************************************************************

  Copyright 2001-2009 Infineon Technologies AG
  Copyright 2009-2015 Lantiq Deutschland GmbH
  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2020 Intel Corporation.
  Copyright 2020-2023 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#include "drv_tapi_ll_if_version.h"

/** driver version, major number */
#define DRV_TAPI_VER_MAJOR 4
/** driver version, minor number */
#define DRV_TAPI_VER_MINOR 26
/** driver version, build number */
#define DRV_TAPI_VER_STEP 0
/** driver version, package type */
#define DRV_TAPI_VER_TYPE 0

#define IFX_TAPI_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

#endif /* _DRV_TAPI_VERSION_H */
