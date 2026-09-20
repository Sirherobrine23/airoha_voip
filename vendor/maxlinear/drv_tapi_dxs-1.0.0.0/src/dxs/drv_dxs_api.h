#ifndef _DRV_DXS_API_H
#define _DRV_DXS_API_H
/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016, 2020 Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_api.h
   This file contains the device and channel structure definitions.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

#include "drv_dxs.h"
#include <drv_tapi_config.h>
#include "drv_dxs_fw_cmd_sdd.h"
#include "drv_dxs_fw_cmd_eop.h"
#include "drv_dxs_fifo.h"

/* Files from common includes */
#include <drv_dxs_io.h>
#include <drv_dxs_io_types.h>
#include <drv_dxs_errno.h>
#include <drv_tapi_osmap.h>

#include <lib_fifo.h>
#include <lib_bufferpool.h>

#include "../tapi/drv_tapi_ll_interface.h"
#include "../tapi/drv_tapi_api.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
TAPI_DECLARE_TRACE_GROUP(TAPI_DXS);

#define DXS_SUCCESS TAPI_SUCCESS

#ifndef RETURN_STATUS
#define RETURN_STATUS(code,info)                                           \
   do{                                                                     \
      if (DXS_SUCCESS(code) == IFX_FALSE)                                  \
         DXS_ChErrorEvent(pCh, code, __LINE__, __FILE__, info);            \
      return code;                                                         \
   }while(0)
#endif

#ifndef RETURN_DEVSTATUS
#define RETURN_DEVSTATUS(code,info)                                        \
   do{                                                                     \
      if (((IFX_int32_t)code > (IFX_int32_t)TAPI_statusClassWarn) ||       \
           ((IFX_int32_t)code == (IFX_int32_t)IFX_ERROR))                  \
         DXS_DevErrorEvent (pDev, code, __LINE__, __FILE__, info);         \
      return code;                                                         \
   }while(0)
#endif

/* device states */
/* firmware successfully downloaded */
#define   DS_FW_DLD                    0x000001
/* device is up (see system event: UP) */
#define   DS_DEV_UP                    0x000002
/* Basic Init done */
#define   DS_BASIC_INIT                0x000004
/* Tapi Init done */
#define   DS_TAPI_INIT                 0x000008
/** device successfully initialized */
#define   DS_DEV_INIT                  0x000010
/** PCM module enabled  */
#define   DS_PCM_EN                    0x000040
/** SPI interface is active */
#define   DS_SPI_ACTIVE                0x000080
/** SPI interface setup phase */
#define   DS_SPI_SETUP                 0x000100

#define CHECK_HOST_ERR(pDev, exec_on_err)                            \
   if (pDev->nErr  == DXS_statusSpiAccErr)                           \
   {                                                                 \
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,                                \
            ("[%s, %d], DXS%d: error on host register access\n",     \
            __FILE__, __LINE__, pDev->nDevNr));                      \
                                                                     \
      (exec_on_err);                                                 \
   }

#define DXS_DEV_TYPE             IFX_TAPI_DEV_TYPE_DUSLIC_XS

/** set module field for tapi event */
#define DXS_TAPI_EVENT_MODULE_SET(m_tapiEvent, m_nModule)                  \
   do{                                                                     \
       m_tapiEvent.module = m_nModule;                                     \
   }while(0)

/** translate int to IFX_boolean_t */
#define DXS_BITFIELD_TO_IFX_BOOL(m_bit) ((0 != m_bit) ? IFX_TRUE : IFX_FALSE)

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
struct DXS_ALM;
struct DXS_PCM;
struct DXS_DTMF_AT;
struct DXS_CID_GEN;
struct DXS_UTD;

typedef struct _DXS_CHANNEL_t DXS_CHANNEL_t;
typedef struct _DXS_DEVICE_t DXS_DEVICE_t;
typedef struct _DXS_IRQ_t DXS_IRQ_t;

#ifdef __LINUX_SPI_H
   struct spi_device;
#endif /* __LINUX_SPI_H */

#include "drv_dxs_alm.h"
#include "drv_dxs_pcm.h"
#include "drv_dxs_debug.h"

/* Note: This file must be available in your build directory. Copy the template
         related to your environment from src/drv_config_user.default.h to your
         build directory and rename it to drv_config_user.h. */
#include "drv_config_user.h"

enum DXS_EventMode
{
   /* Retrieval of events is stopped. */
   DXS_EVENT_STOP,
   /* Events are retrieved after receiving an IRQ from the device. */
   DXS_EVENT_INTERRUPT,
   /* Events are retrieved during periodic poll cycles. */
   DXS_EVENT_POLLING
};

