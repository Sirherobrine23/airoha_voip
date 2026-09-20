/******************************************************************************

  Copyright 2014-2015 Lantiq Deutschland GmbH
  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017, 2020 Intel Corporation.
  Copyright 2021-2022 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_access.c
   Implementation of low level access functions.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_errno.h"
#include "drv_dxs_access.h"

#include "../tapi/drv_tapi_debug_buffer.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
#if (!defined(SPI_MAXBYTES_SIZE) ||\
     !defined(SPI_CS_SET) ||\
     !defined(spi_ll_read_write))
   #error Set SPI support macros in drv_config_user.h and compile again!
#endif /* SPI_MAXBYTES_SIZE| SPI_CS_SET | spi_ll_read_write */

#if (SPI_MAXBYTES_SIZE < 4)
#error SPI_MAXBYTES_SIZE cannot be set smaller than 4. The DUSLIC XS requires \
       at least 4 bytes for each SPI transfer cycle during which the chip \
       select is kept active.
/* Note that this limit is only for the case that the CS is handled by the SPI
   driver with each call of the spi_ll_read_write() macro. */
#endif /* (SPI_MAXBYTES_SIZE < 4) */

/* SPI header length in bytes */
#define DXS_SPI_HDR_LENGTH             2
/* SPI header manipulation flags and masks */
#define DXS_SPI_HDR_B0_R               0x80
#define DXS_SPI_HDR_B0_W               0x40
#define DXS_SPI_HDR_B0_RESERVED        0x3E
#define DXS_SPI_HDR_B1_I               0x01

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_int32_t dxs_spi_write(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t *pbuf,
                        IFX_uint32_t len);

static IFX_int32_t dxs_spi_read(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t *pbuf,
                        IFX_uint32_t len);

static IFX_int32_t dxs_spi_setup (
                        DXS_DEVICE_t *pDev);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

/**
   Write a value to one chip register.

   \param  pDev         Pointer to device struct.
   \param  offset       Register offset (address).
   \param  nValue       16-bit word value to write.

   \return
   Error code from dxs_spi_write().
*/
IFX_int32_t DXS_RegWrite(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t nValue)
{
   IFX_int32_t ret;
   IFX_uint16_t data[2] = {nValue, nValue};

#ifdef DXS_SPI_8BIT_ACCESS
   ret = dxs_spi_write(pDev, offset, data, sizeof(data));
#else
   ret = dxs_spi_write(pDev, offset, data, sizeof(IFX_uint16_t));
#endif

   tapi_debug_buffer_add_reg_entry(pDev->nDevNr, pDev->nChannel,
                                   TAPI_DBUF_SPI_REG_WRITE, offset,
                                   (IFX_uint8_t *) data, 2);

   LOG_WR_REG(pDev->nDevNr, pDev->nChannel, offset, data, 2);

   return ret;
}


/**
   Read a value from one chip register.

   \param  pDev         Pointer to device struct.
   \param  offset       Register offset (address).
   \param  pValue       Pointer to variable where to return
                        the 16-bit word value.

   \return
   Error code from dxs_spi_read().
*/
IFX_int32_t DXS_RegRead(DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t *pValue)
{
   IFX_int32_t ret;
   IFX_uint16_t data[2];

#ifdef DXS_SPI_8BIT_ACCESS
   ret = dxs_spi_read(pDev, offset, data, sizeof(data));
#else
   ret = dxs_spi_read(pDev, offset, data, sizeof(IFX_uint16_t));
#endif
   *pValue = data[0];

   tapi_debug_buffer_add_reg_entry(pDev->nDevNr, pDev->nChannel,
                                   TAPI_DBUF_SPI_REG_READ, offset,
                                   (IFX_uint8_t *) pValue, 2);

   LOG_RD_REG(pDev->nDevNr, pDev->nChannel, offset, pValue, 2);

   return ret;
}


/**
   Write multiple consecutive chip registers.

   \param  pDev         Pointer to device struct.
   \param  offset       Register offset (address).
   \param  pValue       Pointer to array with 16-bit word values to write.
   \param  count        Number of 16-bit words to write.

   \return
   Error code from dxs_spi_write().
*/
IFX_int32_t DXS_RegWriteMulti(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t *pValue,
                        IFX_uint8_t count)
{
   IFX_int32_t ret = dxs_spi_write(pDev, offset, pValue, (2*count));

   tapi_debug_buffer_add_reg_entry(pDev->nDevNr, pDev->nChannel,
                                   TAPI_DBUF_SPI_REG_WRITE, offset,
                                   (IFX_uint8_t *) pValue, count * 2);

   LOG_WR_REG(pDev->nDevNr, pDev->nChannel, offset, pValue, (2*count));

   return ret;
}


