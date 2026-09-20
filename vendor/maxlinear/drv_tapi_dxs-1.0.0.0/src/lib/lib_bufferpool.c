/******************************************************************************

  Copyright 2004, 2006-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2020            Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file lib_bufferpool.c
   The bufferpool provides buffers with preallocated memory.

   \verbatim
                                                           NULL
                                                           ^
                                                           |pPrev
             +----------------+   pHead----->+---------------+
             + bufferPool     +   GET        + bufferElement +
             +----------------+              +---------------+
             + pHead          +               |pNext       ^
             + pTail          +               V            |pPrev
             + ...            +              +---------------+
             +----------------+              + bufferElement +
                                             +---------------+
                                  pTail----->+---------------+
                                  PUT        + bufferElement +
                                             +---------------+
                                              |pNext
                                              V
                                              NULL


           The layout of the buffer including its header and footer
           is depicted below, including the pointers to the next/previous
           buffer.

        a) internal buffer layout and pointers for used buffers

                    31               0
                    +-----------------+
            pbi-->  |  MAGIC Pattern  |   pointer to buffer internally
                    |      state      |   (points to the buffer head)
                    |   ptr to pool   |   pointer to the bufferpool
                    |   ptr to Next   |   pointer to the next/prev buffer
                    |   ptr to Prev   |      element (only if in pool)
                    |   ptr to NxtBlk |   pointer to the next chunk of
                    |                 |      allocated mem, used for free (*)
                    |   owner ID      |   ID set by the caller when getting
                    |                 |   the buffer (0 is unknown)
                    |   padding       |   padding for 32Byte alignment (cache)
                    |                 |   currently 0 bytes
                    |   reserved      |   4 Bytes for a device specific header
                    |                 |      e.g. used for V-CPE mbx headers
                    +-----------------+
            pb -->  .                 .   pointer to buffer used
                    .      data       .   externally
                    .                 .
                    +-----------------+
            pf -->  |  CHECK Pattern  |   pointer to buffer footer
                    |  Element count  |   number of buffers in this chunk (*)
                    |  Element size   |   Overall size of this chunk (*)
                    +-----------------+

                    (*) These fields are only set in the first buffer of a
                        memory chunk.

   \endverbatim

*******************************************************************************/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_tapi_config.h"

/* we need this for the printf, but not in case of linux kernel modules */
#if !defined(LINUX) || !defined(__KERNEL__)
   #include <stdio.h>
#endif

#include "../tapi/drv_tapi_osmap_local.h"
#include <lib_bufferpool.h>

#ifdef GREENHILLS_CHECK
   #include "drv_tapi_ghs.h"
#endif /* GREENHILLS_CHECK */
#if defined(LINUX) && defined(__KERNEL__)
/* if linux/slab.h is not available, use the precessor linux/malloc.h */
   #include <linux/slab.h>
#endif /* LINUX */

#ifdef TAPI_FEAT_LINUX_SMP
#  include <linux/spinlock.h>
#  include <linux/spinlock_types.h>

#  define IFXOS_spinlock_t spinlock_t
#  define BPOS_SPIN_LOCK_INIT(P_OS_SPIN_LOCK)\
      do {spin_lock_init(&(P_OS_SPIN_LOCK)->sl_handle);} while(0)

#  define BPOS_SPIN_LOCK(P_OS_SPIN_LOCK) \
      do {spin_lock(&(P_OS_SPIN_LOCK)->sl_handle);} while(0)
#  define BPOS_SPIN_UNLOCK(P_OS_SPIN_LOCK) \
      do {spin_unlock(&(P_OS_SPIN_LOCK)->sl_handle);} while(0)

#  define BPOS_SPIN_LOCK_IRQ(P_OS_SPIN_LOCK) \
      do {spin_lock_irq(&(P_OS_SPIN_LOCK)->sl_handle);} while(0)
#  define BPOS_SPIN_UNLOCK_IRQ(P_OS_SPIN_LOCK) \
      do {spin_unlock_irq(&(P_OS_SPIN_LOCK)->sl_handle);} while(0)

#  define BPOS_SPIN_LOCK_IRQSAVE(P_OS_SPIN_LOCK) \
      do {spin_lock_irqsave(&(P_OS_SPIN_LOCK)->sl_handle, (P_OS_SPIN_LOCK)->irq_flags);} while(0)
#  define BPOS_SPIN_UNLOCK_IRQRESTORE(P_OS_SPIN_LOCK) \
      do {spin_unlock_irqrestore(&(P_OS_SPIN_LOCK)->sl_handle, (P_OS_SPIN_LOCK)->irq_flags);} while(0)
