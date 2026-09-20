#ifndef DRV_TAPI_H
#define DRV_TAPI_H
/******************************************************************************

  Copyright 2006-2013 Lantiq Deutschland GmbH
  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2022-2023 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi.h
   Contains TAPI functions declaration und structures.
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include <drv_tapi_config.h>
#include "lib_fifo.h"
#include "lib_bufferpool.h"
#include "drv_tapi_io.h"
#include "drv_tapi_kio.h"
#include "drv_tapi_errno.h"
#include "drv_tapi_osmap_local.h"

#if defined(LINUX) && defined(__KERNEL__)
   #include <linux/cdev.h>
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0))
   #define fallthrough   do {} while (0)
#endif

/* ============================= */
/* Global defines                */
/* ============================= */

/* Simplify usage of ptr_chk */
#define IFX_TAPI_PtrChk(ptr) ptr_chk((ptr), #ptr)


#define TAPI_MIN_FLASH                80
#define TAPI_MAX_FLASH               400
#define TAPI_MIN_FLASH_MAKE          200
#define TAPI_MIN_DIGIT_LOW            30
#define TAPI_MAX_DIGIT_LOW            80
#define TAPI_MIN_DIGIT_HIGH           30
#define TAPI_MAX_DIGIT_HIGH           80
#define TAPI_MIN_OFF_HOOK             40
#define TAPI_MIN_ON_HOOK             400
#define TAPI_MIN_INTERDIGIT          300

#ifndef TAPI_RING_CADENCE_GRANULARITY
   /* Duration of one bit of the ring cadence in milliseconds. Please note
      that should the timer service have a granularity this should be a
      multiple of this granularity. */
   #define TAPI_RING_CADENCE_GRANULARITY 50
#endif /* TAPI_RING_CADENCE_GRANULARITY */

#ifndef TAPI_MAX_LL_DRIVERS
   /** Maximum number of allowed LL drivers */
   #define TAPI_MAX_LL_DRIVERS 1 /* Only Duslic XS driver */
#endif /* TAPI_MAX_LL_DRIVERS */

/* TODO: Find out how many resources to set here for Duslic XS */
#ifndef TAPI_TONE_MAXRES
   #ifdef TAPI_VERSION3
        /** Maximum number of tone generators on one channel of any CPE.
            2 on SIG and 1 on DECT */
        #define TAPI_TONE_MAXRES 3
    #else /* !TAPI_VERSION3 */
        /** Maximum tone generators that can run in parallel: 3 modules ALM,
            PCM, COD and CONF and 2 tone generators */
        #define TAPI_TONE_MAXRES 8
    #endif /* TAPI_VERSION3*/
#endif

#ifndef IFX_TAPI_EVENT_POOL_INITIAL_SIZE
/** Initial number of events structures allocated by the driver. If more
    event structures are needed the pool grows automatically in steps of
    IFX_TAPI_EVENT_POOL_GROW_SIZE */
   #define IFX_TAPI_EVENT_POOL_INITIAL_SIZE 70
#endif

#ifndef IFX_TAPI_EVENT_POOL_GROW_SIZE
/** Number of events that the event structure pool grows every time it gets
    depleted. */
   #define IFX_TAPI_EVENT_POOL_GROW_SIZE 70
#endif

#ifndef IFX_TAPI_EVENT_FIFO_SIZE
/** Event Fifo Size */
   #define IFX_TAPI_EVENT_FIFO_SIZE               10
#endif /* IFX_TAPI_EVENT_FIFO_SIZE */

#ifndef IFX_TAPI_EVENT_POOL_GROW_LIMIT
   #ifdef TAPI_VERSION3
      #define IFX_TAPI_EVENT_POOL_GROW_LIMIT           490
   #else /* non TAPI_VERSION3 */
      #define IFX_TAPI_EVENT_POOL_GROW_LIMIT \
         (TAPI_MAX_LL_DRIVERS * IFX_TAPI_EVENT_FIFO_SIZE * 16/* max channels per device */)
   #endif /* TAPI_VERSION3 */
#endif

/* Number of tones for internal driver purposes */
#define TAPI_MAX_RESERVED_TONES       3
/* Maximum number of tones codes, comprising the maximum user tones and the
   maximum internal reserved tones. */
