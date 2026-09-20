#ifndef DRV_TAPI_COMPAT_H
#define DRV_TAPI_COMPAT_H
/******************************************************************************

  Copyright 2018, Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_compat.h
   This file contains structs and defines used to translate the arguments of
   IOCTLs from 32-bit architecture to 64-bit architecture. This is used on
   Linux systems running with 32-bit user-space and 64-bit kernel-space
   architecture.
*/

#include "drv_tapi_config.h"
#ifdef TAPI_FEAT_LX_COMPAT

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

/* Include type definitons */
#include <ifx_types.h>
/* Linux compat datatypes and conversion macros */
#include <linux/compat.h>
/* TAPI API has the typedefs used in the structs below */
#include "drv_tapi_io.h"
/* Automatically generated TAPI ioctl command indexes for defining
   the compatible command IDs */
#include "drv_tapi_io_indexes.h"
/* Driver internal types used in prototypes */
#include "drv_tapi.h"
#ifdef TAPI_FEAT_CID
#include "drv_tapi_cid.h"
#endif /* TAPI_FEAT_CID */
/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/* Magic number for ioctls.*/
#define IFX_TAPI_IOC_MAGIC 'q'

/* IOCTL command ID definitions for 64-bit architecture to handle IOCTLs
   from 32-bit architecture. */

#define IFX_TAPI_CH_INIT_32 \
   _IOW(IFX_TAPI_IOC_MAGIC, IFX_TAPI_CH_INIT_IDX, \
         struct IFX_TAPI_CH_INIT_32_t)

#define IFX_TAPI_CID_TX_INFO_START_32 \
   _IOW(IFX_TAPI_IOC_MAGIC, IFX_TAPI_CID_TX_INFO_START_IDX, \
         struct IFX_TAPI_CID_MSG_32_t)

#define IFX_TAPI_CID_TX_SEQ_START_32 \
   _IOW(IFX_TAPI_IOC_MAGIC, IFX_TAPI_CID_TX_SEQ_START_IDX, \
         struct IFX_TAPI_CID_MSG_32_t)

#define IFX_TAPI_CID_CFG_SET_32 \
   _IOW(IFX_TAPI_IOC_MAGIC, IFX_TAPI_CID_CFG_SET_IDX, \
         struct IFX_TAPI_CID_CFG_32_t)

#define IFX_TAPI_CAP_LIST_32 \
   _IOWR(IFX_TAPI_IOC_MAGIC, IFX_TAPI_CAP_LIST_IDX, compat_uptr_t)

#define IFX_TAPI_CAP_NLIST_32 \
   _IOWR(IFX_TAPI_IOC_MAGIC, IFX_TAPI_CAP_NLIST_IDX, \
         struct IFX_TAPI_CAP_LIST_32_t)

#define IFX_TAPI_EVENT_GET_32 \
   _IOR(IFX_TAPI_IOC_MAGIC, IFX_TAPI_EVENT_GET_IDX, \
         struct IFX_TAPI_EVENT_32_t)

#define IFX_TAPI_EVENT_ENABLE_32 \
   _IOW(IFX_TAPI_IOC_MAGIC, IFX_TAPI_EVENT_ENABLE_IDX, \
         struct IFX_TAPI_EVENT_32_t)

#define IFX_TAPI_EVENT_DISABLE_32 \
   _IOW(IFX_TAPI_IOC_MAGIC, IFX_TAPI_EVENT_DISABLE_IDX, \
         struct IFX_TAPI_EVENT_32_t)

#define IFX_TAPI_EVENT_MULTI_ENABLE_32 \
   _IOW(IFX_TAPI_IOC_MAGIC, IFX_TAPI_EVENT_MULTI_ENABLE_IDX, \
         struct IFX_TAPI_EVENT_MULTI_32_t)

#define IFX_TAPI_EVENT_MULTI_DISABLE_32 \
   _IOW(IFX_TAPI_IOC_MAGIC, IFX_TAPI_EVENT_MULTI_DISABLE_IDX, \
         struct IFX_TAPI_EVENT_MULTI_32_t)

