/******************************************************************************

  Copyright 2014-2015 Lantiq Deutschland GmbH
  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016, Intel Corporation.
  Copyright 2021,2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_dtmf.c
   This file contains the implementation of dtmf/at generator related
   functions of DUSLIC XS.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_dtmf_priv.h"
#include "drv_dxs_dtmf.h"
#include "drv_dxs_cid_priv.h"
#include "drv_dxs_alm_priv.h"
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

/** Offset of the 0dB table index for dB -> HEX conversion. */
static const IFX_int32_t DXS_DTMF_RX_ATT_OFFSET = 40;

/**
   Precalculated attenuation table for the DTMF receiver in steps of 1dB.
   LEVEL = 29205 * 10^((Level[dBFS])/10)    Level: -40 dBFS = X = -6 dBFS
   TWIST = 29205 * 10^(-Twist[dB] / 10)     Twist: 1 dB = X = 12 dB
*/
static const IFX_uint16_t Dxs_DtmfRxAtt[] =
{
   /* -40    -39    -38    -37    -36    -35    -34    -33    -32    -31 */
        3,     4,     5,     6,     7,     9,    12,    15,    18,    23,
   /* -30    -29    -28    -27    -26    -25    -24    -23    -22    -21 */
       29,    37,    46,    58,    73,    92,   116,   146,   184,   232,
   /* -20    -19    -18    -17    -16    -15    -14    -13    -12    -11 */
      292,   368,   463,   583,   734,   924,  1163,  1464,  1843,  2320,
   /* -10     -9     -8     -7     -6     -5     -4     -3     -2     -1 */
     2921,  3677,  4629,  5827,  7336,  9235, 11627, 14637, 18427, 23198,
   /*   0   */
    29205
};

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_int32_t DXS_DTMF_REC_COEF (DXS_CHANNEL_t *pCh);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */
/**
   play a local tone on one or both tone generators

   \param  pCh          Pointer to the channel structure.
   \param  pDtmfAt      Pointer to struct with additional parameters.

   \return
   - DXS_statusOk
   - DXS_statusDtmfAtFreqCfgErr
   - DXS_statusDtmfAtLevCfgErr
   - DXS_statusDtmfAtStartErr
*/
IFX_int32_t DXS_DTMF_AT_CTRL(DXS_CHANNEL_t *pCh,
                             const struct DXS_DTMF_AT_CFG *pDtmfAt)
{
   IFX_int32_t             err               = DXS_statusOk;
   DXS_DEVICE_t            *pDev             = pCh->pParent;
   IFX_uint8_t             ch                = pCh->nChannel - 1;
   DXS_DTMF_AT_GEN_COEF_t  *pDtmfAtGenCoef   = IFX_NULL;
   DXS_DTMF_AT_GEN_CTRL_t  *pDtmfAtGenCtrl   = IFX_NULL;

   /* protect fwmsg against concurrent tasks */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   pDtmfAtGenCtrl = &pCh->pDTMF->dtmf_at_gen_ctrl;

   /*
      If the tone generator should be enabled and is currently running
      disable it before changing the coefficients. To only disable it
      the same code here is used.
   */
   if ((pDtmfAt->nEnDis == DXS_DTMF_AT_CTRL_DIS) ||
       (pDtmfAtGenCtrl->EN == DTMF_AT_GEN_CTRL_EN_EN))
   {
      pDtmfAtGenCtrl->EN = DTMF_AT_GEN_CTRL_EN_DIS;
      /* check if the voice path shall be re-enabled */
      switch (pDtmfAt->nAdd)
      {
         case DXS_DTMF_AT_ADD_DTMF_AT:
            err = DXS_PCM_ChRxMute (pCh, IFX_DISABLE);
            if (!DXS_SUCCESS (err))
            {
               /* unlock */
               TAPI_OS_MutexRelease (&pCh->mtxChAcc);
               return err;
            }
            break;
         case DXS_DTMF_AT_ADD_DTMF_AT_MUTE:
            err = DXS_PCM_ChRxMute (pCh, IFX_ENABLE);
            if (!DXS_SUCCESS (err))
            {
               /* unlock */
               TAPI_OS_MutexRelease (&pCh->mtxChAcc);
               return err;
            }
            break;
         default:
            /* error */
            break;
      }

      err = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)pDtmfAtGenCtrl);
      if (!DXS_SUCCESS (err))
      {
         /* unlock */
         TAPI_OS_MutexRelease (&pCh->mtxChAcc);
         /* errmsg: Stopping the dtmf/at generator failed. */
         RETURN_STATUS(DXS_statusDtmfAtStopErr, IFX_NULL);
      }
   }

   if (pDtmfAt->nEnDis == DXS_DTMF_AT_CTRL_EN)
   {
      /* The tone generator should be enabled.
         First set the level and frequencies. */
      pDtmfAtGenCoef = &pCh->pDTMF->dtmf_at_gen_coef;
      pDtmfAtGenCoef->CHAN = ch;
      /* Set the frequencies in the FW message. */
      /*
         calculate frequency parameter:
         freqX = (X [Hz] / 4000) * 32768
      */
      pDtmfAtGenCoef->FREQ1 = pDtmfAt->nFreq1 * 32768 / 4000;
      pDtmfAtGenCoef->FREQ2 = pDtmfAt->nFreq2 * 32768 / 4000;

      /* Set the levels of the two tone generators. */
      pDtmfAtGenCoef->LEVEL1 = DXS_ALM_rx_level_convert(pCh, pDtmfAt->nLevel1);
      pDtmfAtGenCoef->LEVEL2 = DXS_ALM_rx_level_convert(pCh, pDtmfAt->nLevel2);

#if 0
      /* Warning: this printout for debugging will cause a significant delay
         that affects the timing of the tones to be played. */
      TRACE(TAPI_DXS, DBG_LEVEL_LOW,
            ("DXS_INFO: Parameter provided: Freq1 = %d Hz, Freq2 = %d Hz\n"
            "calculated for message: Freq1 = 0x%x, Freq = 0x%x\n",
            pDtmfAt->nFreq1, pDtmfAt->nFreq2,
            pDtmfAtGenCoef->FREQ1, pDtmfAtGenCoef->FREQ2));
      TRACE(TAPI_DXS, DBG_LEVEL_LOW,
            ("DXS_INFO: Parameter provided: Lev1 = %d dB, Lev2 = %d dB\n"
            "calculated for message: Lev1 = 0x%x, Lev2 = 0x%x\n",
            pDtmfAt->nLevel1, pDtmfAt->nLevel2,
            pDtmfAtGenCoef->LEVEL1, pDtmfAtGenCoef->LEVEL2));
#endif

      /* Write the generator data only if it has changed since the message
         was last written. */
      if (0 != memcmp(pDtmfAtGenCoef,
                      &pCh->pDTMF->dtmf_at_gen_coef_written,
                      sizeof(DXS_DTMF_AT_GEN_COEF_t)))
      {
         err = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)pDtmfAtGenCoef);
         if (!DXS_SUCCESS (err))
         {
            /* unlock */
            TAPI_OS_MutexRelease (&pCh->mtxChAcc);
            /* errmsg: Configuring the dtmf/at level failed.*/
            RETURN_STATUS(DXS_statusDtmfAtLevCfgErr, IFX_NULL);
         }
         memcpy(&pCh->pDTMF->dtmf_at_gen_coef_written, pDtmfAtGenCoef,
                sizeof(DXS_DTMF_AT_GEN_COEF_t));
      }

      /* Set the mode of mixing the tone generator signal with the voice path.*/
      switch (pDtmfAt->nAdd)
      {
         case DXS_DTMF_AT_ADD_DTMF_AT:
            err = DXS_PCM_ChRxMute (pCh, IFX_DISABLE);
            if (!DXS_SUCCESS (err))
            {
               /* unlock */
               TAPI_OS_MutexRelease (&pCh->mtxChAcc);
               return err;
            }
            break;
         case DXS_DTMF_AT_ADD_DTMF_AT_MUTE:
            err = DXS_PCM_ChRxMute (pCh, IFX_ENABLE);
            if (!DXS_SUCCESS (err))
            {
               /* unlock */
               TAPI_OS_MutexRelease (&pCh->mtxChAcc);
               return err;
            }
            break;
         default:
            /* error */
            break;
      }

      /* Finally enable the tone generators. */
      pDtmfAtGenCtrl->EN = DTMF_AT_GEN_CTRL_EN_EN;
      err = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)pDtmfAtGenCtrl);
      if (!DXS_SUCCESS (err))
      {
         /* unlock */
         TAPI_OS_MutexRelease (&pCh->mtxChAcc);
         /* errmsg: Starting the dtmf/at generator failed.*/
         RETURN_STATUS(DXS_statusDtmfAtStartErr, IFX_NULL);
      }
   }
   /* unlock */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);
   return DXS_statusOk;
}


