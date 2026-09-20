#ifndef _DRV_DXS_FW_CMD_EOP_H_
#define _DRV_DXS_FW_CMD_EOP_H_
/******************************************************************************

  Copyright (c) 2014-2015 Lantiq Deutschland GmbH
  Copyright (c) 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
  Copyright 2016, Intel Corporation.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_fw_cmd_eop.h
   This file contains the EOP command messages.
*/

#include "drv_dxs_fw_headers.h"
#include <drv_tapi_osmap.h>

/** Data structure for DXS_FW_SYS_VERS_ECMD firmware command */
typedef struct
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /** Device Number */
   uint32_t DEVNR : 11;
   /** Calender Week */
   uint32_t CW : 6;
   /** Production Year */
   uint32_t YEAR : 6;
   /** Channel */
   uint32_t CH : 1;
   /** Device ID Representing PEB Number */
   uint32_t DEV : 8;
   /** Major consisting of TFW:1 MAJ:7 */
   uint32_t MAJ : 8;
   /** Minor */
   uint32_t MIN : 8;
   /** Hotfix */
   uint32_t HF : 8 ;
   /** Patch */
   uint32_t PATCH : 8;
   /** Time Stamp */
   uint32_t TIME;
#else
   CMD_HEAD_LE;
   /** Device ID Representing PEB Number */
   uint32_t DEV : 8;
   /** Channel */
   uint32_t CH : 1;
   /** Production Year */
   uint32_t YEAR : 6;
   /** Calender Week */
   uint32_t CW : 6;
   /** Device Number */
   uint32_t DEVNR : 11;
   /** Patch */
   uint32_t PATCH : 8;
   /** Hotfix */
   uint32_t HF : 8 ;
   /** Minor */
   uint32_t MIN : 8;
   /** Major consisting of TFW:1 MAJ:7 */
   uint32_t MAJ : 8;
   /** Time Stamp */
   uint32_t TIME;
#endif
} __PACKED__ DXS_FW_SYS_VERS_t;
#define DXS_FW_SYS_VERS_ECMD    6
#define DXS_FW_SYS_VERS_LENGTH  12

/** Data structure for system capabilities firmware command */
typedef struct
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /* Reserved */
   uint32_t Res02 : 31;
   /* GR909 available */
   uint32_t GR909 : 1;
#else
   CMD_HEAD_LE;
   /* GR909 available */
   uint32_t GR909 : 1;
   /* Reserved */
   uint32_t Res02 : 31;
#endif
} __PACKED__ DXS_FW_SYS_CAPS_t;
#define DXS_FW_SYS_CAPS_ECMD    7
#define DXS_FW_SYS_CAPS_LENGTH  4

/** Data structure for DXS_FW_SYS_Control_ECMD firmware command */
typedef struct DXS_FW_SYS_Control
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /* Reserved */
   uint32_t Res02 : 25;
   /* Sync-Fail/ Clock-Fail Handling Enable */
   uint32_t SYCLKE : 1;
   /* PLL Limiter Enable */
   uint32_t PLIM : 1;
   /* Charge Pump Voltage Level */
   uint32_t CP_VOLT : 3;
   /* Charge Pump Enable */
   uint32_t CP_EN : 1;
   /* Sleep Enable */
   uint32_t SE : 1;
#else
   CMD_HEAD_LE;
   /* Sleep Enable */
   uint32_t SE : 1;
   /* Charge Pump Enable */
   uint32_t CP_EN : 1;
   /* Charge Pump Voltage Level */
   uint32_t CP_VOLT : 3;
   /* PLL Limiter Enable */
   uint32_t PLIM : 1;
   /* Sync-Fail/ Clock-Fail Handling Enable */
   uint32_t SYCLKE : 1;
   /* Reserved */
   uint32_t Res02 : 25;
#endif
} __PACKED__ DXS_FW_SYS_Control_t;
#define DXS_FW_SYS_Control_ECMD     8
#define DXS_FW_SYS_Control_LENGTH   4
#define DXS_FW_SYS_Control_CP_VOLT_3_95V  0
#define DXS_FW_SYS_Control_CP_VOLT_5_0V   3

