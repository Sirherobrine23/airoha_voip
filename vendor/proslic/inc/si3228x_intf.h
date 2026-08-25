/*
** Copyright (c) 2015-2017 by Silicon Laboratories
**
** $Id: si3228x_intf.h 6439 2017-04-25 23:55:03Z nizajerk $
**
** Si3228x_Intf.h
** Si3228x ProSLIC interface header file
**
**
** Distributed by:
** Silicon Laboratories, Inc
**
** This file contains proprietary information.
** No dissemination allowed without prior written permission from
** Silicon Laboratories, Inc.
**
** File Description:
** This is the header file for the ProSLIC driver.
**
**
*/

#ifndef SI3228X_INTF_H
#define SI3228X_INTF_H
#include "si3228x.h"

/*
** Calibration Constants
*/
#define SI3228X_CAL_STD_CALR1               0xC0    /* FF */
#define SI3228X_CAL_STD_CALR2               0x18    /* F8 */

/* Timeouts in 10s of ms */
#define SI3228X_TIMEOUT_DCDC_UP             200
#define SI3228X_TIMEOUT_DCDC_DOWN           200

/* The following macros are for backward compatibility */
#define Si3228x_DCFeedSetup(PCHAN,PRESET)    Si3228x_DCFeedSetupCfg((PCHAN),Si3228x_DCfeed_Presets,(PRESET))
#define Si3228x_LoadPatch                    ProSLIC_LoadPatch
#define Si3228x_ReadHookStatus               ProSLIC_ReadHookStatus
#define Si3228x_SetPowersaveMode             ProSLIC_SetPowersaveMode
#define Si3228x_VerifyPatch                  ProSLIC_VerifyPatch
#define Si3228x_Init(PCHAN,SZ)               Si3228x_Init_with_Options((PCHAN),(SZ),INIT_NO_OPT)
#define Si3228x_VerifyControlInterface       ProSLIC_VefifyControlInterface
#define Si3228x_ShutdownChannel              ProSLIC_PowerDownConverter
#define Si3228x_PowerDownConverter           ProSLIC_PowerDownConverter
#define Si3228x_Calibrate                    ProSLIC_Calibrate
#define Si3228x_SetLinefeedStatusBroadcast   ProSLIC_SetLinefeedStatusBroadcast
#define Si3228x_SetLinefeedStatus            ProSLIC_SetLinefeedStatus
#define Si3228x_MWIEnable                    ProSLIC_MWIEnable
#define Si3228x_MWIDisable                   ProSLIC_MWIDisable
#define Si3228x_SetMWIState                  ProSLIC_SetMWIState
#define Si3228x_GetMWIState                  ProSLIC_GetMWIState
#define Si3228x_MWISetup(PCHAN,VPK,LCR)      ProSLIC_MWISetV(PCHAN,VPK)

/* DC Feed */
#ifndef DISABLE_DCFEED_SETUP
extern Si3228x_DCfeed_Cfg Si3228x_DCfeed_Presets[];
#endif


/*
**
** PROSLIC INITIALIZATION FUNCTIONS
**
*/

/*
** Function: PROSLIC_Init_MultiBOM
**
** Description:
** Initializes the ProSLIC w/ selected general parameters
**
** Input Parameters:
** pProslic: pointer to PROSLIC object
** size:     number of channels
** preset:   general configuration preset
**
** Return:
** none
*/
int Si3228x_Init_MultiBOM (proslicChanType_ptr *hProslic,int size,int preset);

/*
** Function: Si3228x_Init_with_Options
**
** Description:
** Initializes the ProSLIC with an option.
**
** Input Parameters:
** pProslic: pointer to PROSLIC object
** size - number of continuous channels to initialize
** init_opt - which initialization type to do.
**
** Return:
** none
*/
int Si3228x_Init_with_Options (proslicChanType_ptr *pProslic, int size,
                               initOptionsType init_opt);

/*
** Function: PROSLIC_VerifyControlInterface
**
** Description:
** Verify SPI port read capabilities
**
** Input Parameters:
** pProslic: pointer to PROSLIC object
**
** Return:
** none
*/
int Si3228x_VerifyControlInterface (proslicChanType_ptr hProslic);

/*
** Function: Si3228x_PowerUpConverter
**
** Description:
** Powers all DC/DC converters sequentially with delay to minimize
** peak power draw on VDC.
**
** Returns:
** int (error)
**
*/
int Si3228x_PowerUpConverter(proslicChanType_ptr hProslic);

/*
** Function: PROSLIC_EnableInterrupts
**
** Description:
** Enables interrupts
**
** Input Parameters:
** hProslic: pointer to Proslic object
**
** Return:
**
*/
int Si3228x_EnableInterrupts (proslicChanType_ptr hProslic);

/*
**
** PROSLIC CONFIGURATION FUNCTIONS
**
*/

/*
** Function: PROSLIC_RingSetup
**
** Description:
** configure ringing
**
** Input Parameters:
** pProslic: pointer to Proslic object
** pRingSetup: pointer to ringing config structure
**
** Return:
** none
*/
int Si3228x_RingSetup (proslicChanType *pProslic, int preset);

/*
** Function: PROSLIC_ZsynthSetup
**
** Description:
** configure impedance synthesis
**
** Input Parameters:
** pProslic: pointer to Proslic object
** pZynth: pointer to zsynth config structure
**
** Return:
** none
*/
int Si3228x_ZsynthSetup (proslicChanType *pProslic, int preset);

