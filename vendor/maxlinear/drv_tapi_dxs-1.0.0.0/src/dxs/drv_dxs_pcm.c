/******************************************************************************

                           Copyright (c) 2014 - 2016
                        Lantiq Beteiligungs-GmbH & Co.KG
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_pcm.c
   This file implements the PCM module.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include "drv_dxs_api.h"
#include "drv_dxs_pcm_priv.h"
#include "drv_dxs_init.h"
#include "drv_dxs_errno.h"
#include "drv_dxs_mbx.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */
static IFX_TAPI_PCM_IF_CFG_t  ifx_tapi_pcm_if_cfg_defaults;

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_int32_t DXS_TAPI_LL_PCM_IF_Cfg(
                    IFX_TAPI_LL_DEV_t *pLLDev,
                    const IFX_TAPI_PCM_IF_CFG_t *pCfg);

static IFX_int32_t DXS_TAPI_LL_PCM_CH_Cfg (
                    IFX_TAPI_LL_CH_t *pLLChannel,
                    IFX_TAPI_PCM_CFG_t const *pPCMConfig);

static IFX_int32_t DXS_TAPI_LL_PCM_CH_Enable(
                    IFX_TAPI_LL_CH_t *pLLChannel,
                    IFX_uint32_t nMode,
                    IFX_TAPI_PCM_CFG_t *pPcmCfg);

static IFX_int32_t DXS_PCM_IF_Cfg (DXS_DEVICE_t *pDev,
                                   const IFX_TAPI_PCM_IF_CFG_t *pCfg);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */
/**
   Configure and enable PCM interface.

   \param pLLDev  pointer to the device structure

   \param pCfg    pointer to the coefficient structure

   \return
   - DXS_statusOk
   - DXS_statusErr -  PCM interface has already been enabled
   - DXS_statusParam -  One of the parameters is not supported
*/
static IFX_int32_t DXS_PCM_IF_Cfg (DXS_DEVICE_t *pDev,
                                   const IFX_TAPI_PCM_IF_CFG_t *pCfg)
{
   DXS_PCM_IF_CTRL_t PCM_CtrlCmd,
                     *pPCM_CtrlCmd  = &PCM_CtrlCmd;
   IFX_uint8_t i;
   IFX_int32_t ret = DXS_statusErr;

   /* sanity check - accept only slave mode */
   if (pCfg->nOpMode == IFX_TAPI_PCM_IF_MODE_MASTER)
      RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);

   /* Check the status of all PCM channels, if any is active we cannot
      re-/configure the PCM if-mode. */
   for (i = 0; i < DXS_MAX_CH_NR; i++)
   {
      if ((pDev->pChannel[i].pPCM != IFX_NULL) &&
          (pDev->pChannel[i].pPCM->fw_pcm_ch.EN != PCM_CH_CTRL_DISABLE))
      {
         /* errmsg: PCM interface cannot be configured while any PCM channel
                    is active */
         RETURN_DEVSTATUS(DXS_statusPcmIfCfgWhileActive, IFX_NULL);
      }
   }

   TRACE(TAPI_DXS, DBG_LEVEL_LOW,
      ("DXS PCM if cfg: mode %d, clk %d (double %d), "
       "slope tx: %s, slope rx: %s, offset tx %d, offset rx %d, timeslot sync %d\n",
      pCfg->nOpMode,
      pCfg->nDCLFreq,
      pCfg->nDoubleClk,
      (pCfg->nSlopeTX==IFX_TAPI_PCM_IF_SLOPE_RISE) ? "rising" : "falling",
      (pCfg->nSlopeRX==IFX_TAPI_PCM_IF_SLOPE_RISE) ? "rising" : "falling",
      pCfg->nOffsetTX,
      pCfg->nOffsetRX,
      pCfg->nTsSync));

   /* Initialise the PCM_IF_CTRL firmware message header. */
   memset (pPCM_CtrlCmd, 0, sizeof (DXS_PCM_IF_CTRL_t));
   pPCM_CtrlCmd->CMD          = DXS_CMD_CMD_EOP;
   pPCM_CtrlCmd->MOD          = DXS_CMD_MOD_PCM;
   pPCM_CtrlCmd->ECMD         = PCM_IF_CTRL_ECMD_PCM_INTERFACE_CONTROL;
   pPCM_CtrlCmd->LENGTH       = PCM_IF_CTRL_LENGTH;

   /* If the interface is already active switch it off before changing
      the parameters. */
   if ((pDev->nDevState & DS_PCM_EN) != 0)
   {
      /* Disable the PCM Interface */
      pPCM_CtrlCmd->EN = PCM_IF_CTRL_DISABLE;

      ret = DXS_CmdWrite (pDev, (IFX_uint32_t*)(IFX_void_t*)pPCM_CtrlCmd);
      if (!DXS_SUCCESS (ret))
      {
         RETURN_DEVSTATUS(ret, IFX_NULL);
      }

      pDev->nDevState &= ~DS_PCM_EN;
   }

   /* Enable the PCM Interface. */
   pPCM_CtrlCmd->EN           = PCM_IF_CTRL_ENABLE;
   pPCM_CtrlCmd->XOFF         = pCfg->nOffsetTX;
   pPCM_CtrlCmd->DBL          = pCfg->nDoubleClk;
   pPCM_CtrlCmd->XS           = pCfg->nSlopeTX;
   pPCM_CtrlCmd->RS           = !pCfg->nSlopeRX;
   pPCM_CtrlCmd->DRV0         = pCfg->nDrive;
   pPCM_CtrlCmd->SHIFT        = pCfg->nShift;
   pPCM_CtrlCmd->ROFF         = pCfg->nOffsetRX;
   /* FIXME: DSEN to be configured from TAPI */
   pPCM_CtrlCmd->DSEN         = 1;

   ret = DXS_CmdWrite (pDev, (IFX_uint32_t*)(IFX_void_t*)pPCM_CtrlCmd);
   if (!DXS_SUCCESS (ret))
   {
      RETURN_DEVSTATUS(ret, IFX_NULL);
   }

   /* Write was successful. So here the interface is activated. */

   pDev->nDevState |= DS_PCM_EN;

   /* For checking the timeslots we need the maximum number of timeslots
      that can be used. However as slave interface we do not set the DCL
      frequency but only synchronize to it. As there is no way to know
      the actual frequency to which the interface has synchronised the
      frequency given in the IOCTL is used instead. */
   switch (pCfg->nDCLFreq)
   {
      case IFX_TAPI_PCM_IF_DCLFREQ_512:
         pDev->nMaxTimeslot = 8;
         break;
      case IFX_TAPI_PCM_IF_DCLFREQ_1024:
         pDev->nMaxTimeslot = 16;
         break;
      case IFX_TAPI_PCM_IF_DCLFREQ_1536:
         pDev->nMaxTimeslot = 24;
         break;
      case IFX_TAPI_PCM_IF_DCLFREQ_2048:
         pDev->nMaxTimeslot = 32;
         break;
      case IFX_TAPI_PCM_IF_DCLFREQ_4096:
         pDev->nMaxTimeslot = 64;
         break;
      case IFX_TAPI_PCM_IF_DCLFREQ_8192:
         pDev->nMaxTimeslot = 128;
         break;
      case IFX_TAPI_PCM_IF_DCLFREQ_16384:
      default:
         TRACE(TAPI_DXS, DBG_LEVEL_LOW, ("Unsupported PCM clock\n"));
         /* errmsg: At least one parameter is wrong. */
         ret = DXS_statusParam;
   }

   /* double clocking offers only half the channels */
   if (pPCM_CtrlCmd->DBL)
   {
      pDev->nMaxTimeslot /= 2;
   }

   RETURN_DEVSTATUS(ret, IFX_NULL);
}

/**
   Configure and enable PCM interface. Wrapper function for drv_tapi.

   \param pLLDev  pointer to the device structure

   \param pCfg    pointer to the coefficient structure

   \return
   - IFX_SUCCESS
   - IFX_ERROR
*/
static IFX_int32_t DXS_TAPI_LL_PCM_IF_Cfg (IFX_TAPI_LL_DEV_t *pLLDev,
                                            const IFX_TAPI_PCM_IF_CFG_t *pCfg)
{
   return DXS_PCM_IF_Cfg ((DXS_DEVICE_t*) pLLDev, pCfg);
}

