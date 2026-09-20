/******************************************************************************

  Copyright 2018, Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_compat.c
   This file implements functions used to translate the arguments of
   IOCTLs from 32-bit architecture to 64-bit architecture. This is used on
   Linux systems running with 32-bit user-space and 64-bit kernel-space
   architecture.
*/

#include "drv_tapi_config.h"
#ifdef TAPI_FEAT_LX_COMPAT

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_tapi_compat.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

void static event32to64 (struct IFX_TAPI_EVENT_32_t *pEv32,
                         IFX_TAPI_EVENT_t *pEv64)
{
   pEv64->dev = pEv32->dev;
   pEv64->ch = pEv32->ch;
   pEv64->module = pEv32->module;
   pEv64->more = pEv32->more;
   pEv64->reserved = pEv32->reserved;
   pEv64->id = pEv32->id;
   pEv64->data = *((IFX_TAPI_EVENT_DATA_t*)(&pEv32->data));
}


static IFX_int32_t convertEventMulti32to64 (struct IFX_TAPI_EVENT_MULTI_32_t *pEv32,
                                          IFX_TAPI_EVENT_MULTI_t *pEv64)
{
   IFX_uint32_t  size, size_64;
   IFX_int32_t ret   = TAPI_statusOk;
   /* temporary dynamic buffer for 32 bit message */
   struct IFX_TAPI_EVENT_ENTRY_32_t *pEvent;

   pEv64->dev = pEv32->dev;
   pEv64->ch = pEv32->ch;
#ifdef TAPI_VERSION4
   pEv64->module = pEv32->module;
#endif /* TAPI_VERSION4 */
   pEv64->nCount = pEv32->nCount;

   /* allocate 32bit aligned buffer and copy data from user space */
   size = pEv32->nCount * sizeof(struct IFX_TAPI_EVENT_ENTRY_32_t);
   pEvent = TAPI_OS_Malloc (size);
   if (pEvent == IFX_NULL)
   {
      ret =TAPI_statusNoMem;
   }
   if (TAPI_SUCCESS(ret))
   {
      if (!TAPI_OS_CpyUsr2Kern (pEvent, compat_ptr(pEv32->pEvent), size))
      {
         TAPI_OS_Free (pEvent);
         ret = TAPI_statusErrKernCpy;
      }
   }

   if (TAPI_SUCCESS(ret))
   {
      /* allocate 64b aligned buffer */
      size_64 = pEv32->nCount * sizeof(IFX_TAPI_EVENT_ENTRY_t);
      pEv64->pEvent = TAPI_OS_Malloc (size_64);
      if (pEv64->pEvent == IFX_NULL)
      {
         ret =TAPI_statusNoMem;
      }
   }

   /* copy data from 32b to 64b buffer */
   if (TAPI_SUCCESS(ret))
   {
     IFX_uint8_t i;
     for (i =0; i < pEv32->nCount; i++)
     {
       (pEv64->pEvent)[i].id = ((struct IFX_TAPI_EVENT_ENTRY_32_t*)pEvent)[i].id;
       memcpy(&((pEv64->pEvent)[i].data), &(((struct IFX_TAPI_EVENT_ENTRY_32_t*)pEvent)[i].data), sizeof((pEv64->pEvent)[i].data));
     }
   }

   /* release 32b buffer */
   TAPI_OS_Free (pEvent);

   return ret;

}


/**
   Linux compat function for IFX_TAPI_Phone_Init()

   \param  pTapiDev     Pointer to TAPI device structure.
   \param  pInit_32     Pointer to struct IFX_TAPI_CH_INIT_32_t.

   \return
   - Value TAPI_statusParam if argument is a pointer to NULL.
   - Returnvalue from IFX_TAPI_Phone_Init() function.
*/
IFX_int32_t IFX_TAPI_Phone_Init_32(
                        TAPI_DEV *pTapiDev,
                        struct IFX_TAPI_CH_INIT_32_t const *pInit_32)
{
   IFX_TAPI_CH_INIT_t init_64;

