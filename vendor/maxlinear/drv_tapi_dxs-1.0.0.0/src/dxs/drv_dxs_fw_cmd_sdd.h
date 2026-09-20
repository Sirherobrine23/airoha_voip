#ifndef _DRV_DXS_FW_CMD_SDD_H_
#define _DRV_DXS_FW_CMD_SDD_H_
/******************************************************************************

                            Copyright (c) 2014, 2016
                        Lantiq Beteiligungs-GmbH & Co.KG
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_fw_cmd_sdd.h
   This file contains the SDD command messages.
*/

#include "drv_dxs_fw_headers.h"
#include <drv_tapi_osmap.h>

/** Data structure for SDD opmode firmware command */
typedef struct DXS_SDD_Opmode
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /** Reserved */
   uint32_t Res02 : 8;
   /** Select Operating Mode */
   uint32_t OpMode : 8;
   /** Reserved */
   uint32_t Res03 : 16;
#else
   CMD_HEAD_LE;
   /** Reserved */
   uint32_t Res03 : 16;
   /** Select Operating Mode */
   uint32_t OpMode : 8;
   /** Reserved */
   uint32_t Res02 : 8;
#endif
} __PACKED__ DXS_SDD_Opmode_t;
#define DXS_SDD_Opmode_ECMD    7
#define DXS_SDD_Opmode_LENGTH  4
#define DXS_SDD_Opmode_Disabled        0
#define DXS_SDD_Opmode_RingBurst       1
#define DXS_SDD_Opmode_Standby         2
#define DXS_SDD_Opmode_Active          3
#define DXS_SDD_Opmode_GroundStart     5
#define DXS_SDD_Opmode_Calibrate       6
#define DXS_SDD_Opmode_GR909           7
#define DXS_SDD_Opmode_MWI             12
#define DXS_SDD_Opmode_Howler          13
#define DXS_SDD_Opmode_RingRevpol      17
#define DXS_SDD_Opmode_ActiveRevpol    19
#define DXS_SDD_Opmode_GroundStart_T2G 21
#define DXS_SDD_Opmode_CapMeas         23
#define DXS_SDD_Opmode_HowlerRevpol    29


/** Data structure for SDD gain config firmware command */
typedef struct
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /** Tx Relative Level Setting */
   uint32_t TxGain : 16;
   /** Rx Relative Level setting */
   uint32_t RxGain : 16;
#else
   CMD_HEAD_LE;
   /** Rx Relative Level setting */
   uint32_t RxGain : 16;
   /** Tx Relative Level Setting */
   uint32_t TxGain : 16;
#endif
} __PACKED__ DXS_SDD_TxRxGain_t;
#define DXS_SDD_TxRxGain_ECMD   14
#define DXS_SDD_TxRxGain_LENGTH 4

#define DXS_CALIBRATION_VERSION  1

/** Data structure for calibration data */
typedef struct
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /** TX Path Offset */
   uint32_t TxOffset : 16;
   /** IDAC Gain Correction */
   uint32_t IdacGain : 16;
   /** Ring Current Offset Compensation */
   uint32_t RingOffset : 16;
   /** Reserved */
   uint32_t Res02 : 16;
#else
   CMD_HEAD_LE;
   /** IDAC Gain Correction */
   uint32_t IdacGain : 16;
   /** TX Path Offset */
   uint32_t TxOffset : 16;
   /** Reserved */
   uint32_t Res02 : 16;
   /** Ring Current Offset Compensation */
   uint32_t RingOffset : 16;
#endif
} __PACKED__ DXS_SDD_Calibrate_t;
#define DXS_SDD_Calibrate_ECMD         3
#define DXS_SDD_Calibrate_LENGTH       8


/** Data structure for capacitance measurement result firmware command */
typedef struct
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /** Capacitance Ring to Ground */
   uint32_t CapR2G : 16;
   /** Capacitance Tip to Ground */
   uint32_t CapT2G : 16;
   /** Capacitance Tip to Ring */
   uint32_t CapT2R : 16;
   /** Reserved */
   uint32_t Res02 : 16;
