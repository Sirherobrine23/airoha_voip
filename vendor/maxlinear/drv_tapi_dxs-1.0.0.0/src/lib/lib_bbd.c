/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

/*
   Module      : lib_bbd.c
   Description : Implementation of block based download generic functions.
   Remarks     : Relevant for the implementation is the block based download
                 specification located in the doc/ directory.
                 The BBD buffer is a big endian buffer according to
                 specification. Therefore, data handling assumes big endian.
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#ifndef MODULE
   #include <stdlib.h>
#endif /* MODULE */

#include <ifx_types.h>
#include <lib_bbd.h>

/* ============================= */
/* Local macros                  */
/* ============================= */

/** Reads 8bit value from big endian byte buffer */
#define BBD_GET_VAL8(buf, pos, val8)  \
            val8 = (buf)[(pos)]; (pos) +=1;

/** Reads 16bit value from big endian byte buffer */
#define BBD_GET_VAL16(buf, pos, val16) \
            val16 = (((buf)[(pos)] << 8) |(buf)[(pos)+1]);\
            (pos)   += 2;

/** Reads 32bit value from big endian byte buffer */
#define BBD_GET_VAL32(buf, pos, val32) \
            val32 = (((buf)[(pos)] << 24) | ((buf)[(pos)+1] << 16) | \
                     ((buf)[(pos)+2] << 8) | (buf)[(pos)+3]);\
            (pos) += 4;

/** Reads a complete bbd head, assuming given position is correct.*/
#define BBD_GET_HEAD(buf, pos, head)\
   do {\
      BBD_GET_VAL16((buf), (pos), (head)->tag);\
      BBD_GET_VAL16((buf), (pos), (head)->version);\
      BBD_GET_VAL32((buf), (pos), (head)->length);\
   }while(0);

/** Reads a complete master block exclusive head,
    assuming given position is correct. */
#define BBD_GET_MASTER(buf, pos, master)\
   do {\
      BBD_GET_VAL32((buf), (pos), (master)->magic);\
      BBD_GET_VAL32((buf), (pos), (master)->identifier);\
      BBD_GET_VAL8 ((buf), (pos), (master)->year);\
      BBD_GET_VAL8 ((buf), (pos), (master)->month);\
      BBD_GET_VAL8 ((buf), (pos), (master)->day);\
      BBD_GET_VAL8 ((buf), (pos), (master)->padding);\
   }while(0);

/** Skip the actual block, assuming given position is correct */
#define BBD_SKIP_BLOCK(pos, block_len)\
            (pos) += (block_len);

/* ============================= */
/* Local structure declaration   */
/* ============================= */

typedef struct
{
   IFX_uint16_t tag;
   IFX_uint16_t version;
   IFX_uint32_t length;
} bdd_head;

typedef struct
{
   IFX_uint32_t magic;
   IFX_uint32_t identifier;
   IFX_uint8_t  year;
   IFX_uint8_t  month;
   IFX_uint8_t  day;
   IFX_uint8_t  padding;
} bdd_master;

/* ============================= */
/* Local variable definition     */
/* ============================= */

/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */

/* ============================= */
/* Global function definition    */
/* ============================= */