/**
   Stop the PCM interface

   \param  pDev         Pointer to the device structure.

   \return
   - DXS_statusOk         If successful
   - DXS_statusCmdWr      Writing the command has failed
*/
IFX_int32_t DXS_PCM_IF_Stop (DXS_DEVICE_t *pDev)
{
   IFX_int32_t ret = DXS_statusErr;

   /* calling function should ensure valid parameters */
   if (pDev == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusErr;
   }

   /* Check that all PCM channels are stopped is not done as this function
      is only called when stopping the driver. */
   if (pDev->nDevState & DS_PCM_EN)
   {
      /* Disable the PCM Interface */
      DXS_PCM_IF_CTRL_t PCM_CtrlCmd = {0};
      PCM_CtrlCmd.CMD    = DXS_CMD_CMD_EOP;
      PCM_CtrlCmd.MOD    = DXS_CMD_MOD_PCM;
      PCM_CtrlCmd.ECMD   = PCM_IF_CTRL_ECMD_PCM_INTERFACE_CONTROL;
      PCM_CtrlCmd.LENGTH = PCM_IF_CTRL_LENGTH;
      PCM_CtrlCmd.EN     = PCM_IF_CTRL_DISABLE;

      ret = DXS_CmdWrite(pDev, (IFX_uint32_t*)&PCM_CtrlCmd);

      if (DXS_SUCCESS (ret))
      {
         pDev->nDevState &= ~DS_PCM_EN;
      }
   }

   RETURN_DEVSTATUS(ret, IFX_NULL);
}

/**
   Prepare and check PCM channel parameters and call the target configuration
   function to configure the PCM interface.

   \param pLLChannel Handle to TAPI low level channel structure

   \param pPCMConfig Contains the new configuration for PCM interface

   \return
   - DXS_statusOk
   - DXS_statusErr
   - DXS_statusNotSupported
   - DXS_statusFuncParam
   - DXS_statusPcmTsInvalid
   - DXS_statusPcmHwInvalid

   \remarks
   Performs error checking according to the underlying device capability.
   This function just checks the configuration. No firmware message is sent.
*/
static IFX_int32_t DXS_TAPI_LL_PCM_CH_Cfg (
                   IFX_TAPI_LL_CH_t *pLLChannel,
                   IFX_TAPI_PCM_CFG_t const *pPCMConfig)
{
   IFX_int32_t    err   = DXS_statusOk;
   DXS_CHANNEL_t  *pCh  = (DXS_CHANNEL_t *) pLLChannel;
   DXS_DEVICE_t   *pDev;
   IFX_uint32_t   timeslots = 1,
                  split = 0;

   if((pCh == IFX_NULL) || (pCh->pParent == IFX_NULL))
      return DXS_statusErr;

   pDev = pCh->pParent;

   if(pPCMConfig == IFX_NULL)
      RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);

   /* check if interface needs to be enabled (with defaults) */
   if (!(pDev->nDevState & DS_PCM_EN))
      /* nDevState DS_PCM_EN is set inside (if successful) */
      err = DXS_PCM_IF_Cfg (pDev, &ifx_tapi_pcm_if_cfg_defaults);

   if (DXS_statusOk != err)
   {
      RETURN_STATUS(err, IFX_NULL);
   }

   /* check PCM resolution for supported values */
   switch (pPCMConfig->nResolution)
   {
      case IFX_TAPI_PCM_RES_NB_ALAW_8BIT:
      case IFX_TAPI_PCM_RES_NB_ULAW_8BIT:
         timeslots = 1;
         break;
      case IFX_TAPI_PCM_RES_NB_LINEAR_16BIT:
         timeslots = 2;
         break;
      case IFX_TAPI_PCM_RES_WB_LINEAR_16BIT:
         timeslots = 4;
         break;
      case IFX_TAPI_PCM_RES_WB_LINEAR_SPLIT_16BIT:
         if (pDev->caps.bfw_WbTsSplit != 1)
         {
            /* FW does not support split timeslots for WB mode. */
            /* errmsg: Requested PCM resolution not supported. */
            RETURN_STATUS(DXS_statusPcmResolutionNotSupported, IFX_NULL);
         }
         timeslots = 2; /* 2nd group has just 2 timeslots */
         split = pDev->nMaxTimeslot >> 1; /* 2nd group offset from 1st group */
         break;
      case IFX_TAPI_PCM_RES_WB_ALAW_8BIT:
      case IFX_TAPI_PCM_RES_WB_ULAW_8BIT:
         /* errmsg: Requested PCM resolution not supported. */
         RETURN_STATUS(DXS_statusPcmResolutionNotSupported, IFX_NULL);
      default:
         RETURN_STATUS(DXS_statusFuncParam, IFX_NULL);
   }

   /* Here we check that the PCM sample has enough space on the given timeslot.
      Because we have also coders that use 2 and 4 consecutive timeslots we
      set above the number of timeslots used by one sample and then check here
      that the maximum timeslot is not exceeded.
      Note: Timeslots start counting from 0 */
   if (((pPCMConfig->nTimeslotRX + timeslots + split) > pDev->nMaxTimeslot) ||
       ((pPCMConfig->nTimeslotTX + timeslots + split) > pDev->nMaxTimeslot))
   {
      /* errmsg: PCM timeslot given out of range. */
      RETURN_STATUS(DXS_statusPcmTsInvalid, IFX_NULL);
   }

   return DXS_statusOk;
}

