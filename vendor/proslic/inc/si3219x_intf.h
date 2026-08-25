/*
** Copyright (c) 2017 by Silicon Laboratories
**
** $Id: si3218x_intf.h 6337 2017-03-15 20:30:45Z nizajerk $
**
** Si3219x ProSLIC interface header file
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

#ifndef SI3219X_INTF_H
#define SI3219X_INTF_H

#include "si3219x.h"

/* DC Feed */
#ifndef DISABLE_DCFEED_SETUP
extern Si3219x_DCfeed_Cfg Si3219x_DCfeed_Presets[];
#endif

#define Si3219x_TXAudioGainScale(CHPTR, PRESET_NDX, SCALE, EQ_SCALE)\
  Si3219x_AudioGainScale((CHPTR),(PRESET_NDX),(SCALE),(EQ_SCALE),TXACGAIN_SEL)
#define Si3219x_RXAudioGainScale(CHPTR, PRESET_NDX, SCALE, EQ_SCALE)\
  Si3219x_AudioGainScale((CHPTR),(PRESET_NDX),(SCALE),(EQ_SCALE),RXACGAIN_SEL)


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
** preset:   General configuration preset
**
** Return:
** none
*/
#ifdef SIVOICE_MULTI_BOM_SUPPORT
int Si3219x_Init_MultiBOM (proslicChanType_ptr *hProslic,int size,int preset);
#endif

/*
** Function: Si3219x_Init_with_Options
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
int Si3219x_Init_with_Options (proslicChanType_ptr *pProslic, int size,
                               initOptionsType init_opt);

/*
** Function: Si3219x_PowerUpConverter
**
** Description:
** Powers all DC/DC converters sequentially with delay to minimize
** peak power draw on VDC.
**
** Returns:
** int (error)
**
*/
int Si3219x_PowerUpConverter(proslicChanType_ptr hProslic);

/*
** Function: Si3219x_PowerDownConverter
**
** Description:
** Power down DCDC converter (selected channel only)
**
** Returns:
** int (error)
**
*/
int Si3219x_PowerDownConverter(proslicChanType_ptr hProslic);

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
int Si3219x_EnableInterrupts (proslicChanType_ptr hProslic);

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
int Si3219x_RingSetup (proslicChanType *pProslic, int preset);

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
int Si3219x_ZsynthSetup (proslicChanType *pProslic, int preset);

/*
** Function: PROSLIC_AudioGainSetup
**
** Description:
** configure audio gains
**
** Input Parameters:
** pProslic: pointer to Proslic object
** preset:   impedance preset to scale
**
** Return:
** none
*/
int Si3219x_TXAudioGainSetup (proslicChanType *pProslic, int preset);
int Si3219x_RXAudioGainSetup (proslicChanType *pProslic, int preset);

/*
** Function: Si3219x_AudioGainScale
**
** Description:
** Multiply path gain by passed value for PGA and EQ scale (no reference to dB,
** multiply by a scale factor)
*/

