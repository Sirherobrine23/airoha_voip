/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

/**
   \file drv_tapi_misc.c
   Contains the High-level TAPI miscellaneous functions.
*/

/* ============================= */
/* Includes                      */
/* ============================= */
#include "drv_tapi.h"
#include "drv_tapi_api.h"
#include "drv_tapi_errno.h"
#include "drv_tapi_version.h"

/* ============================= */
/* Local function declaration    */
/* ============================= */

static void stripPathCpy(IFX_char_t* dst, const IFX_char_t* src);

/* ============================= */
/* Global function definition    */
/* ============================= */


/**
   Returns the version string.

   \param pVer - pointer to buffer for version string

   \return
      TAPI_statusOk

   \remarks
      buffer for result (pVer) should be equal to the IFX_TAPI_VERSION_LEN
*/
IFX_int32_t TAPI_Phone_Get_Version(IFX_char_t *pVer)
{
   *pVer= '\0';
   strncpy(pVer, &TAPI_DRV_WHATVERSION[4], IFX_TAPI_VERSION_LEN);
   pVer[IFX_TAPI_VERSION_LEN - 1] = '\0';

   return TAPI_statusOk;
}

/**
   Returns error if the requested version is not supported

   \param vers     - pointer to version structure

   \return
   TAPI_statusOk if version is supported or TAPI_statusErr

   \remarks
   Since an application is always build against one specific TAPI interface
   version it should check if it is supported. If not the application should
   abort. This interface checks if the current TAPI version supports a
   particular version. For example the TAPI versions 3.2.1 will support TAPI 3.2.0.
   But version 4.3.0 might not support 3.2.0.
*/
IFX_int32_t TAPI_Phone_Check_Version (IFX_TAPI_VERSION_t const *vers)
{
   if (IFX_TAPI_VERSION(DRV_TAPI_VER_MAJOR, DRV_TAPI_VER_MINOR, 0) ==
       IFX_TAPI_VERSION(vers->majorNumber, vers->minorNumber, 0))
   {
      return TAPI_statusOk;
   }
   return TAPI_statusErr;
}

/**
   Check for a valid function pointer and issue a trace with the name of the
   function pointer, in case it doesn't exist.

   \param  ptr          Function pointer to be checked.
   \param  pPtrName     Identifier used to trace in case the ptr is NULL.

   \return
      - IFX_SUCCESS or
      - IFX_ERROR in case the ptr is NULL

   \remarks
   Name or interface of this function can be changed!
   Do not use direct call of this function.
   Please use simple interface IFX_TAPI_PtrChk to check pointer.
   Commented out the 2nd const from the first parameter as gcc complains about
   this with "duplicate const" although C89 specification allows this.
*/
IFX_boolean_t ptr_chk(const IFX_void_t *ptr,
                      const IFX_char_t *pPtrName)
{
   TAPI_UNUSED (pPtrName);

   if (ptr == IFX_NULL)
   {
      /*
      TRACE(TAPI_DXS, DBG_LEVEL_LOW,
            ("INFO, function pointer not registered for %s\n", pPtrName));
      */
      return IFX_FALSE;
   }
   /*
   TRACE(TAPI_DXS, DBG_LEVEL_LOW,
         ("INFO, function pointer called %s\n", pPtrName));
   */
   return IFX_TRUE;
}


/**
   Log an error on the error stack of the TAPI device.

   The error code and details of line and file are put onto the error stack
   that is located in the TAPI device. If the stack is full the entry will
   be lost.

   \param  pTapiDevice  Pointer to TAPI device structure.
   \param  nHlCode      HL driver error code.
   \param  nLlCode      LL driver error code.
   \param  nLine        Line in sourcecode where the error occured.
   \param  sFile        Filename of sourcecode where the error occured.
*/
void TAPI_ErrorStatus (TAPI_DEV *pTapiDevice,
                       TAPI_Status_t nHlCode, IFX_int32_t nLlCode,
                       IFX_uint32_t nLine, const IFX_char_t* sFile)
{
   IFX_TAPI_ErrorLine_t* errorLine;

   if (pTapiDevice == IFX_NULL ||
       pTapiDevice->error.nCnt >= IFX_TAPI_MAX_ERROR_ENTRIES)
   {
      /* stack full */
      return;
   }
   errorLine = &pTapiDevice->error.stack[pTapiDevice->error.nCnt];
   stripPathCpy (errorLine->sFile, sFile);
   if ((IFX_return_t)nHlCode == IFX_ERROR)
      nHlCode = (TAPI_Status_t) (TAPI_statusClassErr | TAPI_statusClassCh);
   errorLine->nHlCode = (IFX_uint16_t)nHlCode;
   errorLine->nLlCode = nLlCode;
   errorLine->nLine = nLine;
   pTapiDevice->error.nCnt++;
   pTapiDevice->error.nCode = (IFX_uint32_t)nHlCode; /*lint !e571 */
   pTapiDevice->error.nCode <<= 16;
   pTapiDevice->error.nCode |= (IFX_uint32_t)nLlCode;
}

/**
   Extract the file name from full path.

   \param  dst          Destination string buffer of length
                        \ref IFX_TAPI_MAX_FILENAME.
   \param  src          Source with zero terminated string.

   \remarks
   Result (zero terminated string) is placed in the dst buffer
*/
static void stripPathCpy (IFX_char_t* dst, const IFX_char_t* src)
{
   IFX_uint32_t nMax = strlen (src);
   IFX_uint32_t i    = nMax;

   /* only name of file without path */
   while (i > 0 && src[i] != '\\' && src[i] != '/')
   {
      i--;
   }
   if (i > 0)
   {
      nMax = nMax - i;
      i++; /* ignore / or \ */
   }

   if (nMax >= IFX_TAPI_MAX_FILENAME)
      nMax = IFX_TAPI_MAX_FILENAME - 1;

   memcpy (dst, &src[i], nMax);

   dst[nMax] = 0;
}
