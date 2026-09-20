/******************************************************************************

  Copyright 2014       Lantiq Deutschland GmbH
  Copyright 2020       Intel Corporation.
  Copyright 2022-2023  MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_ioctl.c
   This file contains the implementation of the TAPI ioctl handling.
*/
/* ============================= */
/* Includes                      */
/* ============================= */

#ifdef EVENT_LOGGER_DEBUG
   #ifdef LINUX
      #include <asm/ioctl.h>
   #endif
   #ifdef VXWORKS
      #include "ioctl.h"
   #endif
#endif /* EVENT_LOGGER_DEBUG */

#if (defined(IFXOS_USE_DEV_IO) && (IFXOS_USE_DEV_IO == 1))
   #include "drv_tapi_osmap_local.h"
   #ifdef LINUX
       #include <asm/ioctl.h> /** \todo Fix it */
   #endif
#endif

#include "drv_tapi.h"
#include "drv_tapi_errno.h"

#include "drv_tapi_cid.h"
#include "drv_tapi_ppd.h"

#include "drv_tapi_ioctl.h"
#include "drv_tapi_compat.h"


#include "drv_tapi_debug_buffer.h"

#define TAPI_DECLARE_IOCTL_HANDLERS_MAP
#include "drv_tapi_ioctl_handlers.h"

#include <drv_tapi_config.h>

/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */
/* ============================= */
/* Local type definition         */
/* ============================= */

/** states of IOCTL logging */
typedef enum
{
   /** before ioctl handling */
   TAPI_IOCTL_PRELOG,
   /** after ioctl handling */
   TAPI_IOCTL_POSTLOG
} TAPI_IOCTL_LOG_t;

#ifdef TAPI_ONE_DEVNODE
/** Generic structure for channel based ioctl arguments. */
typedef struct
{
   /** Device index */
   IFX_uint16_t dev;
   /** Channel "module" index */
   IFX_uint16_t ch;
   /** Any parameter used by ioctls */
   IFX_uint32_t param;
} TAPI_CH_IO_ARG_t;

/** Generic structure for device based ioctl arguments. */
typedef struct
{
   /** Device index */
   IFX_uint16_t dev;
   /** Any parameter used by ioctls */
   IFX_uint32_t param;
} TAPI_DEV_IO_ARG_t;
#endif /* TAPI_ONE_DEVNODE */

/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Local function declaration    */
/* ============================= */

static IFX_int32_t tapi_Dev_Spec_Ioctl (IFX_TAPI_ioctlCtx_t* pCtx,
                         IFX_boolean_t *bCompatIoctl);
static IFX_int32_t tapi_Spec_Ioctl(IFX_TAPI_ioctlCtx_t *pCtx,
                                   const IFX_boolean_t *bCompatIoctl);

/* ============================= */
/* Local variable definition     */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */

#ifdef EVENT_LOGGER_DEBUG
static IFX_void_t tapi_IoctlLog (
   IFX_TAPI_ioctlCtx_t *pCtx,
   IFX_int32_t err,
   TAPI_IOCTL_LOG_t mode)
{
   IFX_uint32_t nDevID = 0, nCh = 0;

   nCh = pCtx->pTapiCh->nChannel;
   nDevID = pCtx->pTapiCh->pTapiDevice->nDevID;

   switch (mode)
   {
      case TAPI_IOCTL_PRELOG:
         /* log read ioctl before it is executed, so that we see it in case
          * of failure */
         if (IFX_TRUE == pCtx->bRead && IFX_FALSE == pCtx->bWrite)
         {
            /* _IOR logged here */
            LOG_RD_IOCTL(nDevID, nCh, pCtx->nCmd, IFX_NULL, 0, err);
         }
         else if (IFX_TRUE == pCtx->bWrite)
         {
            /* _IOWR and _IOW logged here */
            IFX_void_t *pArg = IFX_NULL;

            if (pCtx->nArgSize > sizeof (IFX_uint32_t) || pCtx->nArgKind ==
               TAPI_ARG_POINTER)
            {
               /* we are sure that passed structure */
               pArg = (IFX_void_t *)pCtx->nArg;
            }
            else if (pCtx->nArgSize > 0)
            {
               /* simple integer in argument,
                  or small structure (should not be) */
               pArg = (IFX_void_t *)&pCtx->nArg;
            }

            LOG_WR_IOCTL(nDevID, nCh, pCtx->nCmd, pArg, pCtx->nArgSize, err);
         }
         else
         {
            LOG_WR_IOCTL (nDevID, nCh, pCtx->nCmd, IFX_NULL, 0, err);
         }

         break;
      case TAPI_IOCTL_POSTLOG:
         if (IFX_FALSE == pCtx->bRead)
            break;

         /* \note: IFX_TAPI_CAP_CHECK - returns 1 if capability is supported
            IFX_TAPI_RING - returns 1 if phone hooked off
          */
         if ((err == 1) &&
             ((IFX_TAPI_CAP_CHECK == pCtx->nCmd) ||
              (IFX_TAPI_RING == pCtx->nCmd)))
         {
            err = IFX_SUCCESS;
         }

         LOG_RD_IOCTL(nDevID, nCh, pCtx->nCmd, pCtx->nArg, pCtx->nArgSize, err);
         break;
   }
}
#endif /* EVENT_LOGGER_DEBUG */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   TAPI global IOCTL handling

   \param  pCtx         Pointer to IOCTL context.

   \return
   - TAPI_statusOk if successful
   - TAPI_statusErr in case of error
   - TAPI_statusCtxErr
   - TAPI_statusParam

   \remarks
   This function does the following functions:
      - If the ioctl command is device specific, low-level driver's ioctl function
      - If the ioctl command is TAPI specific, it is handled at this level
