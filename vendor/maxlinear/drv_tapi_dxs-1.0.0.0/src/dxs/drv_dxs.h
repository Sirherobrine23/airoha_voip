#ifndef _DRV_DXS_H_
#define _DRV_DXS_H_
/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016-2017 Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs.h
   This file contains all device specific definitions, e.g. offsets, registers,
   bits, etc.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */
#define DXS_HOST_BASE (0x0000)

/* DUSLIC XS has one PCM highway internally */
#define DXS_PCM_HIGHWAY       1

/*******************************************************************************
 * Register offsets
 ******************************************************************************/
#define DXS_HOST_CFG     ((IFX_vuint32_t)(DXS_HOST_BASE + 0x06))
#define DXS_HOST_IEN1    ((IFX_vuint32_t)(DXS_HOST_BASE + 0x08))
#define DXS_HOST_STAT1   ((IFX_vuint32_t)(DXS_HOST_BASE + 0x0C))
#define DXS_HOST_INT1    ((IFX_vuint32_t)(DXS_HOST_BASE + 0x10))
#define DXS_HOST_IEN2    ((IFX_vuint32_t)(DXS_HOST_BASE + 0x0A))
#define DXS_HOST_STAT2   ((IFX_vuint32_t)(DXS_HOST_BASE + 0x0E))
#define DXS_HOST_INT2    ((IFX_vuint32_t)(DXS_HOST_BASE + 0x12))
#define DXS_HOST_DATA    ((IFX_vuint32_t)(DXS_HOST_BASE + 0x14))
#define DXS_HOST_LEN     ((IFX_vuint32_t)(DXS_HOST_BASE + 0x18))
#define DXS_HOST_CMD     ((IFX_vuint32_t)(DXS_HOST_BASE + 0x1C))
/* Basic config */
#define DXS_HOST_BCFG    ((IFX_vuint32_t)(DXS_HOST_BASE + 0x20))
/* Boot info */
#define DXS_HOST_BINF    ((IFX_vuint32_t)(DXS_HOST_BASE + 0x24))
/* Register used only for SPI registers access test */
#define ACCESS_TEST      ((IFX_vuint32_t)(DXS_HOST_BASE + 0x48))

/*******************************************************************************
 * HOST Configuration Register
 ******************************************************************************/
#define DXS_REG_CFG_INT_MD_INTPOS   (0x1 << 0)
#define DXS_REG_CFG_SC_MD_SCON      (0x1 << 1)
#define DXS_REG_CFG_8BIT_EN         (0x1 << 2)
#define DXS_REG_CFG_RST_RSTCORE     (0x1 << 7)
#define DXS_REG_CFG_RST_MASK        0x0080

/*******************************************************************************
 * HOST Interrupt Enable 1 Register
 ******************************************************************************/
/* Reset value */
#define DXS_REG_IEN1_RESET          0x0000
/* Out-Box Ready Interrupt (11) */
#define DXS_REG_IEN1_OBX_RDY        (0x1 << 11)
/* In-Box Empty Interrupt Mask (9) */
#define DXS_REG_IEN1_IBX_EMP        (0x1 << 9)
/* Error Interrupt Mask (0) */
#define DXS_REG_IEN1_ERR            (0x1)

/*******************************************************************************
 * HOST Interrupt Enable 2 Register
 ******************************************************************************/
/* Reset value */
#define DXS_REG_IEN2_RESET          0x0000
/* Out-Box Underflow Interrupt (9) */
#define DXS_REG_IEN2_OBX_UFL        (0x1 << 9)
/* In-Box Overflow Interrupt (8) */
#define DXS_REG_IEN2_IBX_OFL        (0x1 << 8)

/*******************************************************************************
 * HOST Boot Config Register
 ******************************************************************************/
#define DXS_REG_BCFG_ASC_MASK       0x00FF
#define DXS_REG_BCFG_ASC_ROM        0x0
#define DXS_REG_BCFG_ASC_SPI        0x3

/*******************************************************************************
 * HOST BINF Register
 ******************************************************************************/
#define DXS_REG_BINF_BOOTSTATE_MASK 0x00FF

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

#endif /* _DRV_DXS_H_ */