/**
   Read multiple consecutive chip registers.

   \param  pDev         Pointer to device struct.
   \param  offset       Register offset (address).
   \param  pValue       Pointer to array where to return the 16-bit word values.
   \param  count        Number of 16-bit words to read.

   \return
   Error code from dxs_spi_read().
*/
IFX_int32_t DXS_RegReadMulti(
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t offset,
                        IFX_uint16_t *pValue,
                        IFX_uint8_t count)
{
   IFX_int32_t ret = dxs_spi_read(pDev, offset, pValue, (2*count));

   tapi_debug_buffer_add_reg_entry(pDev->nDevNr, pDev->nChannel,
                                   TAPI_DBUF_SPI_REG_READ, offset,
                                   (IFX_uint8_t *) pValue, count * 2);

   LOG_RD_REG(pDev->nDevNr, pDev->nChannel, offset, pValue, (2*count));

   return ret;
}


/**
   Check if the device can be accessed by writing and reading one register.

   This is used to test if the HW is connected before doing the initialization.

   \param  pDev         Pointer to device struct.

   \return
   - DXS_statusOk
   - DXS_statusSpiAccErr
   - DXS_statusTestChipAccErr

   \remarks
   The selected register can hold only values from 0 to 31. So this test is
   hardcoded to check only this range.
*/
IFX_int32_t DXS_reg_access_test(
                        DXS_DEVICE_t *pDev)
{
   IFX_uint16_t original_value, read_val;
   IFX_uint32_t i = 0, errors = 0;

   /* Backup the original value. */
   IFX_int32_t ret = DXS_RegRead(pDev, ACCESS_TEST, &original_value);
   if (!DXS_SUCCESS(ret))
      RETURN_DEVSTATUS (ret, IFX_NULL);

   for (i = 0; i <= 31; i++)
   {
      IFX_uint16_t write_val = i;

      ret = DXS_RegWrite(pDev, ACCESS_TEST, write_val);
      if (!DXS_SUCCESS(ret))
         RETURN_DEVSTATUS (ret, IFX_NULL);

      ret = DXS_RegRead(pDev, ACCESS_TEST, &read_val);
      if (!DXS_SUCCESS(ret))
         RETURN_DEVSTATUS (ret, IFX_NULL);

      if (write_val != read_val)
      {
         errors++;
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("ERROR! DXS dev %d access mismatch wr:0x%04X rd:0x%04X\n",
               pDev->nDevNr, write_val, read_val));
      }
   }

   /* Restore the original value. */
   ret = DXS_RegWrite(pDev, ACCESS_TEST, original_value);
   if (!DXS_SUCCESS(ret))
      RETURN_DEVSTATUS (ret, IFX_NULL);

   if (errors > 0)
   {
      /* errmsg: Test chip access failed. */
      ret = DXS_statusTestChipAccErr;
   }

   RETURN_DEVSTATUS(ret, IFX_NULL);
}


/* ************************ endianess correct copy ****************************/

/**
   Copy a 16-bit-word buffer into a byte buffer respecting the endianess.

   Odd and even offsets and length are handled by this function.

   \param  pBbuf        Pointer to byte buffer. (Destination)
   \param  pWbuf        Pointer to 16-bit-word buffer. (Source)
   \param  nWoffset     Offset in bytes within the source buffer.
   \param  nB           Number of bytes to be copied.
*/
void DXS_cpw2b (        IFX_uint8_t *pBbuf,
                        const IFX_uint16_t * const pWbuf,
                        const IFX_uint32_t nWoffset,
                        const IFX_uint32_t nB)
{
   IFX_uint32_t i;

   for (i = 0; i < nB; i++)
   {
      IFX_uint16_t nWord = pWbuf[(nWoffset + i) >> 1];
      pBbuf[i] = (IFX_uint8_t)
                 ((((nWoffset+i) % 2) ? nWord : nWord >> 8) & 0xFF);
   }
}

