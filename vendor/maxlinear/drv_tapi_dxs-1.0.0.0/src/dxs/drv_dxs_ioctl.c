/******************************************************************************

  Copyright 2014-2015 Lantiq Deutschland GmbH
  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.
  Copyright 2021-2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_ioctl.c
   Contains ioctl specific implementations according to io type.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"

#include <drv_tapi_config.h>

#include "drv_dxs_access.h"
#include "drv_dxs_dwld.h"
#include "drv_dxs_bbd.h"
#include "drv_dxs_debug.h"
#include "drv_dxs_alm_lt.h"
#include "drv_dxs_init.h"
#include "drv_dxs_mbx.h"
#include "drv_dxs_version.h"

#include "../tapi/drv_tapi_debug.h"
#include "../tapi/drv_tapi_debug_buffer.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/**
   Dispatches ioctrl commands which doesn't need data management

\param   pContext - context pointer, may be device or channel pointer.
\param   msg      - ioctrl command id
\param   func     - function name
\param   arg      - argument for the function, cast before if needed.
*/
#define  ON_DIR_IOCTL(pContext,msg,func,arg)\
            case (msg):\
               ret = func((pContext),arg);\
               break

/**
   Dispatches ioctrl commands and manages user space data handling in both
   directions.

\param   pContext - context pointer, may be device or channel pointer.
\param   msg      - ioctrl command id
\param   func     - function name
\param   arg      - structure identifier used as argument for the function
                    call and for copying user data
\remarks
   As an alternative use a transfer structure DUS_IO_USR and
   DoDataExchange for handling data exchange.
*/
#ifdef LINUX
#define ON_IOCTL(pContext,msg,func,arg)                                       \
      case (msg):                                                             \
         {                                                                    \
            arg* p_arg = TAPI_OS_Malloc(sizeof(arg));                          \
            if (p_arg != IFX_NULL)                                            \
            {                                                                 \
                  TAPI_OS_CpyUsr2Kern(p_arg, (IFX_uint8_t*)ioarg, sizeof(arg));\
                  ret = func((pContext), p_arg);                              \
                  TAPI_OS_CpyKern2Usr((IFX_uint8_t*)ioarg, p_arg, sizeof(arg));\
                  TAPI_OS_Free(p_arg);                                         \
            }                                                                 \
            else                                                              \
            {                                                                 \
               TAPI_ASSERT(p_arg != IFX_NULL);                                 \
            }                                                                 \
         }                                                                    \
         break
#else
#define ON_IOCTL(pContext,msg,func,arg)\
           case (msg):\
              ret = func((pContext),(arg*)ioarg);\
              break
#endif /* LINUX */

/**
   Dispatches ioctrl commands and manages user space data handling in
   driver direction.

\param   pContext - context pointer, may be device or channel pointer.
\param   msg      - ioctrl command id
\param   func     - function name
\param   arg      - structure identifier used as argument for the function
                    call and for copying user data
*/
#ifdef LINUX
#define ON_IOCTL_FRUSR(pContext,msg,func,arg)                                 \
         case (msg):                                                          \
            {                                                                 \
               arg* p_arg = TAPI_OS_Malloc(sizeof(arg));                       \
               if (p_arg != IFX_NULL)                                         \
               {                                                              \
                  TAPI_OS_CpyUsr2Kern(p_arg, (IFX_uint8_t*)ioarg, sizeof(arg));\
                  ret = func((pContext), p_arg);                              \
                  TAPI_OS_Free(p_arg);                                         \
               }                                                              \
               else                                                           \
               {                                                              \
                  TAPI_ASSERT(p_arg != IFX_NULL);                              \
               }                                                              \
            }                                                                 \
            break
#else
#define ON_IOCTL_FRUSR(pContext,msg,func,arg)\
           ON_IOCTL((pContext),msg,func,arg)
#endif /* LINUX */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

#ifdef TAPI_ONE_DEVNODE