/** This command activates the DTMF/AT Generator in the addressed channel. */
typedef struct DXS_DTMF_AT_GEN_CTRL
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
    /** Status of DTMF/AT Generator */
    IFX_uint32_t EN : 1;
    /** Reserved */
    IFX_uint32_t Res00 : 14;
    /** Amplitude Modulation */
    IFX_uint32_t AM : 1;
    /** Reserved */
    IFX_uint32_t Res01 : 16;
#else
   CMD_HEAD_LE;
   /** Reserved */
    IFX_uint32_t Res01 : 16;
    /** Amplitude Modulation */
    IFX_uint32_t AM : 1;
   /** Reserved */
    IFX_uint32_t Res00 : 14;
   /** Status of DTMF/AT Generator */
    IFX_uint32_t EN : 1;
#endif
} __PACKED__ DXS_DTMF_AT_GEN_CTRL_t;

#define DTMF_AT_GEN_CTRL_ECMD_EOP_DTMFATGEN 0x03
#define DTMF_AT_GEN_CTRL_LENGTH 0x04
#define DTMF_AT_GEN_CTRL_EN_DIS 0
#define DTMF_AT_GEN_CTRL_EN_EN 1
#define DTMF_AT_GEN_CTRL_AM_OFF 0
#define DTMF_AT_GEN_CTRL_AM_ON 1

/** This command determines the coefficients for the DTMF/AT Generator. */
typedef struct DXS_DTMF_AT_GEN_COEF
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
    /** Level for Frequency 1 */
    IFX_uint32_t LEVEL1 : 16;
    /** Level for Frequency 2 */
    IFX_uint32_t LEVEL2 : 16;
    /** Frequency 1 of Tone */
    IFX_uint32_t FREQ1 : 16;
    /** Frequency 2 of Tone */
    IFX_uint32_t FREQ2 : 16;
#else
   CMD_HEAD_LE;
   /** Level for Frequency 2 */
    IFX_uint32_t LEVEL2 : 16;
   /** Level for Frequency 1 */
    IFX_uint32_t LEVEL1 : 16;
    /** Frequency 2 of Tone */
    IFX_uint32_t FREQ2 : 16;
    /** Frequency 1 of Tone */
    IFX_uint32_t FREQ1 : 16;
#endif
} __PACKED__ DXS_DTMF_AT_GEN_COEF_t;

#define DTMF_AT_GEN_COEF_ECMD_EOP_DTMFATCOEFF 0x0A
#define DTMF_AT_GEN_COEF_LENGTH 0x08


/** This command determines the coefficients for the CID Sender. */
typedef struct DXS_CID_GEN_COEF
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
    /** CID Transmit Level */
    IFX_uint32_t LEVEL : 16;
    /** Number of Seizure Bits */
    IFX_uint32_t SEIZURE : 16;
    /** Number of Mark Bits */
    IFX_uint32_t MARK : 16;
    /** Reserved */
    IFX_uint32_t Res00 : 16;
#else
   CMD_HEAD_LE;
   /** Number of Seizure Bits */
   IFX_uint32_t SEIZURE : 16;
   /** CID Transmit Level */
   IFX_uint32_t LEVEL : 16;
   /** Reserved */
   IFX_uint32_t Res00 : 16;
   /** Number of Mark Bits */
   IFX_uint32_t MARK : 16;
#endif
} __PACKED__ DXS_CID_GEN_COEF_t;

#define CID_GEN_COEF_ECMD_EOP_CIDS_COEFF 0x08
#define CID_GEN_COEF_LENGTH 0x8
#define CID_GEN_COEF_LEVEL_RESET 0x7FFF
#define CID_GEN_COEF_SEIZURE_RESET 0x0
#define CID_GEN_COEF_MARK_RESET 0x0