/**
   Copy a 32-bit-word buffer into a byte buffer respecting the endianess.

   Odd and even offsets and length are handled by this function.

   \param  pBbuf        Pointer to byte buffer. (Destination)
   \param  pWbuf        Pointer to 16-bit-word buffer. (Source)
   \param  nWoffset     Offset in bytes within the source buffer.
   \param  nB           Number of bytes to be copied.
*/
static void DXS_cpdw2b (IFX_uint8_t *pBbuf,
                        const IFX_uint32_t * const pWbuf,
                        const IFX_uint32_t nWoffset,
                        const IFX_uint32_t nB)
{
   IFX_uint32_t i = 0;

   for (i = 0; i < nB; i++)
   {
      IFX_uint32_t nWord = pWbuf[(nWoffset + i) >> 2];
      pBbuf[i] = (IFX_uint8_t)
                 ((nWord >> ((3 - ((nWoffset + i) % 4)) * 8)) & 0xFF);
   }
}

/**
   Copy a byte buffer into a 16-bit-word buffer respecting the endianess.

   In place copy is possible but otherwise the buffers must not overlap.
   When an odd number of bytes is to be copied the missing byte in the
   16-bit-word is filled with zero.

   \param  pWbuf        Pointer to 16-bit-word buffer. (Destination)
   \param  pBbuf        Pointer to byte buffer. (Source)
   \param  nB           Number of bytes to be copied.
*/
void DXS_cpb2w (        IFX_uint16_t *pWbuf,
                        const IFX_uint8_t * const pBbuf,
                        IFX_uint32_t nB)
{
   IFX_uint32_t i = 0;

   for (i = 0; i < nB; i += 2)
   {
      /* Copy each byte separately into the word buffer. Make sure that when
         an index out of range is addressed zero is copied instead. */
      IFX_uint16_t nWord  = (IFX_uint16_t)pBbuf[i+0] << 8;
      if ((i+1) < nB)
      {
         /* LSB is only added when index is within range. */
         nWord |= (IFX_uint16_t)pBbuf[i+1];
      }
      pWbuf[i>>1] = nWord;
   }
}

/**
   Copy a byte buffer into a 32-bit-word buffer respecting the endianess.

   When an odd number of bytes is to be copied the missing bytes in the
   32-bit-word are filled with zeroes. Also in the first word the bytes
   which are skipped because of an offset are filled with zeroes.

   \param  pDWbuf       Pointer to 32-bit-word buffer. (Destination)
   \param  nWoffset     Offset in bytes within the destination buffer.
   \param  pBbuf        Pointer to byte buffer. (Source)
   \param  nB           Number of bytes to be copied.
*/
void DXS_cpb2dw (       IFX_uint32_t *pDWbuf,
                        const IFX_uint32_t nWoffset,
                        const IFX_uint8_t* const pBbuf,
                        const IFX_uint32_t nB)
{
   IFX_uint32_t i = 0,
                j = 0;
   IFX_uint32_t word32 = 0UL;

   for (i = 0; i < nB; i++)
   {
      j = nWoffset + i;

      word32 |= (IFX_uint32_t)pBbuf[i] << ((3 - (j % 4)) * 8);

      if ((j % 4) == 3)
      {
         pDWbuf[j / 4] = word32;
         word32 = 0UL;
      }
   }

   if ((i > 0) && ((j % 4) != 3))
   {
      pDWbuf[j / 4] = word32;
   }
}

/***************************** SPI bus adaption ******************************/

/**
   Init the SPI interface that is used for communication with the DUSLIC XS

   \param  pDev         Pointer to device structure.
*/
IFX_void_t dxs_init_spi (DXS_DEVICE_t *pDev)
{
#if !defined(SPI_INIT)
   TAPI_UNUSED(pDev);
#endif

   /* initialize SPI access protection semaphore */
   TAPI_OS_MutexInit (&pDev->mtxSpiAcc);

   /* macro defined in drv_config_user.h */
   /* This macro must be executed before the first chip access is done */
#ifdef SPI_INIT
   SPI_INIT(pDev);
#endif

#ifdef DXS_SPI_MODE_SET
   DXS_SPI_MODE_SET(pDev, 3);
#endif

#ifdef DXS_SPI_BAUDRATE_SET
   DXS_SPI_BAUDRATE_SET(pDev, 100000);
#endif
}


