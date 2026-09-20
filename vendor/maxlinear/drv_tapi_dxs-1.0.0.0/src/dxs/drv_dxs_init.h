#ifndef _DRV_DXS_INIT_H
#define _DRV_DXS_INIT_H

/******************************************************************************

  Copyright (c) 2014 Lantiq Deutschland GmbH
  Copyright 2020 Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_init.h
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#ifdef TAPI_FEAT_LX_COMPAT
#include "drv_dxs_io_types_32.h"
#endif

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* Maximum expected BBD firmware file size */
#define DXS_MAX_BBD_FIRMWARE_SIZE           5000
/* Maximum expected file size of Duslic firmware */
#define DXS_MAX_FIRMWARE_SIZE              30000

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */
typedef enum
{
   /* boot directly from ROM */
   DXS_BOOT_ROM = 0,
   DXS_BOOT_UART,
   /* boot from i/o address SPI command inbox (FW patch download) */
   DXS_BOOT_SPI
} DXS_BOOT_INFO_t;

/* ========================================================================== */
/*                       Global variable declaration                          */
/* ========================================================================== */

extern enum DXS_DcDcType nAllowedDcDcType;

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

extern IFX_int32_t   DXS_GetDevice (IFX_uint16_t nr, DXS_DEVICE_t** pDev);

extern DXS_CHANNEL_t *DXS_GetNeighbourChannel (DXS_CHANNEL_t *pCh);

extern IFX_int32_t   DXS_DeviceDriverStart(void);

extern IFX_void_t    DXS_DeviceDriverStop (void);

extern IFX_int32_t   DXS_SetBootConfig (DXS_DEVICE_t *pDev,
                                        DXS_BOOT_INFO_t bootInfo);

extern IFX_int32_t   DXS_ResetController (DXS_DEVICE_t *pDev,
                                          DXS_BOOT_INFO_t bootInfo);

extern IFX_int32_t DXS_Wait4BootFinished (
                        DXS_DEVICE_t *pDev,
                        IFX_uint16_t nLoop);

extern IFX_int32_t   DXS_ReadFwVersion (
                              DXS_DEVICE_t *pDev);

extern IFX_int32_t   DXS_ChipAccessInit (DXS_DEVICE_t *pDev,
                              const DXS_BasicDeviceInit_t *pBasicDeviceInit);

extern IFX_int32_t   DXS_FW_Start (DXS_DEVICE_t *pDev, const DXS_IO_Init_t *pInit);

extern IFX_void_t    DXS_GPIO_Reset(DXS_DEVICE_t *pDev, int reset);

#ifdef TAPI_FEAT_LX_COMPAT
extern IFX_int32_t   DXS_FW_Start_32 (DXS_DEVICE_t *pDev,
                              DXS_IO_Init_32_t *pInit);
#endif

#endif /* _DRV_DXS_INIT_H */