#else
#  define IFXOS_spinlock_t unsigned int
#  define BPOS_SPIN_LOCK_INIT(P_OS_SPIN_LOCK)
#  define BPOS_SPIN_LOCK(P_OS_SPIN_LOCK)
#  define BPOS_SPIN_UNLOCK(P_OS_SPIN_LOCK)
#  define BPOS_SPIN_LOCK_IRQSAVE(P_OS_SPIN_LOCK)
#  define BPOS_SPIN_UNLOCK_IRQRESTORE(P_OS_SPIN_LOCK)
#  define BPOS_SPIN_LOCK_IRQ(P_OS_SPIN_LOCK)
#  define BPOS_SPIN_UNLOCK_IRQ(P_OS_SPIN_LOCK)
#endif   /* #if defined(TAPI_FEAT_LINUX_SMP) */

#if defined(LINUX) && defined(__KERNEL__)
#define DUMP_STACK() dump_stack()
#else
#define DUMP_STACK() do {} while(0)
#endif /* defined((LINUX) && defined(__KERNEL__)) */

#ifdef TAPI_LIBRARY
   #define IFXOS_INTSTAT  unsigned
#endif /* TAPI_LIBRARY */

/* ============================= */
/* Extra type definitions        */
/* ============================= */
#ifndef HAVE_IFX_ULONG_T
   #warning please update your ifx_types.h, using local definition of IFX_ulong_t
   /* unsigned long type - valid for 32bit systems only */
   typedef unsigned long               IFX_ulong_t;
   #define HAVE_IFX_ULONG_T
#endif /* HAVE_IFX_ULONG_T */

#ifndef HAVE_IFX_UINTPTR_T
   #warning please update your ifx_types.h, using local definition of IFX_uintptr_t
   typedef IFX_ulong_t                 IFX_uintptr_t;
   #define HAVE_IFX_UINTPTR_T
#endif /* HAVE_IFX_UINTPTR_T */


/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */
#define MAGIC_PATTERN   0x24101974
#define CHECK_PATTERN   0xAAAAAAAA

#define GET_HDR_PTR(pb) \
   (((tBufferHeader *)pb) - 1);\

#define GET_DATA_PTR(pbi) \
   (((tBufferHeader *)pbi) + 1)

#define GET_FTR_PTR(pbi) \
   (IFX_void_t*) ((((IFX_char_t *)pbi)+ \
      ((tBufferHeader*)pbi)->pbp->hdrSize) + \
      ((tBufferHeader*)pbi)->pbp->bufSize)

#define GET_DATA_SIZE(pbi) \
   ((IFX_uint32_t) (((tBufferHeader*)pbi)->pbp->bufSize))

#if defined(LINUX) && defined(__KERNEL__)
#define printf(...) printk(KERN_DEBUG "BUFPOOL: " __VA_ARGS__)
#endif /* defined((LINUX) && defined(__KERNEL__)) */

enum {
   BUFFER_STATE_FREE,
   BUFFER_STATE_IN_USE
};


/* ============================= */
/* Local type definition         */
/* ============================= */

typedef struct
{
   IFXOS_spinlock_t sl_handle;
   IFXOS_INTSTAT    irq_flags;
}  BP_OS_spin_lock_s;

struct _BUFFERPOOL
{
    IFX_uint32_t    hdrSize;             /* header size (bytes) */
    IFX_uint32_t    bufSize;             /* buf size (bytes) */
    IFX_uint32_t    ftrSize;             /* footer size (bytes) */
    IFX_uint32_t    elements;            /* current number of elements */
    IFX_uint32_t    freeElements;        /* current number of free elements */
    IFX_uint32_t    incrElements;        /* inc step if buffer size too small */
    IFX_uint32_t    growthLimit;         /* limit for pool increase */
    IFX_uint32_t    pool_id;             /* pool_id, to aid debugging */
    IFX_uint32_t    failure_cnt;         /* count unavailability of buffers */

    struct _BufferInternal   *pHead;     /* buffers are taken from head */
    struct _BufferInternal   *pTail;     /* recycled buffers are added here */
    struct _BufferInternal   *pBlockList;/* list of element blocks allocated */

    BP_OS_spin_lock_s bp_spin_lock;
};

struct _BufferInternal;

typedef struct _BufferHeader
{
   IFX_uint32_t            magic;
   IFX_uint32_t            state;
   BUFFERPOOL             *pbp;
   struct _BufferInternal *pNext;
   struct _BufferInternal *pPrev;
   struct _BufferInternal *pNextBlock;
   IFX_uint32_t            ownerID;
   /* Please make sure the struct is a multiple of the size of one cache line
      (32 bytes). This is neccessary to avoid cache write back problems when
      these buffers are directly passed to a second core which has an own
      cache. */
   /* char dummy[0]; Currently the padding is 0 bytes */
   /* reserved for device specific header data in front of the payload */
   char reserved[4];
} tBufferHeader;