/** This command activates the CID Sender in the addressed channel. */
typedef struct DXS_CID_GEN_CTRL
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
    /** Enable */
    IFX_uint32_t EN : 1;
    /** Reserved */
    IFX_uint32_t Res00 : 1;
    /** Auto Deactivation */
    IFX_uint32_t AD : 1;
    /** High Level CID Generation Mode */
    IFX_uint32_t HLEV : 1;
    /** CID Specification */
    IFX_uint32_t V23 : 1;
    /** Reserved */
    IFX_uint32_t Res01 : 27;
#else
   CMD_HEAD_LE;
   /** Reserved */
    IFX_uint32_t Res01 : 27;
   /** CID Specification */
    IFX_uint32_t V23 : 1;
   /** High Level CID Generation Mode */
    IFX_uint32_t HLEV : 1;
   /** Auto Deactivation */
    IFX_uint32_t AD : 1;
   /** Reserved */
    IFX_uint32_t Res00 : 1;
   /** Enable */
    IFX_uint32_t EN : 1;
#endif
} __PACKED__ DXS_CID_GEN_CTRL_t;

#define CID_GEN_CTRL_ECMD_EOP_CIDSEND 0x02
#define CID_GEN_CTRL_LENGTH 0x04
#define CID_GEN_CTRL_EN_DIS 0
#define CID_GEN_CTRL_EN_EN 1
#define CID_GEN_CTRL_AD_OFF 0
#define CID_GEN_CTRL_AD_ON 1
#define CID_GEN_CTRL_HLEV_HLEV_HIGH 1
#define CID_GEN_CTRL_V23_V23_BEL202 0
#define CID_GEN_CTRL_V23_V23_ITU_T 1

/** Maximum number of CID data bytes to be sent via one message */
#define DXS_CID_GEN_DATA_MAX   27

/** This command sends new data to the CID Sender. Each data word contains one
data byte. It is only possible to send this command when DUSLIC XS requests
a new data byte, with the status bit CIS_REQ. Otherwise the command is
discarded. With the acceptance of the command the status bits CIS_BUF and
CIS_REQ are reset. */
typedef struct DXS_CID_GEN_DATA
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
    /** Number of Data Bytes following */
    IFX_uint32_t NRDATA : 8;
    /** Data Byte 0 */
    IFX_uint32_t DATA0 : 8;
    /** Data Byte 1 */
    IFX_uint32_t DATA1 : 8;
    /** Data Byte 2 */
    IFX_uint32_t DATA2 : 8;
#else
   CMD_HEAD_LE;
   /** Data Byte 2 */
   IFX_uint32_t DATA2 : 8;
   /** Data Byte 1 */
   IFX_uint32_t DATA1 : 8;
   /** Data Byte 0 */
   IFX_uint32_t DATA0 : 8;
   /** Number of Data Bytes following */
   IFX_uint32_t NRDATA : 8;
#endif
    /** Data Bytes 3-26 */
    IFX_uint32_t DATA[DXS_CID_GEN_DATA_MAX >> 2];
} __PACKED__ DXS_CID_GEN_DATA_t;

#define CID_GEN_DATA_ECMD_CIDS_DATA 0x09
#define CID_GEN_DATA_LENGTH_MAXLEN  28

/** This command activates the DTMF Receiver in the addressed channel. */
typedef struct DXS_DTMF_REC_CTRL
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
    /** Enable */
    IFX_uint32_t EN : 1;
    /** Reserved */
    IFX_uint32_t RES : 31;
#else
   CMD_HEAD_LE;
   /** Reserved */
    IFX_uint32_t RES : 31;
   /** Enable */
    IFX_uint32_t EN : 1;
#endif
} __PACKED__ DXS_DTMF_REC_CTRL_t;
#define DTMF_REC_CTRL_ECMD_EOP_DTMFREC 0x04
#define DTMF_REC_CTRL_LENGTH  0x04
#define DTMF_REC_CTRL_EN_EN 0x01
#define DTMF_REC_CTRL_EN_DIS 0x0

/** This command determines the coefficients for the DTMF Receiver. */
typedef struct DTMF_REC_COEF
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
    /** Minimum Signal Level */
    IFX_uint32_t LEVEL : 16;
    /** Maximal Allowed Signal Twist */
    IFX_uint32_t TWIST : 16;
