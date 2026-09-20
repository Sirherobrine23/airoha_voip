#ifndef _DRV_TAPI_DXS_VERSION_H
#define _DRV_TAPI_DXS_VERSION_H

/******************************************************************************

  Copyright 2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/** driver version, major number */
#define TAPI_DXS_MAJORSTEP    1
/** driver version, minor number */
#define TAPI_DXS_MINORSTEP    0
/** driver version, build number */
#define TAPI_DXS_VERSIONSTEP  0
/** driver version, package type */
#define TAPI_DXS_VERS_TYPE    0

#define IFX_TAPI_DXS_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

#endif /* _DRV_TAPI_DXS_VERSION_H */
