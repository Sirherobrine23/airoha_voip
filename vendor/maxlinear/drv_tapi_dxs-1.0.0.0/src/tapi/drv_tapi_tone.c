/******************************************************************************

                              Copyright (c) 2014
                            Lantiq Deutschland GmbH

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

*******************************************************************************/

/**
   \file drv_tapi_tone.c
   Contains TAPI Tone Services.
*/

/* ============================= */
/* Includes                      */
/* ============================= */

#include "drv_tapi.h"
#include "drv_tapi_api.h"
#include "drv_tapi_errno.h"
#include "drv_tapi_cid.h"


/* ============================= */
/* Local Macros & Definitions    */
/* ============================= */

/* tone coefficient maximums */
#define MAX_TONE_TIME          16383  /* ms */ /* maximum: (2^14)-1 */
#define MAX_TONE_FREQ          32768
#define MIN_TONE_POWER         -300
#define MAX_TONE_POWER         0
#define MAX_CADENCE_TIME      64000   /* ms */
#define MAX_VOICE_TIME        64000   /* ms */

#define MAX_CMD_WORD             31


/* ============================= */
/* Local type definition         */
/* ============================= */

typedef enum
{
   TAPI_TONE_TYPE_NONE,
   TAPI_TONE_TYPE_SIMPLE,
   TAPI_TONE_TYPE_COMP,
   TAPI_TONE_TYPE_PREDEF,
   TAPI_TONE_TYPE_DUAL
} TAPI_TONE_TYPE;

typedef struct
{
   TAPI_TONE_TYPE          type;
   union
   {
      IFX_TAPI_TONE_DUAL_t       dual;
      IFX_TAPI_TONE_SIMPLE_t     simple;
      IFX_TAPI_TONE_COMPOSED_t   composed;
   } tone;
} COMPLEX_TONE;

struct TAPI_TONE_DATA
{
   /* type of tone sequence SIMPLE or COMPOSED */
   TAPI_TONE_TYPE          nType;
   /* maximum number of repetitions for simple tone sequence */
   IFX_uint32_t            nSimpleMaxReps;
   /* current repetition counter for simple tone sequence */
   IFX_uint32_t            nSimpleCurrReps;
   /* maximum number of repetitions for composed tone sequence */
   IFX_uint32_t            nComposedMaxReps;
   /* current repetition counter for composed tone sequence */
   IFX_uint32_t            nComposedCurrReps;
   /* pause time in ms between simple tones */
   IFX_uint32_t            nPauseTime;
   /* alternate voice time in ms between composed tones */
   IFX_uint32_t            nAlternateVoiceTime;
   /* current simple tone code playing */
   IFX_uint32_t            nSimpleToneCode;
   /* current composed tone code playing */
   IFX_uint32_t            nComposedToneCode;
   /* maximum simple tones within composed tone */
   IFX_uint32_t            nMaxToneCount;
   /* current simple tone within composed tone */
   IFX_uint32_t            nToneCounter;
   /* complex tone playing state */
   TAPI_CMPLX_TONE_STATE_t nToneState;
   /** Specifies the capability of the tone generator regarding tone
       sequence support. See \ref IFX_TAPI_TONE_RESSEQ_t for details. */
   IFX_TAPI_TONE_RESSEQ_t  sequenceCap;
   /** Module type */
   IFX_TAPI_MODULE_TYPE_t  module;
   /** stores the index of the tone currently played */
   IFX_int32_t             nToneIndex;
   /** send IFX_TAPI_EVENT_TONE_GEN_END ? */
   IFX_boolean_t           bSendToneGenEndTapiEvent;
};

/** tone resource structure as time argument */
struct TAPI_TONE_RES
{
   /* timer id for voice path establishment */
   Timer_ID          Tone_Timer;
   TAPI_CHANNEL      *pTapiCh;
   /** Resource number of the tone play unit */
   IFX_uint32_t      nRes;
};


/* ============================= */
/* Local variable definition     */
/* ============================= */

#ifdef TAPI_FEAT_TONETABLE

/* Predefined tones frequencies */
static
const IFX_uint32_t TAPI_PredefinedToneFreq [IFX_TAPI_TONE_INDEX_MIN - 1][2] =
{
    /* Freq1/Freq2(Hz) *  Freq1/Freq2(Hz) - Index - DTMF Digit */
    {697, 1209},   /*   697/1209 Hz,   1,       1 */
    {697, 1336},   /*   697/1336 Hz,   2,       2 */
    {697, 1477},   /*   697/1477 Hz,   3,       3 */
    {770, 1209},   /*   770/1209 Hz,   4,       4 */
    {770, 1336},   /*   770/1336 Hz,   5,       5 */
    {770, 1477},   /*   770/1477 Hz,   6,       6 */
    {852, 1209},   /*   852/1209 Hz,   7,       7 */
    {852, 1336},   /*   852/1336 Hz,   8,       8 */
    {852, 1477},   /*   852/1477 Hz,   9,       9 */
    {941, 1209},   /*   941/1209 Hz,   10,      * */
    {941, 1336},   /*   941/1336 Hz,   11,      0 */
    {941, 1477},   /*   941/1477 Hz,   12,      # */
    {800,    0},   /*   800/0    Hz,   13,      - */
    {1000,   0},   /*   1000/0   Hz,   14,      - */
    {1250,   0},   /*   1250/0   Hz,   15,      - */
    {950,    0},   /*   950/0    Hz,   16,      - */
    {1100,   0},   /*   1100/0   Hz,   17,      - */ /* --> CNG Tone */
    {1400,   0},   /*   1400/0   Hz,   18,      - */
    {1500,   0},   /*   1500/0   Hz,   19,      - */
    {1600,   0},   /*   1600/0   Hz,   20,      - */
    {1800,   0},   /*   1800/0   Hz,   21,      - */
    {2100,   0},   /*   2100/0   Hz,   22,      - */ /* --> CED Tone */
    {2300,   0},   /*   2300/0   Hz,   23,      - */
    {2450,   0},   /*   2450/0   Hz,   24,      - */
    {350,  440},   /*   350/440  Hz,   25,      - */  /* --> Dial Tone */
    {440,  480},   /*   440/480  Hz,   26,      - */  /* --> Ring Back */
    {480,  620},   /*   480/620  Hz,   27,      - */  /* --> Busy Tone */
    {697, 1633},   /*   697/1633 Hz,   28,      A */
    {770, 1633},   /*   770/1633 Hz,   29,      B */
    {852, 1633},   /*   852/1633 Hz,   30,      C */
    {941, 1633},   /*   941/1633 Hz,   31,      D */
};

/* One static tone table array variable that is shared by all devices. */
/* Zero values indicate that a tone table entry is unconfigured. */
static COMPLEX_TONE toneTable[TAPI_MAX_TONE_CODE];

#endif /* TAPI_FEAT_TONETABLE */


/* ============================= */
/* Local function declaration    */
/* ============================= */

#ifdef TAPI_FEAT_TONEENGINE

static IFX_void_t ifx_tapi_Tone_Cleanup_Unprot(
                        TAPI_CHANNEL *pChannel);

static IFX_void_t ComplexTone_Conf (
                        TAPI_TONE_DATA_t *pData,
                        COMPLEX_TONE *pToneCoeffTable,
                        COMPLEX_TONE const *pToneCoeff);

static IFX_void_t Event_SimpleTone (
                        TAPI_CHANNEL *pChannel,
                        IFX_uint8_t nResID);

static IFX_void_t Event_ComposedTone (
                        TAPI_CHANNEL *pChannel,
                        IFX_uint8_t nResID);

static IFX_void_t Tone_OnTimer (
                        Timer_ID Timer,
                        IFX_ulong_t nArg);

static IFX_int32_t TAPI_Phone_Tone_Stop_Unprot (
                        TAPI_CHANNEL *pChannel,
                        const IFX_TAPI_TONE_PLAY_t *pTone);