#define IFX_TAPI_NLT_TEST_START_32 \
   _IOWR(IFX_TAPI_IOC_MAGIC, IFX_TAPI_NLT_TEST_START_IDX, \
         struct IFX_TAPI_NLT_TEST_START_32_t)

#define IFX_TAPI_NLT_RESULT_GET_32 \
   _IOWR(IFX_TAPI_IOC_MAGIC, IFX_TAPI_NLT_RESULT_GET_IDX, \
         struct IFX_TAPI_NLT_RESULT_GET_32_t)


/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/** TAPI initialization structure used for \ref IFX_TAPI_CH_INIT
    on 64-bit architecture to handle IOCTLs from 32-bit architecture. */
struct IFX_TAPI_CH_INIT_32_t
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index.                            \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel 'module' index.                  \endtapiv4 */
   IFX_uint16_t ch;
   /** Channel initialization mode, select from \ref IFX_TAPI_INIT_MODE_t;
       this should be always set to \ref IFX_TAPI_INIT_MODE_VOICE_CODER. */
   IFX_uint8_t nMode;
   /** Reserved; country selection for future purposes. */
   IFX_uint8_t nCountry;
   /** Pointer to the low-level device initialization structure (for example
       VMMC_IO_INIT for XWAY INCA-IP2). For more details, refer to the
       device-specific driver documentation. */
   /* IFX_void_t  *pProc; original declaration */
   compat_uptr_t pProc; /* compat declaration */
};


/** Structure for service type transparent (\ref IFX_TAPI_CID_SERVICE_TYPE_t).*/
struct IFX_TAPI_CID_MSG_TRANSPARENT_32_t
{
   /** Element type. */
   IFX_TAPI_CID_SERVICE_TYPE_t   elementType;
   /** Element length. */
   IFX_uint32_t                  len;
   /** Element buffer. */
   compat_uptr_t                 data;
};


/** Union of element types. */
union IFX_TAPI_CID_MSG_ELEMENT_32_t
{
   /** Message element including date and time information. */
   IFX_TAPI_CID_MSG_DATE_t          date;
   /** Message element formatted as a string.*/
   IFX_TAPI_CID_MSG_STRING_t        string;
   /** Message element formatted as a value.*/
   IFX_TAPI_CID_MSG_VALUE_t         value;
   /** Message element to be sent with transparent transmission. */
   struct IFX_TAPI_CID_MSG_TRANSPARENT_32_t   transparent;
};


/** Structure containing CID configuration possibilities
    on 64-bit architecture to handle IOCTLs from 32-bit architecture. */
struct  IFX_TAPI_CID_CFG_32_t
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index.                            \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel 'module' index.                  \endtapiv4 */
   IFX_uint16_t ch;
   /** Standard used (enumerated in IFX_TAPI_CID_STD_t);
       default is IFX_TAPI_CID_STD_TELCORDIA.*/
   IFX_TAPI_CID_STD_t       nStandard;
   /** Union of the different standards; default IFX_TAPI_CID_STD_TELCORDIA_t.*/
   /* IFX_TAPI_CID_STD_TYPE_t  *cfg; original declaration */
   compat_uptr_t cfg; /* compat declaration */
};


/** Structure containing the CID message type and content as well as
    information about transmission mode. This structure contains all information
    required by IFX_TAPI_CID_TX_INFO_START to start CID generation.
    Definition for 64-bit architecture to handle IOCTLs from 32-bit
    architecture. */
struct IFX_TAPI_CID_MSG_32_t
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index.                            \endtapiv4 */
   IFX_uint16_t dev;
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Channel 'module' index.                  \endtapiv4 */
   IFX_uint16_t ch;
   /** Defines the transmission mode
       (enumerated in \ref IFX_TAPI_CID_HOOK_MODE_t).
       The default is \ref IFX_TAPI_CID_HM_ONHOOK. */
   IFX_TAPI_CID_HOOK_MODE_t      txMode;
   /** Defines the message type to be displayed
       (enumerated in \ref IFX_TAPI_CID_MSG_TYPE_t). */
   IFX_TAPI_CID_MSG_TYPE_t       messageType;
   /** Number of elements of the message array. */
   IFX_uint32_t                  nMsgElements;
   /** Message array. */
   /* IFX_TAPI_CID_MSG_ELEMENT_t    *message; original declaration */
   compat_uptr_t message; /* compat declaration */
};


