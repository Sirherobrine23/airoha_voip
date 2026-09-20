/******************************************************************************

  Copyright 2014-2015 Lantiq Deutschland GmbH
  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016, 2020 Intel Corporation.
  Copyright 2021, 2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_alm.c
   This file contains the implementations of tapi low level functions for
   the ALM module.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include <drv_tapi_config.h>
#include "drv_dxs_api.h"
#include "drv_dxs_alm_priv.h"
#include "drv_dxs_errno.h"
#include "drv_dxs_alm_lt.h"
#include "drv_dxs_init.h"
#include "drv_dxs_fw_cmd_sdd.h"
#include "drv_dxs_mbx.h"
#include "drv_dxs_linux.h"

#ifdef DXS_FEAT_TONE_GENERATOR
   #include "drv_dxs_tg.h"
#endif

#ifdef DXS_FEAT_NLT
   #include "drv_dxs_alm_aclm.h"
#endif

#include "../tapi/drv_tapi_debug_buffer.h"

#include <ifxos_time.h>

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/* Limits for useful values returned by the calibration process. */
#define DXS_SDD_CALIBRATE_TXOFFSET_MAX +983
#define DXS_SDD_CALIBRATE_TXOFFSET_MIN -983
#define DXS_SDD_CALIBRATE_IDACGAIN_MAX +32183
#define DXS_SDD_CALIBRATE_IDACGAIN_MIN +26331
#define DXS_SDD_CALIBRATE_RINGOFFSET_MAX +1966
#define DXS_SDD_CALIBRATE_RINGOFFSET_MIN -1966
/* timeout in ms waiting for calibration finish */
#define DXS_SDD_CALIBRATE_TMOUT_MS     500

#if TAPI_BYTE_ORDER == TAPI_LITTLE_ENDIAN
   #define DXS_LT_FLOAT_MIN_5000000 0xCA989680 /* -5000000.0f */
   #define DXS_LT_FLOAT_MIN_1500000 0xC9B71B00 /* -1500000.0f */
   #define DXS_LT_FLOAT_7p5 0x40F00000 /* 7.50f */
   #define DXS_LT_FLOAT_15  0x41700000 /* 15.0f */

#elif TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   #define DXS_LT_FLOAT_MIN_5000000 0x809698CA /* -5000000.0f */
   #define DXS_LT_FLOAT_MIN_1500000 0x001BB7C9 /* -1500000.0f */
   #define DXS_LT_FLOAT_7p5 0x0000F040 /* 7.50f */
   #define DXS_LT_FLOAT_15  0x00007041 /* 15.0f */
#else
   #error Endianess not defined! Cannot choose proper float value for line testing!
#endif

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */
/** Used to get right table index for dB -> HEX conversion. */
static const IFX_int32_t DXS_ALM_TXGAIN_OFFSET = 4;

/**
   Table with calculated ALM TX gain coefficients. It
   represents values between -4dB and 24dB, in steps of 1dB
   calculated with the formula below.

   TxGain = 20224 * 10^(-Lx[dB]/20)
*/
static const IFX_uint16_t DXS_AlmTxGain[] =
{
   0x7D35, 0x6F97, 0x6375, 0x58A4, 0x4F00,   /* -4dB  -3dB  -2dB  -1dB  0dB   */
   0x4669, 0x3EC0, 0x37ED, 0x31D8, 0x2C6D,   /*  1dB   2dB   3dB   4dB  5dB   */
   0x2798, 0x234A, 0x1F73, 0x1C08, 0x18FB,   /*  6dB   7dB   8dB   9dB 10dB   */
   0x1644, 0x13D8, 0x11B0, 0x0FC3, 0x0E0C,   /* 11dB  12dB  13dB  14dB 15dB   */
   0x0C85, 0x0B29, 0x09F2, 0x98DD, 0x07E6,   /* 16dB  17dB  18dB  19dB 20dB   */
   0x070A, 0x0646, 0x0598, 0x04FC            /* 21dB  22dB  23dB  24dB        */
};

/** Used to get right table index for dB -> HEX conversion. */
static const IFX_int32_t DXS_ALM_RXGAIN_OFFSET = 24;

/**
   Table with calculated ALM RX gain coefficients. It
   represents values between -24dB and 1dB, in steps of 1dB
   calculated with the formula below.

   TxGain = 30720 * 10^(Lr[dB]/20)
*/
static const IFX_uint16_t DXS_AlmRxGain[] =
{
   0x0792, 0x087F, 0x0988, 0x0AB2, 0x0C00,   /* -24dB -23dB -22dB -21dB -20dB */
   0x0D77, 0x0F1B, 0x10F3, 0x1305, 0x1557,   /* -19dB -18dB -17dB -16dB -15dB */
   0x17F1, 0x1ADD, 0x1E25, 0x21D2, 0x25F3,   /* -14dB -13dB -12dB -11dB -10dB */
   0x2A94, 0x2FC6, 0x359A, 0x3C24, 0x437B,   /*  -9dB  -8dB  -7dB  -6dB  -5dB */
   0x4BB7, 0x54F4, 0x5F52, 0x6AF3, 0x7800,   /*  -4dB  -3dB  -2dB  -1dB   0dB */
   0x86A4                                    /*   1dB                         */
};

/** Offset of the 0dB table index for dB -> fw value conversion. */
static const IFX_int32_t DXS_TG_LEVEL_0DB_IDX = 80;

/**
   Lookup table with tone generator level coefficients for dB values
   from -80dB to 0dB in steps of 1dB.
   fw_level = 32767 * (10^(level[dB] / 20))
*/
static const IFX_uint16_t dxs_tg_level[] =
{
   0x0003, 0x0004, 0x0004, 0x0005, 0x0005,   /* -80dB -79dB -78dB -77dB -76dB */
   0x0006, 0x0007, 0x0007, 0x0008, 0x0009,   /* -75dB -74dB -73dB -72dB -71dB */
   0x000A, 0x000C, 0x000D, 0x000F, 0x0010,   /* -70dB -69dB -68dB -67dB -66dB */
   0x0012, 0x0015, 0x0017, 0x001A, 0x001D,   /* -65dB -64dB -63dB -62dB -61dB */
   0x0021, 0x0025, 0x0029, 0x002E, 0x0034,   /* -60dB -59dB -58dB -57dB -56dB */
   0x003A, 0x0041, 0x0049, 0x0052, 0x005C,   /* -55dB -54dB -53dB -52dB -51dB */
   0x0068, 0x0074, 0x0082, 0x0092, 0x00A4,   /* -50dB -49dB -48dB -47dB -46dB */
   0x00B8, 0x00CF, 0x00E8, 0x0104, 0x0124,   /* -45dB -44dB -43dB -42dB -41dB */
   0x0148, 0x0170, 0x019D, 0x01CF, 0x0207,   /* -40dB -39dB -38dB -37dB -36dB */
   0x0247, 0x028E, 0x02DE, 0x0337, 0x039B,   /* -35dB -34dB -33dB -32dB -31dB */
   0x040C, 0x048B, 0x0518, 0x05B8, 0x066A,   /* -30dB -29dB -28dB -27dB -26dB */
   0x0733, 0x0813, 0x0910, 0x0A2B, 0x0B68,   /* -25dB -24dB -23dB -22dB -21dB */
   0x0CCD, 0x0E5D, 0x101D, 0x1214, 0x1449,   /* -20dB -19dB -18dB -17dB -16dB */
   0x16C3, 0x198A, 0x1CA8, 0x2027, 0x2413,   /* -15dB -14dB -13dB -12dB -11dB */
   0x287A, 0x2D6A, 0x32F5, 0x392C, 0x4026,   /* -10dB  -9dB  -8dB  -7dB  -6dB */
   0x47FA, 0x50C3, 0x5A9D, 0x65AC, 0x7214,   /*  -5dB  -4dB  -3dB  -2dB  -1dB */
   0x7FFF                                    /*   0dB */
};

/* Maximum value (upper limit) of the tone generator firmware level value. */
#define DXS_TG_LEVEL_MAX 0x7FFF

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

static IFX_int32_t DXS_ALM_MWL_Activation_Get(
                        DXS_CHANNEL_t *pCh,
                        IFX_TAPI_MWL_ACTIVATION_t *pActivation);

static IFX_int32_t DXS_ALM_MWL_Activation_Set(
                        DXS_CHANNEL_t *pCh,
                        IFX_TAPI_MWL_ACTIVATION_t const *pActivation);

static IFX_int32_t DXS_TAPI_LL_ALM_MWL_Activation_Get(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_MWL_ACTIVATION_t *pActivation);

static IFX_int32_t DXS_TAPI_LL_ALM_MWL_Activation_Set(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_MWL_ACTIVATION_t const *pActivation);

static IFX_int32_t DXS_TAPI_LL_ALM_Volume_Set(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_LINE_VOLUME_t const *pVol);

static IFX_int32_t DXS_TAPI_LL_ALM_Volume_High_Level(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_int32_t bEnable);

static IFX_int32_t DXS_TAPI_LL_ALM_Line_Mode_Set(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_int32_t nMode,
                        IFX_uint8_t nTapiLineMode);

static IFX_int32_t DXS_TAPI_LL_ALM_Line_Mode_Get(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_LINE_FEED_t *pFeed);

static IFX_int32_t DXS_TAPI_LL_ALM_Line_Type_Set(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_LINE_TYPE_t nType);

static IFX_int32_t DXS_TAPI_LL_ALM_Calibration_Start(
                        IFX_TAPI_LL_CH_t *pLLChannel);

static IFX_int32_t DXS_TAPI_LL_ALM_Calibration_Stop(
                        IFX_TAPI_LL_CH_t *pLLChannel);

static IFX_int32_t DXS_TAPI_LL_ALM_Calibration_Finish(
                        IFX_TAPI_LL_CH_t *pLLChannel);

static IFX_int32_t DXS_TAPI_LL_ALM_Calibration_Get(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_CALIBRATION_CFG_t *pClbConfig);

static IFX_int32_t DXS_TAPI_LL_ALM_Calibration_Set(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_CALIBRATION_CFG_t const *pClbConfig);

static IFX_int32_t DXS_TAPI_LL_ALM_Calibration_Results(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_CALIBRATION_CFG_t *pClbConfig);

static IFX_int32_t dxs_alm_Calibration_Start(
                        DXS_CHANNEL_t *pCh,
                        IFX_boolean_t bInternal);

static IFX_int32_t dxs_alm_Calibration_Stop(
                        DXS_CHANNEL_t *pCh);

static IFX_int32_t dxs_alm_Calibration_Get(
                        DXS_CHANNEL_t *pCh,
                        IFX_TAPI_CALIBRATION_CFG_t *pClbConfig);

static IFX_int32_t dxs_alm_Calibration_Set(
                        DXS_CHANNEL_t *pCh,
                        IFX_TAPI_CALIBRATION_CFG_t const *pClbConfig);

static IFX_int32_t dxs_alm_Line_Mode_Set_Unprot(
                        DXS_CHANNEL_t *pCh,
                        IFX_int32_t nMode);

static IFX_void_t  dxs_alm_HookWindow_OnTimer(
                        Timer_ID timer,
                        IFX_ulong_t arg);

static IFX_int32_t DXS_TAPI_LL_ALM_TestLoop(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_TEST_LOOP_t const *pLoop);

static IFX_int32_t DXS_TAPI_LL_ALM_TestHookGen(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_boolean_t bHook);

static IFX_int32_t DXS_TAPI_LL_ALM_RingParams_Get(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        struct IFX_TAPI_RING_PARAM *pRingParam);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */



/**
   Updates current line operating mode.

   This function is used directly from interrupt context to update
   the current line operating mode of an analog channel and wake up
   the thread waiting on sdd_event. The sdd_event informs the waiting
   thread of complete operating mode transition.

   \param  pCh             Pointer to the DXS channel structure.
   \param  lm              Current line mode reported by firmware.
*/
IFX_void_t irq_DXS_ALM_UpdateOpModeAndWakeUp (DXS_CHANNEL_t *pCh,
                                               IFX_uint8_t lm)
{
   if ((pCh == IFX_NULL) || (pCh->pALM == IFX_NULL))
   {
      /* Resource not valid. Channel number out of range. */
      return;
   }

   if (lm != OPMODE_IGNORED)
      pCh->pALM->curr_opmode = lm;

   pCh->pALM->bOpmodeChangePending = IFX_FALSE;
   TAPI_OS_EventWakeUp (&pCh->pALM->sdd_event);
}