#else
   CMD_HEAD_LE;
   /** Maximal Allowed Signal Twist */
    IFX_uint32_t TWIST : 16;
   /** Minimum Signal Level */
    IFX_uint32_t LEVEL : 16;
#endif
} __PACKED__ DXS_DTMF_REC_COEF_t;

#define DTMF_REC_COEF_ECMD_EOP_DTMF_REC_COEFF 0x0C
#define DTMF_REC_COEF_LENGTH 0x04


/** This command starts (and stops) the metering pulse generator for one single
 * metering pulse. */
typedef struct DXS_TTX_GEN_CTRL
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
    /** Enable */
    IFX_uint32_t EN : 1;
    /** Reserved */
    IFX_uint32_t RES : 31;
#else
   CMD_HEAD_LE;
   /** Reserved */
    IFX_uint32_t RES : 31;
   /** Enable */
    IFX_uint32_t EN : 1;
#endif
} __PACKED__ DXS_TTX_GEN_CTRL_t;
#define TTX_GEN_CTRL_ECMD_EOP_TTX 0x04
#define TTX_GEN_CTRL_LENGTH  0x04
#define TTX_GEN_CTRL_EN_EN 0x01
#define TTX_GEN_CTRL_EN_DIS 0x0

/** This command activates the PCM-Interface-Module. This command has to be sent
before a PCM channel can be configured. */
typedef struct DXS_PCM_IF_CTRL
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
    /** Enable */
    IFX_uint32_t EN : 1;
    /** Data Streaming Enable */
    IFX_uint32_t DSEN : 1;
    /** Reserved */
    IFX_uint32_t Res00 : 3;
    /** Transmit Bit Offset */
    IFX_uint32_t XOFF : 3;
    /** Double Bit Clock */
    IFX_uint32_t DBL : 1;
    /** Transmit Slope */
    IFX_uint32_t XS : 1;
    /** Receive Slope */
    IFX_uint32_t RS : 1;
    /** Bit 0 drive length */
    IFX_uint32_t DRV0 : 1;
    /** Shift */
    IFX_uint32_t SHIFT : 1;
    /** Receive Bit Offset */
    IFX_uint32_t ROFF : 3;
    /** Reserved */
    IFX_uint32_t Res01 : 16;
#else
   CMD_HEAD_LE;
   /** Reserved */
   IFX_uint32_t Res01 : 16;
   /** Receive Bit Offset */
    IFX_uint32_t ROFF : 3;
   /** Shift */
    IFX_uint32_t SHIFT : 1;
   /** Bit 0 drive length */
    IFX_uint32_t DRV0 : 1;
   /** Receive Slope */
    IFX_uint32_t RS : 1;
   /** Transmit Slope */
    IFX_uint32_t XS : 1;
   /** Double Bit Clock */
    IFX_uint32_t DBL : 1;
   /** Transmit Bit Offset */
    IFX_uint32_t XOFF : 3;
   /** Reserved */
    IFX_uint32_t Res00 : 3;
    /** Data Streaming Enable */
    IFX_uint32_t DSEN : 1;
   /** Enable */
    IFX_uint32_t EN : 1;
#endif
} __PACKED__ DXS_PCM_IF_CTRL_t;

#define PCM_IF_CTRL_ECMD_PCM_INTERFACE_CONTROL 0x00
#define PCM_IF_CTRL_LENGTH 0x04
#define PCM_IF_CTRL_ENABLE 0x1
#define PCM_IF_CTRL_DISABLE 0x0
#define PCM_IF_CTRL_DS_DOUBLE_BUFFER 0x1
#define PCM_IF_CTRL_DS_FIFO 0x0
#define PCM_IF_CTRL_DBL_SINGLE_CLOCKING_USED 0
#define PCM_IF_CTRL_DBL_DOUBLE_CLOCKING_USED 1
#define PCM_IF_CTRL_XS_RISING_EDGE 0
#define PCM_IF_CTRL_XS_FALLING_EDGE 1
#define PCM_IF_CTRL_RS_RISING_EDGE 1
#define PCM_IF_CTRL_RS_FALLING_EDGE 0
#define PCM_IF_CTRL_DRV0_BIT_0_DRIVEN_THE_ENTIRE_CLOCK_PERIOD 0
#define PCM_IF_CTRL_DRV0_BIT_0_DRIVEN_THE_FIRST_HALF_OF_THE_CLOCK_PERIOD 1
#define PCM_IF_CTRL_SH_NO_SHIFT 0
#define PCM_IF_CTRL_SH_SHIFT 1