/**
   This function converts the given TAPI DTMF code to the DUSLIC XS specific
   table index. The index can be used as an entry to the table containing the
   frequency and level settings for all DTMF digits.

   \param nChar       Digit to encode

   \param pDtmfCode   Pointer to return DTMF code

   \return
   None.
*/
IFX_void_t DXS_DTMF_GetTblIndex (IFX_char_t nChar, IFX_uint8_t *pDtmfCode)
{
   if (pDtmfCode == IFX_NULL)
      return;

   switch (nChar)
   {
      case 'A':
         *pDtmfCode = 0x0A;
         break;
      case 'B':
         *pDtmfCode = 0x0B;
         break;
      case 'C':
         *pDtmfCode = 0x0C;
         break;
      case 'D':
         *pDtmfCode = 0x0D;
         break;
      case '*':
         *pDtmfCode = 0x0E;
         break;
      case '#':
         *pDtmfCode = 0x0F;
         break;
      default:
         if ((nChar >= '0') && (nChar <= '9'))
            *pDtmfCode = (IFX_uint8_t)nChar - '0';
         break;
   }
   return;
}

/**
   This function translates the given FW specific DTMF code to the corresponding
   TAPI DTMF coding.

   \param fwDtmfCode       Digit to encode in FW specific encoding

   \return Returns the translated DTMF digit in TAPI specific encoding
*/
IFX_uint8_t DXS_DTMF_encode_fw2tapi (IFX_uint8_t fwDtmfCode)
{
   if (fwDtmfCode <= 0x0F)
   {
      switch (fwDtmfCode)
      {
      case 0x00:
         return 0x0B;
      case 0x0B:
         return 0x0C;
      default:
         if (fwDtmfCode <= 0x0A)
            return fwDtmfCode;
         else if ((fwDtmfCode >= 0x0C) && (fwDtmfCode <= 0x0F))
            return fwDtmfCode + 0x10;
      }
   }
   /* no key */
   return 0x00;
}