#else
   CMD_HEAD_LE;
   /** Capacitance Tip to Ground */
   uint32_t CapT2G : 16;
   /** Capacitance Ring to Ground */
   uint32_t CapR2G : 16;
   /** Reserved */
   uint32_t Res02 : 16;
   /** Capacitance Tip to Ring */
   uint32_t CapT2R : 16;
#endif
} __PACKED__ DXS_SDD_CapMeasRead_t;
#define DXS_SDD_CapMeasRead_ECMD     16
#define DXS_SDD_CapMeasRead_LENGTH    8

/** Data structure for continuous measurement result firmware command */
typedef struct
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /** Line Voltage */
   uint32_t Vline : 16;
   /** Line Current */
   uint32_t Itrans : 16;
   /** RING Current */
   uint32_t Iring : 16;
   /** RING Voltage */
   uint32_t Vring : 16;
   /** Reserved */
   uint32_t Res02 : 32;
#else
   CMD_HEAD_LE;
   /** Line Current */
   uint32_t Itrans : 16;
   /** Line Voltage */
   uint32_t Vline : 16;
   /** RING Voltage */
   uint32_t Vring : 16;
   /** RING Current */
   uint32_t Iring : 16;
   /** Reserved */
   uint32_t Res02 : 32;
#endif
} __PACKED__ DXS_SDD_ContMeasRead_t;
#define DXS_SDD_ContMeasRead_ECMD      2
#define DXS_SDD_ContMeasRead_LENGTH   12

/** Data structure for GR-909 limits configuration firmware command */
typedef struct
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /** Reserved */
   uint32_t Res02 : 11;
   /** Settle Time */
   uint32_t SettleTime : 5;
   /** HPT Wire to GND AC Limit */
   uint32_t HptW2gAcLim : 16;
   /** HPT Wire to Wire AC Limit */
   uint32_t HptW2wAcLim : 16;
   /** HPT Wire to GND DC Limit */
   uint32_t HptW2gDcLim : 16;
   /** HPT Wire to Wire DC Limit */
   uint32_t HptW2wDcLim : 16;
   /** FEMF Wire to GND AC Limit */
   uint32_t FemfW2gAcLim : 16;
   /** FEMF Wire to Wire AC Limit */
   uint32_t FemfW2wAcLim : 16;
   /** FEMF Wire to GND DC Limit */
   uint32_t FemfW2gDcLim : 16;
   /** FEMF Wire to Wire DC Limit */
   uint32_t FemfW2wDcLim : 16;
   /** RFT Resistance Limit */
   uint32_t RftResLim : 16;
   /** ROH Linearity Limit */
   uint32_t RohLinLim : 16;
   /** RIT Lower Limit */
   uint32_t RitLowLim : 16;
   /** RIT Higher Limit */
   uint32_t RitHighLim : 16;
   /** Reserved */
   uint32_t Res03 : 16;
#else
   CMD_HEAD_LE;
   /** HPT Wire to GND AC Limit */
   uint32_t HptW2gAcLim : 16;
   /** Settle Time */
   uint32_t SettleTime : 5;
   /** Reserved */
   uint32_t Res02 : 11;
   /** HPT Wire to GND DC Limit */
   uint32_t HptW2gDcLim : 16;
   /** HPT Wire to Wire AC Limit */
   uint32_t HptW2wAcLim  : 16;
   /** FEMF Wire to GND AC Limit */
   uint32_t FemfW2gAcLim : 16;
   /** HPT Wire to Wire DC Limit */
   uint32_t HptW2wDcLim : 16;
   /** FEMF Wire to GND DC Limit */
   uint32_t FemfW2gDcLim : 16;
   /** FEMF Wire to GND DC Limit */
   uint32_t FemfW2wAcLim : 16;
   /** RFT Resistance Limit */
   uint32_t RftResLim : 16;
   /** FEMF Wire to Wire DC Limit */
   uint32_t FemfW2wDcLim : 16;
   /** RIT Lower Limit */
   uint32_t RitLowLim : 16;
   /** ROH Linearity Limit */
   uint32_t RohLinLim : 16;
   /** Reserved */
   uint32_t Res03 : 16;
   /** RIT Higher Limit */
   uint32_t RitHighLim : 16;
