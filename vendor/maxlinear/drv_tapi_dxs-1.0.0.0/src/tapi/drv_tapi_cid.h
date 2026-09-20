#ifndef _DRV_TAPI_CID_H
#define _DRV_TAPI_CID_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

/**
   \file drv_tapi_cid.h
   Interface of the TAPI caller id implementation.
   This file contains the declaration of the functions for the CID module.
*/

#ifdef TAPI_FEAT_CID

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

extern IFX_int32_t IFX_TAPI_CID_SetPredefAlertTones (
                        void);

extern IFX_int32_t IFX_TAPI_CID_Initialise_Unprot (
                        TAPI_CHANNEL *pChannel);

extern IFX_void_t  IFX_TAPI_CID_Cleanup (
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t TAPI_Phone_CID_SetConfig (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_CID_CFG_t const *pCidConf);

extern IFX_int32_t TAPI_Phone_CID_SetConfig_Base (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_CID_CFG_t const *pCidConf,
                        IFX_boolean_t const *bCompatIoctl);

extern IFX_int32_t TAPI_Phone_CID_Info_Tx (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_CID_MSG_t const *pCidInfo);

extern IFX_int32_t TAPI_Phone_CID_Info_Tx_Base (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_CID_MSG_t const *pCidInfo,
                        IFX_boolean_t const *bCompatIoctl);

extern IFX_int32_t TAPI_Phone_CID_Seq_Tx (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_CID_MSG_t const *pCidInfo);

extern IFX_int32_t TAPI_Phone_CID_Seq_Tx_Base (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_CID_MSG_t const *pCidInfo,
                        IFX_boolean_t const *bCompatIoctl);

extern IFX_int32_t TAPI_Phone_CID_Stop_Tx(
                        TAPI_CHANNEL *pChannel);

extern IFX_boolean_t TAPI_Cid_UseSequence (
                        TAPI_CHANNEL *pChannel);

/* trigger function called by ringing to indicate a ring pause */
extern IFX_void_t  IFX_TAPI_CID_OnRingpause (
                        TAPI_CHANNEL *pChannel);

/* trigger function to indicate a hook event to CID */
extern IFX_void_t  IFX_TAPI_CID_OnHookEvent (
                        TAPI_CHANNEL *pChannel,
                        IFX_boolean_t bOffhook,
                        IFX_boolean_t *bSendHookEvent);

/* trigger function to indicate a DTMF event to CID */
extern IFX_boolean_t IFX_TAPI_CID_OnDtmfEvent (
                        TAPI_CHANNEL *pChannel,
                        IFX_uint8_t nDtmfAscii);

extern IFX_int32_t TAPI_Cid_RingStart (
                        TAPI_CHANNEL *pChannel);

#endif /* TAPI_FEAT_CID */

#endif /* _DRV_TAPI_CID_H */
