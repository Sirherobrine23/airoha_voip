/******************************************************************************

  Copyright 2015 Lantiq Deutschland GmbH
  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2021 Maxlinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_alm_aclm.c
   This file contains the implementation of functions for ACLM measurements.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include <drv_tapi_config.h>
#include <drv_tapi_osmap.h>

#ifdef DXS_FEAT_NLT

#include "drv_dxs_api.h"
#include "drv_dxs_mbx.h"
#include "drv_dxs_alm_priv.h"
#include "drv_dxs_errno.h"
#include "drv_dxs_init.h"
#include "drv_dxs_dtmf_priv.h"
#include "drv_dxs_dtmf.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
#define DXS_ACLM_RESULT_NUMBER      100
#define DXS_ACLM_RESULT_ARG         101
#define DXS_ACLM_RESULT_VALUE       102

#define SDD_TX_GAIN_MIN          (-900)
#define SDD_TX_GAIN_MAX           40
#define SDD_RX_GAIN_MIN          (-900)
#define SDD_RX_GAIN_MAX          0
#define SDD_TX_GAIN_DEFAULT      (-30)
#define SDD_RX_GAIN_DEFAULT       (-100)
#define DXS_Q_SIZE                15

#define TG_LEVEL_MIN     (-869)  /* in units of 0.1 dBm0 */
#define TG_LEVEL_MAX     31      /* in units of 0.1 dBm0 */

#define TG_FREQ_MAX  4000    /* Hz */


/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */


static IFX_uint16_t dBm0_level_tbl[] = {
/* 0.0 dBm0 */ 0x592a,
/* 0.1 dBm0 */ 0x5a32,
/* 0.2 dBm0 */ 0x5b3e,
/* 0.3 dBm0 */ 0x5c4c,
/* 0.4 dBm0 */ 0x5d5e,
/* 0.5 dBm0 */ 0x5e73,
/* 0.6 dBm0 */ 0x5f8b,
/* 0.7 dBm0 */ 0x60a6,
/* 0.8 dBm0 */ 0x61c4,
/* 0.9 dBm0 */ 0x62e6,
/* 1.0 dBm0 */ 0x640b,
/* 1.1 dBm0 */ 0x6534,
/* 1.2 dBm0 */ 0x6660,
/* 1.3 dBm0 */ 0x678f,
/* 1.4 dBm0 */ 0x68c2,
/* 1.5 dBm0 */ 0x69f9,
/* 1.6 dBm0 */ 0x6b33,
/* 1.7 dBm0 */ 0x6c71,
/* 1.8 dBm0 */ 0x6db2,
/* 1.9 dBm0 */ 0x6ef7,
/* 2.0 dBm0 */ 0x7040,
/* 2.1 dBm0 */ 0x718d,
/* 2.2 dBm0 */ 0x72de,
/* 2.3 dBm0 */ 0x7432,
/* 2.4 dBm0 */ 0x758b,
/* 2.5 dBm0 */ 0x76e7,
/* 2.6 dBm0 */ 0x7847,
/* 2.7 dBm0 */ 0x79ac,
/* 2.8 dBm0 */ 0x7b15,
/* 2.9 dBm0 */ 0x7c82,
/* 3.0 dBm0 */ 0x7df3,
/* 3.1 dBm0 */ 0x7f68
};

static IFX_uint16_t dxs_sdd_tx_gain_precalc[] = {
/* 0.0      0.1      0.2      0.3      0.4       0.5       0.6      0.7    dB */
   0x4f00,  0x4fea,  0x50d7,  0x51c7,  0x52b9,   0x53ae,   0x54a6,  0x55a1,
/* 0.8      0.9      1.0      1.1      1.2       1.3       1.4      1.5    dB */
   0x569f,  0x57a0,  0x58a4,  0x59aa,  0x5ab4,   0x5bc1,   0x5cd1,  0x5de4,
/* 1.6      1.7      1.8      1.9      2.0       2.1       2.2      2.3    dB */
   0x5efb,  0x6014,  0x6131,  0x6251,  0x6375,   0x649b,   0x65c6,  0x66f3,
/* 2.4      2.5      2.6      2.7      2.8       2.9       3.0      3.1    dB */
   0x6824,  0x6959,  0x6a91,  0x6bcd,  0x6d0d,   0x6e50,   0x6f97,  0x70e2,
/* 3.2      3.3      3.4      3.5      3.6       3.7       3.8      3.9    dB */
   0x7231,  0x7383,  0x74d9,  0x7634,  0x7792,   0x78f5,   0x7a5b,  0x7bc6,
/* 4.0  dB */
   0x7d35
};

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

/**
   Function dxs_aclm_tone_start

   \param   pCh         - pointer to DXS channel structure

   \return
   - DXS_status_t
*/
static IFX_int32_t dxs_aclm_tone_start(DXS_CHANNEL_t *pCh)
{

   DXS_DTMF_AT_GEN_CTRL_t  *pDtmfAtGenCtrl;
   DXS_DEVICE_t *pDev = (DXS_DEVICE_t*) pCh->pParent;
   IFX_int32_t ret;

   pDtmfAtGenCtrl = &pCh->pDTMF->dtmf_at_gen_ctrl;
   pDtmfAtGenCtrl->EN = DTMF_AT_GEN_CTRL_EN_EN;
   ret = DXS_CmdWrite(pDev, (uint32_t *)pDtmfAtGenCtrl);
   return ret;
}

