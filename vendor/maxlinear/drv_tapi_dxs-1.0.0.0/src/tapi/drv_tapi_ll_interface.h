#ifndef _DRV_TAPI_LL_INTERFACE_H
#define _DRV_TAPI_LL_INTERFACE_H

/******************************************************************************

   Copyright 2006-2009 Infineon Technologies AG
   Copyright 2009-2015 Lantiq Deutschland GmbH
   Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
   Copyright 2018-2020 Intel Corporation.
   Copyright 2021-2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_ll_interface.h
   Contains the structures which are shared between the high-level TAPI and
    low-level TAPI drivers.
   These structures mainly contains the function pointers which will be exported
   by the low level driver.

   This file is divided in different sections depending on the firmware modules:
   - INTERRUPT_AND_PROTECTION_MODULE
   - CODER_MODULE
   - PCM_MODULE
   - SIG_MODULE
   - ALM_MODULE
   - CON_MODULE
   - MISC_MODULE
*/

#ifdef __cplusplus
   extern "C" {
#endif

/* ============================= */
/* Includes                      */
/* ============================= */

#include <ifx_types.h>
#include "drv_tapi_io.h"

/* ============================= */
/* Local Macros  Definitions    */
/* ============================= */

/* Channel 255 indicates the control device */
#define IFX_TAPI_DEVICE_CH_NUMBER    255

/* ================================ */
/* Enumerations                     */
/* ================================ */

/** Hook validation time array offsets for IFX_TAPI_LL_ALM_HookVt */
enum HOOKVT_IDX
{
   HOOKVT_IDX_HOOK_OFF = 0,
   HOOKVT_IDX_HOOK_ON,
   HOOKVT_IDX_HOOK_FLASH,
   HOOKVT_IDX_HOOK_FLASHMAKE,
   HOOKVT_IDX_DIGIT_LOW,
   HOOKVT_IDX_DIGIT_HIGH,
   HOOKVT_IDX_INTERDIGIT
};

/** Definition of different data streams */
typedef enum
{
   IFX_TAPI_STREAM_COD = 0,
   IFX_TAPI_STREAM_HDLC,
   IFX_TAPI_STREAM_MAX
} IFX_TAPI_STREAM_t;

/** Counters for packet statistics */
typedef enum
{
   /** total number of packets delivered without errors */
   TAPI_STAT_COUNTER_EGRESS_DELIVERED,
   /** total number of packets discarded */
   TAPI_STAT_COUNTER_EGRESS_DISCARDED,
   /** number of packets discarded due to congestion */
   TAPI_STAT_COUNTER_EGRESS_CONGESTED,
#if 0
   /** minimum latency of all packets transported */
   TAPI_STAT_COUNTER_EGRESS_LATENCY_MIN,
   /** maximum latency of all packets transported */
   TAPI_STAT_COUNTER_EGRESS_LATENCY_MAX,
   /** sum of the latency */
   TAPI_STAT_COUNTER_EGRESS_LATENCY_SUM,
#endif
   /** total number of packets delivered without errors */
   TAPI_STAT_COUNTER_INGRESS_DELIVERED,
   /** total number of packets discarded */
   TAPI_STAT_COUNTER_INGRESS_DISCARDED,
   /** number of packets discarded due to congestion */
   TAPI_STAT_COUNTER_INGRESS_CONGESTED,
#if 0
   /** minimum latency of all packets transported */
   TAPI_STAT_COUNTER_INGRESS_LATENCY_MIN,
   /** maximum latency of all packets transported */
   TAPI_STAT_COUNTER_INGRESS_LATENCY_MAX,
   /** sum of the latency */
   TAPI_STAT_COUNTER_INGRESS_LATENCY_SUM,
#endif
   /* maximum value of this enum */
   TAPI_STAT_COUNTER_MAX
} TAPI_STAT_COUNTER_t;

typedef enum
{
   IFX_TAPI_LL_TONE_DIR_NONE = 0,
   IFX_TAPI_LL_TONE_EXTERNAL = 0x1,
   IFX_TAPI_LL_TONE_INTERNAL = 0x2,
   IFX_TAPI_LL_TONE_BOTH = 0x3
} IFX_TAPI_LL_TONE_DIR_t;

/** Directions for DTMF dtetector activation and deactivation  */
typedef enum
{
   IFX_TAPI_LL_DTMFD_DIR_NONE = 0,
   IFX_TAPI_LL_DTMFD_DIR_EXTERNAL = 1,
   IFX_TAPI_LL_DTMFD_DIR_INTERNAL = 2,
   IFX_TAPI_LL_DTMFD_DIR_BOTH = 3
} IFX_TAPI_LL_DTMFD_DIR_t;

typedef struct {
   /* Describes the module type where the tone will be detected. */
   IFX_TAPI_MODULE_TYPE_t nMod;
   /* Direction from which to detect the DTMF signal. */
   IFX_TAPI_LL_DTMFD_DIR_t direction;
   /* Generate event when DTMF ends? */
   IFX_boolean_t bEndEvent;
} IFX_TAPI_LL_DTMFD_CFG_t;

typedef IFX_void_t      IFX_TAPI_LL_DEV_t;
typedef IFX_void_t      IFX_TAPI_LL_CH_t;
typedef IFX_uint16_t    IFX_TAPI_LL_ERR_t;

#ifndef DRV_TAPI_H
typedef struct _TAPI_DEV      TAPI_DEV;
typedef struct _TAPI_CHANNEL  TAPI_CHANNEL;
#endif /* DRV_TAPI_H */


/** \defgroup TAPI_LL_INTERFACE TAPI Low-Level driver interface
   Lists all the functions which are registered by the low level TAPI driver */
/**@{*/

/** \defgroup INTERRUPT_AND_PROTECTION_MODULE Protection service
   This service is used to lock and unlock the interrupts for protecting the
   access to shared data structures.   */

/** \defgroup PCM_MODULE PCM module services
   The PCM - */

/** \defgroup SIG_MODULE Signaling module service
   This service includes the functionalities like DTMF receiver, Tone detection/
   Generation, CID receiver and sender */

/** \defgroup ALM_MODULE Analong Line interface Module service
   Contains the functionalities of ALM module */

/** \defgroup CON_MODULE Connection Module
  This module provides functions to connect different DSP modules. It is used
  for conferencing, but also for basic dynamic connections */

/**@}*/


/** Specifies the capability of the tone generator regarding tone
    sequence support. */
typedef enum
{
   /** Plays out a frequency or silence. No tone sequence with
      cadences are supported */
   IFX_TAPI_TONE_RESSEQ_FREQ = 0x0,
   /** Plays out a full simple tone including cadences and loops */
   IFX_TAPI_TONE_RESSEQ_SIMPLE = 0x1
}IFX_TAPI_TONE_RESSEQ_t;

/** Tone resource information. */
typedef struct
{
   /** Resource ID or number of the generator. Used as index in the tone status array and
   must be a number between 0 and TAPI_TONE_MAXRES */
   IFX_uint8_t nResID;
   /** Number of maximum supported frequencies at one time */
   IFX_uint8_t nFreq;
   /** Specifies the capability of the tone generator regarding tone
       sequence support. See \ref IFX_TAPI_TONE_RESSEQ_t for details. */
   IFX_TAPI_TONE_RESSEQ_t sequenceCap;
} IFX_TAPI_TONE_RES_t;

/* =============================== */
/* Defines for complex tone states */
/* =============================== */
typedef enum
{
   /** Initialization state */
   TAPI_CT_IDLE = 0,
   /** UTG tone sequence is not active, but the tone is in pause state */
   TAPI_CT_ACTIVE_PAUSE,
   /** Tone is currently playing out on the tone generator */
   TAPI_CT_ACTIVE,
   /* UTG tone sequence is deactived by the LL driver automatically. Afterwards a
   next step can be programmed or a new tone can be started. */
   TAPI_CT_DEACTIVATED

}TAPI_CMPLX_TONE_STATE_t;

/* ============================= */
/* Structure for CID data        */
/* ============================= */

/** Caller ID transmission data types. */
typedef enum
{
   IFX_TAPI_CID_DATA_TYPE_FSK_BEL202,
   IFX_TAPI_CID_DATA_TYPE_FSK_V23,
   IFX_TAPI_CID_DATA_TYPE_DTMF,
} IFX_TAPI_CID_DATA_TYPE_t;

/** Structure used for starting the CID transmitter in the LL-driver. */
struct TAPI_CID_DATA;
typedef struct TAPI_CID_DATA TAPI_CID_DATA_t;

typedef enum
{
   TAPI_CID_ALERT_NONE,
   /** first ring burst */
   TAPI_CID_ALERT_FR,
   /** DTAS */
   TAPI_CID_ALERT_DTAS,
   /** Line Reversal with DTAS */
   TAPI_CID_ALERT_LR_DTAS,
   /** Ring Pulse */
   TAPI_CID_ALERT_RP,
   /** Open Switch Interval */
   TAPI_CID_ALERT_OSI,
   /** CPE Alert Signal */
   TAPI_CID_ALERT_CAS,
   /** Alert Signal (NTT) */
   TAPI_CID_ALERT_AS_NTT,
   /** CAR Signal (NTT) */
   TAPI_CID_ALERT_CAR_NTT,
   /** Line Reversal */
   TAPI_CID_ALERT_LR
} IFX_TAPI_CID_ALERT_TYPE_t;

typedef struct
{
   /** The configured CID standard */
   IFX_TAPI_CID_STD_t      nStandard;

   /** Type of ETSI Alert of onhook services associated to ringing.
      Default IFX_TAPI_CID_ALERT_ETSI_FR */
   IFX_TAPI_CID_ALERT_ETSI_t   nETSIAlertRing;
   /** Type of ETSI Alert of onhook services not associated to ringing.
      Default IFX_TAPI_CID_ALERT_ETSI_RP. */
   IFX_TAPI_CID_ALERT_ETSI_t   nETSIAlertNoRing;
   /** Tone table index for the alert tone to be used.
      Required for automatic CID/MWI generation. Default XXXXXd. */
   IFX_uint32_t            nAlertToneOnhook;
   IFX_uint32_t            nAlertToneOffhook;
   /* Time needed to play the alert tone. */
   IFX_uint32_t            nAlertToneTime;
   /** Ring Pulse on time interval. */
   IFX_uint32_t            ringPulseOnTime;
   /** Ring Pulse off time interval. */
   IFX_uint32_t            ringPulseOffTime;
   /** Ring Pulse Loops. */
   IFX_uint32_t            ringPulseLoop;
   /** DTMF ACK after CAS, used for offhook transmission. Default DTMF 'D'. */
   IFX_char_t              ackTone;
   /** Usage of OSI for offhook transmission. Default "no use" */
   IFX_uint32_t            OSIoffhook;
   /** Lenght of the OSI signal in ms. Default 200 ms. */
   IFX_uint32_t            OSItime;
   /** First ring cadence time, used for TAPI_CID_ALERT_FR */
   IFX_uint32_t            cadenceRingBurst;
   /** Cadence ring pause time for CID transmission, used for TAPI_CID_ALERT_FR */
   IFX_uint32_t            cadenceRingPause;
   /** Reception of 2nd ack signal timeout (after data transmission) (NTT)*/
   IFX_uint32_t            ack2Timeout;
   /** Duration of the Subscriber Alerting Signal (SAS) tone. */
   IFX_uint32_t            nSasToneTime;
   /** Time to wait before a SAS tone, in ms, for offhook services. */
   IFX_uint32_t            beforeSAStime;
   /** Time to wait between generation of SAS and CAS tone, in ms. */
   IFX_uint32_t            SAS2CAStime;
   /** Tone table index for the SAS tone to be used. */
   IFX_uint32_t            nSAStone;
   /** Do not transfer Caller ID */
   IFX_boolean_t           bRingOnly;

   IFX_TAPI_CID_ABS_REASON_t   TapiCidDtmfAbsCli;
   IFX_TAPI_CID_TIMING_t       TapiCidTiming;
   IFX_TAPI_CID_FSK_CFG_t      TapiCidFskConf;
   IFX_TAPI_CID_DTMF_CFG_t     TapiCidDtmfConf;
} IFX_TAPI_CID_CONF_t;

/** Struct used for starting the CID sequence in the LL-driver. */
typedef struct
{
   /* Select CID type 1 (ON-) or type 2 (OFF-HOOK) */
   IFX_TAPI_CID_HOOK_MODE_t   txHookMode;
   /* CID data to be sent */
   IFX_uint8_t                *pCidParam;
   /* number of CID data octets */
   IFX_uint16_t               nCidParamLen;
   /* alert type for sequence */
   IFX_TAPI_CID_ALERT_TYPE_t  nAlertType;
   /* flag for starting periodical ringing */
   IFX_boolean_t              bRingStart;
   /* caller id config */
   IFX_TAPI_CID_CONF_t        *pConfData;
   /* pointer to the cadence we currently use */
   IFX_uint8_t                *pCurrentCadence;
   /* copy of the number of bits in the cadence we currently use */
   IFX_uint16_t               BitsInCurrentCadence;
   /* pointer to the cadence we currently use */
   IFX_uint8_t                *pPeriodicCadence;
   /* copy of the number of bits in the cadence we currently use */
   IFX_uint16_t               BitsInPeriodicCadence;
   /* ringing will stop automatically after the maximum ring was reached */
   IFX_uint32_t               nMaxRings;
} IFX_TAPI_CID_SEQ_CONF_t;

typedef struct
{
   /* Select CID type 1 (ON-) or type 2 (OFF-HOOK) */
   IFX_TAPI_CID_HOOK_MODE_t   txHookMode;
   /* The type of CID data to be sent */
   IFX_TAPI_CID_DATA_TYPE_t   cidDataType;
   /* CID data to be sent */
   IFX_uint8_t                *pCidParam;
   /* number of CID data octets */
   IFX_uint16_t               nCidParamLen;
   /* FSK transmitter/receiver specific settings */
   IFX_TAPI_CID_FSK_CFG_t     *pFskConf;
   /* DTMF transmitter specific settings */
   IFX_TAPI_CID_DTMF_CFG_t    *pDtmfConf;
} IFX_TAPI_CID_TX_t;


/** Struct for getting ringing parameters only known to the LL-driver. */
struct IFX_TAPI_RING_PARAM
{
   /** Ring period in milliseconds */
   IFX_uint16_t            ring_period;
};


/* =============================== */
/* Defines for error reporting     */
/* =============================== */

/** Classify error when reported via event dispatcher. For internal use */
typedef enum
{
   /** Report TAPI channel specific error */
   IFX_TAPI_ERRSRC_TAPI_CH    = 0,
   /** Report TAPI device or global error */
   IFX_TAPI_ERRSRC_TAPI_DEV   = 0x1000,
   /** Report low level global error */
   IFX_TAPI_ERRSRC_LL_DEV     = 0x2000,
   /** Report low level channel specific error */
   IFX_TAPI_ERRSRC_LL_CH      = 0x4000,
   /** Bit mask for modification of low level driver error codes. This bit
       is set for low level driver error codes */
   IFX_TAPI_ERRSRC_LL         = 0x8000,
   /** Maks of error sources used for clearing */
   IFX_TAPI_ERRSRC_MASK       = (IFX_TAPI_ERRSRC_LL |
                                 IFX_TAPI_ERRSRC_LL_CH |
                                 IFX_TAPI_ERRSRC_LL_DEV |
                                 IFX_TAPI_ERRSRC_TAPI_DEV |
                                 IFX_TAPI_ERRSRC_TAPI_CH)
}IFX_TAPI_ERRSRC;


/* ============================= */
/* Defines for resource counts   */
/* ============================= */

/** Struct used for reporting resources from LL to HL. */
typedef struct
{
   /** Number of ALM modules or analog lines */
   IFX_uint16_t            AlmCount;
   /** Number of PCM modules */
   IFX_uint16_t            PcmCount;
   /** Number of DTMF generators */
   IFX_uint16_t            DTMFGCount;
   /** Number of DTMF receivers */
   IFX_uint16_t            DTMFRCount;
   /** Number of FSK generators */
   IFX_uint16_t            FSKGCount;
   /** Number of UGT/TG capable channels */
   IFX_uint16_t            ToneCount;
   /* additional resources can be added here
      initialise them in ifx_tapi_Prepare_Dev() */
} IFX_TAPI_RESOURCE;

/* ============================= */
/* Structure for CERR reporting  */
/* ============================= */

/** Report the reason and details of a cmd error to drv_tapi. */
typedef struct _IFX_TAPI_DBG_CERR {
   /** reason code */
   IFX_uint32_t      cause;
   /** cmd header */
   IFX_uint32_t      cmd;
} IFX_TAPI_DBG_CERR_t;

/** Get debug information of an SSI crash event. */
struct IFX_TAPI_DBG_SSI_CRASH {
   /** debug information */
   IFX_uint32_t      cause[3];
};

/* ============================================= */
/* Structure for Capacitance measurement result  */
/* ============================================= */

/** Tip to Ring Capacitance measurement result. */
typedef struct _IFX_TAPI_NLT_T2R_CAPACITANCE_RESULT {
   /** Validity of measurement result tip to ring. */
   IFX_boolean_t  bValidTip2Ring;
   /** Measured capacitance tip to ring [nF]. */
   IFX_uint32_t   nCapTip2Ring;
} IFX_TAPI_NLT_T2R_CAPACITANCE_RESULT_t;

/** Line to Ground Capacitance measurement result. */
typedef struct _IFX_TAPI_NLT_L2GND_CAPACITANCE_RESULT {
   /** Validity of measurement results tip to ground and ring to ground.*/
   IFX_boolean_t  bValidLine2Gnd;
   /** Measured capacitance tip to ground [nF]. */
   IFX_uint32_t   nCapTip2Gnd;
   /** Measured capacitance ring to ground [nF]. */
   IFX_uint32_t   nCapRing2Gnd;
} IFX_TAPI_NLT_L2GND_CAPACITANCE_RESULT_t;

/* ============================================= */
/* Structure for open loop configuration         */
/* ============================================= */
/** Structure used for storing the open loop capacitance.
    The float values in here are correction factors that are just stored in
    the driver and will be returned together with the associated measurement
    results. */
typedef struct _IFX_TAPI_NLT_OL_CAPACITANCE_CONF {
   /** Open loop capacitance tip to ring [nF] (in).
       Float value stored in unsigned int bytes. */
   IFX_uint32_t   fOlCapTip2Ring;
   /** Open loop capacitance tip to ground [nF] (in).
       Float value stored in unsigned int bytes. */
   IFX_uint32_t   fOlCapTip2Gnd;
   /** Open loop capacitance ring to ground [nF] (in).
       Float value stored in unsigned int bytes. */
   IFX_uint32_t   fOlCapRing2Gnd;
} IFX_TAPI_NLT_OL_CAPACITANCE_CONF_t;

/** Structure used for storing the open loop resistance.
    The float values in here are correction factors that are just stored in
    the driver and will be returned together with the associated measurement
    results. */
typedef struct _IFX_TAPI_NLT_OL_RESISTANCE_CONF {
   /** Open loop resistance tip to ring [Ohm] (in).
       Float value stored in unsigned int bytes. */
   IFX_uint32_t   fOlResTip2Ring;
   /** Open loop resistance tip to ground [Ohm] (in).
       Float value stored in unsigned int bytes. */
   IFX_uint32_t   fOlResTip2Gnd;
   /** Open loop resistance ring to ground [Ohm] (in).
       Float value stored in unsigned int bytes. */
   IFX_uint32_t   fOlResRing2Gnd;
} IFX_TAPI_NLT_OL_RESISTANCE_CONF_t;

/* ============================= */
/* Defines for Driver Context    */
/* ============================= */

/** Interrupt and Protection Module */
/** \addtogroup INTERRUPT_AND_PROTECTION_MODULE */
/** Used for data protection by higher layer */
/**@{*/
typedef struct
{
   /** This function disables the irq line if the driver is in interrupt mode
      \param pLLDev     Handle to low-level device
   */
   IFX_void_t (*LockDevice) (IFX_TAPI_LL_DEV_t *pLLDev);
   /** This function enables the irq line if the driver is in interrupt mode
      \param pLLDev     Handle to low-level device
   */
   IFX_void_t (*UnlockDevice) (IFX_TAPI_LL_DEV_t *pLLDev);
   /** This function enables the irq line if the driver is in interrupt mode
      \param pLLDev     Handle to low-level device
   */
   IFX_void_t (*IrqEnable) (IFX_TAPI_LL_DEV_t *pLLDev);
   /** This function disables the irq line if the driver is in interrupt mode
      \param pLLDev     Handle to low-level device
   */
   IFX_void_t (*IrqDisable) (IFX_TAPI_LL_DEV_t *pLLDev);
} IFX_TAPI_DRV_CTX_IRQ_t;

/**@}*/ /* INTERRUPT_AND_PROTECTION_MODULE */

/** PCM Module */  /* ***************************************************** */
/** \addtogroup PCM_MODULE */
/** Used for PCM services higher layer */
/**@{*/
typedef struct
{
   /** Configure and enable the PCM interface
      \param pLLDev     Handle to low-level device
      \param pCfg       Pointer to the configuration structure
      \return
         IFX_SUCCESS if successful else device specific return code.
    */
   IFX_int32_t (*ifCfg) (
      IFX_TAPI_LL_DEV_t *pLLDev,
      IFX_TAPI_PCM_IF_CFG_t const *pCfg);

   /** Prepare parameters and call the target function to activate PCM module
      \param pLLCh      Handle to low-level channel
      \param nMode      Activation mode
      \param pPcmCfg    Pointer to the PCM configuration structure
      \return
         IFX_SUCCESS if successful else device specific return code.
    */
   IFX_int32_t (*Enable) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_uint32_t nMode,
      IFX_TAPI_PCM_CFG_t *);

   /** Prepare parameters and call the target function to Configure the PCM module
      \param pLLCh      Handle to low-level channel
      \param pPcmCfg    Pointer to the PCM configuration structure
      \return
         IFX_SUCCESS if successful else device specific return code.
    */
   IFX_int32_t (*Cfg) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_PCM_CFG_t const *pPCMConfig);

   /** Muting for PCM channels
   \param pLLCh           Pointer to Low-level channel structure
   \param pMuteCfg        Pointer to the PCM Mute structure
   \return
   IFX_SUCCESS if successful
   IFX_ERROR if an error occurred */
   IFX_int32_t (*Mute) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_PCM_MUTE_CFG_t const *pMuteCfg);

} IFX_TAPI_DRV_CTX_PCM_t;
/**@}*/ /* PCM_MODULE*/


