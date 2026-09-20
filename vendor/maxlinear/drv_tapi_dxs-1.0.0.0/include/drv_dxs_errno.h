#ifndef _DRV_DXS_ERRNO_H
#define _DRV_DXS_ERRNO_H
/******************************************************************************

  Copyright 2014-2015 Lantiq Deutschland GmbH
  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016      Intel Corporation.
  Copyright 2022      MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_errno.h
   This file contains error number definitions and macros for setting the error
   code.
   \note The macros must be used for reporting errors.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_tapi_io.h"
#include "ifx_types.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* Event reporting/counting macros, can be modified/adapted if needed.
   It is currently used only for evaluation purposes */
#define DXS_REPORT_EVENT(handle,id,pEvt)

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
/** \defgroup ErrorCodes Driver and Chip Error Codes */
/*@{*/

/* Errorblock */
/** Enumeration for function return status. The upper four bits are reserved for
    error classification */
typedef enum
{
   /** Success, no error occurred. */
   DXS_statusOk = TAPI_statusClassSuccess,
   /** Success, no change in the configuration found and no message send. */
   DXS_statusNoChg,

   /******************************************************** Device warnings */

   /******************************************************* Channel warnings */
   /** No data currently available. */
   DXS_statusNoData = TAPI_statusClassCh | TAPI_statusClassWarn | 0x1,

   /********************************************************** Device errors */
   /** Firmware download failed. */
   DXS_statusFwDwldFail = TAPI_statusClassErr | 0x10,
   /** Firmware download timeout. */
   DXS_statusFwDwldTimeout,
   /** User flag to omit firmware download is set. */
   DXS_statusNoFwDwld,
   /** Setting of boot configuration register failed. */
   DXS_statusSetBootCfgErr,
   /** Controller reset failed. */
   DXS_statusCtrlResErr,
   /** Download of the firmware binary failed. */
   DXS_statusDwldBinErr,
   /** No memory could be allocated. */
   DXS_statusNoMem,
   /** At least one parameter is wrong. */
   DXS_statusParam,
   /** Interrupts could not be cleared. */
   DXS_statusIntStuck,
   /** Device not yet initialized. */
   DXS_statusNotInitialized,
   /** Device read access failed. */
   DXS_statusReadErr,
   /** Not enough space in command inbox for writing command. */
   DXS_statusCmdIbNoSpace,
   /** Command inbox is not available. */
   DXS_statusCmdIbNotAvail,
   /** Timeout while waiting for data in command outbox. */
   DXS_statusCmdObTimeout,
   /** More data in command outbox than expected. */
   DXS_statusCmdObDataOvld,
   /** Reading from the command outbox failed. */
   DXS_statusCmdObRdErr,
   /** No data available in event outbox. */
   DXS_statusEvtObNoData,
   /** Reading from the event mailbox failed. */
   DXS_statusEvtObRdErr,
   /** Corrupt event mailbox. */
   DXS_statusEvtMbErr,
   /** Writing to command mailbox failed. */
   DXS_statusCmdMbWrErr,
   /** Error accessing device registers. */
   DXS_statusSpiAccErr,
   /** PCM interface is not initialized. */
   DXS_statusPcmNotInitialized,
   /** No BBD download buffer available. */
   DXS_statusNoBbdBuf,
   /** The BBD download buffer is corrupt. */
   DXS_statusBbdCorrupt,
   /** BBD download failed.  */
   DXS_statusBbdErr,
   /** Test chip access failed.  */
   DXS_statusTestChipAccErr,
   /** Test command box access failed. */
   DXS_statusTestCmdBoxErr,
   /** Test read continuously messages from command box failed. */
   DXS_statusTestCmdBoxReadErr,
   /** Test write/read back messages to/from command box failed. */
   DXS_statusTestCmdBoxWriteErr,
   /** Device initialization failed. */
   DXS_statusInitFail,
   /** Initializing FW messages failed. */
   DXS_statusInitFwMsgErr,
   /** Initialzing registers with default values failed. */
   DXS_statusRegInitErr,
   /** Enabling of events failed. */
   DXS_statusEvtEnblErr,
   /** Enabling dtmf receiver failed. */
   DXS_statusDtmfRcvInitErr,
   /** GPIO resource is not available. */
   DXS_statusGpioNoResource,
   /** Releasing a GPIO resource failed. */
   DXS_statusGpioRelResErr,
   /** Configuring a GPIO resource failed. */
   DXS_statusGpioConfigResErr,
   /** Setting the value of a GPIO resource failed. */
   DXS_statusGpioSetResErr,
   /** Getting the value of a GPIO resource failed. */
   DXS_statusGpioGetResErr,
   /** Unsupported Device ID. */
   DXS_statusDeviceIdErr,
   /** Error setting defaults for SDD message BasicConfig. */
   DXS_statusSetSddBasicDefaultErr,
   /** ASDSP checksum mismatch  */
   DXS_statusAsdspChecksumMismatch,
   /** ASDSP download failed  */
   DXS_statusAsdspDwldFailed,
   /** Insufficient permissions */
   DXS_statusNoPerm,
/********************************************************* Channel errors */
   /** Unknown error in channel. */
   DXS_statusChErr = TAPI_statusClassErr | TAPI_statusClassCh,
   /** Timeout while waiting for read data. */
   DXS_statusCmdRdTimeout,
   /** Writing a command to channel failed. */
   DXS_statusCmdWrErr,
   /** Resource not available. */
   DXS_statusNoResource,
   /** Resource not valid. Channel number out of range  */
   DXS_statusInvalCh,
   /** Line mode switch is invalid. Not every transition is valid.  */
   DXS_statusInvalLMSwitch,
   /** At least one parameter in function is wrong. */
   DXS_statusFuncParam,
   /** Feature or combination not supported. */
   DXS_statusNotSupported,
   /** Download of CRAM BBD block failed. */
   DXS_statusBbdCramErr,
   /** Download of RingCfg BBD block failed.  */
   DXS_statusBbdRingErr,
   /** Download of DC Basic BBD block failed.  */
   DXS_statusBbdBasicErr,
   /** Downloading a BBD block failed. */
   DXS_statusBbdBlockErr,
   /** Stopping the dtmf/at generator failed. */
   DXS_statusDtmfAtStopErr,
   /** Starting the dtmf/at generator failed. */
   DXS_statusDtmfAtStartErr,
   /** Configuring the dtmf/at level failed. */
   DXS_statusDtmfAtLevCfgErr,
   /** Configuring the dtmf/at frequencies failed. */
   DXS_statusDtmfAtFreqCfgErr,
   /** Invalid direction for the tone to be played out.  */
   DXS_statusToneDirErr,
   /** Unsupported tone type.  */
   DXS_statusToneTypeErr,
   /** Tone not configured.  */
   DXS_statusToneCfgErr,
   /** Error playing out a tone. */
   DXS_statusTonePlayErr,
   /** Playing out a tone failed. */
   DXS_statusToneStartErr,
   /** Error stopping a tone.  */
   DXS_statusToneStopErr,
   /** GR909 might be started only on disabled lines  */
   DXS_statusGR909LineNotDisabled,
   /** GR909 measurement ongoing, cannot read results  */
   DXS_statusGR909Busy,
   /** A CID transmission is already active. */
   DXS_statusCidAct,
   /** Changing the state of the CID generator failed. */
   DXS_statusCidCtrlErr,
   /** Setting coefficients for CID failed. */
   DXS_statusCidSetCoefErr,
   /** CID state handler error from state CID setup. */
   DXS_statusCidShSetupErr,
   /** CID state handler error from state CID transmit. */
   DXS_statusCidShTransmitErr,
   /** CID state handler error from state CID transmit end. */
   DXS_statusCidShTransmitEndErr,
   /** DTMF digit or interdigit timing invalid. */
   DXS_statusDtmfTimingErr,
   /** A DTMF transmission is active. */
   DXS_statusDtmfAct,
   /** Creating a timer for DTMF transmission failed  */
   DXS_statusDtmfCreateTimerErr,
   /** DTMF state handler error from state DTMF setup. */
   DXS_statusDtmfShSetupErr,
   /** DTMF receiver level parameter out of range. */
   DXS_statusDtmfRcvLevErr,
   /** DTMF receiver twist parameter out of range. */
   DXS_statusDtmfRcvTwistErr,
   /** ALM RxGain parameter out of range. */
   DXS_statusRxGainErr,
   /** ALM TxGain parameter out of range. */
   DXS_statusTxGainErr,
   /** Writing the operation mode failed. */
   DXS_statusOpModeWrErr,
   /** Writing the ALM volume failed. */
   DXS_statusAlmVolErr,
   /** CID state handler error. */
   DXS_statusCidShErr,
   /** Starting the DTMF generator failed. */
   DXS_statusDtmfStartErr,
   /** Stopping the DTMF generator failed. */
   DXS_statusDtmfStopErr,
   /** Setting DTMF receiver coefficients failed. */
   DXS_statusDtmfRcvCoefErr,
   /** Activation or deactivation of DTMF receiver failed. */
   DXS_statusDtmfRcvCtrlErr,
   /** Activation or deactivation of DTMF generator failed. */
   DXS_statusDtmfGenCtrlErr,
   /** DTMF state handler error from state DTMF transmit. */
   DXS_statusDtmfShTransmitErr,
   /** DTMF state handler error from state DTMF pause. */
   DXS_statusDtmfShPauseErr,
   /** DTMF state handler error. */
   DXS_statusDtmfShErr,
   /** Initiating a CID sequence failed  */
   DXS_statusCidStartSeqErr,
   /** CID standard not supported. */
   DXS_statusCidStdNotSupported,
   /** CID Tx could not be stopped. */
   DXS_statusCidTxStopErr,
   /** MWL (Message Waiting Lamp) might be activated only in linemode active */
   DXS_statusMwlLMNotActive,
   /** MWL (Message Waiting Lamp) is active, the desired action is not possible  */
   DXS_statusMwlActive,
   /** MWL is currently not active  */
   DXS_statusMwlNotActive,
   /** PCM timeslot given out of range. */
   DXS_statusPcmTsInvalid,
   /** PCM Highway number out of range. */
   DXS_statusPcmHwInvalid,
   /** Current line mode is CALIBRATE  */
   DXS_statusCalInProgress,
   /** Current line mode is not DISABLED  */
   DXS_statusCalLineNotDisabled,
   /** Calibration set values are out of range  */
   DXS_statusCalSetValOutOfRange,
   /** Reading the ring configuration failed  */
   DXS_statusRingCfgRdErr,
   /** Writing the ring configuration failed  */
   DXS_statusRingCfgWrErr,
   /** Reading the basic configuration failed  */
   DXS_statusBasicCfgRdErr,
   /** Writing the basic configuration failed  */
   DXS_statusBasicCfgWrErr,
   /** Chip access is not setup. */
   DXS_statusChipAccNotSetup,
   /** Capacitance measurement is already active  */
   DXS_statusCapMeasStartWhileActive,
   /** Access to chip registers failed. */
   DXS_statusChipAccFailed,
   /** Invalid Rmes value. */
   DXS_statusInvalidRmes,
   /** MWL not possible in combined DC */
   DXS_statusNoMwlAndCombinedDcDc,
   /** Neighbour line mode is not DISABLED for Capacitance measurement */
   DXS_statusCapNeighbourLineNotDisabled,
   /** Neighbour line mode is not DISABLED */
   DXS_statusCalNeighbourLineNotDisabled,
   /** Neighbour line mode is not DISABLED */
   DXS_statusGR909NeighbourLineNotDisabled,
   /** Linefeed change not allowed while neighbour line is doing
       linetesting. */
   DXS_statusNeighbourLineBlocksLMSwitch,
   /** Requested PCM resolution not supported. */
   DXS_statusPcmResolutionNotSupported,
   /** Requested PCM timeslot is already in use. */
   DXS_statusPcmRequestedTsInUse,
   /** Invalid line type for analog line */
   DXS_statusInvalLineType,
   /** Operation blocked until BBD containing DC/DC type setting is
       downloaded */
   DXS_statusBlockedNoDcDcType,
   /** Desired buck-or-boost DC/DC operation not supported by FW */
   DXS_statusBBDcDcNotSupported,
   /** Download of MWL BBD block failed.  */
   DXS_statusBbdMwlErr,
   /** BBD download blocked because not all lines are disabled. */
   DXS_statusBbdDwldBlocked,
   /** Download of DC/DC BBD block failed.  */
   DXS_statusBbdDcDcErr,
   /** Ignoring already set SDD DC/DC config in BBD download. */
   DXS_statusBbdDcDcReconfig,
   /** BBD download must not set differing DC/DC operation mode
       for the channels. */
   DXS_statusBbdDcDcCfgDiffers,
   /** Setting of high level output requires active feeding */
   DXS_statusHighLevelNotActive,
   /** DC/DC converter type in the BBD download must match the
       DC/DC HW type set at driver configuration. */
   DXS_statusBbdDcDcHwDiffers,
   /** Automatic calibration after BBD download failed. */
   DXS_statusAutomaticCalibrationFailed,
   /** Direction specification for UTD activation is invalid. */
   DXS_statusUtdDirError,
   /** Activation of universal tone detector failed. */
   DXS_statusUtdCtrlErr,
   /** UTD coefficients for specified tone missing. */
   DXS_statusUtdCoeffMissing,
   /** UTD coefficents cannot be stored because the table is full. */
   DXS_statusUtdTableFull,
   /** Length of command invalid. */
   DXS_statusCmdLengthInvalid,
   /** Error creating thread. */
   DXS_statusThreadCreateError,
   /** Failed to create fifo for oubox handling. */
   DXS_statusMsgQueueCreatError,
   /** Failed to start thread for outbox handling. */
   DXS_statusObxHandlerStartError,
   /** Chip boot failed. */
   DXS_statusBootFailed,
   /** Failed to read version or capability from FW. */
   DXS_statusCapVersUpdateError,
   /** CmdRead interrupted by signal. */
   DXS_statusReadInterrupted,
   /** Metering might be activated only in linemode active */
   DXS_statusMeteringLMNotActive,
   /** Timeout waiting for an event from SDD. */
   DXS_statusSddEvtWaitTmout,
   /** Waiting for an event from SDD interrupted by a signal. */
   DXS_statusSddEvtWaitInterrupt,
   /** Muting/unmuting the PCM path failed. */
   DXS_statusPcmMuteErr,
   /** Neighbour line mode is not DISABLED */
   DXS_statusAclmStartErrActOpmodeTmout,
   /** measurement is already running */
   DXS_statusAclmInProgress,
   /** Wrong line state */
   DXS_statusAclmStartErrInvOpmode,
   /** ACLM results not available */
   DXS_statusAclmResultsNotAvail,
   /** PCM interface cannot be configured while any PCM channel
       is active */
   DXS_statusPcmIfCfgWhileActive,
   /** Unknown DC/DC converter type detected */
   DXS_statusDcDcTypeUnknown,
   /** BBD archive does not contain the needed BBD. */
   DXS_statusBbdMissingInArchive,
   /** Header of message read from outbox is corrupted. */
   DXS_statusObxFwMsgHdrCorrupt,
   /** Automatic calibration timeout while waiting for event. */
   DXS_statusAutomaticCalibrationTimeout,
   /******************************************************** Critical errors */
   /** Generic or unknown error occurred */
   DXS_statusErr = TAPI_statusClassCritical,
   /** driver initialization failed. */
   DXS_statusDrvInitFail
}DXS_status_t;

/*@}*/ /* ErrorCodes */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

#endif /* _DRV_DXS_ERRNO_H */