/**
   Function dxs_aclm_tone_stop

   \param   pCh         - pointer to DXS channel structure

   \return
   - DXS_status_t
*/
static IFX_int32_t dxs_aclm_tone_stop(DXS_CHANNEL_t *pCh)
{
   DXS_DTMF_AT_GEN_CTRL_t  *pDtmfAtGenCtrl;
   DXS_DEVICE_t            *pDev             = pCh->pParent;
   IFX_int32_t ret;

   pDtmfAtGenCtrl = &pCh->pDTMF->dtmf_at_gen_ctrl;
   pDtmfAtGenCtrl->EN = DTMF_AT_GEN_CTRL_EN_DIS;
   ret = DXS_CmdWrite(pDev, (uint32_t *)pDtmfAtGenCtrl);
   return ret;
}

/**
   Function DXS_Misc_MulQ15

   \param   a    - Q15 number
   \param   b    - Q15 number

   \return
   - IFX_uint16_t multiplication result of two Q15 numbers
*/
static IFX_uint16_t DXS_Misc_MulQ15 (IFX_int16_t a, IFX_int16_t b)
{
   signed long int  temp;

   temp = (long int)a * (long int)b;
   /* Rounding up */
   temp += (1 << (DXS_Q_SIZE-1));

   /* Correct by dividing by base */
   return (temp >> DXS_Q_SIZE);
}

/**
   Function DXS_Misc_LeveldB_to_Factor

   \param   dB_ten    - integer value in units of 0.1 dB
   \remark  Parameter dB_ten must be zero or negative.

   \return
   - IFX_uint16_t factor in Q15 format
*/
static IFX_uint16_t DXS_Misc_LeveldB_to_Factor (IFX_int16_t dB_ten)
{
   IFX_int16_t n, r, hundr, tens, units, mul1 = 32767, mul2 = 32767;

   /* switch to 0.01 dB units */
   dB_ten *= 10;

   /* calculate factors of 0.5 */
   n = dB_ten / -602;

   /* calculate reminder */
   r = -dB_ten - n * 602;

   hundr = r / 100;
   tens = (r % 100) / 10;
   units = (r % 100) % 10;

   while (n--)
   {
      /* multiply 0.5 n times */
      mul1 = DXS_Misc_MulQ15(mul1, 16384);
   }

   while (hundr--)
   {
      /* multiply 0.891251 (1dB) hundr times */
      mul2 = DXS_Misc_MulQ15(mul2, 29205);
   }

   while (tens--)
   {
      /* multiply 0.988553 (0.1dB) tens times */
      mul2 = DXS_Misc_MulQ15(mul2, 32393);
   }

   while (units--)
   {
      /* multiply 0.998849 (0.01dB) units times */
      mul2 = DXS_Misc_MulQ15(mul2, 32730);
   }

   return (DXS_Misc_MulQ15(mul1, mul2));
}

/**
   Function dxs_tone_config

   \param   pCh         - pointer to DXS channel structure
   \param   level1      - level for frequency 1
   \param   level2      - level for frequency 2
   \param   freq1       - tone frequency 1
   \param   freq2       - tone frequency 2

   \return
   - DXS_status_t
*/
static IFX_int32_t dxs_tone_config (DXS_CHANNEL_t *pCh,
      IFX_int16_t level1, IFX_int16_t level2,
      IFX_uint16_t freq1, IFX_uint16_t freq2)
{
   IFX_uint16_t new_level1, new_level2, new_freq1, new_freq2;
   IFX_uint32_t temp;
   IFX_int32_t ret = DXS_statusOk;
   DXS_DEVICE_t            *pDev             = pCh->pParent;
   DXS_DTMF_AT_GEN_COEF_t  *pDtmfAtGenCoef   = IFX_NULL;


   pDtmfAtGenCoef = &pCh->pDTMF->dtmf_at_gen_coef;

   if (level1 < TG_LEVEL_MIN || level1 > TG_LEVEL_MAX)
   {
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }
   if (level2 < TG_LEVEL_MIN || level2 > TG_LEVEL_MAX)
   {
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }
   if (freq1 >= TG_FREQ_MAX || freq2 >= TG_FREQ_MAX)
   {
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }

   /* calculate new values */
   if (level1 >= 0)
   {
      new_level1 = dBm0_level_tbl[level1];
   }
   else
   {
      new_level1 = DXS_Misc_MulQ15(22826, DXS_Misc_LeveldB_to_Factor(level1));
   }

   if (level2 >= 0)
   {
      new_level2 = dBm0_level_tbl[level2];
   }
   else
   {
      new_level2 = DXS_Misc_MulQ15(22826, DXS_Misc_LeveldB_to_Factor(level2));
   }

   temp = 8192 * (IFX_uint32_t)freq1 / 1000;
   new_freq1 = (IFX_uint16_t)temp;
   temp = 8192 * (IFX_uint32_t)freq2 / 1000;
   new_freq2 = (IFX_uint16_t)temp;

   if (new_level1 != pDtmfAtGenCoef->LEVEL1 ||
       new_level2 != pDtmfAtGenCoef->LEVEL2 ||
       new_freq1 != pDtmfAtGenCoef->FREQ1 ||
       new_freq2 != pDtmfAtGenCoef->FREQ2)
   {
      /* write new values */
      pDtmfAtGenCoef->LEVEL1 =  new_level1;
      pDtmfAtGenCoef->LEVEL2 =  new_level2;
      pDtmfAtGenCoef->FREQ1 = new_freq1;
      pDtmfAtGenCoef->FREQ2 = new_freq2;

      ret = DXS_CmdWrite(pDev, (IFX_uint32_t *)pDtmfAtGenCoef);
   }

   return ret;
}