/** Structure containing CID configuration for the Telcordia standard
    on 64-bit architecture to handle IOCTLs from 32-bit architecture. */
struct IFX_TAPI_CID_STD_TELCORDIA_32_t
{
   /** Pointer to a structure containing timing information. If the parameter
    is not given, \ref IFX_TAPI_CID_TIMING_t default values will be used. */
   compat_uptr_t           pCIDTiming;
   /** Pointer to a structure containing FSK configuration parameters. If the
    parameter is not given, \ref IFX_TAPI_CID_FSK_CFG_t default values will be used.*/
   compat_uptr_t           pFSKConf;
   /** Use of OSI for off-hook transmission; default IFX_FALSE.*/
   IFX_uint32_t            OSIoffhook;
   /** Length of the OSI signal in ms; default 200 ms.*/
   IFX_uint32_t            OSItime;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t            nAlertToneOnhook;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t            nAlertToneOffhook;
   /** DTMF ACK after CAS, used for off-hook transmission; default DTMF 'D'.
       This acknowledge tone can be set to the DTMF tones 'A', 'B', 'C' or 'D'.
       Setting this parameter to zero means that any one of the four tones is
       accepted. */
   IFX_char_t              ackTone;
   /** Tone table index for the subscriber alerting signal (SAS) to be used
       in off-hook transmissions. By default, the TAPI plays no SAS tone */
   IFX_uint32_t            nSAStone;
   /** Time to wait before a SAS tone, in ms, for off-hook services.
       The default is 20 ms when OSI is not used, and 100 ms when OSI is used. */
   IFX_uint32_t            beforeSAStime;
   /** Time to wait between generation of SAS and CAS tone, in ms;
       default 20 ms. */
   IFX_uint32_t            SAS2CAStime;
};


/** Structure containing CID configuration for the ETSI standard using FSK
    transmission  on 64-bit architecture to handle IOCTLs
    from 32-bit architecture. */
struct IFX_TAPI_CID_STD_ETSI_FSK_32_t
{
   /** Pointer to a structure containing timing information. If the parameter
    is not given, \ref IFX_TAPI_CID_TIMING_t default values will be used.*/
   compat_uptr_t                 pCIDTiming;
   /** Pointer to a structure containing FSK configuration parameters. If the
    parameter is not given, \ref IFX_TAPI_CID_FSK_CFG_t default values will be used.*/
   compat_uptr_t                 pFSKConf;
   /** Type of ETSI alert of on-hook services associated with ringing
    (enumerated in IFX_TAPI_CID_ALERT_ETSI_t); default
    IFX_TAPI_CID_ALERT_ETSI_FR.*/
   IFX_TAPI_CID_ALERT_ETSI_t     nETSIAlertRing;
   /** Type of ETSI alert of on-hook services not associated with ringing
    (enumerated in IFX_TAPI_CID_ALERT_ETSI_t); default
     IFX_TAPI_CID_ALERT_ETSI_RP.*/
   IFX_TAPI_CID_ALERT_ETSI_t     nETSIAlertNoRing;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t                  nAlertToneOnhook;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t                  nAlertToneOffhook;
   /** Duration of ring pulse, in ms; default 500 ms.*/
   IFX_uint32_t                  ringPulseTime;
   /** DTMF ACK after CAS, used for off-hook transmission; default DTMF is D.
       This acknowledge tone can be set to the DTMF tones 'A', 'B', 'C' or 'D'.
       Setting this parameter to zero means that any one of the four tones is
       accepted. */
   IFX_char_t                    ackTone;
};


/** Structure containing CID configuration for the ETSI standard using DTMF
    transmission on 64-bit architecture to handle IOCTLs
    from 32-bit architecture. */
