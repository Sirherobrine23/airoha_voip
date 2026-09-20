#ifndef _DRV_TAPI_PPD_H
#define _DRV_TAPI_PPD_H
/******************************************************************************

                            Copyright (c) 2011-2015
                        Lantiq Beteiligungs-GmbH & Co.KG
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

/**
   \file drv_tapi_ppd.h
   This file contains the declaration of the Phone Detection state machine.
*/

#include "ifx_types.h"

#ifdef TAPI_FEAT_PHONE_DETECTION

#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS
   #include <linux/proc_fs.h>          /*proc-file system*/
   #include <linux/seq_file.h>
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS */

extern IFX_int32_t IFX_TAPI_PPD_Initialise_Unprot(
                        TAPI_CHANNEL *pChannel);

extern IFX_void_t IFX_TAPI_PPD_Cleanup(
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t IFX_TAPI_PPD_HandleLineFeeding(
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_LINE_MODE_t *pMode);

extern IFX_int32_t IFX_TAPI_PPD_ServiceStart(
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t IFX_TAPI_PPD_ServiceStop(
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t IFX_TAPI_PPD_HandleEvent(
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_EVENT_t *pEvent);

extern IFX_int32_t IFX_TAPI_PPD_Cfg_Get(
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_LINE_PHONE_DETECT_CFG_t *pPpdConf);

extern IFX_int32_t IFX_TAPI_PPD_Cfg_Set(
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_LINE_PHONE_DETECT_CFG_t const *pPpdConf);

#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS
extern IFX_void_t IFX_TAPI_PPD_proc_read(
                        struct seq_file *s,
                        const TAPI_CHANNEL *pChannel,
                        IFX_uint32_t nDev);
#endif /* TAPI_FEAT_PHONE_DETECTION_PROCFS */

extern IFX_boolean_t IFX_TAPI_PPD_DisableEvent(
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t IFX_TAPI_PPD_DisPhoneDetOnCh(
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t IFX_TAPI_PPD_EnPhoneDetOnCh(
                        TAPI_CHANNEL *pChannel);
#endif /* TAPI_FEAT_PHONE_DETECTION */

#ifdef TAPI_FEAT_CAP_MEAS_IOCTL
extern IFX_int32_t IOCTL_TAPI_PPD_CapMeasStop(
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t IOCTL_TAPI_PPD_NLTCapMeasStop(
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_NLT_CAPACITANCE_STOP_t *pArg);

extern IFX_int32_t IOCTL_TAPI_PPD_CapMeasStart(
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t IOCTL_TAPI_PPD_NLTCapMeasStart(
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_NLT_CAPACITANCE_START_t *pArg);
#endif /* TAPI_FEAT_CAP_MEAS_IOCTL */

#endif /* _DRV_TAPI_PPD_H */
