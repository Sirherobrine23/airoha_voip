#ifndef _DRV_DXS_DEBUG_H
#define _DRV_DXS_DEBUG_H
/******************************************************************************

  Copyright 2014 Lantiq Deutschland GmbH
  Copyright 2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_debug.h
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"

#ifdef EVENT_LOGGER_DEBUG
   #include <drv_tapi_osmap_local.h>
   #include "el_log_macros.h"
#endif /* EVENT_LOGGER_DEBUG */

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

#ifdef EVENT_LOGGER_DEBUG

enum { DEV_TYPE_DUSLIC_XS = IFX_TAPI_DEV_TYPE_DUSLIC_XS };

/* Event Logger (debugging) macros, defining more
   meaningful names to Event Logger macros */

/* register read logging macro, count is in bytes,
   LOG_EVENT_T3 counts in 16bit words */
#define LOG_RD_REG(dev_num, ch, reg_offset, reg_data, count)\
   EL_LOG_EVENT_REG_RD(DEV_TYPE_DUSLIC_XS, dev_num, ch, reg_offset,\
                       reg_data, count>>1)

/* register write logging macro, count is in bytes,
   LOG_EVENT_T4 counts in 16bit words */
#define LOG_WR_REG(dev_num, ch, reg_offset, reg_data, count)\
   EL_LOG_EVENT_REG_WR(DEV_TYPE_DUSLIC_XS, dev_num, ch, reg_offset,\
                       reg_data, count>>1)

#if TAPI_BYTE_ORDER == TAPI_BIG_ENDIAN
/* command read logging macro */
#define LOG_RD_CMD(dev_num, ch, pcmd, pdata, count, err)\
   EL_LOG_EVENT_CMD_RD(DEV_TYPE_DUSLIC_XS, dev_num, ch, pcmd, pdata,\
                       (count)>>1, err)

/* command write logging macro */
#define LOG_WR_CMD(dev_num, ch, pdata, count, err)\
   EL_LOG_EVENT_CMD_WR(DEV_TYPE_DUSLIC_XS, dev_num, ch, pdata, (count)>>1, err)

/* event inbox read logging macro */
#define LOG_RD_EVT(dev_num, ch, pdata, count, err)\
   EL_LOG_EVENT_EVT_MBX_RD(DEV_TYPE_DUSLIC_XS, dev_num, ch, pdata, (count)>>1)

#else

/* Backward comatibility for old event logger driver,
 * switch to usual interface */
#ifndef EL_LOG_EVENT_CMD_RD_REV
#define EL_LOG_EVENT_CMD_RD_REV EL_LOG_EVENT_CMD_RD
#endif
#ifndef EL_LOG_EVENT_CMD_WR_REV
#define EL_LOG_EVENT_CMD_WR_REV EL_LOG_EVENT_CMD_WR
#endif
#ifndef EL_LOG_EVENT_EVT_MBX_RD_REV
#define EL_LOG_EVENT_EVT_MBX_RD_REV EL_LOG_EVENT_EVT_MBX_RD
#endif

/* command read logging macro with reversed 16bit words order */
#define LOG_RD_CMD(dev_num, ch, pcmd, pdata, count, err)\
   EL_LOG_EVENT_CMD_RD_REV(DEV_TYPE_DUSLIC_XS, dev_num, ch, pcmd, pdata,\
                       (count)>>1, err)

/* command write logging macro with reversed 16bit words order */
#define LOG_WR_CMD(dev_num, ch, pdata, count, err)\
   EL_LOG_EVENT_CMD_WR_REV(DEV_TYPE_DUSLIC_XS, dev_num, ch, pdata, (count)>>1, err)

/* event inbox read logging macro with reversed 16bit words order */
#define LOG_RD_EVT(dev_num, ch, pdata, count, err)\
   EL_LOG_EVENT_EVT_MBX_RD_REV(DEV_TYPE_DUSLIC_XS, dev_num, ch, pdata, (count)>>1)

#endif


/* interrupt and events logging macro */
#define LOG_IRQ_EVT(dev_type, dev_num, ch, irq_name, irq_details)\
   EL_LOG_EVENT_IRQ(dev_type, dev_num, ch, irq_name, irq_details)

#else /* EVENT_LOGGER_DEBUG */

#define LOG_RD_REG(dev_num, ch, reg_offset, reg_data, count)
#define LOG_WR_REG(dev_num, ch, reg_offset, reg_data, count)
#define LOG_WR_REG_MULTI(dev_num, ch, reg_offset, reg_data, count)
#define LOG_RD_CMD(dev_num, ch, pcmd, pdata, count, err)
#define LOG_WR_CMD(dev_num, ch, pdata, count, err)
#define LOG_RD_EVT(dev_num, ch, pdata, count, err)
#define LOG_IRQ_EVT(dev_type, dev_num, ch, irq_name, irq_details)

#endif /* EVENT_LOGGER_DEBUG */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
extern IFX_int32_t DXS_Report_Set   (IFX_uint32_t driver_level);

#endif /* _DRV_DXS_DEBUG_H */