/**
   Prepare parameters and call the Target Configuration Function to activate/
   deactivate the pcm interface.

   The configuration must be done previously with the low level function
   DXS_TAPI_LL_PCM_CH_Cfg.
   Resource availability check is done. The function returns with error when
   the resource or the timeslot is not available.

   No message will be send and no error returned, when the setting has
   not been changed. The driver checks for the cached values.

   \param pLLChannel   Handle to low level channel structure

   \param nMode  Activation mode
   -1: timeslot activated
   -0: timeslot deactivated
   \param pPcmCfg Pointer to the current PCM configuration

   \return
   - IFX_SUCCESS
   - IFX_ERROR
*/
static IFX_int32_t DXS_TAPI_LL_PCM_CH_Enable(
                   IFX_TAPI_LL_CH_t *pLLChannel,
                   IFX_uint32_t nMode,
                   IFX_TAPI_PCM_CFG_t *pPcmCfg)
{
   IFX_int32_t       err   = DXS_statusOk;
   DXS_CHANNEL_t     *pCh  = (DXS_CHANNEL_t *) pLLChannel;
   DXS_DEVICE_t      *pDev;
   IFX_uint8_t       ch;
   IFX_uint32_t      IdxRx, BitValueRx;
   IFX_uint32_t      IdxTx, BitValueTx;
   DXS_PCM_CH_CTRL_t *pPcmCh;
   IFX_uint32_t      i, j, split = 0, timeslots = 1;

   if((pCh == IFX_NULL) || (pCh->pParent == IFX_NULL))
      return DXS_statusErr;

   pDev = pCh->pParent;
   ch = pCh->nChannel - 1;

   if(pPcmCfg == IFX_NULL)
      RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);

   /* in Duslic XS there is only one highway */
   TAPI_UNUSED(pPcmCfg->nHighway);

   if (ch >= pDev->caps.nPCM)
   {
      pDev->nErr = DXS_statusInvalCh;
      err = DXS_statusInvalCh;
      RETURN_STATUS(err, IFX_NULL);
   }

   if ((pDev->nDevState & DS_PCM_EN) == IFX_FALSE)
   {
      /* errno: PCM interface is not initialized. */
      err = DXS_statusPcmNotInitialized;
      RETURN_STATUS(err, IFX_NULL);
   }

   pPcmCh = &pCh->pPCM->fw_pcm_ch;

   /* Activate Timeslots and set highway */
   switch (nMode)
   {
      case  0:
         if (pPcmCh->EN == PCM_CH_CTRL_ENABLE)
         {
            /*
               the device variables PcmRxTs, PcmTxTs are global for all
               channels and must be protected against concurent tasks access
            */
            TAPI_OS_MutexGet (&pDev->mtxMemberAcc);
            switch (pPcmCh->COD)
            {
               case PCM_CH_CTRL_COD_PCM_RES_LINEAR_16BIT:
                  timeslots = (pPcmCh->WIDE == PCM_CH_CTRL_WIDE_16KHZ) ? 4 : 2;
                  break;
               case PCM_CH_CTRL_COD_PCM_RES_ALAW_8BIT:
               case PCM_CH_CTRL_COD_PCM_RES_ULAW_8BIT:
                  timeslots = 1;
                  break;
               default:
                  /* nothing, just for the compiler */
                  break;
            }

            if (pPcmCh->WBTSC == PCM_CH_CTRL_WBTSC_SPLIT)
            {
               /* The offset between 1st and 2nd group is half the number of
                  maximum timeslots. */
               split = pDev->nMaxTimeslot >> 1;
            }

            /* free the timeslots in the map */
            for (j = 0;  j < timeslots; j++)
            {
               /* Translate the loop parameter so that for split timeslots
                  the index of the 2nd group includes the offset of half the
                  possible timeslots. For consecutive timeslots i=j is used. */
               i = ((split != 0) && (j >= 2)) ? ((j - 2) + split) : j;

               IdxRx = ((pPcmCh->RTS & 0x7F) + i) >> 5;
               BitValueRx = 1 << ((pPcmCh->RTS + i) & 0x1F);
               IdxTx = ((pPcmCh->XTS & 0x7F) + i) >> 5;
               BitValueTx = 1 << ((pPcmCh->XTS + i) & 0x1F);

               pDev->PcmRxTs[IdxRx] &= ~BitValueRx;
               pDev->PcmTxTs[IdxTx] &= ~BitValueTx;
            }

            /* disable channel: This will deactivate the timeslots */
            pPcmCh->EN = PCM_CH_CTRL_DISABLE;

            /* write coefficients back */
            err = DXS_CmdWrite (pDev, (IFX_uint32_t*)(IFX_void_t*)pPcmCh);
            /* release share variables lock */
            TAPI_OS_MutexRelease (&pDev->mtxMemberAcc);
         }
         break;

      case 1:
         /*
            the device variables PcmRxTs, PcmTxTs are global for all
            channels and must be protected against concurent tasks access
         */
         TAPI_OS_MutexGet (&pDev->mtxMemberAcc);
         /* check timeslot allocation */
         switch (pPcmCfg->nResolution)
         {
            case IFX_TAPI_PCM_RES_NB_LINEAR_16BIT:
               /* for 16 Bit mode, the next higher time slot has to be reserved too */
               timeslots = 2;
               break;
            case IFX_TAPI_PCM_RES_WB_LINEAR_16BIT:
               timeslots = 4;
               break;
            case IFX_TAPI_PCM_RES_WB_LINEAR_SPLIT_16BIT:
               timeslots = 4;
               /* The offset between 1st and 2nd group is half the number of
                  maximum timeslots. */
               split = pDev->nMaxTimeslot >> 1;
               break;
            case IFX_TAPI_PCM_RES_NB_ALAW_8BIT:
            case IFX_TAPI_PCM_RES_NB_ULAW_8BIT:
               timeslots = 1;
               break;
            default:
               TAPI_OS_MutexRelease (&pDev->mtxMemberAcc);
               /* errmsg: Requested PCM resolution not supported. */
               RETURN_STATUS(DXS_statusPcmResolutionNotSupported, IFX_NULL);
         }

         for (j = 0;  j < timeslots; j++)
         {
            /* Translate the loop parameter so that for split timeslots
               the index of the 2nd group includes the offset of half the
               possible timeslots. For consecutive timeslots i=j is assigned. */
            i = ((split != 0) && (j >= 2)) ? ((j - 2) + split) : j;
            /* get the index in the time slot array. Each bit reflects one time
            slot. Modulo 32 separate the index of the array and the bit inside
            the field. */
            IdxRx = (IFX_uint32_t)(pPcmCfg->nTimeslotRX + i) >> 5;
            BitValueRx = 1 << ((pPcmCfg->nTimeslotRX + i) & 0x1F);
            IdxTx = (IFX_uint32_t)(pPcmCfg->nTimeslotTX + i) >> 5;
            BitValueTx = 1 << ((pPcmCfg->nTimeslotTX + i) & 0x1F);

            /* RX Slot checking */
            if (pDev->PcmRxTs[IdxRx] & BitValueRx)
            {
               TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                   ("DXS ERROR: Rx Slot %u is already in use\n",
                    pPcmCfg->nTimeslotRX + i));
               /* errmsg: Requested PCM timeslot is already in use. */
               err = DXS_statusPcmRequestedTsInUse;
#ifndef ENABLE_TRACE
               /* Without trace we break here to save time. */
               break;
#endif /* ENABLE_TRACE */
            }

            /* TX Slot checking */
            if (pDev->PcmTxTs[IdxTx] & BitValueTx)
            {
               TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                   ("DXS ERROR: Tx Slot %u is already in use\n",
                    pPcmCfg->nTimeslotTX + i));
               /* errmsg: Requested PCM timeslot is already in use. */
               err = DXS_statusPcmRequestedTsInUse;
#ifndef ENABLE_TRACE
               /* Without trace we break here to save time. */
               break;
#endif /* ENABLE_TRACE */
            }
         }

         if (!DXS_SUCCESS(err))
         {
            TAPI_OS_MutexRelease (&pDev->mtxMemberAcc);
            RETURN_STATUS(err, IFX_NULL);
         }

         for (j = 0;  j < timeslots; j++)
         {
            /* For comments see in the block above. */
            i = ((split != 0) && (j >= 2)) ? ((j - 2) + split) : j;
            IdxRx = (IFX_uint32_t)(pPcmCfg->nTimeslotRX + i) >> 5;
            BitValueRx = 1 << ((pPcmCfg->nTimeslotRX + i) & 0x1F);
            IdxTx = (IFX_uint32_t)(pPcmCfg->nTimeslotTX + i) >> 5;
            BitValueTx = 1 << ((pPcmCfg->nTimeslotTX + i) & 0x1F);
            /* reserve the time slots now, because the parameter are OK */
            pDev->PcmRxTs[IdxRx] |= BitValueRx;
            pDev->PcmTxTs[IdxTx] |= BitValueTx;
         }

         /* activate Timeslots */
         switch (pPcmCfg->nResolution)
         {
            case IFX_TAPI_PCM_RES_NB_ALAW_8BIT:
               pPcmCh->WIDE = PCM_CH_CTRL_WIDE_8KHZ;
               pPcmCh->COD  = PCM_CH_CTRL_COD_PCM_RES_ALAW_8BIT;
               break;
            case IFX_TAPI_PCM_RES_NB_ULAW_8BIT:
               pPcmCh->WIDE = PCM_CH_CTRL_WIDE_8KHZ;
               pPcmCh->COD  = PCM_CH_CTRL_COD_PCM_RES_ULAW_8BIT;
               break;
            case IFX_TAPI_PCM_RES_WB_LINEAR_16BIT:
               pPcmCh->WIDE = PCM_CH_CTRL_WIDE_16KHZ;
               pPcmCh->COD  = PCM_CH_CTRL_COD_PCM_RES_LINEAR_16BIT;
               pPcmCh->WBTSC= PCM_CH_CTRL_WBTSC_CONSECUTIVE;
               break;
            case IFX_TAPI_PCM_RES_WB_LINEAR_SPLIT_16BIT:
               pPcmCh->WIDE = PCM_CH_CTRL_WIDE_16KHZ;
               pPcmCh->COD  = PCM_CH_CTRL_COD_PCM_RES_LINEAR_16BIT;
               pPcmCh->WBTSC= PCM_CH_CTRL_WBTSC_SPLIT;
               break;
            case IFX_TAPI_PCM_RES_NB_LINEAR_16BIT:
            default:
               pPcmCh->WIDE = PCM_CH_CTRL_WIDE_8KHZ;
               pPcmCh->COD  = PCM_CH_CTRL_COD_PCM_RES_LINEAR_16BIT;
               break;
         }

         pPcmCh->XTS  = pPcmCfg->nTimeslotTX;
         pPcmCh->RTS  = pPcmCfg->nTimeslotRX;
         pPcmCh->EN   = PCM_CH_CTRL_ENABLE;
         pPcmCh->CHAN = ch;

         err = DXS_CmdWrite (pDev, (IFX_uint32_t*)(IFX_void_t*)pPcmCh);
         TAPI_OS_MutexRelease (&pDev->mtxMemberAcc);
         break;
      default:
         /* nothing, just for the compiler */
         break;
   }

   return err;
}