struct IFX_TAPI_CID_STD_ETSI_DTMF_32_t
{
   /** Pointer to a structure containing timing information. If the parameter
    is not given, \ref IFX_TAPI_CID_TIMING_t default values will be used. */
   compat_uptr_t               pCIDTiming;
   /** Pointer to a structure containing DTMF configuration parameters. If the
    parameter is not given, \ref IFX_TAPI_CID_DTMF_CFG_t default values will be
   used. */
   compat_uptr_t               pDTMFConf;
   /** Pointer to a structure containing the coding for the
   absence reason of the calling number.*/
   compat_uptr_t               pABSCLICode;
   /** Type of ETSI alert of on-hook services associated with ringing (enumerated
    in \ref IFX_TAPI_CID_ALERT_ETSI_t); default \ref IFX_TAPI_CID_ALERT_ETSI_FR.*/
   IFX_TAPI_CID_ALERT_ETSI_t   nETSIAlertRing;
   /** Type of ETSI alert of on-hook services not associated with ringing
    (enumerated in \ref IFX_TAPI_CID_ALERT_ETSI_t); default \ref
    IFX_TAPI_CID_ALERT_ETSI_RP. */
   IFX_TAPI_CID_ALERT_ETSI_t   nETSIAlertNoRing;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t                nAlertToneOnhook;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition. */
   IFX_uint32_t                nAlertToneOffhook;
   /** Duration of ring pulse, in ms; default is 500 ms. */
   IFX_uint32_t                ringPulseTime;
   /** DTMF ACK after CAS, used for off-hook transmission; default DTMF D.
       This acknowledge tone can be set to the DTMF tones 'A', 'B', 'C' or 'D'.
       Setting this parameter to zero means that any one of the four tones is
       accepted. */
   IFX_char_t                  ackTone;
};


/** Structure for the configuration of the SIN standard on 64-bit architecture
 *  to handle IOCTLs from 32-bit architecture. */
struct IFX_TAPI_CID_STD_SIN_32_t
{
   /** Pointer to a structure containing timing information.
      If the parameter is NULL, default values will be used. */
   compat_uptr_t           pCIDTiming;
   /** Pointer to a structure containing FSK configuration parameters.
      If the parameter is NULL, default values will be used. */
   compat_uptr_t           pFSKConf;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t            nAlertToneOnhook;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t            nAlertToneOffhook;
   /** DTMF ACK after CAS, used for off-hook transmission; default DTMF 'D'.
       This acknowledge tone can be set to the DTMF tones 'A', 'B', 'C' or 'D'.
       Setting this parameter to zero means that any one of the four tones is
       accepted. */
   IFX_char_t              ackTone;
};


/** Structure containing the CID configuration for the NTT standard
 *  on 64-bit architecture to handle IOCTLs from 32-bit architecture. */
struct IFX_TAPI_CID_STD_NTT_32_t
{
   /** Pointer to a structure containing timing information. If the parameter
    is not given, \ref IFX_TAPI_CID_TIMING_t default values will be used.*/
   compat_uptr_t           pCIDTiming;
   /** Pointer to a structure containing FSK configuration parameters. If the
    parameter is not given, \ref IFX_TAPI_CID_FSK_CFG_t default values will be used.*/
   compat_uptr_t           pFSKConf;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t            nAlertToneOnhook;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t            nAlertToneOffhook;
   /** Ring pulse on time (CAR signal), in ms; default 500 ms.*/
   IFX_uint32_t            ringPulseTime;
   /** Maximum number of ring pulses (CAR signals); default 5.*/
   IFX_uint32_t            ringPulseLoop;
   /** Ring pulse off-time (CAR signal), in ms; default 500 ms.*/
   IFX_uint32_t            ringPulseOffTime;
   /** Time-out for incoming successful signal to arrive after CID data
    transmission is completed; default 7000 ms.*/
   IFX_uint32_t            dataOut2incomingSuccessfulTimeout;
};


/** Structure containing CID configuration for the KPN standard, with DTMF
    and FSK transmission. Required on 64-bit architecture to handle IOCTLs
    from 32-bit architecture. */