enum DXS_DcDcType
{
   /* No converter set - initial condition. */
   DXS_DCDC_TYPE_NOTSET = 0,
   /** Inverting Buck-Boost Converter per channel with
      PNP switching transistor and typically 12 V input voltage. */
   DXS_DCDC_TYPE_IBB = 1,
   /** Combined Inverting Buck-Boost Converter. One converter
      supplies both channels and is using a PNP switching transistor
      with typically 12 V input voltage. */
   DXS_DCDC_TYPE_CIBB = 2,
   /** Inverting Boost Converter per channel with NMOS switching
      transistor typically 12 V input voltage. */
   DXS_DCDC_TYPE_IB = 3,
   /** Combined Inverting Boost Converter per channel with NMOS switching
      transistor typically 12 V input voltage. */
   DXS_DCDC_TYPE_CIB = 4,
   /** Buck-or-Boost Converter per channel with selectable buck or boost
      switching to connect to -48 V battery */
   DXS_DCDC_TYPE_BB = 5,
   /** Combined Buck-or-Boost Converter per channel with selectable buck or
      boost switching to connect to -48 V battery. */
   DXS_DCDC_TYPE_CBB = 6,
   /** Inverting Flyback Converter per channel with flyback concept
      and NMOS switching transistor (typically 12 V input supply). */
   DXS_DCDC_TYPE_IFB = 7,
   /** Combined Inverting Flyback Converter per channel with flyback concept and
      NMOS switching transistor (typically 12 V input supply). */
   DXS_DCDC_TYPE_CIFB = 8,
   /** Inverting Boost Converter with Gate Driver to control a standard level
      NMOS switching transistor for typically 12 V input voltage. */
   DXS_DCDC_TYPE_IBGD = 9,
   /** Combined Inverting Boost Converter with Gate Driver to control a standard
      level NMOS switching transistor for typically 12 V input voltage. */
   DXS_DCDC_TYPE_CIBGD = 10,
   /** Inverting Boost Converter with Voltage Doubler per channel with NMOS
      switching transistor for low input voltage (e.g. 3.3 V). */
   DXS_DCDC_TYPE_IBVD = 11,
   /** Combined Inverting Boost Converter with Voltage Doubler per channel with
      NMOS switching transistor for low input voltage (e.g. 3.3 V).*/
   DXS_DCDC_TYPE_CIBVD = 12,
   /* Magic value for DC/DC variant autodetection -
      only for Intel/Lantiq TID cards! */
   DXS_DCDC_TYPE_AUTO = 129
};

struct _DXS_IRQ_t
{
   /** OS/architecture specific IRQ number */
   IFX_int32_t              nIrq;
   /** "registered in OS" flag */
   IFX_boolean_t            bRegistered;
   /** interrupt enabled flag: IFX_TRUE=Enabled / IFX_FALSE=Disabled */
   volatile IFX_boolean_t   bIntEnabled;
#ifdef DXS_HAVE_INTERRUPTS
   /** to lock against multiple enable/disable */
   TAPI_OS_mutex_t           mtxIrqAcc;
#endif /* DXS_HAVE_INTERRUPTS */
   /* pointer to first DUSLIC XS device, registered for this irq */
   DXS_DEVICE_t            *pdev_head;
   /** pointer to next different irq */
   DXS_IRQ_t               *next_irq;
};

/* capabilities structure for internal usage */
struct DXS_Capabilities
{
   /** Number of PCM Channels */
   IFX_uint8_t    nPCM;
   /** Number of Analog Line Channels */
   IFX_uint8_t    nALI;
   /** Number of DTMF Generators */
   IFX_uint8_t    nDTMFG;
   /** Number of DTMF receiver */
   IFX_uint8_t    nDTMFR;
   /** Number of Caller ID Senders */
   IFX_uint8_t    nCIDS;
   /** Number of Universal Tone Detectors */
   IFX_uint8_t    nUTD;
   /* fw capabilities */
   IFX_boolean_t  bfw_MWI;             /* MWI is available */
   IFX_boolean_t  bfw_ASPwrSave;       /* FW internal PwrSave handling */
   IFX_boolean_t  b_DrvPwrSaveEn;      /* DRV internal PwrSave handling */
   IFX_boolean_t  bfw_CMeas;           /* capacitance measurement supported */
   IFX_boolean_t  bfw_ExtLT;           /* extended line testing supported */
   IFX_boolean_t  bfw_combDcDc;        /* combined DC/DC operation supported */
   IFX_boolean_t  bfw_WbTsSplit;       /* split wideband timeslots supported */
   IFX_boolean_t  bfw_BuckBoost;       /* buck/boost DC/DC operation supported*/
   IFX_boolean_t  bfw_UTD;             /* UTD is supported*/
   IFX_boolean_t  bfw_GR909;           /* GR909 is supported*/
};