/** Generic structure for device based ioctl arguments. */
struct DXS_DEV_IO_ARG
{
   /** Device index */
   IFX_uint16_t dev;
   /** Any parameter used by ioctls */
   IFX_uint32_t param;
};
#endif /* TAPI_ONE_DEVNODE */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
#ifdef DEBUG
   #ifdef DXS_FEAT_IOCTL_RW_CMD
      static IFX_int32_t DXS_IOCTL_CmdWrite(DXS_DEVICE_t *pDev,
                                          DXS_IO_MB_CMD_t *pCmd);
      static IFX_int32_t DXS_IOCTL_CmdRead(DXS_DEVICE_t *pDev,
                                          DXS_IO_MB_CMD_t *pCmd);
   #endif /* DXS_FEAT_IOCTL_RW_CMD */

   #ifdef DXS_FEAT_IOCTL_RW_REG
      static IFX_int32_t DXS_IOCTL_RegWrite(DXS_DEVICE_t *pDev,
                                            DXS_IO_RegAccess_t *pCmd);
      static IFX_int32_t DXS_IOCTL_RegRead(DXS_DEVICE_t *pDev,
                                           DXS_IO_RegAccess_t *pCmd);
   #endif /* DXS_FEAT_IOCTL_RW_REG */
#endif /* DEBUG*/

static IFX_int32_t DXS_IOCTL_ChipReset(DXS_DEVICE_t *pDev,
                                       IFX_uint16_t reset);
extern IFX_int32_t DXS_DwldAndStartFW(DXS_DEVICE_t *pDev,
                                      DXS_FW_Download_t *pEdsp);
extern IFX_int32_t DXS_Version(DXS_DEVICE_t *pDev, DXS_IO_Version_t *pInd);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

/**
   Swaps every two 16-bit-words in the buffer

   This swap is needed because of the data format in the FW download and has
   nothing to do with endianess.

   \param pWbuf  Pointer to array of size DXS_IO_MB_CMD_SIZE with the 16-bit-words.
*/
static inline IFX_void_t DXS_IOCTL_CommandWordSwap(IFX_uint16_t (*pWbuf)[DXS_IO_MB_CMD_SIZE])
{
   IFX_uint32_t i = 0;
   /* swap words in each pair of two words, e.g. word 2-1, 4-3, etc. */
   for (; i < DXS_IO_MB_CMD_SIZE; i += 2)
   {
      IFX_uint16_t tmp1 = (*pWbuf)[i];
      (*pWbuf)[i] = (*pWbuf)[i + 1];
      (*pWbuf)[i + 1] = tmp1;
   }
}


#ifdef TAPI_FEAT_LX_COMPAT
IFX_int32_t DXS_Dev_Spec_Compat_Ioctl (IFX_TAPI_LL_CH_t *pLLDummyCh,
                                 IFX_uint32_t iocmd,
                                 IFX_ulong_t  ioarg)
{
   /* if no data type conversion is required convert only IOCTL number: */
   /*
      switch (iocmd)
      {
         case FIO_DXS_XYZ_32:
            iocmd = FIO_DXS_XYZ;
            break;
      }
    */
   return DXS_Dev_Spec_Ioctl(pLLDummyCh, iocmd, (IFX_ulong_t) compat_ptr(ioarg));
}
#endif

#ifdef DEBUG
   #ifdef ENABLE_TRACE
      static struct IoctlLookupTable const dxs_ioctl_table[] =
      {
         IOCTL_LKUP_TBL_ADD(FIO_DXS_VERS),
      #ifndef TAPI_ONE_DEVNODE
         IOCTL_LKUP_TBL_ADD(FIO_DXS_DRVVERS),
      #endif
         IOCTL_LKUP_TBL_ADD(FIO_DXS_REPORT_SET),
         IOCTL_LKUP_TBL_ADD(FIO_DXS_BASICDEV_INIT),
         IOCTL_LKUP_TBL_ADD(FIO_DXS_DEV_RESET),
         IOCTL_LKUP_TBL_ADD(FIO_DXS_BBD_DOWNLOAD),
         IOCTL_LKUP_TBL_ADD(FIO_DXS_FW_DOWNLOAD),
      #ifdef DEBUG
         IOCTL_LKUP_TBL_ADD(FIO_DXS_WCMD),
         IOCTL_LKUP_TBL_ADD(FIO_DXS_RCMD),
         IOCTL_LKUP_TBL_ADD(FIO_DXS_RDREG),
         IOCTL_LKUP_TBL_ADD(FIO_DXS_WRREG),
      #endif
         /* Deprecated ioctls */
         IOCTL_LKUP_TBL_ADD(FIO_DXS_CHIP_RESET)
      };


      /**
       * Translate ioctl number to human readable name
       * 
       * \param nIoctl ioctl number
       * \return const char* string with human readable ioctl name
       */
      static const char *DXS_ioctlNameGet(IFX_uint32_t nIoctl)
      {
         IFX_uint8_t i = 0;
         for (i = 0; i < ARRAY_SIZE(dxs_ioctl_table); ++i)
         {
            if (nIoctl == dxs_ioctl_table[i].val)
               return dxs_ioctl_table[i].name;
         }

         return "UNKNOWN DXS IOCTL";
      }
   #endif /* ENABLE_TRACE */