/**
   Allocate data structures of the PCM module for the given channel.

   \param  pCh          Pointer to the channel structure.

   \return
   - DXS_statusOk
   - DXS_statusNoMem    in case the stucture could not be created

  \remarks The channel parameter is no longer checked because the calling
   function assures correct values.
*/
IFX_int32_t DXS_PCM_Allocate_Ch_Structures (DXS_CHANNEL_t *pCh)
{
   DXS_PCM_Free_Ch_Structures (pCh);

   pCh->pPCM = TAPI_OS_Malloc(sizeof(*pCh->pPCM));
   if (pCh->pPCM == IFX_NULL)
   {
      /* errmsg: No memory could be allocated. */
      RETURN_STATUS(DXS_statusNoMem, IFX_NULL);
   }
   memset(pCh->pPCM, 0, sizeof(*pCh->pPCM));

   return DXS_statusOk;
}

/**
   Free data structure of the PCM module in the given channel.

   \param  pCh             Pointer to the channel structure.
*/
IFX_void_t DXS_PCM_Free_Ch_Structures (DXS_CHANNEL_t *pCh)
{
   if (pCh->pPCM != IFX_NULL)
   {
      TAPI_OS_Free(pCh->pPCM);
      pCh->pPCM = IFX_NULL;
   }
}