/*
   Used for information exchange with event line irq handling - remove event
   generated after TestAct flag state change to disabled
   (LINE_EVENT_MASK_internal fw command).
*/
/* default value - normal processing of hook events */
#define DXS_HOOK_EV_TST_DFLT 0
/* current hook event should be discarded */
#define DXS_HOOK_EV_TST_DISC 1
/* TestAct flag changed state to disabled, next event has to be discarded */
#define DXS_HOOK_EV_TST_CHNG 2

/* Channel structure */
struct _DXS_CHANNEL_t
{
   /* common fields with DXS_CHANNEL_t structure */
   /* Actual  duslic-xs channel, starting with 1, a channel number 0 indicates
      the control device structure DXS_DEVICE */
   IFX_uint8_t                nChannel;
   /* tracking in use of the device */
   IFX_uint16_t               nInUse;

   /* ptr to actual device */
   DXS_DEVICE_t               *pParent;
   /* overall channel protection ( read/write/ioctl level)
      PS: Avoid nested locking of this mutex. It can lead to a deadlock */
   TAPI_OS_mutex_t             mtxChAcc;

   /** status flags */
#define DXS_CH_BBD_DOWNLOADED    0x00000001
#define DXS_CH_OVER_TEMPERATURE  0x00000002
#define DXS_CH_GROUND_FAULT      0x00000004
   volatile IFX_uint32_t      flags;

   /* ALM Channel management structure */
   struct DXS_ALM             *pALM;

   /* PCM Channel management structure */
   struct DXS_PCM             *pPCM;

   /* DTMF/AT channel management structure */
   struct DXS_DTMF_AT         *pDTMF;

   /* CID generator channel management structure */
   struct DXS_CID_GEN         *pCID;

   /* UTD channel management structure */
   struct DXS_UTD             *pUTD;

   TAPI_CHANNEL               *pTapiCh;

   /* tone counter for tone repetition. If 0 endlessly, TG only! */
   IFX_uint8_t                nTone_Cnt;
   /* step counter for ALM tone playout, TG only! */
   IFX_uint8_t                nToneStep;

   /*  Test mode flag (LINE_EVENT_MASK_internal command) state change forces
       discarding 2nd generated hook event. This flag is set before
       writing fw command that disables test mode. */
   IFX_uint8_t                nDiscardHookEvent;
};


/* DUSLIC XS global Device Stucture */
struct _DXS_DEVICE_t
{
   /**
      DUSLIC XS channel. This value is always zero for the device structure.
      Different channel device structure are from type DXS_CHANNEL_t.
      The first field is also a nChannel. This guarantees that the channel
      field is always available, although the cast was wrong. But ensure
      the correct cast after reading the channel field.
   */
   IFX_uint8_t                nChannel;

   /* tracking in use of the device */
   IFX_uint16_t               nInUse;

   /* device error status, never reset. read out for detail
      information if return value of function is not DXS_statusOk */
   IFX_int32_t                nErr;

   /* absolute index of the device, when multiple devices are existing
      For the first device the value is set to zero */
   IFX_int32_t                nDevNr;

   /* Status of device, see states defines .
      Must be protected against interrupts and concurrent tasks */
   IFX_vuint32_t              nDevState;

   /* local storage for the interrupt flags that are returned during disabling
      the global interrupts. This value is used to enable the interrupts
      again */
   TAPI_OS_INTSTAT             nIrqMask;

   /* hardware revision, read from device with SDD_RevisionRead command */
   IFX_uint16_t               nChipRev;

   /* EDSP firmware version number */
   IFX_uint32_t               nFwRev;

   /* device ID read from the chip */
   IFX_uint16_t                nDevId;

   /** Capability list for the device  -  Interface to application-level */
   IFX_TAPI_CAP_t             *CapList;
   IFX_uint8_t                nMaxCaps;
   /** Capability list for the device  -  Internal data for driver */
   struct DXS_Capabilities    caps;
   IFX_boolean_t              bCapsRead;

   /* signaling event used by the outbox thread for command read response */
   TAPI_OS_event_t             obxDataEvt;
   /* Flag to indicate data in the outbox. */
   IFX_boolean_t              bOutBoxData;
   /* command outbox data queue */
   FIFO_t                     *cmd_obx_queue;

   /* DUSLIC XS mbx concurent access protection mutex.
      PS: Avoid nested locking of this mutex. It can lead to a deadlock */
   TAPI_OS_mutex_t             mtxMbxAcc;

   /* DUSLIC XS SPI access protection mutex. */
   TAPI_OS_mutex_t             mtxSpiAcc;

   /* DUSLIC XS firmware download protection mutex. */
   TAPI_OS_mutex_t             mtxFwDlAcc;

   /* cached command inbox length */
   IFX_uint8_t                nMbxCachedCbiLen;

   /* DUSLIC XS share variables concurrent access protection mutex.
      PS: Avoid nested locking of this mutex. It can lead to a deadlock */
   TAPI_OS_mutex_t             mtxMemberAcc;

