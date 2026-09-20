#ifndef _DRV_DXS_BBD_H
#define _DRV_DXS_BBD_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_bbd.h
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "lib_bbd.h"
#ifdef TAPI_FEAT_LX_COMPAT
#include "drv_dxs_io_types_32.h"
#endif
/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
extern IFX_int32_t   DXS_BBD_Download     (DXS_CHANNEL_t *pCh,
                                           DXS_BBD_Download_t *pBBD);

#ifdef TAPI_FEAT_LX_COMPAT
extern IFX_int32_t   DXS_BBD_Download_32  (DXS_CHANNEL_t *pCh,
                                           DXS_BBD_Download_32_t *pBBD);
#endif

extern enum DXS_DcDcType DXS_BBD_DcDcStringTranslate(const char *string);
#endif /* _DRV_DXS_BBD_H */
