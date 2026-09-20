#ifndef _DRV_DXS_STRERRNO_H
#define _DRV_DXS_STRERRNO_H
/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016, Intel Corporation.
  Copyright 2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_strerrno.h
*/

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
#define DXS_ERRNO_CNT 158

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */
const IFX_uint32_t DXS_drvErrnos[DXS_ERRNO_CNT] =
{
   /* DXS_statusOk */
   0,
   /* DXS_statusNoChg */
   0x1,
   /* DXS_statusNoData */
   0x5001,
   /* DXS_statusFwDwldFail */
   0x6010,
   /* DXS_statusFwDwldTimeout */
   0x6011,
   /* DXS_statusNoFwDwld */
   0x6012,
   /* DXS_statusSetBootCfgErr */
   0x6013,
   /* DXS_statusCtrlResErr */
   0x6014,
   /* DXS_statusDwldBinErr */
   0x6015,
   /* DXS_statusNoMem */
   0x6016,
   /* DXS_statusParam */
   0x6017,
   /* DXS_statusIntStuck */
   0x6018,
   /* DXS_statusNotInitialized */
   0x6019,
   /* DXS_statusReadErr */
   0x601a,
   /* DXS_statusCmdIbNoSpace */
   0x601b,
   /* DXS_statusCmdIbNotAvail */
   0x601c,
   /* DXS_statusCmdObTimeout */
   0x601d,
   /* DXS_statusCmdObDataOvld */
   0x601e,
   /* DXS_statusCmdObRdErr */
   0x601f,
   /* DXS_statusEvtObNoData */
   0x6020,
   /* DXS_statusEvtObRdErr */
   0x6021,
   /* DXS_statusEvtMbErr */
   0x6022,
   /* DXS_statusCmdMbWrErr */
   0x6023,
   /* DXS_statusSpiAccErr */
   0x6024,
   /* DXS_statusPcmNotInitialized */
   0x6025,
   /* DXS_statusNoBbdBuf */
   0x6026,
   /* DXS_statusBbdCorrupt */
   0x6027,
   /* DXS_statusBbdErr */
   0x6028,
   /* DXS_statusTestChipAccErr */
   0x6029,
   /* DXS_statusTestCmdBoxErr */
   0x602a,
   /* DXS_statusTestCmdBoxReadErr */
   0x602b,
   /* DXS_statusTestCmdBoxWriteErr */
   0x602c,
   /* DXS_statusInitFail */
   0x602d,
   /* DXS_statusInitFwMsgErr */
   0x602e,
   /* DXS_statusRegInitErr */
   0x602f,
   /* DXS_statusEvtEnblErr */
   0x6030,
   /* DXS_statusDtmfRcvInitErr */
   0x6031,
   /* DXS_statusGpioNoResource */
   0x6032,
   /* DXS_statusGpioRelResErr */
   0x6033,
   /* DXS_statusGpioConfigResErr */
   0x6034,
   /* DXS_statusGpioSetResErr */
   0x6035,
   /* DXS_statusGpioGetResErr */
   0x6036,
   /* DXS_statusDeviceIdErr */
   0x6037,
   /* DXS_statusSetSddBasicDefaultErr */
   0x6038,
   /* DXS_statusAsdspChecksumMismatch */
   0x6039,
   /* DXS_statusAsdspDwldFailed */
   0x603a,
   /* DXS_statusNoPerm */
   0x603b,
   /* DXS_statusChErr */
   0x7000,
   /* DXS_statusCmdRdTimeout */
   0x7001,
   /* DXS_statusCmdWrErr */
   0x7002,
   /* DXS_statusNoResource */
   0x7003,
   /* DXS_statusInvalCh */
   0x7004,
   /* DXS_statusInvalLMSwitch */
   0x7005,
   /* DXS_statusFuncParam */
   0x7006,
   /* DXS_statusNotSupported */
   0x7007,
   /* DXS_statusBbdCramErr */
   0x7008,
   /* DXS_statusBbdRingErr */
   0x7009,
   /* DXS_statusBbdBasicErr */
   0x700a,
   /* DXS_statusBbdBlockErr */
   0x700b,
   /* DXS_statusDtmfAtStopErr */
   0x700c,
   /* DXS_statusDtmfAtStartErr */
   0x700d,
   /* DXS_statusDtmfAtLevCfgErr */
   0x700e,
   /* DXS_statusDtmfAtFreqCfgErr */
   0x700f,
   /* DXS_statusToneDirErr */
   0x7010,
   /* DXS_statusToneTypeErr */
   0x7011,
   /* DXS_statusToneCfgErr */
   0x7012,
   /* DXS_statusTonePlayErr */
   0x7013,
   /* DXS_statusToneStartErr */
   0x7014,
   /* DXS_statusToneStopErr */
   0x7015,
   /* DXS_statusGR909LineNotDisabled */
   0x7016,
   /* DXS_statusGR909Busy */
   0x7017,
   /* DXS_statusCidAct */
   0x7018,
   /* DXS_statusCidCtrlErr */
   0x7019,
   /* DXS_statusCidSetCoefErr */
   0x701a,
   /* DXS_statusCidShSetupErr */
   0x701b,
   /* DXS_statusCidShTransmitErr */
   0x701c,
   /* DXS_statusCidShTransmitEndErr */
   0x701d,
   /* DXS_statusDtmfTimingErr */
   0x701e,
   /* DXS_statusDtmfAct */
   0x701f,
   /* DXS_statusDtmfCreateTimerErr */
   0x7020,
   /* DXS_statusDtmfShSetupErr */
   0x7021,
   /* DXS_statusDtmfRcvLevErr */
   0x7022,
   /* DXS_statusDtmfRcvTwistErr */
   0x7023,
   /* DXS_statusRxGainErr */
   0x7024,
   /* DXS_statusTxGainErr */
   0x7025,
   /* DXS_statusOpModeWrErr */
   0x7026,
   /* DXS_statusAlmVolErr */
   0x7027,
   /* DXS_statusCidShErr */
   0x7028,
   /* DXS_statusDtmfStartErr */
   0x7029,
   /* DXS_statusDtmfStopErr */
   0x702a,
   /* DXS_statusDtmfRcvCoefErr */
   0x702b,
   /* DXS_statusDtmfRcvCtrlErr */
   0x702c,
   /* DXS_statusDtmfGenCtrlErr */
   0x702d,
   /* DXS_statusDtmfShTransmitErr */
   0x702e,
   /* DXS_statusDtmfShPauseErr */
   0x702f,
   /* DXS_statusDtmfShErr */
   0x7030,
   /* DXS_statusCidStartSeqErr */
   0x7031,
   /* DXS_statusCidStdNotSupported */
   0x7032,
   /* DXS_statusCidTxStopErr */
   0x7033,
   /* DXS_statusMwlLMNotActive */
   0x7034,
   /* DXS_statusMwlActive */
   0x7035,
   /* DXS_statusMwlNotActive */
   0x7036,
   /* DXS_statusPcmTsInvalid */
   0x7037,
   /* DXS_statusPcmHwInvalid */
   0x7038,
   /* DXS_statusCalInProgress */
   0x7039,
   /* DXS_statusCalLineNotDisabled */
   0x703a,
   /* DXS_statusCalSetValOutOfRange */
   0x703b,
   /* DXS_statusRingCfgRdErr */
   0x703c,
   /* DXS_statusRingCfgWrErr */
   0x703d,
   /* DXS_statusBasicCfgRdErr */
   0x703e,
   /* DXS_statusBasicCfgWrErr */
   0x703f,
   /* DXS_statusChipAccNotSetup */
   0x7040,
   /* DXS_statusCapMeasStartWhileActive */
   0x7041,
   /* DXS_statusChipAccFailed */
   0x7042,
   /* DXS_statusInvalidRmes */
   0x7043,
   /* DXS_statusNoMwlAndCombinedDcDc */
   0x7044,
   /* DXS_statusCapNeighbourLineNotDisabled */
   0x7045,
   /* DXS_statusCalNeighbourLineNotDisabled */
   0x7046,
   /* DXS_statusGR909NeighbourLineNotDisabled */
   0x7047,
   /* DXS_statusNeighbourLineBlocksLMSwitch */
   0x7048,
   /* DXS_statusPcmResolutionNotSupported */
   0x7049,
   /* DXS_statusPcmRequestedTsInUse */
   0x704a,
   /* DXS_statusInvalLineType */
   0x704b,
   /* DXS_statusBlockedNoDcDcType */
   0x704c,
   /* DXS_statusBBDcDcNotSupported */
   0x704d,
   /* DXS_statusBbdMwlErr */
   0x704e,
   /* DXS_statusBbdDwldBlocked */
   0x704f,
   /* DXS_statusBbdDcDcErr */
   0x7050,
   /* DXS_statusBbdDcDcReconfig */
   0x7051,
   /* DXS_statusBbdDcDcCfgDiffers */
   0x7052,
   /* DXS_statusHighLevelNotActive */
   0x7053,
   /* DXS_statusBbdDcDcHwDiffers */
   0x7054,
   /* DXS_statusAutomaticCalibrationFailed */
   0x7055,
   /* DXS_statusUtdDirError */
   0x7056,
   /* DXS_statusUtdCtrlErr */
   0x7057,
   /* DXS_statusUtdCoeffMissing */
   0x7058,
   /* DXS_statusUtdTableFull */
   0x7059,
   /* DXS_statusCmdLengthInvalid */
   0x705a,
   /* DXS_statusThreadCreateError */
   0x705b,
   /* DXS_statusMsgQueueCreatError */
   0x705c,
   /* DXS_statusObxHandlerStartError */
   0x705d,
   /* DXS_statusBootFailed */
   0x705e,
   /* DXS_statusCapVersUpdateError */
   0x705f,
   /* DXS_statusReadInterrupted */
   0x7060,
   /* DXS_statusMeteringLMNotActive */
   0x7061,
   /* DXS_statusSddEvtWaitTmout */
   0x7062,
   /* DXS_statusSddEvtWaitInterrupt */
   0x7063,
   /* DXS_statusPcmMuteErr */
   0x7064,
   /* DXS_statusAclmStartErrActOpmodeTmout */
   0x7065,
   /* DXS_statusAclmInProgress */
   0x7066,
   /* DXS_statusAclmStartErrInvOpmode */
   0x7067,
   /* DXS_statusAclmResultsNotAvail */
   0x7068,
   /* DXS_statusPcmIfCfgWhileActive */
   0x7069,
   /* DXS_statusDcDcTypeUnknown */
   0x706a,
   /* DXS_statusBbdMissingInArchive */
   0x706b,
   /* DXS_statusObxFwMsgHdrCorrupt */
   0x706c,
   /* DXS_statusAutomaticCalibrationTimeout */
   0x706d,
   /* DXS_statusErr */
   0x8000
};