/**
   Function DXS_SDD_GainGet

   \param   pCh     - pointer to DXS channel structure
   \param   pTxGain - pointer to a digital gain value in dB, A->D direction
   \param   pRxGain - pointer to a digital gain value in dB, D->A direction

   \return
   - DXS_status_t DXS_statusOk always.
*/
static IFX_int32_t DXS_SDD_GainGet (DXS_CHANNEL_t *pCh, IFX_int16_t *pTxGain, IFX_int16_t *pRxGain)
{
   /* missing factor-to-dB calculation;
      workaround: report cached 0.1 dB values stored in a channel context */
   if (pCh->pALM->bGainsConfigured != IFX_TRUE)
   {
      *pTxGain = SDD_TX_GAIN_DEFAULT;
      *pRxGain = SDD_RX_GAIN_DEFAULT;
   }
   else
   {
      *pTxGain = pCh->pALM->tx_gain;
      *pRxGain = pCh->pALM->rx_gain;
   }
   /* end of workaround */

   return DXS_statusOk;
}

/**
   Function DXS_SDD_GainSet

   \param   pCh     - pointer to DXS channel structure
   \param   TxGain  - digital gain in units of 0.1 dB (A->D direction)
   \param   RxGain  - digital gain in units of 0.1 dB (D->A direction)

   \return
   - DXS_status_t
*/
static IFX_int32_t DXS_SDD_GainSet (DXS_CHANNEL_t *pCh, IFX_int16_t TxGain, IFX_int16_t RxGain)
{
   IFX_int32_t err = DXS_statusOk;
   IFX_uint16_t new_gain_tx, new_gain_rx;
   DXS_SDD_TxRxGain_t   *pSddTxRxGain;


   pSddTxRxGain = &pCh->pALM->sdd_txrx_gain;

   /* parameter range check */
   if (TxGain < SDD_TX_GAIN_MIN || TxGain > SDD_TX_GAIN_MAX ||
       RxGain < SDD_RX_GAIN_MIN || RxGain > SDD_RX_GAIN_MAX)
   {
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }

   /* calculate new parameter values */
   if (TxGain >= 0)
   {
      new_gain_tx = dxs_sdd_tx_gain_precalc[TxGain];
   }
   else
   {
      new_gain_tx = DXS_Misc_MulQ15(20224, DXS_Misc_LeveldB_to_Factor(TxGain));
   }

   new_gain_rx = DXS_Misc_MulQ15(30720, DXS_Misc_LeveldB_to_Factor(RxGain));

   if (new_gain_tx != pSddTxRxGain->TxGain || new_gain_rx != pSddTxRxGain->RxGain)
   {
      /* write new values */
      pSddTxRxGain->TxGain = new_gain_tx;
      pSddTxRxGain->RxGain = new_gain_rx;
      err = DXS_CmdWrite(pCh->pParent, (IFX_uint32_t *)pSddTxRxGain);
      if (!DXS_SUCCESS (err))
      {
         RETURN_STATUS (err, IFX_NULL);
      }
   }
   pCh->pALM->tx_gain = TxGain;
   pCh->pALM->rx_gain = RxGain;
   pCh->pALM->bGainsConfigured = IFX_TRUE;

   return err;
}

/**
   ACLM core function. Called for all measurement types.

   \param   pCh    - pointer to DXS channel structure

   \return
   - DXS_status_t
*/
static IFX_int32_t dxs_aclm (DXS_CHANNEL_t *pCh)
{
   /* no parameter sanity check - already done in a calling function */
   struct DXS_ALM *pRes = (struct DXS_ALM *)pCh->pALM;
   DXS_SDD_ACLevelMeterControl_t *pAcLmCtrl = &pRes->fw_sdd_aclm_control;
   DXS_SDD_ACLevelMeterResult_t *pAclmRes = &pRes->fw_sdd_aclm_result;

   IFX_int32_t err;

   /* enable AC measurement */
   pAcLmCtrl->EN = 1;
   err = DXS_CmdWrite (pCh->pParent, (uint32_t *)pAcLmCtrl);
   if (!DXS_SUCCESS (err))
      RETURN_STATUS (err, IFX_NULL);

   /* release channel lock */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);
   err = TAPI_OS_EventWait (&pCh->pALM->aclm_event, 5000, IFX_NULL);
   /* protect channel data from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   if (err == IFX_SUCCESS)
   {
      /* read the measurement result */
      err = DXS_CmdRead (pCh->pParent,
                         (uint32_t *)pAclmRes, (uint32_t *)pAclmRes);
      if (!DXS_SUCCESS (err))
         RETURN_STATUS (err, IFX_NULL);

   } else
   {
      RETURN_STATUS (DXS_statusSddEvtWaitTmout, IFX_NULL);
   }
   return err;
}