/**
   Gets the activation status of the message waiting lamp.

   \param  pCh          Pointer to DXS channel structure.
   \param  pActivation  Pointer to \ref IFX_TAPI_MWL_ACTIVATION_t structure.

   \return
   - DXS_statusOk          if successful
   - DXS_statusNotSupported
   - DXS_statusReadErr
*/
static IFX_int32_t DXS_ALM_MWL_Activation_Get(
                        DXS_CHANNEL_t *pCh,
                        IFX_TAPI_MWL_ACTIVATION_t *pActivation)
{
   IFX_int32_t        ret     = DXS_statusOk;
   DXS_DEVICE_t      *pDev    = pCh->pParent;
   IFX_uint8_t        curr_opmode;

   if (!pDev->caps.bfw_MWI)
   {
      RETURN_STATUS (DXS_statusNotSupported, IFX_NULL);
   }
   /* Get the current opmode - wait while a opmode change is ongoing. */
   ret = DXS_ALM_OpmodeGet (pCh, &curr_opmode);
   /* Only exit when interrupted by a signal. When waiting was aborted by the
      timeout assume that no linemode change is pending and try to continue. */
   if (ret == DXS_statusSddEvtWaitInterrupt)
   {
      RETURN_STATUS (ret, IFX_NULL);
   }

   if (DXS_SDD_Opmode_MWI == curr_opmode)
   {
      pActivation->nActivation = IFX_ENABLE;
   }
   else
   {
      pActivation->nActivation = IFX_DISABLE;
   }

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Gets the activation status of the message waiting lamp.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  pActivation  Pointer to \ref IFX_TAPI_MWL_ACTIVATION_t structure.

   \return
   - DXS_statusOk          if successful
   - DXS_statusInvalCh
   - DXS_statusReadErr
*/
static IFX_int32_t DXS_TAPI_LL_ALM_MWL_Activation_Get(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_MWL_ACTIVATION_t *pActivation)
{
   IFX_int32_t        ret     = DXS_statusOk;
   DXS_CHANNEL_t     *pCh     = (DXS_CHANNEL_t *)pLLChannel;

   /* sanity check */
   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   if (pActivation == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("%s: pActivation is NULL\n", __FUNCTION__));
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   ret = DXS_ALM_MWL_Activation_Get(pCh, pActivation);

   /* release channel */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Activate/deactivates the message waiting lamp.

   \param  pCh          Pointer to DXS channel structure.
   \param  pActivation  Pointer to \ref IFX_TAPI_MWL_ACTIVATION_t structure.

   \return
   - DXS_statusOk             if successful
   - DXS_statusNotSupported
   - DXS_statusParam
   - DXS_statusMwlNotStandby
   - DXS_statusOpModeWrErr
*/
static IFX_int32_t DXS_ALM_MWL_Activation_Set(
                        DXS_CHANNEL_t *pCh,
                        IFX_TAPI_MWL_ACTIVATION_t const *pActivation)
{
   IFX_int32_t           ret       = DXS_statusOk;
   DXS_DEVICE_t          *pDev     = pCh->pParent;
   DXS_SDD_Opmode_t      *pOpmod   = &pCh->pALM->sdd_opmode;

   if (!pDev->caps.bfw_MWI)
   {
      RETURN_STATUS (DXS_statusNotSupported, IFX_NULL);
   }

   /* MWL is not possible until the DC/DC converter type is set. */
   if (pCh->pALM->nDcDcType == DXS_DCDC_TYPE_NOTSET)
   {
      /* errmsg: Operation blocked until BBD containing DC/DC type setting is
                 downloaded  */
      RETURN_STATUS (DXS_statusBlockedNoDcDcType, IFX_NULL);
   }

   /* MWI and combined DC/DC together is not possible. */
   if (pDev->bDcDcHwCombined == IFX_TRUE)
   {
      /* errmsg: MWL not possible in combined DC/DC mode */
      RETURN_STATUS (DXS_statusNoMwlAndCombinedDcDc, IFX_NULL);
   }

   if (IFX_ENABLE == pActivation->nActivation)
   {
      IFX_uint8_t             curr_opmode;

      /* Get the current opmode - wait while a opmode change is ongoing. */
      ret = DXS_ALM_OpmodeGet (pCh, &curr_opmode);
      /* Only exit when interrupted by a signal. When waiting was aborted by
         timeout assume that no linemode change is pending and try to continue.*/
      if (ret == DXS_statusSddEvtWaitInterrupt)
      {
         RETURN_STATUS (ret, IFX_NULL);
      }

      if (DXS_SDD_Opmode_Active != curr_opmode)
      {
         /* errmsg: MWL can be started only in linemode active */
         RETURN_STATUS (DXS_statusMwlLMNotActive, IFX_NULL);
      }
      pOpmod->OpMode   = DXS_SDD_Opmode_MWI;
   }
   else if (IFX_DISABLE == pActivation->nActivation)
   {
      if (pOpmod->OpMode != DXS_SDD_Opmode_MWI)
      {
         return DXS_statusOk;
      }
      pOpmod->OpMode = DXS_SDD_Opmode_Active;
   }
   else
   {
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }

   ret = DXS_ALM_OpmodeSet (pCh);
   if (!DXS_SUCCESS (ret))
   {
      /* errmsg: Writing the operation mode failed. */
      RETURN_STATUS (DXS_statusOpModeWrErr, IFX_NULL);
   }
   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Activate/deactivates the message waiting lamp.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  pActivation  Pointer to \ref IFX_TAPI_MWL_ACTIVATION_t structure.

   \return
   - DXS_statusOk             if successful
   - DXS_statusInvalCh
   - DXS_statusParam
   - DXS_statusMwlNotStandby
   - DXS_statusOpModeWrErr
*/
static IFX_int32_t DXS_TAPI_LL_ALM_MWL_Activation_Set(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_MWL_ACTIVATION_t const *pActivation)
{
   IFX_int32_t       ret      = DXS_statusOk;
   DXS_CHANNEL_t     *pCh     = (DXS_CHANNEL_t *)pLLChannel;

   /* sanity check */
   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   if (pActivation == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("%s: pActivation is NULL\n", __FUNCTION__));
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   ret = DXS_ALM_MWL_Activation_Set(pCh, pActivation);

   /* release channel */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Set the phone volume.

   The Gain Parameter are given in 'dB'. The range is -24dB ... 24dB.
   This function enables the ALM in firmware. There will be
   no error reported if it is not enabled before (IFX_TAPI_CH_INIT done).

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  pVol         Pointer to IFX_TAPI_LINE_VOLUME_t structure.

   \return
   - DXS_statusOk          if successful
   - DXS_statusErr         if channel pointer is null
   - DXS_statusParam       if at least one parameter is wrong
   - DXS_statusTxGainErr   Wrong parameters passed. This code is returned
      when any gain parameter is lower than -24 dB or higher 24 dB than
   - DXS_statusRxGainErr   Wrong parameters passed. This code is returned
      when any gain parameter is lower than -24 dB or higher 24 dB than
   - DXS_statusAlmVolErr   Writing the settings failed.
   - DXS_statusInvalCh
*/
static IFX_int32_t DXS_TAPI_LL_ALM_Volume_Set(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_LINE_VOLUME_t const *pVol)
{
   IFX_int32_t          err   = DXS_statusOk;
   DXS_CHANNEL_t        *pCh  = (DXS_CHANNEL_t *)pLLChannel;
   DXS_DEVICE_t         *pDev;
   DXS_SDD_TxRxGain_t   *pSddTxRxGain;

   if(pCh == IFX_NULL)
      return DXS_statusErr;

   /* sanity check */
   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   if ((pVol->nGainRx > DXS_ALM_RX_GAIN_MAX) ||
       (pVol->nGainRx < DXS_ALM_RX_GAIN_MIN))
   {
      /* parameter is out of supported range */
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("DXS_ERR: RxGain Level out of range (%d), "
            "(allowed range %d..%d dB)\n",
            pVol->nGainRx, DXS_ALM_RX_GAIN_MIN, DXS_ALM_RX_GAIN_MAX));

      /* errmsg: ALM RxGain parameter out of range.*/
      RETURN_STATUS(DXS_statusRxGainErr, IFX_NULL);
   }

   if ((pVol->nGainTx > DXS_ALM_TX_GAIN_MAX) ||
       (pVol->nGainTx < DXS_ALM_TX_GAIN_MIN))
   {
      /* parameter is out of supported range */
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("DXS_ERR: TxGain Level out of range (%d), "
            "(allowed range %d..%d dB)\n",
            pVol->nGainTx, DXS_ALM_TX_GAIN_MIN, DXS_ALM_TX_GAIN_MAX));
      /* errmsg: ALM TxGain parameter out of range.*/
      RETURN_STATUS(DXS_statusTxGainErr, IFX_NULL);
   }

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   pDev = pCh->pParent;
   pSddTxRxGain = &pCh->pALM->sdd_txrx_gain;

   /* get the values for RX and TX gain from table */
   pSddTxRxGain->RxGain = DXS_AlmRxGain[pVol->nGainRx + DXS_ALM_RXGAIN_OFFSET];
   pSddTxRxGain->TxGain = DXS_AlmTxGain[pVol->nGainTx + DXS_ALM_TXGAIN_OFFSET];
   err = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)pSddTxRxGain);
   if (DXS_SUCCESS(err))
   {
#ifdef DXS_FEAT_NLT
      pCh->pALM->tx_gain = pVol->nGainTx * 10;
      pCh->pALM->rx_gain = pVol->nGainRx * 10;
      pCh->pALM->bGainsConfigured = IFX_TRUE;
#endif
      /* Remember the analog gain to correct generator levels by this value. */
      pCh->pALM->sdd_rx_gain = pSddTxRxGain->RxGain;
   }

   /* release channel lock */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   if (!DXS_SUCCESS (err))
   {
      /* errmsg: Writing the ALM volume failed.*/
      RETURN_STATUS (DXS_statusAlmVolErr, IFX_NULL);
   }
   else
      return DXS_statusOk;
}


/**
   Convert a dB level into a value used by FW tone generators.

   The tone generators of the DXS play tones only in local (rx) direction.
   To achive the programmed level on the analog line the given level is
   automatically corrected by the SDD attenuation in local (rx) direction.

   Note that the converted level will be silently limited to the range
   of values in the lookup table. Levels above or below will be set to
   the respective min or max value.

   \param  pCh          Pointer to DXS channel structure.
   \param  level_db     Level in dB from 0dB to -XXdB.

   \return Value for use in tone generator firmware messages.
*/
IFX_uint16_t DXS_ALM_rx_level_convert(
                        DXS_CHANNEL_t *pCh,
                        IFX_int32_t level_db)
{
   IFX_uint32_t fw_level = DXS_TG_LEVEL_MAX;
   /* In the conversion table 0dB is at the highest index. */
   IFX_int32_t index = level_db + DXS_TG_LEVEL_0DB_IDX;

   /* Ensure the index is within the lookup table. */
   if (index < 0)
   {
      index = 0;
   }
   if (index > DXS_TG_LEVEL_0DB_IDX)
   {
      index = DXS_TG_LEVEL_0DB_IDX;
   }

   fw_level = dxs_tg_level[index];

#ifdef DXS_FEAT_TG_LEVEL_COMPENSATION
   /* Correct the levels with the attenuation of the analog line.
      Limit the corrected level to the generator maximum of 0dBFS. */

   fw_level = (fw_level * 30720) / pCh->pALM->sdd_rx_gain;
   if (fw_level > DXS_TG_LEVEL_MAX)
   {
      fw_level = DXS_TG_LEVEL_MAX;
   }
#else
   TAPI_UNUSED(pCh);
#endif /* DXS_FEAT_TG_LEVEL_COMPENSATION */

   return fw_level;
}


/**
   Enables or disables a high level path of a phone channel.

   It is intended for phone channels only and must be used in
   combination with IFX_TAPI_PHONE_VOLUME_SET or IFX_TAPI_PCM_VOLUME_SET
   to set the max. level (IFX_TAPI_LINE_VOLUME_HIGH) or to restore level.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  bEnable      The parameter represent a boolean value of
                        \ref IFX_TAPI_LINE_LEVEL_t.
                        - 0: IFX_TAPI_LINE_LEVEL_DISABLE, disable the
                             high level path.
                        - 1: IFX_TAPI_LINE_LEVEL_ENABLE, enable the
                             high level path.
   \return
   - DXS_statusCmdWr Writing the command failed
   - DXS_statusOk if successful
*/
static IFX_int32_t DXS_TAPI_LL_ALM_Volume_High_Level(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_int32_t bEnable)
{
   DXS_CHANNEL_t    *pCh    = (DXS_CHANNEL_t *)pLLChannel;
   IFX_int32_t       ret;
   IFX_uint8_t       curr_opmode;

   /* sanity check */
   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   /* protect channel data from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   /* Get the current opmode - wait while a opmode change is ongoing. */
   ret = DXS_ALM_OpmodeGet (pCh, &curr_opmode);
   /* Only exit when interrupted by a signal. When waiting was aborted by the
      timeout assume that no linemode change is pending and try to continue
      with the opmode change that was requested. */
   if (ret == DXS_statusSddEvtWaitInterrupt)
   {
      /* release channel lock */
      TAPI_OS_MutexRelease (&pCh->mtxChAcc);
      RETURN_STATUS (ret, IFX_NULL);
   }

   if (bEnable)
   {
      /* Howler tone can only be set when linefeeding is active or active
       * reversed. */
      if (curr_opmode != DXS_SDD_Opmode_Active &&
         curr_opmode != DXS_SDD_Opmode_ActiveRevpol)
      {
         /* release channel lock */
         TAPI_OS_MutexRelease (&pCh->mtxChAcc);
         /* errmsg: Setting of high level output requires active feeding */
         RETURN_STATUS (DXS_statusHighLevelNotActive, IFX_NULL);
      }
      /* howler-tone on */
      if (curr_opmode == DXS_SDD_Opmode_Active)
         ret = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Howler);
      else if (curr_opmode == DXS_SDD_Opmode_ActiveRevpol)
         ret = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_HowlerRevpol);
   }
   else
   {
      /* howler-tone off */
      if (curr_opmode == DXS_SDD_Opmode_Howler)
         ret = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Active);
      else if (curr_opmode == DXS_SDD_Opmode_HowlerRevpol)
         ret = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_ActiveRevpol);
   }

   /* release channel lock */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Prepare parameters and call the target configuration function to switch the
   line mode.

   Checks on valid transitions are done. Nevertheless they should be avoided by
   the user application.

   \param  pCh          Pointer to DXS channel structure.
   \param  nMode        Linefeed mode.

   \return
   - DXS_statusOk             if successful
   - DXS_statusInvalLMSwitch  Trying to do an invalid line mode change.
   - DXS_statusOpModeWrErr    Writing the command has failed