#endif
} __PACKED__ DXS_SDD_GR909Config_t;
#define DXS_SDD_GR909Config_ECMD      8
#define DXS_SDD_GR909Config_LENGTH   28

/** Data structure for GR-909 result firmware command */
typedef struct
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /** HPT Test Valid */
   uint32_t HptValid : 1;
   /** FEMF Test Valid */
   uint32_t FemfValid : 1;
   /** RFT Test Valid */
   uint32_t RftValid : 1;
   /** ROH Test Valid */
   uint32_t RohValid : 1;
   /** RIT Test Valid */
   uint32_t RitValid : 1;
   /** Reserved */
   uint32_t Res02 : 11;
   /** HPT Test Passed */
   uint32_t HptPass : 1;
   /** FEMF Test Passed */
   uint32_t FemfPass : 1;
   /** RFT Test Passed */
   uint32_t RftPass : 1;
   /** ROH Test Passed */
   uint32_t RohPass : 1;
   /** RIT Test Passed */
   uint32_t RitPass : 1;
   /** Reserved */
   uint32_t Res03 : 11;
   /** Test Result HPT or FEMF AC Ring to GND */
   uint32_t HptAcR2g : 16;
   /** Test Result HPT or FEMF AC Tip to GND */
   uint32_t HptAcT2g : 16;
   /** Test Result HPT or FEMF AC Tip to Ring */
   uint32_t HptAcT2r : 16;
   /** Test Result HPT or FEMF DC Ring to GND */
   uint32_t HptDcR2g : 16;
   /** Test Result HPT or FEMF DC Tip to GND */
   uint32_t HptDcT2g : 16;
   /** Test Result HPT or FEMF DC Tip to Ring */
   uint32_t HptDcT2r : 16;
   /** Resistive Fault Ring to Ground Result */
   uint32_t RftR2g : 16;
   /** Resistive Fault Tip to Ground Result */
   uint32_t RftT2g : 16;
   /** Resistive Fault Tip to Ring Result */
   uint32_t RftT2r : 16;
   /** Test Result ROH Low */
   uint32_t RohLow : 16;
   /** Test Result ROH High */
   uint32_t RohHigh : 16;
   /** Test Result RIT */
   uint32_t RitRes : 16;
#else
   CMD_HEAD_LE;
   /** Reserved */
   uint32_t Res03: 11;
   /** RIT Test Passed */
   uint32_t RitPass : 1;
   /** ROH Test Passed */
   uint32_t RohPass : 1;
   /** RFT Test Passed */
   uint32_t RftPass : 1;
   /** FEMF Test Passed */
   uint32_t FemfPass : 1;
   /** HPT Test Passed */
   uint32_t HptPass : 1;
   /** Reserved */
   uint32_t Res02 : 11;
   /** RIT Test Valid */
   uint32_t RitValid : 1;
   /** ROH Test Valid */
   uint32_t RohValid : 1;
   /** RFT Test Valid */
   uint32_t RftValid : 1;
   /** FEMF Test Valid */
   uint32_t FemfValid : 1;
   /** HPT Test Valid */
   uint32_t HptValid : 1;
   /** Test Result HPT or FEMF AC Tip to GND */
   uint32_t HptAcT2g : 16;
   /** Test Result HPT or FEMF AC Ring to GND */
   uint32_t HptAcR2g : 16;
   /** Test Result HPT or FEMF DC Ring to GND */
   uint32_t HptDcR2g : 16;
   /** Test Result HPT or FEMF AC Tip to Ring */
   uint32_t HptAcT2r : 16;
   /** Test Result HPT or FEMF DC Tip to Ring */
   uint32_t HptDcT2r : 16;
   /** Test Result HPT or FEMF DC Tip to GND */
   uint32_t HptDcT2g : 16;
   /** Resistive Fault Tip to Ground Result */
   uint32_t RftT2g : 16;
   /** Resistive Fault Ring to Ground Result */
   uint32_t RftR2g : 16;
   /** Test Result ROH Low */
   uint32_t RohLow : 16;
   /** Resistive Fault Tip to Ring Result */
   uint32_t RftT2r : 16;
   /** Test Result RIT */
   uint32_t RitRes : 16;
   /** Test Result ROH High */
   uint32_t RohHigh : 16;