/**
   ACLM function for Gain Tracking measurement.

   \param   pCh    - pointer to DXS channel structure

   \return
   - DXS_status_t
*/
static IFX_int32_t dxs_aclm_gt_start (DXS_CHANNEL_t *pCh)
{
   struct DXS_ALM *pRes = (struct DXS_ALM *)pCh->pALM;
   IFX_int32_t err;
   IFX_int32_t level;
   const DXS_SDD_ACLevelMeterResult_t *const pAclmRes = &pRes->fw_sdd_aclm_result;
   DXS_SDD_ACLevelMeterConfig_t *const pAclmConf = &pRes->fw_sdd_aclm_config;
   IFX_TAPI_NLT_ACLM_MP_Result_t *pMeasMP = IFX_NULL;
   /** AC Levelmeter Outband Result Shift */
   IFX_int8_t AcOutbSh;
   /** AC Levelmeter Outband Result */
   IFX_uint32_t AcOutb;
   IFX_uint8_t other_curr_opmode;
   IFX_uint8_t mpoint = 0;

   if (pRes->aclm_meas_status == dxs_aclm_in_progress)
   {
      /* errmsg: measurement is already running */
      RETURN_STATUS (DXS_statusAclmInProgress, IFX_NULL);
   }

   /* ACLM start is possible either from DISABLED or ACTIVE opmode */
   if (pRes->curr_opmode != DXS_SDD_Opmode_Disabled &&
          pRes->curr_opmode != DXS_SDD_Opmode_Active)
   {
      /** errmsg: Wrong line state */
      RETURN_STATUS (DXS_statusAclmStartErrInvOpmode, IFX_NULL);
   }

   if (pRes->curr_opmode == DXS_SDD_Opmode_Disabled)
   {
      /* implicit PDH opmode */
      err = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Active);
      if (!DXS_SUCCESS (err))
         RETURN_STATUS (err, IFX_NULL);

      /* workaround: when the SLIC is in sleep the first opmode change command
         will only wake it up but actual setting of the opmode will take until
         the dup counters have expired. To speed up the process the opmode
         change can be repeated after the SLIC is awake. So do a delay here
         to allow enough time for the wake up. */
      TAPI_OS_MSecSleep(3);
      /* end of workaround */

      /* Get the current opmode - wait while a opmode change is ongoing. */
      err = DXS_ALM_OpmodeGet (pCh, &other_curr_opmode);
      /* Only exit when interrupted by a signal. When waiting was aborted by
         timeout assume that no linemode change is pending and try to continue
         with the opmode change that was requested. */
      if (err != DXS_statusOk ||
            other_curr_opmode != DXS_SDD_Opmode_Active)
      {
         /* errmsg: Neighbour line mode is not DISABLED */
         RETURN_STATUS (DXS_statusAclmStartErrActOpmodeTmout, IFX_NULL);
      }
   }

#if 0 /* Unused feature */
   else
   {
      pRes->bAclmRestoreDisabled = 0;
   }