   /* error flag for cmd read - avoid all kind of compiler optimizations */
   volatile IFX_boolean_t     bCmdReadError;

   /* channel structures */
   DXS_CHANNEL_t              pChannel[DXS_MAX_CH_NR];

   /* timeslot allocation management flags */
   /* value PcmRxTs need protection */
   IFX_uint32_t               PcmRxTs[DXS_PCM_TS_ARRAY];

   /* value PcmTxTs need protection */
   IFX_uint32_t               PcmTxTs[DXS_PCM_TS_ARRAY];

   /* Actual number of PCM timeslots resulting from the DCL clock setting. */
   IFX_uint16_t              nMaxTimeslot;

   /* selection of polling or interrupt event handling mode */
   volatile enum DXS_EventMode   nEventHandlingMode;

   /* variables for shared interrupt handling */
   DXS_IRQ_t                  *pIrq;
   DXS_DEVICE_t               *pInt_NextDev;

#ifndef DXS_FEAT_LINUX_THREADED_IRQ
   /** flag for requested interrupt handling, i.e. activation for the realtime
       kernel thread to run the interrupt routine on this device */
   volatile IFX_boolean_t     bNeedIrqHandling;
#endif /* DXS_FEAT_LINUX_THREADED_IRQ */

   /* Indicates combined DC/DC (true) or dedicated DC/DC (false, default) */
   IFX_boolean_t              bDcDcHwCombined;

   /* entry to Tapi device, needs protection  */
   TAPI_DEV                   *pTapiDev;

#ifdef __LINUX_SPI_H
   /** SPI device handle */
   struct spi_device          *pSpiDev;
#endif /* __LINUX_SPI_H */

#ifdef TAPI_LINUX_KERNEL_SPACE
   /* Reset GPIO */
   struct gpio_desc           *pResetGpio;
   /* Interval of the reset */
   unsigned int               nResetInterval;
#endif /* TAPI_LINUX_KERNEL_SPACE */

   DXS_FW_SYS_VERS_t          fw_vers;
   DXS_FW_SYS_CAPS_t          fw_caps;
   DXS_FW_SYS_Control_t       fw_sys_ctrl;
};

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
#ifdef DXS_HAVE_INTERRUPTS
   extern IFX_void_t OS_Install_DxsIRQHandler(DXS_DEVICE_t *pDev,
                                              IFX_int32_t nIrq);
   extern IFX_void_t OS_UnInstall_DxsIRQHandler(DXS_DEVICE_t *pDev);
#endif /* DXS_HAVE_INTERRUPTS */

extern IFX_void_t DXS_ChErrorEvent (DXS_CHANNEL_t *pCh,
                                    IFX_uint16_t err,
                                    IFX_uint32_t nLine,
                                    const IFX_char_t* sFile,
                                    const IFX_void_t *info);

extern IFX_void_t DXS_DevErrorEvent (DXS_DEVICE_t *pDev,
                                     IFX_uint16_t err,
                                     IFX_uint32_t nLine,
                                     const IFX_char_t* sFile,
                                     const IFX_void_t *info);

extern IFX_void_t DXS_HwFaultRecovery_OnTimer (Timer_ID Timer,
                                               IFX_ulong_t arg);
#ifdef DXS_HAVE_INTERRUPTS
   extern IFX_void_t DXS_IrqLockDevice (IFX_TAPI_LL_DEV_t *pLLDev);
   extern IFX_void_t DXS_IrqUnlockDevice (IFX_TAPI_LL_DEV_t *pLLDev);
   extern IFX_void_t DXS_IrqEnable (IFX_TAPI_LL_DEV_t *pLLDev);
   extern IFX_void_t DXS_IrqDisable (IFX_TAPI_LL_DEV_t *pLLDev);
#else
   /* empty defines to avoid multiple #ifdef/#endif in the code */
   #define DXS_IrqLockDevice(pDev)
   #define DXS_IrqUnlockDevice(pLLDev)
#endif /* DXS_HAVE_INTERRUPTS */

extern IFX_void_t DXS_SPI_drvRegister(DXS_DEVICE_t *pDev);
extern IFX_void_t DXS_SPI_drvUnregister(DXS_DEVICE_t *pDev);

extern IFX_int32_t DXS_Dev_Spec_Ioctl(IFX_TAPI_LL_CH_t *pLLDummyCh,
                                      IFX_uint32_t iocmd,
                                      IFX_ulong_t ioarg);
#ifdef TAPI_FEAT_LX_COMPAT
IFX_int32_t DXS_Dev_Spec_Compat_Ioctl(IFX_TAPI_LL_CH_t *pLLDummyCh,
                                      IFX_uint32_t iocmd,
                                      IFX_ulong_t ioarg);
#endif

#endif /* _DRV_DXS_API_H */