   if (pInit_32 == IFX_NULL)
   {
      RETURN_DEVSTATUS (TAPI_statusParam, 0);
   }

   memset(&init_64, 0x00, sizeof(init_64));
   init_64.dev = pInit_32->dev;
   init_64.ch = pInit_32->ch;
   init_64.nMode = pInit_32->nMode;
   init_64.nCountry = pInit_32->nCountry;
   init_64.pProc = compat_ptr(pInit_32->pProc);

   /* Cast adds const to the argument. */
   return IFX_TAPI_Phone_Init(pTapiDev, (IFX_TAPI_CH_INIT_t const *)&init_64);
}


/**
   Linux compat function for IFX_TAPI_Cap_List_Get()

   \param  pTapiDev     Pointer to the TAPI device structure.
   \param  pCapList     Pointer to struct with list and number of elements.

   \return
   - Value TAPI_statusParam if argument is a pointer to NULL.
   - Returnvalue from IFX_TAPI_Cap_List_Get() function.
*/
IFX_int32_t IFX_TAPI_Cap_List_Get_32 (
                        TAPI_DEV *pTapiDev,
                        struct IFX_TAPI_CAP_LIST_32_t *pCapList_32)
{
   IFX_TAPI_CAP_LIST_t cap_list_64;
   IFX_int32_t ret;

   if (pCapList_32 == IFX_NULL)
   {
      RETURN_DEVSTATUS (TAPI_statusParam, 0);
   }

   memset(&cap_list_64, 0, sizeof(cap_list_64));
   cap_list_64.dev = pCapList_32->dev;
   cap_list_64.nCap = pCapList_32->nCap;
   cap_list_64.pList = compat_ptr(pCapList_32->pList);

   ret = IFX_TAPI_Cap_List_Get (pTapiDev, &cap_list_64);

   pCapList_32->nCap = cap_list_64.nCap;

   return ret;
}


/**
   Translate arguments for function TAPI_Event_Get() function.

   \param  pDrvCtx      Pointer to driver context structure.
   \param  pEvent       Pointer to struct IFX_TAPI_EVENT_32_t.

   \return
   Return value of TAPI_Event_Get() function.
*/
IFX_int32_t TAPI_Event_Get_32 (IFX_TAPI_DRV_CTX_t *pDrvCtx,
                        struct IFX_TAPI_EVENT_32_t *pEvent)
{
  IFX_int32_t ret;
  IFX_TAPI_EVENT_t event_64;

  memset(&event_64, 0x00, sizeof(event_64));

  /* input parameters needed by TAPI_Event_Get() */
  event_64.dev = pEvent->dev;
  event_64.ch = pEvent->ch;

  ret = TAPI_Event_Get (pDrvCtx, &event_64);

  pEvent->dev = event_64.dev;
  pEvent->ch = event_64.ch;
  pEvent->module = event_64.module;
  pEvent->more = event_64.more;
  pEvent->reserved = event_64.reserved;
  pEvent->id = event_64.id;
  pEvent->data =  *((union IFX_TAPI_EVENT_DATA_32_t*)(&event_64.data));

  return ret;
}


/**
   Translate arguments for function IOCTL_TAPI_EventEnable.

   \param  pDrvCtx      Pointer to driver context structure.
   \param  pEvent       Pointer to struct IFX_TAPI_EVENT_32_t.

   \return
   Return value of IOCTL_TAPI_EventEnable() function.
*/
IFX_int32_t IOCTL_TAPI_EventEnable_32 (IFX_TAPI_DRV_CTX_t *pDrvCtx,
                        struct IFX_TAPI_EVENT_32_t *pEvent)
{
   IFX_TAPI_EVENT_t event_64;

   memset(&event_64, 0x00, sizeof(event_64));
   event32to64(pEvent, &event_64);
   return IOCTL_TAPI_EventEnable(pDrvCtx, &event_64);
}