int Si3219x_AudioGainScale (proslicChanType *pProslic, int preset,
                            uInt32 pga_scale, uInt32 eq_scale,int rx_tx_sel);

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
int Si3219x_DCFeedSetupCfg (proslicChanType *pProslic, ProSLIC_DCfeed_Cfg *cfg,
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
int Si3219x_PCMSetup(proslicChanType *pProslic, int preset);

/*
**
** PROSLIC CONTROL FUNCTIONS
**
*/

/*
** Function: PROSLIC_GetInterrupts
**
** Description:
** Enables interrupts
**
** Input Parameters:
** hProslic: pointer to Proslic object
** pIntData: pointer to interrupt info retrieved
**
** Return:
**
*/
int Si3219x_GetInterrupts (proslicChanType_ptr hProslic,
                           proslicIntType *pIntData);

/*
** Function: ProSLIC_SetMWIState_ramp
**
** Description:
** Set MWI state
**
** Input Parameters:
** pProslic: pointer to Proslic object
** flash_on: 0 = low, 1 = high (VBATH_NEON)
** step_delay: delay between VBATH steps (ms)
** step_num: number of steps between low and high states
**
** Return:
** none
*/
int Si3219x_SetMWIState_ramp (proslicChanType *pProslic,uInt8 flash_on,
                              uInt8 step_delay,uInt8 step_num);

/*
** Function: PROSLIC_MWI
**
** Description:
** implements message waiting indicator
**
** Input Parameters:
** pProslic: pointer to Proslic object
** lampOn: 0 = turn lamp off, 1 = turn lamp on
**
** Return:
** none
**
** Use Deprecated.
*/
int Si3219x_MWI (proslicChanType *pProslic,uInt8 lampOn);

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
int Si3219x_PulseMeterSetup (proslicChanType *pProslic, int preset);

/*
** Function: PROSLIC_dbgSetDCFeed
**
** Description:
** provision function for setting up
** dcfeed given desired open circuit voltage
** and loop current.
*/
int Si3219x_dbgSetDCFeed (proslicChanType *pProslic, uInt32 v_vlim_val,
                          uInt32 i_ilim_val, int32 preset);

/*
** Function: PROSLIC_dbgSetDCFeedVopen
**
** Description:
** provision function for setting up
** dcfeed given desired open circuit voltage
** and loop current.
*/
int Si3219x_dbgSetDCFeedVopen (proslicChanType *pProslic, uInt32 v_vlim_val,
                               int32 preset);


/*
** Function: PROSLIC_dbgSetDCFeedIloop
**
** Description:
** provision function for setting up
** dcfeed given desired open circuit voltage
** and loop current.
*/
int Si3219x_dbgSetDCFeedIloop (proslicChanType *pProslic, uInt32 i_ilim_val,
                               int32 preset);


/*
** Function: PROSLIC_dbgRingingSetup
**
** Description:
** Provision function for setting up
** Ring type, frequency, amplitude and dc offset.
** Main use will be by peek/poke applications.
*/
int Si3219x_dbgSetRinging (proslicChanType *pProslic,
                           ProSLIC_dbgRingCfg *ringCfg, int preset);

/*
** Function: PROSLIC_dbgSetRXGain
**
** Description:
** Provision function for setting up
** RX path gain.
*/
int Si3219x_dbgSetRXGain (proslicChanType *pProslic, int32 gain,
                          int impedance_preset, int audio_gain_preset);

/*
** Function: PROSLIC_dbgSetTXGain
**
** Description:
** Provision function for setting up
** TX path gain.
*/
int Si3219x_dbgSetTXGain (proslicChanType *pProslic, int32 gain,
                          int impedance_preset, int audio_gain_preset);

/*
** Function: PROSLIC_LineMonitor
**
** Description:
** Monitor line voltages and currents
*/
int Si3219x_LineMonitor(proslicChanType *pProslic, proslicMonitorType *monitor);

/*
** Function: PROSLIC_PSTNCheck
**
** Description:
** Continuous monitor of ilong to detect hot pstn line
*/
int Si3219x_PSTNCheck(proslicChanType *pProslic,
                      proslicPSTNCheckObjType *pstnCheckObj);

/*
** Function: PROSLIC_PSTNCheck
**
** Description:
** Continuous monitor of ilong to detect hot pstn line
*/
int Si3219x_DiffPSTNCheck(proslicChanType *pProslic,
                          proslicDiffPSTNCheckObjType *pstnCheckObj);

/*
** Function: PROSLIC_ReadMADCScaled
**
** Description:
** Read MADC (or other sensed voltages/currents) and
** return scaled value in int32 format
*/
int32 Si3219x_ReadMADCScaled(proslicChanType_ptr pProslic, uInt16 addr,
                             int32 scale);

/*
** Function: Si3219x8_PulseMeterSetup
**
** Description:
** configure pulse metering
*/
int Si3219x_PulseMeterSetup (proslicChanType_ptr hProslic, int preset);

/* 
** Function: Si3219x_GetChipInfo
** Description: Returns the specific chipset in the Si3219x family.
** Input parameters: channel pointer
** output: either RC_SPI_FAIL or RC_NONE.  channel pointer deviceId will be updated.
**/
int Si3219x_GetChipInfo(proslicChanType_ptr pProslic);

#endif