*/
static IFX_int32_t dxs_alm_Line_Mode_Set_Unprot(
                        DXS_CHANNEL_t *pCh,
                        IFX_int32_t nMode)
{
   DXS_SDD_Opmode_t *pOpmod;
   IFX_int32_t       ret;
   IFX_uint8_t       curr_opmode;

   pOpmod = &pCh->pALM->sdd_opmode;

   /* Get the current opmode - wait while a opmode change is ongoing. */
   ret = DXS_ALM_OpmodeGet (pCh, &curr_opmode);
   /* Only exit when interrupted by a signal. When waiting was aborted by the
      timeout assume that no linemode change is pending and try to continue
      with the opmode change that was requested. */
   if (ret == DXS_statusSddEvtWaitInterrupt)
   {
      RETURN_STATUS (ret, IFX_NULL);
   }

   /* check if transition is valid, prepare command */
   switch (nMode)
   {
      case IFX_TAPI_LINE_FEED_ACTIVE:
      /* for backward compatibility only */
      case IFX_TAPI_LINE_FEED_NORMAL_AUTO:
      case IFX_TAPI_LINE_FEED_ACTIVE_LOW:
      case IFX_TAPI_LINE_FEED_ACTIVE_BOOSTED:
         switch (curr_opmode)
         {
            case DXS_SDD_Opmode_Disabled:
            case DXS_SDD_Opmode_Standby:
            case DXS_SDD_Opmode_Active:
            case DXS_SDD_Opmode_ActiveRevpol:
            case DXS_SDD_Opmode_RingBurst:
            case DXS_SDD_Opmode_MWI:
            case DXS_SDD_Opmode_Howler:
            case DXS_SDD_Opmode_GroundStart:
            case DXS_SDD_Opmode_GroundStart_T2G:
               pOpmod->OpMode = DXS_SDD_Opmode_Active;
               break;
            default:
               /* errmsg: Line mode switch is invalid.
                          Not every transition is valid. */
               RETURN_STATUS (DXS_statusInvalLMSwitch, IFX_NULL);
         }
         break;
      case IFX_TAPI_LINE_FEED_RING_PAUSE:
         switch (curr_opmode)
         {
            case DXS_SDD_Opmode_RingRevpol:
               /* leave reversal bit as is */
               pOpmod->OpMode = DXS_SDD_Opmode_ActiveRevpol;
               break;
            case DXS_SDD_Opmode_RingBurst:
            case DXS_SDD_Opmode_Disabled:
            case DXS_SDD_Opmode_GroundStart:
            case DXS_SDD_Opmode_Standby:
            case DXS_SDD_Opmode_Active:
            case DXS_SDD_Opmode_ActiveRevpol:
            case DXS_SDD_Opmode_MWI:
            case DXS_SDD_Opmode_Howler:
               pOpmod->OpMode = DXS_SDD_Opmode_Active;
               break;
            default:
               /* errmsg: Line mode switch is invalid.
                          Not every transition is valid. */
               RETURN_STATUS (DXS_statusInvalLMSwitch, IFX_NULL);
         }
         break;
      case IFX_TAPI_LINE_FEED_ACTIVE_REV:
      /* for backward compatibility only */
      case IFX_TAPI_LINE_FEED_REVERSED_AUTO:
         switch (curr_opmode)
         {
            case DXS_SDD_Opmode_Disabled:
            case DXS_SDD_Opmode_Standby:
            case DXS_SDD_Opmode_Active:
            case DXS_SDD_Opmode_ActiveRevpol:
            case DXS_SDD_Opmode_RingRevpol:
            case DXS_SDD_Opmode_HowlerRevpol:
            case DXS_SDD_Opmode_GroundStart:
            case DXS_SDD_Opmode_GroundStart_T2G:
               pOpmod->OpMode = DXS_SDD_Opmode_ActiveRevpol;
               break;
            default:
               /* errmsg: Line mode switch is invalid.
                          Not every transition is valid. */
               RETURN_STATUS (DXS_statusInvalLMSwitch, IFX_NULL);
         }
         break;

      case IFX_TAPI_LINE_FEED_PARKED_REVERSED:
      case IFX_TAPI_LINE_FEED_STANDBY:
         switch (curr_opmode)
         {
            case DXS_SDD_Opmode_Active:
            case DXS_SDD_Opmode_ActiveRevpol:
            case DXS_SDD_Opmode_RingBurst:
            case DXS_SDD_Opmode_RingRevpol:
            case DXS_SDD_Opmode_Standby:
            case DXS_SDD_Opmode_Disabled:
               pOpmod->OpMode = DXS_SDD_Opmode_Standby;
               break;
            default:
               /* errmsg: Line mode switch is invalid.
                          Not every transition is valid. */
               RETURN_STATUS (DXS_statusInvalLMSwitch, IFX_NULL);
         }
         break;

      case IFX_TAPI_LINE_FEED_DISABLED:
         pOpmod->OpMode = DXS_SDD_Opmode_Disabled;
         break;

      case IFX_TAPI_LINE_FEED_RING_BURST:
         switch (curr_opmode)
         {
            case DXS_SDD_Opmode_RingBurst:
            /*lint -fallthrough */
            case DXS_SDD_Opmode_Active:
            /*lint -fallthrough */
            case DXS_SDD_Opmode_Standby:
            /*lint -fallthrough */
            case DXS_SDD_Opmode_GroundStart:
               /* leave reversal bit as is */
               pOpmod->OpMode = DXS_SDD_Opmode_RingBurst;
               break;

            case DXS_SDD_Opmode_RingRevpol:
            /*lint -fallthrough */
            case DXS_SDD_Opmode_ActiveRevpol:
               /* leave reversal bit as is */
               pOpmod->OpMode = DXS_SDD_Opmode_RingRevpol;
               break;

            default:
               /* errmsg: Line mode switch is invalid.
                          Not every transition is valid. */
               RETURN_STATUS (DXS_statusInvalLMSwitch, IFX_NULL);
         }
         break;

      /* Ground start mode 1: ring line is feeded, tip line is high-impedance.*/
      case IFX_TAPI_LINE_FEED_GROUND_START_TIP_OPEN:
         switch (curr_opmode)
         {
            case DXS_SDD_Opmode_Disabled:       /*lint -fallthrough */
            case DXS_SDD_Opmode_Active:         /*lint -fallthrough */
            case DXS_SDD_Opmode_ActiveRevpol:   /*lint -fallthrough */
            case DXS_SDD_Opmode_RingBurst:      /*lint -fallthrough */
            case DXS_SDD_Opmode_RingRevpol:     /*lint -fallthrough */
            case DXS_SDD_Opmode_GroundStart:    /*lint -fallthrough */
            case DXS_SDD_Opmode_GroundStart_T2G:
               pOpmod->OpMode = DXS_SDD_Opmode_GroundStart;
               break;
            default:
               /* errmsg: Line mode switch is invalid.
                          Not every transition is valid. */
               RETURN_STATUS (DXS_statusInvalLMSwitch, IFX_NULL);
         }
         break;

      /* Ground start mode 2: ring line is feeded, tip line is grounded. */
      case IFX_TAPI_LINE_FEED_GROUND_START_TIP2GND:
         switch (curr_opmode)
         {
            case DXS_SDD_Opmode_GroundStart:    /*lint -fallthrough */
            case DXS_SDD_Opmode_GroundStart_T2G:
               pOpmod->OpMode = DXS_SDD_Opmode_GroundStart_T2G;
               break;
            default:
               /* errmsg: Line mode switch is invalid.
                          Not every transition is valid. */
               RETURN_STATUS (DXS_statusInvalLMSwitch, IFX_NULL);
         }
         break;

      /* unsupported linemodes */
      case IFX_TAPI_LINE_FEED_HIGH_IMPEDANCE:
      case IFX_TAPI_LINE_FEED_METER:
      case IFX_TAPI_LINE_FEED_ACT_TEST:
      case IFX_TAPI_LINE_FEED_ACT_TESTIN:
      case IFX_TAPI_LINE_FEED_DISABLED_RESISTIVE_SWITCH:
      default:
         RETURN_STATUS (DXS_statusNotSupported, IFX_NULL);
   }

   ret = DXS_ALM_OpmodeSet (pCh);

   return ret;
}


/**
   Prepare parameters and call the target configuration function to switch the
   line mode.

   A check is done, whether the channel supports an analog line. If not
   DXS_statusInvalCh is returned.
   Checks on valid transitions are done. Nevertheless they should be avoided by
   the user application.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  nMode        Linefeed mode.
   \param  nTapiLineMode The currently set line mode. (unused)

   \return
   - DXS_statusOk       If successful
   - DXS_statusInvalCh  The parameters are wrong
*/
static IFX_int32_t DXS_TAPI_LL_ALM_Line_Mode_Set(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_int32_t nMode,
                        IFX_uint8_t nTapiLineMode)
{
   DXS_CHANNEL_t    *pCh   = (DXS_CHANNEL_t *)pLLChannel;
   IFX_int32_t       ret;

   TAPI_UNUSED (nTapiLineMode);

   /* sanity check */
   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   /* protect channel from mutual access */
   /* WARNING: this causes a deadlock if PPD state machine is activated.
      Temporarily removed. For analysis. */
   /*TAPI_OS_MutexGet (&pCh->mtxChAcc);*/

   /* Until the DC/DC converter type is set do not allow any linefeed set. */
   if (pCh->pALM->nDcDcType == DXS_DCDC_TYPE_NOTSET)
   {
      /* errmsg: Operation blocked until BBD containing DC/DC type setting is
                 downloaded  */
      RETURN_STATUS (DXS_statusBlockedNoDcDcType, IFX_NULL);
   }

   ret = dxs_alm_Line_Mode_Set_Unprot (pCh, nMode);

   /* release channel lock */
   /*TAPI_OS_MutexRelease (&pCh->mtxChAcc);*/

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Return the current operating mode of the addressed channel.

   Reports IFX_TAPI_LINE_FEED_DISABLED if called during PDH, Calibration or
   GR909.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  pFeed        Pointer to TAPI IFX_TAPI_LINE_FEED_t variable.

   \return
   - DXS_statusFuncParam if at least one parameter in function is wrong
   - DXS_statusErr   if channel pointer is null
   - DXS_statusOk    if successful
   - DXS_statusInvalCh
*/
static IFX_int32_t DXS_TAPI_LL_ALM_Line_Mode_Get(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_LINE_FEED_t *pFeed)
{
   DXS_CHANNEL_t    *pCh   = (DXS_CHANNEL_t *)pLLChannel;
   IFX_TAPI_LINE_MODE_t nMode;
   IFX_uint8_t curr_opmode;
   IFX_int32_t ret;

   /* sanity check */
   if(pCh == IFX_NULL)
      return DXS_statusErr;

   if(pFeed == IFX_NULL)
      RETURN_STATUS(DXS_statusFuncParam, IFX_NULL);

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   /* Get the current opmode - wait while a opmode change is ongoing. */
   ret = DXS_ALM_OpmodeGet (pCh, &curr_opmode);
   /* Only exit when interrupted by a signal. When waiting was aborted by the
      timeout assume that no linemode change is pending and try to continue. */
   if (ret == DXS_statusSddEvtWaitInterrupt)
   {
      RETURN_STATUS (ret, IFX_NULL);
   }

   switch (curr_opmode)
   {
      case DXS_SDD_Opmode_Active:
      case DXS_SDD_Opmode_Howler:
         nMode = IFX_TAPI_LINE_FEED_ACTIVE;
         break;
      case DXS_SDD_Opmode_ActiveRevpol:
      case DXS_SDD_Opmode_HowlerRevpol:
         nMode = IFX_TAPI_LINE_FEED_ACTIVE_REV;
         break;

      case DXS_SDD_Opmode_Standby:
         nMode = IFX_TAPI_LINE_FEED_STANDBY;
         break;

      case DXS_SDD_Opmode_RingBurst:
      case DXS_SDD_Opmode_RingRevpol:
         nMode = IFX_TAPI_LINE_FEED_RING_BURST;
         break;

      /* report disabled status in case of PDH, FXO, Calibration and GR909 */
      default:
         nMode = IFX_TAPI_LINE_FEED_DISABLED;
         break;
   }

#ifdef TAPI_ONE_DEVNODE
   pFeed->lineMode = nMode;
#else  /* TAPI_ONE_DEVNODE */
   *pFeed = nMode;
#endif /* TAPI_ONE_DEVNODE */

   return DXS_statusOk;
}


/**
   Inform TAPI-hl for changed operation mode

   \param pCh        Handle to the DXS channel structure
   \param nOpmode    Current analog line operating mode

   \return
      - DXS_statusOk if successful
*/
IFX_int32_t DXS_ALM_OnLineModeChanged (DXS_CHANNEL_t *pCh, IFX_uint32_t nOpmode)
{
   const DXS_DEVICE_t *const pDev = (DXS_DEVICE_t*) (pCh->pParent);

   IFX_TAPI_EVENT_t tapiEvent = {0};
   tapiEvent.id = IFX_TAPI_EVENT_NONE;
   tapiEvent.dev = pDev->nDevNr;
   tapiEvent.ch = pCh->nChannel - 1;
   DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_ALM);

   /* in VMMC driver there is no channel protection in this place */
   /* TAPI_OS_MutexGet (&pCh->mtxChAcc); */

   /* To detect end of calibration this code looks for a transition
      from opmode calibration to any other opmode. */
   if (nOpmode == DXS_SDD_Opmode_Calibrate)
   {
      /* Set flag that now calibration is running. The check below
         will use this flag to find the end of calibration. */
      pCh->pALM->bCalibrationRunning = IFX_TRUE;
   }
   if ((nOpmode != DXS_SDD_Opmode_Calibrate) &&
       (pCh->pALM->bCalibrationRunning == IFX_TRUE))
   {
      pCh->pALM->bCalibrationRunning = IFX_FALSE;
      tapiEvent.id = IFX_TAPI_EVENT_CALIBRATION_END_INT;
   }
   /* in VMMC driver there is no channel protection in this place */
   /* TAPI_OS_MutexRelease (&pCh->mtxChAcc); */
   if (IFX_TAPI_EVENT_NONE != tapiEvent.id)
      IFX_TAPI_Event_DeferredDispatch (pCh->pTapiCh, &tapiEvent);

   return DXS_statusOk;
}


