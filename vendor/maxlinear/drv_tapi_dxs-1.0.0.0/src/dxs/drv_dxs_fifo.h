#ifndef __DXS_FIFO_H__
#define __DXS_FIFO_H__
/******************************************************************************

  Copyright 2014 Lantiq Deutschland GmbH
  Copyright 2021 Maxlinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_fifo.h
   DxS fifo declarations.
*/

/*
|               header                 |  elem 0  |  elem 1  |  elem 2  | ...
+------------+-------+--------+--------+---+------+---+------+---+------+-------
| magic code | count | wr_idx | rd_idx | p | size | p | size | p | size | ...
+------------+-------+--------+--------+-+-+------+-+-+------+-+-+------+-------
                                         |          |          |
                                         V          V          V
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
#define MAX_FIFO_SIZE    200 /* in elements */
#define DXS_FIFO_MAGIC_CODE   0x41464946
/* return codes */
#define DXS_FIFO_OK     0
#define DXS_FIFO_FULL   (-1)
#define DXS_FIFO_EMPTY  (-2)
#define DXS_FIFO_INV    (-3)

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
/** FIFO_Elem_t */
typedef struct
{
   IFX_void_t     *data;
   IFX_uint32_t   size;
} FIFO_Elem_t;

/** FIFO_t */
typedef struct
{
   IFX_uint32_t   magic;
   IFX_uint32_t   in_use;
   IFX_uint32_t   fifo_size;
   IFX_uint32_t   count;
   IFX_uint32_t   wr_idx;
   IFX_uint32_t   rd_idx;
   TAPI_OS_mutex_t mtx;
   FIFO_Elem_t    elem[MAX_FIFO_SIZE];
} FIFO_t;

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
extern FIFO_t*       fifo_init(
                        IFX_uint32_t elements);

extern IFX_void_t    fifo_destroy (
                        FIFO_t *fifo);

extern IFX_void_t    fifo_flush(
                        FIFO_t *fifo);

extern IFX_uint32_t  fifo_count(
                        const FIFO_t *fifo);

extern IFX_int32_t   fifo_put(
                        FIFO_t *fifo,
                        IFX_void_t *data,
                        IFX_uint32_t size);

extern IFX_int32_t   fifo_get(
                        FIFO_t *fifo,
                        IFX_void_t **data,
                        IFX_uint32_t *size);

#endif /* __DXS_FIFO_H__ */