/**
   This function converts the given FW specific DTMF code to the corresponding
   ASCII character coding.

   \param fwDtmfCode       Digit to encode in FW specific encoding

   \return  Returns the translated DTMF digit in ASCII encoding. In case the FW
            code can not be decoded an 'X' is returned.

   \remarks
   For the dxs firmware, following caracters are coded as follows:
       '*' = 0x2A (ASCII) = 0x0A (DUSLIC XS)
       '#' = 0x23 (ASCII) = 0x0B (DUSLIC XS)
       'A' = 0x41 (ASCII) = 0x0C (DUSLIC XS)
       'B' = 0x42 (ASCII) = 0x0D (DUSLIC XS)
       'C' = 0x43 (ASCII) = 0x0E (DUSLIC XS)
       'D' = 0x44 (ASCII) = 0x0F (DUSLIC XS)
       '0' - '9'          = 0x00 - 0x09
*/
IFX_char_t DXS_DTMF_encode_fw2ascii (IFX_uint8_t fwDtmfCode)
{
   if (fwDtmfCode <= 0x0F)
   {
      switch (fwDtmfCode)
      {
         case 0x0A:
            return '*';
         case 0x0B:
            return '#';
         default:
            if (fwDtmfCode <= 0x09)
               return fwDtmfCode + '0';
            else if ((fwDtmfCode >= 0x0C) && (fwDtmfCode <= 0x0F))
               return (fwDtmfCode - 0x0C) + 'A';
      }
   }
   /* return 'X' in case the FW code could not be decoded */
   return 'X';
}


