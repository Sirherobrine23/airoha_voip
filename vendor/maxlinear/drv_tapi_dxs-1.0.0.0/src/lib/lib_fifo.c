/******************************************************************************

  Copyright 2014  Lantiq Deutschland GmbH
  Copyright 2020  Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/*
   \file lib_fifo.c
   This library implements a general FIFO functionality.

   The FIFO stores all data in a ring buffer of element structures.

   The design uses a preallocated ring buffer that would allow the fifoSize
   to grow after its initialisation while still providing a very efficient
   element handling. This is not yet implemented.
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include <drv_tapi_osmap.h>
#include "lib_fifo.h"

#if !defined(LINUX) || !defined(__KERNEL__)
   #include <string.h> /* memcpy(), memset() */
#endif /* !LINUX || !__KERNEL__ */


/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* ============================= */
/* Local type definitions        */
/* ============================= */
/** Header of each element in the FIFO. */
struct FifoElement {
   /* Pointer to the next header element in the FIFO. */
   struct FifoElement   *pNext;
};

/** The FIFO management structure */
struct _FIFO_ID {
   /** Pointer to the bottom (base) element's structure. */
   struct FifoElement   *pBottom;
   /** Pointer to the next element to be stored. */
   struct FifoElement   *pPut;
   /** Pointer to the last element that was stored*/
   struct FifoElement   *pLastPut;
   /** Pointer to the next element to be given. */
   struct FifoElement   *pGet;
   /** Total number of element structures in the ring buffer. */
   IFX_uint32_t         fifoSize;
   /** Number of element structures currently used. */
   IFX_uint32_t         fifoElements;
   /** Size of each element in the fifo. */
   IFX_uint32_t         elementSize;
};

/* ============================= */
/* Global variable definition    */
/* ============================= */

/* ============================= */
/* Global function declaration   */
/* ============================= */

/* ============================= */
/* Local function declaration    */
/* ============================= */

/* ============================= */
/* Local variable definition     */
/* ============================= */

/* ============================= */
/* Local function definition     */
/* ============================= */

/**
   Create a FIFO structure and initialise it

   \param  initialElements Number of elements to be stored in the fifo.

   \return
   Pointer to new created fifo management structure or IFX_NULL on error.
*/
FIFO_ID* fifoInit(const IFX_uint32_t initialElements,
                  const IFX_uint16_t elementSize)
{
   FIFO_ID              *pf;
   struct FifoElement   *pe;
   IFX_uint32_t         internalElementSize;
   IFX_uint32_t         i;

   internalElementSize = sizeof(struct FifoElement) + elementSize;

   /* Allocate the FIFO management structure and addtionally memory for
      the initial amount of elements the FIFO shall be able to handle. */
   pf = (FIFO_ID *)TAPI_OS_Malloc(sizeof(FIFO_ID) +
                                    (initialElements * internalElementSize));
   if (pf == IFX_NULL)
      return IFX_NULL;
   memset(pf, 0x00, sizeof(FIFO_ID) + (initialElements * internalElementSize));

   /* The array of elements starts behind the management structure. */
   pf->pBottom      = (struct FifoElement*)(pf+1);
   pf->pPut         = pf->pBottom;
   pf->pGet         = pf->pBottom;
   pf->pLastPut     = IFX_NULL;
   pf->fifoSize     = initialElements;
   pf->fifoElements = 0;
   pf->elementSize  = elementSize;

   /* Initialize the FIFO elements */
   for (i=1, pe = pf->pBottom; i < pf->fifoSize; i++)
   {
      pe->pNext = (struct FifoElement *)
           ((IFX_uint8_t *)pf->pBottom + (i * internalElementSize));
      pe = pe->pNext;
   }
   pe->pNext = pf->pBottom;

   return pf;
}


/**
   Reset the FIFO loosing all data

   \param  pf           Pointer to the fifo management structure.

   \return
   IFX_ERROR - if pf is NULL
   IFX_SUCCESS - in other cases.
*/
IFX_int32_t fifoReset(FIFO_ID *pf)
{
   if(pf == IFX_NULL)
      return IFX_ERROR;

   pf->pPut                = pf->pBottom;
   pf->pGet                = pf->pBottom;
   pf->pLastPut            = IFX_NULL;
   pf->fifoElements        = 0;
   return IFX_SUCCESS;
}


/**
   Add a new element to the tail of the FIFO

   \param  pf           Pointer to the fifo management structure.
   \param  pData        Pointer to the data to be stored in the FIFO.

   \return
   IFX_SUCCESS or IFX_ERROR if no more element can be added to the FIFO.
*/
IFX_int32_t fifoPut(FIFO_ID *pf, const IFX_void_t *pData)
{
   struct FifoElement *pe, *pNext;

   if (IFX_NULL == pf)
      return IFX_ERROR;

   pe    = pf->pPut;
   pNext = pe->pNext;

   /* check if the FIFO has room for a new element */
   if (pNext == pf->pGet)
      return IFX_ERROR;

   /* add the new element */
   memcpy (pe->pNext+1, pData, pf->elementSize);
   pf->pLastPut = pf->pPut;
   pf->pPut = pNext;
   pf->fifoElements++;

   return IFX_SUCCESS;
}


/**
   Retrieve the head element from the FIFO

   \param  pf           Pointer to the fifo management structure.
   \param  pData        Pointer to memory where to store the data.
                        If IFX_ERROR is returned no data is copied.

   \return
   - IFX_SUCCESS when an element is returned
   - IFX_ERROR if the fifo is empty or the pointer to the FIFO management
               structure is IFX_NULL.
*/
IFX_int32_t fifoGet(FIFO_ID *pf, IFX_void_t *pData)
{
   if ((pf == IFX_NULL) || (pData == IFX_NULL))
      return IFX_ERROR;

   /* check if the FIFO has an element available */
   if (pf->pPut == pf->pGet)
      return IFX_ERROR;

   memcpy(pData, pf->pGet->pNext+1, pf->elementSize);
   pf->pGet = pf->pGet->pNext;
   pf->fifoElements--;

   return IFX_SUCCESS;
}