/** This command configures the PCM channels. */
typedef struct DXS_PCM_CH_CTRL
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
    /** Enable */
    IFX_uint32_t EN : 1;
    /** Coder */
    IFX_uint32_t COD : 3;
    /** Wideband 16kHz */
    IFX_uint32_t WIDE : 1;
    /** Wideband PCM Time Slot Configuration Bit */
    IFX_uint32_t WBTSC : 1;
    /** Reserved 0 */
    IFX_uint32_t Res0 : 11;
    /** Transmit Highway Time Slot */
    IFX_uint32_t XTS : 7;
    /** Reserved 1 */
    IFX_uint32_t Res1 : 1;
    /** Transmit Highway Time Slot */
    IFX_uint32_t RTS : 7;
#else
   CMD_HEAD_LE;
   /** Transmit Highway Time Slot */
   IFX_uint32_t RTS : 7;
   /** Reserved 1 */
   IFX_uint32_t Res1 : 1;
   /** Transmit Highway Time Slot */
   IFX_uint32_t XTS : 7;
   /** Reserved 0 */
   IFX_uint32_t Res0 : 11;
   /** Wideband PCM Time Slot Configuration Bit */
   IFX_uint32_t WBTSC : 1;
   /** Wideband 16kHz */
   IFX_uint32_t WIDE : 1;
   /** Coder */
   IFX_uint32_t COD : 3;
   /** Enable */
   IFX_uint32_t EN : 1;
#endif
} __PACKED__ DXS_PCM_CH_CTRL_t;

#define PCM_CH_CTRL_ECMD_PCM_CHAN 0x01
#define PCM_CH_CTRL_LENGTH 0x04
#define PCM_CH_CTRL_ENABLE 0x1
#define PCM_CH_CTRL_DISABLE 0x0
#define PCM_CH_CTRL_COD_PCM_RES_LINEAR_16BIT 0x0
#define PCM_CH_CTRL_COD_PCM_RES_ALAW_8BIT 0x2
#define PCM_CH_CTRL_COD_PCM_RES_ULAW_8BIT 0x3
#define PCM_CH_CTRL_WIDE_8KHZ 0x0
#define PCM_CH_CTRL_WIDE_16KHZ 0x1
#define PCM_CH_CTRL_WBTSC_CONSECUTIVE 0x0
#define PCM_CH_CTRL_WBTSC_SPLIT 0x1


/** This command mutes the channel in RX direction. */
typedef struct DXS_PCM_CH_MUTE
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
    /** Reserved 0 */
    IFX_uint32_t Res0 : 31;
    /** Mute of RX Direction */
    IFX_uint32_t RX_MUTE : 1;
#else
   CMD_HEAD_LE;
   /** Mute of RX Direction */
   IFX_uint32_t RX_MUTE : 1;
   /** Reserved 0 */
   IFX_uint32_t Res0 : 31;
#endif
} __PACKED__ DXS_PCM_CH_MUTE_t;
#define PCM_CH_MUTE_ECMD_PCM_MUTE 0x04
#define PCM_CH_MUTE_LENGTH 0x04
#define PCM_CH_MUTE_RX_MUTE_NO_MUTE 0
#define PCM_CH_MUTE_RX_MUTE_MUTE 1