/**
   activate/deactivate DTMF receiver

   \param pCh  handle to a DuSLIC channel structure

   \param bEn  IFX_TRUE = activate, IFX_FALSE = deactivate DTMF receiver

   \return
   - DXS_statusOk
   - DXS_statusDtmfRcvCtrlErr
*/
IFX_int32_t DXS_DTMF_REC_CTRL (
                        DXS_CHANNEL_t *pCh,
                        IFX_boolean_t bEn)
{
   IFX_int32_t          ret   = DXS_statusOk;
   DXS_DEVICE_t         *pDev = pCh->pParent;
   DXS_DTMF_REC_CTRL_t  *pDtmfRxCtrl;

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   pDtmfRxCtrl = &pCh->pDTMF->dtmf_rec_ctrl;
   if (bEn)
   {
      pDtmfRxCtrl->EN = DTMF_REC_CTRL_EN_EN;
   }
   else
   {
      pDtmfRxCtrl->EN = DTMF_REC_CTRL_EN_DIS;
   }
   ret = DXS_CmdWrite (pDev, (IFX_uint32_t *)(IFX_void_t *)pDtmfRxCtrl);
   /* unlock */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   if (!DXS_SUCCESS (ret))
   {
      /* errmsg: Activation or deactivation of DTMF receiver failed.*/
      ret = DXS_statusDtmfRcvCtrlErr;
   }

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   set paramter for DTMF receiver

   \param pCh  handle to a DuSLIC channel structure

   \return
   - DXS_statusOk
   - DXS_statusDtmfRcvCoefErr
*/
static IFX_int32_t DXS_DTMF_REC_COEF (
                        DXS_CHANNEL_t *pCh)
{
   IFX_int32_t          ret   = DXS_statusOk;
   DXS_DEVICE_t         *pDev = pCh->pParent;
   DXS_DTMF_REC_COEF_t  *pDtmfRxCoef;

   /* protect fwmsg against concurrent tasks */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   pDtmfRxCoef = &pCh->pDTMF->dtmf_rec_coef;
   /* Write the DTMF Receiver Coefficients */
   pDtmfRxCoef->LEVEL = pCh->pDTMF->dtmfReceive.level;
   pDtmfRxCoef->TWIST = pCh->pDTMF->dtmfReceive.twist;
   ret = DXS_CmdWrite (pDev, (IFX_uint32_t *)(IFX_void_t *)pDtmfRxCoef);
   /* unlock */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   if (!DXS_SUCCESS (ret))
   {
      /* errmsg: Setting DTMF receiver coefficients failed.*/
      ret = DXS_statusDtmfRcvCoefErr;
   }

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Sets DTMF Receiver Coefficients

   \param  pLLChannel   Pointer to TAPI LL channel structure.
   \param  bRW          IFX_TRUE to read (not supported), IFX_FALSE to
                        write coefficients.
   \param pCoeff        Pointer to DTMF Rx coefficients data structure.

   \return
   - DXS_statusOk
   - DXS_statusInvalCh
   - DXS_statusDtmfRcvLevErr DTMF receiver level parameter out of range.
   - DXS_statusDtmfRcvTwistErr DTMF receiver twist parameter out of range.
   - DXS_statusFuncParam if at least one parameter in function is wrong
   - DXS_statusErr if channel pointer is null
   - error code from mailbox access

   \remarks
   Setting of the DTMF coefficients is only allowed while the DTMF receiver
   is disabled. As a result, if setting of the coefficients is attempted while
   the DTMF receiver is enabled, it will be disabled temporarily in order to
   write the coefficients, and reenabled again.
*/
IFX_return_t DXS_TAPI_LL_DTMF_RX_CFG (
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_boolean_t bRW,
                        IFX_TAPI_DTMF_RX_CFG_t *pCoeff)
{
   IFX_int32_t          ret   = DXS_statusOk;
   DXS_CHANNEL_t        *pCh  = (DXS_CHANNEL_t *) pLLChannel;
   DXS_DEVICE_t         *pDev;
   DXS_DTMF_REC_COEF_t  *pDtmfRxCoef;
   IFX_boolean_t        bDtmfRecAct = IFX_FALSE;

   if((pLLChannel == IFX_NULL) || (pCh->pParent == IFX_NULL))
      return DXS_statusErr;

   pDev = pCh->pParent;

   if(pCh->pDTMF == IFX_NULL)
      RETURN_STATUS(DXS_statusInvalCh, IFX_NULL);

   pDtmfRxCoef = &pCh->pDTMF->dtmf_rec_coef;

   if(pCoeff == IFX_NULL)
      RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);

   if (bRW == IFX_FALSE)
   {
      /* Write DTMF receiver coefficients */

      /*
         Lookup LEVEL coefficient value
      */
      if (pCoeff->nLevel < DXS_DTMF_RX_LEVEL_MIN ||
          pCoeff->nLevel > DXS_DTMF_RX_LEVEL_MAX)
      {
         /* parameter is out of supported range */
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                ("DXS_ERR: DTMF Receiver Level out of range (%d),"
                 "               (allowed range %d..%d dB)\n",
                 pCoeff->nLevel, DXS_DTMF_RX_LEVEL_MIN, DXS_DTMF_RX_LEVEL_MAX));
         /* errmsg: DTMF receiver level parameter out of range.*/
         ret = DXS_statusDtmfRcvLevErr;
         goto error;
      }

      /*
         Lookup TWIST coefficient value
      */
      if (pCoeff->nTwist < DXS_DTMF_RX_TWIST_MIN ||
          pCoeff->nTwist > DXS_DTMF_RX_TWIST_MAX)
      {
         /* parameter is out of supported range */
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
                ("DXS_ERR: DTMF Receiver Twist out of range (%d),"
                 "               (allowed range %d..%d dB)\n",
                 pCoeff->nTwist, DXS_DTMF_RX_TWIST_MIN, DXS_DTMF_RX_TWIST_MAX));
         /* errmsg: DTMF receiver twist parameter out of range.*/
         ret = DXS_statusDtmfRcvTwistErr;
         goto error;
      }

      /* LEVEL = 29205 * 10^((Level[dBFS])/10) */
      pCh->pDTMF->dtmfReceive.level =
         Dxs_DtmfRxAtt[pCoeff->nLevel + DXS_DTMF_RX_ATT_OFFSET];
      /* TWIST = 29205 * 10^(-Twist[dB] / 10) */
      pCh->pDTMF->dtmfReceive.twist =
         Dxs_DtmfRxAtt[-pCoeff->nTwist + DXS_DTMF_RX_ATT_OFFSET];

      /* Check first if DTMF Receiver is enabled */
      if (pCh->pDTMF->dtmf_rec_ctrl.EN)
      {
         /* DTMF Receiver is enabled, however must be disabled before
            writing the DTMF Receiver Coefficients */
         ret = DXS_DTMF_REC_CTRL(pCh, IFX_FALSE);
         if (DXS_statusOk != ret)
            goto error;

         /* remember to re-activate DTMF receiver after changing coefficients */
         bDtmfRecAct = IFX_TRUE;
      }

      ret = DXS_DTMF_REC_COEF(pCh);
      if (DXS_statusOk != ret)
            goto error;

      if (bDtmfRecAct)
      {
         /* Reenable the DTMF Receiver, as it was initially enabled */
         ret = DXS_DTMF_REC_CTRL(pCh, IFX_TRUE);
         if (DXS_statusOk != ret)
               goto error;
      }
   }
   else
   {
      IFX_uint32_t i;
      const IFX_uint32_t nTableLen = ARRAY_SIZE(Dxs_DtmfRxAtt);

      ret = DXS_CmdRead (pDev, (IFX_uint32_t *)(IFX_void_t *)pDtmfRxCoef,
                               (IFX_uint32_t *)(IFX_void_t *)pDtmfRxCoef);
      if (DXS_statusOk != ret)
            goto error;

      /* Lookup LEVEL db value */
      for (i = 0; i < nTableLen; i++)
      {
         if ((i == 0) &&
             (pDtmfRxCoef->LEVEL < Dxs_DtmfRxAtt[0]))
            break;

         if ((pDtmfRxCoef->LEVEL == Dxs_DtmfRxAtt[i]) ||
             ((i < nTableLen - 1) &&
              (pDtmfRxCoef->LEVEL > Dxs_DtmfRxAtt[i]) &&
              (pDtmfRxCoef->LEVEL < Dxs_DtmfRxAtt[i+1])))
            break;
      }
      pCoeff->nLevel = i - DXS_DTMF_RX_ATT_OFFSET;

      /* Lookup TWIST dB value */
      for (i = 0; i < nTableLen; i++)
      {
         if ((i == 0) &&
             (pDtmfRxCoef->TWIST < Dxs_DtmfRxAtt[0]))
            break;

         if ((pDtmfRxCoef->TWIST == Dxs_DtmfRxAtt[i]) ||
             ((i < nTableLen - 1) &&
              (pDtmfRxCoef->TWIST > Dxs_DtmfRxAtt[i]) &&
              (pDtmfRxCoef->TWIST < Dxs_DtmfRxAtt[i+1])))
            break;
      }
      pCoeff->nTwist = -(i - DXS_DTMF_RX_ATT_OFFSET);
   }