#endif

   pRes->aclm_current_meas = DXS_ACLM_GT;
   pRes->aclm_meas_status = dxs_aclm_in_progress;

   /* configure the AC measurement */
   pAclmConf->Int = DXS_ACLM_INT_TIME_MS_DEFAULT;
   pAclmConf->Del = DXS_ACLM_AC_DELAY_MS_DEFAULT;
   pAclmConf->BP = 0;
   pAclmConf->TH = 0;
   err = DXS_CmdWrite (pCh->pParent, (uint32_t *)pAclmConf);
   if (!DXS_SUCCESS (err))
   {
      pRes->aclm_meas_status = dxs_aclm_aborted;
      RETURN_STATUS (err, IFX_NULL);
   }
   /* configure TG */
   err = dxs_tone_config (pCh, -869, -100, 0, DXS_ACLM_FR_FREQ_CENTER_HZ);
   if (!DXS_SUCCESS (err))
   {
      pRes->aclm_meas_status = dxs_aclm_aborted;
      RETURN_STATUS (err, IFX_NULL);
   }
   /* enable the TG */
   err = dxs_aclm_tone_start (pCh);
   if (!DXS_SUCCESS (err))
   {
      pRes->aclm_meas_status = dxs_aclm_aborted;
      RETURN_STATUS (err, IFX_NULL);
   }
   /* measure reference tone from inband */
   err = dxs_aclm (pCh);
   if (!DXS_SUCCESS (err))
   {
      pRes->aclm_meas_status = dxs_aclm_aborted;
      /* stop TG */
      dxs_aclm_tone_stop (pCh);
      RETURN_STATUS (err, IFX_NULL);
   }
   AcOutbSh = pAclmRes->AcInbSh; /* Save reference to out-band */
   AcOutb   = pAclmRes->AcInb; /* Save reference to out-band */

   for (level = DXS_ACLM_GT_LEVEL_MIN_DBM0;
        mpoint < DXS_ACLM_GT_MEASUREMENTS;
        mpoint++, level += DXS_ACLM_GT_SNR_LEVEL_STEP_DBM0)
   {
      err = dxs_tone_config(pCh, -869, (level*10), 0, DXS_ACLM_FR_FREQ_CENTER_HZ);
      if (!DXS_SUCCESS (err))
      {
         RETURN_STATUS (err, IFX_NULL);
      }

      /* run the measurement */
      err = dxs_aclm (pCh);
      if (!DXS_SUCCESS (err))
      {
         pRes->aclm_meas_status = dxs_aclm_aborted;
         /* stop TG */
         dxs_aclm_tone_stop (pCh);
         RETURN_STATUS (err, IFX_NULL);
      }

      pMeasMP = &pRes->aclm_results.mp_val[mpoint];
      /* Get the ACLM results from the device */
      pMeasMP->Int      = pAclmConf->Int;
      pMeasMP->AcInbSh  = pAclmRes->AcInbSh;
      pMeasMP->AcInb    = pAclmRes->AcInb;
      pMeasMP->AcOutbSh = AcOutbSh; /* Reference value */
      pMeasMP->AcOutb   = AcOutb; /* Reference value */
      pMeasMP->freq     = DXS_ACLM_FR_FREQ_CENTER_HZ;
      pMeasMP->level    = level;
      pMeasMP->ref_level = -100;
   }
   pRes->aclm_results.mp_count = mpoint;

   /* stop TG */
   dxs_aclm_tone_stop (pCh);

#if 0 /* Unused feature */
   if (pRes->bAclmRestoreDisabled)
   {
      /* implicit PDH opmode */
      err = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Disabled);
      if (!DXS_SUCCESS (err))
         RETURN_STATUS (err, IFX_NULL);
   }
#endif

   pRes->aclm_meas_status = dxs_aclm_finished;

   /* At this point, err can only be success */
   return err;
}

/**
   ACLM function for Signal to Noise Ratio measurement.

   \param   pCh    - pointer to DXS channel structure

   \return
   - DXS_status_t
*/
static IFX_int32_t dxs_aclm_snr_start (DXS_CHANNEL_t *pCh)
{
   struct DXS_ALM *pRes = (struct DXS_ALM *)pCh->pALM;
   IFX_int32_t err;
   IFX_int16_t level;
   IFX_uint8_t mpoint = 0;
   const DXS_SDD_ACLevelMeterResult_t *pAclmRes = &pRes->fw_sdd_aclm_result;
   DXS_SDD_ACLevelMeterConfig_t *pAcLmCfg;
   IFX_TAPI_NLT_ACLM_MP_Result_t *pMeasMP;
   IFX_int16_t rxGain, txGain;
   IFX_uint8_t other_curr_opmode;

   if (pRes->aclm_meas_status == dxs_aclm_in_progress)
   {
      /* errmsg: measurement is already running */
      RETURN_STATUS (DXS_statusAclmInProgress, IFX_NULL);
   }

   /* ACLM start is possible either from DISABLED or ACTIVE opmode */
   if (pRes->curr_opmode != DXS_SDD_Opmode_Disabled &&
          pRes->curr_opmode != DXS_SDD_Opmode_Active)
   {
      /** errmsg: Wrong line state */
      RETURN_STATUS (DXS_statusAclmStartErrInvOpmode, IFX_NULL);
   }

   if (pRes->curr_opmode == DXS_SDD_Opmode_Disabled)
   {
      /* implicit PDH opmode */
      err = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Active);
      if (!DXS_SUCCESS (err))
         RETURN_STATUS (err, IFX_NULL);

      /* workaround: when the SLIC is in sleep the first opmode change command
         will only wake it up but actual setting of the opmode will take until
         the dup counters have expired. To speed up the process the opmode
         change can be repeated after the SLIC is awake. So do a delay here
         to allow enough time for the wake up. */
      TAPI_OS_MSecSleep(3);
      /* end of workaround */

      /* Get the current opmode - wait while a opmode change is ongoing. */
      err = DXS_ALM_OpmodeGet (pCh, &other_curr_opmode);
      /* Only exit when interrupted by a signal. When waiting was aborted by
         timeout assume that no linemode change is pending and try to continue
         with the opmode change that was requested. */
      if (err != DXS_statusOk ||
            other_curr_opmode != DXS_SDD_Opmode_Active)
      {
         /* errmsg: Neighbour line mode is not DISABLED */
         RETURN_STATUS (DXS_statusAclmStartErrActOpmodeTmout, IFX_NULL);
      }
   }