/**
   Checks integrity of bbd buffer, i.e availability of master block, end block
   and matching masterblock magic.
\param
   bbd        - handle to bbd_format_t structure
\param
   identifier - master block identifier, related to family.

\return
   value from bbd_error_t enum and out of bbd integrity error domain.
*/
bbd_error_t bbd_check_integrity (bbd_format_t *bbd, IFX_uint32_t identifier)
{
   bdd_head      block_head;
   bdd_master    master_block;
   IFX_uint32_t  master_cnt = 0, end_cnt = 0, block_cnt = 0,
                 family_cnt = 0, i = 0;

   /* safety check */
   if (bbd == IFX_NULL)
      return BBD_INTG_ERR_INVALID;
   if ((bbd->buf == IFX_NULL) || (bbd->size == 0))
      return BBD_INTG_ERR_INVALID;

   /* lets go through the whole buffer */
   while (i < bbd->size)
   {
      BBD_GET_HEAD (bbd->buf, i, &block_head);
      /*
      printk ("%s,%d, tag = 0x%04X, ver = 0x%04X, len = 0x%08X\n\r", __FUNCTION__, __LINE__,
                    block_head.tag, block_head.version, block_head.length);
      */
      switch (block_head.tag)
      {
         case  BBD_MASTER_BLOCK:
            master_cnt ++;
            BBD_GET_MASTER(bbd->buf, i, &master_block);
            /* wrong master magic: Go out now */
            if (master_block.magic != BBD_MASTER_MAGIC)
               return BBD_INTG_ERR_WRONGMASTER;
            if (master_block.identifier == identifier)
               family_cnt ++;
            break;
         case  BBD_END_BLOCK:
            end_cnt ++;
            BBD_SKIP_BLOCK (i, block_head.length);
            break;
         default:
            /* previously detected masterblock was mine ? */
            if (family_cnt == 1)
               block_cnt ++;
            /* go to next block */
            BBD_SKIP_BLOCK (i, block_head.length);
            break;
      }
   }
   /* check master block :
      at least one master block is required */
   if (master_cnt == 0)
      return BBD_INTG_ERR_NOMASTER;
   /* check integrity of family:
      only one family master block required */
   if (family_cnt != 1)
   {
      if (family_cnt > 0)
         return BBD_INTG_ERR_MULTIFAMILY;
      else
         return BBD_INTG_ERR_NOFAMILY;
   }
   /* check end block :
      only one end block required */
   if (end_cnt != 1)
   {
      if (end_cnt > 0)
         return BBD_INTG_ERR_MULTIEND;
      else
         return BBD_INTG_ERR_NOEND;
   }
   /* check if specified masterblock has payload */
   if (block_cnt == 0)
      return BBD_INTG_ERR_EMPTYMASTER;

   /* lucky */
   return BBD_INTG_OK;
}

/**
   Checks availability of given tag.
\param
   bbd   - handle to bbd_format_t structure
\param
   block - handle to bbd_block_t structure,
           contains ptr to block to retrieve.
\return
   none
\remarks
   - No integrity checks are done in this function. It is assumed that
     it was done before.
   - In case the researched block isn't available, NULL pointer and size=0 are
     returned.
*/
IFX_void_t  bbd_get_block (bbd_format_t *bbd, bbd_block_t *block)
{
   bdd_head      block_head;
   bdd_master    master_block;
   IFX_uint32_t  index_cnt = 0, i = 0;
   IFX_boolean_t b_my_family = IFX_FALSE;

   /* safety check */
   if ((bbd == IFX_NULL) || (block == IFX_NULL))
      return;

   /* reset out values */
   block->pData = IFX_NULL;
   block->version = 0;
   block->size = 0;
   /* check bbd buffer before going through it */
   if ((bbd->buf == IFX_NULL) || (bbd->size == 0))
      return;
   /* lets go through the whole buffer */
   while (i < bbd->size)
   {
      BBD_GET_HEAD (bbd->buf, i, &block_head);
      switch (block_head.tag)
      {
         case  BBD_MASTER_BLOCK:
            BBD_GET_MASTER(bbd->buf, i, &master_block);
            b_my_family = IFX_FALSE;
            if (master_block.identifier == block->identifier)
               b_my_family = IFX_TRUE;
            break;
         default:
            /* last master block identified was the one needed ? */
            if ((b_my_family == IFX_TRUE) && (block_head.tag == block->tag))
            {
               /* block found, so set out values and return */
               if (index_cnt == block->index)
               {
                  block->pData   = &bbd->buf [i];
                  block->version = block_head.version;
                  block->size    = block_head.length;
                  /* printk("bbd found tag 0x%04X size %d, ptr 0x%p\n\r",
                            block->tag, block->size, block->data);*/
                  return;
               }
               index_cnt ++;
            }
            /* go to next block */
            BBD_SKIP_BLOCK (i, block_head.length);
            break;
      }
   }

   return;
}