/**
   Set line type to FXS or FXO.

   Currently there is no DUSLIC XS device with any FXO port. So this function
   is not really required. It is mainly for compatiblitly with other LL-drivers.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  nType        New line type, can be IFX_TAPI_LINE_TYPE_FXS or
                        IFX_TAPI_LINE_TYPE_FXO.

   \return
   - DXS_statusOk       If successful.
   - DXS_statusInvalCh  No analog line on this channel.
*/
static IFX_int32_t DXS_TAPI_LL_ALM_Line_Type_Set(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_LINE_TYPE_t nType)
{
   IFX_int32_t       ret   = DXS_statusOk;
   DXS_CHANNEL_t     *pCh  = (DXS_CHANNEL_t *)pLLChannel;

   /* sanity check */
   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   /* DXS only supports FXS ports. There is no DXS with FXO port. */
   switch (nType)
   {
      case IFX_TAPI_LINE_TYPE_FXS_NB:
      case IFX_TAPI_LINE_TYPE_FXS_WB:
      case IFX_TAPI_LINE_TYPE_FXS_AUTO:
         break;

      default:
         /* errmsg: Invalid line type for analog line */
         RETURN_STATUS (DXS_statusInvalLineType, IFX_NULL);
   }

   return ret;
}


/**
   Initalize the firmware message for the analog line / Smart Device Driver.

   The SDD_Opmode message is used to write and read all parameters that are
   needed for setting the desired operating mode of the Analog Line / Smart
   Device Driver

   \param  pCh          Pointer to DXS channel structure.
*/
IFX_void_t DXS_ALM_InitCh(DXS_CHANNEL_t *pCh)
{
   DXS_SDD_Opmode_t      *pOpmode       = IFX_NULL;
   DXS_SDD_TxRxGain_t    *pSddTxRxGain  = IFX_NULL;
   DXS_SDD_GR909Config_t *p_ctrl        = IFX_NULL;
   DXS_SDD_BasicConfig_t *pBasicCfg     = IFX_NULL;
   DXS_SDD_Calibrate_t   *pCalibrate    = IFX_NULL;
#ifdef DXS_FEAT_CAPACITANCE_MEASUREMENT
   DXS_SDD_CapMeasRead_t *pCapMeas  = IFX_NULL;
#endif /* DXS_FEAT_CAPACITANCE_MEASUREMENT */
#ifdef DXS_FEAT_CONT_MEASUREMENT
   DXS_SDD_ContMeasRead_t    *pContMeasRead  = IFX_NULL;
#endif /* DXS_FEAT_CONT_MEASUREMENT */
   IFX_uint8_t           ch             = pCh->nChannel - 1;
#ifdef DXS_FEAT_NLT
    DXS_SDD_ACLevelMeterControl_t *pAclmControl  = IFX_NULL;
    DXS_SDD_ACLevelMeterConfig_t *pAclmConfig  = IFX_NULL;
    DXS_SDD_ACLevelMeterResult_t *pAclmResult  = IFX_NULL;
#endif
   /* create timer to supervise elapsed time since last hook event */
   pCh->pALM->nHookWindowTimerId =
      TAPI_Create_Timer((TIMER_ENTRY)dxs_alm_HookWindow_OnTimer,
                        (IFX_ulong_t)pCh);

   /* begin with disabled line feeding */
   pCh->pALM->curr_opmode = DXS_SDD_Opmode_Disabled;
   pCh->pALM->bOpmodeChangePending = IFX_FALSE;

   /* initialize SDD event */
   TAPI_OS_EventInit (&pCh->pALM->sdd_event);
#ifdef DXS_FEAT_NLT
   /* initialize ACLM event */
   TAPI_OS_EventInit (&pCh->pALM->aclm_event);
#endif
   /* setup opmode control cmd */
   pOpmode = &pCh->pALM->sdd_opmode;
   memset (pOpmode, 0, sizeof (*pOpmode));
   pOpmode->CMD         = DXS_CMD_CMD_SDD;
   pOpmode->CHAN        = ch;
   pOpmode->MOD         = DXS_CMD_MOD_SDD;
   pOpmode->ECMD        = DXS_SDD_Opmode_ECMD;
   pOpmode->LENGTH      = DXS_SDD_Opmode_LENGTH;
   pOpmode->OpMode      = DXS_SDD_Opmode_Disabled;

   /* setup gain cmd */
   pSddTxRxGain = &pCh->pALM->sdd_txrx_gain;
   memset (pSddTxRxGain, 0, sizeof (*pSddTxRxGain));
   pSddTxRxGain->CMD    = DXS_CMD_CMD_SDD;
   pSddTxRxGain->CHAN   = ch;
   pSddTxRxGain->MOD    = DXS_CMD_MOD_SDD;
   pSddTxRxGain->ECMD   = DXS_SDD_TxRxGain_ECMD;
   pSddTxRxGain->LENGTH = DXS_SDD_TxRxGain_LENGTH;

#ifdef DXS_FEAT_GR909
   /* setup the default configuration of the measurement path for
      line testing. */
   pCh->pALM->nRmeas = IFX_TAPI_NLT_RMEAS_1_5MOHM;
   /* Values are given in bytes representation of 4-bytes float value */

   pCh->pALM->nlt_ResistanceConfig.fOlResTip2Ring = DXS_LT_FLOAT_MIN_5000000;
   pCh->pALM->nlt_ResistanceConfig.fOlResTip2Gnd = DXS_LT_FLOAT_MIN_1500000;
   pCh->pALM->nlt_ResistanceConfig.fOlResRing2Gnd = DXS_LT_FLOAT_MIN_1500000;
#endif /* DXS_FEAT_GR909 */

   /* setup gr909 line testing control cmd */
   p_ctrl = &pCh->pALM->sdd_gr909_config;
   memset(p_ctrl, 0, sizeof(*p_ctrl));
   p_ctrl->CMD          = DXS_CMD_CMD_SDD;
   p_ctrl->CHAN         = ch;
   p_ctrl->MOD          = DXS_CMD_MOD_SDD;
   p_ctrl->ECMD         = DXS_SDD_GR909Config_ECMD;
   p_ctrl->LENGTH       = DXS_SDD_GR909Config_LENGTH;

   /* set default limits. Used values for Rmeas = 1,5MOhm */
   DXS_ALM_GR909_SetLimits(pCh, pCh->pALM->nRmeas);

#ifdef DXS_FEAT_CAPACITANCE_MEASUREMENT
   /* Values are given in bytes representation of 4-bytes float value */
   pCh->pALM->nlt_CapacitanceConfig.fOlCapTip2Ring = DXS_LT_FLOAT_7p5;
   pCh->pALM->nlt_CapacitanceConfig.fOlCapTip2Gnd = DXS_LT_FLOAT_15;
   pCh->pALM->nlt_CapacitanceConfig.fOlCapRing2Gnd = DXS_LT_FLOAT_15;

   /* setup SDD_CapacitanceMeas command */
   pCapMeas = &pCh->pALM->fw_sdd_capacitance_meas;
   memset (pCapMeas, 0, sizeof(*pCapMeas));
   pCapMeas->CMD        = DXS_CMD_CMD_SDD;
   pCapMeas->CHAN       = ch;
   pCapMeas->MOD        = DXS_CMD_MOD_SDD;
   pCapMeas->ECMD       = DXS_SDD_CapMeasRead_ECMD;
   pCapMeas->LENGTH     = DXS_SDD_CapMeasRead_LENGTH;
#endif /* DXS_FEAT_CAPACITANCE_MEASUREMENT */

   /* setup basic configuration command */
   pBasicCfg = &pCh->pALM->fw_sdd_basic_config;
   memset (pBasicCfg, 0, sizeof (*pBasicCfg));
   pBasicCfg->CMD       = DXS_CMD_CMD_SDD;
   pBasicCfg->CHAN      = ch;
   pBasicCfg->MOD       = DXS_CMD_MOD_SDD;
   pBasicCfg->ECMD      = DXS_SDD_BasicConfig_ECMD;
   pBasicCfg->LENGTH    = DXS_SDD_BasicConfig_LENGTH;

   /* Reading calibration data */
   pCalibrate = &pCh->pALM->fw_sdd_calibrate;
   memset (pCalibrate, 0, sizeof (*pCalibrate));
   pCalibrate->CMD      = DXS_CMD_CMD_SDD;
   pCalibrate->CHAN     = ch;
   pCalibrate->MOD      = DXS_CMD_MOD_SDD;
   pCalibrate->ECMD     = DXS_SDD_Calibrate_ECMD;
   pCalibrate->LENGTH   = DXS_SDD_Calibrate_LENGTH;

   /* Initialise the semaphore to wait for calibration to finish. */
#ifdef LINUX
   TAPI_OS_EventInit(&pCh->pALM->evtCalibrationWait);
#elif defined(VXWORKS)
   pCh->pALM->mtxCalibrationWait = semBCreate (SEM_Q_PRIORITY, SEM_EMPTY);
#endif

   /* no calibration was done yet */
   pCh->pALM->nCalibrationState = IFX_TAPI_CALIBRATION_STATE_NO;
   /* No calibration is needed - flag will be set during BBD download. */
   pCh->pALM->bCalibrationNeeded = IFX_FALSE;

   /* Clear the cache for the last calibration results */
   memset(&pCh->pALM->calibrationLastResults, 0,
          sizeof(pCh->pALM->calibrationLastResults));

   /* at start no DC/DC type is defined */
   pCh->pALM->nDcDcType = DXS_DCDC_TYPE_NOTSET;

   /* FW default ring frequency is 25 Hz corresponding to a 40 ms period. */
   pCh->pALM->nRingPeriod = 40;

#ifdef DXS_FEAT_CONT_MEASUREMENT
   pContMeasRead  = &pCh->pALM->fw_sdd_contMeasRead;
   memset (pContMeasRead, 0, sizeof (*pContMeasRead));
   pContMeasRead->CMD   = DXS_CMD_CMD_SDD;
   pContMeasRead->CHAN  = ch;
   pContMeasRead->MOD   = DXS_CMD_MOD_SDD;
   pContMeasRead->ECMD  = DXS_SDD_ContMeasRead_ECMD;
   pContMeasRead->LENGTH = DXS_SDD_ContMeasRead_LENGTH;
#endif /* DXS_FEAT_CONT_MEASUREMENT */

#ifdef DXS_FEAT_NLT
    pAclmControl = &pCh->pALM->fw_sdd_aclm_control;
    memset (pAclmControl, 0, sizeof(*pAclmControl));
    pAclmControl->CMD       = DXS_CMD_CMD_SDD;
    pAclmControl->CHAN      = ch;
    pAclmControl->MOD       = DXS_CMD_MOD_SDD;
    pAclmControl->ECMD      = DXS_SDD_ACLevelMeterControl_ECMD;
    pAclmControl->LENGTH    = DXS_SDD_ACLevelMeterControl_LENGTH;

    pAclmConfig = &pCh->pALM->fw_sdd_aclm_config;
    memset (pAclmConfig, 0, sizeof(*pAclmConfig));
    pAclmConfig->CMD        = DXS_CMD_CMD_SDD;
    pAclmConfig->CHAN       = ch;
    pAclmConfig->MOD        = DXS_CMD_MOD_SDD;
    pAclmConfig->ECMD       = DXS_SDD_ACLevelMeterConfig_ECMD;
    pAclmConfig->LENGTH     = DXS_SDD_ACLevelMeterConfig_LENGTH;

    pAclmResult = &pCh->pALM->fw_sdd_aclm_result;
    memset (pAclmResult, 0, sizeof(*pAclmResult));
    pAclmResult->CMD        = DXS_CMD_CMD_SDD;
    pAclmResult->CHAN       = ch;
    pAclmResult->MOD        = DXS_CMD_MOD_SDD;
    pAclmResult->ECMD       = DXS_SDD_ACLevelMeterResult_ECMD;
    pAclmResult->LENGTH     = DXS_SDD_ACLevelMeterResult_LENGTH;
#endif

   pCh->pALM->sdd_rx_gain = 0x25F3; /* FW default: -10 dBr */
}


