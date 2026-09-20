/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.
  Copyright 2022-2024 MaxLinear, Inc.


  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_mbx.c
   This file contains the mailbox access functions.

   These functions allow the message exchange with the FW of the DUSLIC XS.
   The functions provide protection against concurrent access to the mailboxes
   for message exchange with the FW.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_errno.h"
#include "drv_dxs_access.h"
#include "drv_dxs_linux.h"
#include "drv_dxs_mbx.h"

#undef DXS_CMD_PRINT

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/** Size of the in message box  (Host data message box in)  in bytes */
#define DXS_MBI_SIZE          32 /* 8 words @ 32-bit */
/** Size of the out message box (Host data message box out) in bytes */
#define DXS_MBO_SIZE          32 /* 8 words @ 32-bit */

#define RW_WRITE 0
#define RW_READ  1

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
/*lint -e 754 */
/** Data structure for firmware command header */
struct dxs_cmd_hdr
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   /* Read write Access */
   IFX_uint32_t RW : 1;
   /* Reserved */
   IFX_uint32_t Res : 23;
   /* Length */
   IFX_uint32_t LENGTH : 8;
#else
   /* Length */
   IFX_uint32_t LENGTH : 8;
   /* Reserved */
   IFX_uint32_t Res : 23;
   /* Read write Access */
   IFX_uint32_t RW : 1;
#endif
} __PACKED__;

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_int32_t dxs_Wait4MBIspace(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t nCount);

static IFX_int32_t dxs_cmdWrite (
                        DXS_DEVICE_t *pDev,
                        IFX_uint32_t *pCmd,
                        IFX_uint8_t  nCount);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */
/** \defgroup DxsMbx DUSLIC XS Mailbox API */
/* @{ */
/**
   Internal: write command to the mailbox

   \param  pDev         Pointer to the device structure.
   \param  pCmd         Pointer to the buffer with the command. It contains the
                        command header and the data to be written.
   \param  nCount       Number of data BYTES to write, without command header.
                        Must not be larger than the maximum mailbox size.

   \return
   - DXS_statusOk       On success.
   - DXS_statusCmdMbWrErr On error.
                        Error can occur if command-inbox space is not
                        sufficient or if low level function/macros fail.

   \remarks
   Protection against tasks and interrupts needs to be done outside.
*/
IFX_int32_t dxs_cmdWrite(
                        DXS_DEVICE_t *pDev,
                        IFX_uint32_t *pCmd,
                        IFX_uint8_t nCount)
{
   IFX_int32_t ret = DXS_statusErr;

   /* increase count by the length of the command header */
   nCount += sizeof(struct dxs_cmd_hdr);

   /* The count must be a multiple of 4 bytes. */
   TAPI_ASSERT ((nCount % 4) == 0);

   /* wait for free space in command inbox */
   ret = dxs_Wait4MBIspace(pDev, nCount);

   if (ret == DXS_statusOk)
   {
      ret = DXS_RegWriteMulti(pDev, DXS_HOST_DATA,
                              (IFX_uint16_t *)pCmd, nCount / 2);
   }

   if (!DXS_SUCCESS (ret))
   {
      /* errmsg: Writing to command mailbox failed.*/
      RETURN_DEVSTATUS (DXS_statusCmdMbWrErr, IFX_NULL);
   }

   return DXS_statusOk;
}


