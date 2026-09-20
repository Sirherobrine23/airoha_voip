#ifndef _DRV_TAPI_STRERRNO_H
#define _DRV_TAPI_STRERRNO_H
/******************************************************************************

  Copyright 2014 Lantiq Deutschland GmbH
  Copyright 2023-2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_strerrno.h  Contains the error codes and textual descriptions
   for use by the application which uses TAPI.
*/

#ifdef __cplusplus
   extern "C" {
#endif

#define TAPI_ERRNO_CNT 123

static const IFX_uint32_t TAPI_drvErrnos[TAPI_ERRNO_CNT] =
{
   /* TAPI_statusErr */
   0x6000,
   /* TAPI_statusLineModeFail */
   0x6001,
   /* TAPI_statusCidInfoStartFail */
   0x6002,
   /* TAPI_statusParam */
   0x6003,
   /* TAPI_statusInvalidIoctl */
   0x6004,
   /* TAPI_statusNoIoctl */
   0x6005,
   /* TAPI_statusNotSupported */
   0x6006,
   /* TAPI_statusLLNotSupp */
   0x6007,
   /* TAPI_statusInitLL */
   0x6008,
   /* TAPI_statusInitLLReg */
   0x6009,
   /* TAPI_statusInitLLRegVersMismatch */
   0x600a,
   /* TAPI_statusInitLLRegCp */
   0x600b,
   /* TAPI_statusRegLLExists */
   0x600c,
   /* TAPI_statusDevAlloc */
   0x600d,
   /* TAPI_statusToneNoRes */
   0x600e,
   /* TAPI_statusTonePredefFail */
   0x600f,
   /* TAPI_statusTonePlayAct */
   0x6010,
   /* TAPI_statusInvalidToneRes */
   0x6011,
   /* TAPI_statusRingAct */
   0x6012,
   /* TAPI_statusRingCad */
   0x6013,
   /* TAPI_statusRingTimer */
   0x6014,
   /* TAPI_statusRingStop */
   0x6015,
   /* TAPI_statusRingCfg */
   0x6016,
   /* TAPI_statusMeterAct */
   0x6017,
   /* TAPI_statusLineNotAct */
   0x6018,
   /* TAPI_statusPhoneOffHook */
   0x6019,
   /* TAPI_statusCtxErr */
   0x601a,
   /* TAPI_statusInvalidCh */
   0x601b,
   /* TAPI_statusErrKernCpy */
   0x601c,
   /* TAPI_statusInitFail */
   0x601d,
   /* TAPI_statusInitRingFail */
   0x601e,
   /* TAPI_statusInitDialFail */
   0x601f,
   /* TAPI_statusInitMeterFail */
   0x6020,
   /* TAPI_statusInitCIDFail */
   0x6021,
   /* TAPI_statusInitToneFail */
   0x6022,
   /* TAPI_statusToneNotAvail */
   0x6023,
   /* TAPI_statusCIDBuffNoMem */
   0x6024,
   /* TAPI_statusCIDWrongConf */
   0x6025,
   /* TAPI_statusCIDNoLineModeOk */
   0x6026,
   /* TAPI_statusCIDActive */
   0x6027,
   /* TAPI_statusCIDHook */
   0x6028,
   /* TAPI_statusCIDStopTx */
   0x6029,
   /* TAPI_statusCIDStartRx */
   0x602a,
   /* TAPI_statusCIDStopRx */
   0x602b,
   /* TAPI_statusCIDCopy */
   0x602c,
   /* TAPI_statusCODAGCEn */
   0x602d,
   /* TAPI_statusPCMUnknownMode */
   0x602e,
   /* TAPI_statusPCMActivation */
   0x602f,
   /* TAPI_statusPCMConf */
   0x6030,
   /* TAPI_statusPhoneNo */
   0x6031,
   /* TAPI_statusDataNo */
   0x6032,
   /* TAPI_statusToneStop */
   0x6033,
   /* TAPI_statusRTPConf */
   0x6034,
   /* TAPI_statusMFTDEnFail */
   0x6035,
   /* TAPI_statusMFTDDisFail */
   0x6036,
   /* TAPI_statusTonePlayLLFailed */
   0x6037,
   /* TAPI_statusLLFailed */
   0x6038,
   /* TAPI_statusNoMem */
   0x6039,
   /* TAPI_statusFIFO */
   0x603a,
   /* TAPI_statusEvtNoHandle */
   0x603b,
   /* TAPI_statusWorkFail */
   0x603c,
   /* TAPI_statusTimerFail */
   0x603d,
   /* TAPI_statusRingInit */
   0x603e,
   /* TAPI_statusEvtFifoFull */
   0x603f,
   /* TAPI_statusInitDevFail */
   0x6040,
   /* TAPI_statusDtmfRxCfg */
   0x6041,
   /* TAPI_statusCidSmStart */
   0x6042,
   /* TAPI_statusUninitializedDev */
   0x6043,
   /* TAPI_statusCPTDEnFail */
   0x6044,
   /* TAPI_statusCPTDDisFail */
   0x6045,
   /* TAPI_statusIoctlBlocked */
   0x6046,
   /* TAPI_statusCIDBuffNoSpaceForDATE */
   0x6047,
   /* TAPI_statusCIDBuffNoSpaceForElement */
   0x6048,
   /* TAPI_statusCIDBuffNoSpaceForSTRING */
   0x6049,
   /* TAPI_statusCIDBuffNoSpaceForCLI */
   0x604a,
   /* TAPI_statusCIDBuffNoSpaceForNAME */
   0x604b,
   /* TAPI_statusCIDBuffMandatoryElementMissing */
   0x604c,
   /* TAPI_statusCIDBuffIncorrectTRANSPARENT */
   0x604d,
   /* TAPI_statusCIDConfAlertToneFailed */
   0x604e,
   /* TAPI_statusCIDHookMissmatch */
   0x604f,
   /* TAPI_statusCIDLineModeNotSuitable */
   0x6050,
   /* TAPI_statusCIDNoPhoneAtDataCh */
   0x6051,
   /* TAPI_statusCIDRXNoDataAvailable */
   0x6052,
   /* TAPI_statusChAlloc */
   0x6053,
   /* TAPI_statusMeterBurstFail */
   0x6054,
   /* TAPI_statusPCMIfConfError */
   0x6055,
   /* TAPI_statusPCMChCfgError */
   0x6056,
   /* TAPI_statusPCMChNoCfg */
   0x6057,
   /* TAPI_statusFWStart */
   0x6058,
   /* TAPI_statusBbdDownload */
   0x6059,
   /* TAPI_statusCIDInvalCharInDtmfMsgElem */
   0x605a,
   /* TAPI_statusRingLineModeNotOk */
   0x605b,
   /* TAPI_statusToneIdxOutOfRange */
   0x605c,
   /* TAPI_statusToneOnTimeOutOfRange */
   0x605d,
   /* TAPI_statusToneInitialTimeZero */
   0x605e,
   /* TAPI_statusTonePauseTimeOutOfRange */
   0x605f,
   /* TAPI_statusToneFrequencyOutOfRange */
   0x6060,
   /* TAPI_statusTonePowerLevelOutOfRange */
   0x6061,
   /* TAPI_statusToneAltVoiceTimeOutOfRange */
   0x6062,
   /* TAPI_statusToneLoopCountInvalid */
   0x6063,
   /* TAPI_statusToneCountOutOfRange */
   0x6064,
   /* TAPI_statusToneSimpleIdxOutOfRange */
   0x6065,
   /* TAPI_statusToneSimpleToneUnconfigured */
   0x6066,
   /* TAPI_statusLLDevNodeName */
   0x6067,
   /* TAPI_statusInitDaa */
   0x6068,
   /* TAPI_statusQosServiceNotSupported */
   0x6069,
   /* TAPI_statusInitPpdFail */
   0x606a,
   /* TAPI_statusInitPpdStateMachineFailed */
   0x606b,
   /* TAPI_statusNoQosRegistered */
   0x606c,
   /* TAPI_statusQosStartFailed */
   0x606d,
   /* TAPI_statusQosStartFdFailed */
   0x606e,
   /* TAPI_statusQosStopFailed */
   0x606f,
   /* TAPI_statusQosCleanupFailed */
   0x6070,
   /* TAPI_statusDtmfRxDirectionCombination */
   0x6071,
   /* TAPI_statusCIDRingPauseTooShort */
   0x6072,
   /* TAPI_statusRegisterToPmcuFailed */
   0x6073,
   /* TAPI_statusDeviceNotAddedToProcFs */
   0x6074,
   /* TAPI_statusLineFault */
   0x6075,
   /* TAPI_statusCapMeasStartWhileActive */
   0x6076,
   /* TAPI_statusDialZeroParam */
   0x6077,
   /* TAPI_statusDialMaxMinParam */
   0x6078,
   /* TAPI_statusDialTypeParam */
   0x6079,
   /* TAPI_statusEvtNotDisabled */
   0x607a
};

static const IFX_char_t * const TAPI_drvErrStrings [TAPI_ERRNO_CNT] =
{
   "Generic or unknown error occured",
   "Setting line mode failed",
   "Starting CID Info failed",
   "Invalid parameter",
   "Invalid ioctl call",
   "Unknown or unsupported ioctl call",
   "Desired action is not supported",
   "Service is not supported by the low level driver",
   "LL driver init failed",
   "LL driver already registered",
   "LL driver registration - incompatible HL/LL version",
   "LL driver registration info can not be copied",
   "LL driver with that major number already exists",
   "Device structure allocation failed",
   "No tone resource found or error in finding it for playing the tone",
   "Failed to configure predefined tone table",
   "Tone is already playing",
   "Invalid resource for tone service specified",
   "Ringing is already active",
   "Ringing cadence not correct",
   "No ring timer available",
   "Ringing can not be stopped",
   "Ringing can not be configured",
   "Metering already active",
   "Line not in active mode",
   "Phone in off hook state",
   "Service called with wrong context fd",
   "Service not supported on called channel context",
   "Copy KERN2USR or USR2KERN failed",
   "General initialization failed",
   "Ringing initialization failed",
   "Dialing initialization failed",
   "Metering initialization failed",
   "CID initialization failed",
   "Tone initialization failed",
   "Reference to unconfigured tone code entry",
   "Not enough spac in CID data buffer",
   "Wrong CID configuration",
   "Unsuitable line mode for CID Tx",
   "CID Tx already active",
   "CID hook state mismatch",
   "Can not stop CID Tx",
   "Can not start CID Rx",
   "Can not stop CID Rx",
   "CID data not copied",
   "Failed to enable COD AGC",
   "PCM mode unknown",
   "PCM activation failed due to LL error",
   "PCM configuration failed due to LL error",
   "Phone channel not available",
   "Data channel not available",
   "Stopping tone failed",
   "RTP payload table configuration failed",
   "MFTD enable failed",
   "MFTD disabled failed",
   "Playing tone in LL driver failed",
   "LL driver returned an error",
   "Memory not available",
   "FIFO size or FIFO initialization error",
   "Event not known or not handled",
   "TAPI_DeferWork schedule_work failed",
   "Timer creation failed",
   "Ringing was not initialized",
   "Event fifo queue full",
   "Device initialization failed",
   "DTMF receiver configuration failed in LL driver",
   "Starting CID state machine failed",
   "Device not initialized",
   "Failed to enable CPTD",
   "Failed to disable CPTD",
   "IOCTL is blocked until the device is started",
   "Not enough space in buffer for DATE element",
   "Not enough space in buffer for element",
   "Not enough space in buffer for STRING element",
   "Not enough space in buffer for CLI element",
   "Not enough space in buffer for NAME element",
   "No valid data for NTT CID, mandatory elements missing",
   "Wrong size for TRANSPARENT element or TRANSPARENT together with other elements types",
   "Configuration of predefined CID alert tone failed",
   "Hookstate not matching with transmission mode",
   "Line mode not suitable for CID sequence transmission",
   "No phone is connected on the data channel main input",
   "No data available in CID receiver buffer",
   "Channel structure allocation failed",
   "Failed to start metering",
   "PCM interface configuration failed",
   "PCM channel configuration failed",
   "PCM channel not configured",
   "FW starting failed",
   "BBD downoading failed",
   "Invalid character passed in DTMF CID message element",
   "Unsuitable line mode for ringing",
   "Tone index out of range",
   "Simple tone on-time time exceeds the maximum range",
   "Simple tone time of first step may not be zero",
   "Simple tone pause time exceeds the maximum supported pause time",
   "Simple tone frequency exceeds the maximum supported frequency",
   "Simple tone power level exceeds the supported range",
   "Composed tone max alternate voice time exceeds the maximum",
   "Composed tone single repetition not allowed when alternate voice time is non-zero",
   "Composed tone number of simple tone fields is out of range",
   "Composed tone contains a simple tone index that is out of range",
   "Composed tone contains a simple tone that is not configured",
   "LL driver was not registered because devNodeName is not set",
   "Daa was not initialized",
   "QOS: Service not supported by QOS driver - IOCTL failed",
   "PPD initialization failed",
   "PPD state machine failed",
   "No QOS driver registered - IOCTL aborted",
   "QOS start failed",
   "QOS start on existing socket failed",
   "QOS stop failed",
   "QOS cleanup failed",
   "Combination of directions for DTMF digit and DTMF end detection is not possible.",
   "Configured cadence pause time not sufficient for CID data to be transmitted.",
   "Registration to PMCU failed",
   "Device not added to proc fs",
   "Cannot set linefeeding different than disabled while a line fault is present.",
   "Capacitance measurement is already active",
   "Value zero not allowed as hook state validation time",
   "Max must be larger than min hook state validation time",
   "Unknown hook state validation type parameter",
   "Event cannot be disable, event always enabled"
};

#ifdef __cplusplus
   }
#endif

#endif /* _DRV_TAPI_STRERRNO_H */