/**
   Stop ALM on this channel.

   \param  pCh          Pointer to DXS channel structure.
   \param  bChipAccess  If IFX_FALSE no writing to the chip must be done.

   \return
   - DXS_statusOk      If successful
   - DXS_statusCmdWr   Writing the command has failed
   - DXS_statusInvalCh
*/
IFX_int32_t DXS_ALM_ChStop(
                        DXS_CHANNEL_t *pCh,
                        IFX_boolean_t bChipAccess)
{
   IFX_int32_t          ret   = DXS_statusOk;

   /* calling function should ensure valid parameters */
   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM != IFX_NULL)
   {
      TAPI_OS_MutexGet (&pCh->mtxChAcc);

      if (pCh->pALM->nHookWindowTimerId != 0)
      {
         TAPI_Delete_Timer (pCh->pALM->nHookWindowTimerId);
      }

      if (bChipAccess != IFX_FALSE)
      {
         /* set the line to disabled (this implicitly writes the fw message) */
         ret = DXS_TAPI_LL_ALM_Line_Mode_Set (pCh,
                                              IFX_TAPI_LINE_FEED_DISABLED, 0);
      }

      /* forget about the DC/DC type */
      pCh->pALM->nDcDcType = DXS_DCDC_TYPE_NOTSET;

      TAPI_OS_MutexRelease (&pCh->mtxChAcc);
      /* Delete the semaphore to wait for calibration to finish. */
#ifdef LINUX
      /* DXS_OS_EventDelete(&pCh->pALM->evtCalibrationWait); */
#elif defined(VXWORKS)
      semFlush (pCh->pALM->mtxCalibrationWait);
      semDelete (pCh->pALM->mtxCalibrationWait);
#endif
   }

   RETURN_STATUS(ret, IFX_NULL);
}


/**
   Allocate data structure of the ALM module in the given channel.

   \param  pCh          Pointer to DXS channel structure.

   \return
   - DXS_statusOk
   - DXS_statusNoMem    in case the stucture could not be created
*/
IFX_int32_t DXS_ALM_Allocate_Ch_Structures(
                        DXS_CHANNEL_t *pCh)
{
   DXS_ALM_Free_Ch_Structures (pCh);

   pCh->pALM = TAPI_OS_Malloc(sizeof(*pCh->pALM));
   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: No memory could be allocated. */
      RETURN_STATUS(DXS_statusNoMem, IFX_NULL);
   }
   memset(pCh->pALM, 0, sizeof(*pCh->pALM));

   return DXS_statusOk;
}


/**
   Free data structure of the ALM module in the given channel.

   \param  pCh          Pointer to DXS channel structure.
*/
IFX_void_t DXS_ALM_Free_Ch_Structures(
                        DXS_CHANNEL_t *pCh)
{
   if (pCh->pALM != IFX_NULL)
   {
      TAPI_OS_Free(pCh->pALM);
      pCh->pALM = IFX_NULL;
   }
}


/**
   This service controls an 8 kHz loop switching in the device for testing.

   If switched on, signals that are played to the subscriber are looped back
   to the receiving side.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  pLoop        Pointer to the test loop details:
                        - if pLoop->bAnalog == 0x1 - loop is switched on
                        - if pLoop->bAnalog == 0x0 - loop is switched off

   \return
   - DXS_statusCmdWrErr Writing the command has failed
   - DXS_statusInvalCh The resource is not valid
   - DXS_statusFuncParam if at least one parameter in function is wrong
   - DXS_statusErr if channel pointer is null
   - DXS_statusOk if successful
*/
IFX_int32_t DXS_TAPI_LL_ALM_TestLoop(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_TEST_LOOP_t const *pLoop)
{
   DXS_CHANNEL_t *pCh = (DXS_CHANNEL_t *)pLLChannel;
   DXS_DEVICE_t *pDev = IFX_NULL;
   IFX_uint8_t ch = 0;

   if (pCh == IFX_NULL)
      return DXS_statusErr;

   if (pLoop == IFX_NULL)
      RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);

   pDev = pCh->pParent;
   ch = pCh->nChannel - 1;

   if (pCh->pALM != IFX_NULL)
   {
      IFX_int32_t ret;
      DXS_SDD_En8kLoop_t ctrl8kLoopCfg = {0};
      ctrl8kLoopCfg.CMD = DXS_SDD_En8kLoop_CMD;
      ctrl8kLoopCfg.MOD = DXS_SDD_En8kLoop_MOD;
      ctrl8kLoopCfg.ECMD = DXS_SDD_En8kLoop_ECMD;
      ctrl8kLoopCfg.CHAN = ch;
      ctrl8kLoopCfg.LENGTH = DXS_SDD_En8kLoop_LEN;

      /* depending on the request enable or disable the test loop */
      if (pLoop->bAnalog)
      {
         /* close digital loop at 8kHz */
         ctrl8kLoopCfg.En8kLoop = DXS_SDD_En8kLoop_En8kLoop_ENABLE;
      }
      else
      {
         /* normal operation, digital loop at 8 kHz open */
         ctrl8kLoopCfg.En8kLoop = DXS_SDD_En8kLoop_En8kLoop_DISABLE;
      }

      /* write message contents */
      ret = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)&ctrl8kLoopCfg);

      if (!DXS_SUCCESS (ret))
      {
         RETURN_STATUS (DXS_statusCmdWrErr, IFX_NULL);
      }

      return DXS_statusOk;
   }
   else
   {
      /* errmsg: Resource not valid. Channel number out of range */
      RETURN_STATUS (DXS_statusInvalCh, IFX_NULL);
   }
}


/**
   This service generates an on or off hook event for the low level driver.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  bHook        Selects the hook mode:
                        - IFX_FALSE - OFFHOOK
                        - IFX_TRUE - ONHOOK

   \return
   - DXS_statusCmdWrErr Writing the command has failed
   - DXS_statusInvalCh The resource is not valid
   - DXS_statusFuncParam Input parameter is wrong
   - DXS_statusOk if successful
*/
IFX_int32_t DXS_TAPI_LL_ALM_TestHookGen(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_boolean_t bHook)
{
   DXS_CHANNEL_t *pCh = (DXS_CHANNEL_t *)pLLChannel;
   DXS_DEVICE_t *pDev;
   IFX_TAPI_EVENT_t nEvent = {0};

   if (pCh == IFX_NULL)
   {
      return DXS_statusFuncParam;
   }

   pDev = pCh->pParent;

   /* call event dispatcher */
   nEvent.dev = pDev->nDevNr;
   nEvent.ch = pCh->nChannel - 1;
   if (bHook)
      nEvent.id = IFX_TAPI_EVENT_FXS_OFFHOOK;
   else
      nEvent.id = IFX_TAPI_EVENT_FXS_ONHOOK;
   nEvent.module = IFX_TAPI_MODULE_TYPE_ALM;
   IFX_TAPI_Event_ImmediateDispatch(pCh->pTapiCh, &nEvent);

   return DXS_statusOk;
}


/* ========================================================================== */
/*                         Calibration stuff                                  */
/* ========================================================================== */
/**
   Start calibration process for analog channel.

   Run calibration mechanism and check new calibration offsets

   \param  pCh          Pointer to the DXS channel structure.
   \param  bInternal    Flag indicating call from external TAPI API (IFX_FALSE)
                        or driver internal (IFX_TRUE).


   \return
   - DXS_statusOk                   if successful
   - DXS_statusNoResource           no resource on given channel
   - DXS_statusCalLineNotDisabled   Invalid current line mode
   - DXS_statusOpModeWrErr          Writing the command has failed
*/
static IFX_int32_t dxs_alm_Calibration_Start(
                        DXS_CHANNEL_t *pCh,
                        IFX_boolean_t bInternal)
{
   IFX_int32_t       ret = DXS_statusOk;
   IFX_uint8_t       curr_opmode;

   /* Until the DC/DC converter type is set no linefeed change is allowed. */
   if (pCh->pALM->nDcDcType == DXS_DCDC_TYPE_NOTSET)
   {
      /* errmsg: Operation blocked until BBD containing DC/DC type setting is
                 downloaded  */
      RETURN_STATUS (DXS_statusBlockedNoDcDcType, IFX_NULL);
   }

   /* Get the current opmode - wait while a opmode change is ongoing. */
   ret = DXS_ALM_OpmodeGet (pCh, &curr_opmode);
   /* Only exit when interrupted by a signal. When waiting was aborted by the
      timeout assume that no linemode change is pending and try to continue
      with the opmode change that was requested. */
   if (ret == DXS_statusSddEvtWaitInterrupt)
   {
      RETURN_STATUS (ret, IFX_NULL);
   }

   /* if calibration is already active - do nothing and return immediately. */
   if (curr_opmode == DXS_SDD_Opmode_Calibrate)
   {
      return DXS_statusOk;
   }

   /* Check if transition is valid */
   if (curr_opmode != DXS_SDD_Opmode_Disabled)
   {
      /* errmsg: Current line mode is not DISABLED */
      RETURN_STATUS (DXS_statusCalLineNotDisabled, IFX_NULL);
   }

   /* Remember if started from API or driver internal. */
   pCh->pALM->bCalibrationInternal = bInternal;

   /* Changing the operating mode does the actual start. */
   ret = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Calibrate);

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Stop calibration process for analog channel.

   \param  pCh          Pointer to the DXS channel structure.

   \return
   - DXS_statusOk                    if successful
   - DXS_statusOpModeWrErr           Writing the command has failed