static IFX_int32_t TAPI_Tone_ResIdGet (
                        TAPI_CHANNEL* pChannel,
                        const IFX_TAPI_TONE_PLAY_t* pTone,
                        IFX_TAPI_TONE_RES_t *pRes);

#endif /* TAPI_FEAT_TONEENGINE */


/* ============================= */
/* Function implementation       */
/* ============================= */

#ifdef TAPI_FEAT_TONEENGINE
/**
   If the LL function for resource information is not available
   this function is called to set the resource ID by using the parameters
   src and dst.

   \param  pChannel     Pointer to TAPI_CHANNEL structure.
   \param  pTone        Pointer to tone to be played.
   \param  pRes         The resource information of the tone generator.
                        The structure is filled in this function.
   \return
   - TAPI_statusToneNoRes No free resource was found.
   - TAPI_statusLLNotSupp LL driver does not support any tone playing.
   - TAPI_statusOk if the resource was successfully specified.
*/
static IFX_int32_t TAPI_Tone_ResIdGet (
                        TAPI_CHANNEL* pChannel,
                        const IFX_TAPI_TONE_PLAY_t* pTone,
                        IFX_TAPI_TONE_RES_t *pRes)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;

   /* Make sure this TAPI channel was initialised for playing tones. */
   if (pChannel->pToneRes == IFX_NULL)
   {
      /* errmsg: Service not supported on called channel context */
      RETURN_STATUS (TAPI_statusInvalidCh, 0);
   }

   if (pTone->module == IFX_TAPI_MODULE_TYPE_ALM)
   {
      if (pDrvCtx->ALM.TG_Play == IFX_NULL)
      {
         RETURN_STATUS (TAPI_statusLLNotSupp, 0);
      }
      pRes->sequenceCap = IFX_TAPI_TONE_RESSEQ_FREQ;
   }

   /* try to find a free resource that is idle or
      a resource that is already playing the same index */
   for (pRes->nResID = 0; pRes->nResID < TAPI_TONE_MAXRES; pRes->nResID++)
   {
      const TAPI_TONE_DATA_t *pData = &pChannel->TapiComplexToneData[pRes->nResID];

      if ((pData->nToneState == TAPI_CT_IDLE) ||
          (pData->nToneState == TAPI_CT_DEACTIVATED))
      {
         /* a free resource was found */
         break;
      }

      if (pData->nToneIndex == pTone->index)
      {
         /* a resource already playing the same tone was found */
         break;
      }
   }

   return TAPI_statusOk;
}
#endif /* TAPI_FEAT_TONEENGINE */


#ifdef TAPI_FEAT_TONEENGINE
/**
   Initialise tone service on the given channel.

   Initialise the data structures and resources needed for the tone service.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.

   \return
     TAPI_statusOk if successful, otherwise error code

   \remarks This function is not protected. The global protection is done
   in the calling function with TAPI_OS_MutexGet (&pChannel->semTapiChDataLock)
*/
IFX_int32_t IFX_TAPI_Tone_Initialise_Unprot(TAPI_CHANNEL *pChannel)
{
   IFX_int32_t ret = TAPI_statusOk;
   IFX_uint32_t i;

   /* Initialise tone only on channels which include tone capabilities. */
   if (pChannel->nChannel >= pChannel->pTapiDevice->nResource.ToneCount)
   {
      /* not a tone capable channel -> nothing to do  --
         this is not an error */
      return IFX_SUCCESS;
   }

   /* allocate memory for tone resources */
   pChannel->pToneRes =
      TAPI_OS_Malloc (sizeof(TAPI_TONE_RES_t) * TAPI_TONE_MAXRES);
   if (pChannel->pToneRes == IFX_NULL)
   {
      ret = TAPI_statusNoMem;
   }
   else
   {
      memset (pChannel->pToneRes, 0,
              sizeof(TAPI_TONE_RES_t) * TAPI_TONE_MAXRES);
   }

   if (TAPI_SUCCESS(ret))
   {
      pChannel->TapiComplexToneData =
         TAPI_OS_Malloc (sizeof(TAPI_TONE_DATA_t) * TAPI_TONE_MAXRES);
      if (pChannel->TapiComplexToneData == IFX_NULL)
      {
         ret = TAPI_statusNoMem;
      }
      else
      {
         memset (pChannel->TapiComplexToneData, 0,
                 sizeof(TAPI_TONE_DATA_t) * TAPI_TONE_MAXRES);
      }
   }

   /* allocate timer and set status for tone resources */
   for (i = 0; TAPI_SUCCESS(ret) && (i < TAPI_TONE_MAXRES); ++i)
   {
      /* start complex tone generation in idle state */
      pChannel->TapiComplexToneData[i].nToneState = TAPI_CT_IDLE;
      /* create a timer for each of the tone generator resources */
      if (pChannel->pToneRes[i].Tone_Timer == 0)
      {
         /* initialize (create) voice path teardown timer */
         pChannel->pToneRes[i].Tone_Timer =
           TAPI_Create_Timer((TIMER_ENTRY)Tone_OnTimer,
                             (IFX_uintptr_t)&pChannel->pToneRes[i]);
         pChannel->pToneRes[i].pTapiCh = pChannel;
         pChannel->pToneRes[i].nRes = i;
         if (pChannel->pToneRes[i].Tone_Timer == 0)
         {
            ret = TAPI_statusTimerFail;
         }
      }
   }

   if (!TAPI_SUCCESS(ret))
   {
      ifx_tapi_Tone_Cleanup_Unprot(pChannel);
   }

   RETURN_STATUS (ret, 0);
}
#endif /* TAPI_FEAT_TONEENGINE */


#ifdef TAPI_FEAT_TONEENGINE
/**
   Cleanup Tone service on the given channel.

   Free the resources needed for the tone service.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.
*/
IFX_void_t ifx_tapi_Tone_Cleanup_Unprot(TAPI_CHANNEL *pChannel)
{
   if (pChannel->pToneRes != IFX_NULL)
   {
      IFX_int32_t i;

      /* delete the tone timer resources */
      for (i = 0; i < TAPI_TONE_MAXRES; ++i)
      {
         if (pChannel->pToneRes[i].Tone_Timer != 0)
         {
            TAPI_Delete_Timer (pChannel->pToneRes[i].Tone_Timer);
         }
      }

      /* free tone resource */
      TAPI_OS_Free (pChannel->pToneRes);
      pChannel->pToneRes = IFX_NULL;
   }

   if (pChannel->TapiComplexToneData != IFX_NULL)
   {
      /* free complex tone data struct */
      TAPI_OS_Free (pChannel->TapiComplexToneData);
      pChannel->TapiComplexToneData = IFX_NULL;
   }
}
#endif /* TAPI_FEAT_TONEENGINE */


#ifdef TAPI_FEAT_TONEENGINE
/**
   Cleanup Tone service on the given channel.

   Free the resources needed for the tone service.

   \param   pChannel    Pointer to TAPI_CHANNEL structure.
*/
IFX_void_t IFX_TAPI_Tone_Cleanup(TAPI_CHANNEL *pChannel)
{
   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);
   ifx_tapi_Tone_Cleanup_Unprot(pChannel);
   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);
}
#endif /* TAPI_FEAT_TONEENGINE */


