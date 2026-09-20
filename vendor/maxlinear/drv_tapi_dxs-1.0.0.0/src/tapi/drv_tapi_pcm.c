/******************************************************************************

                            Copyright (c) 2006-2016
                        Lantiq Beteiligungs-GmbH & Co.KG

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_pcm.c
   Desription  : Contains PCM Services.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "drv_tapi.h"

#ifdef TAPI_FEAT_PCM

#include "drv_tapi_errno.h"

/* ============================= */
/* Global function definition    */
/* ============================= */


/**
   Sets the configuration of the PCM channel interface

   \param pTapiDev      Pointer to TAPI_DEV structure.
   \param pPCMif        Contains the configuration for the pcm interface.

   \return
   Returns an error code:
      - \ref TAPI_statusOk if configuration is set
      - \ref TAPI_statusLLNotSupp if configuration do not supported by LL driver
      - \ref TAPI_statusLLFailed   if configuration is NOT set.
*/
IFX_int32_t TAPI_Phone_PCM_IF_Set_Config(TAPI_DEV *pTapiDev,
                                         IFX_TAPI_PCM_IF_CFG_t const *pPCMif)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pTapiDev->pDevDrvCtx;
   IFX_int32_t LLerr = TAPI_statusOk;

   /* chip specific function */
   if (!IFX_TAPI_PtrChk (pDrvCtx->PCM.ifCfg))
   {
      RETURN_DEVSTATUS (TAPI_statusLLNotSupp, 0);
   }

   LLerr = pDrvCtx->PCM.ifCfg (pTapiDev->pLLDev, pPCMif);

   if (!TAPI_SUCCESS(LLerr))
   {
      /*errmsg: PCM interface configuration failed */
      RETURN_DEVSTATUS (TAPI_statusPCMIfConfError, LLerr);
   }

   return TAPI_statusOk;
}

/**
   Sets the configuration of one PCM channel

   \param pChannel      Pointer to TAPI_CHANNEL structure.
   \param pPCMConfig    Contains the configuration for the pcm channel.

   \return
   Returns an error code:
      - IFX_SUCCESS if configuration is set
      - IFX_ERROR   if configuration is NOT set.
*/
IFX_int32_t TAPI_Phone_PCM_Set_Config(TAPI_CHANNEL *pChannel,
                                      IFX_TAPI_PCM_CFG_t const *pPCMConfig)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;
   IFX_int32_t LLerr = TAPI_statusOk;

   /* configuration is not saved */
   pChannel->TapiPCMData.bCfgSuccess = IFX_FALSE;

   /* chip specific function */
   if (!IFX_TAPI_PtrChk (pDrvCtx->PCM.Cfg))
   {
      RETURN_STATUS (TAPI_statusLLNotSupp, 0);
   }

   LLerr = pDrvCtx->PCM.Cfg (pChannel->pLLChannel, pPCMConfig);
   if (!TAPI_SUCCESS(LLerr))
   {
      /*errmsg: PCM channel configuration failed */
      RETURN_STATUS (TAPI_statusPCMChCfgError, LLerr);
   }

   /* save configuration */
   memcpy(&pChannel->TapiPCMData.PCMConfig, pPCMConfig,
          sizeof(IFX_TAPI_PCM_CFG_t));

   pChannel->TapiPCMData.bCfgSuccess = IFX_TRUE;

   return TAPI_statusOk;
}


/**
   Gets the configuration of one PCM channel

   \param pChannel      Pointer to TAPI_CHANNEL structure.
   \param pPCMConfig    Contains the configuration for pcm channel.

   \return
   Returns an error code:
      - IFX_SUCCESS if configuration is set
      - IFX_ERROR   if configuration is NOT set.
*/
IFX_int32_t TAPI_Phone_PCM_Get_Config(TAPI_CHANNEL *pChannel,
                                      IFX_TAPI_PCM_CFG_t *pPCMConfig)
{
   /* get configuration */
   memcpy(pPCMConfig, &pChannel->TapiPCMData.PCMConfig,
          sizeof(IFX_TAPI_PCM_CFG_t));

   return TAPI_statusOk;
}


/**
   Activate or deactivate the pcm timeslots configured for this channel

   \param pChannel   Pointer to TAPI_CHANNEL structure.
   \param nActive    PCM activation status
                           - 1: timeslot activated
                           - 0: timeslot deactivated
   \return
   IFX_SUCCESS or IFX_ERROR
*/
IFX_int32_t TAPI_Phone_PCM_Set_Activation (TAPI_CHANNEL *pChannel,
   IFX_uint32_t nActive)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;
   IFX_int32_t LLerr = TAPI_statusOk;

   if (nActive > 1)
   {
      RETURN_STATUS (TAPI_statusPCMUnknownMode, 0);
   }

   /* check if PCM configuration was successful, if not - block activation */
   if ((nActive != 0) && (IFX_FALSE == pChannel->TapiPCMData.bCfgSuccess))
   {
      /*errmsg: PCM channel not configured */
      RETURN_STATUS (TAPI_statusPCMChNoCfg, 0);
   }

   /* chip specific function */
   if (!IFX_TAPI_PtrChk (pDrvCtx->PCM.Enable))
   {
      /*errmsg: PCM channel not configured */
      RETURN_STATUS (TAPI_statusLLNotSupp, 0);
   }

   LLerr = pDrvCtx->PCM.Enable (pChannel->pLLChannel, nActive,
                              &(pChannel->TapiPCMData.PCMConfig));
   if (!TAPI_SUCCESS(LLerr))
   {
      /*errmsg: PCM channel activation failed */
      RETURN_STATUS (TAPI_statusPCMActivation, LLerr);
   }

   pChannel->TapiPCMData.bTimeSlotActive = (IFX_boolean_t)(nActive == 1);

   return TAPI_statusOk;
}

/**
   Get the activation status from the pcm interface

   \param pChannel      Pointer to TAPI_CHANNEL structure.
   \param pActive       Handle to the result storage, returning the activation state.

   \return IFX_SUCCESS
*/
IFX_int32_t TAPI_Phone_PCM_Get_Activation (TAPI_CHANNEL *pChannel,
   IFX_TAPI_PCM_ACTIVATION_t *pActive)
{
#ifdef TAPI_ONE_DEVNODE
   pActive->mode = (IFX_TRUE == pChannel->TapiPCMData.bTimeSlotActive) ?
      IFX_ENABLE : IFX_DISABLE;
#else
   *pActive =  (IFX_TRUE == pChannel->TapiPCMData.bTimeSlotActive) ?
      IFX_ENABLE : IFX_DISABLE;
#endif /* TAPI_ONE_DEVNODE */

   return TAPI_statusOk;
}

#endif /* TAPI_FEAT_PCM */