#if 0 /* Unused feature */
   else
   {
      pRes->bAclmRestoreDisabled = 0;
   }
#endif

   pRes->aclm_current_meas = DXS_ACLM_SNR;
   pRes->aclm_meas_status = dxs_aclm_in_progress;

   /* configure the AC measurement */
   pAcLmCfg = &pRes->fw_sdd_aclm_config;
   pAcLmCfg->Int = DXS_ACLM_INT_TIME_MS_DEFAULT;
   pAcLmCfg->Del = 50;
   pAcLmCfg->BP = 1;
   pAcLmCfg->TH = 0;
   pAcLmCfg->BW = 864;
   pAcLmCfg->CF = 22804;
   err = DXS_CmdWrite (pCh->pParent, (IFX_uint32_t *)pAcLmCfg);
   if (!DXS_SUCCESS (err))
   {
      pRes->aclm_meas_status = dxs_aclm_aborted;
      RETURN_STATUS (err, IFX_NULL);
   }

   /* configure TxRx gain to 0 dBr */
   /* DXS_SDD_GainGet always returns DXS_statusOk */
   (void) DXS_SDD_GainGet(pCh, &txGain, &rxGain);

   err = DXS_SDD_GainSet(pCh, 0, rxGain);
   if (!DXS_SUCCESS (err))
   {
      pRes->aclm_meas_status = dxs_aclm_aborted;
      RETURN_STATUS (err, IFX_NULL);
   }


   /* configure TG */
   err = dxs_tone_config(pCh, -869, (DXS_ACLM_SNR_LEVEL_MIN_DBM0 * 10), 0, DXS_ACLM_FR_FREQ_CENTER_HZ);
   if (!DXS_SUCCESS (err))
   {
      pRes->aclm_meas_status = dxs_aclm_aborted;
      RETURN_STATUS (err, IFX_NULL);
   }
   /* enable the TG */
   err = dxs_aclm_tone_start (pCh);
   if (!DXS_SUCCESS (err))
   {
      pRes->aclm_meas_status = dxs_aclm_aborted;
      RETURN_STATUS (err, IFX_NULL);
   }


   for (level = DXS_ACLM_SNR_LEVEL_MIN_DBM0;
        mpoint < DXS_ACLM_SNR_MEASUREMENTS;
        mpoint++, level += DXS_ACLM_GT_SNR_LEVEL_STEP_DBM0)
   {
      err = dxs_tone_config(pCh, -869, (level*10), 0, DXS_ACLM_FR_FREQ_CENTER_HZ);
      if (!DXS_SUCCESS (err))
         break;

      if (level > -10)
      {
         /* set gain 10 dBr */
         DXS_SDD_GainSet(pCh, -100, rxGain);
      }

      /* run the measurement */
      err = dxs_aclm (pCh);
      if (!DXS_SUCCESS (err))
      {
         pRes->aclm_meas_status = dxs_aclm_aborted;
         /* stop TG */
         dxs_aclm_tone_stop (pCh);
         RETURN_STATUS (err, IFX_NULL);
      }

      pMeasMP = &pRes->aclm_results.mp_val[mpoint];
      /* Get the ACLM results from the device */
      pMeasMP->Int      = pAcLmCfg->Int;
      pMeasMP->AcInbSh  = pAclmRes->AcInbSh;
      pMeasMP->AcInb    = pAclmRes->AcInb;
      pMeasMP->AcOutbSh = pAclmRes->AcOutbSh;
      pMeasMP->AcOutb   = pAclmRes->AcOutb;
      pMeasMP->freq     = DXS_ACLM_FR_FREQ_CENTER_HZ;
      pMeasMP->level    = level;
   }
   pRes->aclm_results.mp_count = mpoint;
   /* stop TG */
   dxs_aclm_tone_stop (pCh);

#if 0 /* Unused feature */
   if (pRes->bAclmRestoreDisabled)
   {
      /* implicit PDH opmode */
      err = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Disabled);
      if (!DXS_SUCCESS (err))
         RETURN_STATUS (err, IFX_NULL);
   }
#endif 

   pRes->aclm_meas_status = dxs_aclm_finished;

   /* At this point, err can only be success */
   return err;
}