#ifdef TAPI_FEAT_TONETABLE
/**
   Configures predefined tones from Index 1 to 31 and additionally reserved
   tones for CID.

   \return
   - TAPI_statusOk tones configured successfully.
   - TAPI_statusParam Parameter outside of allowed range.

   \remarks
   Constant Table TAPI_PredefinedToneFreq defines the frequencies used for
   the predefined tones in index range 1 to 31. In case this table is changed
   in the future, this function must be adapted accordingly.
   The current table status is :
      - Either Freq2 > Freq 1 => DTMF,
      - or Freq2 = 0 (e.g CNG tone)
*/
IFX_int32_t TAPI_Phone_Tone_Predef_Config (void)
{
   IFX_TAPI_TONE_SIMPLE_t predef_tone;
   IFX_uint32_t i;
   IFX_int32_t ret = TAPI_statusOk;

   /* setup tone structure for predefined tone */
   memset (&predef_tone, 0, sizeof (predef_tone));
   /* add all predefined tones to tone table */
   for (i = 0; i < (IFX_TAPI_TONE_INDEX_MIN - 1); i++)
   {
      /* read frequencies from contant table */
      predef_tone.freqA       = TAPI_PredefinedToneFreq[i][0];
      predef_tone.freqB       = TAPI_PredefinedToneFreq[i][1];
      /* tone indexes start from 1 on */
      predef_tone.index       = i + 1 ;
      /* set frequencies to play and levels */
      switch (predef_tone.freqB)
      {
         case  0:
            /* play only freqA */
            predef_tone.frequencies [0] = IFX_TAPI_TONE_FREQA;
            /* -9 dB = -90 x 0.1 dB for the higher frequency */
            predef_tone.levelA          = -90;
            predef_tone.levelB          = 0;
            break;
         default:
            /* In this case, freqB > freqA according to table
               TAPI_PredefinedToneFreq : Play both frequencies as dtmf  */
            predef_tone.frequencies [0] = (IFX_TAPI_TONE_FREQA |
                                           IFX_TAPI_TONE_FREQB);
            /* -11 dB = -110 x 0.1 dB for the lower frequency ,
               -9  dB = -90  * 0.1 dB for the higher frequency
                        (to attenuate the higher tx loss) */
            predef_tone.levelA          = -110;
            predef_tone.levelB          = -90;
            break;
      }
      /* set cadences and loop */
      switch (predef_tone.index)
      {
         case 17:  /* CNG */
         case 22:  /* CED */
            predef_tone.cadence [0] = 1000;
            predef_tone.cadence [1] = 0;
            /* tone played continuously */
            predef_tone.loop = 0;
            break;
         case 25:  /* (dialtone) */
            predef_tone.cadence [0] = 1000;
            predef_tone.cadence [1] = 0;
            /* tone played continuously */
            predef_tone.loop = 0;
            break;
         case 26:  /* (ringback tone) */
            predef_tone.cadence [0] = 1000;
            predef_tone.cadence [1] = 4000;
            /* tone played continuously */
            predef_tone.loop = 0;
            break;
         case 27:  /* (busy tone) */
            predef_tone.cadence [0] = 500;
            predef_tone.cadence [1] = 500;
            /* tone played continuously */
            predef_tone.loop = 0;
            break;
         default:
            /* all other tones will be played only once for 100 ms */
            predef_tone.cadence [0] = 100;
            predef_tone.cadence [1] = 0;
            /* tone played only once */
            predef_tone.loop = 1;
            break;
      }
      ret = TAPI_Phone_Add_SimpleTone (&predef_tone);
   }

#ifdef TAPI_FEAT_CID
   if (TAPI_SUCCESS (ret))
   {
      /* setup predefined CID alert tones */
      ret = IFX_TAPI_CID_SetPredefAlertTones ();
   }
#endif /* TAPI_FEAT_CID */

   return ret;
}
#endif /* TAPI_FEAT_TONETABLE */


#ifdef TAPI_FEAT_TONETABLE
/**
   Add simple tone to internal tone coefficients table

   \param  pSimpleTone  Simple tone to be set.

   \return
   - TAPI_statusOk - success
   - TAPI_statusToneIdxOutOfRange - Tone index out of range
   - TAPI_statusToneOnTimeOutOfRange - Simple tone on-time time exceeds the
         maximum range
   - TAPI_statusToneInitialTimeZero - Simple tone time of first step may not
         be zero
   - TAPI_statusTonePauseTimeOutOfRange - Simple tone pause time exceeds the
         maximum supported pause time
   - TAPI_statusToneFrequencyOutOfRange - Simple tone frequency exceeds the
         maximum supported frequency
   - TAPI_statusTonePowerLevelOutOfRange - Simple tone power level exceeds the
         supported range
*/
IFX_int32_t TAPI_Phone_Add_SimpleTone (IFX_TAPI_TONE_SIMPLE_t const *pSimpleTone)
{
   IFX_uint32_t i;

   if (pSimpleTone->index >= TAPI_MAX_TONE_CODE || pSimpleTone->index == 0)
   {
      /* errmsg: Tone index out of range */
      return TAPI_statusToneIdxOutOfRange;
   }
   /* make sure the first cadence time is not zero */
   if (pSimpleTone->cadence[0] == 0)
   {
      /* errmsg: Simple tone time of first step may not be zero */
      return TAPI_statusToneInitialTimeZero;
   }
   /* make sure the defined cadence times do not exceed the maximum time */
   for (i=0; (i < IFX_TAPI_TONE_STEPS_MAX) &&
             (pSimpleTone->cadence[i] != 0); ++i)
   {
      if (pSimpleTone->cadence[i] > MAX_TONE_TIME)
      {
         /* errmsg: Simple tone on-time time exceeds the maximum range */
         return TAPI_statusToneOnTimeOutOfRange;
      }
   }
   /* check pause time to set */
   if (pSimpleTone->pause > MAX_CADENCE_TIME)
   {
      /* errmsg: Simple tone pause time exceeds the maximum supported pause time */
      return TAPI_statusTonePauseTimeOutOfRange;
   }

   /* check frequencies to set */
   if (pSimpleTone->freqA > MAX_TONE_FREQ ||
       pSimpleTone->freqB > MAX_TONE_FREQ ||
       pSimpleTone->freqC > MAX_TONE_FREQ ||
       pSimpleTone->freqD > MAX_TONE_FREQ)
   {
      /* errmsg: Simple tone frequency exceeds the maximum supported frequency */
      return TAPI_statusToneFrequencyOutOfRange;
   }
   /* check power level to set */
   if (pSimpleTone->levelA < MIN_TONE_POWER ||
       pSimpleTone->levelA > MAX_TONE_POWER ||
       pSimpleTone->levelB < MIN_TONE_POWER ||
       pSimpleTone->levelB > MAX_TONE_POWER ||
       pSimpleTone->levelD < MIN_TONE_POWER ||
       pSimpleTone->levelD > MAX_TONE_POWER ||
       pSimpleTone->levelC < MIN_TONE_POWER ||
       pSimpleTone->levelC > MAX_TONE_POWER)
   {
      /* errmsg: Simple tone power level exceeds the supported range */
      return TAPI_statusTonePowerLevelOutOfRange;
   }

   /* Add entry to internal tone table */
   toneTable[pSimpleTone->index].type = TAPI_TONE_TYPE_SIMPLE;
   toneTable[pSimpleTone->index].tone.simple = *pSimpleTone;

   return TAPI_statusOk;
}
#endif /* TAPI_FEAT_TONETABLE */