#define TAPI_MAX_TONE_CODE           (1 /*index 0*/ + \
                                      IFX_TAPI_TONE_INDEX_MAX + \
                                      TAPI_MAX_RESERVED_TONES)

/* Used as parameter to IFX_TAPI_Dial_OffhookTime_Override(). */
#define IFX_TAPI_DIAL_TIME_NORMAL   0

/* =================================== */
/* Utility macros                      */
/* =================================== */

#ifndef ARRAY_SIZE
   #define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif /* ARRAY_SIZE */

/* =================================== */
/* Global typedef forward declarations */
/* =================================== */

typedef struct _TAPI_DEV      TAPI_DEV;
typedef struct _TAPI_CHANNEL  TAPI_CHANNEL;

/* =================================== */
/* Error handling macros               */
/* =================================== */
#define ERROR_CLASS_MASK           (TAPI_statusClassErr |                  \
                                    TAPI_statusClassWarn |                 \
                                    TAPI_statusClassCritical)

/* Note on both RETURN_STATUS() and RETURN_DEVSTATUS():
   "code" may contain an already combined HL and LL error code. In this case
   the "llcode" field is ignored. In all other cases the error code is the
   combination of the HL and LL error code. The "code" field may also contain
   an old style error code (IFX_ERROR) which is also treated as an error.
   The error is put on the stack if either "code" or "llcode" contain an
   error code. */

#define RETURN_STATUS(code, llcode)                                        \
   /*lint -save -e{506, 572, 774, 778} */                                  \
   do{                                                                     \
      IFX_uint32_t   HLcode, LLcode;                                       \
      if (((IFX_uint32_t)(code)) & 0xFFFF0000)                             \
      {                                                                    \
         HLcode = (((IFX_uint32_t)(code)) & 0xFFFF0000) >> 16;             \
         LLcode = (((IFX_uint32_t)(code)) & 0x0000FFFF);                   \
      }                                                                    \
      else                                                                 \
      {                                                                    \
         HLcode = (((IFX_uint32_t)(code)) & 0x0000FFFF);                   \
         LLcode = (((IFX_uint32_t)(llcode)) & 0x0000FFFF);                 \
      }                                                                    \
      if (((IFX_return_t)(HLcode) == IFX_ERROR) ||                         \
          ((IFX_return_t)(LLcode) == IFX_ERROR) ||                         \
          ((TAPI_statusClass_t)(HLcode) & ERROR_CLASS_MASK) ||             \
          ((TAPI_statusClass_t)(LLcode) & ERROR_CLASS_MASK)   )            \
      {                                                                    \
         TAPI_ErrorStatus (pChannel->pTapiDevice,                          \
                           (TAPI_Status_t)(HLcode), (LLcode),              \
                           __LINE__, __FILE__);                            \
      }                                                                    \
      return   (((IFX_uint32_t)(HLcode) & 0x0000FFFF) << 16) |             \
                ((IFX_uint32_t)(LLcode) & 0x0000FFFF);                     \
   }while(0) /*lint -restore */

#define RETURN_DEVSTATUS(code, llcode)                                     \
   /*lint -save -e{506, 572, 774, 778} */                                  \
   do{                                                                     \
      IFX_uint32_t   HLcode, LLcode;                                       \
      if (((IFX_uint32_t)(code)) & 0xFFFF0000)                             \
      {                                                                    \
         HLcode = (((IFX_uint32_t)(code)) & 0xFFFF0000) >> 16;             \
         LLcode = (((IFX_uint32_t)(code)) & 0x0000FFFF);                   \
      }                                                                    \
      else                                                                 \
      {                                                                    \
         HLcode = (((IFX_uint32_t)(code)) & 0x0000FFFF);                   \
         LLcode = (((IFX_uint32_t)(llcode)) & 0x0000FFFF);                 \
      }                                                                    \
      if (((IFX_return_t)(HLcode) == IFX_ERROR) ||                         \
          ((IFX_return_t)(LLcode) == IFX_ERROR) ||                         \
          ((TAPI_statusClass_t)(HLcode) & ERROR_CLASS_MASK) ||             \
          ((TAPI_statusClass_t)(LLcode) & ERROR_CLASS_MASK)   )            \
      {                                                                    \
         TAPI_ErrorStatus (pTapiDev,                                       \
                           (TAPI_Status_t)(HLcode), (LLcode),              \
                           __LINE__, __FILE__);                            \
      }                                                                    \
      return   (((IFX_uint32_t)(HLcode) & 0x0000FFFF) << 16) |             \
                ((IFX_uint32_t)(LLcode) & 0x0000FFFF);                     \
   }while(0) /*lint -restore */