#endif /* DEBUG */

/**
   DUSLIC XS device specific ioctl handling

   \param pLLDummyCh    Handle to DXS_CHANNEL_t structure (could be also a
                        pointer to a DXS_DEVICE_t structure - will be detected)
   \param iocmd         IOCTL identifier.
   \param ioarg         Argument of the IOCTL.

   \return
   - DXS_statusOk
   - DXS_statusNotInitialized
*/
IFX_int32_t DXS_Dev_Spec_Ioctl (IFX_TAPI_LL_CH_t *pLLDummyCh,
                                IFX_uint32_t iocmd,
                                IFX_ulong_t ioarg)
{
   IFX_int32_t   ret    = DXS_statusOk;
   DXS_CHANNEL_t *pCh   = (DXS_CHANNEL_t *) pLLDummyCh;
   DXS_DEVICE_t  *pDev;
#ifdef TAPI_ONE_DEVNODE
   struct DXS_DEV_IO_ARG* pDevArg = IFX_NULL;
#endif /* TAPI_ONE_DEVNODE */

#ifdef DEBUG
   TRACE(TAPI_DXS, DBG_LEVEL_LOW,
         ("DXS: %s [id:0x%08X][arg:0x%lX][tid:%X]\n",
         DXS_ioctlNameGet(iocmd), iocmd, ioarg, TAPI_OS_THREAD_ID));
#endif /* DEBUG */

#ifndef TAPI_ONE_DEVNODE
   /* validate correct channel pointer */
   if (IFX_NULL == pCh)
      return DXS_statusParam;

   /* distinguish device / channel context */
   if (pCh->nChannel == 0)
   {
      /* initial assumption is wrong - we received a device pointer */
      pDev = (DXS_DEVICE_t *) pLLDummyCh;
   }
   else
   {
      /* initial assumption was correct so we only need to get the
         corresponding device pointer */
      pDev = pCh->pParent;
   }
#else /* TAPI_ONE_DEVNODE */
   /* by default use first device and set channel to null */
   DXS_GetDevice(0, &pDev);
   pCh = IFX_NULL;

#if defined(LINUX) && defined(TAPI_FEAT_IOCTL_CAPABILITY_CHECK)
   if (!capable(CAP_SYS_PACCT))
   {
      RETURN_DEVSTATUS(DXS_statusNoPerm, IFX_NULL);
   }
#endif

   /* getting context for specific ioctls */
   switch (iocmd)
   {
      /* Drv specific ioctls */
      case FIO_DXS_REPORT_SET:
         break;
      /* Dev specific ioctls */
      case FIO_DXS_VERS:
      case FIO_DXS_BASICDEV_INIT:
      case FIO_DXS_DEV_RESET:
      case FIO_DXS_FW_DOWNLOAD:
#ifdef TAPI_FEAT_LX_COMPAT
      case FIO_DXS_FW_DOWNLOAD_32:
      case FIO_DXS_INIT_32:
#endif /* TAPI_FEAT_LX_COMPAT */
      case FIO_DXS_CHIP_RESET:

#ifdef DEBUG
      case FIO_DXS_WCMD:
      case FIO_DXS_RCMD:
      case FIO_DXS_WRREG:
      case FIO_DXS_RDREG:
#endif /* DEBUG */
         /* simple, not perfect, argument validation */
         if (ioarg <= 255)
         {
            RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);
         }

         /* get data from user space */
         pDevArg = TAPI_OS_Malloc(sizeof(*pDevArg));
         if (IFX_NULL == pDevArg)
         {
            RETURN_DEVSTATUS(DXS_statusNoMem, IFX_NULL);
         }

         if (TAPI_OS_CpyUsr2Kern(pDevArg, (IFX_void_t *)ioarg, sizeof(*pDevArg)) == IFX_NULL)
         {
            TAPI_OS_Free(pDevArg);
            RETURN_DEVSTATUS(DXS_statusNoMem, IFX_NULL);
         }

         if (DXS_statusOk != DXS_GetDevice(pDevArg->dev, &pDev))
         {
            TAPI_OS_Free(pDevArg);
            return DXS_statusDeviceIdErr;
         }

         if (iocmd == FIO_DXS_CHIP_RESET)
         {
            /* get reset value so handling of argument doesn't have to change
               for tapi v4 */
            ioarg = ((DXS_ChipReset_t*) pDevArg)->nReset;
         }

         TAPI_OS_Free(pDevArg);

         break;
      /* channel specific ioctls - currently only one */
#ifdef TAPI_FEAT_LX_COMPAT
      case FIO_DXS_BBD_DOWNLOAD_32:
#endif
      case FIO_DXS_BBD_DOWNLOAD:
         {
            void *pBbdArg;
            IFX_uint32_t size;
            IFX_uint16_t devUsr;
            IFX_uint16_t chUsr;
            IFX_uint32_t bBroadcastUsr;

            /* simple, not perfect, argument validation */
            if (ioarg <= 255)
            {
               RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);
            }
            /* use typecast specific for compat/non compat case) */
            if (iocmd == FIO_DXS_BBD_DOWNLOAD)
               size = sizeof(*(DXS_BBD_Download_t*)pBbdArg);
#ifdef TAPI_FEAT_LX_COMPAT
            else
               size = sizeof(*(DXS_BBD_Download_32_t*)pBbdArg);
#endif /* TAPI_FEAT_LX_COMPAT */

            /* get data from user space */
            pBbdArg = TAPI_OS_Malloc(size);
            if (NULL == pBbdArg)
            {
               RETURN_DEVSTATUS(DXS_statusNoMem, IFX_NULL);
            }

            if (TAPI_OS_CpyUsr2Kern(pBbdArg, (IFX_void_t *)ioarg, size) == IFX_NULL)
            {
               TAPI_OS_Free(pBbdArg);
               RETURN_DEVSTATUS(DXS_statusNoMem, IFX_NULL);
            }

            if (iocmd == FIO_DXS_BBD_DOWNLOAD)
            {
               devUsr = ((DXS_BBD_Download_t*)pBbdArg)->dev;
               chUsr = ((DXS_BBD_Download_t*)pBbdArg)->ch;
               bBroadcastUsr = ((DXS_BBD_Download_t*)pBbdArg)->bBroadcast;
            }
#ifdef TAPI_FEAT_LX_COMPAT
            else
            {
               devUsr = ((DXS_BBD_Download_32_t*)pBbdArg)->dev;
               chUsr = ((DXS_BBD_Download_32_t*)pBbdArg)->ch;
               bBroadcastUsr = ((DXS_BBD_Download_32_t*)pBbdArg)->bBroadcast;
            }
#endif /* TAPI_FEAT_LX_COMPAT */

            if (DXS_statusOk != DXS_GetDevice(devUsr, &pDev))
            {
               TAPI_OS_Free(pBbdArg);
               return DXS_statusDeviceIdErr;
            }
            /* if broadcast, then use this BBD for all channels */
            if (IFX_TRUE == bBroadcastUsr)
            {
               pCh = (DXS_CHANNEL_t *) pDev;
            }
            /* otherwise use only specified channel */
            else
            {
               /* get specific channels */
               if (DXS_MAX_CH_NR <= chUsr)
               {
                  TAPI_OS_Free(pBbdArg);
                  RETURN_DEVSTATUS(DXS_statusInvalCh, IFX_NULL);
               }
               pCh = &pDev->pChannel[chUsr];
            }
            TAPI_OS_Free(pBbdArg);
         }
         break;
      default:
         /* errmsg: Ioctl not supported. */
         RETURN_DEVSTATUS(DXS_statusNotSupported, IFX_NULL);
   }