typedef struct _BufferFooter {
   IFX_uint32_t            pattern;
   IFX_uint32_t            nBlockElementCount; /* Only used in the first buffer */
   IFX_uint32_t            nBlockSize; /* Only used in the first buffer */
   char dummy[20];
} tBufferFooter;

typedef struct _BufferInternal {
   tBufferHeader           hdr;
} tBufferInternal;


/* ============================= */
/* Local variable definition     */
/* ============================= */
static IFX_uint32_t   errorCnt_MAGIC   = 0;
static IFX_uint32_t   errorCnt_Pattern = 0;
static IFX_uint32_t   errorCnt_PutNULL = 0;
static IFX_uint32_t   errorCnt_PutFree = 0;


/* ============================= */
/* Local function definition     */
/* ============================= */

#if (DO_CHECKS_ON_PUT == 1)
static IFX_int32_t checkMagic(const tBufferHeader *ph) {
   return ((ph->magic == MAGIC_PATTERN) ? 1 : 0);
}

static IFX_int32_t checkPattern(const tBufferHeader *ph) {
   IFX_uint32_t *pFtr;
   pFtr = GET_FTR_PTR(ph);
   return ((*pFtr == CHECK_PATTERN) ? 1 : 0);
}

static IFX_int32_t checkInUse (const tBufferHeader *ph) {
   return ((ph->state == BUFFER_STATE_IN_USE) ? 1 : 0);
}
#endif


/**
   Add <count> new buffers to the bufferpool (pbp).

   \param  pbp          Pointer to a bufferpool management structure.
   \param  count        Number of buffers to be added to the bufferpool.

   \return
   - \ref BUFFERPOOL_SUCCESS: if successful
   - \ref BUFFERPOOL_ERROR: when pointer to bufferpool is incorrect or
                            allocation of memory failed
*/
static IFX_int32_t initBuffer(BUFFERPOOL *pbp, const IFX_uint32_t count)
{
   tBufferInternal    *pbi;    /* internal handle pointing to buffer header */
   tBufferFooter      *pf;     /* buffer footer */
   IFX_uint32_t        elementSize = 0;
   IFX_uint32_t        initialSize = 0;
   IFX_uint32_t        i;

   if ((pbp->growthLimit != 0) && (pbp->elements >= pbp->growthLimit))
   {
      /* The growth limit of the pool has been reached - do not grow further. */
      /* printf("INFO: Buffer pool growth limit reached\n"); */
      return BUFFERPOOL_ERROR;
   }

   elementSize = pbp->hdrSize + pbp->bufSize + pbp->ftrSize;
   initialSize = count * elementSize;

   /* Sanity Check */
   if (pbp->pTail != IFX_NULL)
   {
      printf("ERROR initBuffer pTail != IFX_NULL\n");
      return(BUFFERPOOL_ERROR);
   }

   /* Allocate memory and add the new buffers to the bufferpool */
   pbp->pTail = (tBufferInternal *) TAPI_OS_Malloc(initialSize);


   if (pbp->pTail == IFX_NULL)
   {
      return (BUFFERPOOL_ERROR);
   }

   pbp->pHead           = (tBufferInternal *)
      (((IFX_uintptr_t) pbp->pTail) + initialSize - elementSize);
   pbi                  = pbp->pTail;
   pbp->elements       += count;
   pbp->freeElements   += count;
   /* link this memory block into a list needed for buffer pool free */
   pbi->hdr.pNextBlock  = pbp->pBlockList;
   pbp->pBlockList      = pbi;
   /* The number of buffers in this memory block is stored in the footer of
      the first buffer. */
   pbi->hdr.pbp   = pbp; /* must be set for the GET_FTR_PTR macro */
   pf = GET_FTR_PTR (pbi);
   pf->nBlockElementCount = count;
   pf->nBlockSize = initialSize;

   /* Initialize the new buffers */
   for (i=0; i < count ; i++)
   {
      /* add header */
      pbi->hdr.magic    = MAGIC_PATTERN;
      pbi->hdr.pbp      = pbp;
      pbi->hdr.state    = BUFFER_STATE_FREE;
      pbi->hdr.ownerID  = 0x0;

      /* add footer */
      pf = GET_FTR_PTR (pbi);
      pf->pattern    = CHECK_PATTERN;

      /* set pPrev and pNext for each buffer */
      if (i < count-1)
         pbi->hdr.pPrev = (tBufferInternal*)(((IFX_uintptr_t) pbi) + elementSize);
      else
         pbi->hdr.pPrev = IFX_NULL;
      if (i > 0)
         pbi->hdr.pNext = (tBufferInternal*)(((IFX_uintptr_t) pbi) - elementSize);
      else
         pbi->hdr.pNext = IFX_NULL;

      pbi = pbi->hdr.pPrev;
   }

   return BUFFERPOOL_SUCCESS;
}