*/
IFX_int32_t  TAPI_Ioctl (IFX_TAPI_ioctlCtx_t *pCtx, IFX_boolean_t *bCompatIoctl)
{
   IFX_int32_t ret = TAPI_statusOk;

   TAPI_ASSERT (pCtx);

   if (bCompatIoctl == IFX_NULL)
      return IFX_ERROR;
   /* Clear the error stack before executing any IOCTL. Otherwise the errors
      could accumulate. LASTERR returns and clears the error stack itself so
      for this IOCTL the error stack must be preserved. */
   if (pCtx->nCmd != IFX_TAPI_LASTERR)
   {
      pCtx->pTapiCh->pTapiDevice->error.nCnt = 0;
      pCtx->pTapiCh->pTapiDevice->error.nCode = 0;
   }

   if (IFX_TAPI_PtrChk (pCtx->pIOCtl))
   {
#ifdef EVENT_LOGGER_DEBUG
      /* \note: logging of TAPI known ioctls only */
      tapi_IoctlLog (pCtx, TAPI_statusOk, TAPI_IOCTL_PRELOG);
#endif /* EVENT_LOGGER_DEBUG */

      /* TAPI specific ioctl */
      ret = tapi_Spec_Ioctl (pCtx, bCompatIoctl);

#ifdef EVENT_LOGGER_DEBUG
      tapi_IoctlLog (pCtx, ret, TAPI_IOCTL_POSTLOG);
#endif /* EVENT_LOGGER_DEBUG */

      /* \note: IFX_TAPI_CAP_CHECK - returns 1 if capability is supported
         IFX_TAPI_RING - returns 1 if phone hooked off
       */
      if ((IFX_TAPI_CAP_CHECK == pCtx->nCmd) ||
          (IFX_TAPI_RING == pCtx->nCmd))
         return ret;
   }
   else
   {
      /* \note: logging of TAPI unknown ioctls should be done by ll-driver */
      /* Low-level device specific ioctl */
      ret = tapi_Dev_Spec_Ioctl (pCtx, bCompatIoctl);
   }

   if (!TAPI_SUCCESS (ret))
      return IFX_ERROR;

   return IFX_SUCCESS;
}

/**
   Retrieve channel and device number by IOCTL conext

   \param   pDrvCtx     Handle to the driver context
   \param   pCtx        Handle to the IOCTL context
   \param   pnDevice    Handle to the result of device number
                        For real device used range [-1..n)
                        -1 value mean no device detected
   \param   pnChannel   Handle to the result of channel number
                        For real channel used range [-1..n)
                        -1 value mean no channel detected

   \return
      IFX_SUCCESS on success

   \remarks
      Single device node find out the device and channel from the
      given structure only for known IOCTL. Otherwise used minor number.
      Following is supported:
      - (1) driver ioctl using simple integers or structures
      - (2) device ioctl using structures with 'dev' fields
      - (3) channel ioctl using structures with 'dev' and 'ch' fields
*/
static IFX_return_t tapi_ioctlDevChGet (
   const IFX_TAPI_DRV_CTX_t *pDrvCtx,
   IFX_TAPI_ioctlCtx_t *pCtx,
   IFX_int32_t *pnDevice,
   IFX_int32_t *pnChannel)
{
   TAPI_ASSERT (pDrvCtx);
   TAPI_ASSERT (pCtx);
   TAPI_ASSERT (pnDevice);
   TAPI_ASSERT (pnChannel);

   if (pDrvCtx->maxDevs == 1)
   {
      /* Extended fd numbering scheme for single device only. */
      /* If only one device is supported allow more than 9 channel fd. */
      *pnDevice = 0;
      *pnChannel = pCtx->nMinor - pDrvCtx->minorBase - 1;
   }
   else
   {
      /* Regular fd numbering scheme for multiple devices. */
      /* retrieve device and channel number by device node */
      *pnDevice = (pCtx->nMinor / pDrvCtx->minorBase) - 1;
      *pnChannel = (pCtx->nMinor % pDrvCtx->minorBase) - 1;
   }

#ifndef TAPI_ONE_DEVNODE

   if (IFX_TAPI_PtrChk (pCtx->pIOCtl))
   {
      /* correct context according to the known IOCTL handler */
      if ((TAPI_IOC_CTX_NONE == pCtx->pIOCtl->nContext) ||
          (TAPI_IOC_CTX_DRV == pCtx->pIOCtl->nContext))
      {
         *pnDevice = -1;
      }
      else if (TAPI_IOC_CTX_DEV == pCtx->pIOCtl->nContext)
      {
         *pnChannel = -1;
      }
   }

#else /* TAPI_ONE_DEVNODE */

   if (IFX_TAPI_PtrChk (pCtx->pIOCtl))
   {
      TAPI_CH_IO_ARG_t *pArg = (TAPI_CH_IO_ARG_t*)pCtx->nArg;

      /* set default value */
      *pnDevice = -1;
      *pnChannel = -1;

      if (TAPI_IOC_CTX_NONE == pCtx->pIOCtl->nContext)
      {
         /* context independent IOCTLs do not contain 'dev' and 'ch' fields in argument */
         return IFX_SUCCESS;
      }

      if (TAPI_IOC_CTX_DRV == pCtx->pIOCtl->nContext)
      {
         /* driver specific IOCTLs do not contain 'dev' and 'ch' fields in argument */
         return IFX_SUCCESS;
      }

      /*  retrieve correct device for channel and device specific channel,
          argument contain 'dev' field */
      if (pArg->dev == IFX_TAPI_EVENT_ALL_DEVICES)
      {
         /* use first device */
         *pnDevice = 0;
      }
      else
      {
         *pnDevice = pArg->dev;
      }

      if (TAPI_IOC_CTX_DEV == pCtx->pIOCtl->nContext)
      {
         /* device specific IOCTLs do not contain 'ch' field in argument */
         return IFX_SUCCESS;
      }

      /*  retrieve correct channel for channel specific IOCTL,
          argument contain 'ch' field */
      if (pArg->ch == IFX_TAPI_EVENT_ALL_CHANNELS)
      {
         /* use firs channel */
         *pnChannel = 0;
      }
      else
      {
         *pnChannel = pArg->ch;
      }
   }
#endif /* TAPI_ONE_DEVNODE */

   return IFX_SUCCESS;
}