/** SIG Module */ /* ********************************************************/
/** \addtogroup SIG_MODULE */
/** Signalling module services*/
/**@{*/
typedef struct
{
   /** Sets/Gets DTMF receiver coefficients.
      \param pLLCh         Handle to low-level channel
      \param bRW           IFX_FALSE to write, IFX_TRUE to read settings
      \param pDtmfRxCoeff  Pointer to DTMF Rx coefficients settings
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*DTMF_RxCoeff) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_boolean_t bRW,
      IFX_TAPI_DTMF_RX_CFG_t *pDtmfRxCoeff);

   /** Start CID data transmission
      \param pLLCh      Handle to low-level channel
      \param pCidData   Pointer to the CID transmition configuration and data
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*CID_TX_Start) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_CID_TX_t const *pCidData);

   /** Stop CID data transmission
      \param pLLCh      Handle to low-level channel
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*CID_TX_Stop) (IFX_TAPI_LL_CH_t *pLLCh);

} IFX_TAPI_DRV_CTX_SIG_t;


/** ALM Module */ /* ********************************************************/
/** \addtogroup ALM_MODULE */
/** Analog line module services*/
/**@{*/
typedef struct
{
   /** Set line type and sampling operation mode of the analog line.
      \param pLLCh   Handle to low-level channel
      \param nType   Line type and sampling mode to be set
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Line_Type_Set) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_LINE_TYPE_t nType);

   /** Set the line mode of the analog line
      \param pLLCh            Handle to low-level channel
      \param nMode
      \param nTapiLineMode
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Line_Mode_Set) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_int32_t nMode,
      IFX_uint8_t nTapiLineMode);

   /** Read back the line mode of the analog line
      \param pLLCh            Handle to low-level channel
      \param pMode
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Line_Mode_Get) (
      IFX_TAPI_LL_CH_t *pLLCh, IFX_TAPI_LINE_FEED_t *pMode);

   /** Set the phone volume
      \param pLLCh      Handle to low-level channel
      \param pVol       Pointer to IFX_TAPI_LINE_VOLUME_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Volume_Set) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_LINE_VOLUME_t const *pVol);

   /** This service enables or disables a high level path of a phone channel.
      \param pLLCh      Handle to low-level channel
      \param bEnable    Enable or disable
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Volume_High_Level) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_int32_t bEnable);

   /** Ring parameter get
      \param pLLCh         Handle to low-level channel
      \param pRingParam    Pointer to ring parameter structure which is to be
                           filled with data.
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Ring_Parameter_Get) (
      IFX_TAPI_LL_CH_t *pLLCh,
      struct IFX_TAPI_RING_PARAM *pRingParam);

   /** Send metering burst
      \param pLLCh      Handle to low-level channel
      \param nPulseNum  Number of pulses
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Metering_Start) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_uint32_t nPulseNum);

   /** Starts playing out a tone on the ALM tone generator.
      \param pLLCh         Handle to low-level channel
      \param res           Resource number used for playing the tone.
      \param pToneSimple   Pointer to the tone definition to play
      \param dst           Destination where to play the tone
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*TG_Play) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_uint8_t res,
      IFX_TAPI_TONE_SIMPLE_t const *pToneSimple);

   /** Stop playing the tone with the given tone definition
      \param pLLCh   Handle to low-level channel
      \param res     Resource number used for playing the tone
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*TG_Stop) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_uint8_t res);

   /** Starts playing out the next tone of the simple tone definition.
      \param pLLCh      Handle to low-level channel
      \param pTone      Pointer to the current simple tone definition
      \param res        Resource number used for playing the tone
      \param nToneStep  Identifies the next tone step of the simple tone
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*TG_ToneStep) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_TONE_SIMPLE_t const *pTone,
      IFX_uint8_t res, IFX_uint8_t *nToneStep);

   /** Gets the activation status of the message waiting lamp.
      \param pLLCh         Handle to low-level channel
      \param pActivation   Handle to \ref IFX_TAPI_MWL_ACTIVATION_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*MWL_Activation_Get) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_MWL_ACTIVATION_t *pActivation);

   /** Activate/deactivates the message waiting lamp.
      \param pLLCh         Handle to low-level channel
      \param pActivation   Handle to \ref IFX_TAPI_MWL_ACTIVATION_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*MWL_Activation_Set) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_MWL_ACTIVATION_t const *pActivation);

   /** Simulate Hook generation (for debug use only)
      \param pLLCh   Handle to low-level channel
      \param bHook   Hook state
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*TestHookGen) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_boolean_t bHook);

   /** ALM 8kHz test loop switch (for debug use only)
      \param pLLCh   Handle to low-level channel
      \param pLoop    Handle to \ref IFX_TAPI_TEST_LOOP_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*TestLoop) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_TEST_LOOP_t const *pLoop);

   /** Start calibration process for analog channel
      \param pLLCh   Handle to low-level channel
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Calibration_Start) (IFX_TAPI_LL_CH_t *pLLCh);

   /** Stop calibration process for analog channel
      \param pLLCh   Handle to low-level channel
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Calibration_Stop) (IFX_TAPI_LL_CH_t *pLLCh);

   /** Finish the calibration process on an analog channel
      \param pLLCh   Handle to low-level channel
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Calibration_Finish) (IFX_TAPI_LL_CH_t *pLLCh);

   /** Read out the current calibration values of the analog channel
      \param pLLCh         Handle to low-level channel
      \param pClbConfig    Handle to \ref IFX_TAPI_CALIBRATION_CFG_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Calibration_Get) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_CALIBRATION_CFG_t *pClbConfig);

   /** Writes the calibration values of the analog channel
      \param pLLCh         Handle to low-level channel
      \param pClbConfig    Handle to \ref IFX_TAPI_CALIBRATION_CFG_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Calibration_Set) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_CALIBRATION_CFG_t const *pClbConfig);

   /** Read out the calibration results of the analog channel
      \param pLLCh         Handle to low-level channel
      \param pClbConfig    Handle to \ref IFX_TAPI_CALIBRATION_CFG_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Calibration_Results_Get) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_CALIBRATION_CFG_t *pClbConfig);

   /** Start selected subset (or all) GR909 tests
      \param pLLCh         Handle to low-level channel
      \param pGR909Start   Handle to \ref IFX_TAPI_GR909_START_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*GR909_Start) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_GR909_START_t const *pGR909Start);

   /** Stop GR909 tests
      \param pLLCh   Handle to low-level channel
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*GR909_Stop) (IFX_TAPI_LL_CH_t *pLLCh);

   /** Read GR909 results
      \param pLLCh         Handle to low-level channel
      \param pGR909Result  Handle to \ref IFX_TAPI_GR909_RESULT_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*GR909_Result) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_GR909_RESULT_t *pGR909Result);

   /** Request continuous measurement results
      \param pLLCh   Handle to low-level channel
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*ContMeasReq)  (IFX_TAPI_LL_CH_t *pLLCh);

   /** Return the stored continuous measurement results
      \param pLLCh      Handle to low-level channel
      \param pContMeas  Handle to \ref IFX_TAPI_CONTMEASUREMENT_GET_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*ContMeasGet) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_CONTMEASUREMENT_GET_t *pContMeas);

   /** Start an analog line capacitance measurement session
      \param pLLCh      Handle to low-level channel
      \param bTip2RingOnly   Measure only tip to ring capacitance.
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*CapMeasStart) (IFX_TAPI_LL_CH_t *pLLCh,
                                IFX_boolean_t bTip2RingOnly);

   /** Stop any running analog line capacitance measurement session
      \param pLLCh      Handle to low-level channel
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*CapMeasStop) (IFX_TAPI_LL_CH_t *pLLCh);

   /** Get capacitance measurement result
      \param pLLCh      Handle to low-level channel
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*CapMeasResult) (IFX_TAPI_LL_CH_t *pLLCh);

   /** Check cpacitance measurement support.
      \param pLLCh       Handle to low-level channel
      \param pSupported  Returns information whether capacitance measurement
                         is supported (IFX_TRUE) or not (IFX_FALSE).
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*CheckCapMeasSup) (IFX_TAPI_LL_CH_t *pLLCh,
                                   IFX_boolean_t *pSupported);

   /**Configures the open loop calibration factors of the measurement path
       for line testing
      \param pLLCh   Handle to low-level channel
      \param pArg    Pointer to IFX_TAPI_NLT_CONFIGURATION_OL_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*NLT_OLConfigSet) (
      IFX_TAPI_LL_CH_t *pLLCh,
      const IFX_TAPI_NLT_CONFIGURATION_OL_t *pArg);

   /**Gets the open loop calibration factors of the measurement path
       for line testing
      \param pLLCh   Handle to low-level channel
      \param pArg    Pointer to IFX_TAPI_NLT_CONFIGURATION_OL_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*NLT_OLConfigGet) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_NLT_CONFIGURATION_OL_t *pArg);

   /**Configures the measurement path for line testing according to
      Rmes resitor .
      \param pLLCh   Handle to low-level channel
      \param pArg    Pointer to IFX_TAPI_NLT_CONFIGURATION_RMES_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*NLT_RmesConfigSet) (
      IFX_TAPI_LL_CH_t *pLLCh,
      const IFX_TAPI_NLT_CONFIGURATION_RMES_t *pArg);

   /** Reads the results of the capacitance measurement.
      \param pLLCh    Handle to low-level channel
      \param pResult  Pointer to IFX_TAPI_NLT_CAPACITANCE_RESULT_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*NLT_capacitance_result_get) (
      IFX_TAPI_LL_CH_t *pLLCh,
      IFX_TAPI_NLT_CAPACITANCE_RESULT_t *pResult);

} IFX_TAPI_DRV_CTX_ALM_t;
/**@}*/


/** Network Linetesting (NLT) Interface */ /* *********************************/
/** \addtogroup NLT_INTERFACE */
/** List of required low-level NLT services */
/**@{*/
typedef struct
{
   /** Starts the specified NLT test on the specified channel.
      \param pLLCh   Handle to low-level channel
      \param pArg            Pointer to IFX_TAPI_NLT_TEST_START_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*NLT_test_start) (
      IFX_TAPI_LL_CH_t *pLLCh,
      const IFX_TAPI_NLT_TEST_START_t *pArg);

   /** Gets results of the specified NLT test on the specified channel.
      \param pLLCh   Handle to low-level channel
      \param pArg    Pointer to IFX_TAPI_NLT_RESULT_GET_t structure
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*NLT_result_get) (
      IFX_TAPI_LL_CH_t *pLLCh,
      const IFX_TAPI_NLT_RESULT_GET_t *pArg);

} IFX_TAPI_DRV_CTX_NLT_t;
/**@}*/


/* driver context data structure */
/** \addtogroup TAPI_LL_INTERFACE */
/** Interface between High-Level TAPI and Low-Level TAPI */
/**@{*/
typedef struct
{
   /** high-level and low-level interface API version, keep as first element */
   IFX_char_t                              *hlLLInterfaceVersion;
   /** device nodes prefix (if DEVFS is used) /dev/\<devNodeName\>\<number\> */
   IFX_char_t                              *devNodeName;
   /** driverName */
   IFX_char_t                              *drvName;
   /** driverVersion */
   IFX_char_t                              *drvVersion;

   IFX_uint16_t                             majorNumber;
   IFX_uint16_t                             minorBase;
   IFX_uint16_t                             maxDevs;
   IFX_uint16_t                             maxChannels;

   /** To get events from multiple devices in round robin manner remember
       which device to service next. */
   IFX_uint16_t                             nLastEventDevice;

   /* The following two prepare functions receive a pointer to the
      HL device and channel structures and return pointer to the
      LL device and channel structures(!)
   */

   /** Prepare the low-level device struct.
      \param pTapiDev   Pointer to the high-level device struct.
      \param devNum     Device number.
      \return
         Pointer to low-level device struct.
   */
   IFX_TAPI_LL_DEV_t* (*Prepare_Dev) (
      TAPI_DEV *pTapiDev,
      IFX_uint32_t devNum);

   /** Initialise the low-level device struct.
      \param pDev       Pointer to low-level device struct.
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Init_Dev) (IFX_TAPI_LL_DEV_t *pDev);

   /** Stop the low-level device and free all allocated resources.
      \param pDev       Pointer to low-level device struct.
      \param bChipAccess Allow or deny chip access in this function.
   */
   IFX_void_t (*Exit_Dev) (
      IFX_TAPI_LL_DEV_t *pDev,
      IFX_boolean_t bChipAccess);

   /** Prepare the low-level channel struct.
      \param pTapiCh    Pointer to high-level channel struct.
      \param pLLDev     Pointer to low-level device struct.
      \param chNum        Channel number.
      \return
         Pointer to low-level channel struct.
   */
   IFX_TAPI_LL_CH_t* (*Prepare_Ch) (
      TAPI_CHANNEL *pTapiCh,
      IFX_TAPI_LL_DEV_t *pLLDev,
      IFX_uint32_t chNum);

   /** Initialise the low level channel struct.
      \param pCh        Pointer to low-level channel struct.
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Init_Ch) (IFX_TAPI_LL_CH_t *pCh);

   /** Start the firmware
      \param pLLDev     Pointer to low-level device struct.
      \param pProc      Pointer to low-level device initialization structure.
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*FW_Start) (
      IFX_TAPI_LL_DEV_t *pLLDev,
      IFX_void_t const *pProc);

   /** Initialise the firmware
      \param pLLDev     Pointer to low-level device struct.
      \param nMode      Enum from IFX_TAPI_INIT_MODE_t specifying the setup.
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*FW_Init) (
      IFX_TAPI_LL_DEV_t *pLLDev,
      IFX_uint8_t nMode);

   /** Download a BBD file.
      \param pCh        Pointer to low-level channel/device struct.
      \param pProc      Pointer to low-level device initialization structure.
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*BBD_Dnld) (
      IFX_TAPI_LL_CH_t *pCh,
      IFX_void_t const *pProc);

   /* obsolete / reserved */
   IFX_int32_t (*Pwr_Save_Dev) (IFX_TAPI_LL_DEV_t *pDev);

   /** Returns the number of entries in the capability list.
      \param pDev       Pointer to low-level device struct.
      \return
         The number of capability entries.
   */
   IFX_int32_t (*CAP_Number_Get) (IFX_TAPI_LL_DEV_t *pDev);

   /** Return the low-level device capability list.
      \param pDev       Pointer to low-level device struct.
      \param pCapList   Pointer to IFX_TAPI_CAP_LIST_t structure with details
                        where to copy the data to. No more capabilities than
                        specified in the element nCap will be copied into the
                        memory given in this structure.
      \return
         IFX_SUCCESS Always successful.
   */
   IFX_int32_t (*CAP_List_Get) (
      IFX_TAPI_LL_DEV_t *pDev,
      IFX_TAPI_CAP_LIST_t *pCapList);

   /** Checks in the capability list if a specific capability is supported.
      \param pDev       Pointer to low-level device struct.
      \param pCapList   Pointer to IFX_TAPI_CAP_t structure.
      \return
         Support status of the capability
         - 0 if not supported
         - 1 if supported
   */
   IFX_int32_t (*CAP_Check) (
      IFX_TAPI_LL_DEV_t *pDev,
      IFX_TAPI_CAP_t *pCapList);

   /** Returns free command mailbox inbox space.
      \param pDev       Pointer to low-level device struct.
      \param cmdmbx_size Pointer to variable where to return the command inbox
                        size. No check is done on this parameter!
      \return
         IFX_SUCCESS or IFX_ERROR.
   */
   IFX_int32_t (*GetCmdMbxSize) (
      IFX_TAPI_LL_DEV_t *pDev,
      IFX_uint8_t *cmdmbx_size);

   /** Called when TAPI channel is opened.
      \param pLLCh      Pointer to low-level channel struct.
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Open) (IFX_TAPI_LL_CH_t *pLLCh);
   /** Called when TAPI channel is closed.
      \param pLLCh      Pointer to low-level channel struct.
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Release) (IFX_TAPI_LL_CH_t *pLLCh);

   /** Forward IOCTL to low level driver.
      \param pCh        Pointer to either device or channel struct.
      \param nCmd       IOCTL identifier.
      \param ioarg      IOCTL argument.
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*Ioctl) (
      IFX_TAPI_LL_CH_t *pCh,
      IFX_uint32_t nCmd,
      IFX_ulong_t ioarg);

   /** Forward IOCTL to low level driver. Designated for handling 32 bit IOCTL
       in 64 bit kernel.
      \param pCh        Pointer to either device or channel struct.
      \param nCmd       IOCTL identifier.
      \param ioarg      IOCTL argument.
      \return
         IFX_SUCCESS if successful else device specific return code.
   */
   IFX_int32_t (*CompatIoctl) (
      IFX_TAPI_LL_CH_t *pCh,
      IFX_uint32_t nCmd,
      IFX_ulong_t ioarg);

   /** Write a packet downstream
      \param pCh        Handle to low-level channel.
      \param buf        Pointer to a buffer with the data to be sent.
      \param count      Data length in bytes.
      \param ppos       unused
      \param stream     Tag that identifies the packet contents.
      \return
         - IFX_ERROR    on failure
         - nLength      length of handled data
   */
   IFX_int32_t (*Write) (
      IFX_TAPI_LL_CH_t *pCh,
      const IFX_char_t *buf,
      IFX_int32_t count,
      IFX_int32_t* ppos,
      IFX_TAPI_STREAM_t stream);

   /** amount of bytes HL must reserve before the data in downstream packets */
   IFX_uint32_t                             pktBufPrependSpace;
   /** for each stream the number of channels that provide packet read */
   IFX_uint16_t                             readChannels[IFX_TAPI_STREAM_MAX];

   /* PCM related functions for the HL TAPI */
   IFX_TAPI_DRV_CTX_PCM_t                   PCM;

   /* Signalling Module related functions for HL TAPI */
   IFX_TAPI_DRV_CTX_SIG_t                   SIG;

   /* Analog Line Module related functions for HL TAPI */
   IFX_TAPI_DRV_CTX_ALM_t                   ALM;

   /* Protection andn Interrupt module functions for the HL TAPI */
   IFX_TAPI_DRV_CTX_IRQ_t                   IRQ;

   /* Network Linetesting (NLT) related LL routines to be used by HL TAPI */
   IFX_TAPI_DRV_CTX_NLT_t                   NLT;

   /** array of pTapiDev pointers associated with this driver context */
   TAPI_DEV                                *pTapiDev;
} IFX_TAPI_DRV_CTX_t;
/**@}*/

#ifndef DRV_TAPI_H
struct _TAPI_DEV
{
   /* channel number IFX_TAPI_DEVICE_CH_NUMBER indicates the control device */
   /* ATTENTION, nChannel must be the first element */
   IFX_uint8_t               nChannel;
   /* pointer to LL device structure */
   IFX_TAPI_LL_DEV_t        *pLLDev;
};

struct _TAPI_CHANNEL
{
   /* channel number */
   /* ATTENTION, nChannel must be the first element */
   IFX_uint8_t                   nChannel;
   /* pointer to the Low level driver channel */
   IFX_TAPI_LL_CH_t             *pLLChannel;
   /* pointer to the tapi device structure */
   TAPI_DEV                     *pTapiDevice;

   /* \note:
      This is an incomplete declaration of the TAPI channel structure which is
      used only in the LL driver.
      For the complete implementation see in drv_tapi.h.
   */
};
#endif /* DRV_TAPI_H */


typedef IFX_void_t* Timer_ID;
typedef IFX_void_t (*TIMER_ENTRY)(Timer_ID timer_id, IFX_ulong_t arg);

typedef struct IFX_TAPI_DRV_CTX_t TAPI_LOW_LEVEL_DRV_CTX_t;

/* Registration function for the Low Level TAPI driver */
extern IFX_int32_t  IFX_TAPI_Register_LL_Drv    (IFX_TAPI_DRV_CTX_t*);
extern IFX_int32_t  IFX_TAPI_Unregister_LL_Drv  (IFX_int32_t majorNumber);
extern IFX_void_t   IFX_TAPI_DeviceReset        (TAPI_DEV *pTapiDev);
extern IFX_void_t   IFX_TAPI_ReportResources    (TAPI_DEV *pTapiDev,
                                                 const IFX_TAPI_RESOURCE *pResources);

extern Timer_ID      TAPI_Create_Timer          (TIMER_ENTRY pTimerEntry,
                                                 IFX_ulong_t nArgument);
extern IFX_boolean_t TAPI_SetTime_Timer         (Timer_ID Timer,
                                                 IFX_uint32_t nTime,
                                                 IFX_boolean_t bPeriodically,
                                                 IFX_boolean_t bRestart);
extern IFX_boolean_t TAPI_Delete_Timer          (Timer_ID Timer);
extern IFX_boolean_t TAPI_Stop_Timer            (Timer_ID Timer);

extern IFX_void_t*   IFX_TAPI_VoiceBufferGet    (void);
extern IFX_void_t*   IFX_TAPI_VoiceBufferGetAndSize(
                        IFX_uint32_t *buffer_size);
extern IFX_int32_t   IFX_TAPI_VoiceBufferPut    (IFX_void_t *pData);
extern IFX_int32_t   IFX_TAPI_UpStreamFifo_Reset(TAPI_CHANNEL* pChannel,
                                                 IFX_TAPI_STREAM_t nStream);
extern IFX_int32_t   IFX_TAPI_UpStreamFifo_Put  (TAPI_CHANNEL* pTapiCh,
                                                 IFX_TAPI_STREAM_t nStream,
                                                 const IFX_void_t * const pData,
                                                 const IFX_uint32_t nLength,
                                                 const IFX_uint32_t nOffset);
extern IFX_void_t*   IFX_TAPI_DownStreamFifo_Handle_Get(TAPI_DEV* pTapiDev);

extern void    IFX_TAPI_DownStream_RequestData(
                        TAPI_CHANNEL* pChannel,
                        IFX_boolean_t bRequest);

extern IFX_int32_t IFX_TAPI_Ring_Stop_Ext(TAPI_CHANNEL *pChannel,
                                          IFX_boolean_t bRestoreLineFeed);

#define IFX_TAPI_Event_ImmediateDispatch(pCh,pEvent) \
      IFX_TAPI_Event_DispatchExt (pCh,pEvent, IFX_FALSE)
#define IFX_TAPI_Event_DeferredDispatch(pCh,pEvent) \
      IFX_TAPI_Event_DispatchExt (pCh,pEvent, IFX_TRUE)

extern IFX_int32_t IFX_TAPI_Event_Dispatch (TAPI_CHANNEL *pChannel,
   IFX_TAPI_EVENT_t *pTapiEvent);

extern IFX_int32_t IFX_TAPI_Event_DispatchExt (TAPI_CHANNEL *pChannel,
   IFX_TAPI_EVENT_t *pTapiEvent, IFX_boolean_t bDefer);

#ifdef TAPI_VERSION3
extern IFX_void_t TAPI_Tone_Set_Source(TAPI_CHANNEL *pChannel,
                                       IFX_uint8_t nResId,
                                       IFX_TAPI_TONE_RESSEQ_t src);
#endif /* TAPI_VERSION3 */

extern IFX_void_t TAPI_Cid_Abort(TAPI_CHANNEL *pChannel);
extern IFX_boolean_t TAPI_Cid_IsActive(TAPI_CHANNEL *pChannel);

extern TAPI_CMPLX_TONE_STATE_t TAPI_ToneState(TAPI_CHANNEL *pChannel,
                                              IFX_uint8_t nResId);

#ifdef __cplusplus
   }
#endif

#endif /* _DRV_TAPI_LL_INTERFACE_H */