/**
   Waits until the requested amount of space is available in the command-inbox
   or until the timeout occurs.

   The function returns immediately with success (DXS_statusOk) if the
   requested amount of space is available in the command-inbox. Otherwise it
   repeatedly reads the field ILEN of register HOST_LEN_CB until the requested
   amount of bytes become available in the command-inbox. A maximum time of
   (WAIT_POLLTIME * DXS_FIBXMS_POLL_LOOP)us can be waited in the process.
   Upon timeout the function resets the command-inbox, waits for end of the
   reset and returns the error DXS_statusCmdIbNoSpace.

   To optimise the code the actual access to the register is cached in a
   variable in the device context. This value is reduced with each request.
   When the cached value is too small to satisfy the request the cache is
   updated with the actual value from the register.

   \param  pDev         Pointer to the device structure.
   \param nCount        Requested amount of space in bytes to wait for.
                        No check is done on this parameter!

   \return
   - DXS_statusOk
   - DXS_statusCmdIbNoSpace

   \remarks
   This function does not perform any protection against concurrent accesses
   (either interrupt or multi-task). It is an obligation to the calling function
   to provide such protection.
*/
IFX_int32_t dxs_Wait4MBIspace(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t nCount)
{
   IFX_int32_t nWaitCnt = 0;
   IFX_uint16_t nReg = 0;

   do
   {
      if (pDev->nMbxCachedCbiLen < nCount)
      {
         DXS_RegRead(pDev, DXS_HOST_LEN, &nReg);
         /* shift to get number of bytes from 16bit words */
         pDev->nMbxCachedCbiLen = (nReg & 0xff) << 1;
      }

      if (nCount <= pDev->nMbxCachedCbiLen)
      {
         /* Sufficient free space in command-inbox to grant the request. */
         /* Reduce the cache by the amount that was just granted. */
         pDev->nMbxCachedCbiLen -= nCount;
         return DXS_statusOk;
      }
      /* Not enough free space in the command-inbox, we have to wait until more
         space becomes available and start the loop again after a short delay.
         The iterations are limited and the resulting timeout is:
         (DXS_WAIT_POLLTIME * DXT_FIBXMS_POLL_LOOP)us. */
      TAPI_OS_USecSleep(DXS_WAIT_POLLTIME);
   } while ((++nWaitCnt < DXS_FIBXMS_POLL_LOOP));

#if 0
   /* exceeded maximum wait time or device error,
      reset the command-inbox by writing CMDMBX_RES bit */
   nReg = 1;
   REG_WRITE_UNPROT(pDev, DXS_HOST_CMD, nReg);

   /* poll the HOST_CMD.CMDMBX_RES bit until cleared */
   do
   {
      REG_READ_UNPROT(pDev, DXS_HOST_CMD, &nReg);
   } while (nReg & 1);
#endif

   /* errmsg: Not enough inbox space for writing command. */
   RETURN_DEVSTATUS(DXS_statusCmdIbNoSpace, IFX_NULL);
}

#if 0
/**
   Returns free command mailbox inbox space.

   \param  pLLDev       Pointer to the device structure.
   \param  cmdmbx_size  Pointer to variable where to return the command inbox
                        size. No check is done on this parameter!

   \return
   IFX_SUCCESS or IFX_ERROR.
*/
IFX_int32_t DXS_TAPI_LL_GetCmdMbxSize(IFX_TAPI_LL_DEV_t *pLLDev,
                                             IFX_uint8_t *cmdmbx_size)
{
   const DXS_DEVICE_t *pDev = (DXS_DEVICE_t *)pLLDev;

   if (pDev == IFX_NULL)
      return IFX_ERROR;

   *cmdmbx_size = pDev->nMbxCachedCbiLen;

   return IFX_SUCCESS;
}
#endif


/**
   Write DUSLIC XS Command

   The data to be written as well as the length of the data is taken from the
   command that is passed as a pointer to this function.

   \param  pDev         Pointer to the device structure.
   \param  pCmd         Pointer to the buffer with the command. It contains
                        the 2 header command words needed and the data to be
                        written.

   \return
   - DXS_statusOk       On success.
   - DXS_statusCmdMbWrErr On error.
                        Error can occur if command-inbox space is not
                        sufficient or if low level function/macros fail.
*/
IFX_int32_t DXS_CmdWrite(
                        DXS_DEVICE_t *pDev,
                        IFX_uint32_t *pCmd)
{
   struct dxs_cmd_hdr *pMsgHead = (struct dxs_cmd_hdr *)pCmd;
   IFX_int32_t ret;

   if (sizeof(struct dxs_cmd_hdr) + pMsgHead->LENGTH > DXS_MBI_SIZE)
   {
      /* errmsg: Length of command invalid. */
      RETURN_DEVSTATUS(DXS_statusCmdLengthInvalid, IFX_NULL);
   }

   pMsgHead->RW = RW_WRITE;

   /* protect against concurrent tasks and interrupts */
   DXS_HOST_PROTECT (pDev);

   ret = dxs_cmdWrite(pDev, pCmd, pMsgHead->LENGTH);

   /* release protection for concurrent tasks and interrupts */
   DXS_HOST_RELEASE (pDev);

   /* log this write command */
   LOG_WR_CMD(pDev->nDevNr, pDev->nChannel, pCmd,
              (pMsgHead->LENGTH+sizeof(struct dxs_cmd_hdr)),
              !ret ? ret : pDev->nErr);

   if (ret)
   {
      RETURN_DEVSTATUS (DXS_statusCmdMbWrErr, IFX_NULL);
   }

   return DXS_statusOk;
}