#endif /* TAPI_ONE_DEVNODE */


   /* this block handles the initialization of the DUSLIC XS driver */
   switch (iocmd)
   {
      /* This must be the very first ioctl operation before hardware access. */
      ON_IOCTL_FRUSR (pDev, FIO_DXS_BASICDEV_INIT, DXS_ChipAccessInit,
                      DXS_BasicDeviceInit_t);

      /* This ioctl executes board specific DXS chip reset routine */
      ON_DIR_IOCTL   (pDev, FIO_DXS_CHIP_RESET, DXS_IOCTL_ChipReset,
                      (IFX_uint16_t) ioarg);
      /*
         This can be done to reset the device internal structure without doing
         any basic device initialization. Only the initialization state remains
         unchanged. All other states are reset.
      */
      case FIO_DXS_DEV_RESET:
         IFX_TAPI_DeviceReset (pDev->pTapiDev);
         return IFX_SUCCESS;

#ifndef TAPI_ONE_DEVNODE
      /* no basic driver initialization needed for these ioctls */
      case FIO_DXS_DRVVERS:
#ifndef LINUX
         ((DXS_IO_Version_t*)ioarg)->nDrvVers =
            (MAJORSTEP << 24 | MINORSTEP << 16 | VERSIONSTEP << 8 | VERS_TYPE);
         return IFX_SUCCESS;
#else
         return IFX_ERROR;
#endif /* LINUX */
#endif /* TAPI_ONE_DEVNODE */

      case FIO_DXS_REPORT_SET:
         return DXS_Report_Set((IFX_uint32_t)ioarg);

#ifdef DEBUG
      case FIO_DXS_WCMD:
      case FIO_DXS_RCMD:
      case FIO_DXS_WRREG:
      case FIO_DXS_RDREG:
         /* allow these ioctls even without basic device driver init */
         break;
#endif

      default:
         /* do not allow other ioctl if basic init is missing! */
          if ((pDev->nDevState & DS_BASIC_INIT) != DS_BASIC_INIT)
          {
             /* errmsg: Device not yet initialized. */
             RETURN_DEVSTATUS(DXS_statusNotInitialized, IFX_NULL);
          }
          break;
   }

   /*
      This code block handles ioctls which require previous basic device driver
      initialization.
   */
   switch (iocmd)
   {
#ifdef DEBUG
   #ifdef DXS_FEAT_IOCTL_RW_CMD
         ON_IOCTL_FRUSR (pDev, FIO_DXS_WCMD, DXS_IOCTL_CmdWrite,
                        DXS_IO_MB_CMD_t );
         ON_IOCTL       (pDev, FIO_DXS_RCMD, DXS_IOCTL_CmdRead,
                        DXS_IO_MB_CMD_t);
   #endif /* DXS_FEAT_IOCTL_RW_CMD */
   
   #ifdef DXS_FEAT_IOCTL_RW_REG
      ON_IOCTL_FRUSR (pDev, FIO_DXS_WRREG, DXS_IOCTL_RegWrite,
                      DXS_IO_RegAccess_t);
      ON_IOCTL       (pDev, FIO_DXS_RDREG, DXS_IOCTL_RegRead,
                      DXS_IO_RegAccess_t);
   #endif /* DXS_FEAT_IOCTL_RW_REG */
#endif /* DEBUG */

      ON_IOCTL       (pDev, FIO_DXS_VERS, DXS_Version,
                     DXS_IO_Version_t);
      ON_IOCTL       (pCh, FIO_DXS_BBD_DOWNLOAD, DXS_BBD_Download,
                      DXS_BBD_Download_t);
      ON_IOCTL       (pDev, FIO_DXS_FW_DOWNLOAD, DXS_DwldAndStartFW,
                      DXS_FW_Download_t);

#ifdef TAPI_FEAT_LX_COMPAT
      ON_IOCTL       (pCh, FIO_DXS_BBD_DOWNLOAD_32, DXS_BBD_Download_32,
                      DXS_BBD_Download_32_t);
      ON_IOCTL       (pDev, FIO_DXS_FW_DOWNLOAD_32, DXS_DwldAndStartFW_32,
                      DXS_FW_Download_32_t);
      ON_IOCTL       (pDev, FIO_DXS_INIT_32, DXS_FW_Start_32,
                      DXS_IO_Init_32_t);
#endif /* TAPI_FEAT_LX_COMPAT */

      /* already handled in first switch, if not handled again here an unknown
         ioctl would be traced */
      case FIO_DXS_BASICDEV_INIT:
      case FIO_DXS_CHIP_RESET:
         break;
      default:
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("Unknown IOCTL command 0x%08X or feature deactivated\n",iocmd));
         break;
   }

   return ret;
}