/**
   Create a new bufferpool and initalise it.

   Initialize a new bufferpool with "initialElements" buffers of "buffersize"
   bytes. Define the "extensionStep" of buffers to add to the bufferpool when
   the last buffer is taken from the pool. If the dynamic extension of the
   bufferpool is not desired set "extensionStep" to 0.

   \param  bufferSize        Size of one buffer in bytes.
   \param  initialElements   Number of empty buffers created at initialisation.
   \param  extensionStep     Number of empty buffers to be added when the last
                             buffer is taken out of the bufferpool. Set to 0
                             to prevent dynamic growing of the bufferpool.
   \param  growthLimit       Number of elements which are the limit.
                             0 means no limit.

   \return
   The return value is a handle (pointer) to the bufferpool or a IFX_NULL pointer
   if the initialisation failed.
*/
BUFFERPOOL* bufferPoolInit (const IFX_uint32_t bufferSize,
                            const IFX_uint32_t initialElements,
                            const IFX_uint32_t extensionStep,
                            const IFX_uint32_t growthLimit)
{
   BUFFERPOOL        *pbp;    /* buffer pool */

   /* Allocate memory for the bufferpool management structure */
   pbp = (BUFFERPOOL *) TAPI_OS_Malloc (sizeof(BUFFERPOOL));
   if (IFX_NULL == pbp)
      return (IFX_NULL);

   /* Initalize the bufferpool management structure */
   pbp->hdrSize       = sizeof(tBufferHeader);
   pbp->bufSize       = bufferSize;
   pbp->ftrSize       = sizeof(tBufferFooter);

   pbp->elements      = 0;
   pbp->freeElements  = 0;

   pbp->incrElements  = extensionStep;
   pbp->growthLimit   = growthLimit;
   pbp->pool_id       = 0; /* no ID set */

   pbp->pHead         = IFX_NULL;
   pbp->pTail         = IFX_NULL;
   pbp->pBlockList    = IFX_NULL;

   BPOS_SPIN_LOCK_INIT(&pbp->bp_spin_lock);

   if (BUFFERPOOL_ERROR == initBuffer (pbp, initialElements))
   {
      TAPI_OS_Free (pbp);
      return (IFX_NULL);
   }

   return pbp;
}


/**
   Unconditionally free an existing bufferpool.

   Free all memory allocated for the elements of the buffer pool and also
   free the bufferpool management structure. No check is done if all buffer
   elements are unused.

   \param   pbp         Pointer to a bufferpool management structure.

   \return
   - \ref BUFFERPOOL_SUCCESS: if successful
   - \ref BUFFERPOOL_ERROR: when pointer to bufferpool is incorrect
*/
IFX_int32_t bufferPoolFree(BUFFERPOOL *pbp)
{
   if (pbp == IFX_NULL)
   {
      printf ("\nERROR bufferPoolFree, no valid buffer pool\n");
      return BUFFERPOOL_ERROR;
   }

   BPOS_SPIN_LOCK_IRQSAVE(&pbp->bp_spin_lock);

   if (pbp->pHead == IFX_NULL)
   {
      BPOS_SPIN_UNLOCK_IRQRESTORE(&pbp->bp_spin_lock);

      printf ("\nERROR bufferPoolFree, no valid buffer pool (head)\n");
      return BUFFERPOOL_ERROR;
   }

   /* Free all allocated memory blocks. */
   while (pbp->pBlockList != IFX_NULL)
   {
      tBufferInternal   *pbi;
      pbi = pbp->pBlockList->hdr.pNextBlock;

      TAPI_OS_Free (pbp->pBlockList);

      pbp->pBlockList = pbi;
   }

   BPOS_SPIN_UNLOCK_IRQRESTORE(&pbp->bp_spin_lock);

   /* Free the bufferpool management structure */
   TAPI_OS_Free (pbp);

   return BUFFERPOOL_SUCCESS;
}