/**
   DUSLIC XS Read Command

   \param  pDev         Pointer to the device structure.
   \param  pCmd         Pointer to the buffer with the command. It contains the
                        command header and the data to be written.
   \param  pData        Pointer to a buffer where to store the read command.

   \return
   - DXS_statusOk
   - DXS_statusCmdIbNoSpace
   - DXS_statusCmdMbWrErr
   - DXS_statusCmdObTimeout
   - DXS_statusCmdObRdErr     -  Reading from the event mailbox failed.
   - DXS_statusCmdObDataOvld
*/
IFX_int32_t DXS_CmdRead(DXS_DEVICE_t *pDev,
                        IFX_uint32_t *pCmd,
                        IFX_uint32_t *pData)
{
   struct dxs_cmd_hdr *pMsgHead = (struct dxs_cmd_hdr *)pCmd;
   IFX_int32_t ret = DXS_statusOk;
   IFX_uint8_t nReadBytes,
               nExpectedBytes,
               nCnt = 0;
   IFX_uint32_t *pWriteData = pData;

   if (pMsgHead->LENGTH > DXS_MBO_SIZE)
   {
      /* errmsg: Length of command invalid. */
      RETURN_DEVSTATUS(DXS_statusCmdLengthInvalid, IFX_NULL);
   }

   /* protect against concurrent tasks and interrupts */
   DXS_HOST_PROTECT (pDev);

   pDev->bOutBoxData = IFX_FALSE;

   /* Write the read request */
   pMsgHead->RW = RW_READ;
   if ((ret = dxs_cmdWrite(pDev, pCmd, 0)) != DXS_statusOk)
   {
      DXS_HOST_RELEASE (pDev);
      /* errno: Writing to command mailbox failed. */
      RETURN_DEVSTATUS (DXS_statusCmdMbWrErr, IFX_NULL);
   }

   /* This is the number of bytes we are going to read. */
   nExpectedBytes = sizeof(struct dxs_cmd_hdr) + pMsgHead->LENGTH;

   /* Wait for data in the mailbox. */
   ret = DXS_WaitForCmdMbxData(pDev);

   if (!DXS_SUCCESS (ret))
   {
      DXS_HOST_RELEASE (pDev);
      RETURN_DEVSTATUS (ret, IFX_NULL);
   }

   /* nCnt is a number of elements, 32-bit each */
   nCnt = fifo_count (pDev->cmd_obx_queue);
   nReadBytes = (nCnt * sizeof(uint32_t));

   if (nReadBytes > nExpectedBytes)
   {
      /* more data than expected */
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("DXS%d: more data in command outbox(%d) than requested(%d) ",
            pDev->nDevNr, nReadBytes, nExpectedBytes));
#ifdef DXS_CMD_PRINT
      while (nCnt--)
      {
         void *pdummy;
         uint32_t nData;

         /* Clear the fifo by reading it and print the content. */
         fifo_get (pDev->cmd_obx_queue, &pdummy, &nData);
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH, (" %08X", nData));
      }
#else
      /* Discard all data. The error code will report this event. */
      fifo_flush (pDev->cmd_obx_queue);
#endif
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("\n"));

      DXS_HOST_RELEASE (pDev);
      RETURN_DEVSTATUS (DXS_statusCmdObDataOvld, IFX_NULL);
   }

   while (nCnt--)
   {
      void *pdummy;

      /* read the data from the fifo to pData */
      fifo_get (pDev->cmd_obx_queue, &pdummy, pWriteData++);
   }

   /* release protection for concurrent tasks and interrupts */
   DXS_HOST_RELEASE (pDev);

   /* log this read command */
   LOG_RD_CMD(pDev->nDevNr, pDev->nChannel, pCmd,
              pData, nExpectedBytes, ret);

   return DXS_statusOk;
}


