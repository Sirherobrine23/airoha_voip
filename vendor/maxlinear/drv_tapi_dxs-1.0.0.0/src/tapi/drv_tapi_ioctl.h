#ifndef DRV_TAPI_IOCTL_H
#define DRV_TAPI_IOCTL_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

 *****************************************************************************
   \file
   \remarks
 *******************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#include <ifx_types.h>

/* ============================= */
/* Global type definition        */
/* ============================= */

typedef struct TAPI_IOCtl_s TAPI_IOCtl_t;

/** IOCTL handler context dependence */
typedef enum {
   /* context independent */
   TAPI_IOC_CTX_NONE = 0,
   /* low-level driver context dependent */
   TAPI_IOC_CTX_DRV,
   /* device context dependent */
   TAPI_IOC_CTX_DEV,
   /* channel context dependent */
   TAPI_IOC_CTX_CH
} TAPI_CtxType_t;

/** Kind of IOCtl arguments */
typedef enum {
   /* no argument */
   TAPI_ARG_NONE = 0,
   /* simple integer */
   TAPI_ARG_INTEGER,
   /* pointer to the memory */
   TAPI_ARG_POINTER,
} TAPI_ArgKind_t;

typedef struct
{
   /* Minor number of device node */
   IFX_uint32_t nMinor;
   /* IOCTL command */
   IFX_uint32_t nCmd;
   /** IOCTL read request detected */
   IFX_boolean_t bRead;
   /** IOCTL write request detected */
   IFX_boolean_t bWrite;
   /* IOCTL argument */
   IFX_ulong_t nArg;
   /* IOCTL argument require copy from/to user space */
   IFX_boolean_t bArgUsrCpy;
   /** IOCTL Argument size if available, otherwise 0. The parameter size is
       given by the IOCTL definition, or calculated for some IOCTLs */
   IFX_uint32_t nArgSize;
   /** detected IOCTL argument type */
   TAPI_ArgKind_t nArgKind;
   /** handle to the TAPI channel
       \note: by channel can be reached device and ll-driver handle */
   TAPI_CHANNEL *pTapiCh;
   /** detected IOCTL context type */
   TAPI_CtxType_t nContext;
   /*  Handle to the IOCTL descriptor */
   const TAPI_IOCtl_t *pIOCtl;
} IFX_TAPI_ioctlCtx_t;

/* ============================= */
/* Global function declaration   */
/* ============================= */

extern IFX_int32_t  TAPI_Ioctl (IFX_TAPI_ioctlCtx_t *pCtx,
   IFX_boolean_t *bCompatIoctl);

extern IFX_return_t TAPI_ioctlContextGet (
   IFX_TAPI_DRV_CTX_t *pDrvCtx,
   IFX_uint32_t nMinor,
   IFX_uint32_t iocmd,
   IFX_ulong_t ioarg,
   IFX_boolean_t bArgUsrCpy,
   IFX_TAPI_ioctlCtx_t *pCtx);

IFX_void_t TAPI_ioctlContextPut (
   IFX_ulong_t ioarg,
   IFX_TAPI_ioctlCtx_t *pCtx);

#endif /* DRV_TAPI_IOCTL_H */