/**
   Request a buffer from the bufferpool.

   \param   pbp         Pointer to a bufferpool management structure.

   \return
   The return value is a void pointer to the provided buffer or a IFX_NULL pointer
   if no more buffers are available in the bufferpool.

   \remarks
   This function is kept for compatibility.
*/
IFX_void_t* bufferPoolGet(IFX_void_t *pbp)
{
   return bufferPoolGetWithOwnerId((BUFFERPOOL *)pbp,
                                   0 /* default (undefined) */);
}


/**
   Request a buffer from the bufferpool and sets the owner ID within the buffer.

   \param   pbp         Pointer to a bufferpool management structure.
   \param   ownerId     A value defined by the caller.

   \return
   The return value is a void pointer to the provided buffer or a IFX_NULL pointer
   if no more buffers are available in the bufferpool.
*/
IFX_void_t* bufferPoolGetWithOwnerId(BUFFERPOOL *pbp, IFX_uint32_t ownerId)
{
   if (IFX_NULL == pbp)
      return IFX_NULL;

   BPOS_SPIN_LOCK_IRQSAVE(&pbp->bp_spin_lock);
   if (IFX_NULL != pbp->pHead)
   {
      IFX_void_t *pd = IFX_NULL;

      tBufferInternal *pbi = pbp->pHead;
      if (pbi->hdr.state == BUFFER_STATE_IN_USE)
      {
         BPOS_SPIN_UNLOCK_IRQRESTORE(&pbp->bp_spin_lock);
         return IFX_NULL;
      }
      pbp->freeElements--;
      pd = GET_DATA_PTR(pbp->pHead);

      if (pbp->pHead->hdr.pNext != IFX_NULL)
      {
         pbp->pHead->hdr.pNext->hdr.pPrev = IFX_NULL;
         pbp->pHead = pbp->pHead->hdr.pNext;
      }
      else
      {
         /* This was the last packet. Now the pool is empty. */
         pbp->pHead = IFX_NULL;
         pbp->pTail = IFX_NULL;
         /* If possible grow the bufferpool. */
         if (pbp->incrElements != 0)
         {
            initBuffer(pbp, pbp->incrElements);
         }
      }
      #if (GET_CLEAN_BUFFERS == 1)
      memset(pd, 0, GET_DATA_SIZE(pbi));
      #endif
      pbi->hdr.state = BUFFER_STATE_IN_USE;
      pbi->hdr.ownerID = ownerId;

      BPOS_SPIN_UNLOCK_IRQRESTORE(&pbp->bp_spin_lock);

      return pd;
   }
#if (SHOW_NOFREEBUF_ERROR == 1)
   else
   {
      if (pbp->failure_cnt % 5000 == 0)
      {
         printf("ERROR bufferPoolGet - no buffer free (id %u), %u repetitions\n",
               pbp->pool_id,
               pbp->failure_cnt);
      }
      pbp->failure_cnt++;
   }
#endif /* SHOW_NOFREEBUF_ERROR == 1 */

   BPOS_SPIN_UNLOCK_IRQRESTORE(&pbp->bp_spin_lock);

   return IFX_NULL;
}

static IFX_int32_t bufferPoolPut_intern(
      BUFFERPOOL  *pbp,
      IFX_void_t  *pb)
{
#if (DO_CHECKS_ON_PUT == 1)
   tBufferFooter *pf  = IFX_NULL;
#endif

   tBufferHeader *ph = GET_HDR_PTR(pb);

#if (DO_CHECKS_ON_PUT == 1) /* ********************************checks****/
   if (!checkMagic(ph))
   {
#if (SHOW_MAGIC_ERROR == 1)
      printf("\nERROR bufferPoolPut Magic   failure, <%04X>\n", ph->magic);
#endif
      errorCnt_MAGIC++;
      return BUFFERPOOL_ERROR;
   }
   if (!checkPattern(ph))
   {
#if (SHOW_PATTERN_WARNING == 1)
      bufferPoolDump("Pattern failed", pb);
      printf("\nWARNING bufferPoolPut Pattern failure\n");
#endif
      /* fix footer */
      pf = GET_FTR_PTR(ph);
      pf->pattern = CHECK_PATTERN;
      errorCnt_Pattern++;
   }
   if (!checkInUse(ph))
   {
#if (SHOW_INUSE_ERROR == 1)
      printf("\nERROR bufferPoolPut, buffer has been returned already\n");
      DUMP_STACK();
#endif
      errorCnt_PutFree++;
      return BUFFERPOOL_ERROR;
   }
#  endif                      /* ********************************checks****/

   /* Set the bufferstate to free and add it to the bufferpool */
   ph->state = BUFFER_STATE_FREE;
   ((tBufferInternal*)ph)->hdr.pPrev = pbp->pTail;
   ((tBufferInternal*)ph)->hdr.pNext = IFX_NULL;

   /* Add the link from the tail element to the new element (if available) */
   if (pbp->pTail != IFX_NULL)
      pbp->pTail->hdr.pNext = (tBufferInternal *) ph;
   /* If the bufferpool was empty, set the pHead pointer to the new buffer */
   if (pbp->pHead == IFX_NULL)
      pbp->pHead            = (tBufferInternal *) ph;
   /* Move the pTail pointer to the new buffer and increase  freeElements  */
   pbp->pTail               = (tBufferInternal *) ph;
   pbp->freeElements++;

   return BUFFERPOOL_SUCCESS;
}