/* =================================== */
/* Defines for tapi device init state  */
/* =================================== */
/** Device status: nothing initialised */
#define   TAPI_INITSTATUS_UNINITIALISED            0x00000000
/** Device status: PPD procfs entry for the device created. */
#define   TAPI_INITSTATUS_PPD_PROCFS_CREATED       0x00000001

/* ================================= */
/* Enums                             */
/* ================================= */

/* Channel file descriptor flags */
enum CH_FLAGS
{
   /** Indicates that a task is pending via select on this device */
   CF_NEED_WAKEUP          = 0x00100000,
};

/* =============================== */
/* Further includes                */
/* =============================== */

/* include debugging interface */
#include "drv_tapi_debug.h"

/* include definitions of the low level driver interface which require
   some of the definitions above */
#include "drv_tapi_ll_interface.h"

/* Event handling */
#include "drv_tapi_event.h"

#include "drv_tapi_ppd.h"

#define TAPI_MAX_DEVFS_HANDLES 36

/** Structure for low level driver context elements.
 * 
 * TAPI HL driver uses array of such structures to store information about 
 * low level drivers.
 */
typedef struct _IFX_TAPI_HL_DRV_CTX
{
   /* Pointer to the driver context provided by the LL driver. A value of
      NULL indicates that this struct is unused. */
   IFX_TAPI_DRV_CTX_t *pDrvCtx;
   
   #if defined(LINUX) && defined (__KERNEL__)
      /* Buffer to store the registered device driver name, Linux references to
         this string, e.g. on cat /proc/devices - please don't remove */
      IFX_char_t registeredDrvName[20];
      
      /* Count of contiguous device numbers requested. */
      unsigned int nDevNrCount;

      struct device *pLL_Device[TAPI_MAX_DEVFS_HANDLES];
   #endif /* LINUX and __KERNEL__ */
} IFX_TAPI_HL_DRV_CTX_t;

/* ============================= */
/** Structure for operation control
    \internal */
/* ============================= */
typedef struct
{
   /* interrupt routine save hookstatus here */
   IFX_uint8_t               bHookState;
   /* polarity status from the line; 1 = reversed; 0 = normal */
   IFX_uint8_t               nPolarity;
   /* automatic battery switch; 1 = automatic; 0 = normal */
   IFX_uint8_t               nBatterySw;
   /* last line feed mode; is set after ringing stops */
   IFX_TAPI_LINE_MODE_t      nLineMode;
   /* Last line type; changed when new line type changed with success. */
   IFX_TAPI_LINE_TYPE_t      nLineType;
   /* set when the fault condition occurs of the device. It will be reset
      when the line mode is modified */
   IFX_boolean_t             bFaulCond;
   /* Shutdown because of overtemp or ground fault event. */
   IFX_boolean_t             bEmergencyShutdown;
} TAPI_OPCONTROL_DATA_t;

/* ============================= */
/* Structure for ring data       */
/* ============================= */
struct TAPI_RING_DATA;
typedef struct TAPI_RING_DATA TAPI_RING_DATA_t;

/* ============================= */
/* Structure for pcm data        */
/* ============================= */
typedef struct
{
   /* configuration data for pcm services */
   IFX_TAPI_PCM_CFG_t   PCMConfig;
   /* save activation status */
   IFX_boolean_t        bTimeSlotActive;
   /* save configuration status */
   IFX_boolean_t        bCfgSuccess;
} TAPI_PCM_DATA_t;

/* ============================= */
/* Structure for dial data       */
/* ============================= */
struct TAPI_DIAL_DATA;
typedef struct TAPI_DIAL_DATA TAPI_DIAL_DATA_t;

