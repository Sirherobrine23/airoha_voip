/******************************************************************************

                              Copyright (c) 2009
                            Lantiq Deutschland GmbH
                     Am Campeon 3; 85579 Neubiberg, Germany

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/* ============================================================================
   Description : IFX Linux adaptation - memory allocation (Kernel Space)
   ========================================================================= */

#ifdef LINUX
#ifdef __KERNEL__

/** \file
   This file contains the IFXOS Layer implementation for LINUX Kernel 
   Memory Allocation.
*/

#ifndef IFXOS_LOCAL_CHECK
#  define IFXOS_LOCAL_CHECK               0
#else
#  undef  IFXOS_LOCAL_CHECK
#  define IFXOS_LOCAL_CHECK               1
#endif

/* ============================================================================
   IFX Linux adaptation - Global Includes - Kernel
   ========================================================================= */

#include <linux/kernel.h>
#ifdef MODULE
   #include <linux/module.h>
#endif
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "../include/ifx_types.h"
#include "../include/os_calls/ifxos_linux_drv.h"
#include "../include/os_calls/ifxos_debug.h"
#include "../include/os_calls/ifxos_memory_alloc.h"

#if (IFXOS_LOCAL_CHECK == 1)
#include <linux/hardirq.h>
#endif


/* ============================================================================
   IFX Linux adaptation - Kernel memory handling, malloc
   ========================================================================= */

/** \addtogroup IFXOS_MEM_ALLOC_LINUX_DRV
@{ */

#if ( defined(IFXOS_HAVE_BLOCK_ALLOC) && (IFXOS_HAVE_BLOCK_ALLOC == 1) )

/**
   LINUX Kernel - Allocate a continuous memory block of the given size [byte]
\par Implementation
   - Allocates a continuous memory block with the kernel function "kmalloc"
   - The option "GFP_KERNEL" is used for normal kernel allocation (may sleep)
   - This implementation is not allowed on interrupt level (may sleep)

\param
   memSize_byte   Size of the requested memory block [byte]

\return
   IFX_NULL in case of error, else
   pointer to the allocated memory block.
*/
IFX_void_t *LINUX_BlockAlloc(
               IFX_size_t memSize_byte)
{
   IFX_void_t *pMemBlock = IFX_NULL;
   TAPI_OS_CHECKPOINT

#if (IFXOS_LOCAL_CHECK == 1)
   if (in_interrupt())
   {
      IFXOS_PRN_INT_ERR_NL( IFXOS, IFXOS_PRN_LEVEL_ERR, 
         ("ERROR - kmalloc call within interrupt" IFXOS_CRLF));
      return (pMemBlock);
   }
#endif

   if(memSize_byte)
      pMemBlock = (IFX_void_t *)kmalloc((unsigned int)memSize_byte, GFP_KERNEL);

   return (pMemBlock);
}

/**
   LINUX Kernel - Free the given memory block.
\par Implementation
   Free a continious memory block with the kernel function "kfree"

\param
   pMemBlock   Points to the memory block to free.

\return
   NONE
*/
IFX_void_t LINUX_BlockFree(
               IFX_void_t *pMemBlock)
{
   TAPI_OS_CHECKPOINT

   if (pMemBlock)
   {
      kfree((void*)pMemBlock);
   }
   else
   {
      IFXOS_PRN_INT_ERR_NL( IFXOS, IFXOS_PRN_LEVEL_ERR, 
         ("WARNING - Cannot free NULL pointer" IFXOS_CRLF));
   }

   return;
}
#endif      /* #if ( defined(IFXOS_HAVE_BLOCK_ALLOC) && (IFXOS_HAVE_BLOCK_ALLOC == 1) ) */


#if ( defined(IFXOS_HAVE_MEM_ALLOC) && (IFXOS_HAVE_MEM_ALLOC == 1) )
/**
   LINUX Kernel - Allocate Memory Space from the OS

\par Implementation
   Allocates a memory block with the kernel function "vmalloc"

\param
   memSize_byte   Size of the requested memory block [byte]

\return
   IFX_NULL in case of error, else
   pointer to the allocated memory block.
*/
IFX_void_t *LINUX_MemAlloc(
               IFX_size_t memSize_byte)
{
   IFX_void_t *pMemBlock = IFX_NULL;
   TAPI_OS_CHECKPOINT

   if(memSize_byte)
      pMemBlock = (IFX_void_t*)vmalloc((unsigned long)memSize_byte);

   return (pMemBlock);
}

/**
   LINUX Kernel - Free Memory Space
\par Implementation
   Free a memory block with the kernel function "vfree"

\param
   pMemBlock   Points to the memory block to free.

\return
   NONE
*/
IFX_void_t LINUX_MemFree(
               IFX_void_t *pMemBlock)
{
   TAPI_OS_CHECKPOINT

   if (pMemBlock)
   {
      vfree((void*)pMemBlock);
   }
   else
   {
      IFXOS_PRN_INT_ERR_NL(IFXOS, IFXOS_PRN_LEVEL_WRN, 
         ("WARNING - Cannot vfree NULL pointer" IFXOS_CRLF));
   }

   return;
}
#endif      /* #if ( defined(IFXOS_HAVE_MEM_ALLOC) && (IFXOS_HAVE_MEM_ALLOC == 1) ) */

/** @} */


#endif      /* #ifdef __KERNEL__ */
#endif      /* #ifdef LINUX */

