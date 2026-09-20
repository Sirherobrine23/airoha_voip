#ifndef _DRV_DXS_ALM_PRIV_H
#define _DRV_DXS_ALM_PRIV_H
/******************************************************************************

  Copyright 2014-2015 Lantiq Deutschland GmbH
  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016, 2020 Intel Corporation.
  Copyright 2021,2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_alm_priv.h
   This file contains the defines, the structures declarations for ALM module.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include "../tapi/drv_tapi_ll_interface.h"
#include "drv_dxs_fw_cmd_sdd.h"
#include "drv_dxs_api.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

#ifdef DXS_FEAT_NLT

/** AC Level Meter measurement status. */
enum ACLM_meas_status
{
   /** Initial */
   dxs_aclm_init,
   /** Measurement in progress */
   dxs_aclm_in_progress,
   /** Measurement finished */
   dxs_aclm_finished,
   /** Measurement aborted */
   dxs_aclm_aborted
};

/** AC Level Meter measurement type. */
typedef enum
{
   /** Frequency Response measurement */
   DXS_ACLM_FR = IFX_TAPI_NLT_AC_FREQRESPONSE_ID,
   /** Transhybrid measurement */
   DXS_ACLM_TH = IFX_TAPI_NLT_AC_TRANSHYBRID_ID,
   /** Gain Tracking measurement */
   DXS_ACLM_GT = IFX_TAPI_NLT_AC_GAINTRACKING_ID,
   /** Signal to Noise Ratio measurement */
   DXS_ACLM_SNR = IFX_TAPI_NLT_AC_IDLENOISE_ID
} DXS_ACLM_Measurement_t;

#endif /* DXS_FEAT_NLT */

/**
   Structure for the ALM channel including firmware message cache */
struct DXS_ALM
{
   /* Current line operating mode - reflects the actual opmode.
      Written in interrupt context by the firmware event,
      read by opmode change requesting thread to ensure that the
      opmode transition is complete */
   volatile IFX_uint8_t    curr_opmode;
   volatile IFX_boolean_t  bOpmodeChangePending;
   /* SDD event */
   TAPI_OS_event_t          sdd_event;
   DXS_SDD_Opmode_t        sdd_opmode,
                           last_sdd_opmode;
   DXS_SDD_GR909Config_t   sdd_gr909_config;
   DXS_SDD_TxRxGain_t      sdd_txrx_gain;
   /* Cached FW message for calibration */
   DXS_SDD_Calibrate_t     fw_sdd_calibrate,
   /* cache for the last calibration results */
                           calibrationLastResults;
   IFX_TAPI_CALIBRATION_STATE_t  nCalibrationState;
   /* Basic config command */
   struct DXS_SDD_BasicConfig fw_sdd_basic_config;
#ifdef DXS_FEAT_CONT_MEASUREMENT
   /* read the results of the continuous measurement */
   DXS_SDD_ContMeasRead_t  fw_sdd_contMeasRead;
#endif /* DXS_FEAT_CONT_MEASUREMENT */

#ifdef DXS_FEAT_CAPACITANCE_MEASUREMENT
   /* capacitance measurement */
   DXS_SDD_CapMeasRead_t   fw_sdd_capacitance_meas;
   /*IFX_uint16_t            nCapMeasCycle;*/
   volatile IFX_boolean_t  bCapMeasInProgress;
   /* IFX_TRUE - only Tip to Ring capacitance is measured. */
   IFX_boolean_t           bCapMeasTip2RingOnly;
   /* Tip to Ring Capacitance measurement result. */
   IFX_TAPI_NLT_T2R_CAPACITANCE_RESULT_t    t2r_cap_result;
   /* Line to Ground Capacitance measurement result. */
   IFX_TAPI_NLT_L2GND_CAPACITANCE_RESULT_t  l2g_cap_result;
#endif /* DXS_FEAT_CAPACITANCE_MEASUREMENT */
#ifdef DXS_FEAT_NLT
   /* event used to continue measurement process */
   TAPI_OS_event_t          aclm_event;
   /* AC Level measurement */
   DXS_SDD_ACLevelMeterControl_t   fw_sdd_aclm_control;
   DXS_SDD_ACLevelMeterConfig_t    fw_sdd_aclm_config;
   DXS_SDD_ACLevelMeterResult_t    fw_sdd_aclm_result;
   /** Current measurement selector */
   DXS_ACLM_Measurement_t  aclm_current_meas;
   /** Measurement status */
   enum ACLM_meas_status   aclm_meas_status;

#if 0 /* Unused feature */
   /** Flag to restore DISABLED opmode after measurement finish */
   IFX_uint8_t             bAclmRestoreDisabled;
#endif

   /** Result index in a table */
   IFX_uint8_t             aclm_result_tbl_idx;
   /** Results table */
   IFX_TAPI_NLT_ACLM_Result_t aclm_results;
   /** Tx gain in units of 0.1 dB */
   IFX_int16_t             tx_gain;
   /** Rx gain in units of 0.1 dB */
   IFX_int16_t             rx_gain;
   /* Status of gains configuration */
   IFX_boolean_t           bGainsConfigured;

#endif /* DXS_FEAT_NLT */
   /* Indicates that calibration was called driver internal. */
   IFX_boolean_t           bCalibrationInternal;
   /* Indicates that calibration is needed after BBD download. */
   IFX_boolean_t           bCalibrationNeeded;
   /* Indicates that calibration is running */
   IFX_boolean_t           bCalibrationRunning;
   /* Semaphore to defer until calibration has completed. */
#ifdef LINUX
   TAPI_OS_event_t          evtCalibrationWait;
#else
   TAPI_OS_mutex_t          mtxCalibrationWait;
#endif
   /* open loop resistance configuration. */
   IFX_TAPI_NLT_OL_RESISTANCE_CONF_t  nlt_ResistanceConfig;
   /* open loop capacitance configuration. */
   IFX_TAPI_NLT_OL_CAPACITANCE_CONF_t nlt_CapacitanceConfig;
   /** Configuration which Rmeas variant is connected to the chip. */
   IFX_TAPI_NLT_RMEAS_CFG_t           nRmeas;

   /* elapsed time since last hook event */
   IFX_uint16_t            nLastHookEvtTimestamp;
   Timer_ID                nHookWindowTimerId;
   volatile IFX_boolean_t  bHookWindow;
   /* Type of DC/DC converter */
   enum DXS_DcDcType       nDcDcType;

   /* Ring period in milliseconds calculated from ring frequency in BBD file. */
   IFX_uint16_t            nRingPeriod;

   /** analog path gain for D->A direction - FW gain value */
   IFX_uint16_t            sdd_rx_gain;
};

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

#endif /* _DRV_DXS_ALM_PRIV_H */
