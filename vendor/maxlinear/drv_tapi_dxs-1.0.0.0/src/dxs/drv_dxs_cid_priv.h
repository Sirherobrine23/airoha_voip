#ifndef _DRV_DXS_CID_PRIV_H
#define _DRV_DXS_CID_PRIV_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_cid_priv.h
   This file contains the defines, the structures declarations for caller ID
   sender module of DUSLIC XS.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
#define DXS_CID_STATE_NO_STATES                 3

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
/** CID state machine states */
typedef enum
{
   /* initial state of the callerID state handler */
   DXS_CID_STATE_SETUP,
   /* callerID data is transmitted */
   DXS_CID_STATE_TRANSMIT,
   /* all callerID data is sent to device */
   DXS_CID_STATE_TRANSMIT_END
} DXS_CID_STATES_t;

/** CID sending structure */
struct DXS_CID_SEND
{
   /* array to store the CID bytes */
   IFX_uint8_t                pCid [258];
   /* Caller ID transmission data type */
   IFX_TAPI_CID_DATA_TYPE_t  nCidDataType;
   /* number of CID bytes to be transmitted */
   IFX_vuint8_t               nCidCnt;
   /* current position in array pCid */
   IFX_vuint8_t               nPos;
   /* current state of the cid state machine */
   DXS_CID_STATES_t           state_cid;
};

/**
   Structure for the CID information, including firmware message cache
*/
struct DXS_CID_GEN
{
   /* controls the CID sender */
   DXS_CID_GEN_CTRL_t      cid_gen_ctrl;
   /* determines the data for the CID sender */
   DXS_CID_GEN_DATA_t      cid_gen_data;
   /* determines the coefficients (level, seizure, mark) for the CID sender */
   DXS_CID_GEN_COEF_t      cid_gen_coef;
   /* CID sender structure, need protection  */
   struct DXS_CID_SEND     cidSend;
};

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

#endif /* _DRV_DXS_CID_PRIV_H */