#ifdef TAPI_FEAT_TONETABLE
/**
   Add composed tone to internal tone coefficients table

   \param pComposedTone  Entry for internal tone table.

   \return
   - TAPI_statusOk - tone successfuly added  or IFX_ERROR
   - TAPI_statusToneIdxOutOfRange - Tone index out of range
   - TAPI_statusToneAltVoiceTimeOutOfRange - Composed tone max alternate voice
         time exceeds the maximum
   - TAPI_statusToneLoopCountInvalid - Composed tone single repetition not
         allowed when alternate voice time is non-zero
   - TAPI_statusToneCountOutOfRange - Composed tone number of simple tone
         fields is out of range
   - TAPI_statusToneSimpleIdxOutOfRange - Composed tone contains a simple tone
         index that is out of range
   - TAPI_statusToneSimpleToneUnconfigured - Composed tone contains a simple
         tone that is not configured
*/
IFX_int32_t TAPI_Phone_Add_ComposedTone (IFX_TAPI_TONE_COMPOSED_t const *pComposedTone)
{
   IFX_uint32_t i;

   if (pComposedTone->index >= TAPI_MAX_TONE_CODE || pComposedTone->index == 0)
   {
      /* errmsg: Tone index out of range */
      return TAPI_statusToneIdxOutOfRange;
   }
   if (pComposedTone->index < IFX_TAPI_TONE_INDEX_MIN)
   {
      return TAPI_statusTonePredefFail;
   }

   /* check alternate voice time to set */
   if (pComposedTone->alternatVoicePath > MAX_VOICE_TIME)
   {
      /* errmsg: Composed tone max alternate voice time exceeds the maximum */
      return TAPI_statusToneAltVoiceTimeOutOfRange;
   }

   /* check loop count against alternate voice time */
   if (pComposedTone->loop == 1 && pComposedTone->alternatVoicePath != 0)
   {
      /* errmsg: Composed tone single repetition not allowed when
                 alternate voice time is non-zero */
      return TAPI_statusToneLoopCountInvalid;
   }

   /* Validate the number of simple tones allowed within composed tone */
   if (pComposedTone->count == 0 ||
       pComposedTone->count > IFX_TAPI_TONE_SIMPLE_MAX)
   {
      /* errmsg: Composed tone number of simple tone fields is out of range */
      return TAPI_statusToneCountOutOfRange;
   }

   /* Validate if all simple tone codes are configured within the composed tone */
   for (i = 0; i < pComposedTone->count; i++)
   {
      if (pComposedTone->tones[i] >= TAPI_MAX_TONE_CODE ||
         pComposedTone->tones[i] == 0)
      {
         /* errmsg: Composed tone contains a simple tone index that is out of range */
         return TAPI_statusToneSimpleIdxOutOfRange;
      }
      if (toneTable[pComposedTone->tones[i]].type == TAPI_TONE_TYPE_NONE)
      {
         /* errmsg: Composed tone contains a simple tone that is not configured */
         return TAPI_statusToneSimpleToneUnconfigured;
      }
   }

   /* Add entry to internal tone table */
   toneTable[pComposedTone->index].type = TAPI_TONE_TYPE_COMP;
   memcpy ((void *)&toneTable[pComposedTone->index].tone.composed,
           (const void *)pComposedTone, sizeof(IFX_TAPI_TONE_COMPOSED_t));

   return TAPI_statusOk;
}
#endif /* TAPI_FEAT_TONETABLE */


#ifdef TAPI_FEAT_TONETABLE
/**
   Add tone to internal tone coefficients table

   \param  pTone        Tone to be inserted into the internal tone table.

   \return
   - TAPI_statusOk - tone successfully added to the table
   - TAPI_statusInvalidToneRes - invalid tone resource specified or
                                 return specific error code
*/
IFX_int32_t TAPI_Phone_Tone_TableConf (IFX_TAPI_TONE_t const *pTone)
{
   IFX_int32_t ret = TAPI_statusOk;

   switch (pTone->simple.format)
   {
      case  IFX_TAPI_TONE_TYPE_SIMPLE:
         ret = TAPI_Phone_Add_SimpleTone (&pTone->simple);
      break;
      case  IFX_TAPI_TONE_TYPE_COMPOSED:
         ret = TAPI_Phone_Add_ComposedTone (&pTone->composed);
      break;
      default:
         ret = TAPI_statusInvalidToneRes;
   }

   return ret;
}
#endif /* TAPI_FEAT_TONETABLE */


#ifdef TAPI_FEAT_TONEENGINE
/**
   Configure complex tone play

   \param  pData           Pointer to TAPI tone resource data.
   \param  pToneCoeffTable Pointer to TAPI complex tone table.
   \param  pToneCoeff      Pointer to tone to be played.
*/
static IFX_void_t ComplexTone_Conf (TAPI_TONE_DATA_t * pData,
                                    COMPLEX_TONE *pToneCoeffTable,
                                    COMPLEX_TONE const *pToneCoeff)
{
   /* Setup the tone code type */
   pData->nType = pToneCoeff->type;

   /* This is a composed tone so get the first simple tone sequence to play */
   if (pToneCoeff->type == TAPI_TONE_TYPE_COMP)
   {
      IFX_uint32_t firstToneCode = 0;

      /* Retrieve composed configuration from internal tone coefficients table */
      pData->nComposedMaxReps    = pToneCoeff->tone.composed.loop;
      pData->nComposedCurrReps   = 0;
      pData->nAlternateVoiceTime = pToneCoeff->tone.composed.alternatVoicePath;
      pData->nComposedToneCode   = pToneCoeff->tone.composed.index;
      pData->nMaxToneCount       = pToneCoeff->tone.composed.count;
      pData->nToneCounter        = 0;

      /* First simple tone sequence to play */
      firstToneCode = pToneCoeff->tone.composed.tones[0];

      /* Use the simple tone code as an index into internal tone coefficients table */
      pToneCoeff = &pToneCoeffTable[firstToneCode];
   }

   /* Retrieve simple tone configuration from internal tone coefficients table */
   pData->nSimpleMaxReps      = pToneCoeff->tone.simple.loop;
   pData->nSimpleCurrReps     = 0;
   pData->nPauseTime          = pToneCoeff->tone.simple.pause;
   pData->nSimpleToneCode     = pToneCoeff->tone.simple.index;
}
#endif /* TAPI_FEAT_TONEENGINE */


#ifdef TAPI_FEAT_TONEGEN
/**
   Plays a dial tone.

   \param pChannel     - handle to TAPI_CHANNEL structure

   \return
   IFX_SUCCESS or IFX_ERROR
*/
IFX_int32_t TAPI_Phone_Tone_Dial(TAPI_CHANNEL *pChannel)
{
   /* play dial tone */
   return TAPI_Phone_Tone_Local_Play (pChannel, 25);
}
#endif /* TAPI_FEAT_TONEGEN */


#ifdef TAPI_FEAT_TONEGEN
/**
   Plays a ringback tone.

   \param pChannel     - handle to TAPI_CHANNEL structure

   \return
   IFX_SUCCESS or IFX_ERROR
*/
IFX_int32_t TAPI_Phone_Tone_Ringback(TAPI_CHANNEL *pChannel)
{
   return TAPI_Phone_Tone_Local_Play (pChannel, 26);
}
#endif /* TAPI_FEAT_TONEGEN */


#ifdef TAPI_FEAT_TONEGEN
/**
   Plays a busy tone.

   \param pChannel     - handle to TAPI_CHANNEL structure

   \return:
   TAPI_statusOk - plays busy tone correctly
   or error code returned from the called function
*/
IFX_int32_t TAPI_Phone_Tone_Busy (TAPI_CHANNEL *pChannel)
{
   return TAPI_Phone_Tone_Local_Play (pChannel, 27);
}
#endif /* TAPI_FEAT_TONEGEN */


#ifdef TAPI_FEAT_TONEGEN
/**
   Plays/Stop a tone from the tone table

   \param pChannel   Handle to TAPI channel
   \param pTone      Handle to the tone descriptor

   \return TAPI_statusOk on success

   \remarks
      Tone index equal to zero will stop the tone.
*/
IFX_int32_t TAPI_Target_Tone_Play (TAPI_CHANNEL *pChannel,
                                   IFX_TAPI_TONE_PLAY_t *pTone)
{
   IFX_int32_t    ret;

   if (pTone->module < IFX_TAPI_MODULE_TYPE_NONE ||
       pTone->module > IFX_TAPI_MODULE_TYPE_ALL)
   {
      return TAPI_statusParam;
   }

   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);
   /* Tone index == 0 is stop */
   if (pTone->index == 0)
   {
      /* if index is 0 stop all tone generators in this direction */
      ret = TAPI_Phone_Tone_Stop_Unprot (pChannel, pTone);
   }
   else
   {
      ret = TAPI_Phone_Tone_Play_Unprot (pChannel, pTone,
         IFX_TRUE /* send TONE_END event */);
   }
   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);

   return ret;
}
#endif /* TAPI_FEAT_TONEGEN */