/* ============================= */
/* Structure for PPD data        */
/* ============================= */
struct TAPI_PPD_DATA;
typedef struct TAPI_PPD_DATA TAPI_PPD_DATA_t;

/* ============================= */
/* Structure for metering data   */
/*                               */
/* ============================= */
struct TAPI_METER_DATA;
typedef struct TAPI_METER_DATA TAPI_METER_DATA_t;

/* ============================= */
/* Structure for complex tone    */
/* data                          */
/* ============================= */
struct TAPI_TONE_DATA;
typedef struct TAPI_TONE_DATA TAPI_TONE_DATA_t;

struct TAPI_TONE_RES;
typedef struct TAPI_TONE_RES TAPI_TONE_RES_t;

/* ============================== */
/* Structure for statistic data   */
/*                                */
/* ============================== */
struct TAPI_STAT_DATA;
typedef struct TAPI_STAT_DATA TAPI_STAT_DATA_t;

/* ============================= */
/* channel specific structure    */
/* ============================= */
struct _TAPI_CHANNEL
{
   /* channel number */
   /* ATTENTION, nChannel must be the first element */
   IFX_uint8_t                   nChannel;
   /* pointer to the Low level driver channel */
   IFX_TAPI_LL_CH_t             *pLLChannel;
   /* pointer to the tapi device structure */
   TAPI_DEV                     *pTapiDevice;
   /* semaphore used only in blocking read access,
      in this case given from interrupt context */
   TAPI_OS_event_t               semReadBlock;
   /* wakeup queue for select on read */
   TAPI_OS_drvSelectQueue_t      wqRead;
   /* wakeup queue for select on write */
   TAPI_OS_drvSelectQueue_t      wqWrite;
   /* flags for different purposes, see CH_FLAGS */
   volatile IFX_uint32_t         nFlags;
   /* In Use counter */
   IFX_uint16_t                  nInUse;
   /* channel is initialized */
   IFX_boolean_t                 bInitialized;
   /* locking semaphore for protecting data */
   TAPI_OS_mutex_t               semTapiChDataLock;
   /* overall channel protection ( read/write/ioctl level)
   PS: Avoid nested locking of this mutex. It can lead to a deadlock */
   TAPI_OS_mutex_t               semTapiChSingleIoctlAccess;
   /* data structures for services */
   TAPI_OPCONTROL_DATA_t         TapiOpControlData;
   IFX_TAPI_EVENT_HANDLER_DATA_t *pEventHandler;
   
#ifdef TAPI_FEAT_METERING
   TAPI_METER_DATA_t             *pTapiMeterData;
#endif /* TAPI_FEAT_METERING */

#ifdef TAPI_FEAT_RINGENGINE
   TAPI_RING_DATA_t              *pTapiRingData;
#endif /* TAPI_FEAT_RINGENGINE */

#ifdef TAPI_FEAT_PCM
   TAPI_PCM_DATA_t               TapiPCMData;
#endif /* TAPI_FEAT_PCM */

#ifdef TAPI_FEAT_DIAL
   TAPI_DIAL_DATA_t              *pTapiDialData;
#endif /* TAPI_FEAT_DIAL */

#ifdef TAPI_FEAT_TONEENGINE
   TAPI_TONE_RES_t               *pToneRes;
   /* complex tone data */
   TAPI_TONE_DATA_t              *TapiComplexToneData;
#endif /* TAPI_FEAT_TONEENGINE */

#ifdef TAPI_FEAT_CID
   TAPI_CID_DATA_t               *pTapiCidData;
#endif /* TAPI_FEAT_CID */

#ifdef TAPI_FEAT_PHONE_DETECTION
   TAPI_PPD_DATA_t               *pTapiPpdData;
#endif /* TAPI_FEAT_PHONE_DETECTION */
};

/* ============================= */
/* tapi structure                */
/* ============================= */
struct _TAPI_DEV
{
   /* channel number IFX_TAPI_DEVICE_CH_NUMBER indicates the control device */
   /* ATTENTION, nChannel must be the first element */
   IFX_uint8_t               nChannel;
   /* pointer to LL device structure */
   IFX_TAPI_LL_DEV_t        *pLLDev;
   /* link to the device driver context */
   IFX_TAPI_DRV_CTX_t       *pDevDrvCtx;