*/
static IFX_int32_t dxs_alm_Calibration_Stop(
                        DXS_CHANNEL_t *pCh)
{
   IFX_int32_t ret = DXS_statusOk;
   IFX_uint8_t       curr_opmode;

   /* Get the current opmode - wait while a opmode change is ongoing. */
   ret = DXS_ALM_OpmodeGet (pCh, &curr_opmode);
   /* Only exit when interrupted by a signal. When waiting was aborted by the
      timeout assume that no linemode change is pending and try to continue. */
   if (ret == DXS_statusSddEvtWaitInterrupt)
   {
      RETURN_STATUS (ret, IFX_NULL);
   }
   /* check if transition is valid */
   if (curr_opmode != DXS_SDD_Opmode_Calibrate)
   {
      /* We are not in calibration state. So stopping is not needed.
         This is what was intended by the user and so not an error. */
      return DXS_statusOk;
   }

   /* Stop calibration by changing operating mode to PDH */
   ret = DXS_ALM_OpmodeModeSet (pCh, DXS_SDD_Opmode_Disabled);

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Retrieve calibration data.

   \param  pCh          Pointer to the DXS channel structure.
   \param  pClbConfig   Result as current calibration data.

   \return
   - DXS_statusOk                   if successful
   - DXS_statusNoResource           no resource on given channel
   - DXS_statusCalInProgress        Calibration in progress
   - DXS_statusOpModeWrErr          Writing the command has failed
*/
static IFX_int32_t dxs_alm_Calibration_Get(
                        DXS_CHANNEL_t *pCh,
                        IFX_TAPI_CALIBRATION_CFG_t *pClbConfig)
{
   DXS_DEVICE_t        *pDev    = pCh->pParent;
   DXS_SDD_Calibrate_t *pCalibrate = &pCh->pALM->fw_sdd_calibrate;
   IFX_int32_t          ret = DXS_statusOk;
   IFX_uint8_t          curr_opmode;

   /* Sanity check */
   if (pClbConfig == IFX_NULL)
   {
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }

   /* Set return structure to defined values. */
   memset(pClbConfig, 0x00, sizeof(*pClbConfig));
   pClbConfig->dev = pDev->nDevNr;
   pClbConfig->ch = pCh->nChannel - 1;

   /* Get the current opmode - wait while a opmode change is ongoing. */
   ret = DXS_ALM_OpmodeGet (pCh, &curr_opmode);
   /* Only exit when interrupted by a signal. When waiting was aborted by the
      timeout assume that no linemode change is pending and try to continue
      with the opmode change that was requested. */
   if (ret == DXS_statusSddEvtWaitInterrupt)
   {
      RETURN_STATUS (ret, IFX_NULL);
   }

   if (curr_opmode == DXS_SDD_Opmode_Calibrate)
   {
      /* All returned values are invalid */
      pClbConfig->nState = IFX_TAPI_CALIBRATION_STATE_NO;
      /* errmsg: Current line mode is CALIBRATE */
      RETURN_STATUS (DXS_statusCalInProgress, IFX_NULL);
   }

   /* Read calibration values from FW */
   ret = DXS_CmdRead (pDev, (IFX_uint32_t *)(IFX_void_t *)pCalibrate,
                            (IFX_uint32_t *)(IFX_void_t *)pCalibrate);

   if (DXS_SUCCESS (ret))
   {
      pClbConfig->nState = pCh->pALM->nCalibrationState;
      pClbConfig->nITransOffset = (IFX_int16_t)(
             ((IFX_int32_t)((IFX_int16_t)pCalibrate->TxOffset) * 5000) / 32768);
      pClbConfig->nIdacGain     = (IFX_int16_t)(
    (((IFX_int32_t)((IFX_int16_t)pCalibrate->IdacGain) * 1000) / 29257) - 1000);
      pClbConfig->nIRingOffset = (IFX_int16_t)(
         ((IFX_int32_t)((IFX_int16_t)pCalibrate->RingOffset) * 5000) / 32768);
   }
   else
   {
      /* All returned values are invalid */
      pClbConfig->nState = IFX_TAPI_CALIBRATION_STATE_NO;
   }

   RETURN_STATUS(ret, IFX_NULL);
}


/**
   Set calibration data.

   \param  pCh          Pointer to the DXS channel structure.
   \param  pClbConfig   New calibration data.

   \return
   - DXS_statusOk                   if successful
   - DXS_statusCalInProgress        Calibration in progress
   - DXS_statusOpModeWrErr          Writing the command has failed
*/
static IFX_int32_t dxs_alm_Calibration_Set(
                        DXS_CHANNEL_t *pCh,
                        IFX_TAPI_CALIBRATION_CFG_t const *pClbConfig)
{
   DXS_DEVICE_t        *pDev    = pCh->pParent;
   DXS_SDD_Calibrate_t *pCalibrate = &pCh->pALM->fw_sdd_calibrate;
   IFX_int32_t          ret;
   IFX_int16_t          TxOffset, IdacGain, RingOffset;
   IFX_uint8_t          curr_opmode;

   /* Get the current opmode - wait while a opmode change is ongoing. */
   ret = DXS_ALM_OpmodeGet (pCh, &curr_opmode);
   /* Only exit when interrupted by a signal. When waiting was aborted by the
      timeout assume that no linemode change is pending and try to continue
      with the opmode change that was requested. */
   if (ret == DXS_statusSddEvtWaitInterrupt)
   {
      RETURN_STATUS (ret, IFX_NULL);
   }

   if (curr_opmode == DXS_SDD_Opmode_Calibrate)
   {
      /* errmsg: Current line mode is CALIBRATE */
      RETURN_STATUS (DXS_statusCalInProgress, IFX_NULL);
   }

   /* calculate SDD values from TAPI */
   TxOffset     = (IFX_int16_t)
                  ((IFX_int32_t)pClbConfig->nITransOffset * 32768 / 5000);
   IdacGain     = (IFX_int16_t)
                  (((IFX_int32_t)pClbConfig->nIdacGain + 1000) * 29257 / 1000);
   RingOffset     = (IFX_int16_t)
                  ((IFX_int32_t)pClbConfig->nIRingOffset * 32768 / 5000);

   TRACE(TAPI_DXS, DBG_LEVEL_LOW,
         ("Calibration set values. ch%d:\n"
         "  TX path offset %i\n"
         "  IDAC gain correction %i\n"
         "  Ring current offset %i\n",
         pCh->nChannel - 1,
         TxOffset,
         IdacGain,
         RingOffset));

   /* sanity check */
   if ( (TxOffset > DXS_SDD_CALIBRATE_TXOFFSET_MAX) ||
        (TxOffset < DXS_SDD_CALIBRATE_TXOFFSET_MIN) ||
        (IdacGain > DXS_SDD_CALIBRATE_IDACGAIN_MAX) ||
        (IdacGain < DXS_SDD_CALIBRATE_IDACGAIN_MIN) ||
        (RingOffset > DXS_SDD_CALIBRATE_RINGOFFSET_MAX) ||
        (RingOffset < DXS_SDD_CALIBRATE_RINGOFFSET_MIN) )
   {
      /* errmsg: Calibration set values are out of range  */
      RETURN_STATUS (DXS_statusCalSetValOutOfRange, IFX_NULL);
   }

   /* prepare command */
   pCalibrate->TxOffset       = TxOffset;
   pCalibrate->IdacGain       = IdacGain;
   pCalibrate->RingOffset     = RingOffset;
   ret = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)pCalibrate);

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Start calibration on the given analog line.

   Wrapper to protect the actual start function that runs calibration on
   the analog line. Calibration takes about a second and ends automatically.
   The end is signalled with the event IFX_TAPI_EVENT_CALIBRATION_END.

   \param  pLLChannel   Pointer to DXS channel structure.

   \return
   - DXS_statusOk                    if successful
   - DXS_statusInvalCh               Channel number out of range
*/
static IFX_int32_t DXS_TAPI_LL_ALM_Calibration_Start(
                        IFX_TAPI_LL_CH_t *pLLChannel)
{
   DXS_CHANNEL_t     *pCh  = (DXS_CHANNEL_t *)pLLChannel;
   IFX_int32_t       ret = DXS_statusOk;

   /* sanity check */
   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   ret = dxs_alm_Calibration_Start (pCh, IFX_FALSE);

   /* release protection */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   return ret;
}


/**
   Stop a started calibration immediately.

   \param  pLLChannel   Pointer to DXS channel structure.

   \return
   - DXS_statusOk                   if successful
   - DXS_statusInvalCh              Channel number out of range
*/
static IFX_int32_t DXS_TAPI_LL_ALM_Calibration_Stop(
                        IFX_TAPI_LL_CH_t *pLLChannel)
{
   DXS_CHANNEL_t     *pCh  = (DXS_CHANNEL_t *)pLLChannel;
   IFX_int32_t       ret = DXS_statusOk;

   /* sanity check */
   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   ret = dxs_alm_Calibration_Stop (pCh);

   /* release protection */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   return ret;
}


/**
   Finish calibration process on the analog line.

   This function is called after the firmware indicated that calibration has
   finished. It reads the calibration values and does a plausibility check.
   If values are out of range a reset to defaults is done. In the end the
   line is made available again and a TAPI event is sent to the application
   to inform about the result of the calibration process.

   \param  pLLChannel   Pointer to DXS channel structure.

   \return
   - DXS_statusOk                   if successful
   - DXS_statusInvalCh              Channel number out of range
*/
static IFX_int32_t DXS_TAPI_LL_ALM_Calibration_Finish(
                        IFX_TAPI_LL_CH_t *pLLChannel)
{
   DXS_CHANNEL_t     *pCh  = (DXS_CHANNEL_t *)pLLChannel;
   DXS_DEVICE_t      *pDev;
   DXS_SDD_Calibrate_t *pCalibrate, *pLastResults;

   IFX_TAPI_EVENT_t tapiEvent;
   IFX_int32_t   ret = DXS_statusOk;

   /* sanity check */
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

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   pCalibrate = &pCh->pALM->fw_sdd_calibrate;
   pLastResults = &pCh->pALM->calibrationLastResults;

   /* protect channel from mutual access */
   if (!pCh->pALM->bCalibrationInternal)
      TAPI_OS_MutexGet (&pCh->mtxChAcc);

   /* Read the calibrated values for verification below. */
   ret = DXS_CmdRead (pDev, (IFX_uint32_t *)(IFX_void_t *)pCalibrate,
                            (IFX_uint32_t *)(IFX_void_t *)pCalibrate);

   if (DXS_SUCCESS(ret))
   {
      /* Remember the just read results for later reading. */
      *pLastResults = *pCalibrate;

      TRACE(TAPI_DXS, DBG_LEVEL_LOW,
            ("Calibration results on dev %d channel %d:\n"
            "  TX path offset %i\n"
            "  IDAC gain correction %i\n"
            "  Ring current offset %i\n",
            pDev->nDevNr,
            pCh->nChannel - 1,
            (IFX_int16_t)pCalibrate->TxOffset,
            (IFX_int16_t)pCalibrate->IdacGain,
            (IFX_int16_t)pCalibrate->RingOffset));
   }

   /* If reading the values failed or range check results that values are
      out of range reset to zero offset values to prevent that implausible
      values are used. */
   if (!DXS_SUCCESS(ret) ||
       ((IFX_int16_t)pCalibrate->TxOffset > DXS_SDD_CALIBRATE_TXOFFSET_MAX) ||
       ((IFX_int16_t)pCalibrate->TxOffset < DXS_SDD_CALIBRATE_TXOFFSET_MIN) ||
       ((IFX_int16_t)pCalibrate->IdacGain > DXS_SDD_CALIBRATE_IDACGAIN_MAX) ||
       ((IFX_int16_t)pCalibrate->IdacGain < DXS_SDD_CALIBRATE_IDACGAIN_MIN) ||
       ((IFX_int16_t)pCalibrate->RingOffset > DXS_SDD_CALIBRATE_RINGOFFSET_MAX) ||
       ((IFX_int16_t)pCalibrate->RingOffset < DXS_SDD_CALIBRATE_RINGOFFSET_MIN)
      )
   {
      /* At least one of the values is out of range -> reset to defaults. */
      pCalibrate->TxOffset       = 0;
      pCalibrate->IdacGain       = 0;
      pCalibrate->RingOffset     = 0;
      ret = DXS_CmdWrite(pDev,(IFX_uint32_t *)(IFX_void_t *)pCalibrate);
      /* Set flag that calibration failed */
      pCh->pALM->nCalibrationState = IFX_TAPI_CALIBRATION_STATE_FAILED;
   }
   else
   {
      pCh->pALM->nCalibrationState = IFX_TAPI_CALIBRATION_STATE_DONE;
   }

   pCh->pALM->bCalibrationNeeded = IFX_FALSE;

   if (pCh->pALM->bCalibrationInternal)
   {
#ifdef LINUX
      TAPI_OS_EventWakeUp(&pCh->pALM->evtCalibrationWait);
#elif defined(VXWORKS)
      semGive (pCh->pALM->mtxCalibrationWait);
#endif
   }
   else
   {
      /* release protection */
      TAPI_OS_MutexRelease (&pCh->mtxChAcc);

      /* Send the calibration-end event for the application */
      memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
      tapiEvent.ch = pCh->nChannel - 1;
      tapiEvent.id = IFX_TAPI_EVENT_CALIBRATION_END;
      DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_ALM);

      if (pCh->pALM->nCalibrationState == IFX_TAPI_CALIBRATION_STATE_DONE)
      {
         tapiEvent.data.calibration = IFX_TAPI_EVENT_CALIBRATION_SUCCESS;
      }
      else
      {
         tapiEvent.data.calibration = IFX_TAPI_EVENT_CALIBRATION_ERROR_RANGE;
      }
      IFX_TAPI_Event_Dispatch(pCh->pTapiCh, &tapiEvent);
   }

   RETURN_STATUS(ret, IFX_NULL);
}


