#ifndef _DRV_TAPI_ERRNO_H
#define _DRV_TAPI_ERRNO_H

/******************************************************************************

  Copyright 2014 Lantiq Deutschland GmbH
  Copyright 2023 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_errno.h  TAPI error code values.
*/

#ifdef __cplusplus
   extern "C" {
#endif

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_tapi_io.h"

/* ============================= */
/* Error code symbol list        */
/* ============================= */

/*Errorblock*/
/** Enumeration for function return status.
    The upper four bits are reserved for error classification. */
typedef enum
{
   TAPI_statusOk = TAPI_statusClassSuccess,
   /** Generic or unknown error occured */
   TAPI_statusErr = TAPI_statusClassErr,
   /** Setting line mode failed */
   TAPI_statusLineModeFail = TAPI_statusClassErr | 0x1,
   /** Starting CID Info failed */
   TAPI_statusCidInfoStartFail,
   /** Invalid parameter */
   TAPI_statusParam,
   /** Invalid ioctl call */
   TAPI_statusInvalidIoctl,
   /** Unknown or unsupported ioctl call */
   TAPI_statusNoIoctl,
   /** Desired action is not supported */
   TAPI_statusNotSupported,
   /** Service is not supported by the low level driver */
   TAPI_statusLLNotSupp,
   /** LL driver init failed */
   TAPI_statusInitLL,
   /** LL driver already registered */
   TAPI_statusInitLLReg,
   /** LL driver registration - incompatible HL/LL version */
   TAPI_statusInitLLRegVersMismatch,
   /** LL driver registration info can not be copied */
   TAPI_statusInitLLRegCp,
   /** LL driver with that major number already exists */
   TAPI_statusRegLLExists,
   /** Device structure allocation failed */
   TAPI_statusDevAlloc,
   /** No tone resource found or error in finding it for playing the tone */
   TAPI_statusToneNoRes,
   /** Failed to configure predefined tone table */
   TAPI_statusTonePredefFail,
   /** Tone is already playing */
   TAPI_statusTonePlayAct,
   /** Invalid resource for tone service specified */
   TAPI_statusInvalidToneRes,
   /** Ringing is already active */
   TAPI_statusRingAct,
   /** Ringing cadence not correct */
   TAPI_statusRingCad,
   /** No ring timer available */
   TAPI_statusRingTimer,
   /** Ringing can not be stopped */
   TAPI_statusRingStop,
   /** Ringing can not be configured */
   TAPI_statusRingCfg,
   /** Metering already active */
   TAPI_statusMeterAct,
   /** Line not in active mode */
   TAPI_statusLineNotAct,
   /** Phone in off hook state */
   TAPI_statusPhoneOffHook,
   /** Service called with wrong context fd */
   TAPI_statusCtxErr,
   /** Service not supported on called channel context */
   TAPI_statusInvalidCh,
   /** Copy KERN2USR or USR2KERN failed */
   TAPI_statusErrKernCpy,
   /** General initialization failed */
   TAPI_statusInitFail,
   /** Ringing initialization failed */
   TAPI_statusInitRingFail,
   /** Dialing initialization failed */
   TAPI_statusInitDialFail,
   /** Metering initialization failed */
   TAPI_statusInitMeterFail,
   /** CID initialization failed */
   TAPI_statusInitCIDFail,
   /** Tone initialization failed */
   TAPI_statusInitToneFail,
   /** Reference to unconfigured tone code entry  */
   TAPI_statusToneNotAvail,
   /** Not enough spac in CID data buffer */
   TAPI_statusCIDBuffNoMem,
   /** Wrong CID configuration */
   TAPI_statusCIDWrongConf,
   /** Unsuitable line mode for CID Tx */
   TAPI_statusCIDNoLineModeOk,
   /** CID Tx already active */
   TAPI_statusCIDActive,
   /** CID hook state mismatch */
   TAPI_statusCIDHook,
   /** Can not stop CID Tx */
   TAPI_statusCIDStopTx,
   /** Can not start CID Rx */
   TAPI_statusCIDStartRx,
   /** Can not stop CID Rx */
   TAPI_statusCIDStopRx,
   /** CID data not copied */
   TAPI_statusCIDCopy,
   /** Failed to enable COD AGC */
   TAPI_statusCODAGCEn,
   /** PCM mode unknown */
   TAPI_statusPCMUnknownMode,
   /** PCM activation failed due to LL error */
   TAPI_statusPCMActivation,
   /** PCM configuration failed due to LL error */
   TAPI_statusPCMConf,
   /** Phone channel not available */
   TAPI_statusPhoneNo,
   /** Data channel not available */
   TAPI_statusDataNo,
   /** Stopping tone failed */
   TAPI_statusToneStop,
   /** RTP payload table configuration failed */
   TAPI_statusRTPConf,
   /** MFTD enable failed */
   TAPI_statusMFTDEnFail,
   /** MFTD disabled failed */
   TAPI_statusMFTDDisFail,
   /** Playing tone in LL driver failed */
   TAPI_statusTonePlayLLFailed,
   /** LL driver returned an error */
   TAPI_statusLLFailed,
   /** Memory not available */
   TAPI_statusNoMem,
   /** FIFO size or FIFO initialization error */
   TAPI_statusFIFO,
   /** Event not known or not handled */
   TAPI_statusEvtNoHandle,
   /** TAPI_DeferWork schedule_work failed */
   TAPI_statusWorkFail,
   /** Timer creation failed */
   TAPI_statusTimerFail,
   /** Ringing was not initialized */
   TAPI_statusRingInit,
   /** Event fifo queue full */
   TAPI_statusEvtFifoFull,
   /** Device initialization failed */
   TAPI_statusInitDevFail,
   /** DTMF receiver configuration failed in LL driver */
   TAPI_statusDtmfRxCfg,
   /** Starting CID state machine failed*/
   TAPI_statusCidSmStart,
   /** Device not initialized */
   TAPI_statusUninitializedDev,
   /** Failed to enable CPTD */
   TAPI_statusCPTDEnFail,
   /** Failed to disable CPTD */
   TAPI_statusCPTDDisFail,
   /** IOCTL is blocked until the device is started */
   TAPI_statusIoctlBlocked,
   /** Not enough space in buffer for DATE element */
   TAPI_statusCIDBuffNoSpaceForDATE,
   /** Not enough space in buffer for element */
   TAPI_statusCIDBuffNoSpaceForElement,
   /** Not enough space in buffer for STRING element */
   TAPI_statusCIDBuffNoSpaceForSTRING,
   /** Not enough space in buffer for CLI element */
   TAPI_statusCIDBuffNoSpaceForCLI,
   /** Not enough space in buffer for NAME element */
   TAPI_statusCIDBuffNoSpaceForNAME,
   /** No valid data for NTT CID, mandatory elements missing */
   TAPI_statusCIDBuffMandatoryElementMissing,
   /** Wrong size for TRANSPARENT element or
       TRANSPARENT together with other elements types */
   TAPI_statusCIDBuffIncorrectTRANSPARENT,
   /** Configuration of predefined CID alert tone failed */
   TAPI_statusCIDConfAlertToneFailed,
   /** Hookstate not matching with transmission mode */
   TAPI_statusCIDHookMissmatch,
   /** Line mode not suitable for CID sequence transmission */
   TAPI_statusCIDLineModeNotSuitable,
   /** No phone is connected on the data channel main input */
   TAPI_statusCIDNoPhoneAtDataCh,
   /** No data available in CID receiver buffer */
   TAPI_statusCIDRXNoDataAvailable,
   /** Channel structure allocation failed */
   TAPI_statusChAlloc,
   /** Failed to start metering */
   TAPI_statusMeterBurstFail,
   /** PCM interface configuration failed */
   TAPI_statusPCMIfConfError,
   /** PCM channel configuration failed */
   TAPI_statusPCMChCfgError,
   /** PCM channel not configured */
   TAPI_statusPCMChNoCfg,
   /** FW starting failed */
   TAPI_statusFWStart,
   /** BBD downoading failed */
   TAPI_statusBbdDownload,
   /** Invalid character passed in DTMF CID message element */
   TAPI_statusCIDInvalCharInDtmfMsgElem,
   /** Unsuitable line mode for ringing */
   TAPI_statusRingLineModeNotOk,
   /** Tone index out of range */
   TAPI_statusToneIdxOutOfRange,
   /** Simple tone on-time time exceeds the maximum range */
   TAPI_statusToneOnTimeOutOfRange,
   /** Simple tone time of first step may not be zero */
   TAPI_statusToneInitialTimeZero,
   /** Simple tone pause time exceeds the maximum supported pause time */
   TAPI_statusTonePauseTimeOutOfRange,
   /** Simple tone frequency exceeds the maximum supported frequency */
   TAPI_statusToneFrequencyOutOfRange,
   /** Simple tone power level exceeds the supported range */
   TAPI_statusTonePowerLevelOutOfRange,
   /** Composed tone max alternate voice time exceeds the maximum */
   TAPI_statusToneAltVoiceTimeOutOfRange,
   /** Composed tone single repetition not allowed when
       alternate voice time is non-zero */
   TAPI_statusToneLoopCountInvalid,
   /** Composed tone number of simple tone fields is out of range */
   TAPI_statusToneCountOutOfRange,
   /** Composed tone contains a simple tone index that is out of range */
   TAPI_statusToneSimpleIdxOutOfRange,
   /** Composed tone contains a simple tone that is not configured */
   TAPI_statusToneSimpleToneUnconfigured,
   /** LL driver was not registered because devNodeName is not set */
   TAPI_statusLLDevNodeName,
   /** Daa was not initialized */
   TAPI_statusInitDaa,
   /** QOS: Service not supported by QOS driver - IOCTL failed */
   TAPI_statusQosServiceNotSupported,
   /** PPD initialization failed */
   TAPI_statusInitPpdFail,
   /** PPD state machine failed */
   TAPI_statusInitPpdStateMachineFailed,
   /** No QOS driver registered - IOCTL aborted */
   TAPI_statusNoQosRegistered,
   /** QOS start failed */
   TAPI_statusQosStartFailed,
   /** QOS start on existing socket failed */
   TAPI_statusQosStartFdFailed,
   /** QOS stop failed */
   TAPI_statusQosStopFailed,
   /** QOS cleanup failed */
   TAPI_statusQosCleanupFailed,
   /** Combination of directions for DTMF digit and DTMF end
       detection is not possible. */
   TAPI_statusDtmfRxDirectionCombination,
   /** Configured cadence pause time not sufficient for CID data
       to be transmitted. */
   TAPI_statusCIDRingPauseTooShort,
   /** Registration to PMCU failed */
   TAPI_statusRegisterToPmcuFailed,
   /** Device not added to proc fs */
   TAPI_statusDeviceNotAddedToProcFs,
   /** Cannot set linefeeding different than disabled while a line
       fault is present. */
   TAPI_statusLineFault,
   /** Capacitance measurement is already active  */
   TAPI_statusCapMeasStartWhileActive,
   /** Value zero not allowed as hook state validation time */
   TAPI_statusDialZeroParam,
   /** Max must be larger than min hook state validation time */
   TAPI_statusDialMaxMinParam,
   /** Unknown hook state validation type parameter */
   TAPI_statusDialTypeParam,
   /** Event cannot be disable, event always enabled */
   TAPI_statusEvtNotDisabled,
   /* ++ insert new error codes here ++ */
} TAPI_Status_t;

#ifdef __cplusplus
   }
#endif

#endif /* _DRV_TAPI_ERRNO_H */