#endif
} __PACKED__ DXS_SDD_GR909Result_t;
#define DXS_SDD_GR909Result_ECMD      9
#define DXS_SDD_GR909Result_LENGTH   28

/** This structure contains data specific to BBD basic config fw command. */
typedef struct DXS_SDD_BasicConfig
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /* Reserved */
   uint32_t  Res01 : 4;
   /* DUP Time for Hook Debouncing in ACT Mode */
   uint32_t ActiveDup : 4;
   /* DUP Time for Hook Debouncing in GS Mode */
   uint32_t GsDup : 4;
   /* Reserved */
   uint32_t Res02 : 4;
   /* DUP Time for Ground Key Debouncing */
   uint32_t GndkDup : 4;
   /* Emergency Shut Down Debounce Time */
   uint32_t EsdDup : 4;
   /* Reserved */
   uint32_t Res03 : 2;
   /* Automatic Sense Bias Enable */
   uint32_t AutoBiasEn : 1;
   /* Reserved */
   uint32_t Res04 : 5;
   /* Combined Low Power Offhook Voltage */
   uint32_t CLPOffhook : 4;
   /* DC/DC Overhead Voltage */
   uint32_t DcDcOvh : 4;
   /* Combined Low Power Overhead Voltage */
   uint32_t CLPOvh : 5;
   /* Standby Voltage */
   uint32_t StbyVolt : 2;
   /* Reserved */
   uint32_t Res07 : 1;
   /*  TTX Burst Length */
   uint32_t TtxBurstLength : 16;
   /* Current gain for PID regulator */
   uint32_t DcPidGain : 16;
   /* On-Hook Threshold in ACT Mode */
   uint32_t ActOnhookThresh : 16;
   /* Off-Hook Threshold in ACT Mode */
   uint32_t ActOffhookThresh : 16;
   /* Reserved */
   uint32_t Res08 : 16;
   /* Open Loop Voltage Limit */
   uint32_t VoltageLimit : 16;
   /* Closed Loop Current Limit */
   uint32_t CurrentLimit : 16;
#else
   CMD_HEAD_LE;
   /* Reserved */
   uint32_t Res04 : 5;
   /* Automatic Sense Bias Enable */
   uint32_t AutoBiasEn : 1;
   /* Reserved */
   uint32_t Res03 : 2;
   /* Emergency Shut Down Debounce Time */
   uint32_t EsdDup : 4;
   /* DUP Time for Ground Key Debouncing */
   uint32_t GndkDup : 4;
   /* Reserved */
   uint32_t Res02 : 4;
   /* DUP Time for Hook Debouncing in GS Mode */
   uint32_t GsDup : 4;
   /* DUP Time for Hook Debouncing in ACT Mode */
   uint32_t ActiveDup : 4;
   /* Reserved */
   uint32_t  Res01 : 4;
   /*  TTX Burst Length */
   uint32_t TtxBurstLength : 16;
   /* Reserved */
   uint32_t Res07 : 1;
   /* Standby Voltage */
   uint32_t StbyVolt : 2;
   /* Combined Low Power Overhead Voltage */
   uint32_t CLPOvh : 5;
   /* DC/DC Overhead Voltage */
   uint32_t DcDcOvh : 4;
   /* Combined Low Power Offhook Voltage */
   uint32_t CLPOffhook : 4;
   /* On-Hook Threshold in ACT Mode */
   uint32_t ActOnhookThresh : 16;
   /* Current gain for PID regulator */
   uint32_t DcPidGain : 16;
   /* Reserved */
   uint32_t Res08 : 16;
   /* Off-Hook Threshold in ACT Mode */
   uint32_t ActOffhookThresh : 16;
   /* Closed Loop Current Limit */
   uint32_t CurrentLimit : 16;
   /* Open Loop Voltage Limit */
   uint32_t VoltageLimit : 16;
