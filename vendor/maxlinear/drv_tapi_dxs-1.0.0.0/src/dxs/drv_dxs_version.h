#ifndef _DRV_DXS_VERSION_H
#define _DRV_DXS_VERSION_H

/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2019 Intel Corporation.
  Copyright (c) 2020-2023 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#include "drv_dxs_ll_if_version.h"

#define DXS_MAJORSTEP    1
#define DXS_MINORSTEP    10
#define DXS_VERSIONSTEP  0
#define DXS_VERS_TYPE    0

/* Minimum required FW version for DXS V1.1 */
#define DXS_V11_MIN_FW_DIGIT1  1
#define DXS_V11_MIN_FW_DIGIT2  4
#define DXS_V11_MIN_FW_DIGIT3  7

/* Minimum required FW version for DXS1 (1 channel) V1.2 */
#define DXS1_V12_MIN_FW_DIGIT1 2
#define DXS1_V12_MIN_FW_DIGIT2 1
#define DXS1_V12_MIN_FW_DIGIT3 9

/* Minimum required FW version for DXS2 (2 channel) V1.2 */
#define DXS2_V12_MIN_FW_DIGIT1 2
#define DXS2_V12_MIN_FW_DIGIT2 1
#define DXS2_V12_MIN_FW_DIGIT3 9

#endif /* _DRV_DXS_VERSION_H */