/**
   Return a buffer to the bufferpool.

   \param   pb          A Pointer to the buffer to be returned.

   \return
   - \ref BUFFERPOOL_SUCCESS: if successful
   - \ref BUFFERPOOL_ERROR: when pointer to bufferpool is incorrect

   FIXME: return value should use IFX_SUCCESS or IFX_ERROR.
*/
IFX_int32_t bufferPoolPut(IFX_void_t *pb)
{
   IFX_int32_t  retval = BUFFERPOOL_ERROR;
   BUFFERPOOL   *pbp = IFX_NULL;
   tBufferHeader *ph  = IFX_NULL;

   if (pb != IFX_NULL)
   {
      /* Get relevant bufferpool */
      ph = GET_HDR_PTR(pb);
      if ((pbp = ph->pbp) == IFX_NULL)
      {
#        if (SHOW_WARNINGS == 1)
         printf("\nERROR bufferPoolPut - missing pool ptr\n");
#        endif
         errorCnt_PutNULL++;
         return BUFFERPOOL_ERROR;
      }
   }
   else
   {
#     if (SHOW_WARNINGS == 1)
      printf("\nERROR bufferPoolPut IFX_NULL\n");
#     endif
      errorCnt_PutNULL++;
      return BUFFERPOOL_ERROR;
   }

   BPOS_SPIN_LOCK_IRQSAVE(&pbp->bp_spin_lock);
   retval = bufferPoolPut_intern(pbp, pb);
   BPOS_SPIN_UNLOCK_IRQRESTORE(&pbp->bp_spin_lock);

   return retval;
}

/**
   Forcefully returns all buffers that carry a given owner ID back to the pool.

   \param   pbp         Pointer to a bufferpool management structure.
   \param   ownerId     A value defined by the caller.

   \return
   - \ref BUFFERPOOL_SUCCESS: if successful
   - \ref BUFFERPOOL_ERROR: when pointer to bufferpool is incorrect
*/
IFX_int32_t bufferPoolFreeAllOwnerId(BUFFERPOOL *pbp, IFX_uint32_t ownerId)
{
   /* From the bufferpool there is no link to the currently allocated buffers.
      So this walks over all buffers in all segments of the bufferpool.
      If a buffer is found which is in-use and also carries the given owner ID
      it is returned to the pool. */

   struct _BufferInternal *pBufferBlock;
   IFX_uint32_t            elementSize = 0;
   IFX_int32_t             ret = BUFFERPOOL_SUCCESS;

   if (IFX_NULL == pbp)
      {return BUFFERPOOL_ERROR;}

   BPOS_SPIN_LOCK_IRQSAVE(&pbp->bp_spin_lock);
   pBufferBlock = pbp->pBlockList;
   elementSize = pbp->hdrSize + pbp->bufSize + pbp->ftrSize;

   /* walk over all memory blocks */
   while ((pBufferBlock != IFX_NULL) && (ret == BUFFERPOOL_SUCCESS))
   {
      tBufferHeader      *pBufferHeader = (tBufferHeader *)pBufferBlock;
      tBufferFooter      *pBufferFooter;
      IFX_uint32_t        i;

      pBufferFooter = GET_FTR_PTR (pBufferHeader);

      /* walk over all buffers in this memory block */
      for (i=0; (ret == BUFFERPOOL_SUCCESS) &&
                (i < pBufferFooter->nBlockElementCount); i++)
      {
         if ((pBufferHeader->state == BUFFER_STATE_IN_USE) &&
             (pBufferHeader->ownerID == ownerId)             )
         {
            /* Header+1 is the userdata area of the buffer */
            ret = bufferPoolPut_intern(pbp, pBufferHeader+1);
         }

         /* this may result in an address that is one behind the buffer end */
         pBufferHeader = (tBufferHeader *)
                         (((IFX_uintptr_t) pBufferHeader) + elementSize);
      } /* for i */

      /* advance to the next memory block */
      pBufferBlock = pBufferBlock->hdr.pNextBlock;
   } /* while */
   BPOS_SPIN_UNLOCK_IRQRESTORE(&pbp->bp_spin_lock);

   return ret;
}

