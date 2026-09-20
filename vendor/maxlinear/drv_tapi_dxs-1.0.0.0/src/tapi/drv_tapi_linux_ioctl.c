#include "drv_tapi.h"
#include "drv_tapi_api.h"
#include "drv_tapi_ioctl.h"
#include "drv_tapi_ll_interface.h"

#include "drv_tapi_linux_ioctl.h"

#include <ifx_types.h>

/**
   Configuration / Control for the device.

   \param  inode        Pointer to the inode.
   \param  nCmd         IOCTL identifier.
   \param  nArg         Optional argument.

   \return
   0 and positive values - success,
   negative value - ioctl failed
   \remarks
   This function does the following functions:
      - If the ioctl command is device specific, low-level driver's ioctl function
      - If the ioctl command is TAPI specific, it is handled at this level
*/
static long ifx_tapi_ioctl_base(struct inode *inode, unsigned int nCmd,
                                unsigned long nArg, IFX_boolean_t *bCompatIoctl)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = IFX_NULL;
   IFX_TAPI_ioctlCtx_t ctx = {0};
   IFX_int32_t ret = TAPI_statusErr;

#ifdef TAPI_FEAT_LX_COMPAT
   TAPI_UNUSED(bCompatIoctl);
#endif

#if defined(TAPI_FEAT_IOCTL_CAPABILITY_CHECK)
   if (!capable(CAP_SYS_PACCT))
   {
      return -EACCES;
   }
#endif

   /* get the device driver context */
   pDrvCtx = IFX_TAPI_DeviceDriverContextGet(MAJOR(inode->i_rdev));
   if (pDrvCtx == IFX_NULL)
   {
      return IFX_ERROR;
   }

   /* get the ioctl context: channel, device etc. */
   ret = TAPI_ioctlContextGet(pDrvCtx, MINOR(inode->i_rdev),
                              nCmd, nArg, IFX_TRUE, &ctx);
   if (IFX_SUCCESS == ret)
   {
      ret = TAPI_Ioctl (&ctx, bCompatIoctl);
   }

   TAPI_ioctlContextPut (nArg, &ctx);

   return ret;
}


/**
   Configuration / Control for the device.

   \param  filp         Pointer to the file descriptor.
   \param  nCmd         IOCTL identifier.
   \param  nArg         Optional argument.

   \return
   0 and positive values - success,
   negative value - ioctl failed
   \remarks
   This function does the following functions:
      - If the ioctl command is device specific, low-level driver's ioctl function
      - If the ioctl command is TAPI specific, it is handled at this level
*/
long ifx_tapi_ioctl(struct file *filp,
                    unsigned int nCmd, unsigned long nArg)
{
   IFX_boolean_t compatIoctl = IFX_FALSE;
   IFX_int32_t ret = TAPI_statusErr;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0))
   struct inode *inode = file_inode(filp);
#else
   struct inode *inode = filp->f_dentry->d_inode;
#endif

   TAPI_UNUSED(filp);
   ret = ifx_tapi_ioctl_base(inode, nCmd, nArg, &compatIoctl);

   return ret;
}


#ifdef TAPI_FEAT_LX_COMPAT
/**
   Configuration / Control for the device for compat mode with mixed
   32 bit userspace app and 64 bit kernel.

   \param  inode        Pointer to the inode.
   \param  filp         Pointer to the file descriptor.
   \param  nCmd         IOCTL identifier.
   \param  nArg         Optional argument.

   \return
   0 and positive values - success,
   negative value - ioctl failed
   \remarks
   This function does the following functions:
      - If the ioctl command is device specific, low-level driver's ioctl function
      - If the ioctl command is TAPI specific, it is handled at this level
*/
long ifx_tapi_compat_ioctl(struct file *filp, unsigned int nCmd,
                           unsigned long nArg)
{
   IFX_int32_t ret = TAPI_statusErr;
   IFX_boolean_t compatIoctl = IFX_TRUE;
   
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0))
   struct inode *inode = file_inode(filp);
#else
   struct inode *inode = filp->f_dentry->d_inode;
#endif

   TAPI_UNUSED(filp);
   ret = ifx_tapi_ioctl_base(inode, nCmd, nArg, &compatIoctl);

   return ret;
}
#endif /* TAPI_FEAT_LX_COMPAT */