/*
   Retrieve IOCTL argument from user

   \param pDrvCtx    Handle for driver context
   \param ioarg      IOCTL argument
   \param pCtx       Handle to the IOCTL context to be updated

   \return
      IFX_SUCCESS on success
 */
static IFX_return_t tapi_ioctlArgGet (
   IFX_TAPI_DRV_CTX_t *pDrvCtx,
   IFX_ulong_t ioarg,
   IFX_TAPI_ioctlCtx_t *pCtx)
{
   IFX_boolean_t bTranslateArg = IFX_FALSE;

   TAPI_ASSERT (pCtx);

   /* Store argument in the context. */
   pCtx->nArg = ioarg;


   if (!IFX_TAPI_PtrChk (pCtx->pIOCtl))
   {
      /* TAPI high-level do not know anything about low-level services,
         just pass it through */
      return IFX_SUCCESS;
   }

   /*
    * Prepare argument for known service
    */

   /* Retrieve IOCTL argument size from IOCTL declaration. */
   pCtx->nArgSize = TAPI_IOC_SIZE (pCtx->nCmd);

   /* For the list of commands below the old style argument will be translated
      into the new style (struct) argument. This struct is built in dynamically
      allocated memory. Here the size needs to be adjusted to allocate the
      correct amount of memory for the new style (struct) argument. */
#if !defined (TAPI_ONE_DEVNODE)
   if ((IFX_TAPI_TONE_STOP == pCtx->nCmd) && (ioarg < 255))
   {
      pCtx->nArgSize = sizeof (IFX_TAPI_TONE_PLAY_t);
      bTranslateArg = IFX_TRUE;
   }
#endif /* !defined (TAPI_ONE_DEVNODE) */

#ifdef TAPI_FEAT_LX_COMPAT
   if ((IFX_TAPI_CAP_LIST == pCtx->nCmd) ||
       (IFX_TAPI_CAP_LIST_32 == pCtx->nCmd))
#else
   if (IFX_TAPI_CAP_LIST == pCtx->nCmd)
#endif
   {
      pCtx->nArgSize = sizeof (IFX_TAPI_CAP_LIST_t);
      bTranslateArg = IFX_TRUE;
   }

   if (TAPI_ARG_POINTER != pCtx->nArgKind)
   {
      /* non pointer values do not require copy from/to user-space */
      pCtx->bArgUsrCpy = IFX_FALSE;
   }

   /* Allocate memory and copy arg if copy from user-space required.
      For special commands build a struct in allocated memory. */
   if ((IFX_TRUE == pCtx->bArgUsrCpy) || (IFX_TRUE == bTranslateArg))
   {
      IFX_void_t *pArg;

      TAPI_ASSERT (pCtx->nArgSize);

      pArg = TAPI_OS_Malloc (pCtx->nArgSize);
      if (!IFX_TAPI_PtrChk (pArg))
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("TAPI ERROR: "
               "Cannot allocate memory for ioctl %s." TAPI_CRLF,
               TAPI_ioctlNameGet (pCtx->nCmd)));
         return IFX_ERROR;
      }

      /* For the list of commands below the old style argument will be
         translated into the new style (struct) argument. This struct is
         built in the above dynamically allocated memory. */