   /* Number of channels for which memory is allocated in the array below.
     This does not reflect the number of analog, signaling, pcm or coder
     channels. They are reported in the nResource struct below. */
   IFX_uint8_t               nMaxChannel;
   /* array of tapi channel structures */
   TAPI_CHANNEL             *pChannel;
   /* struct with counters how many resources are available and initialised */
   IFX_TAPI_RESOURCE        nResource;

   /** Device ID (unique only for devices of the same device context) */
   IFX_uint32_t             nDev;
   /** Globally unique TAPI device ID [0,1,...] (even across device context) */
   IFX_uint32_t             nDevID;
   /* usage counter, counts the number of open fds */
   IFX_uint16_t              nInUse;
   /* already opened or not */
   IFX_boolean_t             bInitialized;
   /* Flags to remember which parts are initialised. */
   IFX_uint32_t            nInitStatusFlags;

#if defined(LINUX) && defined (__KERNEL__)
   /** Linux char device struct */
   struct cdev          cdev;
#endif /* LINUX */

   /* Event wakeup queue for select, this one is used to report all kind of
      device and channel events to the application. Its something like VxWorks
      version of select() call. */
   TAPI_OS_drvSelectQueue_t  wqEvent;
   /* Additional flag for vxWorks if a real wakeup is required */
   volatile IFX_boolean_t    bNeedWakeup;

   /* overall channel protection (ioctl level)
   PS: Avoid nested locking of this mutex. It can lead to a deadlock */
   TAPI_OS_mutex_t           semTapiDevSingleIoctlAccess;

   /** last error code and error stack */
   IFX_TAPI_Error_t         error;

   /** Last channel where an event was reported. */
   IFX_uint8_t               nLastEventChannel;
};


/* ============================= */
/* Global variables declaration  */
/* ============================= */

/* Declarations for debug interface */
TAPI_DECLARE_TRACE_GROUP(TAPI_DXS);

/* global high level driver context used in system interface only */
extern struct _IFX_TAPI_HL_DRV_CTX gHLDrvCtx[TAPI_MAX_LL_DRIVERS];

extern const IFX_char_t TAPI_DRV_WHATVERSION[];

#ifdef HAVE_CONFIG_H
   extern const IFX_char_t DRV_TAPI_WHICHCONFIG[];
#endif /* HAVE_CONFIG_H */

/* ======================================== */
/**  Driver identification                  */
/* ======================================== */
extern IFX_int32_t TAPI_Phone_Get_Version (
                        IFX_char_t *pVer);

extern IFX_int32_t TAPI_Phone_Check_Version (
                        IFX_TAPI_VERSION_t const *vers);

extern IFX_int32_t IFX_TAPI_Cap_Nr_Get (
                        TAPI_DEV *pTapiDev,
                        IFX_TAPI_CAP_NR_t *pCap);

extern IFX_int32_t IFX_TAPI_Cap_List_Get (
                        TAPI_DEV *pTapiDev,
                        IFX_TAPI_CAP_LIST_t *pCapList);

/* ======================================== */
/**  Error reporting                        */
/* ======================================== */
extern IFX_int32_t TAPI_Last_Err_Get (
                        TAPI_DEV *pTapiDev,
                        IFX_TAPI_Error_t *pErr);

extern void TAPI_ErrorStatus (
                        TAPI_DEV *pTapiDevice,
                        TAPI_Status_t nHlCode,
                        IFX_int32_t nLlCode,
                        IFX_uint32_t nLine,
                        const IFX_char_t* sFile);

/* ======================================== */
/**  TAPI Initialisation                    */
/* ======================================== */
#ifdef EVENT_LOGGER_DEBUG
extern TAPI_DEV *TAPI_DeviceGetByID(IFX_uint32_t nDevID);
#endif /* EVENT_LOGGER_DEBUG */

extern IFX_int32_t IFX_TAPI_Driver_Start (void);
extern IFX_void_t  IFX_TAPI_Driver_Stop (void);

extern IFX_int32_t IFX_TAPI_Event_On_Driver_Start (void);
extern IFX_void_t  IFX_TAPI_Event_On_Driver_Stop  (void);