#ifdef TAPI_FEAT_TONEGEN
/**
   Plays a tone from the tone table to the local direction

   \param pChannel     Handle to TAPI channel
   \param nToneIndex   index in the tone table

   \return TAPI_statusOk on success
*/
IFX_int32_t TAPI_Phone_Tone_Local_Play (TAPI_CHANNEL *pChannel,
   IFX_int32_t nToneIndex)
{
   return TAPI_Phone_Tone_Play (pChannel, nToneIndex);
}
#endif /* TAPI_FEAT_TONEGEN */


#ifdef TAPI_FEAT_TONEGEN
/**
   Stops tone generation on local direction

   \param pChannel     Handle to TAPI channel
   \param nToneIndex   index in the tone table

   \return TAPI_statusOk on success
*/
IFX_int32_t TAPI_Phone_Tone_Local_Stop (TAPI_CHANNEL *pChannel,
   IFX_int32_t nToneIndex)
{
   IFX_TAPI_TONE_PLAY_t tmp = {0};

   tmp.index = (IFX_uint32_t)nToneIndex;
   tmp.module = IFX_TAPI_MODULE_TYPE_ALM;
   return TAPI_Phone_Tone_Stop(pChannel, &tmp);
}
#endif /* TAPI_FEAT_TONEGEN */


#ifdef TAPI_FEAT_TONEGEN
/**
   Stops tone generation on default direction

   \param pChannel     Handle to TAPI channel
   \param pTone      Handle to the tone descriptor

   \return TAPI_statusOk on success
*/
IFX_int32_t TAPI_Phone_Tone_Def_Stop (TAPI_CHANNEL *pChannel,
   IFX_TAPI_TONE_PLAY_t *pTone)
{
   return TAPI_Phone_Tone_Stop (pChannel, pTone);
}
#endif /* TAPI_FEAT_TONEGEN */


#ifdef TAPI_FEAT_TONEENGINE
/**
   Plays a tone from the tone table (protected from multiple access).

   This function is called when protection is needed for
   TAPI_Phone_Tone_Play_Unprot()
   It does the protection before the function for playing a tone is called.

   \param pChannel     - handle to TAPI_CHANNEL structure
   \param nToneIndex   - index in the tone table
   \param dst          - direction where to play the tone: external or internal

   \return
   IFX_SUCCESS or IFX_ERROR
*/
IFX_int32_t TAPI_Phone_Tone_Play(TAPI_CHANNEL *pChannel,
                                 IFX_int32_t nToneIndex)
{
   IFX_int32_t ret = IFX_ERROR;
   IFX_TAPI_TONE_PLAY_t tone = {0};

   nToneIndex &= ~IFX_TAPI_TONE_SRC_TG;

   tone.index = (IFX_uint32_t)nToneIndex;
   tone.internal = IFX_FALSE;
   tone.module = IFX_TAPI_MODULE_TYPE_ALM;
   tone.external = IFX_TRUE;

   /* make sure the resource is not in use before allocating it */
   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);
   /* Tone index == 0 is stop */
   if (nToneIndex == 0)
   {
      /* if index is 0 stop all tone generators in this direction */
      ret = TAPI_Phone_Tone_Stop_Unprot (pChannel, &tone);
   }
   else
   {
      /* Send TAPI event for all tones played through IOCTL. */
      ret = TAPI_Phone_Tone_Play_Unprot (pChannel, &tone,
         IFX_TRUE /* send TONE_END event */);
   }
   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);

   return ret;
}
#endif /* TAPI_FEAT_TONEENGINE */


#ifdef TAPI_FEAT_TONEENGINE
/**
   Plays a tone from the tone table (without protection).

   This function is to be called only when the channel protection semaphore
   is already held by the caller. Only internal functions may call this.

   \param pChannel      - handle to TAPI_CHANNEL structure
   \param pTone         - module, tone index and channel
   \param bSendEndEvent - generate event IFX_TAPI_EVENT_TONE_GEN_END

   \return
   TAPI_statusOk - plays tone correctly
   TAPI_statusToneIdxOutOfRange - Tone index out of range
   TAPI_statusToneNotAvail - tone not configured
   TAPI_statusLLNotSupp - LL code not available
   TAPI_statusToneNoRes - invalid tone specified to play the tone
*/
IFX_int32_t TAPI_Phone_Tone_Play_Unprot (TAPI_CHANNEL *pChannel,
                                const IFX_TAPI_TONE_PLAY_t *pTone,
                                IFX_boolean_t bSendEndEvent)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;
   IFX_int32_t ret = TAPI_statusOk;
   IFX_int32_t nToneIndex = pTone->index;
   TAPI_TONE_DATA_t *pData;
   COMPLEX_TONE *pToneCoeff;
   IFX_TAPI_TONE_SIMPLE_t *pToneSimple;
   IFX_uint32_t nSimpleToneIndex;
   IFX_TAPI_TONE_RES_t ToneRes;

   memset (&ToneRes, 0, sizeof(IFX_TAPI_TONE_RES_t ));

   /* check tone index parameter */
   if (nToneIndex >= TAPI_MAX_TONE_CODE || nToneIndex < 0)
   {
      /* errmsg: Tone index out of range */
      RETURN_STATUS (TAPI_statusToneIdxOutOfRange, 0);
   }
   /* Use the tone code as an index into internal tone coefficients table */
   pToneCoeff = &(toneTable[nToneIndex]);
   /* Check the kind of tone and get the index of the first simple tone */
   switch (pToneCoeff->type)
   {
   case TAPI_TONE_TYPE_COMP:
      /* For composed tones fetch the first simple tone */
      nSimpleToneIndex = pToneCoeff->tone.composed.tones[0];
      break;
   case TAPI_TONE_TYPE_SIMPLE:
      /* For simple tones the index already points to a simple tone */
      nSimpleToneIndex = (IFX_uint32_t)nToneIndex;
      break;
   case TAPI_TONE_TYPE_NONE:
      RETURN_STATUS (TAPI_statusInvalidToneRes, 0);
   default:
      /* Reference to unconfigured tone code entry */
      RETURN_STATUS (TAPI_statusToneNotAvail, 0);
   }

   /* default for LL-drivers not providing a resource get function */
   ret = TAPI_Tone_ResIdGet (pChannel, pTone, &ToneRes);
   if (!TAPI_SUCCESS (ret))
      return ret;

   if (ToneRes.nResID >= TAPI_TONE_MAXRES)
      RETURN_STATUS (TAPI_statusToneNoRes, 0);

   pData = &pChannel->TapiComplexToneData [ToneRes.nResID];
   if (!((pData->nToneState == TAPI_CT_IDLE) ||
       (pData->nToneState == TAPI_CT_DEACTIVATED)))
   {
      RETURN_STATUS (TAPI_statusTonePlayAct, 0);
   }

   /* preparation for complex tones */
   ComplexTone_Conf (pData, toneTable, pToneCoeff);
   /* retrieve simple tone from internal tone coefficients table */
   pToneSimple = &(toneTable[nSimpleToneIndex].tone.simple);

   if (IFX_TAPI_PtrChk (pDrvCtx->ALM.TG_Play))
   {
      /* Play the first simple tone */
      ret = pDrvCtx->ALM.TG_Play (pChannel->pLLChannel,
                                  ToneRes.nResID, pToneSimple);
#ifdef TAPI_VERSION3
      /*
         start the timer for the first cadence only when it is not a
         continuously played tone (loop = 0 or cadence[1] = 0 or pause > 0)
      */
      if ((ret == TAPI_statusOk) &&
         ((pToneSimple->cadence[1] != 0) ||
         (pToneSimple->pause > 0) ||
         (pToneSimple->loop != 0)))
      {
         /* start the timer for the first cadence of the simple tone. */
         TAPI_SetTime_Timer(pChannel->pToneRes[ToneRes.nResID].Tone_Timer,
                            pToneSimple->cadence[0],
                            IFX_FALSE, IFX_TRUE);
      }
#else
      /* \TODO: re-confirm */

      /* start the timer for the first cadence of the simple tone. */
      TAPI_SetTime_Timer(pChannel->pToneRes[ToneRes.nResID].Tone_Timer,
                           1 /* start immediately */, IFX_FALSE, IFX_TRUE);

#endif /* TAPI_VERSION3 */
   }
   else
   {
      RETURN_STATUS (TAPI_statusLLNotSupp, 0);
   }

   if (!TAPI_SUCCESS(ret))
   {
      /* errmsg: Playing tone in LL driver failed */
      RETURN_STATUS (TAPI_statusTonePlayLLFailed, ret);
   }

   /* store the information */
   pData->sequenceCap = ToneRes.sequenceCap;
   pData->nToneIndex = nToneIndex;
   pData->nToneState = TAPI_CT_ACTIVE;
   pData->module     = pTone->module;
   pData->bSendToneGenEndTapiEvent = bSendEndEvent;

   return TAPI_statusOk;
}
#endif /* TAPI_FEAT_TONEENGINE */