/**
   Translate arguments for function IOCTL_TAPI_EventDisable.

   \param  pDrvCtx      Pointer to driver context structure.
   \param  pEvent       Pointer to struct IFX_TAPI_EVENT_32_t.

   \return
   Return value of IOCTL_TAPI_EventDisable() function.
*/
IFX_int32_t IOCTL_TAPI_EventDisable_32 (IFX_TAPI_DRV_CTX_t *pDrvCtx,
                        struct IFX_TAPI_EVENT_32_t *pEvent)
{
   IFX_TAPI_EVENT_t event_64;

   memset(&event_64, 0x00, sizeof(event_64));
   event32to64(pEvent, &event_64);
   return IOCTL_TAPI_EventDisable(pDrvCtx, &event_64);
}


static void freeEventMulti64(IFX_TAPI_EVENT_MULTI_t *pEvent64)
{
  /* release multi part message buffer */
  TAPI_OS_Free(pEvent64->pEvent);
}


/**
   Translate arguments for function TAPI_EventMultiState_set.

   \param  pDrvCtx      Pointer to driver context structure.
   \param  pEvent       Pointer to struct IFX_TAPI_EVENT_MULTI_32_t.
   \param  nEvtState    Event reporting state
                        -IFX_EVENT_ENABLE to allow the event to the application
                        -IFX_EVENT_DISABLE to discard the event

   \return
   Return value of TAPI_EventMultiState_Set function.
*/
static IFX_int32_t eventMultiState_Set (IFX_TAPI_DRV_CTX_t *pDrvCtx,
                        struct IFX_TAPI_EVENT_MULTI_32_t *pEvent,
                        IFX_uint32_t nEvtState)
{
   IFX_TAPI_EVENT_MULTI_t event_64;
   IFX_boolean_t compatIoctl = IFX_TRUE;
   IFX_int32_t ret           = TAPI_statusOk;

   memset(&event_64, 0x00, sizeof(event_64));
   ret = convertEventMulti32to64(pEvent, &event_64);

   if (TAPI_SUCCESS(ret))
   {
      ret = TAPI_EventMultiState_Set(pDrvCtx, &event_64, nEvtState,
                                 (IFX_boolean_t const *)&compatIoctl);
   }

   freeEventMulti64(&event_64);
   return ret;
}

/**
   Translate arguments for function TAPI_EventMultiEnable.

   \param  pDrvCtx      Pointer to driver context structure.
   \param  pEvent       Pointer to struct IFX_TAPI_EVENT_MULTI_32_t.

   \return
   Return value of IOCTL_TAPI_EventMultiEnable() function.
*/
IFX_int32_t TAPI_EventMultiEnable_32 (IFX_TAPI_DRV_CTX_t *pDrvCtx,
                        struct IFX_TAPI_EVENT_MULTI_32_t *pEvent)
{
   return eventMultiState_Set(pDrvCtx, pEvent, IFX_EVENT_ENABLE);
}


/**
   Translate arguments for function TAPI_EventMultiDisable.

   \param  pDrvCtx      Pointer to driver context structure.
   \param  pEvent       Pointer to struct IFX_TAPI_EVENT_MULTI_32_t.

   \return
   Return value of IOCTL_TAPI_EventMultiDisable() function.
*/
IFX_int32_t TAPI_EventMultiDisable_32 (IFX_TAPI_DRV_CTX_t *pDrvCtx,
                        struct IFX_TAPI_EVENT_MULTI_32_t *pEvent)
{
   return eventMultiState_Set(pDrvCtx, pEvent, IFX_EVENT_DISABLE);
}