struct IFX_TAPI_CID_STD_KPN_DTMF_FSK_32_t
{
   /** Pointer to a structure containing timing information. If the parameter
    is not given, IFX_TAPI_CID_TIMING_t default values will be used.*/
   compat_uptr_t                 pCIDTiming;
   /** Pointer to a structure containing DTMF configuration parameters. If the
    parameter is not given, \ref IFX_TAPI_CID_DTMF_CFG_t default values will be
   used. */
   compat_uptr_t                 pDTMFConf;
   /** Pointer to a structure containing the coding for the
   absence reason of the calling number.*/
   compat_uptr_t                 pABSCLICode;
   /** Pointer to a structure containing FSK configuration parameters. If the
    parameter is not given, IFX_TAPI_CID_FSK_CFG_t default values will be used.*/
   compat_uptr_t                 pFSKConf;
   /** Tone table index for the alert tone to be used. Required for automatic
    CID/MWI generation. By default, TAPI uses an internal tone definition.*/
   IFX_uint32_t                  nAlertToneOffhook;
   /** DTMF ACK after CAS, used for off-hook transmission; default DTMF is D.
       This acknowledge tone can be set to the DTMF tones 'A', 'B', 'C' or 'D'.
       Setting this parameter to zero means that any one of the four tones is
       accepted. */
   IFX_char_t                    ackTone;
};


/** Structure containing CID configuration for the KPN standard, with DTMF
    transmission. Required on 64-bit architecture to handle IOCTLs
    from 32-bit architecture. */
struct IFX_TAPI_CID_STD_KPN_DTMF_32_t
{
   /** Pointer to a structure containing timing information. If the parameter
    is not given, IFX_TAPI_CID_TIMING_t default values will be used.*/
   compat_uptr_t                 pCIDTiming;
   /** Pointer to a structure containing DTMF configuration parameters. If the
    parameter is not given, \ref IFX_TAPI_CID_DTMF_CFG_t default values will be
   used. */
   compat_uptr_t                 pDTMFConf;
   /** Pointer to a structure containing the coding for the
   absence reason of the calling number.*/
   compat_uptr_t                 pABSCLICode;
   /** Pointer to a structure containing FSK configuration parameters. If the
    parameter is not given, IFX_TAPI_CID_FSK_CFG_t default values will be used.*/
   compat_uptr_t                 pFSKConf;
   /** Tone table index for the alert tone to be used; required for automatic
    CID/MWI generation. By default, the TAPI uses an internal tone definition.*/
   IFX_uint32_t                  nAlertToneOffhook;
   /** DTMF ACK after CAS, used for off-hook transmission; default DTMF is D.
       This acknowledge tone can be set to the DTMF tones 'A', 'B', 'C' or 'D'.
       Setting this parameter to zero means that any one of the four tones is
       accepted. */
   IFX_char_t                    ackTone;
};


/** Structure containing CID configuration for the TDC standard.
    Required on 64-bit architecture to handle IOCTLs from 32-bit architecture. */
struct IFX_TAPI_CID_STD_TDC_32_t
{
   /** Pointer to a structure containing timing information. If the parameter
    is not given, IFX_TAPI_CID_TIMING_t default values will be used.*/
   compat_uptr_t                 pCIDTiming;
   /** Pointer to a structure containing DTMF configuration parameters. If the
    parameter is not given, \ref IFX_TAPI_CID_DTMF_CFG_t default values will be
   used. */
   compat_uptr_t                 pDTMFConf;
   /** Pointer to a structure containing the coding for the
   absence reason of the calling number.*/
   compat_uptr_t                 pABSCLICode;
};


/** Union of the CID configuration structures for different standards.
    Required on 64-bit architecture to handle IOCTLs from 32-bit architecture. */