error:
   TRACE(TAPI_DXS, DBG_LEVEL_LOW,
          ("DTMF receiver coefficients %s (%s): LEVEL=%04x(%ddB), "
           "TWIST=%04x(%ddB)\n", bRW == IFX_FALSE ? "written" : "read",
           ret == IFX_FALSE ? "success" : "error",
           pCh->pDTMF->dtmfReceive.level, pCoeff->nLevel,
           pCh->pDTMF->dtmfReceive.twist, pCoeff->nTwist));

   RETURN_STATUS(ret, IFX_NULL);
}


/**
   Initalize the cached firmware message for the analog line / Smart
   Device Driver.

   \param  pCh          Pointer to the channel structure.

   \return
   None.
*/
IFX_void_t DXS_DTMF_AT_InitCh (DXS_CHANNEL_t *pCh)
{
   struct DXS_DTMF_AT      *pDtmf            = pCh->pDTMF;
   DXS_DTMF_AT_GEN_COEF_t  *pDtmfAtGenCoef   = IFX_NULL,
                           *pDtmfAtGenCoefLast = IFX_NULL;
   DXS_DTMF_AT_GEN_CTRL_t  *pDtmfAtGenCtrl   = IFX_NULL;
   DXS_DTMF_REC_CTRL_t     *pDtmfRecCtrl     = IFX_NULL;
   DXS_DTMF_REC_COEF_t     *pDtmfRecCoef     = IFX_NULL;
   IFX_uint8_t             ch                = pCh->nChannel - 1;

   /* initialize FW message for activation/deactivation of dtmf/at generator */
   pDtmfAtGenCtrl = &pDtmf->dtmf_at_gen_ctrl;
   memset (pDtmfAtGenCtrl, 0, sizeof(*pDtmfAtGenCtrl));

   pDtmfAtGenCtrl->CMD     = DXS_CMD_CMD_EOP;
   pDtmfAtGenCtrl->CHAN    = ch;
   pDtmfAtGenCtrl->MOD     = DXS_CMD_MOD_SIG_GEN;
   pDtmfAtGenCtrl->ECMD    = DTMF_AT_GEN_CTRL_ECMD_EOP_DTMFATGEN;
   pDtmfAtGenCtrl->LENGTH  = DTMF_AT_GEN_CTRL_LENGTH;
   pDtmfAtGenCtrl->EN      = DTMF_AT_GEN_CTRL_EN_DIS;

   /* initialize FW message for configuration of dtmf/at signal levels and
      frequencies. */
   pDtmfAtGenCoef = &pDtmf->dtmf_at_gen_coef;
   memset (pDtmfAtGenCoef, 0, sizeof (*pDtmfAtGenCoef));
   /* reset last written FW message */
   pDtmfAtGenCoefLast = &pDtmf->dtmf_at_gen_coef_written;
   memset (pDtmfAtGenCoefLast, 0, sizeof (*pDtmfAtGenCoefLast));

   pDtmfAtGenCoef->CMD     = DXS_CMD_CMD_EOP;
   pDtmfAtGenCoef->CHAN    = ch;
   pDtmfAtGenCoef->MOD     = DXS_CMD_MOD_SIG_GEN;
   pDtmfAtGenCoef->ECMD    = DTMF_AT_GEN_COEF_ECMD_EOP_DTMFATCOEFF;
   pDtmfAtGenCoef->LENGTH  = DTMF_AT_GEN_COEF_LENGTH;

   /* initialize FW message for activation/deactivation of the DTMF receiver */
   pDtmfRecCtrl = &pDtmf->dtmf_rec_ctrl;
   memset (pDtmfRecCtrl, 0, sizeof (*pDtmfRecCtrl));

   pDtmfRecCtrl->CMD       = DXS_CMD_CMD_EOP;
   pDtmfRecCtrl->CHAN      = ch;
   pDtmfRecCtrl->MOD       = DXS_CMD_MOD_SIG_DET;
   pDtmfRecCtrl->ECMD      = DTMF_REC_CTRL_ECMD_EOP_DTMFREC;
   pDtmfRecCtrl->LENGTH    = DTMF_REC_CTRL_LENGTH;

   /* initialize FW message for configuration of the DTMF receiver */
   pDtmfRecCoef = &pDtmf->dtmf_rec_coef;
   memset (pDtmfRecCoef, 0, sizeof (*pDtmfRecCoef));

   pDtmfRecCoef->CMD       = DXS_CMD_CMD_EOP;
   pDtmfRecCoef->CHAN      = ch;
   pDtmfRecCoef->MOD       = DXS_CMD_MOD_SIG_DET;
   pDtmfRecCoef->ECMD      = DTMF_REC_COEF_ECMD_EOP_DTMF_REC_COEFF;
   pDtmfRecCoef->LENGTH    = DTMF_REC_COEF_LENGTH;

   /* initialize (create) dial timer */
   pDtmf->dtmfSend.dtmfTimerId =
      TAPI_Create_Timer((TIMER_ENTRY)DXS_TCB_DTMF, (IFX_uintptr_t)(pCh));
}