#ifdef TAPI_FEAT_CID
static IFX_int32_t convertCid32to64(struct IFX_TAPI_CID_MSG_32_t const *pCidInfo32,
                                    IFX_TAPI_CID_MSG_t *pCidInfo64)
{
   IFX_uint32_t size;
   IFX_int32_t ret   = TAPI_statusOk;
   /* temporary dynamic buffer for 32 bit message */
   union IFX_TAPI_CID_MSG_ELEMENT_32_t *message;
   pCidInfo64->dev = pCidInfo32->dev;
   pCidInfo64->ch = pCidInfo32->ch;
   pCidInfo64->txMode = pCidInfo32->txMode;
   pCidInfo64->messageType = pCidInfo32->messageType;
   pCidInfo64->nMsgElements = pCidInfo32->nMsgElements;

   /* allocate 32bit aligned buffer and copy data from user space */
   size = pCidInfo32->nMsgElements * sizeof(union IFX_TAPI_CID_MSG_ELEMENT_32_t);
   message = TAPI_OS_Malloc (size);
   if (message == IFX_NULL)
   {
      ret =TAPI_statusNoMem;
   }

   if (TAPI_SUCCESS(ret))
   {
      if (!TAPI_OS_CpyUsr2Kern (message, compat_ptr(pCidInfo32->message), size))
      {
         TAPI_OS_Free (message);
         ret = TAPI_statusErrKernCpy;
      }
   }

   if (TAPI_SUCCESS(ret))
   {
      /* allocate 64b aligned buffer */
      IFX_uint32_t size_64 = pCidInfo32->nMsgElements *
                             sizeof(IFX_TAPI_CID_MSG_ELEMENT_t);
      pCidInfo64->message = TAPI_OS_Malloc (size_64);
      if (pCidInfo64->message == IFX_NULL)
      {
         ret =TAPI_statusNoMem;
      }
   }

   /* copy data from 32b to 64b buffer */
   if (TAPI_SUCCESS(ret))
   {
     IFX_uint8_t i;
     for (i =0; i < pCidInfo32->nMsgElements; i++)
     {
        memcpy(&(pCidInfo64->message)[i], &message[i], sizeof((pCidInfo64->message)[i]));
        /* allocate string buffer for transparent message  */
        if (((pCidInfo64->message)[i]).transparent.elementType == IFX_TAPI_CID_ST_TRANSPARENT)
        {
           IFX_uint32_t len = ((pCidInfo64->message)[i]).transparent.len;
           IFX_uint8_t *data = TAPI_OS_Malloc(len);
           if (data == IFX_NULL)
           {
              ret = TAPI_statusNoMem;
              break;
           }

           ((pCidInfo64->message)[i]).transparent.data = data;
           /* copy data from userspace */
           if (!TAPI_OS_CpyUsr2Kern (data, compat_ptr((message[i]).transparent.data), len))
           {
              TAPI_OS_Free (data);
              ret = TAPI_statusErrKernCpy;
              break;
           }
        }
     }
   }

   /* release 32b buffer */
   TAPI_OS_Free (message);

   return ret ;
}


static void freeCid64(IFX_TAPI_CID_MSG_t *pCidInfo64)
{
  IFX_uint8_t i;

  /* release string buffers for transparent messages  */
  for (i = 0; i < pCidInfo64->nMsgElements; i++)
  {
     if (((pCidInfo64->message)[i]).transparent
         .elementType == IFX_TAPI_CID_ST_TRANSPARENT)
     {
        TAPI_OS_Free(((pCidInfo64->message)[i]).transparent.data);
     }
  }

  /* release multi part message buffer */
  TAPI_OS_Free(pCidInfo64->message);
}


/**
   Translate arguments for function TAPI_Phone_CID_Info_Tx.

   \param  pChannel     Pointer to channel.
   \param  pCidInfo     Pointer to struct  IFX_TAPI_CID_MSG_32_t.

   \return
   Return value of TAPI_Phone_CID_Info_Tx() function.
*/
IFX_int32_t TAPI_Phone_CID_Info_Tx_32 (TAPI_CHANNEL *pChannel,
                        struct IFX_TAPI_CID_MSG_32_t const *pCidInfo)
{
   IFX_TAPI_CID_MSG_t cidInfo_64;
   IFX_boolean_t compatIoctl = IFX_TRUE;
   IFX_int32_t ret           = TAPI_statusOk;

