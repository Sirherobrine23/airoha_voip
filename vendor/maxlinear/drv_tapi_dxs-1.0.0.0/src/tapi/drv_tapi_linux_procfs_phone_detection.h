#ifndef DRV_TAPI_LINUX_PROCFS_PHONE_DETECTION_H
#define DRV_TAPI_LINUX_PROCFS_PHONE_DETECTION_H

/******************************************************************************

  Copyright 2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#include "drv_tapi_config.h"

#include <linux/seq_file.h>


#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS
   int tapi_proc_ppd_read_status(struct seq_file *s, int pos);
#endif

#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
   IFX_int32_t TAPI_ProcPpdDeviceEntryInstall(TAPI_DEV *pTapiDev);
   IFX_void_t TAPI_ProcPpdDeviceEntryRemove(TAPI_DEV *pTapiDev);
#endif

#endif /* DRV_TAPI_LINUX_PROCFS_PHONE_DETECTION_H */