#ifdef DEBUG
#ifdef DXS_FEAT_IOCTL_RW_CMD
/**
   DUSLIC XS command write

   \param  pDev         Pointer to device struct.
   \param  pCmd         Pointer to struct with command data.

   \return
   - DXS_statusOk if no error
   - error code from DXS_CmdWrite otherwise
*/
static IFX_int32_t DXS_IOCTL_CmdWrite (
                        DXS_DEVICE_t *pDev,
                        DXS_IO_MB_CMD_t *pCmd)
{
   IFX_int32_t  ret;
   IFX_uint32_t *pData;

   /* validate parameter */
   if (IFX_NULL == pCmd)
      RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);

#if TAPI_BYTE_ORDER == TAPI_LITTLE_ENDIAN
   {
      /* Pointer to array with commands and data, without device index. */
      IFX_uint16_t (*commands_and_data)[DXS_IO_MB_CMD_SIZE] =
            (IFX_uint16_t (*)[DXS_IO_MB_CMD_SIZE]) &pCmd->cmd1;

      DXS_IOCTL_CommandWordSwap(commands_and_data);
   }
#endif /* TAPI_BYTE_ORDER == TAPI_LITTLE_ENDIAN */

   /* pCmd is valid (not IFX_NULL) */
   pData = (IFX_uint32_t *)&pCmd->cmd1;

   ret = DXS_CmdWrite (pDev, pData);

   RETURN_DEVSTATUS(ret, IFX_NULL);
}