/**
   Release the SPI interface that is used for communication with the DUSLIC XS

   \param  pDev         Pointer to device structure.
*/
IFX_void_t dxs_exit_spi (DXS_DEVICE_t *pDev)
{
#if !defined(SPI_EXIT)
   TAPI_UNUSED(pDev);
#endif /* !defined(SPI_EXIT) */

   /* macro defined in drv_config_user.h */
#ifdef SPI_EXIT
   DXS_SPI_PROTECT(pDev);
   SPI_EXIT(pDev);
   DXS_SPI_RELEASE(pDev);
#endif /* #ifdef SPI_EXIT */
}


/**
   Write a number of bytes to a given DXS register using SPI.

   If the length of the data exceeds the SPI_MAXBYTES_SIZE this function
   fragments the data into multiple blocks and calls the SPI write interface
   repeatedly until all data is transmitted.

   \param  pDev         Pointer to device structure.
   \param  offset       DXS register address.
   \param  pbuf         Pointer to buffer with data to be written.
   \param  len          Number of bytes to write.

   \return
   DXS_statusOk or error code.
*/
IFX_int32_t dxs_spi_write(DXS_DEVICE_t *pDev,
                          IFX_uint8_t offset,
                          IFX_uint16_t *pbuf,
                          IFX_uint32_t len)
{
   IFX_uint8_t   tx_buf[SPI_MAXBYTES_SIZE];
   IFX_uint32_t  tx_buf_length,
                 written,
                 fragment_length;
   IFX_int32_t   ret = DXS_statusOk;

   /* (len == 0) not allowed : assertion */
   TAPI_ASSERT(len != 0);
   /* offset must be even, as we do 16bit access : assertion */
   TAPI_ASSERT((offset & 0x1) == 0);

   /* Wait until SPI becomes accessible and configure it. */
   if (dxs_spi_setup(pDev) != DXS_statusOk)
   {
      pDev->nErr = DXS_statusSpiAccErr;
      return DXS_statusSpiAccErr;
   }

   /* This variable counts the number of bytes already written. */
   written = 0;

   /* Build SPI header (16-bit word) */
   tx_buf[0] = DXS_SPI_HDR_B0_W | DXS_SPI_HDR_B0_RESERVED;
   tx_buf[1] = offset;
#ifndef DXS_SPI_8BIT_ACCESS
   /* Set auto increment if register isn't a mailbox register. */
   if ((len > 2) && (offset != DXS_HOST_DATA))
   {
      tx_buf[1] |= DXS_SPI_HDR_B1_I;
   }
#endif
   /* The tx_buf now contains the SPI header. */
   tx_buf_length = DXS_SPI_HDR_LENGTH;
#ifdef SPI_DEVICE_RESERVE
   /* Select which device to address */
   SPI_DEVICE_RESERVE(pDev->nDevNr);
#endif /* #ifdef SPI_DEVICE_RESERVE */

   /* This is the loop that fragments the data to be written. */
   while ((ret == DXS_statusOk) && (written < len))
   {
      /* Calculate the amount of payload bytes that can be sent during one
         write call. */
      fragment_length = len - written; /* remaining bytes to transfer */
      if (fragment_length > (SPI_MAXBYTES_SIZE - tx_buf_length))
      {
         /* Limit the transfer to the maximum the SPI can transmit. */
         fragment_length = (SPI_MAXBYTES_SIZE - tx_buf_length);
      }
      if ((offset == DXS_HOST_DATA) && (fragment_length > 4))
      {
         /* For mailboxes ensure that the fragment has a multiple of 4 bytes. */
         fragment_length = fragment_length - (fragment_length % 4);
      }
      /* Copy the payload into the write buffer with correct endianess. */
      if (offset == DXS_HOST_DATA)
      {
         /* The mailbox has 32-bit datawidth. */
         DXS_cpdw2b(tx_buf+tx_buf_length,
                    (IFX_uint32_t *)pbuf, written, fragment_length);
      }
      else
      {
         DXS_cpw2b(tx_buf+tx_buf_length, pbuf, written, fragment_length);
      }

      /* Increment by the number of payload bytes written. */
      tx_buf_length += fragment_length;
      written += fragment_length;

      /* Activate DXS by setting chip select line. (low means active) */
      SPI_CS_SET (pDev->nDevNr, IFX_LOW);

      /* Access SPI for writing (read ptr is NULL). */
      ret = spi_ll_read_write (pDev, tx_buf, tx_buf_length, NULL, 0);
      /* ret is evaluated before the next iteration. */

      /* Deactivate DXS by setting chip select line. (high means inactive) */
      SPI_CS_SET(pDev->nDevNr, IFX_HIGH);

      /* Preserve and reuse the SPI header for the next fragment. */
      tx_buf_length = DXS_SPI_HDR_LENGTH;
   }

#ifdef SPI_DEVICE_RELEASE
   /* Unselect the device */
   SPI_DEVICE_RELEASE(pDev->nDevNr);
#endif /* #ifdef SPI_DEVICE_RELEASE */

   return ret;
}