/**
   Initialize the PCM module and the cached firmware messages

   \param  pCh          Pointer to the channel structure.

   \return
   None.
*/
IFX_void_t DXS_PCM_InitCh (DXS_CHANNEL_t *pCh)
{
   DXS_PCM_CH_MUTE_t *pPcmChMute = IFX_NULL;
   IFX_uint8_t ch = pCh->nChannel - 1;
   DXS_PCM_CH_CTRL_t *pPcmCh = &pCh->pPCM->fw_pcm_ch;

   memset(pPcmCh, 0, sizeof(*pPcmCh));

   /* PCM ch message */
   pPcmCh->CMD          = DXS_CMD_CMD_EOP;
   pPcmCh->CHAN         = ch;
   pPcmCh->MOD          = DXS_CMD_MOD_PCM;
   pPcmCh->ECMD         = PCM_CH_CTRL_ECMD_PCM_CHAN;
   pPcmCh->LENGTH       = PCM_CH_CTRL_LENGTH;
   pPcmCh->EN           = PCM_CH_CTRL_DISABLE;
   pPcmCh->COD          = PCM_CH_CTRL_COD_PCM_RES_ALAW_8BIT;   /* G711- ALaw */
   pPcmCh->WIDE         = PCM_CH_CTRL_WIDE_8KHZ;

   /* initialize FW message for muting of the PCM path */
   pPcmChMute = &pCh->pPCM->pcm_ch_mute;
   memset (pPcmChMute, 0, sizeof(*pPcmChMute));
   pPcmChMute->CMD     = DXS_CMD_CMD_EOP;
   pPcmChMute->CHAN    = ch;
   pPcmChMute->MOD     = DXS_CMD_MOD_PCM;
   pPcmChMute->ECMD    = PCM_CH_MUTE_ECMD_PCM_MUTE;
   pPcmChMute->LENGTH  = PCM_CH_MUTE_LENGTH;
   pPcmChMute->RX_MUTE = PCM_CH_MUTE_RX_MUTE_NO_MUTE;

   /* set static defaults for PCM i/f */
   ifx_tapi_pcm_if_cfg_defaults.nOpMode    = IFX_TAPI_PCM_IF_MODE_SLAVE_AUTOFREQ;
   ifx_tapi_pcm_if_cfg_defaults.nDoubleClk = IFX_DISABLE;
   ifx_tapi_pcm_if_cfg_defaults.nSlopeTX   = IFX_TAPI_PCM_IF_SLOPE_RISE;
   ifx_tapi_pcm_if_cfg_defaults.nSlopeRX   = IFX_TAPI_PCM_IF_SLOPE_FALL;
   ifx_tapi_pcm_if_cfg_defaults.nOffsetTX  = IFX_TAPI_PCM_IF_OFFSET_NONE;
   ifx_tapi_pcm_if_cfg_defaults.nOffsetRX  = IFX_TAPI_PCM_IF_OFFSET_NONE;
   ifx_tapi_pcm_if_cfg_defaults.nDrive     = IFX_TAPI_PCM_IF_DRIVE_ENTIRE;
   ifx_tapi_pcm_if_cfg_defaults.nShift     = IFX_DISABLE;
   ifx_tapi_pcm_if_cfg_defaults.nDCLFreq   = IFX_TAPI_PCM_IF_DCLFREQ_2048;

   /* DCL Freq (2048) / 64 = 32 Timeslots */
   pCh->pParent->nMaxTimeslot = 32;
}