#if !defined (TAPI_ONE_DEVNODE)
      if ((IFX_TAPI_TONE_STOP == pCtx->nCmd) && (ioarg < 255))
      {
         /* Translate TAPI v3 style integer arguments for 'IFX_TAPI_TONE_STOP'
            service. */
         IFX_TAPI_TONE_PLAY_t *pTone = pArg;

         memset (pTone, 0, sizeof (*pTone));
         /* translate parameter into new struct */
         pTone->index = (IFX_uint32_t)ioarg;
         pTone->module = IFX_TAPI_MODULE_TYPE_ALM;
      }
#endif /* !defined (TAPI_ONE_DEVNODE) */
#ifdef TAPI_FEAT_LX_COMPAT
      if ((IFX_TAPI_CAP_LIST == pCtx->nCmd) ||
          (IFX_TAPI_CAP_LIST_32 == pCtx->nCmd))
#else
      if (IFX_TAPI_CAP_LIST == pCtx->nCmd)
#endif
      {
         /* Translate TAPI v3 style 'IFX_TAPI_CAP_LIST' argument. */
         IFX_TAPI_CAP_LIST_t *pList = pArg;

         memset (pList, 0, sizeof (*pList));
         /* translate parameter into new struct */
         pList->nCap = pDrvCtx->CAP_Number_Get (pDrvCtx->pTapiDev[0].pLLDev);
#ifdef TAPI_FEAT_LX_COMPAT
         pList->pList = compat_ptr(ioarg);
#else
         pList->pList = (IFX_TAPI_CAP_t*)ioarg;
#endif
      }

      /* copy ioctl argument into the local memory */
      if ((IFX_TRUE == pCtx->bArgUsrCpy) && (IFX_TRUE != bTranslateArg))
      {
         if (!TAPI_OS_CpyUsr2Kern (pArg, (IFX_void_t*)ioarg, pCtx->nArgSize))
         {
            TAPI_OS_Free (pArg);
            TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("TAPI ERROR: "
                  "Cannot copy argument for ioctl %s." TAPI_CRLF,
                  TAPI_ioctlNameGet (pCtx->nCmd)));
            return IFX_ERROR;
         }
      }

      pCtx->nArg = (IFX_ulong_t)pArg;
   }

   if (TAPI_ARG_POINTER == pCtx->nArgKind)
   {
      /* check for integer value in IOCTL argument
         \note:
            not working for IFX_TAPI_RING_CADENCE_SET,
            because cadence can looks like address. */
      if ((pCtx->nArgSize <= sizeof (IFX_uint32_t)) &&
          (ioarg > 0) && (ioarg <= 255))
      {
         /* integer value cannot be used as pointer */
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("TAPI ERROR: "
               "Integer argument detected for ioctl %s, "
               "required pointer." TAPI_CRLF,
               TAPI_ioctlNameGet (pCtx->nCmd)));
         return IFX_ERROR;
      }

      /* check for NULL pointer in IOCTL argument */
      if (0 == pCtx->nArg)
      {
         /* integer value can not be used as pointer */
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("TAPI ERROR: "
               "NULL pointer detected for ioctl %s" TAPI_CRLF,
               TAPI_ioctlNameGet (pCtx->nCmd)));
         return IFX_ERROR;
      }

#if defined(ENABLE_TRACE) && defined(ENABLE_IOCTL_DETAILS)
      /* Print memory dump for pointer argument type.
         Byte by byte, maximum 16 bytes in line, each line prefixed with
         ioctl name, colon and space */
      {
         IFX_char_t line_prefix[60] = {0};
         snprintf(line_prefix, sizeof(line_prefix),
                  "%s (%X): ", TAPI_ioctlNameGet(pCtx->nCmd), TAPI_OS_THREAD_ID);

         TAPI_TRACE_BYTES_FROM_MEMORY(line_prefix,
                                      (IFX_char_t *) pCtx->nArg, pCtx->nArgSize);
      }
#endif /* defined(ENABLE_TRACE) && defined(ENABLE_IOCTL_DETAILS) */
   }

#if !defined (TAPI_ONE_DEVNODE)
#ifdef CLASSIC_TAPI_COMPATIBLE
   if (IFX_TAPI_CH_INIT == pCtx->nCmd)
   {
      /* For backward compatibility of the 'IFX_TAPI_CH_INIT' service
         the 'ch' field is corrected here. */
      if (pDrvCtx->maxDevs == 1)
      {
         /* Extended fd numbering scheme for single device only. */
         ((IFX_TAPI_CH_INIT_t*)pCtx->nArg)->ch =
            pCtx->nMinor - pDrvCtx->minorBase - 1;
      }
      else
      {
         /* Regular fd numbering scheme for multiple devices. */
         ((IFX_TAPI_CH_INIT_t*)pCtx->nArg)->ch = \
            (pCtx->nMinor % pDrvCtx->minorBase) - 1;
      }
   }
#endif /* CLASSIC_TAPI_COMPATIBLE */
#endif /* !defined (TAPI_ONE_DEVNODE) */

   return IFX_SUCCESS;
}

/*
   Put IOCTL argument back in to the user

   \param ioarg      IOCTL argument
   \param pCtx       Handle to the IOCTL context to be updated
 */