#ifdef TAPI_FEAT_TONEENGINE
/**
   Stops Tone Generation, only in connection with "IFX_TAPI_TONE_STOP"

   \param pChannel     - handle to TAPI_CHANNEL structure
   \param nToneIndex   - tone to be stopped
   \param nDirection   - direction into which tone is played

   \return
   TAPI_statusOk - tone stopped successfully
*/
IFX_int32_t TAPI_Phone_Tone_Stop(TAPI_CHANNEL *pChannel,
                                 IFX_TAPI_TONE_PLAY_t *pTone)
{
   IFX_int32_t ret = TAPI_statusErr;

   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);

   if (pTone->external && pTone->internal)
   {
      /* TAPI_Phone_Tone_Stop_Unprot can't stop both directions at the same
       * time. nDirection not used because external/internal has priority over
       * it for TAPIv4. */
      pTone->external = 0;
      (void) TAPI_Phone_Tone_Stop_Unprot(pChannel, pTone);
      pTone->external = 1;
      pTone->internal = 0;
      ret = TAPI_Phone_Tone_Stop_Unprot(pChannel, pTone);
   }
   else
   {
      ret = TAPI_Phone_Tone_Stop_Unprot(pChannel, pTone);
   }

   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);

   return ret;
}
#endif /* TAPI_FEAT_TONEENGINE */


#ifdef TAPI_FEAT_TONEENGINE
/**
   Stops Tone Generation, only in connection with "IFX_TAPI_TONE_STOP"

   \param pChannel     - handle to TAPI_CHANNEL structure
   \param nToneIndex   - tone to be stopped
   \param nDirection   - direction into which tone is played

   \return
   TAPI_statusOk - tone stopped successfully
   TAPI_statusParam - parameter not allowed

   \remarks No protection against concurrent access. If you need protection
            TAPI_Phone_Tone_Stop must be called.
*/
static IFX_int32_t TAPI_Phone_Tone_Stop_Unprot (TAPI_CHANNEL *pChannel,
                                                const IFX_TAPI_TONE_PLAY_t *pTone)
{
   IFX_TAPI_DRV_CTX_t *pDrvCtx = pChannel->pTapiDevice->pDevDrvCtx;
   IFX_TAPI_TONE_RES_t ToneRes = {0};
   IFX_int32_t ret = TAPI_statusOk;
   IFX_int32_t nToneIndex = pTone->index;

   do
   {
      /* get tone definition */
      TAPI_TONE_DATA_t *pData = &pChannel->TapiComplexToneData[ToneRes.nResID];
      /* stop given tone index played into the given direction
         a direction of TAPI_TONE_DST_DEFAULT means don't care for the direction
         if no index is specified every tone currently played is stopped */
      if ((nToneIndex != 0 && pData->nToneIndex == nToneIndex) ||
          (nToneIndex == 0 && pData->nToneState != TAPI_CT_IDLE))
      {
         if (pData->nToneState == TAPI_CT_IDLE)
            return TAPI_statusOk;
         /* Set tone generation back to initial state */
         pData->nToneState = TAPI_CT_IDLE;

         /* Stop any voice path teardown timer that may be running */
         TAPI_Stop_Timer (pChannel->pToneRes[ToneRes.nResID].Tone_Timer);

         if (IFX_TAPI_PtrChk (pDrvCtx->ALM.TG_Stop))
         {
            ret = pDrvCtx->ALM.TG_Stop(pChannel->pLLChannel, ToneRes.nResID);
         }

         if (!(TAPI_SUCCESS (ret)))
            RETURN_STATUS (TAPI_statusToneStop, ret);

         /* reset index afterwards anyway */
         pData->nToneIndex = 0;
      }
      ToneRes.nResID++;
   }
   while (ToneRes.nResID < TAPI_TONE_MAXRES);

   return (ret);
}
#endif /* TAPI_FEAT_TONEENGINE */


#ifdef TAPI_FEAT_TONEENGINE
/*
 * Tone State machine
 */
/**
   Function called from the teardown voice path timer.
   The timer expiry indicates the following:
     - Stop the voice path recording(teardown) in order for the next
       tone playing sequence step to be executed on that channel
\param
   Timer  - TimerID of timer that exipres
\param
   nArg   - Argument of timer including the VINETIC_CHANNEL structure
           (as integer pointer)
\return
*/
static IFX_void_t Tone_OnTimer(Timer_ID Timer, IFX_ulong_t nArg)
{
   TAPI_TONE_RES_t *pRes = (TAPI_TONE_RES_t*)nArg;
   TAPI_CHANNEL *pChannel = pRes->pTapiCh;
   TAPI_DEV     *pTapiDev = pChannel->pTapiDevice;
   IFX_TAPI_DRV_CTX_t   *pDrvCtx = pTapiDev->pDevDrvCtx;

   IFX_uint32_t toneCode           = 0;
   IFX_TAPI_TONE_SIMPLE_t *pToneSimple;
   TAPI_TONE_DATA_t *pData;

   IFX_uint8_t  nToneStep = 1;
   IFX_int32_t ret = TAPI_statusErr;

   TAPI_UNUSED (Timer);

   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);
   pData = &pChannel->TapiComplexToneData[pRes->nRes];

   /* Ignore timer when state is idle.
      May be caused when timer is processed with lower priority
      than an application that does a tone stop at the same time. */
   if (pData->nToneState == TAPI_CT_IDLE)
   {
      TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);
      return;
   }

   /* check if mailbox supports all commands, otherwise return immediately */
   if (pData->sequenceCap == IFX_TAPI_TONE_RESSEQ_SIMPLE)
   {
      IFX_uint8_t nCmdMbxSize;
      IFX_TAPI_LL_DEV_t *pDev = pTapiDev->pLLDev;

      if (IFX_TAPI_PtrChk (pDrvCtx->GetCmdMbxSize))
      {
         if(((IFX_int32_t)pDrvCtx->GetCmdMbxSize (pDev, &nCmdMbxSize)) ==
               TAPI_statusOk)
         {
            if (nCmdMbxSize < MAX_CMD_WORD)
            {
               TAPI_SetTime_Timer(Timer,
                  1 /* start immediately */, IFX_FALSE, IFX_FALSE);
               goto exit;
            }
         }
      }
   }

   /* Get the simple tone code */
   if (pData->nType == TAPI_TONE_TYPE_SIMPLE)
   {
      toneCode = pData->nSimpleToneCode;
   }
   else if (pData->nType == TAPI_TONE_TYPE_COMP)
   {
      /* Get the simple tone to play within the composed tone */
      IFX_TAPI_TONE_COMPOSED_t *pComposedTone;

      pComposedTone = &toneTable[pData->nComposedToneCode].tone.composed;
      toneCode = pComposedTone->tones[pData->nToneCounter];
   }

   /* Any errors on the simple tone code index is taken care of here */
   if (toneCode >= TAPI_MAX_TONE_CODE || toneCode == 0)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
            ("TAPI ERROR: Repeat complex tone code out of range\n"));
      goto exit;
   }

   /* Retrieve simple tone configuration from internal tone coefficients table */
   pToneSimple = &(toneTable[toneCode].tone.simple);
   pData->nSimpleMaxReps      = pToneSimple->loop;
   pData->nPauseTime          = pToneSimple->pause;
   pData->nSimpleToneCode     = pToneSimple->index;

   if (pData->sequenceCap == IFX_TAPI_TONE_RESSEQ_FREQ)
   {
      /* when the next simple tone in a composed tone must be played, call
         the low level function to set up the tone */
      if ((pData->nToneState == TAPI_CT_IDLE) &&
          (pData->nType == TAPI_TONE_TYPE_COMP))
      {
         /* tone has finished */
         if (IFX_TAPI_PtrChk (pDrvCtx->ALM.TG_Play))
            ret = pDrvCtx->ALM.TG_Play (pChannel->pLLChannel, pRes->nRes,
                                  pToneSimple);
         if (ret != TAPI_statusOk)
         {
            TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("TAPI ERROR: Setup of next simple tone failed\n"));

            goto exit;
         }

         /* store the information */
         pData->nToneIndex = toneCode;
         pData->nToneState = TAPI_CT_ACTIVE;
      }

      /* at the end of the simple tone pause, set back to state active */
      if (pData->nToneState == TAPI_CT_ACTIVE_PAUSE)
         pData->nToneState = TAPI_CT_ACTIVE;

      /* tone has finished */
      if (IFX_TAPI_PtrChk (pDrvCtx->ALM.TG_ToneStep))
      {
         /* important release lock to avoid deadlocks as the TG_Step
            jumps back into TAPI context via the evtDispatch... */
         TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);
         ret = pDrvCtx->ALM.TG_ToneStep (pChannel->pLLChannel,
            pToneSimple, pRes->nRes, &nToneStep);
         TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);
         /* don't modify pData->nToneState anymore, it has been
            handled in the event dispatcher already, i.e. we'll
            ignore the return value of nToneStep (!) */

         if (ret != IFX_SUCCESS)
         {
            goto exit;
         }
         if (
            /* only restart the timer if we are active
               and continuously played tone -
               otherwise the tone has completed already */
            (pData->nToneState == TAPI_CT_ACTIVE) &&
            (pToneSimple->cadence[1] || pToneSimple->loop ||
             (pToneSimple->pause > 0))
            )
         {
            if (nToneStep < 1 || nToneStep > IFX_TAPI_TONE_STEPS_MAX)
            {
               /* For TG a value of 0 is normal here. */
               goto exit;
            }
            /* set tone timer */
            TAPI_SetTime_Timer (Timer, pToneSimple->cadence[nToneStep - 1],
                                 IFX_FALSE, IFX_TRUE);
         }
      }
   }

