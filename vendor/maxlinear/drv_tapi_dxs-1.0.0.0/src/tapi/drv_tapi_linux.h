#ifndef DRV_TAPI_LINUX_H
#define DRV_TAPI_LINUX_H
/******************************************************************************

  Copyright 2023          MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#include <ifx_types.h>

/* ============================= */
/* Global function declaration   */
/* ============================= */
int ifx_tapi_module_init(void);
void ifx_tapi_module_exit(void);

int ifx_tapi_open (struct inode *inode, struct file *filp);
int ifx_tapi_release(struct inode *inode, struct file *filp);
long ifx_tapi_ioctl(struct file *filp, unsigned int nCmd, unsigned long nArg);
IFX_uint32_t ifx_tapi_poll(struct file *filp, poll_table *wait);

#endif /* DRV_TAPI_LINUX_H */