#endif
} __PACKED__ DXS_SDD_BasicConfig_t;
#define DXS_SDD_BasicConfig_ECMD     4
#define DXS_SDD_BasicConfig_LENGTH   20


/** This structure contains data specific to ring config firmware command. */
typedef struct DXS_SDD_RingConfig
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /* Reserved */
   uint32_t Res02 : 2;
   /* Ring Trip Type */
   uint32_t RingTripType : 2;
   /* Wave Form of Ringing Signal */
   uint32_t WaveForm : 1;
   /* Ring Crest Factor */
   uint32_t CrestFact : 3;
   /* Reserved */
   uint32_t Res03 : 8;
   /* Ringing Frequency */
   uint32_t Frequency : 16;
   /* Ringing Amplitude */
   uint32_t Amplitude : 16;
   /* Ringing Hook Threshold */
   uint32_t Thresh : 16;
   /* Ringing DC Offset */
   uint32_t DcOffset : 16;
   /* Maximum Ring Current */
   uint32_t Imax : 16;
   /* Ringing Regulation Coefficient */
   uint32_t RegCoeff : 16;
   /* Minimum Ringing Voltage (peak) */
   uint32_t Vmin : 16;
   /* Reserved */
   uint32_t Res04 : 16;
   /* Fast Hook Threshold */
   uint32_t FastThresh : 16;
#else
   CMD_HEAD_LE;
   /* Ringing Frequency */
   uint32_t Frequency : 16;
   /* Reserved */
   uint32_t Res03 : 8;
   /* Ring Crest Factor */
   uint32_t CrestFact : 3;
   /* Wave Form of Ringing Signal */
   uint32_t WaveForm : 1;
   /* Ring Trip Type */
   uint32_t RingTripType : 2;
   /* Reserved */
   uint32_t Res02 : 2;
   /* Ringing Hook Threshold */
   uint32_t Thresh : 16;
   /* Ringing Amplitude */
   uint32_t Amplitude : 16;
   /* Maximum Ring Current */
   uint32_t Imax : 16;
   /* Ringing DC Offset */
   uint32_t DcOffset : 16;
   /* Minimum Ringing Voltage (peak) */
   uint32_t Vmin : 16;
   /* Ringing Regulation Coefficient */
   uint32_t RegCoeff : 16;
   /* Fast Hook Threshold */
   uint32_t FastThresh : 16;
   /* Reserved */
   uint32_t Res04 : 16;
#endif
} __PACKED__ DXS_SDD_RingConfig_t;
#define DXS_SDD_RingConfig_ECMD          5
#define DXS_SDD_RingConfig_LENGTH        20


/** This structure contains data specific to DC/DC config firmware command. */
typedef struct DXS_SDD_DcDcConfig
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /* DCDC Offtime Coefficient */
   uint32_t DcDcOffC : 4;
   /* DC/DC Capacitance */
   uint32_t DcDcCap : 4;
   /* DC/DC Hardware Option */
   uint32_t DcDcHw : 4;
   /* Reserved */
   uint32_t RingVN : 1;
   /* DC/DC Switch Output Enable */
   uint32_t DcDcSwEn : 1;
   /* Combined Low Power Mode */
   uint32_t CombLP : 1;
   /* DC/DC Switch Output Inverted */
   uint32_t DcDcSwInv : 1;
   /* Power Value for Boost Calculation */
   uint32_t P1: 16;
   /* DC/DC Maximum On Time of Switching Transistor */
   uint32_t MaxOnTime : 8;
   /* Maximum Frequency of DC/DC Converter */
   uint32_t MaxFreq  : 8;
   /* DC/DC On Time of Switching Transistor in STANDBY Mode */
   uint32_t OnTimeStby : 8;
   /* Maximum Frequency of DC/DC Converter in STANDBY Mode */
   uint32_t MaxFreqStby : 8;