const IFX_char_t *DXS_drvErrStrings[DXS_ERRNO_CNT] =
{
   "Success, no error occurred.",
   "Success, no change in the configuration found and no message send.",
   "No data currently available.",
   "Firmware download failed.",
   "Firmware download timeout.",
   "User flag to omit firmware download is set.",
   "Setting of boot configuration register failed.",
   "Controller reset failed.",
   "Download of the firmware binary failed.",
   "No memory could be allocated.",
   "At least one parameter is wrong.",
   "Interrupts could not be cleared.",
   "Device not yet initialized.",
   "Device read access failed.",
   "Not enough space in command inbox for writing command.",
   "Command inbox is not available.",
   "Timeout while waiting for data in command outbox.",
   "More data in command outbox than expected.",
   "Reading from the command outbox failed.",
   "No data available in event outbox.",
   "Reading from the event mailbox failed.",
   "Corrupt event mailbox.",
   "Writing to command mailbox failed.",
   "Error accessing device registers.",
   "PCM interface is not initialized.",
   "No BBD download buffer available.",
   "The BBD download buffer is corrupt.",
   "BBD download failed.",
   "Test chip access failed.",
   "Test command box access failed.",
   "Test read continuously messages from command box failed.",
   "Test write/read back messages to/from command box failed.",
   "Device initialization failed.",
   "Initializing FW messages failed.",
   "Initialzing registers with default values failed.",
   "Enabling of events failed.",
   "Enabling dtmf receiver failed.",
   "GPIO resource is not available.",
   "Releasing a GPIO resource failed.",
   "Configuring a GPIO resource failed.",
   "Setting the value of a GPIO resource failed.",
   "Getting the value of a GPIO resource failed.",
   "Unsupported Device ID.",
   "Error setting defaults for SDD message BasicConfig.",
   "ASDSP checksum mismatch",
   "ASDSP download failed",
   "Insufficient permissions",
   "Unknown error in channel.",
   "Timeout while waiting for read data.",
   "Writing a command to channel failed.",
   "Resource not available.",
   "Resource not valid. Channel number out of range",
   "Line mode switch is invalid. Not every transition is valid.",
   "At least one parameter in function is wrong.",
   "Feature or combination not supported.",
   "Download of CRAM BBD block failed.",
   "Download of RingCfg BBD block failed.",
   "Download of DC Basic BBD block failed.",
   "Downloading a BBD block failed.",
   "Stopping the dtmf/at generator failed.",
   "Starting the dtmf/at generator failed.",
   "Configuring the dtmf/at level failed.",
   "Configuring the dtmf/at frequencies failed.",
   "Invalid direction for the tone to be played out.",
   "Unsupported tone type.",
   "Tone not configured.",
   "Error playing out a tone.",
   "Playing out a tone failed.",
   "Error stopping a tone.",
   "GR909 might be started only on disabled lines",
   "GR909 measurement ongoing, cannot read results",
   "A CID transmission is already active.",
   "Changing the state of the CID generator failed.",
   "Setting coefficients for CID failed.",
   "CID state handler error from state CID setup.",
   "CID state handler error from state CID transmit.",
   "CID state handler error from state CID transmit end.",
   "DTMF digit or interdigit timing invalid.",
   "A DTMF transmission is active.",
   "Creating a timer for DTMF transmission failed",
   "DTMF state handler error from state DTMF setup.",
   "DTMF receiver level parameter out of range.",
   "DTMF receiver twist parameter out of range.",
   "ALM RxGain parameter out of range.",
   "ALM TxGain parameter out of range.",
   "Writing the operation mode failed.",
   "Writing the ALM volume failed.",
   "CID state handler error.",
   "Starting the DTMF generator failed.",
   "Stopping the DTMF generator failed.",
   "Setting DTMF receiver coefficients failed.",
   "Activation or deactivation of DTMF receiver failed.",
   "Activation or deactivation of DTMF generator failed.",
   "DTMF state handler error from state DTMF transmit.",
   "DTMF state handler error from state DTMF pause.",
   "DTMF state handler error.",
   "Initiating a CID sequence failed",
   "CID standard not supported.",
   "CID Tx could not be stopped.",
   "MWL (Message Waiting Lamp) might be activated only in linemode active",
   "MWL (Message Waiting Lamp) is active, the desired action is not possible",
   "MWL is currently not active",
   "PCM timeslot given out of range.",
   "PCM Highway number out of range.",
   "Current line mode is CALIBRATE",
   "Current line mode is not DISABLED",
   "Calibration set values are out of range",
   "Reading the ring configuration failed",
   "Writing the ring configuration failed",
   "Reading the basic configuration failed",
   "Writing the basic configuration failed",
   "Chip access is not setup.",
   "Capacitance measurement is already active",
   "Access to chip registers failed.",
   "Invalid Rmes value.",
   "MWL not possible in combined DC",
   "Neighbour line mode is not DISABLED for Capacitance measurement",
   "Neighbour line mode is not DISABLED",
   "Neighbour line mode is not DISABLED",
   "Linefeed change not allowed while neighbour line is doing linetesting.",
   "Requested PCM resolution not supported.",
   "Requested PCM timeslot is already in use.",
   "Invalid line type for analog line",
   "Operation blocked until BBD containing DC/DC type setting is downloaded",
   "Desired buck-or-boost DC/DC operation not supported by FW",
   "Download of MWL BBD block failed.",
   "BBD download blocked because not all lines are disabled.",
   "Download of DC/DC BBD block failed.",
   "Ignoring already set SDD DC/DC config in BBD download.",
   "BBD download must not set differing DC/DC operation mode for the channels.",
   "Setting of high level output requires active feeding",
   "DC/DC converter type in the BBD download must match the DC/DC HW type set at driver configuration.",
   "Automatic calibration after BBD download failed.",
   "Direction specification for UTD activation is invalid.",
   "Activation of universal tone detector failed.",
   "UTD coefficients for specified tone missing.",
   "UTD coefficents cannot be stored because the table is full.",
   "Length of command invalid.",
   "Error creating thread.",
   "Failed to create fifo for oubox handling.",
   "Failed to start thread for outbox handling.",
   "Chip boot failed.",
   "Failed to read version or capability from FW.",
   "CmdRead interrupted by signal.",
   "Metering might be activated only in linemode active",
   "Timeout waiting for an event from SDD.",
   "Waiting for an event from SDD interrupted by a signal.",
   "Muting/unmuting the PCM path failed.",
   "Neighbour line mode is not DISABLED",
   "measurement is already running",
   "Wrong line state",
   "ACLM results not available",
   "PCM interface cannot be configured while any PCM channel is active",
   "Unknown DC/DC converter type detected",
   "BBD archive does not contain the needed BBD.",
   "Header of message read from outbox is corrupted.",
   "Automatic calibration timeout while waiting for event.",
   "Generic or unknown error occurred"
};

#endif /* _DRV_DXS_STRERRNO_H */
