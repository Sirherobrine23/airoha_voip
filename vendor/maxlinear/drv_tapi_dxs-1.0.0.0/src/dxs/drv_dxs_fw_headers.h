#ifndef _DRV_DXS_FW_HEADERS_H_
#define _DRV_DXS_FW_HEADERS_H_
/******************************************************************************

                            Copyright (c) 2014, 2016
                        Lantiq Beteiligungs-GmbH & Co.KG
                             http://www.lantiq.com

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_fw_headers.h
   This file contains the common header of FW messages.
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include <drv_tapi_osmap.h>

/* ============================= */
/* Global Defines                */
/* ============================= */

/** Command header definition (big endian) */
#define CMD_HEAD_BE         \
   /* Read/Write */         \
   uint32_t RW : 1;         \
   /* Reserved */           \
   uint32_t ResHead0 : 2;   \
   /* Command */            \
   uint32_t CMD : 5;        \
   /* Reserved */           \
   uint32_t ResHead1 : 4;   \
   /* Channel */            \
   uint32_t CHAN : 4;       \
   /* Module */             \
   uint32_t MOD : 3;        \
   /* Command Sub-Mode */   \
   uint32_t ECMD : 5;       \
   /* Length of Message */  \
   uint32_t LENGTH : 8

/** Command header definition (little endian) */
#define CMD_HEAD_LE         \
   /* Length of Message */  \
   uint32_t LENGTH : 8;     \
   /* Command Sub-Mode */   \
   uint32_t ECMD : 5;       \
   /* Module */             \
   uint32_t MOD : 3;        \
   /* Channel */            \
   uint32_t CHAN : 4;       \
   /* Reserved */           \
   uint32_t ResHead1 : 4;   \
   /* Command */            \
   uint32_t CMD : 5;        \
   /* Reserved */           \
   uint32_t ResHead0 : 2;   \
   /* Read/Write */         \
   uint32_t RW : 1

/* Command type values */
#define DXS_CMD_CMD_SDD       1
#define DXS_CMD_CMD_EOP       6

/* Command mode values for SDD commands */
#define DXS_CMD_MOD_SDD       0

/* Command mode values for EOP commands */
#define DXS_CMD_MOD_GPIO      0
#define DXS_CMD_MOD_PCM       0
#define DXS_CMD_MOD_SIG_GEN   3
#define DXS_CMD_MOD_SIG_DET   6
#define DXS_CMD_MOD_SYS       7

/* Command channel values */
#define DXS_CMD_CHAN_A        0
#define DXS_CMD_CHAN_B        1

/* ============================= */
/* Global Types                  */
/* ============================= */

/** DuSLIC-xS firmware command header type. */
struct DXS_FW_Cmd_Header
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
#else
   CMD_HEAD_LE;
#endif
} __PACKED__;

#endif /* _DRV_DXS_FW_HEADERS_H_ */