/**
   Retrieve the first element from the FIFO without removing it

   \param  pf           Pointer to the fifo management structure.
   \param  pData        Pointer to memory where to store the data.
                        If IFX_ERROR is returned no data is copied.

   \return
   - IFX_SUCCESS when an element is returned
   - IFX_ERROR if the fifo is empty or the pointer to the FIFO management
               structure is IFX_NULL.
*/
IFX_int32_t fifoPeek(const FIFO_ID *pf, IFX_void_t *pData)
{
   if ((pf == IFX_NULL) || (pData == IFX_NULL))
      return IFX_ERROR;

   /* check if the FIFO has an element available */
   if (pf->pPut == pf->pGet)
      return IFX_ERROR;

   memcpy(pData, pf->pGet->pNext+1, pf->elementSize);

   return IFX_SUCCESS;
}


/**
   Retrieve the last element from the FIFO without removing the element

   \param  pf           Pointer to the fifo management structure.
   \param  pData        Pointer to memory where to store the data.
                        If IFX_ERROR is returned no data is copied.

   \return
   - IFX_SUCCESS when an element is returned
   - IFX_ERROR if the fifo is empty or the pointer to the FIFO management
               structure is IFX_NULL.
*/
IFX_int32_t fifoPeekTail(const FIFO_ID *pf, IFX_void_t *pData)
{
   if ((pf == IFX_NULL) || (pData == IFX_NULL))
      return IFX_ERROR;

   /* check if the FIFO has an element available */
   if (pf->pPut == pf->pGet)
      return IFX_ERROR;

   memcpy(pData, pf->pLastPut->pNext+1, pf->elementSize);

   return IFX_SUCCESS;
}


/**
   Returns if the fifo is empty or not

   \param  pf           Pointer to the fifo management structure.

   \return
   - 1 fifo is empty or the pointer to the FIFO management structure
       was IFX_NULL.
   - 0 fifo contains elements and is not empty
*/
IFX_uint8_t fifoEmpty(const FIFO_ID *pf)
{
   if (pf == IFX_NULL)
      return 1;

   return (pf->pPut == pf->pGet) ? 1 : 0;
}


/**
   Delete the FIFO structure

   The fifo must be empty before it can be deleted. This routines then
   frees all allocated resource. It terminates the fifo instance.

   \param  pf           Pointer to the fifo management structure.

   \return
   - IFX_ERROR if there are still elements in the fifo or the pointer to
     the FIFO management structure was IFX_NULL. The fifo is not
     deleted in this case. Please retrieve all elements from the fifo or
     call fifoReset() to empty the fifo.
   - IFX_SUCCESS the fifo was successfully deleted.
*/
IFX_int32_t fifoFree(FIFO_ID *pf)
{
   if (pf == IFX_NULL)
      return IFX_ERROR;

   /* check if the 'read' and the 'write' index point to the same fifo */
   /* element. Otherwise the fifo is not empty and can not be released */
   /* by this function. Please call explicite the 'fifoReset()' to     */
   /* reset the fifo before release the resources of the fifo.         */
   if (pf->pPut != pf->pGet)
   {
      return IFX_ERROR;
   }

   /* free all resources, which were allocated by the fifo */
   TAPI_OS_Free (pf);

   return IFX_SUCCESS;
}


/**
   Get the capacity of the FIFO (overall number of elements)

   \param  pf           Pointer to the fifo management structure.

   \return
   The number of elements that can be max. stored in the fifo.
*/
IFX_uint32_t fifoSize(const FIFO_ID *pf)
{
   if (pf == IFX_NULL)
      return 0;

   return(pf->fifoSize);
}


/**
   Get the number of elements currently in the FIFO

   \param  pf           Pointer to the fifo management structure.

   \return
   The number of elements currently in the FIFO.
*/
IFX_uint32_t fifoElements(const FIFO_ID *pf)
{
   if (pf == IFX_NULL)
      return 0;

   return(pf->fifoElements);
}


#ifdef DEBUG
/**
   Debug function to check the fifo data integrity

   Check the integrity of the FIFO by going through the linked list of
   FIFO elements checking that the number of linked elements equals the number
   of elements in the FIFO management structure.

   \param  pf           Pointer to the fifo management structure.

   \return
   IFX_SUCCESS or (-n) integrity error (negative number of cnt)
*/
IFX_int32_t fifoIntegrity(const FIFO_ID *pf)
{
   struct FifoElement *pe;
   IFX_uint32_t cnt = 0;

   pe = pf->pGet;
   while (pe != pf->pPut)
   {
      pe = pe->pNext;
      cnt++;
   }
   if (cnt != pf->fifoElements) {
      IFXOS_PRINT_USR_RAW ("ERROR fifoIntegrity fifoElements %d != cnt %d\n",
              pf->fifoElements, cnt);
      return -(IFX_int32_t)cnt;
   }
   cnt = 0;
   pe = pf->pBottom;
   do
   {
      pe = pe->pNext;
      cnt++;
   } while (pe != pf->pBottom);
   if (cnt != pf->fifoSize) {
      IFXOS_PRINT_USR_RAW ("ERROR fifoIntegrity fifoSize %d != cnt %d\n",
              pf->fifoSize, cnt);
      return -(IFX_int32_t)cnt;
   }

   return IFX_SUCCESS;
}
#endif /* DEBUG */