/**
   Change bufferpool owner ID.

   \param   pb          A Pointer to the buffer to be updated.
   \param   ownerID     A new owner id

   \return
   - \ref BUFFERPOOL_SUCCESS: if successful
   - \ref BUFFERPOOL_ERROR: when pointer to bufferpool is incorrect

   FIXME: return value should use IFX_SUCCESS or IFX_ERROR.
*/
IFX_int32_t bufferPoolChOwn(IFX_void_t *pb, IFX_uint32_t ownerID)
{
   tBufferHeader *ph  = IFX_NULL;
  /*  BUFFERPOOL    *pbp = IFX_NULL; */
   #if (DO_CHECKS_ON_PUT == 1)
   tBufferFooter *pf  = IFX_NULL;
   #endif

   if (pb != IFX_NULL)
   {
      ph = GET_HDR_PTR(pb);

      #if (DO_CHECKS_ON_PUT == 1) /* ********************************checks****/
      if (!checkMagic(ph))
      {
         #if (SHOW_MAGIC_ERROR == 1)
         printf("\nERROR bufferPoolSetOwnerID Magic   failure, <%04X>\n", ph->magic);
         #endif
         errorCnt_MAGIC++;
         return BUFFERPOOL_ERROR;
      }
      if (!checkPattern(ph))
      {
         #if (SHOW_PATTERN_WARNING == 1)
         bufferPoolDump("Pattern failed", pb);
         printf("\nWARNING bufferPoolSetOwnerID Pattern failure\n");
         #endif
         /* fix footer */
         pf = GET_FTR_PTR(ph);
         pf->pattern = CHECK_PATTERN;
         errorCnt_Pattern++;
      }
      if (!checkInUse(ph))
      {
         #if (SHOW_INUSE_ERROR == 1)
         printf("\nERROR bufferPoolSetOwnerID, buffer has been returned already\n");
         #endif
         return BUFFERPOOL_ERROR;
      }
      #endif                      /* ********************************checks****/

      /* Get relevant bufferpool */
      /* pbp = ph->pbp; */
      /* Set the new ownerID */
      ph->ownerID = ownerID;
   }
   else
   {
      #if (SHOW_WARNINGS == 1)
      printf("\nERROR bufferPoolSetOwnerID IFX_NULL\n");
      #endif
      return BUFFERPOOL_ERROR;
   }
   return BUFFERPOOL_SUCCESS;
}

/**
   Create a bufferpool report

   The report is an array of allocated memory which has to be freed
   by the caller upon processing the content.

   \param   pbp         Pointer to a bufferpool management structure.
   \param   count       Pointer to an integer for storing the count of entries
                        in the array.
   \param   **pReport   Pointer to an array with the buffer pool report.

   \return
   - \ref BUFFERPOOL_SUCCESS: if successful
   - \ref BUFFERPOOL_ERROR: when pointer to bufferpool is incorrect
*/
IFX_int32_t bufferPoolReport(
                        const BUFFERPOOL *pbp,
                        IFX_uint32_t *count,
                        struct buffer_report_entry **pReport)
{
   struct _BufferInternal *pBufferBlock;
   IFX_uint32_t            elementSize = 0;
   IFX_uint32_t            i = 0;
   struct buffer_report_entry *report;

   if ((pbp == IFX_NULL) || (count == IFX_NULL) || (*pReport = IFX_NULL))
      return BUFFERPOOL_ERROR;

   pBufferBlock = pbp->pBlockList;

   report = (struct buffer_report_entry *)
            TAPI_OS_Malloc(pbp->elements * sizeof(struct buffer_report_entry));

   if (report == IFX_NULL)
      return BUFFERPOOL_ERROR;

   elementSize = pbp->hdrSize + pbp->bufSize + pbp->ftrSize;

   /* walk over all memory blocks */
   while (pBufferBlock != IFX_NULL)
   {
      tBufferHeader *pBufferHeader = (tBufferHeader *)pBufferBlock;
      tBufferFooter *pBufferFooter;
      IFX_uint32_t j;

      pBufferFooter = GET_FTR_PTR (pBufferHeader);

      /* walk over all buffers in this memory block */
      for (j=0; j < pBufferFooter->nBlockElementCount; j++,
           pBufferHeader = (tBufferHeader *)
                           (((IFX_uintptr_t)pBufferHeader) + elementSize))
      {
         report[i].pBuffer = pBufferHeader+1;
         report[i].nOwnerID = pBufferHeader->ownerID;
         report[i].nState = pBufferHeader->state;
         i++;
      } /* for all buffers in the block */

      /* advance to the next memory block */
      pBufferBlock = pBufferBlock->hdr.pNextBlock;
   } /* while */