exit:
   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);
   return;
}
#endif /* TAPI_FEAT_TONEENGINE */


#ifdef TAPI_FEAT_TONEENGINE
/**
   UTG deactivated event handling function. Is called upon event.

   \param pChannel     - handle to TAPI_CHANNEL structure
   \param nResID       - index of resource which is reporting this

   \return none
*/
IFX_void_t TAPI_Tone_Step_Completed (TAPI_CHANNEL *pChannel, IFX_uint8_t nResID)
{
   TAPI_TONE_DATA_t     *pData   = IFX_NULL;
   IFX_TAPI_EVENT_t     tapiEvent;

   if (nResID >= TAPI_TONE_MAXRES)
      return;

   TAPI_OS_MutexGet (&pChannel->semTapiChDataLock);

   /* get pointer to TAPI data storage for this resource  */
   pData = &pChannel->TapiComplexToneData[nResID];

   /* Ignore events when state is not active.
      May be caused by stop after the event was already dispatched. */
   if (pData->nToneState != TAPI_CT_ACTIVE)
   {
      TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);
      return;
   }

   /* Tone generation is in deactivated state */
   pData->nToneState = TAPI_CT_DEACTIVATED;

   /* Set tone generation state */
   Event_SimpleTone(pChannel, nResID);

   if (pData->nType == TAPI_TONE_TYPE_COMP)
   {
      /* This is a composed tone so retrieve the next (if any) simple tone
      sequence to play */
      /* check if no more simple tone is played out. */
      if (pData->nToneState == TAPI_CT_DEACTIVATED)
      {
         /* Init the simple tone repetition counter again */
         pData->nSimpleCurrReps = 0;
         /* Setup the next simple tone to play within the composed tone */
         pData->nToneCounter++;
         /* There are no more simple tones to play within the composed tone,
            apply pause times now */
         if (pData->nToneCounter >= pData->nMaxToneCount)
         {
            /* Init the tone counter again to play repetative composed tones */
            pData->nToneCounter = 0;
            /* Prepare for the for next repetition (if any) of a composed tone */
            Event_ComposedTone(pChannel, nResID);

            if ((pData->nComposedMaxReps != 0) &&
                (pData->nComposedCurrReps >= pData->nComposedMaxReps))
            {
               /* This was the last composed tone sequence to be played */
               /* Set tone generation back to initial state */
               pData->nToneState = TAPI_CT_DEACTIVATED;
            }
         }
         /* Kick off the next simple tone sequence to play */
         else
         {
            if (pData->nPauseTime == 0)
            {
               TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);
               pData->nToneState = TAPI_CT_ACTIVE;
               /* For tone generators make it appear as if a pause was just
                  played. But do not delay as with a real pause. */
               if (pChannel->pTapiDevice->pDevDrvCtx->ALM.TG_Play)
                  pData->nToneState = TAPI_CT_ACTIVE_PAUSE;

               /* No delay - call the timer handler function directly */
               Tone_OnTimer (pChannel->pToneRes[nResID].Tone_Timer,
                  (IFX_uintptr_t)&pChannel->pToneRes [nResID]);
               return;
            }
            else
            {
               pData->nToneState = TAPI_CT_ACTIVE_PAUSE;
               /* Start the voice path teardown timer */
               TAPI_SetTime_Timer (pChannel->pToneRes[nResID].Tone_Timer,
                                pData->nPauseTime, IFX_FALSE, IFX_FALSE);
            }
         }
      }
   }

   if (pData->nToneState == TAPI_CT_DEACTIVATED)
   {
      /* Stop the voice path teardown timer */
      TAPI_Stop_Timer(pChannel->pToneRes[nResID].Tone_Timer);

      pData->nToneState = TAPI_CT_IDLE;
      if (pData->bSendToneGenEndTapiEvent)
      {
         /* issue TAPI event to the application */
         memset(&tapiEvent, 0, sizeof(tapiEvent));
         tapiEvent.id = IFX_TAPI_EVENT_TONE_GEN_END;

         if (IFX_TAPI_MODULE_TYPE_ALM == pData->module)
            TAPI_ALM_EXTERNAL (tapiEvent.data.tone_gen) = 1;
         else
            TAPI_COD_INTERNAL (tapiEvent.data.tone_gen) = 1;

         tapiEvent.module = pData->module;
         tapiEvent.data.tone_gen.index = (IFX_uint32_t)pData->nToneIndex;
         IFX_TAPI_Event_Dispatch(pChannel, &tapiEvent);
      }
   }

   TAPI_OS_MutexRelease (&pChannel->semTapiChDataLock);

   return;
}
#endif /* TAPI_FEAT_TONEENGINE */