/**
   Allocate data structure of the TG module for the given channel.

   \param  pCh          Pointer to the channel structure.

   \return
   - DXS_statusOk
   - DXS_statusNoMem    in case the stucture could not be created

   \remarks The channel parameter is not checked because the calling
   function assures correct values.
*/
IFX_int32_t DXS_DTMF_AT_Allocate_Ch_Structures (DXS_CHANNEL_t *pCh)
{
   DXS_DTMF_AT_Free_Ch_Structures (pCh);

   pCh->pDTMF = TAPI_OS_Malloc(sizeof(*pCh->pDTMF));
   if (pCh->pDTMF == IFX_NULL)
   {
      /* errmsg: No memory could be allocated. */
      RETURN_STATUS(DXS_statusNoMem,IFX_NULL);
   }
   memset(pCh->pDTMF, 0, sizeof(*pCh->pDTMF));

   return DXS_statusOk;
}


/**
   Free data structure of the TG module in the given channel.

   \param  pCh             Pointer to the channel structure.
*/
IFX_void_t DXS_DTMF_AT_Free_Ch_Structures (DXS_CHANNEL_t *pCh)
{
   if (pCh->pDTMF != IFX_NULL)
   {
      if (pCh->pDTMF->dtmfSend.dtmfTimerId != 0)
      {
         TAPI_Delete_Timer (pCh->pDTMF->dtmfSend.dtmfTimerId);
      }

      TAPI_OS_Free(pCh->pDTMF);
      pCh->pDTMF = IFX_NULL;
   }
}
