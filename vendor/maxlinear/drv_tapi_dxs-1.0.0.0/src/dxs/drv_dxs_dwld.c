/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_dwld.c
   This file contains the implementation of the DUSLIC XS patch download.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_init.h"
#include "drv_dxs_access.h"
#include "drv_dxs_dwld.h"
#include "drv_dxs_mbx.h"
#ifdef TAPI_FEAT_LX_COMPAT
#include "drv_dxs_io_types_32.h"
#endif /* TAPI_FEAT_LX_COMPAT */

#include "../tapi/drv_tapi_debug_buffer.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
/* FW download container header struct values */
#define DXS_FW_TYPE_GLOBAL_V1   0xD0000001  /* type 0xD000, version 0x0001 */
#define DXS_FW_GLOBAL_MAGIC     0x44585346  /* 'DXSF' character sequence */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
/* FW download container header (big endian) */
struct fw_container_header
{
   /* Type and Version identifier */
   IFX_uint32_t   nType;
   /* Length of the payload following this header */
   IFX_uint32_t   nLength;
   /* MAGIC value for endianess checking */
   IFX_uint32_t   nMagic;
   /* Version, each of the 4 bytes represents a digit */
   IFX_uint32_t   nVersion;
   /* Epoch timestamp */
   IFX_uint32_t   nTimestamp;
   /* Offset of the DXS V11 FW within the payload section */
   IFX_uint32_t   nDxsV11FwOffset;
   /* Length of the DXS V11 FW within the payload section */
   IFX_uint32_t   nDxsV11FwLength;
   /* Offset of the DXS1 V12 FW within the payload section */
   IFX_uint32_t   nDxs1V12FwOffset;
   /* Length of the DXS1 V12 FW within the payload section */
   IFX_uint32_t   nDxs1V12FwLength;
   /* Offset of the DXS2 V12 FW within the payload section */
   IFX_uint32_t   nDxs2V12FwOffset;
   /* Length of the DXS2 V12 FW within the payload section */
   IFX_uint32_t   nDxs2V12FwLength;
};

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */
/**
   Download firmware patch and reboot

   \param  pDev         Pointer to the device structure.
   \param  pBuffer      Pointer to Buffer for download.
   \param  nSize        Size of the buffer in bytes.

   \return
   - DXS_statusOk
   - DXS_statusParam
   - DXS_statusNoFwDwld
   - DXS_statusSetBootCfgErr
   - DXS_statusCtrlResErr
   - DXS_statusDwldBinErr
   - DXS_statusSpiAccErr
   - DXS_statusFwDwldTimeout
*/
static IFX_int32_t DXS_DwldFirmware (
                        DXS_DEVICE_t *pDev,
                        IFX_uint8_t *pBuffer,
                        IFX_uint32_t nSize)
{
   /* Reset controller to start the boot process from SPI */
   IFX_int32_t err = DXS_ResetController (pDev, DXS_BOOT_SPI);

   if (DXS_statusOk != err)
   {
      /* errmsg: Controller reset failed. */
      err = DXS_statusCtrlResErr;
   }

   tapi_debug_buffer_add_user_entry("dxs #%d firmware download", pDev->nDevNr);

   /* now download firmware */
   if (DXS_statusOk == err)
   {
      /* write PRAM binary */
      err = DXS_DwldPatch (pDev, pBuffer, nSize);
   }

   if (DXS_statusOk != err)
   {
      /* errmsg: Download of the firmware binary failed. */
      err = DXS_statusDwldBinErr;
   }

   /* Check the success of the download by reading the boot info register. */
   if (DXS_statusOk == err)
   {
      if (DXS_Wait4BootFinished(pDev, 10) != DXS_statusOk)
      {
         /* errmsg: Firmware download timeout. */
         err = DXS_statusFwDwldTimeout;
      }
   }

   if (DXS_statusOk == err)
   {
      pDev->nDevState |= DS_FW_DLD;
      TRACE(TAPI_DXS, DBG_LEVEL_LOW,
            ("INFO: DXS_DwldFirmware returned successfully\n"));

      /* Set bootmode back to ROM in case a recovery is needed. */
      err = DXS_SetBootConfig(pDev, DXS_BOOT_ROM);
      if (DXS_statusOk != err)
      {
         /* errmsg: Setting of boot configuration register failed. */
         err = DXS_statusSetBootCfgErr;
      }
   }
   else
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("INFO: DXS_DwldFirmware returned an error\n"));
   }

   RETURN_DEVSTATUS(err, IFX_NULL);
}