static IFX_void_t tapi_ioctlArgPut (
   IFX_ulong_t ioarg,
   IFX_TAPI_ioctlCtx_t *pCtx)
{
   TAPI_ASSERT (pCtx);

   /* do nothing if direct argument was used */
   if (ioarg == pCtx->nArg)
      return;

   do {
      /* Do not copy back into the temporary structure. */
#ifdef TAPI_FEAT_LX_COMPAT
      if ((IFX_TAPI_CAP_LIST == pCtx->nCmd) ||
          (IFX_TAPI_CAP_LIST_32 == pCtx->nCmd))
#else
      if (IFX_TAPI_CAP_LIST == pCtx->nCmd)
#endif
         break;

      /* only read direction can require copy result back */
      if (IFX_FALSE == pCtx->bRead)
         break;

      /* copy result back to the user */
      if (IFX_FALSE == pCtx->bArgUsrCpy)
      {
         memcpy ((IFX_void_t*)ioarg, (IFX_void_t*)pCtx->nArg,
            pCtx->nArgSize);
      }
      else
      {
         if (!TAPI_OS_CpyKern2Usr ((IFX_void_t*)ioarg,
                                   (IFX_void_t*)pCtx->nArg, pCtx->nArgSize))
         {
            TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("TAPI ERROR: "
                  "Failed to copy result for ioctl %s." TAPI_CRLF,
                  TAPI_ioctlNameGet (pCtx->nCmd)));
         }
      }
   } while (0);

   /* release local memory */
   TAPI_OS_Free ((IFX_void_t*)pCtx->nArg);
}

/**
   Prepare IOCTL context

   \param pDrvCtx    Handle for driver context
   \param nMinor     Minor number of device node
   \param iocmd      IOCTL command
   \param ioarg      IOCTL argument
   \param bArgUsrCpy IOCTL argument require copy from/to user space
   \param pCtx       Handle to the IOCTL context to be updated

   \return
      IFX_SUCCESS on success
*/
IFX_return_t TAPI_ioctlContextGet (
   IFX_TAPI_DRV_CTX_t *pDrvCtx,
   IFX_uint32_t nMinor,
   IFX_uint32_t iocmd,
   IFX_ulong_t ioarg,
   IFX_boolean_t bArgUsrCpy,
   IFX_TAPI_ioctlCtx_t *pCtx)
{
   IFX_int32_t nDevice = -1;
   IFX_int32_t nChannel = -1;

   TAPI_ASSERT (pDrvCtx);
   TAPI_ASSERT (pCtx);

   /* do basic initialization */
   memset (pCtx, 0, sizeof(IFX_TAPI_ioctlCtx_t));

   /* store TAPI service identifier */
   pCtx->nCmd = iocmd;
   /* remember storage space of argument value */
   pCtx->bArgUsrCpy = bArgUsrCpy;
   /* store minor number for future intermediate use */
   pCtx->nMinor = nMinor;

   /* retrieve IOCTL descriptor */
   switch (TAPI_IOC_MAGIC (iocmd))
   {
      case IFX_TAPI_IOC_MAGIC:
         if (TAPI_IOC_IDX (iocmd) >= ARRAY_SIZE (gIOCtls))
         {
            TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                  ("TAPI ERROR: Unknown IOCTL command %s" TAPI_CRLF,
                  TAPI_ioctlNameGet(iocmd)));
            return IFX_ERROR;
         }

         /* retrieve corresponding IOCTL descriptor */
         pCtx->pIOCtl = gIOCtls + TAPI_IOC_IDX (iocmd);
         break;
      default:
         
         /* NOTE: TAPI do not know the argument structure of low-level IOCTLs,
            just forward to the low-level handler without device or channel detection.
            low-level driver responsible for it own target detection */
         break;
   }

   /* prepare context for known IOCTL */
   if (IFX_TAPI_PtrChk (pCtx->pIOCtl))
   {
      /* detect IOCTL read direction */
      pCtx->bRead = TAPI_IOC_READ (iocmd);
      /* detect IOCTL write direction */
      pCtx->bWrite = TAPI_IOC_WRITE (iocmd);

#ifdef DEBUG
      TRACE(TAPI_DXS, DBG_LEVEL_LOW,
            ("TAPI DXS: %s -> %s [fd:%d][arg:0x%lX][%s%s][tid:%X]" TAPI_CRLF,
            TAPI_ioctlNameGet(iocmd), pCtx->pIOCtl->info,
            nMinor, ioarg,
            (pCtx->bRead ? "r" : ""), (pCtx->bWrite ? "w" : ""),
            TAPI_OS_THREAD_ID));
#endif /* DEBUG */

      /* for integer argument write direction available only */
      if ((TAPI_ARG_INTEGER == pCtx->pIOCtl->nArgKind) &&
          (IFX_TRUE == pCtx->bRead))
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("TAPI ERROR: Incorrect direction detected for ioctl %s." TAPI_CRLF,
                TAPI_ioctlNameGet(iocmd)));
         return IFX_ERROR;
      }

      /* detect the expected argument type */
      pCtx->nArgKind = pCtx->pIOCtl->nArgKind;