extern IFX_return_t TAPI_OS_RegisterLLDrv (
                        IFX_TAPI_DRV_CTX_t *pLLDrvCtx,
                        IFX_TAPI_HL_DRV_CTX_t *pHLDrvCtx);

extern IFX_return_t TAPI_OS_UnregisterLLDrv (
                        const IFX_TAPI_DRV_CTX_t *pLLDrvCtx,
                        IFX_TAPI_HL_DRV_CTX_t *pHLDrvCtx);

extern IFX_TAPI_DRV_CTX_t* IFX_TAPI_DeviceDriverContextGet (
                        IFX_int32_t Major);

extern IFX_int32_t IFX_TAPI_DeviceStart (
                        TAPI_DEV *pTapiDev,
                        IFX_TAPI_DEV_START_CFG_t const *pDevStartCfg);

extern IFX_int32_t IFX_TAPI_DeviceStop (
                        TAPI_DEV *pTapiDev);

/* Legacy support for driver initialisation */
extern IFX_int32_t IFX_TAPI_Phone_Init (
                        TAPI_DEV *pTapiDev,
                        IFX_TAPI_CH_INIT_t const *pInit);

/* ======================================== */
/* Analog line services                     */
/* ======================================== */

extern IFX_int32_t TAPI_Phone_Set_LineType (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_LINE_TYPE_CFG_t const *pCfg);

extern IFX_int32_t TAPI_Phone_Set_Linefeed (
                        TAPI_CHANNEL *pChannel,
                        IFX_int32_t nMode);

extern IFX_void_t  TAPI_Phone_Linefeed_Get (
                        TAPI_CHANNEL *pChannel,
                        IFX_uint8_t *nLineMode);

extern IFX_void_t  TAPI_Phone_Change_Linefeed (
                        TAPI_CHANNEL *pChannel,
                        IFX_int32_t nMode);