/** This command configures the GPIO directions and enables the module. If the
module is not enabled, the GPIOs are defined with >input< behaviour. */
typedef struct DXS_GPIO_CTRL
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
    /** Reserved */
    IFX_uint32_t Res00 : 4;
    /** Enable GPIO Pull-up Resistor */
    IFX_uint32_t GPIO3_PULL_UP : 1;
    /** Enable GPIO Pull-up Resistor */
    IFX_uint32_t GPIO2_PULL_UP : 1;
    /** Enable GPIO Pull-up Resistor */
    IFX_uint32_t GPIO1_PULL_UP : 1;
    /** Enable GPIO Pull-up Resistor */
    IFX_uint32_t GPIO0_PULL_UP : 1;
    /** Reserved */
    IFX_uint32_t Res01 : 4;
    /** Direction of GPIO */
    IFX_uint32_t GPIO3_DIR : 1;
    /** Direction of GPIO */
    IFX_uint32_t GPIO2_DIR : 1;
    /** Direction of GPIO */
    IFX_uint32_t GPIO1_DIR : 1;
    /** Direction of GPIO */
    IFX_uint32_t GPIO0_DIR : 1;
    /** Reserved */
    IFX_uint32_t Res02 : 16;
#else
   CMD_HEAD_LE;
   /** Reserved */
   IFX_uint32_t Res02 : 16;
   /** Direction of GPIO */
   IFX_uint32_t GPIO0_DIR : 1;
   /** Direction of GPIO */
   IFX_uint32_t GPIO1_DIR : 1;
   /** Direction of GPIO */
   IFX_uint32_t GPIO2_DIR : 1;
   /** Direction of GPIO */
   IFX_uint32_t GPIO3_DIR : 1;
   /** Reserved */
   IFX_uint32_t Res01 : 4;
   /** Enable GPIO Pull-up Resistor */
   IFX_uint32_t GPIO0_PULL_UP : 1;
   /** Enable GPIO Pull-up Resistor */
   IFX_uint32_t GPIO1_PULL_UP : 1;
   /** Enable GPIO Pull-up Resistor */
   IFX_uint32_t GPIO2_PULL_UP : 1;
   /** Enable GPIO Pull-up Resistor */
   IFX_uint32_t GPIO3_PULL_UP : 1;
   /** Reserved */
   IFX_uint32_t Res00 : 4;
#endif
} __PACKED__ DXS_GPIO_CTRL_t;

#define GPIO_CTRL_ECMD_GPIO_CTRL 0x02
#define GPIO_CTRL_LENGTH_READ 0x04
#define GPIO_CTRL_DIR_INPUT 1
#define GPIO_CTRL_DIR_OUTPUT 0
#define GPIO_CTRL_PULL_UP_DISABLE 0
#define GPIO_CTRL_PULL_UP_ENABLE 1


/** This command reads data from a GPIO or writes Data to the GPIO. If
configured as output this value is written to the GPIO for a write access.
For a read access the actual output value is reflected.  */
typedef struct DXS_GPIO_RW_DATA
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /** Reserved */
   IFX_uint32_t Res00 : 4;
    /** Write Mask for GPIO */
    IFX_uint32_t GPIO3_WM : 1;
    /** Write Mask for GPIO */
    IFX_uint32_t GPIO2_WM : 1;
    /** Write Mask for GPIO */
    IFX_uint32_t GPIO1_WM : 1;
    /** Write Mask for GPIO */
    IFX_uint32_t GPIO0_WM : 1;
    /** Reserved */
    IFX_uint32_t Res01 : 4;
    /** Value of GPIO */
    IFX_uint32_t GPIO3_VAL : 1;
    /** Value of GPIO */
    IFX_uint32_t GPIO2_VAL : 1;
    /** Value of GPIO */
    IFX_uint32_t GPIO1_VAL : 1;
    /** Value of GPIO */
    IFX_uint32_t GPIO0_VAL : 1;
    /** Reserved */
    IFX_uint32_t Res02 : 16;