#if defined (TAPI_ONE_DEVNODE)
      if ((TAPI_IOC_CTX_DEV == pCtx->pIOCtl->nContext) ||
          (TAPI_IOC_CTX_CH == pCtx->pIOCtl->nContext))
      {
         /* Device and channel specific service for single device node
            always using pointer as argument. */
         pCtx->nArgKind = TAPI_ARG_POINTER;
      }
#endif /* TAPI_ONE_DEVNODE */
   }

   /* retrieve argument for TAPI IOCTL */
   if (IFX_SUCCESS != tapi_ioctlArgGet (pDrvCtx, ioarg, pCtx))
   {
      return IFX_ERROR;
   }

   /* retrieve destination for TAPI IOCTL */
   if (IFX_SUCCESS != tapi_ioctlDevChGet (pDrvCtx, pCtx, &nDevice, &nChannel))
   {
      return IFX_ERROR;
   }

   if (nDevice >= pDrvCtx->maxDevs)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_NORMAL,
            ("TAPI ERROR: Unsupported device '%d' used for '%s' ioctl %s" TAPI_CRLF,
            nDevice, pDrvCtx->drvName, TAPI_ioctlNameGet (iocmd)));
      return IFX_ERROR;
   }

   if (nChannel >= pDrvCtx->maxChannels)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_NORMAL,
            ("TAPI ERROR: Unsupported channel '%d' used for '%s' ioctl %s" TAPI_CRLF,
            nChannel, pDrvCtx->drvName, TAPI_ioctlNameGet (iocmd)));
      return IFX_ERROR;
   }

   /* assign handle to the IOCTL context */
   if (nDevice < 0)
   {
      if (IFX_TAPI_PtrChk (pCtx->pIOCtl) &&
          (TAPI_IOC_CTX_NONE == pCtx->pIOCtl->nContext))
      {
         /* context independent */
         pCtx->nContext = TAPI_IOC_CTX_NONE;
      }
      else
      {
         /* driver context */
         pCtx->nContext = TAPI_IOC_CTX_DRV;
      }

      /* took first channel of first device */
      nDevice = 0;
      nChannel = 0;
   }
   else if (nChannel < 0)
   {
      /* device context */
      pCtx->nContext = TAPI_IOC_CTX_DEV;
      /* took first channel */
      nChannel = 0;
   }
   else
   {
      /* channel context */
      pCtx->nContext = TAPI_IOC_CTX_CH;
   }

   /* validate declared and detected service description */
   if (IFX_TAPI_PtrChk (pCtx->pIOCtl))
   {
      /*  check the detected and available contexts */
      if (pCtx->nContext != pCtx->pIOCtl->nContext)
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("TAPI ERROR: Incorrect context detected [%d], "
               "expected [%d] for ioctl %s." TAPI_CRLF,
               pCtx->nContext, pCtx->pIOCtl->nContext,
               TAPI_ioctlNameGet (iocmd)));
         /* Service called with wrong context fd */
         return IFX_ERROR;
      }
   }

   /* retrieve context handle */
   pCtx->pTapiCh = pDrvCtx->pTapiDev[nDevice].pChannel + nChannel;

   return IFX_SUCCESS;
}

/**
   Finalize IOCTL context communication

   \param ioarg      IOCTL argument
   \param pCtx       Handle to the IOCTL context to be updated
*/
IFX_void_t TAPI_ioctlContextPut(IFX_ulong_t ioarg, IFX_TAPI_ioctlCtx_t *pCtx)
{
   /* handle argument value */
   tapi_ioctlArgPut(ioarg, pCtx);

   /* do cleanup memory */
   memset(pCtx, 0, sizeof(*pCtx));
}

/**
   Device Specific IOCTL handling

   \param  pCtx         Pointer to IOCTL context.

   \return
   - TAPI_statusOk if successful
   - TAPI_statusErr in case of error
*/
static IFX_int32_t tapi_Dev_Spec_Ioctl (IFX_TAPI_ioctlCtx_t* pCtx,
                     IFX_boolean_t *bCompatIoctl)
{
   TAPI_DEV *pTapiDev = IFX_NULL;
   IFX_void_t *pLLHnd = IFX_NULL;
   IFX_int32_t retLL = TAPI_statusErr;

   IFX_int32_t (*ioctl_ll_handler)(IFX_TAPI_LL_CH_t *pCh, IFX_uint32_t nCmd,
                                   IFX_ulong_t ioarg);

   if (pCtx == IFX_NULL)
      return IFX_ERROR;

   if (pCtx->pTapiCh == IFX_NULL)
      return IFX_ERROR;

   /* retrieve current device handle */
   pTapiDev = pCtx->pTapiCh->pTapiDevice;

   if (pTapiDev == IFX_NULL)
      return IFX_ERROR;

   ioctl_ll_handler = *bCompatIoctl ? pTapiDev->pDevDrvCtx->CompatIoctl
                                    : pTapiDev->pDevDrvCtx->Ioctl;

#ifdef TAPI_ONE_DEVNODE
   /* \note: low level handler should manually detect context,
      pass IFX_NULL as ioctl handle.
      */
#else
   /* Provide either a low-level device or channel context to the low-level
      IOCTL handler. The low level IOCTL handler has to check the correctness
      of the context. */
   switch (pCtx->nContext)
   {
      case TAPI_IOC_CTX_CH:
         pLLHnd = (IFX_void_t*) pCtx->pTapiCh->pLLChannel;
         break;
      case TAPI_IOC_CTX_DRV:
         /* use device handle for driver context */
         /*lint -fallthrough*/
      case TAPI_IOC_CTX_NONE:
         /* use device handle for context independent service */
         /*lint -fallthrough*/
      case TAPI_IOC_CTX_DEV:
         pLLHnd = (IFX_void_t*) pTapiDev->pLLDev;
         break;
      default:
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("TAPI ERROR: "
               "Unsupported context detected [%d] for ll-ioctl %s" TAPI_CRLF,
               pCtx->nContext, TAPI_ioctlNameGet (pCtx->nCmd)));
         /* should never occur */
         return TAPI_statusCtxErr;
   }