/**
   Download binary into patch ram.

   The given data is sent in to the mailbox. This code takes care of
   fragmenting it into chunks of the size of the mailbox.
   Note that the download does not need to read the mailbox length register
   and wait for enough space to be available because the DXS reads the patch
   faster out of the mailbox than it can be filled via the SPI interface.

   \param  pDev         Pointer to the device structure.
   \param  pBuffer      Pointer to Buffer for download.
   \param  nSize        Size of the buffer in bytes.
   \return
   Error code.
*/
int32_t DXS_DwldPatch ( DXS_DEVICE_t *pDev,
                        IFX_uint8_t *pBuffer,
                        IFX_uint32_t nSize)
{
   IFX_int32_t    ret = DXS_statusOk;
   IFX_uint32_t   pos = 0,
                  remaining = nSize;

   /* Find the maximum size a fragment may have to pass through SPI without
      incurring additional fragmentation there. */
   const IFX_uint32_t fragment_limit = DXS_spi_blocksize_get();
   IFX_uint32_t pBuf[DXS_MBI_SIZE >> 2] = {0};

   if ((pBuffer == NULL) || (nSize == 0))
   {
      /* errmsg: At least one parameter in function is wrong. */
      RETURN_DEVSTATUS (DXS_statusFuncParam, IFX_NULL);
   }

   /* protect against concurrent tasks and interrupts */
   DXS_HOST_PROTECT (pDev);

   /* protect during firmware download */
   DXS_FW_DL_PROTECT (pDev);

   /* write all data */
   while ((remaining > 0) && (ret == DXS_statusOk))
   {
      /* calculate length of fragment: note that count is in bytes */
      IFX_uint32_t fragment = (remaining > DXS_MBI_SIZE) ? DXS_MBI_SIZE : remaining;
      /* cppcheck-suppress knownConditionTrueFalse */
      fragment = (fragment > fragment_limit) ? fragment_limit : fragment;
      /* ensure that the fragment has a multiple of 4 bytes */
      fragment = fragment - (fragment % 4);
      /* copy into the output buffer */
      DXS_cpb2dw(pBuf, 0, &pBuffer[pos], fragment);

      ret = DXS_RegWriteMulti(pDev, DXS_HOST_DATA, (IFX_uint16_t*)pBuf, fragment>>1);
      /* increment position */
      pos += fragment;
      /* decrement count */
      remaining -= fragment;
   } /* while */

   /* release protection for firmware download */
   DXS_FW_DL_RELEASE (pDev);

   /* release protection for concurrent tasks and interrupts */
   DXS_HOST_RELEASE (pDev);

   RETURN_DEVSTATUS (ret, IFX_NULL);
}


/**
   DXS_ObxRead - read DATA outbox content

   \param  pDev         Pointer to the device structure.
   \param  pData        Pointer to a memory area where to store the read data.
   \param  length       On calling sets the limit how many 32-bit words to read.
                        On return holds the data length in 32-bit words.
*/
IFX_int32_t DXS_ObxRead( DXS_DEVICE_t *pDev,
                        IFX_uint32_t *pData,
                        IFX_uint8_t *length)
{
   IFX_uint16_t dxs_mbx_len_reg, words_to_read = 0;
   IFX_int32_t ret = DXS_statusOk;

   /* Determine how many words are available in the DXS mailbox fifo. */
   ret = DXS_RegRead(pDev, DXS_HOST_LEN, &dxs_mbx_len_reg);

   if (DXS_SUCCESS(ret))
   {
      /* Upper 8-bit has count of 16-bit words waiting in the outbox. */
      words_to_read = dxs_mbx_len_reg >> 8;

      /* validate the MBI length value */
      if (((words_to_read % 2) != 0) || (words_to_read > (DXS_MBI_SIZE / 2)))
      {
         /* invalid length value - return an error */
         words_to_read = 0;
         ret = DXS_statusEvtMbErr;
      }

      /* Apply limit given as parameter. */
      if (words_to_read > (*length * 2))
      {
         words_to_read = (*length * 2);
      }

      if (words_to_read > 0)
      {
         /* Read from HOST_DATA, the out mailbox. */
         ret = DXS_RegReadMulti(pDev, DXS_HOST_DATA,
                                (IFX_uint16_t *)pData, words_to_read);
      }
   }

   /* On error return a length of zero.
      Otherwise the number of words actually read. */
   *length = DXS_SUCCESS(ret) ? (words_to_read / 2) : 0;

   return ret;
}

/* @} */ /* DxsMbx */
