#ifndef _DRV_TAPI_DEBUG_H
#define _DRV_TAPI_DEBUG_H
/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_tapi_debug.h
   This header provide interface to the debug functionality.
*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include <ifx_types.h>
#include <drv_tapi_config.h>

#ifdef EVENT_LOGGER_DEBUG
   #include <el_log_macros.h>
   #if defined(TAPI_LINUX_USER_SPACE)
      #include "el_ioctl.h"
   #endif
#endif /* EVENT_LOGGER_DEBUG */

/* ========================================================================== */
/*                       Global Macro Definitions                             */
/* ========================================================================== */

#ifdef EVENT_LOGGER_DEBUG
   #define TAPI_EL_IOCTL_MAX_LEN 150

   #if defined(TAPI_LINUX_USER_SPACE)
      #ifndef min
         #define min(a,b)            (((a) < (b)) ? (a) : (b))
      #endif

      /* write ioctl logging macro */
      #define LOG_WR_IOCTL(dev_num, ch, ioctl, pdata, count, err)\
         EL_USR_LOG_EVENT_IOCTL_WR(tapi_nEl_fd, TAPI_EL_USR_format, \
                                 IFX_TAPI_DEV_TYPE_NONE, dev_num, ch, ioctl, \
                                 pdata, count, err)

      /* read ioctl logging macro */
      #define LOG_RD_IOCTL(dev_num, ch, ioctl, pdata, count, err)\
         EL_USR_LOG_EVENT_IOCTL_RD(tapi_nEl_fd, TAPI_EL_USR_format, \
                                 IFX_TAPI_DEV_TYPE_NONE, dev_num, ch, ioctl, \
                                 pdata, count, err)

   #else /* for Linux kernel space */
      /* write ioctl logging macro */
      #define LOG_WR_IOCTL(dev_num, ch, ioctl, pdata, count, err)\
         EL_LOG_EVENT_IOCTL_WR(IFX_TAPI_DEV_TYPE_NONE, dev_num, ch, ioctl, pdata,\
            count, err)

      /* read ioctl logging macro */
      #define LOG_RD_IOCTL(dev_num, ch, ioctl, pdata, count, err)\
         EL_LOG_EVENT_IOCTL_RD(IFX_TAPI_DEV_TYPE_NONE, dev_num, ch, ioctl, pdata,\
            count, err)
   #endif /*!defined(__KERNEL__) && defined(LINUX)*/

#else /* EVENT_LOGGER_DEBUG */
   #define LOG_WR_IOCTL(dev_num, ch, ioctl, pdata, count, err)
   #define LOG_RD_IOCTL(dev_num, ch, ioctl, pdata, count, err)
#endif /* EVENT_LOGGER_DEBUG */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */
#ifdef EVENT_LOGGER_DEBUG
   #if defined(TAPI_LINUX_USER_SPACE)
      extern IFX_int32_t tapi_nEl_fd;
   #endif
#endif

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* Ioctls values to names resolution */
struct IoctlLookupTable
{
   IFX_uint32_t val;
   IFX_char_t name[40];
};

/* IOCTL number to IOCTL name lookup array entry definition helper */
#define IOCTL_LKUP_TBL_ADD(ioctl) \
   { ioctl, #ioctl }

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

extern IFX_int32_t TAPI_DebugReportSet (TAPI_DEV *pDev, IFX_uint32_t nLevel);
extern IFX_void_t TAPI_LogInit (TAPI_DEV *pDev);
extern IFX_void_t TAPI_LogClose (TAPI_DEV *pDev);
extern IFX_char_t const *TAPI_ioctlNameGet(IFX_uint32_t nIoctl);

#ifdef EVENT_LOGGER_DEBUG
   #if defined(TAPI_LINUX_USER_SPACE)
      IFX_int32_t TAPI_EL_USR_format(EL_IoctlAddLog_t *pLog, IFX_char_t **ppOutput);
   #endif
#endif

#endif /* _DRV_TAPI_DEBUG_H */