/**
   DUSLIC XS command read

   \param  pDev         Pointer to device struct.
   \param  pCmd         Pointer to struct with command data.

   \return
   - DXS_statusOk if no error
   - error code from DXS_CmdRead otherwise
*/
static IFX_int32_t DXS_IOCTL_CmdRead (
                        DXS_DEVICE_t *pDev,
                        DXS_IO_MB_CMD_t *pCmd)
{
   IFX_int32_t ret;
   IFX_uint32_t *pData;

   /* validate parameter */
   if (IFX_NULL == pCmd)
      RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);

#if TAPI_BYTE_ORDER == TAPI_LITTLE_ENDIAN
   {
      /* Pointer to array with commands and data, without device index. */
      IFX_uint16_t (*commands_and_data)[DXS_IO_MB_CMD_SIZE] =
            (IFX_uint16_t (*)[DXS_IO_MB_CMD_SIZE]) &pCmd->cmd1;

      DXS_IOCTL_CommandWordSwap(commands_and_data);
   }
#endif /* TAPI_BYTE_ORDER == TAPI_LITTLE_ENDIAN */

   pData = (IFX_uint32_t *)&pCmd->cmd1;

   TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("Command read: %08X\n", *pData));

   ret = DXS_CmdRead (pDev, pData, pData);

   TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("Command data: %08X\n", *pData));

#if TAPI_BYTE_ORDER == TAPI_LITTLE_ENDIAN
   {
      /* Pointer to array with commands and data, without device index. */
      IFX_uint16_t (*commands_and_data)[DXS_IO_MB_CMD_SIZE] =
            (IFX_uint16_t (*)[DXS_IO_MB_CMD_SIZE]) pData;

      /* Swap response back */
      DXS_IOCTL_CommandWordSwap(commands_and_data);
   }
#endif /* TAPI_BYTE_ORDER == TAPI_LITTLE_ENDIAN */

   /* if read failed set the cmd length to 0 */
   if (ret != IFX_SUCCESS)
      pCmd->cmd2 = 0;

   RETURN_DEVSTATUS(ret, IFX_NULL);
}
#endif /* DXS_FEAT_IOCTL_RW_CMD */
#endif /* DEBUG */