/**
   Stop PCM on this channel

   \param  pCh          Pointer to the channel structure.

   - DXS_statusOk
   - DXS_statusCmdWr    Writing the command has failed
   - DXS_statusInvalCh
*/
IFX_int32_t DXS_PCM_ChStop (DXS_CHANNEL_t *pCh)
{
   DXS_PCM_CH_CTRL_t    *pPcmCh = IFX_NULL;
   DXS_DEVICE_t         *pDev;
   IFX_int32_t          ret   = DXS_statusOk;

   /* calling function should ensure valid parameters */
   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pParent == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      RETURN_STATUS (DXS_statusInvalCh, IFX_NULL);
   }

   pDev = pCh->pParent;

   if (pCh->pPCM != IFX_NULL)
   {
      /* protect fw msg */
      TAPI_OS_MutexGet (&pCh->mtxChAcc);

      /* get pointer to cached fw message */
      pPcmCh = &pCh->pPCM->fw_pcm_ch;

      /* PCM module deactivation is needed if the module is enabled */
      if ((pPcmCh->EN != PCM_CH_CTRL_DISABLE))
      {
         /* disable PCM channel */
         pPcmCh->EN = PCM_CH_CTRL_DISABLE;

         /* write PCM channel command */
         ret = DXS_CmdWrite (pDev, (IFX_uint32_t*)(IFX_void_t*)pPcmCh);
      }

      TAPI_OS_MutexRelease (&pCh->mtxChAcc);
   }

   RETURN_STATUS(ret, IFX_NULL);
}