extern IFX_void_t  TAPI_Phone_Linefeed_Restore (
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t TAPI_Phone_HookstateGet (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_LINE_HOOK_STATUS_GET_t *pHookMode);

/* ======================================== */
/* Ringing services                         */
/* ======================================== */
#ifdef TAPI_FEAT_RINGENGINE
extern IFX_int32_t IFX_TAPI_Ring_Initialise_Unprot (
                        TAPI_CHANNEL *pChannel);

extern IFX_void_t  IFX_TAPI_Ring_Cleanup (
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t IFX_TAPI_Ring_Prepare (
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t IFX_TAPI_Ring_Start (
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t IFX_TAPI_Ring_Stop (
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t IFX_TAPI_Ring_DoBlocking (
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t IFX_TAPI_Ring_IsActive (
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t IFX_TAPI_Ring_SetCadenceHighRes (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_RING_CADENCE_t const *pCadence);

extern IFX_int32_t IFX_TAPI_Ring_SetMaxRings (
                        TAPI_CHANNEL *pChannel,
                        IFX_uint32_t nMaxRings);

extern IFX_int32_t IFX_TAPI_Ring_Engine_Start (
                        TAPI_CHANNEL *pChannel,
                        IFX_boolean_t bStartWithInitial);
#endif /* TAPI_FEAT_RINGENGINE */

#ifdef TAPI_FEAT_CID
extern IFX_int32_t IFX_TAPI_Ring_CalculateRingTiming (
                        TAPI_CHANNEL *pChannel,
                        IFX_uint32_t *pCadenceRingBurst,
                        IFX_uint32_t *pCadenceRingPause);

extern IFX_void_t  IFX_TAPI_Ring_CidCadencePrepare (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_CID_SEQ_CONF_t *pCidData);
#endif /* TAPI_FEAT_CID */

/* ======================================== */
/* DTMF services                            */
/* ======================================== */
#ifdef TAPI_FEAT_DTMF
extern IFX_int32_t TAPI_Phone_DTMFR_Cfg_Get (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_DTMF_RX_CFG_t *pDtmfRxCoeff);

extern IFX_int32_t TAPI_Phone_DTMFR_Cfg_Set (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_DTMF_RX_CFG_t const *pDtmfRxCoeff);
#endif /* TAPI_FEAT_DTMF */

/* ======================================== */
/* Tone services                            */
/* ======================================== */
#ifdef TAPI_FEAT_TONETABLE
extern IFX_int32_t TAPI_Phone_Tone_Predef_Config (
                        void);

extern IFX_int32_t TAPI_Phone_Tone_TableConf (
                        IFX_TAPI_TONE_t const *pTone);

extern IFX_int32_t TAPI_Phone_Add_SimpleTone (
                        IFX_TAPI_TONE_SIMPLE_t const *pSimpleTone);

extern IFX_int32_t TAPI_Phone_Add_ComposedTone (
                        IFX_TAPI_TONE_COMPOSED_t const *pComposedTone);

extern IFX_uint32_t IFX_TAPI_Tone_DurationGet (
                        IFX_uint32_t nToneIndex);
#endif /* TAPI_FEAT_TONETABLE */

#ifdef TAPI_FEAT_TONEENGINE
extern IFX_int32_t IFX_TAPI_Tone_Initialise_Unprot (
                        TAPI_CHANNEL *pChannel);

extern IFX_void_t  IFX_TAPI_Tone_Cleanup (
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t TAPI_Phone_Tone_Play (
                        TAPI_CHANNEL *pChannel,
                        IFX_int32_t nToneIndex);

extern IFX_int32_t TAPI_Phone_Tone_Play_Unprot (
                        TAPI_CHANNEL *pChannel,
                        const IFX_TAPI_TONE_PLAY_t *pTone,
                        IFX_boolean_t bSendEndEvent);

extern IFX_int32_t TAPI_Phone_Tone_Stop (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_TONE_PLAY_t *pTone);

extern IFX_void_t TAPI_Tone_Step_Completed(
                        TAPI_CHANNEL* pChannel,
                        IFX_uint8_t nResID);
#endif /* TAPI_FEAT_TONEENGINE */

#ifdef TAPI_FEAT_TONEGEN
extern IFX_int32_t TAPI_Target_Tone_Play (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_TONE_PLAY_t *pTone);

extern IFX_int32_t TAPI_Phone_Tone_Local_Play (
                        TAPI_CHANNEL *pChannel,
                        IFX_int32_t nToneIndex);

extern IFX_int32_t TAPI_Phone_Tone_Local_Stop (
                        TAPI_CHANNEL *pChannel,
                        IFX_int32_t nToneIndex);

extern IFX_int32_t TAPI_Phone_Tone_Net_Play (
                        TAPI_CHANNEL *pChannel,
                        IFX_int32_t nToneIndex);

extern IFX_int32_t TAPI_Phone_Tone_Net_Stop (
                        TAPI_CHANNEL *pChannel,
                        IFX_int32_t nToneIndex);

extern IFX_int32_t TAPI_Phone_Tone_Def_Stop (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_TONE_PLAY_t *pTone);

extern IFX_int32_t TAPI_Phone_Tone_Ringback (
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t TAPI_Phone_Tone_Busy (
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t TAPI_Phone_Tone_Dial (
                        TAPI_CHANNEL *pChannel);

#endif /* TAPI_FEAT_TONEGEN */


#ifdef TAPI_FEAT_CID
extern IFX_int32_t IFX_TAPI_Module_Find_Connected_Data_Channel (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_MAP_TYPE_t nModType,
                        TAPI_CHANNEL **pTapiCh);
#endif /* TAPI_FEAT_CID */


/* ======================================== */
/* PCM services                             */
/* ======================================== */
#ifdef TAPI_FEAT_PCM
extern IFX_int32_t TAPI_Phone_PCM_IF_Set_Config (
                        TAPI_DEV *pTapiDev,
                        IFX_TAPI_PCM_IF_CFG_t const *pPCMif);

extern IFX_int32_t TAPI_Phone_PCM_Set_Config (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_PCM_CFG_t const *pPCMConfig);

extern IFX_int32_t TAPI_Phone_PCM_Get_Config (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_PCM_CFG_t *pPCMConfig);

extern IFX_int32_t TAPI_Phone_PCM_Set_Activation(
                        TAPI_CHANNEL *pChannel,
                        IFX_uint32_t nActive);

extern IFX_int32_t TAPI_Phone_PCM_Get_Activation(
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_PCM_ACTIVATION_t *pActive);
#endif /* TAPI_FEAT_PCM */

/* ======================================== */
/* Pulse dial detection services            */
/* ======================================== */
#ifdef TAPI_FEAT_DIAL
extern IFX_int32_t IFX_TAPI_Dial_Initialise_Unprot (
                        TAPI_CHANNEL *pChannel);

extern IFX_void_t  IFX_TAPI_Dial_Cleanup (
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t IFX_TAPI_Dial_SetValidationTime (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_LINE_HOOK_VT_t const *pTime);

extern IFX_void_t  IFX_TAPI_Dial_OffhookTime_Override(
                        TAPI_CHANNEL *pChannel,
                        IFX_uint32_t const nTime);

#endif /* TAPI_FEAT_DIAL */

#ifdef TAPI_FEAT_DIALENGINE
extern IFX_void_t  IFX_TAPI_Dial_LineDisable (
                        TAPI_CHANNEL * pChannel);

extern IFX_void_t  IFX_TAPI_Dial_HookEvent (
                        TAPI_CHANNEL * pChannel,
                        IFX_boolean_t bHookState,
                        IFX_uint16_t nTime);

extern IFX_void_t  IFX_TAPI_Dial_HookSet (
                        TAPI_CHANNEL *pChannel,
                        IFX_boolean_t bHookState);
#endif /* TAPI_FEAT_DIALENGINE */

/* ======================================== */
/* Metering services                        */
/* ======================================== */
#ifdef TAPI_FEAT_METERING
extern IFX_int32_t IFX_TAPI_Meter_Initialise_Unprot (
                        TAPI_CHANNEL *pChannel);

extern IFX_void_t  IFX_TAPI_Meter_Cleanup (
                        TAPI_CHANNEL *pChannel);

extern IFX_boolean_t TAPI_Phone_Meter_IsActive (
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t TAPI_Phone_Meter_Config (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_METER_CFG_t const *pMeterConfig);

extern IFX_int32_t TAPI_Phone_Meter_Start (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_METER_START_t const *pMeterStart);

extern IFX_int32_t TAPI_Phone_Meter_Stop (
                        TAPI_CHANNEL *pChannel);

extern IFX_int32_t TAPI_Phone_Meter_Burst (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_METER_BURST_t const *pMeterBurst);

extern IFX_int32_t TAPI_Phone_Meter_Stat (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_METER_STATISTICS_t *pMeterStat);

extern IFX_void_t IFX_TAPI_Meter_EventServe (
                        TAPI_CHANNEL *pChannel);
#endif /* TAPI_FEAT_METERING */

/* ======================================== */
/* FXS Phone Detection                      */
/* ======================================== */
#ifdef TAPI_FEAT_PHONE_DETECTION
extern IFX_int32_t IFX_TAPI_PPD_Cfg_Get (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_LINE_PHONE_DETECT_CFG_t *pPpdConf);

extern IFX_int32_t IFX_TAPI_PPD_Cfg_Set (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_LINE_PHONE_DETECT_CFG_t const *pPpdConf);
#endif /* TAPI_FEAT_PHONE_DETECTION */

/* ======================================== */
/* Line testing                             */
/* ======================================== */
#ifdef TAPI_FEAT_NLT
extern IFX_int32_t IFX_TAPI_NLT_Test_Start (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_NLT_TEST_START_t *pTestParam);

extern IFX_int32_t IFX_TAPI_NLT_Result_Get (
                        TAPI_CHANNEL *pChannel,
                        IFX_TAPI_NLT_RESULT_GET_t *pTestResult);
#endif /* TAPI_FEAT_NLT */

/* ======================================== */
/**  Other functions                        */
/* ======================================== */
extern IFX_boolean_t ptr_chk(
                        const IFX_void_t /* const */ *ptr,
                        const IFX_char_t *pPtrName);

extern IFX_int32_t TAPI_DeferWork (
                        IFX_void_t* pFunc,
                        IFX_void_t* pParam);

extern IFX_int32_t TAPI_Test_Hook_Gen (
                        TAPI_CHANNEL *pChannel,
                        IFX_uint32_t nHook);

#endif  /* DRV_TAPI_H */
