/******************************************************************************

  Copyright 2023 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#include <drv_tapi_config.h>

#ifdef TAPI_FEAT_KIOCTL

#include <ifx_types.h>

#ifdef LINUX
#ifdef __KERNEL__

#include "drv_tapi.h"
#include "drv_tapi_ioctl.h"
#include <drv_tapi_kio.h>
#include <drv_tapi_osmap.h>

#include <linux/fs.h>

/*
   Get functions normally used to access devices from user space
*/
extern int ifx_tapi_open(struct inode *inode, struct file *filp);
extern int ifx_tapi_release(struct inode *inode, struct file *filp);
extern long ifx_tapi_ioctl(struct file *filp, unsigned int nCmd,
                           unsigned long nArgument);

/* ============================= */
/* kernel ioctl api: structure   */
/* to reference a channel        */
/* (see drv_tapi_kio.h)          */
/* ============================= */
typedef struct
{
   /* storage for major and minor numbers */
   struct inode inode;
   /* storage for private date of real open procedure */
   struct file file;
} IFX_TAPI_Ch_KernRef_t;


/**
   Open API to be called from kernel space.

   \param name  Pointer to a dev node name (character string)
   \return IFX_TAPI_KIO_HANDLE  handler
*/
IFX_TAPI_KIO_HANDLE ifx_tapi_kopen(char *name)
{
   IFX_TAPI_Ch_KernRef_t *pRef = IFX_NULL;
   IFX_char_t *devname_substr = IFX_NULL;
   IFX_char_t *devnum_substr = IFX_NULL;
   IFX_char_t *dev_name = IFX_NULL;
   IFX_char_t *expected_dev_names[] = {"dxs"};
   IFX_uint_t i = 0;
   IFX_uint8_t ucMajor = 0;
   IFX_uint8_t ucMinor = 0;

   if (name == IFX_NULL)
   {
      return IFX_TAPI_KIO_FD_INVALID;
   }

   /* In parameter name expect a device node name like this: vmmc10.
      Split into name and minor-number and lookup the major via the
      device driver context structs. */
   for (i = 0; i < ARRAY_SIZE(expected_dev_names); ++i)
   {
      devname_substr = strstr(name, expected_dev_names[i]);
      if (devname_substr != IFX_NULL)
      {
         dev_name = expected_dev_names[i];
         break;
      }
   }

   if (dev_name == IFX_NULL)
   {
      /* device node name not found among expected names */
      return IFX_TAPI_KIO_FD_INVALID;
   }

   /* extract minor number from name parameter */
   devname_substr += strlen(dev_name);
   if (*devname_substr)
   {
      ucMinor = simple_strtol(devname_substr, &devnum_substr, 10);
   }

   /* find a proper major number */
   for (i = 0; i < TAPI_MAX_LL_DRIVERS; i++)
   {
      if (gHLDrvCtx[i].pDrvCtx != IFX_NULL &&
          strstr((gHLDrvCtx[i].pDrvCtx)->devNodeName, dev_name) != IFX_NULL)
      {
         ucMajor = (gHLDrvCtx[i].pDrvCtx)->majorNumber;
         break;
      }
   }

   if (i >= TAPI_MAX_LL_DRIVERS && !ucMajor)
   {
      /* major number not found */
      return IFX_TAPI_KIO_FD_INVALID;
   }

   pRef = TAPI_OS_Malloc(sizeof(*pRef));
   if (IFX_NULL == pRef)
   {
      /* malloc error */
      return IFX_TAPI_KIO_FD_INVALID;
   }

   memset(pRef, 0, sizeof(*pRef));

   pRef->inode.i_rdev = MKDEV(ucMajor, ucMinor);

   if (ifx_tapi_open(&pRef->inode, &pRef->file) != 0)
   {
      TAPI_OS_Free(pRef);
      return IFX_TAPI_KIO_FD_INVALID;
   }

   return (IFX_TAPI_KIO_HANDLE) pRef;
}


/**
   Close API to be called from kernel space.

   \param  handle      IFX_TAPI_KIO_HANDLE type

   \return
   0 - if no error,
   otherwise error code
*/
int ifx_tapi_kclose(IFX_TAPI_KIO_HANDLE handle)
{
   int ret = 0;
   IFX_TAPI_Ch_KernRef_t *pRef = (IFX_TAPI_Ch_KernRef_t *)handle;

   if (pRef == IFX_NULL)
   {
      return -ENODEV;
   }

   ret = ifx_tapi_release(&pRef->inode, &pRef->file);
   if (0 != ret)
      return ret;

   TAPI_OS_Free(pRef);

   return 0;
}


/**
   Ioctl API to be called from kernel space.

   \param handle   IFX_TAPI_KIO_HANDLE type
   \param command  Ioctl command
   \param arg      Ioctl argument

   \return
      IFX_SUCCESS or IFX_ERROR
*/
int ifx_tapi_kioctl(IFX_TAPI_KIO_HANDLE handle, unsigned int command, void *arg)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = IFX_NULL;
   IFX_TAPI_ioctlCtx_t ctx = {0};
   IFX_int32_t ret = -1;
   struct inode *inode = IFX_NULL;

   if (handle == IFX_TAPI_KIO_FD_INVALID)
      return IFX_ERROR;

   inode = &((IFX_TAPI_Ch_KernRef_t *)handle)->inode;

   switch (command) {
      case IFX_TAPI_CID_TX_SEQ_START:
         /*lint -fallthrough*/
      case IFX_TAPI_CID_TX_INFO_START:
         /*lint -fallthrough*/
      case IFX_TAPI_CID_CFG_SET:
         /*lint -fallthrough*/
      case IFX_TAPI_EVENT_MULTI_ENABLE:
         /*lint -fallthrough*/
      case IFX_TAPI_EVENT_MULTI_DISABLE:
         /*lint -fallthrough*/
      case IFX_TAPI_CAP_LIST:
         /* that services require nested copy from/to user-space,
            which are not supported for KIO interface */
         return IFX_ERROR;
      default:
         break;
   }

   /* get the device driver context */
   pDrvCtx = IFX_TAPI_DeviceDriverContextGet(MAJOR(inode->i_rdev));
   if (pDrvCtx == IFX_NULL)
   {
      return IFX_ERROR;
   }

   /* get the ioctl context: channel, device etc. */
   ret = TAPI_ioctlContextGet(pDrvCtx, MINOR(inode->i_rdev),
                              command, (IFX_ulong_t)arg, IFX_FALSE, &ctx);
   if (IFX_SUCCESS == ret)
   {
      IFX_boolean_t compatIoctl = IFX_FALSE;
      ret = TAPI_Ioctl(&ctx, &compatIoctl);
   }

   TAPI_ioctlContextPut((IFX_ulong_t)arg, &ctx);

   return ret;
}

EXPORT_SYMBOL(ifx_tapi_kopen);
EXPORT_SYMBOL(ifx_tapi_kclose);
EXPORT_SYMBOL(ifx_tapi_kioctl);

#endif /* __KERNEL__ */
#endif /* LINUX */
#endif /* TAPI_FEAT_KIOCTL */