union IFX_TAPI_CID_STD_TYPE_32_t
{
   /** Structure defining configuration parameters for the Telcordia standard. */
   struct IFX_TAPI_CID_STD_TELCORDIA_32_t     telcordia;
   /** Structure defining configuration parameters for the ETSI standard, with FSK
    transmission.*/
   struct IFX_TAPI_CID_STD_ETSI_FSK_32_t      etsiFSK;
   /** Structure defining configuration parameters for the ETSI standard, with DTMF
    transmission.*/
   struct IFX_TAPI_CID_STD_ETSI_DTMF_32_t     etsiDTMF;
   /** Structure defining configuration parameters for the BT SIN standard. */
   struct IFX_TAPI_CID_STD_SIN_32_t           sin;
   /** Structure defining configuration parameters for the NTT standard. */
   struct IFX_TAPI_CID_STD_NTT_32_t           ntt;
   /** Structure defining configuration parameters for the KPN standard, with DTMF
       transmission. */
   struct IFX_TAPI_CID_STD_KPN_DTMF_32_t      kpnDTMF;
   /** Structure defining configuration parameters for the KPN standard, with DTMF
       and FSK transmission. */
   struct IFX_TAPI_CID_STD_KPN_DTMF_FSK_32_t  kpnDTMF_FSK;
   /** Structure defining configuration parameters for the TDC standard */
   struct IFX_TAPI_CID_STD_TDC_32_t           tdc;
};


/** Capability structure
    on 64-bit architecture to handle IOCTLs from 32-bit architecture. */
struct IFX_TAPI_CAP_LIST_32_t
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index.                            \endtapiv4 */
   IFX_uint16_t dev;
   /** Count of allocated \ref IFX_TAPI_CAP_t elements. On return it contains
       the number of elements that were copied. */
   IFX_uint32_t nCap;
   /** Allocated memory for capability list;
       amount of memory should be equal to (sizeof (IFX_TAPI_CAP_t) * nCap). */
   /* IFX_TAPI_CAP_t *pList; original declaration */
   compat_uptr_t pList; /* compat declaration */
};


/** Union for events that can be reported.
    Definition for 64-bit architecture to handle IOCTLs from 32-bit
    architecture. */
union IFX_TAPI_EVENT_DATA_32_t
{
   /** Pulse digit information.*/
   IFX_TAPI_EVENT_DATA_PULSE_t         pulse;
   /** DTMF digit information. */
   IFX_TAPI_EVENT_DATA_DTMF_t          dtmf;
   /** Tone generation index. */
   IFX_TAPI_EVENT_DATA_TONE_GEN_t      tone_gen;
   /** Tone detection information. */
   IFX_TAPI_EVENT_DATA_TONE_DET_t      tone_det;
   /** Command error event details. */
   IFX_TAPI_EVENT_DATA_CERR_t          cerr;
   /** Synchronization details. */
   IFX_TAPI_EVENT_FAULT_HW_SYNC_t      hw_sync;
   /** Event FIFO details.*/
   IFX_TAPI_EVENT_DATA_EVT_FIFO_t      event_fifo;
   /** Analog line calibration process. */
   IFX_TAPI_EVENT_CALIBRATION_t        calibration;
   /** Metering event. */
   IFX_TAPI_EVENT_METERING_t           metering;
   /** Event generated when new continuous measurement results are available
    to be read out by IFX_TAPI_CONTMEASUREMENT_GET. */
   IFX_TAPI_EVENT_CONTMEASUREMENT_t    contmeasurement;
    /** Error line. */
   /* IFX_TAPI_ErrorLine_t*               error; original declaration */
   compat_uptr_t                       error; /* compat declaration*/
   /** Line mode change event information. */
   IFX_TAPI_LINE_MODE_t                linemode;
   /** FXO line polarity event details. */
   IFX_TAPI_EVENT_DATA_FXO_POLARITY_t  fxo_polarity;
   /** Analog line capacitance measurement results. */
   IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_t lcap;
   /** Capacitance to ground measurement results. */
   IFX_TAPI_EVENT_LINE_MEASURE_CAPACITANCE_GND_t cap2gnd;
#ifndef TAPI_DXY_DOC
   IFX_TAPI_EVENT_HOOK_INT_t           hook_int;
#endif /* #ifndef TAPI_DXY_DOC */
   /** Reserved. */
   IFX_uint16_t                        value;
};