#ifdef TAPI_FEAT_TONEENGINE
/**
   Simple tone deactivated event handling function.

   \param  pChannel     Pointer to TAPI_CHANNEL structure.
   \param  nResID       Resource ID of the tone.

   \remarks
   This function is called from interrupt level. Is handles
   tone resource 0 or 1.
*/
static IFX_void_t Event_SimpleTone(TAPI_CHANNEL *pChannel, IFX_uint8_t nResID)
{
   TAPI_TONE_DATA_t *pData;

   /* get context for this resource */
   pData = &pChannel->TapiComplexToneData[nResID];

   /* One simple tone sequence has been applied on this channel */
   pData->nSimpleCurrReps++;

   if (pData->nPauseTime == 0)
   {
      /* Special case: A zero pause time was specified on this channel,
      therefore ignore the repetition count.
      The simple tone sequence must be applied only once.

      For DxT and xT16 (TG) this special treatment is wrong, as we need to
      continue with the next step via Tone_OnTimer...
      */
      if (! pChannel->pTapiDevice->pDevDrvCtx->ALM.TG_Play)
         return;
   }

   /* An infinite tone sequence repetition was requested
      on this channel or there are still simple tone sequences to repeat.
      Repeat tone sequence again after the voice path timer expires.
      Special case for loop = 1, no pause to be played on simple tone
      but on composed tone. Handling of that pause is done on higher
      level */
   if ((pData->nSimpleMaxReps == 0) ||
       (pData->nSimpleCurrReps < pData->nSimpleMaxReps))
   {
      /* Tone generation is in pause state */
      pData->nToneState = TAPI_CT_ACTIVE_PAUSE;
      /* Start the voice path teardown timer */
      TAPI_SetTime_Timer(pChannel->pToneRes[nResID].Tone_Timer,
                         pData->nPauseTime, IFX_FALSE, IFX_FALSE);
   }
}
#endif /* TAPI_FEAT_TONEENGINE */


#ifdef TAPI_FEAT_TONEENGINE
/**
   Composed tone deactivated event handling function.

   \param  pChannel     Pointer to TAPI_CHANNEL structure.
   \param  nResID       Resource ID of the tone.
*/
static IFX_void_t Event_ComposedTone (TAPI_CHANNEL *pChannel,
                                      IFX_uint8_t nResID)
{
   TAPI_TONE_DATA_t *pData;

   pData = &pChannel->TapiComplexToneData[nResID];
   /* One composed tone sequence has been applied on this channel */
   pData->nComposedCurrReps++;
   if (pData->nAlternateVoiceTime == 0)
   {
   /* Special case: A zero alternate voice time was specified on this channel,
     therefore ignore the repetition count.
     The composed tone sequence must be applied only once.   */

      /* Set tone generation back to initial state */
   }
   /* An infinite tone sequence repetition was requested
      on this channel or there are still composed tone sequences to repeat.
      Repeat tone sequence again after the voice path timer expires.
      Consider also the pause time of the last simple tone if any */
   if ((pData->nComposedMaxReps == 0) ||
       (pData->nComposedCurrReps < pData->nComposedMaxReps) ||
       pData->nPauseTime != 0)
   {
      /* Tone generation is in pause state */
      pData->nToneState = TAPI_CT_ACTIVE_PAUSE;
      TAPI_SetTime_Timer(pChannel->pToneRes[nResID].Tone_Timer,
                         pData->nAlternateVoiceTime + pData->nPauseTime,
                         IFX_FALSE, IFX_FALSE);
   }
}
#endif /* TAPI_FEAT_TONEENGINE */


TAPI_CMPLX_TONE_STATE_t TAPI_ToneState (TAPI_CHANNEL *pChannel, IFX_uint8_t nResId)
{
   if (!IFX_TAPI_PtrChk (pChannel) || nResId >= TAPI_TONE_MAXRES )
      return TAPI_CT_IDLE;

#ifdef TAPI_FEAT_TONEENGINE
   return pChannel->TapiComplexToneData[nResId].nToneState;
#else
   return TAPI_CT_DEACTIVATED;
#endif /* TAPI_FEAT_TONEENGINE */
}


#ifdef TAPI_VERSION3
/* Note: this function shall be replaced by TAPI_Tone_ResIdGet */
IFX_void_t TAPI_Tone_Set_Source (TAPI_CHANNEL *pChannel, IFX_uint8_t nResId,
                                 IFX_TAPI_TONE_RESSEQ_t src)
{

   if (!IFX_TAPI_PtrChk (pChannel) || nResId >= TAPI_TONE_MAXRES)
      return;

#ifdef TAPI_FEAT_TONEENGINE
   if (src < IFX_TAPI_TONE_RESSEQ_FREQ || src > IFX_TAPI_TONE_RESSEQ_SIMPLE)
      return;
   pChannel->TapiComplexToneData[nResId].sequenceCap = src;
#else
   TAPI_UNUSED (pChannel);
   TAPI_UNUSED (nResId);
   TAPI_UNUSED (src);
#endif /* TAPI_FEAT_TONEENGINE */
}
#endif /* TAPI_VERSION3 */


#ifdef TAPI_FEAT_TONETABLE
/**
   Gets the duration it will take to play a specified tone.

   This function calculates the duration that the given tone will play and
   returns it to the caller.

   \param   nToneIndex  Tone table index of the tone to be calculated.

   \return  Duration in ms.
            The value 0 is special and indicates an infinite sequence.
*/
IFX_uint32_t IFX_TAPI_Tone_DurationGet(IFX_uint32_t nToneIndex)
{
   IFX_uint32_t         nSimpleToneIndex;
   COMPLEX_TONE         *pToneCoeff;
   IFX_uint16_t         nSimpleStep,
                        nComplexStep = 0;
   IFX_uint32_t         nTotalDuration = 0;
   IFX_boolean_t        nMoreSimpleTones = IFX_TRUE;

   /* check tone index parameter */
   if (nToneIndex >= TAPI_MAX_TONE_CODE)
   {
      return 0;
   }

   /* Use the tone code as an index into the internal tone coefficients table */
   pToneCoeff = &(toneTable[nToneIndex]);

   while (nMoreSimpleTones == IFX_TRUE)
   {
      IFX_TAPI_TONE_SIMPLE_t *pToneSimple;
      IFX_uint32_t nSimpleDuration = 0;

      switch (pToneCoeff->type)
      {
      case TAPI_TONE_TYPE_COMP:
         /* For composed tones fetch the next simple tone */
         nSimpleToneIndex = pToneCoeff->tone.composed.tones[nComplexStep];
         nComplexStep++;
         if ((nComplexStep >= IFX_TAPI_TONE_SIMPLE_MAX) ||
             (nComplexStep >= pToneCoeff->tone.composed.count))
         {
            nMoreSimpleTones = IFX_FALSE;
         }
         break;
      case TAPI_TONE_TYPE_SIMPLE:
         /* For simple tones the index already points to a simple tone */
         nSimpleToneIndex = nToneIndex;
         nMoreSimpleTones = IFX_FALSE;
         break;
      default:
         /* Reference to unconfigured tone code entry */
         return 0;
      }

      /* retrieve simple tone from internal tone coefficients table */
      pToneSimple = &(toneTable[nSimpleToneIndex].tone.simple);

      if (pToneSimple->loop == 0)
      {
         /* a loop of 0 defines an infinite repetition -> duration infinite */
         return 0;
      }

      /* Calculate the duration of this simple tone*/
      /* Duration is the sum of all cadences plus the pause multiplied by
         the loop factor.*/
      for (nSimpleStep = 0; (nSimpleStep < IFX_TAPI_TONE_STEPS_MAX) &&
                            (pToneSimple->cadence[nSimpleStep] != 0); nSimpleStep++)
      {
         nSimpleDuration += pToneSimple->cadence[nSimpleStep];
      }
      nSimpleDuration += pToneSimple->pause;
      nSimpleDuration *= pToneSimple->loop;
      /* Add the partial result after multiplication to the total result */
      nTotalDuration  += nSimpleDuration;
   }

   if (pToneCoeff->type == TAPI_TONE_TYPE_COMP)
   {
      /* a loop of 0 defines an infinite repetition -> duration infinite */
      nTotalDuration  *= pToneCoeff->tone.composed.loop;
   }

   return nTotalDuration;
}
#endif /* TAPI_FEAT_TONETABLE */