#endif /* not TAPI_ONE_DEVNODE */

   /* make sure that the low-level supports IOCTLs */
   if (!IFX_TAPI_PtrChk(ioctl_ll_handler))
      RETURN_DEVSTATUS(TAPI_statusNoIoctl, 0);

   /* dispatch to low level IOCTL handler */
   retLL = ioctl_ll_handler (pLLHnd, pCtx->nCmd, pCtx->nArg);

   if (!TAPI_SUCCESS(retLL))
   {
      /* if low-level failed add a generic high-level error code */
      RETURN_DEVSTATUS(TAPI_statusLLFailed, retLL);
   }

   return TAPI_statusOk;
}

/**
   TAPI specific IOCTL handling

   \param  pCtx         Pointer to IOCTL context.

   \return
   - TAPI_statusOk if successful
   - TAPI_statusErr in case of error
   - TAPI_statusCtxErr
   - TAPI_statusParam

   \remarks
      Only TAPI IOCTLs with appropriate TAPI magic number are
      handled within this function.
*/
static IFX_int32_t tapi_Spec_Ioctl(IFX_TAPI_ioctlCtx_t *pCtx,
                                   const IFX_boolean_t *bCompatIoctl)
{
   IFX_int32_t ret = TAPI_statusOk, retLL = TAPI_statusOk;
   IFX_void_t     *pHnd = IFX_NULL,
                  *pLLHnd = IFX_NULL;
   TAPI_Handler_t pHandler;
   TAPI_OS_mutex_t *pSemSingleIoctlAccess = IFX_NULL;

   /* retrieve current device handle */
   TAPI_DEV *pTapiDev = pCtx->pTapiCh->pTapiDevice;
   IFX_void_t *pArg = (IFX_void_t*)pCtx->nArg;

   /* retrieve device handle for error reports */
   switch (pCtx->nContext)
   {
      case TAPI_IOC_CTX_CH:
         pHnd = (IFX_void_t *) pCtx->pTapiCh;
         pLLHnd = (IFX_void_t *) pCtx->pTapiCh->pLLChannel;
         pSemSingleIoctlAccess = &pCtx->pTapiCh->semTapiChSingleIoctlAccess;
         break;
      case TAPI_IOC_CTX_DEV:
         pHnd = (IFX_void_t *) pTapiDev;
         pLLHnd = (IFX_void_t*) pTapiDev->pLLDev;
         pSemSingleIoctlAccess = &pTapiDev->semTapiDevSingleIoctlAccess;
         break;
      case TAPI_IOC_CTX_DRV:
         pHnd = (IFX_void_t *) pTapiDev->pDevDrvCtx;
         pLLHnd = IFX_NULL;
         pSemSingleIoctlAccess = IFX_NULL;
         break;
      case TAPI_IOC_CTX_NONE:
         pHnd = IFX_NULL;
         pLLHnd = IFX_NULL;
         pSemSingleIoctlAccess = IFX_NULL;
         break;
      default:
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("TAPI ERROR: "
               "Unsupported context detected [%d] for hl-ioctl %s" TAPI_CRLF,
               pCtx->nContext,
               TAPI_ioctlNameGet (pCtx->nCmd)));
         /* should never occur */
         return TAPI_statusCtxErr;
   }

   if (IFX_FALSE == pTapiDev->bInitialized)
   {
      /* check requirement for initialized device */
      if (TAPI_DS_INIT == (TAPI_DS_INIT & pCtx->pIOCtl->nDevState))
      {
         TRACE(TAPI_DXS, DBG_LEVEL_LOW,
               ("IOCTL %s for dev%d blocked." TAPI_CRLF,
               TAPI_ioctlNameGet (pCtx->nCmd), pTapiDev->nDev));
         /* errmsg: IOCTL is blocked until the device is started */
         RETURN_DEVSTATUS (TAPI_statusIoctlBlocked, 0);
      }

      /* do not use protection for uninitialized device */
      pSemSingleIoctlAccess = IFX_NULL;
   }

   /* retrieve IOCTL handler */