/** This structure is reported by an 'EVENT_GET' ioctl. For event masking
    'EVENT_MASK' re-using IFX_TAPI_EVENT_t should be used.
    Definition for 64-bit architecture to handle IOCTLs from 32-bit
    architecture. */
struct IFX_TAPI_EVENT_32_t
{
   /** \tapiv3 Not used with the TAPI for CPE products. \endtapiv3
       \tapiv4 Device index.                            \endtapiv4 */
   IFX_uint16_t dev;
   /** Channel where the event occurred or \ref IFX_TAPI_EVENT_ALL_CHANNELS to
       indicate 'any channel' and for events that cannot be assigned to a
       specific channel (such as hardware errors). */
   IFX_uint16_t ch;
   /** Module type. */
   IFX_TAPI_MODULE_TYPE_t module;
   /** Indicates that more TAPI events are waiting to be read out
       with the \ref IFX_TAPI_EVENT_GET ioctl. It has to be set to zero
       for \ref IFX_TAPI_EVENT_ENABLE or \ref IFX_TAPI_EVENT_DISABLE.  */
   IFX_uint16_t            more;
   /** Reserved. */
   IFX_uint16_t reserved;
   /** Event type and sub-type. */
   IFX_TAPI_EVENT_ID_t id;
   /** Specific data of individual event. */
   /* IFX_TAPI_EVENT_DATA_t   data; original declaration */
   union IFX_TAPI_EVENT_DATA_32_t data; /* compat declaration */
};


struct IFX_TAPI_EVENT_ENTRY_32_t
{
   /** Event type and sub-type. */
   IFX_TAPI_EVENT_ID_t id;
   /** Specific data of individual event (for the event type/subtype). */
   union IFX_TAPI_EVENT_DATA_32_t data;
};


/** This structure is used to mask one or multiple events of the TAPI using
    \ref IFX_TAPI_EVENT_ENABLE or \ref IFX_TAPI_EVENT_DISABLE.
    Definition for 64-bit architecture to handle IOCTLs from 32-bit
    architecture. */
struct IFX_TAPI_EVENT_MULTI_32_t
{
   /** Device index of event. If set to IFX_TAPI_EVENT_ALL_DEVICES this
       information is global. All masked events are related to this device. */
   IFX_uint16_t dev;
   /** Channel information of event.
       If set to IFX_TAPI_EVENT_ALL_CHANNELS this information is global.
       All masked events are related to this module channel. */
   IFX_uint16_t ch;
#ifdef TAPI_VERSION4
   /** Module type. All masked events are related to this module type.*/
   IFX_TAPI_MODULE_TYPE_t module;
#endif /* TAPI_VERSION4 */
   /** Number of events in the pEvent. */
   IFX_uint16_t nCount;
   /** List of events to enable or disable;
       the amount of memory should be equal
       to (sizeof (IFX_TAPI_EVENT_ENTRY_t) * nCount). */
   /* IFX_TAPI_EVENT_ENTRY_t *pEvent;  original declaration */
   compat_uptr_t pEvent; /* compat declaration */
};


/** Structure used during start of an NLT test. Used by
    \ref IFX_TAPI_NLT_TEST_START.
    Definition for 64-bit architecture to handle IOCTLs from 32-bit
    architecture. */
struct IFX_TAPI_NLT_TEST_START_32_t
{
   /** Device number (in). */
   IFX_uint16_t dev;
   /** Channel number (in). */
   IFX_uint16_t ch;
   /** Test start status (out). */
   IFX_int32_t nStatus;
   /** When set to IFX_TRUE, the calling process is put to sleep on test start.
       The process is woken up upon reception of the LTEST (in). */
   IFX_boolean_t bBlock;
   /** Identifier of test to be started (in). */
   IFX_TAPI_NLT_TESTID_t testID;
   /** Where required, configuration arguments relative to the test are passed.
       The TAPI does not have to know about the details, therefore the
       details are known by the LL driver and NLT Library, only (in). */
   /* IFX_void_t *pTestCfg; original declaration */
   compat_uptr_t pTestCfg; /* compat declaration */
};


/** Structure used for reading NLT test results. Used by
    \ref IFX_TAPI_NLT_RESULT_GET. */
