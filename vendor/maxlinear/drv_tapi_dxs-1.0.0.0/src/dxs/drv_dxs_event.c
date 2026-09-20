/******************************************************************************

  Copyright 2014 Lantiq Deutschland GmbH
  Copyright 2021 Maxlinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

/**
   \file drv_dxs_event.c
   Handling of driver, firmware and device specific events.*/

/* ========================================================================== */
/*                                 Includes                                   */
/* ========================================================================== */
#include "drv_dxs_api.h"
#include "drv_dxs_access.h"

/* ========================================================================== */
/*                             Macro definitions                              */
/* ========================================================================== */

/* ========================================================================== */
/*                             Type definitions                               */
/* ========================================================================== */

/* ========================================================================== */
/*                             Global variables                               */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */
static IFX_void_t DXS_StripPathCpy (IFX_char_t *dst, const IFX_char_t *src);

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */
/**
   Report a channel error to TAPI

   \param  pCh          Pointer to the DXS channel structure.
   \param  err          error number
   \param  nLine        Line number in source file this error is reported.
   \param  sFile        Source filename in which this error is reported.
   \param  info         Extra data to be logged for this error.

   \return
   None.
*/
IFX_void_t DXS_ChErrorEvent (DXS_CHANNEL_t *pCh,
                             IFX_uint16_t err,
                             IFX_uint32_t nLine,
                             const IFX_char_t *sFile,
                             const IFX_void_t *info)
{
   IFX_TAPI_ErrorLine_t errLine = {0};
   IFX_TAPI_EVENT_t tapiEvent = {0};
   /* Get tapi channel context. */
   TAPI_CHANNEL *pChannel = (TAPI_CHANNEL *)pCh->pTapiCh;

   /* Fill event structure. */
   errLine.nLlCode = err;
   errLine.nHlCode = 0;
   errLine.nLine = nLine;
   /*lint -save -e(420) */
   if (info != IFX_NULL)
   {
      memcpy(errLine.msg, info, sizeof(errLine.msg));
   }
   /*lint -restore */
   DXS_StripPathCpy(errLine.sFile, sFile);
   tapiEvent.id = IFX_TAPI_EVENT_FAULT_GENERAL_CHINFO;
   DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_NONE);
   tapiEvent.data.error = &errLine;

   IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
}

/**
   Report a device error to TAPI

   \param  pDev         Pointer to DXS device struct.
   \param  err          error number
   \param  nLine        Line number in source file this error is reported.
   \param  sFile        Source filename in which this error is reported.
   \param  info         Extra data to be logged for this error.

   \return
   None.
*/
IFX_void_t DXS_DevErrorEvent (DXS_DEVICE_t *pDev,
                              IFX_uint16_t err,
                              IFX_uint32_t nLine,
                              const IFX_char_t* sFile,
                              const IFX_void_t* info)
{
   /*lint -save -e670 -e420 ignore copying more bytes than info contains */
   IFX_TAPI_ErrorLine_t errLine;
   IFX_TAPI_EVENT_t tapiEvent;

   /* Fill event structure. */
   memset(&tapiEvent, 0, sizeof(IFX_TAPI_EVENT_t));
   errLine.nHlCode = 0;
   errLine.nLlCode = err;
   errLine.nLine = nLine;
   if (info != IFX_NULL)
   {
      memcpy (errLine.msg, info, sizeof (errLine.msg));
   }
   DXS_StripPathCpy (errLine.sFile, sFile);
   tapiEvent.id = IFX_TAPI_EVENT_FAULT_GENERAL_DEVINFO;
   DXS_TAPI_EVENT_MODULE_SET(tapiEvent, IFX_TAPI_MODULE_TYPE_NONE);
   tapiEvent.data.error = &errLine;

   IFX_TAPI_Event_Dispatch(pDev->pChannel[0].pTapiCh, &tapiEvent);
   /*lint -restore */
}

/**
   Extract filename from complete path information.

   \param dst   pointer to character array accepting the filename

   \param src   pointer to constant character array holding the complete path
                information of the file

   \return
   None.
*/
/*lint -save -e123 */
static IFX_void_t DXS_StripPathCpy (IFX_char_t *dst, const IFX_char_t *src)
{
   IFX_uint32_t nMax = strlen (src);
   IFX_uint32_t i = nMax;

   /* find position of the last separator */
   while (i > 0 && src[i] != '\\' && src[i] != '/')
   {
      i--;
   }

   if (i > 0)
   {
      nMax = nMax - i;
      /* ignore separator */
      i++;
   }

   if (nMax > (IFX_TAPI_MAX_FILENAME - 1))
   {
      nMax = IFX_TAPI_MAX_FILENAME - 1;
      dst[nMax] = 0;
   }

   memcpy (dst, &src[i], nMax);
}

/*lint -restore */