/*
** Function: PROSLIC_AudioGainSetup
**
** Description:
** configure audio gains
**
** Input Parameters:
** pProslic: pointer to Proslic object
** pAudio: pointer to audio gains config structure
**
** Return:
** none
*/
int Si3228x_TXAudioGainSetup (proslicChanType *pProslic, int preset);
int Si3228x_RXAudioGainSetup (proslicChanType *pProslic, int preset);
#define Si3228x_AudioGainSetup ProSLIC_AudioGainSetup
int Si3228x_TXAudioGainScale (proslicChanType *pProslic, int preset,
                              uInt32 pga_scale, uInt32 eq_scale);
int Si3228x_RXAudioGainScale (proslicChanType *pProslic, int preset,
                              uInt32 pga_scale, uInt32 eq_scale);

/*
** Function: PROSLIC_DCFeedSetup
**
** Description:
** configure dc feed
**
** Input Parameters:
** pProslic: pointer to Proslic object
** pDcFeed: pointer to dc feed config structure
**
** Return:
** none
*/
int Si3228x_DCFeedSetupCfg (proslicChanType *pProslic,ProSLIC_DCfeed_Cfg *cfg,
                            int preset);

/*
** Function: PROSLIC_PCMSetup
**
** Description:
** configure pcm
**
** Input Parameters:
** pProslic: pointer to Proslic object
** pPcm: pointer to pcm config structure
**
** Return:
** none
*/
int Si3228x_PCMSetup (proslicChanType *pProslic, int preset);

/*
**
** PROSLIC CONTROL FUNCTIONS
**
*/

/*
** Function: PROSLIC_PulseMeterSetup
**
** Description:
** configure pulse metering
**
** Input Parameters:
** pProslic: pointer to Proslic object
** pPulseCfg: pointer to pulse metering config structure
**
** Return:
** none
*/
int Si3228x_PulseMeterSetup (proslicChanType *pProslic, int preset);

/*
** Function: PROSLIC_dbgSetDCFeed
**
** Description:
** provisionary function for setting up
** dcfeed given desired open circuit voltage
** and loop current.
*/
int Si3228x_dbgSetDCFeed (proslicChanType *pProslic, uInt32 v_vlim_val,
                          uInt32 i_ilim_val, int32 preset);

/*
** Function: PROSLIC_dbgSetDCFeedVopen
**
** Description:
** provisionary function for setting up
** dcfeed given desired open circuit voltage
** and loop current.
*/
int Si3228x_dbgSetDCFeedVopen (proslicChanType *pProslic, uInt32 v_vlim_val,
                               int32 preset);


/*
** Function: PROSLIC_dbgSetDCFeedIloop
**
** Description:
** provisionary function for setting up
** dcfeed given desired open circuit voltage
** and loop current.
*/
int Si3228x_dbgSetDCFeedIloop (proslicChanType *pProslic, uInt32 i_ilim_val,
                               int32 preset);


/*
** Function: PROSLIC_dbgRingingSetup
**
** Description:
** Provisionary function for setting up
** Ring type, frequency, amplitude and dc offset.
** Main use will be by peek/poke applications.
*/
int Si3228x_dbgSetRinging (proslicChanType *pProslic,
                           ProSLIC_dbgRingCfg *ringCfg, int preset);

/*
** Function: PROSLIC_dbgSetRXGain
**
** Description:
** Provisionary function for setting up
** RX path gain.
*/
int Si3228x_dbgSetRXGain (proslicChanType *pProslic, int32 gain,
                          int impedance_preset, int audio_gain_preset);

/*
** Function: PROSLIC_dbgSetTXGain
**
** Description:
** Provisionary function for setting up
** TX path gain.
*/
int Si3228x_dbgSetTXGain (proslicChanType *pProslic, int32 gain,
                          int impedance_preset, int audio_gain_preset);


/*
** Function: PROSLIC_LineMonitor
**
** Description:
** Monitor line voltages and currents
*/
int Si3228x_LineMonitor(proslicChanType *pProslic, proslicMonitorType *monitor);


/*
** Function: PROSLIC_PSTNCheck
**
** Description:
** Continuous monitor of ilong to detect hot pstn line
*/
int Si3228x_PSTNCheck(proslicChanType *pProslic,
                      proslicPSTNCheckObjType *pstnCheckObj);

/*
** Function: PROSLIC_DiffPSTNCheck
**
** Description:
** Detection of foreign PSTN
*/
int Si3228x_DiffPSTNCheck (proslicChanType *pProslic,
                           proslicDiffPSTNCheckObjType *pPSTNCheck);

/*
** Function: PROSLIC_SetPowersaveMode
**
** Description:
** Enable or Disable powersave mode
*/
int Si3228x_SetPowersaveMode(proslicChanType *pProslic, int pwrsave);

/*
** Function: PROSLIC_ReadMADCScaled
**
** Description:
** ReadMADC (or other sensed voltage/currents) and
** return scaled value in int32 format
*/
int32 Si3228x_ReadMADCScaled(proslicChanType *pProslic, uInt16 addr,
                             int32 scale);
/* 
** Function: Si3228x_GetChipInfo
** Description: Returns the specific chipset in the Si3228x family.
** Input parameters: channel pointer
** output: either RC_SPI_FAIL or RC_NONE.  channel pointer deviceId will be updated.
**/
int Si3228x_GetChipInfo(proslicChanType_ptr pProslic);

#endif

