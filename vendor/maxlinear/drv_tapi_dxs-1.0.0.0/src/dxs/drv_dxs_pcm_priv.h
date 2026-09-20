#ifndef _DRV_DXS_PCM_PRIV_H
#define _DRV_DXS_PCM_PRIV_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_pcm_priv.h
   This file contains the defines, the structures declarations for PCM module.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_fw_cmd_eop.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* Structure for the PCM channel including firmware message cache */
struct DXS_PCM
{
   DXS_PCM_CH_CTRL_t    fw_pcm_ch;
   DXS_PCM_CH_MUTE_t    pcm_ch_mute;
};

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

#endif /* _DRV_DXS_PCM_PRIV_H */