/**
   Read a number of bytes from a given DXS register using SPI.

   If the length of the data to be read exceeds the SPI_MAXBYTES_SIZE this
   function calls the SPI read function repeatedly until all data is read
   and reassembles the data in the return buffer.

   \param  pDev         Pointer to device structure.
   \param  offset       DXS register address.
   \param  pbuf         Pointer to buffer where to return the data.
   \param  len          Number of bytes to be read.

   \return
   DXS_statusOk or error code.
*/
IFX_int32_t dxs_spi_read(DXS_DEVICE_t *pDev,
                         IFX_uint8_t offset,
                         IFX_uint16_t *pbuf,
                         IFX_uint32_t len)
{
   IFX_uint8_t   tx_buf[SPI_MAXBYTES_SIZE],
                 rx_buf[SPI_MAXBYTES_SIZE];
   IFX_uint32_t  tx_buf_length,
                 read,
                 fragment_length,
                 rx_data_offset = 0;
   IFX_int32_t   ret = DXS_statusOk;

   /* (len == 0) not allowed : assertion */
   TAPI_ASSERT(len != 0);
   /* offset must be even, as we do 16bit access : assertion */
   TAPI_ASSERT((offset & 0x1) == 0);

   /* Wait until SPI becomes accessible and configure it. */
   if (dxs_spi_setup(pDev) != DXS_statusOk)
   {
      pDev->nErr = DXS_statusSpiAccErr;
      return DXS_statusSpiAccErr;
   }

   /* tx_buf is the buffer for writing the read request. Because SPI in general
      is full duplex the complete buffer is set to zero to avoid sending random
      data after the SPI header that we set below. */
   memset (tx_buf, 0x00, sizeof(tx_buf));
   /* Build SPI header (16-bit word) */
   tx_buf[0] = DXS_SPI_HDR_B0_R | DXS_SPI_HDR_B0_RESERVED;
   tx_buf[1] = offset;
#ifndef DXS_SPI_8BIT_ACCESS
   /* Set auto increment if register isn't a mailbox register. */
   if ((len > 2) && (offset != DXS_HOST_DATA))
   {
      tx_buf[1] |= DXS_SPI_HDR_B1_I;
   }
#endif
   /* The tx_buf contains now the SPI header. */
   tx_buf_length = DXS_SPI_HDR_LENGTH;

#ifdef SPI_DEVICE_RESERVE
   /* Select which device to address */
   SPI_DEVICE_RESERVE(pDev->nDevNr);
#endif /* #ifdef SPI_DEVICE_RESERVE */

   /* Loop while more data needs to be read. */
   for (read = 0; (ret == DXS_statusOk) && (read < len); read += fragment_length)
   {
      /* Calculate the amount of payload bytes that can be read during one
         read call. */
      fragment_length = len - read; /* remaining bytes to read */
      if (fragment_length > (SPI_MAXBYTES_SIZE - tx_buf_length))
      {
         /* Limit the transfer to the maximum the SPI can transmit. */
         fragment_length = (SPI_MAXBYTES_SIZE - tx_buf_length);
      }
      if ((offset == DXS_HOST_DATA) && (fragment_length > 4))
      {
         /* For mailboxes ensure that the fragment has a multiple of 4 bytes. */
         fragment_length = fragment_length - (fragment_length % 4);
      }

      /* Activate DXS by setting chip select line. (low means active) */
      SPI_CS_SET(pDev->nDevNr, IFX_LOW);

      /* Access SPI for writing and reading. */
      ret = spi_ll_read_write (pDev, tx_buf, tx_buf_length,
                                     rx_buf, fragment_length);

      /* Deactivate DXS by setting chip select line. (high means inactive) */
      SPI_CS_SET(pDev->nDevNr, IFX_HIGH);

      if (ret == DXS_statusOk)
      {
#ifndef DXS_SPI_READ_HALF_DUPLEX
         /* Received data is at offset where the write ends. */
         rx_data_offset = DXS_SPI_HDR_LENGTH;
#endif

         /* Copy the byte buffer into the read buffer respecting
            the endianess. */
         if (offset == DXS_HOST_DATA)
         {
            /* The mailbox has 32-bit datawidth. */
            DXS_cpb2dw ((IFX_uint32_t *)(pbuf + (read/2)), 0,
                     rx_buf + rx_data_offset, fragment_length);
         }
         else
         {
            DXS_cpb2w (pbuf + (read/2),
                     rx_buf + rx_data_offset, fragment_length);
         }
      }

      /* ret is evaluated before the next iteration. */
   }

#ifdef SPI_DEVICE_RELEASE
   /* Unselect the device */
   SPI_DEVICE_RELEASE(pDev->nDevNr);
#endif

   return ret;
}