#ifdef TAPI_FEAT_LX_COMPAT
   if (*bCompatIoctl)
      pHandler = pCtx->pIOCtl->pCompatHandler.p == IFX_NULL
           ? pCtx->pIOCtl->pHandler  /* ioctl doesn't require compat handler */
           : pCtx->pIOCtl->pCompatHandler;
   else
#endif /* TAPI_FEAT_LX_COMPAT */
      pHandler = pCtx->pIOCtl->pHandler;
   if (!IFX_TAPI_PtrChk (pHandler.p))
      RETURN_DEVSTATUS (TAPI_statusNotSupported, 0);

   if (TAPI_IS_LL_HANDLER (pHandler))
   {
      /* retrieve low-level handler from driver context */
      pHandler = TAPI_LL_HANDLER_GET (pTapiDev->pDevDrvCtx, pHandler);
      if (!IFX_TAPI_PtrChk (pHandler.p))
         RETURN_DEVSTATUS (TAPI_statusLLNotSupp, 0);

      /* use low-level handle for low-level handler */
      pHnd = pLLHnd;
   }

#if !defined (TAPI_ONE_DEVNODE)
   if ((pCtx->nCmd == IFX_TAPI_EVENT_GET) ||
       (pCtx->nCmd == IFX_TAPI_EVENT_ENABLE) ||
       (pCtx->nCmd == IFX_TAPI_EVENT_DISABLE) ||
       (pCtx->nCmd == IFX_TAPI_EVENT_MULTI_ENABLE) ||
       (pCtx->nCmd == IFX_TAPI_EVENT_MULTI_DISABLE))
   {
      /* The device number for the above IOCTLs was set to 0 because the
         IOCTL context was defined as driver global. For the multiple
         device node approach the IOCTLs are actually device specific
         so here the device number is set again from the fd minor number. */
      if (pTapiDev->pDevDrvCtx->maxDevs == 1)
      {
         /* Extended fd numbering scheme for single device only. */
         ((IFX_TAPI_EVENT_t*)pArg)->dev = 0;
      }
      else
      {
         /* Regular fd numbering scheme for multiple devices. */
         ((IFX_TAPI_EVENT_t*)pArg)->dev =
               (pCtx->nMinor / pTapiDev->pDevDrvCtx->minorBase) - 1;
      }
   }
#endif /* !defined (TAPI_ONE_DEVNODE) */

   if (IFX_TAPI_PtrChk (pSemSingleIoctlAccess))
   {
      /* protect ioctl handler against concurrent access */
      TAPI_OS_MutexGet (pSemSingleIoctlAccess);
   }

   /* call IOCtl handler */
   if (TAPI_IOC_CTX_NONE == pCtx->nContext)
   {
      switch (pCtx->pIOCtl->nArgKind)
      {
         case TAPI_ARG_POINTER:
            ret = pHandler.pPtr (pArg);
            break;
         case TAPI_ARG_INTEGER:
            ret = pHandler.pInt ((IFX_uint32_t)((IFX_uintptr_t)pArg));
            break;
         case TAPI_ARG_NONE:
            ret = pHandler.pNone ();
            break;
      }
   }
   else
   {
      switch (pCtx->pIOCtl->nArgKind)
      {
         case TAPI_ARG_POINTER:
            ret = pHandler.pPtrPtr (pHnd, pArg);
            break;
         case TAPI_ARG_INTEGER:
   #ifdef TAPI_ONE_DEVNODE
            if (TAPI_IOC_CTX_CH == pCtx->nContext)
            {
               ret = pHandler.pPtrInt (pHnd, ((TAPI_CH_IO_ARG_t *)pArg)->param);
            }
            else if (TAPI_IOC_CTX_DEV == pCtx->nContext)
            {
               ret = pHandler.pPtrInt (pHnd, ((TAPI_DEV_IO_ARG_t *)pArg)->param);
            }
            else
   #endif /* TAPI_ONE_DEVNODE */
            {
               ret = pHandler.pPtrInt (pHnd, (IFX_uint32_t)((IFX_uintptr_t)pArg));
            }
            break;
         case TAPI_ARG_NONE:
            ret = pHandler.pPtr (pHnd);
            break;
      }
   }

   if (IFX_TAPI_PtrChk (pSemSingleIoctlAccess))
   {
      /* release ioctl handler protection against concurrent access */
      TAPI_OS_MutexRelease (pSemSingleIoctlAccess);
   }

   if (!TAPI_SUCCESS (ret) &&
       TAPI_IS_LL_HANDLER (pCtx->pIOCtl->pHandler))
   {
      /* The following IOCTLs have individual return values that will
         be passed transparently to the application. For all other IOCTLs
         the return value is handled as an error code and stored onto the
         error stack. */

      if ((pCtx->nCmd != IFX_TAPI_CAP_CHECK) &&
          (pCtx->nCmd != IFX_TAPI_RING))
      {
         /*  correct error code for low-level failure */
         retLL = ret;
         ret = TAPI_statusLLFailed;
      }
   }

   if (!TAPI_SUCCESS (retLL))
      RETURN_DEVSTATUS (ret, retLL);

   return ret;
}