#else
   CMD_HEAD_LE;
   /** Reserved */
   IFX_uint32_t Res02 : 16;
   /** Value of GPIO */
    IFX_uint32_t GPIO0_VAL : 1;
   /** Value of GPIO */
    IFX_uint32_t GPIO1_VAL : 1;
   /** Value of GPIO */
    IFX_uint32_t GPIO2_VAL : 1;
   /** Value of GPIO */
    IFX_uint32_t GPIO3_VAL : 1;
    /** Reserved */
    IFX_uint32_t Res01 : 4;
   /** Write Mask for GPIO */
    IFX_uint32_t GPIO0_WM : 1;
   /** Write Mask for GPIO */
    IFX_uint32_t GPIO1_WM : 1;
   /** Write Mask for GPIO */
    IFX_uint32_t GPIO2_WM : 1;
   /** Write Mask for GPIO */
    IFX_uint32_t GPIO3_WM : 1;
    /** Reserved */
    IFX_uint32_t Res00 : 4;
#endif
} __PACKED__ DXS_GPIO_RW_DATA_t;

#define GPIO_RW_DATA_ECMD_GPIO_READ_WRITE 0x03
#define GPIO_RW_DATA_LENGTH 0x04
#define GPIO_RW_DATA_WM_PROGRAM 0
#define GPIO_RW_DATA_WM_NOT_PROGRAM 1
#define GPIO_RW_DATA_GPIO_VAL_LOW 0
#define GPIO_RW_DATA_GPIO_VAL_HIGH 1


/** UTD Control */
typedef struct DXS_UTD_CTRL
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /** Enable */
   IFX_uint32_t EN : 1;
   /** Reserved */
   IFX_uint32_t Res01 : 14;
   /** Direction */
   IFX_uint32_t DIR : 1;
   /** Reserved */
   IFX_uint32_t Res02 : 16;
#else
   CMD_HEAD_LE;
   /** Reserved */
   IFX_uint32_t Res02 : 16;
   /** Direction */
   IFX_uint32_t DIR : 1;
   /** Reserved */
   IFX_uint32_t Res01 : 14;
   /** Enable */
   IFX_uint32_t EN : 1;
#endif
} __PACKED__ DXS_UTD_CTRL_t;

#define UTD_CTRL_ECMD 0x11
#define UTD_CTRL_LENGTH 0x04
#define UTD_CTRL_EN_DIS 0x0
#define UTD_CTRL_EN_EN 0x1
/* TX: UTD is configured to detect tones coming from the analog line. */
#define UTD_CTRL_DIR_TX 0x0
/* RX: UTD is configured to detect tones coming from the PCM interface. */
#define UTD_CTRL_DIR_RX 0x1

/** UTD Coefficients */
typedef struct DXS_UTD_COEF
{
#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
   CMD_HEAD_BE;
   /** Bandwidth */
   IFX_uint32_t BW : 16;
   /** Center Frequency */
   IFX_uint32_t CF : 16;
   /** Noise Level */
   IFX_uint32_t NLEV : 16;
   /** Signal Level */
   IFX_uint32_t SLEV : 16;
   /** Delta Level */
   IFX_uint32_t DLEV : 16;
   /** Low Pass Coefficient */
   IFX_uint32_t LPCOEF : 16;
   /** Data Upstream Persistance Counter */
   IFX_uint32_t DUP : 8;
   /** Reserved */
   IFX_uint32_t Res01 : 24;
#else
   CMD_HEAD_LE;
   /** Center Frequency */
   IFX_uint32_t CF : 16;
   /** Bandwidth */
   IFX_uint32_t BW : 16;
   /** Signal Level */
   IFX_uint32_t SLEV : 16;
   /** Noise Level */
   IFX_uint32_t NLEV : 16;
   /** Low Pass Coefficient */
   IFX_uint32_t LPCOEF : 16;
   /** Delta Level */
   IFX_uint32_t DLEV : 16;
   /** Reserved */
   IFX_uint32_t Res01 : 24;
   /** Data Upstream Persistance Counter */
   IFX_uint32_t DUP : 8;
#endif
} __PACKED__ DXS_UTD_COEF_t;

#define UTD_COEF_ECMD 0x12
#define UTD_COEF_LEN 16

#endif /* _DRV_DXS_FW_CMD_EOP_H_ */