/**
   Download firmware patch and reboot

   Ensure that the FW binary is a container with FW patches for the different
   chip variants. Select the FW patch matching the chip variant and call the
   actual download.

   \param  pDev         Pointer to the device structure.
   \param  pFwDwld      Pointer to DXS_FW_Download_t structure.

   \return
   - DXS_statusOk
   - DXS_statusParam
   - DXS_statusNoFwDwld
   - DXS_statusSetBootCfgErr
   - DXS_statusCtrlResErr
   - DXS_statusDwldBinErr
   - DXS_statusSpiAccErr
   - DXS_statusFwDwldTimeout
   - DXS_statusFwDwldFail
*/
IFX_int32_t DXS_DwldFirmwareSelect (
                        DXS_DEVICE_t *pDev,
                        const DXS_FW_Download_t *pFwDwld)
{
   struct fw_container_header header;
   IFX_uint32_t header_length = sizeof(header);
   IFX_int32_t ret = DXS_statusOk;

   /* check if the pointer is valid */
   if (IFX_NULL == pFwDwld)
   {
      pDev->nErr = DXS_statusParam;
      /* errmsg: At least one parameter is wrong. */
      RETURN_DEVSTATUS(DXS_statusParam, IFX_NULL);
   }

   if (pFwDwld->nEdspFlags & DXS_NO_FW_DWLD)
   {
      /* no firmware download is done, because no PRAM buffer was provided,
         ROM FW used */
      pDev->nDevState &= ~DS_FW_DLD;
      RETURN_DEVSTATUS(DXS_statusOk, IFX_NULL);
   }

   /* Set struct for easy access to the container header. */
   if (pFwDwld->pram_size < header_length)
   {
      /* errmsg: Firmware download failed. */
      RETURN_DEVSTATUS(DXS_statusFwDwldFail, IFX_NULL);
   }

   /* copy the file header */
   DXS_cpb2dw((IFX_uint32_t *)&header, 0, pFwDwld->pPRAMfw, sizeof(header));

   /* Process the container after verifying the header. */
   if (header.nType == DXS_FW_TYPE_GLOBAL_V1 &&
       header.nLength + header_length == pFwDwld->pram_size &&
       header.nMagic == DXS_FW_GLOBAL_MAGIC &&
       header.nVersion != 0 &&
       header.nTimestamp != 0 &&
       header.nLength == header.nDxsV11FwLength +
                         header.nDxs1V12FwLength +
                         header.nDxs2V12FwLength)
   {
      /* The container header (header version 1) is ok. */

      /* Identify the chip to determine which FW patch is needed. */
      ret = DXS_ReadFwVersion(pDev);
      if (DXS_statusOk != ret)
      {
         /* errmsg: Firmware download failed. */
         RETURN_DEVSTATUS(DXS_statusFwDwldFail, IFX_NULL);
      }

      switch (pDev->fw_vers.MAJ & 0x7F)  /* ignore test-bit in major nr. */
      {
      case 1:
         /* ROM version 1.x.x */
         if (header.nDxsV11FwLength > 0)
         {
            ret = DXS_DwldFirmware (pDev, pFwDwld->pPRAMfw + header_length +
                     header.nDxsV11FwOffset,
                     header.nDxsV11FwLength);
         }
         break;

      case 2:
         /* ROM version 2.x.x */
         if (pDev->fw_vers.CH == 0)
         {
            /* DXS 2-channel device */
            if (header.nDxs2V12FwLength > 0)
            {
               ret = DXS_DwldFirmware (pDev, pFwDwld->pPRAMfw + header_length +
                        header.nDxs2V12FwOffset,
                        header.nDxs2V12FwLength - (6 * 4));
               /* 6*4 are size of a delimiter and appended footer */
            }
         }
         else
         {
            /* DXS 1-channel device */
            if (header.nDxs1V12FwLength > 0)
            {
               ret = DXS_DwldFirmware (pDev, pFwDwld->pPRAMfw + header_length +
                        header.nDxs1V12FwOffset,
                        header.nDxs1V12FwLength - (6 * 4));
               /* 6*4 are size of a delimiter and appended footer */
            }
         }
         break;

      default:
         /* unknown ROM version - unsupported */
         /* errmsg: Firmware download failed. */
         ret = DXS_statusFwDwldFail;
         break;
      }
   }
   else
   {
      /* errmsg: Firmware download failed. */
      ret = DXS_statusFwDwldFail;
   }

   return ret;
}


/**
   Download and start EDSP FW

   \param pDev    pointer to the device interface
   \param pEdsp   pointer to \ref DXS_FW_Download_t structure

   \return
   - DXS_statusOk             if successful
   - DXS_statusSpiAccErr
   - DXS_statusReadErr
   - DXS_statusFwDwldFail     firmware download failed.
   - DXS_statusRegInitErr

   \remarks
*/
IFX_int32_t DXS_DwldAndStartFW (DXS_DEVICE_t *pDev, DXS_FW_Download_t *pEdsp)
{
   IFX_int32_t ret = DXS_statusErr;

   DXS_IO_Init_t IoInit = {0};
   IoInit.pPRAMfw = pEdsp->pPRAMfw;
   IoInit.pram_size = pEdsp->pram_size;
   IoInit.nFlags = pEdsp->nEdspFlags;

   ret = DXS_FW_Start(pDev, &IoInit);
   RETURN_DEVSTATUS(ret, IFX_NULL);
}

#ifdef TAPI_FEAT_LX_COMPAT
/**
   Linux compat function for DXS_DwldAndStartFW
 */
IFX_int32_t DXS_DwldAndStartFW_32 (DXS_DEVICE_t *pDev, DXS_FW_Download_32_t *pEdsp)
{
   DXS_FW_Download_t edsp_64 = {0};
   edsp_64.dev = pEdsp->dev;
   edsp_64.nEdspFlags = pEdsp->nEdspFlags;
   edsp_64.pPRAMfw = compat_ptr(pEdsp->pPRAMfw);
   edsp_64.pram_size = pEdsp->pram_size;

   return DXS_DwldAndStartFW(pDev, &edsp_64);
}
#endif /* TAPI_FEAT_LX_COMPAT */

/* @} */