/**
   ACLM function for Frequency Response and Transhybrid measurements.

   \param   pCh    - pointer to DXS channel structure
   \param   type   - ACLM measurement code

   \return
   - DXS_status_t
*/
static IFX_int32_t dxs_aclm_fr_th_start (DXS_CHANNEL_t *pCh, DXS_ACLM_Measurement_t type)
{
   struct DXS_ALM *pRes = (struct DXS_ALM *)pCh->pALM;
   DXS_SDD_ACLevelMeterConfig_t *pAcLmCfg;
   IFX_int32_t err;
   IFX_uint16_t freq;
   IFX_int16_t level;
   IFX_uint8_t mpoint = 0, measurements = 0;
   const DXS_SDD_ACLevelMeterResult_t *pAclmRes = &pRes->fw_sdd_aclm_result;
   const DXS_SDD_ACLevelMeterConfig_t *pAclmConf = &pRes->fw_sdd_aclm_config;
   IFX_uint8_t other_curr_opmode;
   IFX_TAPI_NLT_ACLM_MP_Result_t *pMeasMP = IFX_NULL;

   if (pRes->aclm_meas_status == dxs_aclm_in_progress)
   {
      /* errmsg: measurement is already running */
      RETURN_STATUS (DXS_statusAclmInProgress, IFX_NULL);
   }

   /* ACLM start is possible either from DISABLED or ACTIVE opmode */
   if (pRes->curr_opmode != DXS_SDD_Opmode_Disabled &&
          pRes->curr_opmode != DXS_SDD_Opmode_Active)
   {
      /** errmsg: Wrong line state */
      RETURN_STATUS (DXS_statusAclmStartErrInvOpmode, IFX_NULL);
   }

   if (pRes->curr_opmode == DXS_SDD_Opmode_Disabled)
   {
      /* implicit PDH opmode */
      err = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Active);
      if (!DXS_SUCCESS (err))
         RETURN_STATUS (err, IFX_NULL);

      /* workaround: when the SLIC is in sleep the first opmode change command
         will only wake it up but actual setting of the opmode will take until
         the dup counters have expired. To speed up the process the opmode
         change can be repeated after the SLIC is awake. So do a delay here
         to allow enough time for the wake up. */
      TAPI_OS_MSecSleep(3);
      /* end of workaround */

      /* Get the current opmode - wait while a opmode change is ongoing. */
      err = DXS_ALM_OpmodeGet (pCh, &other_curr_opmode);
      /* Only exit when interrupted by a signal. When waiting was aborted by
         timeout assume that no linemode change is pending and try to continue
         with the opmode change that was requested. */
      if (err != DXS_statusOk ||
            other_curr_opmode != DXS_SDD_Opmode_Active)
      {
         /* errmsg: Neighbour line mode is not DISABLED */
         RETURN_STATUS (DXS_statusAclmStartErrActOpmodeTmout, IFX_NULL);
      }
   }

#if 0 /* Unused feature */
   else
   {
      pRes->bAclmRestoreDisabled = 0;
   }
#endif

   pRes->aclm_current_meas = type;
   pRes->aclm_meas_status = dxs_aclm_in_progress;

   pAcLmCfg = &pRes->fw_sdd_aclm_config;
   /* configure the AC measurement */
   pAcLmCfg->Int = DXS_ACLM_INT_TIME_MS_DEFAULT;
   pAcLmCfg->Del = DXS_ACLM_AC_DELAY_MS_DEFAULT;
   pAcLmCfg->BP = 0;
   if (type == DXS_ACLM_FR)
   {
      measurements = DXS_ACLM_FR_MEASUREMENTS;
      freq = DXS_ACLM_FR_FREQ_CENTER_HZ;
      level = -100; /* -10.0 dBm0 */
      pAcLmCfg->TH = 0;
   }
   else
   {
      measurements = DXS_ACLM_TH_MEASUREMENTS;
      freq = DXS_ACLM_FR_TH_FREQ_MIN_HZ;
      level = 0; /* 0.0 dBm0 */
      pAcLmCfg->TH = 1;
   }
   err = DXS_CmdWrite (pCh->pParent, (uint32_t *)pAcLmCfg);
   if (!DXS_SUCCESS (err))
   {
      pRes->aclm_meas_status = dxs_aclm_aborted;
      RETURN_STATUS (err, IFX_NULL);
   }
   /* configure TG */
   err = dxs_tone_config (pCh, -869, level, 0, freq);
   if (!DXS_SUCCESS (err))
   {
      pRes->aclm_meas_status = dxs_aclm_aborted;
      RETURN_STATUS (err, IFX_NULL);
   }

   /* enable the TG */
   err = dxs_aclm_tone_start (pCh);
   if (!DXS_SUCCESS (err))
   {
      pRes->aclm_meas_status = dxs_aclm_aborted;
      RETURN_STATUS (err, IFX_NULL);
   }

   for (freq = DXS_ACLM_FR_TH_FREQ_MIN_HZ;
        mpoint < measurements;
        mpoint++, freq += DXS_ACLM_FR_TH_FREQ_STEP_HZ)
   {
      if (mpoint >= IFX_TAPI_ACLM_MAX_MP_RESULTS)
         /* Should never happens */
         RETURN_STATUS (DXS_statusChErr, IFX_NULL);

      /* configure TG */
      err = dxs_tone_config(pCh, -869, level, 0, freq);
      if (!DXS_SUCCESS (err))
         RETURN_STATUS (err, IFX_NULL);

      /* run the measurement */
      err = dxs_aclm (pCh);
      if (!DXS_SUCCESS (err))
         RETURN_STATUS (err, IFX_NULL);

      pMeasMP = &pRes->aclm_results.mp_val[mpoint];
      /* Get the ACLM results from the device */
      pMeasMP->Int      = pAclmConf->Int;
      pMeasMP->AcInbSh  = pAclmRes->AcInbSh;
      pMeasMP->AcInb    = pAclmRes->AcInb;
      pMeasMP->AcOutbSh = pAclmRes->AcOutbSh;
      pMeasMP->AcOutb   = pAclmRes->AcOutb;
      pMeasMP->freq     = freq;
      pMeasMP->level    = level;
   }
   pRes->aclm_results.mp_count = mpoint;
   /* stop TG */
   dxs_aclm_tone_stop (pCh);