#else
   CMD_HEAD_LE;
   /* Power Value for Boost Calculation */
   uint32_t P1: 16;
   /* DC/DC Switch Output Inverted */
   uint32_t DcDcSwInv : 1;
   /* Combined Low Power Mode */
   uint32_t CombLP : 1;
   /* DC/DC Switch Output Enable */
   uint32_t DcDcSwEn : 1;
   /* Reserved */
   uint32_t RingVN : 1;
   /* DC/DC Hardware Option */
   uint32_t DcDcHw : 4;
   /* DC/DC Capacitance */
   uint32_t DcDcCap : 4;
   /* DCDC Offtime Coefficient */
   uint32_t DcDcOffC : 4;
   /* Maximum Frequency of DC/DC Converter in STANDBY Mode */
   uint32_t MaxFreqStby : 8;
   /* DC/DC On Time of Switching Transistor in STANDBY Mode */
   uint32_t OnTimeStby : 8;
   /* Maximum Frequency of DC/DC Converter */
   uint32_t MaxFreq  : 8;
   /* DC/DC Maximum On Time of Switching Transistor */
   uint32_t MaxOnTime : 8;
#endif
} __PACKED__ DXS_SDD_DcDcConfig_t;
#define DXS_SDD_DcDcConfig_ECMD          12
#define DXS_SDD_DcDcConfig_LENGTH        8

/** This structure contains data specific to MWL config firmware command. */
typedef struct DXS_SDD_MwlConfig
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /* Message Waiting Lamp Voltage */
   uint32_t Voltage : 16;
   /* Message Waiting Hook Threshold */
   uint32_t Thresh : 16;
   /* Message Waiting Slope */
   uint32_t Slope : 16;
   /* Message Waiting On-time (Lamp ON) */
   uint32_t OnTime : 8;
   /* Message Waiting Off-time (Lamp OFF) */
   uint32_t OffTime :8;
#else
   CMD_HEAD_LE;
   /* Message Waiting Hook Threshold */
   uint32_t Thresh : 16;
   /* Message Waiting Lamp Voltage */
   uint32_t Voltage : 16;
   /* Message Waiting Off-time (Lamp OFF) */
   uint32_t OffTime :8;
   /* Message Waiting On-time (Lamp ON) */
   uint32_t OnTime : 8;
   /* Message Waiting Slope */
   uint32_t Slope : 16;
#endif
} __PACKED__ DXS_SDD_MwlConfig_t;
#define DXS_SDD_MwlConfig_ECMD          6
#define DXS_SDD_MwlConfig_LENGTH        8

/* The SDD_Coeff message writes or reads
   programmable coefficients. */
typedef struct DXS_SDD_Coeff
{
   struct DXS_FW_Cmd_Header  hdr;
   IFX_uint32_t   data[7] __PACKED__;
} DXS_SDD_Coeff_t;
#define DXS_SDD_Coeff_ECMD        13
#define DXS_SDD_Coeff_MAXLENGTH   28
#define DXS_SDD_Coeff_Dest_ACSW   0
#define DXS_SDD_Coeff_Dest_ACDC   1

/** SDD_En8kLoop */
typedef struct DXS_SDD_En8kLoop
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
    CMD_HEAD_BE;
    /** Reserved */
    uint32_t Res01 : 31;
    /** Enable the 8k Testloop in the ASDSP */
    uint32_t En8kLoop : 1;
