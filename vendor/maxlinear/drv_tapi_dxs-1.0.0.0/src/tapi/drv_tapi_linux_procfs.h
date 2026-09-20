#ifndef DRV_TAPI_LINUX_PROCFS_H
#define DRV_TAPI_LINUX_PROCFS_H
/******************************************************************************

  Copyright 2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_linux.h
   This file contains the declaration for common types of High-Level TAPI Driver,
   Linux proc fs specific part.
*/

#ifdef LINUX
#ifdef __KERNEL__

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_tapi_config.h"

#ifdef TAPI_FEAT_PROCFS

#include <ifx_types.h>

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>

/* ============================= */
/* Macro definitions             */
/* ============================= */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0))
   #define PDE_DATA(inode) PDE(inode)->data
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(5,17,0))
   #define PDE_DATA pde_data
#endif

/* ======================================================= */
/* Common procfs related function definitions              */
/* ======================================================= */

IFX_int32_t proc_EntriesInstall(void);
IFX_void_t proc_EntriesRemove(void);

#ifdef TAPI_FEAT_PHONE_DETECTION_PROCFS_WRITE
   IFX_void_t tapi_proc_ppd_read_help(struct seq_file *s);
#endif

#endif /* TAPI_FEAT_PROCFS */
#endif /* LINUX */
#endif /* __KERNEL__ */

#endif /* DRV_TAPI_LINUX_PROCFS_H */