#if 0 /* Unused feature */
   if (pRes->bAclmRestoreDisabled)
   {
      /* implicit PDH opmode */
      err = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Disabled);
      if (!DXS_SUCCESS (err))
         RETURN_STATUS (err, IFX_NULL);
   }
#endif

   pRes->aclm_meas_status = dxs_aclm_finished;

   RETURN_STATUS (err, IFX_NULL);
}



/* ========================================================================== */
/*                         API Function implementation                        */
/* ========================================================================== */

/**
   ACLM driver interface function for starting measurement.

   \param   pLLChannel  - pointer to DXS channel structure
            pArg        - pointer to structure
                          used during start of an NLT test.

   \return
   - DXS_status_t
*/
IFX_int32_t DXS_TAPI_LL_ALM_NLT_Test_Start (IFX_TAPI_LL_CH_t *pLLChannel,
                                            const IFX_TAPI_NLT_TEST_START_t *pArg)
{

   DXS_CHANNEL_t         *pCh  = (DXS_CHANNEL_t *) pLLChannel;
   IFX_int32_t            ret     = IFX_SUCCESS;

   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   /* sanity check */
   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   if (pArg == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("%s: pArg is NULL\n", __FUNCTION__));
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }

   /* protect channel data from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   switch (pArg->testID)
   {
   case IFX_TAPI_NLT_AC_TRANSHYBRID_ID:

      ret = dxs_aclm_fr_th_start (pCh, DXS_ACLM_TH);
      break;
   case IFX_TAPI_NLT_AC_FREQRESPONSE_ID:
      ret = dxs_aclm_fr_th_start (pCh, DXS_ACLM_FR);
      break;
   case IFX_TAPI_NLT_AC_GAINTRACKING_ID:
      ret = dxs_aclm_gt_start (pCh);
      break;
   case  IFX_TAPI_NLT_AC_IDLENOISE_ID:
      ret = dxs_aclm_snr_start (pCh);
      break;
   default:
      /* release channel lock */
      TAPI_OS_MutexRelease (&pCh->mtxChAcc);
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }
   /* release channel lock */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   ACLM interface function for measurement results retriving.

   \param   pLLChannel  - pointer to DXS channel structure
            pArg        - pointer to structure used for
                          reading NLT test results.

   \return
   - DXS_status_t
*/
IFX_int32_t DXS_TAPI_LL_ALM_NLT_Results_Get(IFX_TAPI_LL_CH_t *pLLChannel,
                                            const IFX_TAPI_NLT_RESULT_GET_t *pArg)
{

   DXS_CHANNEL_t         *pCh = (DXS_CHANNEL_t *) pLLChannel;
   IFX_int32_t            ret = IFX_SUCCESS;
   IFX_TAPI_NLT_ACLM_Result_t *pMeasRes;

   if (pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   /* sanity check */
   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   if (pArg == IFX_NULL)
   {
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }

   if (pArg->pTestResults == IFX_NULL)
   {
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }

   /* Note: The typecase is ok while the local enum values are derived from
      the external enum values. */
   if (pArg->testID != (IFX_TAPI_NLT_TESTID_t)pCh->pALM->aclm_current_meas)
   {
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }

   if (pCh->pALM->aclm_meas_status != dxs_aclm_finished)
   {
      /** errmsg: ACLM results not available */
      RETURN_STATUS (DXS_statusAclmResultsNotAvail, IFX_NULL);
   }

   /* protect channel data from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   pMeasRes = (IFX_TAPI_NLT_ACLM_Result_t *)pArg->pTestResults;
   /* Copy the AC level Meter measurement Results */
   TAPI_OS_CpyKern2Usr((IFX_void_t*)pMeasRes,
                      (const IFX_void_t *)&pCh->pALM->aclm_results,
                       sizeof(IFX_TAPI_NLT_ACLM_Result_t));

   /* release channel lock */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);
   return ret;
}

/**
   Function called by init_module of device, fills up NLT function pointers
   which are passed to HL TAPI during registration.

   \param  pAlm         Pointer to ALM module.
*/
IFX_void_t DXS_NLT_Func_Register (IFX_TAPI_DRV_CTX_NLT_t *pNLT)
{
   /* Fill the function pointers of NLT module */
   pNLT->NLT_test_start          = DXS_TAPI_LL_ALM_NLT_Test_Start;
   pNLT->NLT_result_get          = DXS_TAPI_LL_ALM_NLT_Results_Get;
}

#endif /* DXS_FEAT_NLT */