/**
   PCM path mute control

   \param  pCh          Pointer to the channel structure.
   \param  nMute        enum of type IFX_enDis_t specifying mute/unmute of
                        PCM path

   \return
   - DXS_statusOk
   - DXS_statusPcmMuteErr    Muting/unmuting the PCM path failed
*/
IFX_int32_t DXS_PCM_ChRxMute (DXS_CHANNEL_t *pCh, IFX_enDis_t nMute)
{
   DXS_PCM_CH_MUTE_t    *pPcmChMute = IFX_NULL;
   DXS_DEVICE_t         *pDev = pCh->pParent;
   IFX_int32_t          ret   = DXS_statusOk;

   pPcmChMute = &pCh->pPCM->pcm_ch_mute;
   /* Set the mode of mixing the tone generator signal with the voice path.*/
   if (nMute == IFX_DISABLE)
   {
      if (pPcmChMute->RX_MUTE != PCM_CH_MUTE_RX_MUTE_NO_MUTE)
      {
         pPcmChMute->RX_MUTE = PCM_CH_MUTE_RX_MUTE_NO_MUTE;
         ret = DXS_CmdWrite(pDev,
            (IFX_uint32_t *)(IFX_void_t *)pPcmChMute);
         if (!DXS_SUCCESS (ret))
         {
            /** errmsg: Muting/unmuting the PCM path failed. */
            RETURN_STATUS(DXS_statusPcmMuteErr, IFX_NULL);
         }
      }
   }
   else
   {
      if (pPcmChMute->RX_MUTE != PCM_CH_MUTE_RX_MUTE_MUTE)
      {
         pPcmChMute->RX_MUTE = PCM_CH_MUTE_RX_MUTE_MUTE;
         ret = DXS_CmdWrite(pDev,
            (IFX_uint32_t *)(IFX_void_t *)pPcmChMute);
         if (!DXS_SUCCESS (ret))
         {
            /** errmsg: Muting/unmuting the PCM path failed. */
            RETURN_STATUS(DXS_statusPcmMuteErr, IFX_NULL);
         }
      }
   }

   RETURN_STATUS(ret, IFX_NULL);
}


/**
   Mute PCM Channel.

   \param pLLChannel Handle to TAPI low level channel structure
   \param pPCMConfig Contains the new configuration for PCM interface

   \return
      - DXS_statusOk if successful else error code

   \remarks
      This function configures firmware to mute selected PCM channel
*/
static IFX_int32_t DXS_TAPI_LL_PCM_CH_Mute (IFX_TAPI_LL_CH_t *pLLChannel,
      IFX_TAPI_PCM_MUTE_CFG_t const *pPCMMute)
{
   DXS_CHANNEL_t        *pCh        = (DXS_CHANNEL_t *) pLLChannel;
   IFX_int32_t          ret         = DXS_statusOk;

   TAPI_ASSERT (pLLChannel);
   TAPI_ASSERT (pPCMMute);

   /* sanity check */
   if (pCh->pPCM == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      RETURN_STATUS (DXS_statusInvalCh, IFX_NULL);
   }

   TAPI_OS_MutexGet (&pCh->mtxChAcc);
   if (pPCMMute->bMuteRx == IFX_TRUE)
      ret = DXS_PCM_ChRxMute(pCh, IFX_ENABLE);
   else
      ret = DXS_PCM_ChRxMute(pCh, IFX_DISABLE);
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);
   RETURN_STATUS (ret, IFX_NULL);
}

/* ========================================================================== */
/*                         Function pointer exports                           */
/* ========================================================================== */
/**
   Function called by init_module of device, fills up PCM module function
   pointers which are passed to HL TAPI during registration.

   \param pPCM            pointer to PCM module

   \return
*/
IFX_void_t DXS_PCM_Func_Register (IFX_TAPI_DRV_CTX_PCM_t *pPCM)
{
   pPCM->ifCfg          = DXS_TAPI_LL_PCM_IF_Cfg;
   pPCM->Cfg            = DXS_TAPI_LL_PCM_CH_Cfg;
   pPCM->Enable         = DXS_TAPI_LL_PCM_CH_Enable;
   pPCM->Mute           = DXS_TAPI_LL_PCM_CH_Mute;
   return;
}