   memset(&cidInfo_64, 0x00, sizeof(cidInfo_64));
   convertCid32to64(pCidInfo, &cidInfo_64);

   if (TAPI_SUCCESS(ret))
   {
      ret = TAPI_Phone_CID_Info_Tx_Base(pChannel,
                                        (IFX_TAPI_CID_MSG_t const *)&cidInfo_64,
                                        (IFX_boolean_t const *)&compatIoctl);
   }

   freeCid64(&cidInfo_64);

   RETURN_STATUS (ret, 0);
}


/**
   Translate arguments for function TAPI_Phone_CID_Seq_Tx.

   \param  pChannel     Pointer to channel.
   \param  pCidInfo     Pointer to struct  IFX_TAPI_CID_MSG_32_t.

   \return
   Return value of TAPI_Phone_CID_Seq_Tx() function.
*/
IFX_int32_t TAPI_Phone_CID_Seq_Tx_32 (TAPI_CHANNEL *pChannel,
                        struct IFX_TAPI_CID_MSG_32_t const *pCidInfo)
{
   IFX_TAPI_CID_MSG_t cidInfo_64;
   IFX_boolean_t compatIoctl = IFX_TRUE;
   IFX_int32_t ret           = TAPI_statusOk;

   memset(&cidInfo_64, 0x00, sizeof(cidInfo_64));
   ret = convertCid32to64(pCidInfo, &cidInfo_64);


   if (TAPI_SUCCESS(ret))
   {
      ret = TAPI_Phone_CID_Seq_Tx_Base(pChannel,
                                       (IFX_TAPI_CID_MSG_t const *)&cidInfo_64,
                                       (IFX_boolean_t const *)&compatIoctl);
   }

   freeCid64(&cidInfo_64);

   RETURN_STATUS (ret, 0);

}

/**
   Translate arguments for function TAPI_Phone_CID_SetConfig.

   \param  pChannel     Pointer to channel.
   \param  pCidInfo     Pointer to struct  IFX_TAPI_CID_CFG_32_t.

   \return
   Return value.
*/
IFX_int32_t TAPI_Phone_CID_SetConfig_32 (TAPI_CHANNEL *pChannel,
                        struct IFX_TAPI_CID_CFG_32_t const *pCidConf)
{
   union IFX_TAPI_CID_STD_TYPE_32_t cidType;
   IFX_TAPI_CID_CFG_t         cidConf_64;
   IFX_TAPI_CID_STD_TYPE_t    cidType_64;
   IFX_TAPI_CID_TIMING_t      cidTiming_64;
   IFX_TAPI_CID_FSK_CFG_t     fskConf_64;
   IFX_TAPI_CID_DTMF_CFG_t    dtmfConf_64;
   IFX_TAPI_CID_ABS_REASON_t  absCLICode_64;
   IFX_boolean_t compatIoctl = IFX_TRUE;
   IFX_int32_t ret           = TAPI_statusOk;

   memset(&cidConf_64, 0x00, sizeof(cidConf_64));
   cidConf_64.dev = pCidConf->dev;
   cidConf_64.ch = pCidConf->ch;
   cidConf_64.nStandard = pCidConf->nStandard;
   cidConf_64.cfg = &cidType_64;


