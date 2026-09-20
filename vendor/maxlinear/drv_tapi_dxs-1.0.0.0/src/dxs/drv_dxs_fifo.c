/******************************************************************************

  Copyright 2014 Lantiq Deutschland GmbH
  Copyright 2021 Maxlinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_fifo.c
   Implementation of DxS fifo functions.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_fifo.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/* two fifos per device:
   one for messages, one for command outbox data */
#define FIFOS_PER_DEVICE      2

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */
/** placeholder for the fifos */
FIFO_t  dxs_fifos[DXS_MAX_DEVICES * FIFOS_PER_DEVICE];

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */
/**
   Function fifo_init

   \param   elements    - elements

   \return
   - FIFO_t
*/
FIFO_t*  fifo_init (IFX_uint32_t elements)
{
   FIFO_t *p;
   IFX_int32_t idx;

   for (idx=0; idx<(DXS_MAX_DEVICES * FIFOS_PER_DEVICE); idx++)
   {
      p = &dxs_fifos[idx];
      if (!p->in_use)
         break;
   }
   if (idx >= (DXS_MAX_DEVICES * FIFOS_PER_DEVICE))
   {
      return NULL;
   }

   p->magic = DXS_FIFO_MAGIC_CODE;

   if (elements > MAX_FIFO_SIZE)
      elements = MAX_FIFO_SIZE;

   p->fifo_size = elements;
   p->count = p->wr_idx = p->rd_idx = 0;

   TAPI_OS_MutexInit (&p->mtx);

   p->in_use = 1;

   return p;
}

/**
   Function fifo_destroy

   \param   fifo    - pointer to FIFO_t

*/
IFX_void_t fifo_destroy (FIFO_t *fifo)
{
   if ((fifo != IFX_NULL) &&
       (fifo->magic == DXS_FIFO_MAGIC_CODE) && (fifo->in_use != 0))
   {
      TAPI_OS_MutexRelease(&fifo->mtx);
      TAPI_OS_MutexDelete(&fifo->mtx);

      fifo->in_use = 0;
      fifo->magic = 0;
   }
}

/**
   Function fifo_flush

   \param   fifo    - pointer to FIFO_t

*/
IFX_void_t fifo_flush (FIFO_t *fifo)
{
   TAPI_OS_MutexGet(&fifo->mtx);

   fifo->count = fifo->wr_idx = fifo->rd_idx = 0;

   TAPI_OS_MutexRelease(&fifo->mtx);
}

/**
   Function fifo_count

   \param   fifo    - pointer to FIFO_t

   \return
   - IFX_uint32_t count
*/
IFX_uint32_t fifo_count(const FIFO_t *fifo)
{
   return fifo->count;
}

/**
   Function fifo_put

   \param   fifo    - pointer to FIFO_t
   \param   data    - pointer to data
   \param   size    - size

   \return
      DXS_FIFO_INV
      DXS_FIFO_FULL
      DXS_FIFO_OK
*/
IFX_int32_t fifo_put (FIFO_t *fifo, IFX_void_t *data, IFX_uint32_t size)
{
   FIFO_Elem_t *p = IFX_NULL;

   if (fifo->magic != DXS_FIFO_MAGIC_CODE)
      return DXS_FIFO_INV;

   if (fifo->count == fifo->fifo_size)
      return DXS_FIFO_FULL;

   TAPI_OS_MutexGet(&fifo->mtx);

   p = &fifo->elem[fifo->wr_idx];

   p->data = data;
   p->size = size;
   fifo->count++;

   if (++fifo->wr_idx == fifo->fifo_size)
      fifo->wr_idx = 0;

   TAPI_OS_MutexRelease(&fifo->mtx);

   return DXS_FIFO_OK;
}

/**
   Function fifo_get

   \param   fifo    - pointer to FIFO_t
   \param   data    - pointer to data
   \param   size    - pointer size

   \return
      DXS_FIFO_INV
      DXS_FIFO_EMPTY
      DXS_FIFO_OK
*/
IFX_int32_t fifo_get (FIFO_t *fifo, IFX_void_t **data, IFX_uint32_t *size)
{
   FIFO_Elem_t *p = IFX_NULL;

   if (fifo->magic != DXS_FIFO_MAGIC_CODE)
      return DXS_FIFO_INV;

   if (fifo->count == 0)
      return DXS_FIFO_EMPTY;

   TAPI_OS_MutexGet(&fifo->mtx);

   p = &fifo->elem[fifo->rd_idx];

   *data = p->data;
   *size = p->size;
   fifo->count--;

   if (++fifo->rd_idx == fifo->fifo_size)
      fifo->rd_idx = 0;

   TAPI_OS_MutexRelease(&fifo->mtx);

   return DXS_FIFO_OK;
}