/**
   Used internal to start calibration on the given analog line.

   This function is called after a BBD download was done to calibrate the
   analog channel where the download has just been done. This function blocks
   until the finished event has been received and the validation of the
   calibration was done. This function passes a parameter to prevent the
   sending of the TAPI event IFX_TAPI_EVENT_CALIBRATION_END and at the
   same time initiate the wakeup from the blocking wait.

   \param  pCh          Pointer to the DXS channel structure.

   \return
   - DXS_statusOk                   if successful
   - DXS_statusInvalCh              Channel number out of range
*/
IFX_int32_t DXS_ALM_Calibration (DXS_CHANNEL_t *pCh)
{
   IFX_int32_t ret = DXS_statusErr;

   /* sanity check */
   if (pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   tapi_debug_buffer_add_user_entry("dxs #%d:%d analog calibration",
                                    pCh->pParent->nDevNr, pCh->nChannel);

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   ret = dxs_alm_Calibration_Start (pCh, IFX_TRUE);

   /* release protection */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   if (DXS_SUCCESS (ret))
   {
      /* Block until the semaphore is given in the calibration finished
         function or a signal interrupts. */
#ifdef LINUX
      IFX_int32_t retCode = 0;

      ret = TAPI_OS_EventWait(&pCh->pALM->evtCalibrationWait,
                             DXS_SDD_CALIBRATE_TMOUT_MS, &retCode);
      if (ret != DXS_statusOk && retCode == 1)
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("DXS_ERR: TIMEOUT waiting for CALIBRATION finish"
                "dev(%d) ch(%d)\n", pCh->pParent->nDevNr, pCh->nChannel-1));
      }

#elif defined(VXWORKS)
      semTake (pCh->pALM->mtxCalibrationWait, WAIT_FOREVER);
      /* FIXME: handle timeout error code */
#endif

      if (ret != DXS_statusOk)
      {
         /* errmsg: Automatic calibration timeout while waiting for event. */
         ret = DXS_statusAutomaticCalibrationTimeout;
      }
   }

   RETURN_STATUS(ret, IFX_NULL);
}


/**
   Retrieve calibration data from device.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  pClbConfig   Result as current calibration data.

   \return
   - DXS_statusOk             if successful
   - DXS_statusInvalCh        Channel number out of range
*/
static IFX_int32_t DXS_TAPI_LL_ALM_Calibration_Get(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_CALIBRATION_CFG_t *pClbConfig)
{
   DXS_CHANNEL_t     *pCh  = (DXS_CHANNEL_t *)pLLChannel;
   IFX_int32_t       ret = DXS_statusOk;

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

   if (pClbConfig == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("%s: pClbConfig is NULL\n", __FUNCTION__));
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   ret = dxs_alm_Calibration_Get (pCh, pClbConfig);

   /* release protection */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   return ret;
}


/**
   Set the calibration data.

   \param  pLLChannel   Pointer to DXS channel structure.
    \param  pClbConfig  New calibration data.

    \return
    - DXS_statusOk             if successful
    - DXS_statusErr            if channel pointer is null
    - DXS_statusInvalCh        Channel number out of range
    - DXS_statusFuncParam      if at least one parameter in function is wrong
 */
static IFX_int32_t DXS_TAPI_LL_ALM_Calibration_Set(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_CALIBRATION_CFG_t const *pClbConfig)
{
   DXS_CHANNEL_t     *pCh  = (DXS_CHANNEL_t *)pLLChannel;
   IFX_int32_t       ret = DXS_statusOk;

   if(pCh == IFX_NULL)
      return DXS_statusErr;

   if(pClbConfig == IFX_NULL)
      RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);

   /* sanity check */
   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   ret = dxs_alm_Calibration_Set (pCh, pClbConfig);

   /* release protection */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   return ret;
}


/**
   Retrieve the results of the last calibration process.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  pClbConfig   Result as current calibration data.

   \return
   - DXS_statusOk             if successful
   - DXS_statusInvalCh        Channel number out of range
*/
static IFX_int32_t DXS_TAPI_LL_ALM_Calibration_Results(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_CALIBRATION_CFG_t *pClbConfig)
{
   DXS_CHANNEL_t       *pCh  = (DXS_CHANNEL_t *)pLLChannel;
   DXS_SDD_Calibrate_t *pLastResults;

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

   pLastResults = &pCh->pALM->calibrationLastResults;

   if (pClbConfig == IFX_NULL)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("%s: pClbConfig is NULL\n", __FUNCTION__));
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   /* Set return structure to defined values. */
   memset(pClbConfig, 0x00, sizeof(*pClbConfig));
   pClbConfig->dev = pCh->pParent->nDevNr;
   pClbConfig->ch = pCh->nChannel - 1;
   pClbConfig->nState = pCh->pALM->nCalibrationState;

   pClbConfig->nITransOffset = (IFX_int16_t)(
          ((IFX_int32_t)((IFX_int16_t)pLastResults->TxOffset) * 5000) / 32768);
   pClbConfig->nIdacGain     = (IFX_int16_t)(
 (((IFX_int32_t)((IFX_int16_t)pLastResults->IdacGain) * 1000) / 29257) - 1000);
   pClbConfig->nIRingOffset = (IFX_int16_t)(
      ((IFX_int32_t)((IFX_int16_t)pLastResults->RingOffset) * 5000) / 32768);

   /* release protection */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   return DXS_statusOk;
}


/* ========================================================================== */
/*                           Metering stuff                                   */
/* ========================================================================== */
#ifdef DXS_FEAT_METERING
/**
   Sends one metering burst.

   \param  pCh          Pointer to DXS channel structure.
   \param  nPulseNum    Number of pulses in burst

   \return
   - DXS_statusParm     Wrong parameters passed. This code is returned
                        when the nPulseNum parameter has an invalid or
                        unsupported value.
   - DXS_statusCmdWr    Writing the command has failed
   - DXS_statusOk       if successful
   - DXS_statusInvalCh  Channel not valid or channel number out of range

   \remarks
   The DXS FW allows only one pulse to be sent with each call. Therefore the
   nPulseNum parameter must always be set to 1 only.
*/
static IFX_int32_t dxs_alm_MeteringBurst(
                        DXS_CHANNEL_t *pCh,
                        IFX_uint32_t nPulseNum)
{
   DXS_DEVICE_t         *pDev;
   DXS_TTX_GEN_CTRL_t   ttx_gen;
   IFX_int32_t          ret;
   IFX_uint8_t          curr_opmode;

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

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   /* Until the DC/DC converter type is set no linefeed change is allowed. */
   if (pCh->pALM->nDcDcType == DXS_DCDC_TYPE_NOTSET)
   {
      /* errmsg: Operation blocked until BBD containing DC/DC type setting is
                 downloaded  */
      RETURN_STATUS (DXS_statusBlockedNoDcDcType, IFX_NULL);
   }

   /* only single pulse supported */
   if (nPulseNum != 1)
   {
      RETURN_STATUS (DXS_statusParam, IFX_NULL);
   }
   /* Get the current opmode - wait while a opmode change is ongoing. */
   ret = DXS_ALM_OpmodeGet (pCh, &curr_opmode);
   /* Only exit when interrupted by a signal. When waiting was aborted by the
      timeout assume that no linemode change is pending and try to continue
      with the opmode change that was requested. */
   if (ret == DXS_statusSddEvtWaitInterrupt)
   {
      RETURN_STATUS (ret, IFX_NULL);
   }
   if (curr_opmode != DXS_SDD_Opmode_Active &&
      curr_opmode != DXS_SDD_Opmode_ActiveRevpol)
   {
      RETURN_STATUS(DXS_statusMeteringLMNotActive, IFX_NULL);
   }

   memset(&ttx_gen, 0, sizeof(ttx_gen));
   ttx_gen.CMD         = DXS_CMD_CMD_EOP;
   ttx_gen.CHAN        = pCh->nChannel - 1;
   ttx_gen.MOD         = DXS_CMD_MOD_SIG_GEN;
   ttx_gen.ECMD        = TTX_GEN_CTRL_ECMD_EOP_TTX;
   ttx_gen.LENGTH      = TTX_GEN_CTRL_LENGTH;
   ttx_gen.EN          = TTX_GEN_CTRL_EN_EN;
   ret = DXS_CmdWrite(pDev, (IFX_uint32_t *)(IFX_void_t *)&ttx_gen);

   RETURN_STATUS (ret, IFX_NULL);
}


/**
   Sends one metering burst

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  nPulseNum    Number of pulses in burst.

   \return
   - DXS_statusInvalCh  Channel not valid or channel number out of range
   - DXS_statusParm     Wrong parameters passed. This code is returned
                        when the nPulseNum parameter has an invalid or
                        unsupported value.
   - DXS_statusCmdWr    Writing the command has failed
   - DXS_statusOk       if successful
*/
static IFX_int32_t DXS_TAPI_LL_ALM_MeteringStart(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_uint32_t nPulseNum)
{
   DXS_CHANNEL_t *pCh   = (DXS_CHANNEL_t *)pLLChannel;
   IFX_int32_t ret = DXS_statusOk;

   /* sanity check */
   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   ret = dxs_alm_MeteringBurst (pCh, nPulseNum);

   /* release channel */
   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   RETURN_STATUS (ret, IFX_NULL);
}
#endif /* DXS_FEAT_METERING */


/**
   Write the SDD_Opmode command.

   Defers when called while another opmode change is still not complete until
   this is done. A possible timeout error from the deferring is shaddowed by
   errors from the CmdWrite.
   The opmode "disabled" is always written even if already in status "disabled".

   \param  pCh             Pointer to the DXS channel structure.

   \return
   - DXS_statusCmdWr               writing the command has failed
   - DXS_statusSddEvtWaitTmout     timeout waiting for an event from SDD
   - DXS_statusSddEvtWaitInterrupt waiting for event from SDD is interrupted
                                    by a signal
   - DXS_statusOk                  if successful
   - DXS_statusInvalCh             Channel not valid or channel number out of range
*/
IFX_int32_t DXS_ALM_OpmodeSet (DXS_CHANNEL_t *pCh)
{
   DXS_SDD_Opmode_t *pOpmod;
   IFX_int32_t ret;
   IFX_uint8_t curr_opmode;

   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   pOpmod = &pCh->pALM->sdd_opmode;

   /* Get the current opmode - wait while a opmode change is ongoing. */
   ret = DXS_ALM_OpmodeGet (pCh, &curr_opmode);
   /* Only exit when interrupted by a signal. When waiting was aborted by the
      timeout assume that no linemode change is pending and try to continue
      with the opmode change that was requested. */
   if (ret == DXS_statusSddEvtWaitInterrupt)
   {
      return ret;
   }

   /* do nothing if parameters didn't change */
   if (pOpmod->OpMode != DXS_SDD_Opmode_Disabled &&
       pOpmod->OpMode == curr_opmode)
   {
      return DXS_statusOk;
   }

   /* Only when the opmode changes wait for the opmode change event. */
   if (pOpmod->OpMode != curr_opmode)
   {
      pCh->pALM->bOpmodeChangePending = IFX_TRUE;
   }
   else
   {
      pCh->pALM->bOpmodeChangePending = IFX_FALSE;
   }

   ret = DXS_CmdWrite(pCh->pParent, (IFX_uint32_t *)(IFX_void_t *)pOpmod);
   return ret;
}


/**
   Wrapper for the above function that sets the given opmode.

   \param  pCh             Pointer to the DXS channel structure.
   \param  nOperatingMode  Operating mode.

   \return
   Returnvalue from \ref DXS_ALM_OpmodeSet().
*/
IFX_int32_t DXS_ALM_OpmodeModeSet (DXS_CHANNEL_t *pCh,
                        IFX_uint32_t nOperatingMode)
{
   DXS_SDD_Opmode_t *pOpmod;

   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }


   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   pOpmod = &pCh->pALM->sdd_opmode;

   pOpmod->OpMode = nOperatingMode;
   return DXS_ALM_OpmodeSet (pCh);
}


/**
   Get the current opmode but defer while a opmode change is in progress.

   \param  pCh             Pointer to the DXS channel structure.
   \param  pCurrentOpmode  Pointer to variable returning the current opmode.

   \return
   - DXS_statusOk                  if successful
   - DXS_statusSddEvtWaitTmout     timeout waiting for an event from SDD
   - DXS_statusSddEvtWaitInterrupt waiting for event from SDD is interrupted
                                    by a signal
   - DXS_statusInvalCh             Channel not valid or channel number out of range
*/
IFX_int32_t DXS_ALM_OpmodeGet (DXS_CHANNEL_t *pCh,
                                IFX_uint8_t *pCurrentOpmode)
{
   IFX_int32_t timeout;

   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* errmsg: The requested resource is not available. */
      RETURN_STATUS (DXS_statusNoResource, IFX_NULL);
   }

   /* Wait while a linemode change is pending to avoid returning an outdated
      linemode. When the waiting was aborted by a signal or a timeout occured
      while waiting inform about this in the return code. */
   timeout = DXS_WaitForSddOpmodeChEvt(pCh);

   /* Return the current opmode regardless of the result of the waiting. */
   *pCurrentOpmode = pCh->pALM->curr_opmode;

   if (timeout > 0)
   {
      /* Positive values indicate the wait ended because the expected event
         occured. */
      return DXS_statusOk;
   }
   else if (timeout == 0)
   {
      /* A return value of zero indicates that the timeout expired. */
      /* errmsg: Timeout waiting for an event from SDD. */
      return DXS_statusSddEvtWaitTmout;
   }

   /* All negative values indicate that the waiting was interrupted by a
      signal from the OS. */
   /* errmsg: Waiting for an event from SDD interrupted by a signal. */
   return DXS_statusSddEvtWaitInterrupt;
}