struct IFX_TAPI_NLT_RESULT_GET_32_t
{
   /** Device number (in). */
   IFX_uint16_t dev;
   /** Channel number (in). */
   IFX_uint16_t ch;
   /** Result get status (out). */
   IFX_int32_t nStatus;
   /** Identifier of test from which results are to be read (in). */
   IFX_TAPI_NLT_TESTID_t testID;
   /** When the respective NLT LIB API is called to retrieve the results
       from a test previously started (non-blocking call), it might be necessary,
       depending on the test, to retrieve the configuration parameters which were
       used during the test. These configuration parameters could be required for
       the calculation of the final results. For this reason, the caller
       where needed passes a pointer (else NULL) of the related type, so that the LL
       implementation copies in the cached configuration parameters used by the
       last test started (in). */
   /* IFX_void_t *pTestCfg; original declaration */
   compat_uptr_t pTestCfg; /* compat declaration */
   /** Pointer to a data structure large enough to accumulate the results
       related to the NLT test. The TAPI does not have to know about the details,
       therefore the details are known by the LL driver and NLT Library (in). */
   /* IFX_void_t *pTestResults; original declaration */
   compat_uptr_t pTestResults; /* compat declaration */
};

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

extern IFX_int32_t IFX_TAPI_Phone_Init_32(
                        TAPI_DEV *pTapiDev,
                        struct IFX_TAPI_CH_INIT_32_t const *pInit_32);

extern IFX_int32_t IFX_TAPI_Cap_List_Get_32 (
                        TAPI_DEV *pTapiDev,
                        struct IFX_TAPI_CAP_LIST_32_t *pCapList_32);

extern IFX_int32_t TAPI_Event_Get_32 (IFX_TAPI_DRV_CTX_t *pDrvCtx,
                        struct IFX_TAPI_EVENT_32_t *pEvent);

extern IFX_int32_t IOCTL_TAPI_EventEnable_32 (IFX_TAPI_DRV_CTX_t *pDrvCtx,
                        struct IFX_TAPI_EVENT_32_t *pEvent);

extern IFX_int32_t IOCTL_TAPI_EventDisable_32 (IFX_TAPI_DRV_CTX_t *pDrvCtx,
                        struct IFX_TAPI_EVENT_32_t *pEvent);

extern IFX_int32_t TAPI_EventMultiEnable_32 (IFX_TAPI_DRV_CTX_t *pDrvCtx,
                        struct IFX_TAPI_EVENT_MULTI_32_t *pEvent);

extern IFX_int32_t TAPI_EventMultiDisable_32 (IFX_TAPI_DRV_CTX_t *pDrvCtx,
                        struct IFX_TAPI_EVENT_MULTI_32_t *pEvent);

#ifdef TAPI_FEAT_CID
extern IFX_int32_t TAPI_Phone_CID_Info_Tx_32 (TAPI_CHANNEL *pChannel,
                        struct IFX_TAPI_CID_MSG_32_t const *pCidInfo);

extern IFX_int32_t TAPI_Phone_CID_Seq_Tx_32 (TAPI_CHANNEL *pChannel,
                        struct IFX_TAPI_CID_MSG_32_t const *pCidInfo);

extern IFX_int32_t TAPI_Phone_CID_SetConfig_32 (TAPI_CHANNEL *pChannel,
                        struct IFX_TAPI_CID_CFG_32_t const *pCidConf);
#endif /* TAPI_FEAT_CID */

#ifdef TAPI_FEAT_NLT
extern IFX_int32_t IFX_TAPI_NLT_Test_Start_32 (TAPI_CHANNEL *pChannel,
                        struct IFX_TAPI_NLT_TEST_START_32_t *pTestParam);

extern IFX_int32_t IFX_TAPI_NLT_Result_Get_32 (TAPI_CHANNEL *pChannel,
                        struct IFX_TAPI_NLT_RESULT_GET_32_t *pTestResult);
#endif /* TAPI_FEAT_NLT */

#endif /* TAPI_FEAT_LX_COMPAT */
#endif /* DRV_TAPI_COMPAT_H */