   /* return the array and the number of elements in the array*/
   *pReport = report;
   *count = pbp->elements;

   return BUFFERPOOL_SUCCESS;
}

/*******************************************************************************
Description:
   Set the ID of the bufferpool.
Arguments:
   pbp          - a handle (pointer) to the bufferpool
   growthLimit  - id (will be printed if the pool's elements are exhausted)
Return:
   nothing
*******************************************************************************/
IFX_void_t  bufferPoolIDSet   (const BUFFERPOOL *pbp, const IFX_uint32_t id)
{
   if (pbp != IFX_NULL)
      ((BUFFERPOOL *)pbp)->pool_id = id;
}

/*******************************************************************************
Description:
   Return the size of a buffer element managed by the bufferpool. All buffer
   in a buffer pool have the same size. This is the maximum size and not the
   used size of the buffer elements. This size is given by the 'bufferPoolInit'
   routine.
Arguments:
   pbp   - a handle (pointer) to the bufferpool
Return:
   The return value is the overall number of buffers managed by the bufferpool.
*******************************************************************************/
IFX_int32_t bufferPoolElementSize(const BUFFERPOOL *pbp)
{
   return pbp->bufSize;
}

/*******************************************************************************
Description:
   Return the overall number of buffers managed by the bufferpool.
Arguments:
   pbp   - a handle (pointer) to the bufferpool
Return:
   The return value is the overall number of buffers managed by the bufferpool
   or BUFFERPOOL_ERROR if pbp is a IFX_NULL pointer.
*******************************************************************************/
IFX_int32_t bufferPoolSize(const BUFFERPOOL *pbp)
{
   if (pbp == IFX_NULL)
      return BUFFERPOOL_ERROR;

   return pbp->elements;
}

/*******************************************************************************
Description:
   Return the number of currently free buffers
Arguments:
   pbp   - a handle (pointer) to the bufferpool
Return:
   The return value is the number of free buffers in the bufferpool
   or BUFFERPOOL_ERROR if pbp is a IFX_NULL pointer.
*******************************************************************************/
IFX_int32_t bufferPoolAvail(const BUFFERPOOL *pbp)
{
   if (pbp == IFX_NULL)
      return BUFFERPOOL_ERROR;

   return pbp->freeElements;
}

/*******************************************************************************
Description:
   Debug function -- under construction --
   Trace of the buffer content
Arguments:
   whatHappened   - string to be printed together with the buffer contents
   pb             - a handle (pointer) to a buffer
Return:
   BUFFERPOOL_SUCCESS or BUFFERPOOL_ERROR
*******************************************************************************/
IFX_int32_t bufferPoolDump(const IFX_char_t *whatHappened,
                           const IFX_void_t *pb)
{
   IFX_uint32_t i = 0;
   tBufferHeader *ph = IFX_NULL;
   tBufferFooter *pf = IFX_NULL;

   if (pb == IFX_NULL) {
      printf("ERROR no buffer to dump\n");
      return BUFFERPOOL_ERROR;
   }

   /*printf("bufferPoolDump @%p\n", pb);*/
   ph = GET_HDR_PTR(pb);
   pf = GET_FTR_PTR(ph);

   printf("%s\n", whatHappened);
   printf("HDR @[%lXh]  <", (IFX_ulong_t) ph);
   for (i=0; i < ph->pbp->hdrSize; i++)
   {
      printf("%02X", ((IFX_uint8_t *)ph)[i]);
      if (i < ph->pbp->hdrSize -1) printf(" ");
   }
   printf("> [%u] Bytes\n", i);

   printf("BUF @[%lXh]  <", (IFX_ulong_t) pb);
   for (i=0; i < GET_DATA_SIZE(ph); i++)
   {
      printf("%02X", ((IFX_uint8_t *)pb)[i]);
      if (i < GET_DATA_SIZE(ph) -1) printf(" ");
   }
   printf("> [%u] DataSize\n", GET_DATA_SIZE(ph));

   printf("FTR @[%lXh]  <", (IFX_ulong_t) pf);
   for (i=0; i < ph->pbp->ftrSize; i++)
   {
      printf("%02X", ((IFX_uint8_t *)pf)[i]);
      if (i < ph->pbp->ftrSize -1) printf(" ");
   }
   printf("> [%u] Bytes\n", i);

   return BUFFERPOOL_SUCCESS;
}