/* ========================================================================== */
/*                     Analog Line Continuous Measurement                     */
/* ========================================================================== */
#ifdef DXS_FEAT_CONT_MEASUREMENT
/**
   Request continuous measurement results.

   This function does nothing on firmware interface. It spawns an
   event IFX_TAPI_EVENT_CONTMEASUREMENT in a task context. Added for
   compatibility with TAPI_V4 interface.

   \param  pLLChannel   Pointer to DXS channel structure.

   \return
   - DXS_statusInvalCh
   - DXS_statusOk
*/
static IFX_int32_t DXS_TAPI_LL_ALM_ContMeas_Req(
                        IFX_TAPI_LL_CH_t *pLLChannel)
{
   DXS_CHANNEL_t *pCh   = (DXS_CHANNEL_t *)pLLChannel;
   TAPI_CHANNEL    *pChannel;
   IFX_TAPI_EVENT_t tapiEvent;

   /* sanity check */
   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* Resource not valid. Channel number out of range or
                 line type is FXO */
      RETURN_STATUS (DXS_statusInvalCh, IFX_NULL);
   }

   pChannel = pCh->pTapiCh;

   memset (&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
   tapiEvent.id = IFX_TAPI_EVENT_CONTMEASUREMENT;
   DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_ALM);

   IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);

   RETURN_STATUS (DXS_statusOk, IFX_NULL);
}


/**
   Return the stored continuous measure results.

   \param  pLLChannel   Pointer to DXS channel structure.

   \return
   - DXS_statusInvalCh        wrong resource or FXO channel
   - DXS_statusCmdIbNoSpace   DXS_CmdRead error
   - DXS_statusCmdMbWrErr     DXS_CmdRead error
   - DXS_statusCmdObTimeout   DXS_CmdRead error
   - DXS_statusCmdObRdErr     DXS_CmdRead error
   - DXS_statusCmdObDataOvld  DXS_CmdRead error
   - DXS_statusOk if successful
*/
static IFX_int32_t DXS_TAPI_LL_ALM_ContMeas_Get(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        IFX_TAPI_CONTMEASUREMENT_GET_t *pContMeas)
{
   DXS_CHANNEL_t *pCh   = (DXS_CHANNEL_t *)pLLChannel;
   DXS_DEVICE_t  *pDev;
   DXS_SDD_ContMeasRead_t *pContMeasRead;
   IFX_int32_t   ret;

   /* sanity check */
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

   if (pCh->pALM == IFX_NULL)
   {
      /* Resource not valid. Channel number out of range or
                 line type is FXO */
      RETURN_STATUS (DXS_statusInvalCh, IFX_NULL);
   }

   pContMeasRead = &pCh->pALM->fw_sdd_contMeasRead;

   /* protect channel from mutual access */
   TAPI_OS_MutexGet (&pCh->mtxChAcc);

   ret = DXS_CmdRead (pDev, (IFX_uint32_t *)(IFX_void_t *)pContMeasRead,
                            (IFX_uint32_t *)(IFX_void_t *)pContMeasRead);

   if (DXS_SUCCESS (ret))
   {
      memset(pContMeas, 0, sizeof(*pContMeas));
      pContMeas->nVLineDesired =
               (IFX_int16_t)(((IFX_int32_t)pContMeasRead->Vline * 14400) >> 15);

      pContMeas->nILine =
              (IFX_int16_t)(((IFX_int32_t)pContMeasRead->Itrans * 10000) >> 15);
      pContMeas->nILineRingPeak =
               (IFX_int16_t)(((IFX_int32_t)pContMeasRead->Iring * 10000) >> 15);
      pContMeas->nVLineRingPeak =
               (IFX_int16_t)(((IFX_int32_t)pContMeasRead->Vring * 14400) >> 15);
   }

   TAPI_OS_MutexRelease (&pCh->mtxChAcc);

   RETURN_STATUS (ret, IFX_NULL);
}
#endif /* DXS_FEAT_CONT_MEASUREMENT */


/**
   Timer return function to supervise the hook event window.

   The DXS provides a timestamp with each FW event. However this timestamp
   has a wraparound after ~66 seconds. To prevent calculation after a wraparound
   this timer resets the HookWindow flag.

   \param  timer        Timer ID.
   \param  arg          Pointer to a DXS channel structure.
*/
static IFX_void_t dxs_alm_HookWindow_OnTimer(
                        Timer_ID timer,
                        IFX_ulong_t arg)
{
   DXS_CHANNEL_t *pCh = (DXS_CHANNEL_t *)arg;
   TAPI_UNUSED(timer);

   if ((pCh != IFX_NULL) && (pCh->pALM != IFX_NULL))
   {
      pCh->pALM->bHookWindow = IFX_FALSE;
   }
}


/**
   Calculate the elapsed time since the last hook event.

   This function is to be called with each hook event. It calculates the
   elapsed time since the last hook event.

   The DXS provides a timestamp with each FW event. However, this timestamp
   has a wraparound after ~66 seconds. So an additional timer is used to prevent
   calculation after a wraparound. This function wraps the calculation and
   also restarts the timer.

   \param  pCh          Pointer to DXS channel structure.
   \param  current_timestamp  Current timestamp value from the FW event.

   \return
   Elapsed time since the last hook event in 1/8 ms steps.
   If the last event happened too long ago 0xFFFF is returned.
*/
IFX_uint16_t DXS_ALM_ElapsedTimeSinceLastHook(
                        DXS_CHANNEL_t *pCh,
                        IFX_uint16_t current_timestamp)
{
   IFX_uint16_t   elapsedTime;

   if(pCh == IFX_NULL)
   {
      /* Resource not valid. Channel number out of range */
      return 0xFFFF;
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* Resource not valid. Channel number out of range */
      return 0xFFFF;
   }

   if (pCh->pALM->bHookWindow)
   {
      /* Calculate elapsed time since the saved timestamp. In case the 16-bit
         counter had a wraparound since the saved timestamp the calculation
         will still be correct because the calculation is done with 16-bit
         unsigned data type. This will always result in the positive distance
         between the two timestamps. */
      elapsedTime = current_timestamp - pCh->pALM->nLastHookEvtTimestamp;
      /* Multiply by 8 to get from 1 ms to 1/8 ms. 3 MSB are 0 before the shift,
       * otherwise bHookWindow would be FALSE. */
      elapsedTime <<= 3;
   }
   else
   {
      elapsedTime = 0xFFFF;
   }

   /* Start timer, length of timer nominal 7000 ms. A random component is
      added to distribute the expiration of the timers in systems with multiple
      devices using polling where a lot of timers might be started in parallel.
      This time needs to be below the wraparound of the HW timestamp. */
   if (pCh->pALM->nHookWindowTimerId != 0)
   {
      pCh->pALM->bHookWindow = IFX_TRUE;
      TAPI_SetTime_Timer (pCh->pALM->nHookWindowTimerId,
         7000 + (current_timestamp & 0x00FF), IFX_FALSE, IFX_TRUE);
   }
   /* store the timestamp from this event */
   pCh->pALM->nLastHookEvtTimestamp = current_timestamp;

   return elapsedTime;
}


/**
   Return ring parameters.

   \param  pLLChannel   Pointer to DXS channel structure.
   \param  pRingParam   Pointer to struct for returning the data.

   \return
   - DXS_statusInvalCh wrong channel
   - DXS_statusOk if successful
*/
static IFX_int32_t DXS_TAPI_LL_ALM_RingParams_Get(
                        IFX_TAPI_LL_CH_t *pLLChannel,
                        struct IFX_TAPI_RING_PARAM *pRingParam)
{
   DXS_CHANNEL_t *pCh = (DXS_CHANNEL_t *)pLLChannel;

   /* sanity check */
   if(pCh == IFX_NULL)
   {
      /* errmsg: Resource not valid. Channel number out of range */
      return DXS_statusInvalCh;
   }

   if (pRingParam == IFX_NULL)
   {
      RETURN_STATUS (DXS_statusFuncParam, IFX_NULL);
   }

   if (pCh->pALM == IFX_NULL)
   {
      /* Resource not valid.
         Channel number out of range or line type is FXO */
      RETURN_STATUS (DXS_statusInvalCh, IFX_NULL);
   }

   pRingParam->ring_period = pCh->pALM->nRingPeriod;

   RETURN_STATUS (DXS_statusOk, IFX_NULL);
}


/* ========================================================================== */
/*                         Function pointer exports                           */
/* ========================================================================== */
/**
   Function called by init_module of device, fills up ALM function pointers
   which are passed to HL TAPI during registration.

   \param  pAlm         Pointer to ALM module.
*/
IFX_void_t DXS_ALM_Func_Register (IFX_TAPI_DRV_CTX_ALM_t *pAlm)
{
   /* Fill the function pointers of ALM module */
   pAlm->Volume_Set        = DXS_TAPI_LL_ALM_Volume_Set;
   pAlm->Volume_High_Level = DXS_TAPI_LL_ALM_Volume_High_Level;

#ifdef DXS_FEAT_TONE_GENERATOR
   pAlm->TG_Play           = DXS_TAPI_LL_ALM_TG_Play;
   pAlm->TG_Stop           = DXS_TAPI_LL_ALM_TG_Stop;
   pAlm->TG_ToneStep       = DXS_TAPI_LL_ALM_TG_Step;
#endif /* DXS_FEAT_TONE_GENERATOR */

   pAlm->Line_Type_Set     = DXS_TAPI_LL_ALM_Line_Type_Set;
   pAlm->Line_Mode_Set     = DXS_TAPI_LL_ALM_Line_Mode_Set;
   pAlm->Line_Mode_Get     = DXS_TAPI_LL_ALM_Line_Mode_Get;

#ifdef DXS_FEAT_METERING
   pAlm->Metering_Start    = DXS_TAPI_LL_ALM_MeteringStart;
#endif /* DXS_FEAT_METERING */

   pAlm->TestLoop          = DXS_TAPI_LL_ALM_TestLoop;
   pAlm->TestHookGen       = DXS_TAPI_LL_ALM_TestHookGen;

   pAlm->MWL_Activation_Get   = DXS_TAPI_LL_ALM_MWL_Activation_Get;
   pAlm->MWL_Activation_Set   = DXS_TAPI_LL_ALM_MWL_Activation_Set;

   pAlm->Calibration_Start       = DXS_TAPI_LL_ALM_Calibration_Start;
   pAlm->Calibration_Stop        = DXS_TAPI_LL_ALM_Calibration_Stop;
   pAlm->Calibration_Finish      = DXS_TAPI_LL_ALM_Calibration_Finish;
   pAlm->Calibration_Get         = DXS_TAPI_LL_ALM_Calibration_Get;
   pAlm->Calibration_Set         = DXS_TAPI_LL_ALM_Calibration_Set;
   pAlm->Calibration_Results_Get = DXS_TAPI_LL_ALM_Calibration_Results;

#ifdef DXS_FEAT_GR909
   pAlm->GR909_Start             = DXS_TAPI_LL_ALM_LT_GR909_Start;
   pAlm->GR909_Stop              = DXS_TAPI_LL_ALM_LT_GR909_Stop;
   pAlm->GR909_Result            = DXS_TAPI_LL_ALM_LT_GR909_Result;
   pAlm->NLT_RmesConfigSet       = DXS_TAPI_LL_ALM_NLT_RmesConfig_Set;
#endif /* DXS_FEAT_GR909 */

#ifdef DXS_FEAT_CAPACITANCE_MEASUREMENT
   pAlm->CheckCapMeasSup         = DXS_TAPI_LL_ALM_LT_CheckCapMeasSup;
   pAlm->CapMeasStart            = DXS_TAPI_LL_ALM_LT_CapMeasStart;
   pAlm->CapMeasStop             = DXS_TAPI_LL_ALM_LT_CapMeasStop;
   pAlm->CapMeasResult           = DXS_TAPI_LL_ALM_LT_CapMeasResult;
   pAlm->NLT_capacitance_result_get = DXS_TAPI_LL_ALM_NLT_Cap_Result;
#endif /* DXS_FEAT_CAPACITANCE_MEASUREMENT */

#ifdef DXS_FEAT_CALIBRATION_STORAGE
   pAlm->NLT_OLConfigSet         = DXS_TAPI_LL_ALM_NLT_OLConfig_Set;
   pAlm->NLT_OLConfigGet         = DXS_TAPI_LL_ALM_NLT_OLConfig_Get;
#endif /* DXS_FEAT_CALIBRATION_STORAGE */

#ifdef DXS_FEAT_CONT_MEASUREMENT
   pAlm->ContMeasReq             = DXS_TAPI_LL_ALM_ContMeas_Req;
   pAlm->ContMeasGet             = DXS_TAPI_LL_ALM_ContMeas_Get;
#endif /* DXS_FEAT_CONT_MEASUREMENT */

   pAlm->Ring_Parameter_Get      = DXS_TAPI_LL_ALM_RingParams_Get;
}
