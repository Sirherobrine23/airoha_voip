#ifndef _DRV_TAPIEVENT_H
#define _DRV_TAPIEVENT_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_event.h
   Interface of the TAPI event handling implementation.
*/

/* ========================================================================== */
/*                       Global Macro Definitions                             */
/* ========================================================================== */

/* Disable or Enable event. */
#define IFX_EVENT_DISABLE 1
#define IFX_EVENT_ENABLE 0

#ifdef TAPI_VERSION4
   #define TAPI_ALM_EXTERNAL(el) (el).external
   #define TAPI_ALM_INTERNAL(el) (el).internal
   #define TAPI_COD_EXTERNAL(el) (el).external
   #define TAPI_COD_INTERNAL(el) (el).internal
#endif /* TAPI_VERSION4 */
#ifdef TAPI_VERSION3
   #define TAPI_ALM_EXTERNAL(el) (el).local
   #define TAPI_ALM_INTERNAL(el) (el).network
   #define TAPI_COD_EXTERNAL(el) (el).network
   #define TAPI_COD_INTERNAL(el) (el).local
#endif /* TAPI_VERSION3 */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

struct IFX_TAPI_EVENT_HANDLER_DATA;
typedef struct IFX_TAPI_EVENT_HANDLER_DATA IFX_TAPI_EVENT_HANDLER_DATA_t;

/* internal structure used by the event dispatcher to transport the event
   details from timer/interrupt context into process context
   Note:
   this structure requires the definition of the TAPI_CHANNEL above
   while dependencies to drv_tapi_event.h exist in parallel, the
   best place (although not nice) for this structure is here.
*/
typedef struct
{
#ifdef LINUX
#ifndef TAPI_LIBRARY
   /* !!! important, work struct/tq_struct must be the first element,
          because we need to cast it later on to its surrounding structure
          IFX_TAPI_EXT_EVENT_PARAM_t */
   struct work_struct tapiWs;
#endif /* TAPI_LIBRARY */
#endif /* LINUX */
#if (defined(IFXOS_USE_DEV_IO) && (IFXOS_USE_DEV_IO == 1))
   TAPI_OS_ThreadCtrl_t defferThread;
#endif
   void             (*pFunc) (void*);
   TAPI_CHANNEL     *pChannel;
   IFX_TAPI_EVENT_t tapiEvent;
} IFX_TAPI_EXT_EVENT_PARAM_t;

/* struct that reports counters from both fifos of one channel */
typedef struct
{
   /* High priority fifo counters */
   IFX_uint32_t      nHighWaiting;
   /* Low priority fifo counters */
   IFX_uint32_t      nLowWaiting;
   /* Possible extensions are counters on the size, the full state or
      the number of transactions. */
} IFX_TAPI_EVENT_FIFO_COUNTERS_t;

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

extern IFX_int32_t IFX_TAPI_Event_SetMask (TAPI_CHANNEL *pChannel,
                                           IFX_TAPI_EVENT_t *pEvent,
                                           IFX_uint32_t const value);
extern IFX_int32_t IFX_TAPI_EventDispatcher_Init (TAPI_CHANNEL *);
extern IFX_int32_t IFX_TAPI_EventDispatcher_Exit (TAPI_CHANNEL * pChannel);

extern IFX_uint8_t IFX_TAPI_EventFifoEmpty (TAPI_CHANNEL *);

extern IFX_int32_t IFX_TAPI_EventFifoCounters (
   TAPI_CHANNEL *pChannel,
   IFX_TAPI_EVENT_FIFO_COUNTERS_t *pCounters);

extern IFX_int32_t IFX_TAPI_Event_Dispatch_ProcessCtx (
   IFX_TAPI_EXT_EVENT_PARAM_t *pParam);

extern IFX_int32_t TAPI_Event_Get (
   IFX_TAPI_DRV_CTX_t *pDrvCtx,
   IFX_TAPI_EVENT_t *pEvent);

extern IFX_int32_t IOCTL_TAPI_EventEnable (
   IFX_TAPI_DRV_CTX_t *pDrvCtx,
   IFX_TAPI_EVENT_t *pEvent);

extern IFX_int32_t IOCTL_TAPI_EventDisable (
   IFX_TAPI_DRV_CTX_t *pDrvCtx,
   IFX_TAPI_EVENT_t *pEvent);

extern IFX_int32_t TAPI_EventMultiEnable (
   IFX_TAPI_DRV_CTX_t *pDrvCtx,
   IFX_TAPI_EVENT_MULTI_t *pMulEvent);

extern IFX_int32_t TAPI_EventMultiDisable (
   IFX_TAPI_DRV_CTX_t *pDrvCtx,
   IFX_TAPI_EVENT_MULTI_t *pMulEvent);

extern IFX_int32_t TAPI_EventMultiState_Set (
   IFX_TAPI_DRV_CTX_t *pDrvCtx,
   IFX_TAPI_EVENT_MULTI_t *pMulEvent,
   IFX_uint32_t nEvtState,
   IFX_boolean_t const *bCompatIoctl);

IFX_int32_t IFX_TAPI_EventWrpBufferPool_ElementCountGet(void);
IFX_int32_t IFX_TAPI_EventWrpBufferPool_ElementAvailCountGet(void);

#endif /* _DRV_TAPIEVENT_H */