#ifdef DEBUG
#ifdef DXS_FEAT_IOCTL_RW_REG
/**
   IOCTL handling function used to write a single host register.

   \param  pDev         Pointer to device struct.
   \param  pCmd         Pointer to struct with register details.

   \return
   - DXS_statusOk if no error
   - error code from DXS_RegWriteMulti otherwise
*/
static IFX_int32_t DXS_IOCTL_RegWrite (
                        DXS_DEVICE_t *pDev,
                        DXS_IO_RegAccess_t *pCmd)
{
   IFX_int32_t ret = IFX_ERROR;

   /* validate parameter */
   if (IFX_NULL == pCmd || (!pCmd->count) || (pCmd->count & 1))
      RETURN_DEVSTATUS(DXS_statusParam,IFX_NULL);

   if (pCmd->count == 2)
      ret = DXS_RegWrite(pDev, pCmd->offset, pCmd->pData[0]);
   else
      ret = DXS_RegWriteMulti(pDev, pCmd->offset, pCmd->pData,
                                   (pCmd->count >> 1));

   RETURN_DEVSTATUS(ret, IFX_NULL);
}

/**
   Ioctl handling function used to read a single host register.

   \param  pDev         Pointer to device struct.
   \param  pCmd         Pointer to struct with register details.

   \return
   - DXS_statusOk if no error
   - error code from DXS_RegReadMulti otherwise
*/
static IFX_int32_t DXS_IOCTL_RegRead (
                        DXS_DEVICE_t *pDev,
                        DXS_IO_RegAccess_t *pCmd)
{
   IFX_int32_t ret = IFX_ERROR;

   /* validate parameter */
   if (IFX_NULL == pCmd || (!pCmd->count) || (pCmd->count & 1))
      RETURN_DEVSTATUS(DXS_statusParam,IFX_NULL);

   if (pCmd->count == 2)
      ret = DXS_RegRead(pDev, pCmd->offset, pCmd->pData);
   else
      ret = DXS_RegReadMulti(pDev, pCmd->offset, pCmd->pData,
                                  (pCmd->count >> 1));

   RETURN_DEVSTATUS(ret, IFX_NULL);
}
#endif /* DXS_FEAT_IOCTL_RW_REG */
#endif /* DEBUG */
/**
   Ioctl handling function used to execute board specific chip reset.

   \param pDev pointer to device data

   \param reset 0=deactivate 1=activate

   \return
   returns IFX_SUCCESS, always
*/
static IFX_int32_t DXS_IOCTL_ChipReset (DXS_DEVICE_t *pDev,
                                        IFX_uint16_t reset)
{
   /* Mark as unused in case the macro is not there. */
   TAPI_UNUSED (pDev);
   TAPI_UNUSED (reset);

   tapi_debug_buffer_add_user_entry("dxs #%d chip reset", pDev->nDevNr);

   /* This macro is defined in drv_config_user.h */
#ifdef CHIP_RESET
   CHIP_RESET (pDev, reset);
#endif /* #ifdef CHIP_RESET */

   return IFX_SUCCESS;
}


/**
   Get the DUSLIC XS Chip Version

   \param  pDev         Pointer to the device interface.
   \param  pInd         Pointer to the user interface.

   \return
   DXS_statusOk or error code.
*/
IFX_int32_t DXS_Version (DXS_DEVICE_t *pDev, DXS_IO_Version_t *pInd)
{
   /* validate parameter */
   if (IFX_NULL == pInd)
      RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);

   if (!(pDev->nDevState & DS_DEV_INIT))
   {
      /* While DevStart was not done read the version every time directly
         from the firmware. */
      IFX_int32_t err = DXS_ReadFwVersion(pDev);

      if (DXS_statusOk != err)
         RETURN_DEVSTATUS(err, IFX_NULL);
   }

   pInd->nHwRev    = pDev->nChipRev;
   pInd->nDevID    = pDev->nDevId;
   pInd->nFwRev    = pDev->nFwRev;
   pInd->nAsdspRev = 0;

   /* Set number of supported FXS channels. LSB of DevId indicates the number
      of channels where 0 is a 2-channel device. */
   if ((pInd->nDevID & 0x01) == 0)
   {
      /* 2-channel device */
      pInd->nFxsCh = 0x2;
      pInd->nFxoCh = 0x0;
   }
   else
   {
      /* 1-channel device */
      pInd->nFxsCh = 0x1;
      pInd->nFxoCh = 0x0;
   }

   /* set device index */
   pInd->dev = pDev->nDevNr;

   /* set driver version */
   pInd->nDrvVers = DXS_MAJORSTEP << 24 | DXS_MINORSTEP << 16 |
                    DXS_VERSIONSTEP << 8 | DXS_VERS_TYPE;

   return DXS_statusOk;
}