#else
   CMD_HEAD_LE;
   /** Enable the 8k Testloop in the ASDSP */
   uint32_t En8kLoop : 1;
   /** Reserved */
   uint32_t Res01 : 31;
#endif
} __PACKED__ DXS_SDD_En8kLoop_t;
#define DXS_SDD_En8kLoop_CMD                 0x01
#define DXS_SDD_En8kLoop_MOD                 0x00
#define DXS_SDD_En8kLoop_ECMD                30
#define DXS_SDD_En8kLoop_LEN                 4
#define DXS_SDD_En8kLoop_En8kLoop_DISABLE    0x00
#define DXS_SDD_En8kLoop_En8kLoop_ENABLE     0x01


/** Data structure for SDD_ACLevelMeterControl firmware command. */
typedef struct
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /** Enable: 0=stop, 1=start */
   uint32_t EN : 1;
   /** Reserved */
   uint32_t Res31 : 31;
#else
   CMD_HEAD_LE;
   /** Reserved */
   uint32_t Res31 : 31;
   /** Enable: 0=stop, 1=start */
   uint32_t EN : 1;
#endif
} __PACKED__ DXS_SDD_ACLevelMeterControl_t;
#define DXS_SDD_ACLevelMeterControl_ECMD     17
#define DXS_SDD_ACLevelMeterControl_LENGTH   4


/** Data structure for SDD_ACLevelMeterConfig firmware command. */
typedef struct
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /** Bandwidth Coefficient */
   uint32_t BW : 16;
   /** Center Frequency Coefficient */
   uint32_t CF : 16;
   /** Reserved */
   uint32_t Res02 : 14;
   /** Bandpass enable: 0=disable, 1=enable */
   uint32_t BP : 1;
   /** Transhybrid enable: 0=disable, 1=enable */
   uint32_t TH : 1;
   /** AC Delay [ms] */
   uint32_t Del : 7;
   /** AC Integration Time [ms] */
   uint32_t Int : 9;
#else
   CMD_HEAD_LE;
   /** Center Frequency Coefficient */
   uint32_t CF : 16;
   /** Bandwidth Coefficient */
   uint32_t BW : 16;
   /** AC Integration Time [ms] */
   uint32_t Int : 9;
   /** AC Delay [ms] */
   uint32_t Del : 7;
   /** Transhybrid enable: 0=disable, 1=enable */
   uint32_t TH : 1;
   /** Bandpass enable: 0=disable, 1=enable */
   uint32_t BP : 1;
   /** Reserved */
   uint32_t Res02 : 14;
#endif
} __PACKED__ DXS_SDD_ACLevelMeterConfig_t;
#define DXS_SDD_ACLevelMeterConfig_ECMD     18
#define DXS_SDD_ACLevelMeterConfig_LENGTH   8


/** The SDD_ACLevelMeterResult message carries the AC level meter
    measurement result. */
typedef struct
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /** AC Levelmeter Inband Result */
   uint32_t AcInb : 24;
   /** AC Levelmeter Inband Result Shift (signed) */
   int32_t AcInbSh : 8;
   /** AC Levelmeter Outband Result */
   uint32_t AcOutb : 24;
   /** AC Levelmeter Outband Result Shift (signed) */
   int32_t AcOutbSh : 8;
#else
   CMD_HEAD_LE;
   /** AC Levelmeter Inband Result Shift */
   int32_t AcInbSh : 8;
   /** AC Levelmeter Inband Result */
   uint32_t AcInb : 24;
   /** AC Levelmeter Outband Result Shift */
   int32_t AcOutbSh : 8;
   /** AC Levelmeter Outband Result */
   uint32_t AcOutb : 24;
#endif
} __PACKED__ DXS_SDD_ACLevelMeterResult_t;
#define DXS_SDD_ACLevelMeterResult_ECMD    19
#define DXS_SDD_ACLevelMeterResult_LENGTH  8

#endif /* _DRV_DXS_FW_CMD_SDD_H_ */
