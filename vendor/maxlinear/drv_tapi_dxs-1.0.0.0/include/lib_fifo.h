#ifndef _LIB_FIFO_H
#define _LIB_FIFO_H

/******************************************************************************

  Copyright 2014 Lantiq Deutschland GmbH
  Copyright 2023 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file lib_fifo.h
   This library implements a general FIFO functionality.
*/

#ifdef __cplusplus
   extern "C" {
#endif

/* ============================= */
/* Includes                      */
/* ============================= */
#include <ifx_types.h>

/* ============================= */
/* Type definitions              */
/* ============================= */
struct _FIFO_ID;

typedef struct _FIFO_ID  FIFO_ID;

/* ============================= */
/* Global function declaration   */
/* ============================= */
extern FIFO_ID*      fifoInit (
                        const IFX_uint32_t initialElements,
                        const IFX_uint16_t elementSize);

extern IFX_int32_t   fifoFree(
                        FIFO_ID *pf);

extern IFX_int32_t   fifoPut(
                        FIFO_ID *pf,
                        const IFX_void_t *pData);

extern IFX_int32_t   fifoGet(
                        FIFO_ID *pf,
                        IFX_void_t *pData);

extern IFX_int32_t   fifoPeek(
                        const FIFO_ID *pf,
                        IFX_void_t *pData);

extern IFX_int32_t   fifoPeekTail (
                        const FIFO_ID *pf,
                        IFX_void_t *pData);

extern IFX_uint8_t   fifoEmpty(
                        const FIFO_ID *pf);

extern IFX_uint32_t  fifoSize(
                        const FIFO_ID *pf);

extern IFX_uint32_t  fifoElements(
                        const FIFO_ID *pf);

extern IFX_int32_t   fifoReset(
                        FIFO_ID *pf);

#ifdef DEBUG
extern IFX_int32_t   fifoIntegrity(
                        const FIFO_ID *pf);
#else
#define fifoIntegrity(pf) IFX_SUCCESS
#endif /* DEBUG */

#ifdef __cplusplus
   }
#endif

#endif /* _LIB_FIFO_H */