/**
   Return the number of payload bytes that can be transferred over SPI
   without being fragmented into multiple SPI transfers.

   \return
   Maximum number of bytes for a single SPI transfer.
*/
const IFX_uint32_t DXS_spi_blocksize_get(void)
{
   return SPI_MAXBYTES_SIZE - DXS_SPI_HDR_LENGTH;
}


/**
   Wait for the SPI interface of the DXS to become accessible and set the
   DXS IRQ behaviour in the configuration register.

   After a reset via the RESET_N pin (hard reset), the host controller must
   first wait for the SPI interface to become accessible again and then
   configure the basic behavior of the DXS (the polarity of the interrupt
   line and behavior of the interrupt acknowledge) before it can access
   the DUSLIC XS.

   \param  pDev         Pointer to the device structure.

   \return
   -DXS_statusOk
   -DXS_statusSpiAccErr
*/
static IFX_int32_t dxs_spi_setup(DXS_DEVICE_t *pDev)
{
   const IFX_uint16_t nRegWrite = (DXS_REG_CFG_SC_MD_SCON | DXS_REG_CFG_8BIT_EN);
   IFX_uint16_t nLoop = 0;
   IFX_uint16_t nRetry = 0;

   if (pDev->nDevState & (DS_SPI_ACTIVE | DS_SPI_SETUP))
   {
      /* No need to configure more than once. */
      return DXS_statusOk;
   }

   /* SPI access is not configured */
   if (pDev->pSpiDev == IFX_NULL)
   {
      pDev->nDevState &= ~DS_SPI_SETUP;
      RETURN_DEVSTATUS(DXS_statusSpiAccErr, IFX_NULL);
   }

   /* Set flag that is tested above to get out of the recursion that happens
      when DXS_RegWrite() is called below. */
   pDev->nDevState |= DS_SPI_SETUP;

   while (nLoop <= 400)
   {
      IFX_uint16_t nRegRead = 0;
      /* Write SPI configuration. */
      IFX_int32_t ret = DXS_RegWrite(pDev, DXS_HOST_CFG, nRegWrite);
      if (ret != DXS_statusOk)
         break;

      /* Read configuration back. */
      ret = DXS_RegRead(pDev, DXS_HOST_CFG, &nRegRead);
      if (ret != DXS_statusOk)
         break;

      /* While the SPI is not accessible it returns 0xFFFF. During this
         we loop until we read a value which is different. */

      if (nRegWrite == nRegRead)
      {
         pDev->nDevState &= ~DS_SPI_SETUP;
         pDev->nDevState |= DS_SPI_ACTIVE;
         return ret;
      }

      if (nRegRead != 0xFFFF)
      {
         /* Retry up to 3 times when read and write do not match. */
         if (nRetry >= 3)
         {
            break;
         }
         nRetry++;
      }

      /*lint -save -e(62) 'udelay' incompatible types for operator ":" */
      /* Wait 5ms between register accesses. */
      TAPI_OS_MSecSleep(5);
      nLoop++;
   };

   pDev->nDevState &= ~DS_SPI_SETUP;
   RETURN_DEVSTATUS(DXS_statusSpiAccErr, IFX_NULL);
}