   if (compat_ptr(pCidConf->cfg) != IFX_NULL)
   {
      switch (pCidConf->nStandard)
      {
      case IFX_TAPI_CID_STD_TELCORDIA:
         if (!TAPI_OS_CpyUsr2Kern (&cidType.telcordia, compat_ptr(pCidConf->cfg), sizeof(cidType.telcordia)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&cidTiming_64, compat_ptr(cidType.telcordia.pCIDTiming), sizeof(cidTiming_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&fskConf_64, compat_ptr(cidType.telcordia.pFSKConf), sizeof(fskConf_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }

         cidConf_64.cfg->telcordia.pCIDTiming = &cidTiming_64;
         cidConf_64.cfg->telcordia.pFSKConf = &fskConf_64;
         cidConf_64.cfg->telcordia.OSIoffhook = cidType.telcordia.OSIoffhook;
         cidConf_64.cfg->telcordia.OSItime = cidType.telcordia.OSItime;
         cidConf_64.cfg->telcordia.nAlertToneOnhook = cidType.telcordia.nAlertToneOnhook;
         cidConf_64.cfg->telcordia.nAlertToneOffhook = cidType.telcordia.nAlertToneOffhook;
         cidConf_64.cfg->telcordia.ackTone = cidType.telcordia.ackTone;
         cidConf_64.cfg->telcordia.nSAStone = cidType.telcordia.nSAStone;
         cidConf_64.cfg->telcordia.beforeSAStime = cidType.telcordia.beforeSAStime;
         cidConf_64.cfg->telcordia.SAS2CAStime = cidType.telcordia.SAS2CAStime;
         break;
      case IFX_TAPI_CID_STD_ETSI_FSK:
         if (!TAPI_OS_CpyUsr2Kern (&cidType.etsiFSK, compat_ptr(pCidConf->cfg), sizeof(cidType.etsiFSK)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&cidTiming_64, compat_ptr(cidType.etsiFSK.pCIDTiming), sizeof(cidTiming_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&fskConf_64, compat_ptr(cidType.etsiFSK.pFSKConf), sizeof(fskConf_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }

         cidConf_64.cfg->etsiFSK.pCIDTiming = &cidTiming_64;
         cidConf_64.cfg->etsiFSK.pFSKConf = &fskConf_64;
         cidConf_64.cfg->etsiFSK.nETSIAlertRing = cidType.etsiFSK.nETSIAlertRing;
         cidConf_64.cfg->etsiFSK.nETSIAlertNoRing = cidType.etsiFSK.nETSIAlertNoRing;
         cidConf_64.cfg->etsiFSK.nAlertToneOnhook = cidType.etsiFSK.nAlertToneOnhook;
         cidConf_64.cfg->etsiFSK.nAlertToneOffhook = cidType.etsiFSK.nAlertToneOffhook;
         cidConf_64.cfg->etsiFSK.ringPulseTime = cidType.etsiFSK.ringPulseTime;
         cidConf_64.cfg->etsiFSK.ackTone = cidType.etsiFSK.ackTone;
         break;
      case IFX_TAPI_CID_STD_ETSI_DTMF:
         if (!TAPI_OS_CpyUsr2Kern (&cidType.etsiDTMF, compat_ptr(pCidConf->cfg), sizeof(cidType.etsiDTMF)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&cidTiming_64, compat_ptr(cidType.etsiDTMF.pCIDTiming), sizeof(cidTiming_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&dtmfConf_64, compat_ptr(cidType.etsiDTMF.pDTMFConf), sizeof(dtmfConf_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&absCLICode_64, compat_ptr(cidType.etsiDTMF.pABSCLICode), sizeof(absCLICode_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }

         cidConf_64.cfg->etsiDTMF.pCIDTiming = &cidTiming_64;
         cidConf_64.cfg->etsiDTMF.pDTMFConf = &dtmfConf_64;
         cidConf_64.cfg->etsiDTMF.pABSCLICode = &absCLICode_64;
         cidConf_64.cfg->etsiDTMF.nETSIAlertRing = cidType.etsiDTMF.nETSIAlertRing;
         cidConf_64.cfg->etsiDTMF.nETSIAlertNoRing = cidType.etsiDTMF.nETSIAlertNoRing;
         cidConf_64.cfg->etsiDTMF.nAlertToneOnhook = cidType.etsiDTMF.nAlertToneOnhook;
         cidConf_64.cfg->etsiDTMF.nAlertToneOffhook = cidType.etsiDTMF.nAlertToneOffhook;
         cidConf_64.cfg->etsiDTMF.ringPulseTime = cidType.etsiDTMF.ringPulseTime;
         cidConf_64.cfg->etsiDTMF.ackTone = cidType.etsiDTMF.ackTone;
         break;
      case IFX_TAPI_CID_STD_SIN:
         if (!TAPI_OS_CpyUsr2Kern (&cidType.sin, compat_ptr(pCidConf->cfg), sizeof(cidType.sin)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&cidTiming_64, compat_ptr(cidType.sin.pCIDTiming), sizeof(cidTiming_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&fskConf_64, compat_ptr(cidType.sin.pFSKConf), sizeof(fskConf_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }

         cidConf_64.cfg->sin.pCIDTiming = &cidTiming_64;
         cidConf_64.cfg->sin.pFSKConf = &fskConf_64;
         cidConf_64.cfg->sin.nAlertToneOnhook = cidType.sin.nAlertToneOnhook;
         cidConf_64.cfg->sin.nAlertToneOffhook = cidType.sin.nAlertToneOffhook;
         cidConf_64.cfg->sin.ackTone = cidType.sin.ackTone;
         break;
      case IFX_TAPI_CID_STD_NTT:
         if (!TAPI_OS_CpyUsr2Kern (&cidType.ntt, compat_ptr(pCidConf->cfg), sizeof(cidType.ntt)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&cidTiming_64, compat_ptr(cidType.ntt.pCIDTiming), sizeof(cidTiming_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&fskConf_64, compat_ptr(cidType.ntt.pFSKConf), sizeof(fskConf_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }

         cidConf_64.cfg->ntt.pCIDTiming = &cidTiming_64;
         cidConf_64.cfg->ntt.pFSKConf = &fskConf_64;
         cidConf_64.cfg->ntt.nAlertToneOnhook = cidType.ntt.nAlertToneOnhook;
         cidConf_64.cfg->ntt.nAlertToneOffhook = cidType.ntt.nAlertToneOffhook;
         cidConf_64.cfg->ntt.ringPulseTime = cidType.ntt.ringPulseTime;
         cidConf_64.cfg->ntt.ringPulseLoop = cidType.ntt.ringPulseLoop;
         cidConf_64.cfg->ntt.ringPulseOffTime = cidType.ntt.ringPulseOffTime;
         cidConf_64.cfg->ntt.dataOut2incomingSuccessfulTimeout = cidType.ntt.dataOut2incomingSuccessfulTimeout;
         break;
      /* KPN_DTMF and KPN_DTMF_FSK use identical structures */
      case IFX_TAPI_CID_STD_KPN_DTMF:
      case IFX_TAPI_CID_STD_KPN_DTMF_FSK:
         if (!TAPI_OS_CpyUsr2Kern (&cidType.kpnDTMF, compat_ptr(pCidConf->cfg), sizeof(cidType.kpnDTMF)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&cidTiming_64, compat_ptr(cidType.kpnDTMF.pCIDTiming), sizeof(cidTiming_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&dtmfConf_64, compat_ptr(cidType.kpnDTMF.pDTMFConf), sizeof(dtmfConf_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&absCLICode_64, compat_ptr(cidType.kpnDTMF.pABSCLICode), sizeof(absCLICode_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&fskConf_64, compat_ptr(cidType.kpnDTMF.pFSKConf), sizeof(fskConf_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }

         cidConf_64.cfg->kpnDTMF.pCIDTiming = &cidTiming_64;
         cidConf_64.cfg->kpnDTMF.pDTMFConf = &dtmfConf_64;
         cidConf_64.cfg->kpnDTMF.pABSCLICode = &absCLICode_64;
         cidConf_64.cfg->kpnDTMF.pFSKConf = &fskConf_64;
         cidConf_64.cfg->kpnDTMF.nAlertToneOffhook = cidType.kpnDTMF.nAlertToneOffhook;
         cidConf_64.cfg->kpnDTMF.ackTone = cidType.kpnDTMF.ackTone;
         break;
      case IFX_TAPI_CID_STD_TDC:
         if (!TAPI_OS_CpyUsr2Kern (&cidType.tdc, compat_ptr(pCidConf->cfg), sizeof(cidType.tdc)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&cidTiming_64, compat_ptr(cidType.tdc.pCIDTiming), sizeof(cidTiming_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&dtmfConf_64, compat_ptr(cidType.tdc.pDTMFConf), sizeof(dtmfConf_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }
         if (!TAPI_OS_CpyUsr2Kern (&absCLICode_64, compat_ptr(cidType.tdc.pABSCLICode), sizeof(absCLICode_64)))
         {
            ret = TAPI_statusErrKernCpy;
            break;
         }

         cidConf_64.cfg->tdc.pCIDTiming = &cidTiming_64;
         cidConf_64.cfg->tdc.pDTMFConf = &dtmfConf_64;
         cidConf_64.cfg->tdc.pABSCLICode = &absCLICode_64;
         break;
      default:
         ret = TAPI_statusErr;
         break;
      }
   }

   if (TAPI_SUCCESS(ret))
   {
      ret = TAPI_Phone_CID_SetConfig_Base(pChannel, (const IFX_TAPI_CID_CFG_t *)&cidConf_64,
                                          (IFX_boolean_t const *)&compatIoctl);
   }

   RETURN_STATUS (ret, 0);
}
#endif /* TAPI_FEAT_CID */


#ifdef TAPI_FEAT_NLT
/**
   Translate arguments for IFX_TAPI_NLT_Test_Start

   \param   pChannel    Handle to the TAPI channel
   \param   pTestParam  Params for test to be run

   \return
      TAPI_statusOk - success
      TAPI_statusLLNotSupp - service not supported by low-level driver
      TAPI_statusLLFailed - failed low-level call
*/
IFX_int32_t IFX_TAPI_NLT_Test_Start_32 (TAPI_CHANNEL *pChannel,
   struct IFX_TAPI_NLT_TEST_START_32_t *pTestParam)
{
   IFX_TAPI_NLT_TEST_START_t testParam_64;
   IFX_int32_t ret;

   memset(&testParam_64, 0x00, sizeof(testParam_64));
   testParam_64.dev = pTestParam->dev;
   testParam_64.ch = pTestParam->ch;
   testParam_64.bBlock = pTestParam->bBlock;
   testParam_64.testID = pTestParam->testID;
   testParam_64.pTestCfg = compat_ptr(pTestParam->pTestCfg);

   ret = IFX_TAPI_NLT_Test_Start(pChannel, &testParam_64);
   /* nStatus is output parameter */
   pTestParam->nStatus = testParam_64.nStatus;

   return ret;
}


/**
   Translate arguments for IFX_TAPI_NLT_Result_Get

   \param   pChannel      Handle to the TAPI channel
   \param   pTestResults  Test results

   \return
      TAPI_statusOk - success
      TAPI_statusLLNotSupp - service not supported by low-level driver
      TAPI_statusLLFailed - failed low-level call
*/
IFX_int32_t IFX_TAPI_NLT_Result_Get_32 (TAPI_CHANNEL *pChannel,
   struct IFX_TAPI_NLT_RESULT_GET_32_t *pTestResult)
{
   IFX_TAPI_NLT_RESULT_GET_t testResult_64;
   IFX_int32_t ret;

   memset(&testResult_64, 0x00, sizeof(testResult_64));
   testResult_64.dev = pTestResult->dev;
   testResult_64.ch = pTestResult->dev;
   testResult_64.testID = pTestResult->testID;
   testResult_64.pTestCfg = compat_ptr(pTestResult->pTestCfg);
   testResult_64.pTestResults = compat_ptr(pTestResult->pTestResults);

   ret = IFX_TAPI_NLT_Result_Get (pChannel, &testResult_64);
   /* nStatus is output parameter */
   pTestResult->nStatus = testResult_64.nStatus;

   return ret;
}
#endif /* TAPI_FEAT_NLT */

#endif /* TAPI_FEAT_LX_COMPAT */
