#include <ifx_types.h>
#include <drv_tapi_osmap.h>
#include "drv_tapi_ll_interface.h"

/* ============================= */
/* Timer abstraction             */
/* ============================= */
#ifdef TAPI_HAVE_TIMERS

#ifdef __KERNEL__

/* Local functions */
static enum hrtimer_restart TAPI_timer_call_back(struct hrtimer *hr);
static IFX_void_t TAPI_tqueue(struct work_struct *pWork);

/* Local variables */

/* TAPI's timers workqueue */
struct workqueue_struct *pTAPItimersWq;

/* Timer ID */
struct Timer_ID_s
{
   struct work_struct timerTask;
   struct hrtimer Timer_HR;
   IFX_boolean_t bPeriodical;
   IFX_uint32_t Periodical_Time;
   TIMER_ENTRY pTimerEntry;
   IFX_ulong_t nArgument;
   IFX_boolean_t bStopped;
};

#else /* ! __KERNEL__ */

/** List entry */
struct list_entry {
   /** Previous list entry */
   struct list_entry *next;
   /** Next list entry */
   struct list_entry *prev;
};

/** List structure */
struct list {
   /** Used list entries, this first element is empty
       and can be used only to pint to other elements */
   struct list_entry first_element;
   /** Size of list entry data */
   size_t payload_size;
   /** List lock */
   IFXOS_lock_t lock;
};

/** timeout descriptor structure */
struct timeout {
   /** set IFX_TRUE if handler should be called periodically,
       otherwise set IFX_FALSE */
   IFX_boolean_t bPeriodical;
   /** timeout argument */
   unsigned long arg1;
   /** timeout handler */
   TIMER_ENTRY handler;
};

/** timeout list entry */
struct timeout_list_entry {
   /** Time when the timeout becomes active (in milliseconds) */
   time_t timeout_time;
   /** Time to wait from setting the timeout, used for periodic events */
   time_t time_in;
   /** timeout descriptor */
   struct timeout timeout;
};

/** timeout control structure */
struct TAPI_TM_Context {
   /* IFX_TRUE if timers were initialized */
   IFX_boolean_t bTimersInialized;
   /** timeout list */
   struct list timeout_list;
   /** Timeout thread control structure */
   IFXOS_ThreadCtrl_t timeout_thread_ctrl;
};

/** default timeout thread poll time (in milliseconds),
    this is the polling time used to check if new entries/events were added */
#ifndef TIMEOUT_THREAD_POLL_TIME
   #define TIMEOUT_THREAD_POLL_TIME 50 /* [ms] */
#endif

/** get pointer to payload of given entry */
#define list_entry_data(ENTRY) ((void *)((char *)ENTRY + sizeof(struct list_entry)))

/** check if list is empty */
#define is_list_empty(LIST) \
   (((LIST)->first_element.next == &(LIST)->first_element) ? IFX_TRUE : IFX_FALSE)

#define foreach_list_entry_safe_ll(PLIST, ENTRY, NEXT_ENTRY) \
   for ((ENTRY) = (PLIST)->next, (NEXT_ENTRY) = (ENTRY)->next; \
        (ENTRY)->next != (PLIST)->next; \
        (ENTRY) = (NEXT_ENTRY), (NEXT_ENTRY) = (ENTRY)->next)

#define foreach_list_entry(PLIST, ENTRY) \
   for ((ENTRY) = (PLIST)->first_element.next; \
        (ENTRY)->next != (PLIST)->first_element.next; \
        (ENTRY) = (ENTRY)->next)

static IFX_return_t TAPI_TM_list_init(struct list *list, size_t payload_size);
static void TAPI_TM_list_delete(struct list *list);
static void TAPI_TM_list_entry_free(struct list *list,
                                    struct list_entry *entry);
static void TAPI_TM_list_entry_remove(struct list *list, struct list_entry *entry);

static IFX_return_t TAPI_TM_lockless_event_remove(struct TAPI_TM_Context *context,
                                                  struct list_entry *entry_to_remove);
static IFX_return_t TAPI_TM_timeout_event_remove(struct TAPI_TM_Context *context,
                                                 struct list_entry *timer_entry);
static IFX_return_t TAPI_TM_timeout_event_stop(struct TAPI_TM_Context *context,
                                               struct list_entry *timer_entry);
static IFX_return_t TAPI_TM_lockless_event_stop(struct TAPI_TM_Context *context,
                                                struct list_entry *entry_to_stop);
static struct list_entry *TAPI_TM_next_active_event_get(struct TAPI_TM_Context *context,
                                                        struct timeout *timeout,
                                                        time_t *time_to_next_entry);
static IFX_int32_t TAPI_TM_timeout_thread_main(struct IFXOS_ThreadParams_s *thr_params);
static IFX_return_t TAPI_TM_timeout_init(struct TAPI_TM_Context *context);
static struct list_entry *TAPI_TM_event_entry_create(struct TAPI_TM_Context *context,
                                                     const struct timeout *timeout);
static struct list_entry *TAPI_TM_timeout_event_create(struct TAPI_TM_Context *context,
                                                       TIMER_ENTRY handler,
                                                       unsigned long arg1);
static IFX_return_t TAPI_TM_timeout_event_start(struct TAPI_TM_Context *context,
                                                struct list_entry *timer_entry,
                                                time_t timeout_time,
                                                IFX_boolean_t bPeriodical);
static IFX_return_t TAPI_TM_event_entry_start(struct TAPI_TM_Context *context,
                                              struct list_entry *timer_entry,
                                              time_t timeout_time,
                                              IFX_boolean_t bPeriodical);
static IFX_return_t TAPI_TM_lockless_event_entry_start(struct TAPI_TM_Context *context,
                                                       struct list_entry *timer_entry,
                                                       time_t timeout_time);

static void TAPI_TM_list_entry_add_before(struct list *list,
                                          struct list_entry *entry,
                                          struct list_entry *new_entry);
static void TAPI_TM_list_entry_add_tail(struct list *list,
                                        struct list_entry *new_entry);
static void TAPI_TM_list_entry_add_after(struct list *list,
                                         struct list_entry *entry,
                                         struct list_entry *new_entry);


/* Local variables */

/** \todo deleting/clean up of timers is missing for user space, it should
    be called upon closing the tapi thread or even maybe for dev stop */
/** control structure for timers in user's space */
static struct TAPI_TM_Context G_timers;


/**
   Create new list_entry element and assign a callback function to it.

   \param  context      pointer to tapi timer context structure
   \param  handler      pointer to callback function
   \param  arg1         private data for the callback, can be pointer or int

   \return
     - pointer to newly allocated list_entry
     - IFX_NULL in case of error, return value of TAPI_TM_event_entry_create()
*/
static struct list_entry *TAPI_TM_timeout_event_create(struct TAPI_TM_Context *context,
              TIMER_ENTRY handler,
              unsigned long arg1)
{
   struct timeout timeout;

   timeout.handler = handler;
   timeout.arg1 = arg1;
   timeout.bPeriodical = IFX_FALSE;

   return TAPI_TM_event_entry_create(context, &timeout);
}


/**
   Create new list_entry element, allocate memory and copy timeout data.

   \param  context      pointer to tapi timer context structure
   \param  timeout      pointer to timeout stucture

   \return
     - pointer to newly allocated list_entry
     - IFX_NULL in case of error
*/
static struct list_entry *TAPI_TM_event_entry_create(struct TAPI_TM_Context *context,
                   const struct timeout *timeout)
{
   struct list_entry *new_entry;
   struct timeout_list_entry *new_timeout_entry;

   TAPI_OS_LockGet(&context->timeout_list.lock);
   /* allocate memory for list entry with payload */
   new_entry = TAPI_OS_Malloc(sizeof(struct list_entry) + context->timeout_list.payload_size);
   if (!new_entry) {
      TAPI_OS_LockRelease(&context->timeout_list.lock);
      return IFX_NULL;
   }

   new_timeout_entry = list_entry_data(new_entry);

   memcpy(&new_timeout_entry->timeout, timeout,
          sizeof(struct timeout));

   TAPI_OS_LockRelease(&context->timeout_list.lock);

   return new_entry;
}


/**
   Initialize TAPI_TM_Context structure and its list and start timeout control
   thread.

   \param  context      pointer to tapi timer context structure

   \return
     - IFX_SUCCESS or IFX_ERROR in case of errors
*/
static IFX_return_t TAPI_TM_timeout_init(struct TAPI_TM_Context *context)
{
   IFX_return_t error;

   error = TAPI_TM_list_init(&context->timeout_list, sizeof(struct timeout_list_entry));

   if (error)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
           ("TAPI DXS: TAPI_TM_list_init() failed for timer Initialization\n"));
      return error;
   }

   error = (IFX_SUCCESS == TAPI_OS_ThreadInit(&context->timeout_thread_ctrl,
                 "tapidxstm",
                 TAPI_TM_timeout_thread_main,
                 IFXOS_DEFAULT_STACK_SIZE,
                 TAPI_OS_THREAD_PRIO_HIGHEST,
                 (unsigned long)context,
                 0)) ? IFX_SUCCESS : IFX_ERROR;

   if (error)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
           ("TAPI DXS: TAPI_OS_ThreadInit() failed for timer Initialization\n"));
      TAPI_TM_list_delete(&context->timeout_list);
      return error;
   }
   return IFX_SUCCESS;
}


/**
   Release memory and other resources used.

   \param  list         pointer to list

*/
static void TAPI_TM_list_delete(struct list *list)
{
   struct list_entry *entry, *tmp_entry;

   foreach_list_entry_safe_ll(&list->first_element, entry, tmp_entry)
   {
      TAPI_OS_Free(entry);
   }
   /* if list is empty fileds next and prev should point first_element */
   list->first_element.next = &list->first_element;
   list->first_element.prev = &list->first_element;
   (void)TAPI_OS_LockDelete(&list->lock);
}


/** timeout events handling thread

   \param[in] thr_params Thread arguments

   \return 0
*/
static IFX_int32_t TAPI_TM_timeout_thread_main(struct IFXOS_ThreadParams_s *thr_params)
{
   struct TAPI_TM_Context *context = (struct TAPI_TM_Context *)thr_params->nArg1;
   struct timeout timer = {0};
   struct list_entry *timer_to_execute_entry;
   time_t wait_time;

   /* while thread is running */
   while (thr_params->bRunning == IFX_TRUE &&
          thr_params->bShutDown == IFX_FALSE)
   {
      /* wait for message in FIFO */
      while (1)
      {
         wait_time = 0;

         TAPI_OS_LockGet(&context->timeout_list.lock);
         timer_to_execute_entry = TAPI_TM_next_active_event_get(context, &timer,
                                     &wait_time);
         TAPI_OS_LockRelease(&context->timeout_list.lock);

         if (timer_to_execute_entry == IFX_NULL &&
             thr_params->bShutDown == IFX_FALSE &&
             thr_params->bRunning == IFX_TRUE)
         {
            /* if time to the next timeout is longer than the poll time,
               then wait only TIMEOUT_THREAD_POLL_TIME ms */
            if ((wait_time == 0) || (wait_time > TIMEOUT_THREAD_POLL_TIME))
            {
               wait_time = TIMEOUT_THREAD_POLL_TIME;
            }

            /** \todo replace sleep with select, for this some sync mechanism
               with add/start entry is needed, for example select could wait
               for a pipe fd being readable, each function for add/start entry
               would write to this pipe, this could replace the polling,
               if above is implemented then wait_time could be set to exact
               time to next timeout or wait forever until and event/entry
               is added */
            TAPI_OS_MSecSleep((IFX_time_t) wait_time);
         }
         else
         {
            break;
         }
      }

      /* check if we are shutting down */
      if (thr_params->bShutDown == IFX_TRUE ||
          thr_params->bRunning == IFX_FALSE)
      {
         break;
      }

      TAPI_OS_LockGet(&context->timeout_list.lock);
      {
         struct timeout_list_entry *periodical_timeout_entry = 
            list_entry_data(timer_to_execute_entry);

         (void)TAPI_TM_lockless_event_stop(context, timer_to_execute_entry);

         if (periodical_timeout_entry && timer.bPeriodical)
         {
            /* for periodical events get time to next timeout and add new timeout */
            time_t time_in = periodical_timeout_entry->time_in;
            (void) TAPI_TM_lockless_event_entry_start(context, timer_to_execute_entry,
                                                      time_in);
         }
      }
      TAPI_OS_LockRelease(&context->timeout_list.lock);

      if (timer.handler != NULL)
      {
         (void) timer.handler((Timer_ID) timer_to_execute_entry, (IFX_ulong_t) timer.arg1);
      }
      else
      {
         TRACE(TAPI_DXS, DBG_LEVEL_HIGH,
               ("TAPI_TM_timeout_thread_main - ERROR: found event without timer handler"));
      }
   }
   return 0;
}


/**
   Lockless version of TAPI_TM_timeout_event_remove.

   \param  context         pointer to tapi timer context structure
   \param  entry_to_stop   pointer to tapi entry structure

   \return
     - IFX_SUCCESS
*/
static IFX_return_t TAPI_TM_lockless_event_stop(struct TAPI_TM_Context *context,
                    struct list_entry *entry_to_stop)
{
   struct list_entry *current_entry;

   if (is_list_empty(&context->timeout_list))
   {
      return IFX_SUCCESS;
   }

   foreach_list_entry(&context->timeout_list, current_entry)
   {
      if (current_entry == entry_to_stop)
      {
         TAPI_TM_list_entry_remove(&context->timeout_list, current_entry);
         return IFX_SUCCESS;
      }
   }
   return IFX_SUCCESS;
}


/**
   Remove entry from the list.

   \param  list      pointer to list structure
   \param  entry     pointer to entry structure that will be removed
*/
static void TAPI_TM_list_entry_remove(struct list *list,
             struct list_entry *entry)
{
   entry->next->prev = entry->prev;
   entry->prev->next = entry->next;
   entry->prev = IFX_NULL;
   entry->next = IFX_NULL;
   if (list->first_element.next == entry)
   {
      list->first_element.next = &list->first_element;
   }
}


/** Get next timeouted event

   \param[in]  context              timer context pointer
   \param[out] timeout              Returns timeout descriptor
   \param[out] time_to_next_entry   time till the next timeout expires,
                                    set only if no entry was found

   \return first entry on the list for which timeout expired or IFX_NULL
*/
static struct list_entry *TAPI_TM_next_active_event_get(struct TAPI_TM_Context *context,
                    struct timeout *timeout,
                    time_t * time_to_next_entry)
{
   struct timeout_list_entry *first_entry;
   time_t currentTime;

   if (is_list_empty(&context->timeout_list))
   {
      return IFX_NULL;
   }

   /* get the first entry from the list,
      should be the one for which timeout expires first */
   first_entry = list_entry_data(context->timeout_list.first_element.next);
   if (!first_entry)
   {
      return IFX_NULL;
   }

   currentTime = (time_t)IFXOS_ElapsedTimeMSecGet(0);
   if (first_entry->timeout_time <= currentTime)
   {
      /* timeout occurred for this entry */
      *timeout = first_entry->timeout;
      return context->timeout_list.first_element.next;
   }
   else
   {
      /* get time to next timeout */
      *time_to_next_entry = first_entry->timeout_time - currentTime;
   }
   return IFX_NULL;
}


/** Stop handling of given entry

   \param[in]  context        timer context pointer
   \param[in]  timer_entry    timer entry to stop

   \return always IFX_SUCCESS as TAPI_TM_lockless_event_stop does
*/
static IFX_return_t TAPI_TM_timeout_event_stop(struct TAPI_TM_Context *context,
                 struct list_entry *timer_entry)
{
   TAPI_OS_LockGet(&context->timeout_list.lock);
   struct timeout_list_entry *timeout_entry_to_start = list_entry_data(timer_entry);
   timeout_entry_to_start->timeout.bPeriodical = IFX_FALSE;
   IFX_return_t error = TAPI_TM_lockless_event_stop(context, timer_entry);
   TAPI_OS_LockRelease(&context->timeout_list.lock);

   return error;
}


/** Remove given entry from the list

   \param[in]  context        timer context pointer
   \param[in]  timer_entry    timer entry to remove

   \return always IFX_SUCCESS as TAPI_TM_lockless_event_remove does
*/
static IFX_return_t TAPI_TM_timeout_event_remove(struct TAPI_TM_Context *context,
                 struct list_entry *timer_entry)
{
   IFX_return_t error;

   TAPI_OS_LockGet(&context->timeout_list.lock);
   error = TAPI_TM_lockless_event_remove(context, timer_entry);
   TAPI_OS_LockRelease(&context->timeout_list.lock);

   return error;
}


/** Lockless version of TAPI_TM_timeout_event_remove

   \param[in]  context           timer context pointer
   \param[in]  entry_to_remove   entry to remove

   \return always IFX_SUCCESS
 */
static IFX_return_t TAPI_TM_lockless_event_remove(struct TAPI_TM_Context *context,
                    struct list_entry *entry_to_remove)
{
   struct list_entry *current_entry;

   if (is_list_empty(&context->timeout_list))
   {
      return IFX_SUCCESS;
   }
   foreach_list_entry(&context->timeout_list, current_entry)
   {
      if (current_entry == entry_to_remove)
      {
         TAPI_TM_list_entry_free(&context->timeout_list, current_entry);
         return IFX_SUCCESS;
      }
   }
   return IFX_SUCCESS;
}


/** Release memory allocated for given entry

   \param[in]  list     pointer to the list - unused
   \param[in]  entry    entry to remove and free memory
 */
static void TAPI_TM_list_entry_free(struct list *list,
             struct list_entry *entry)
{
   TAPI_UNUSED(list);

   if ((IFX_NULL != entry->next) && (IFX_NULL != entry->prev))
   {
      entry->next->prev = entry->prev;
      entry->prev->next = entry->next;
   }
   TAPI_OS_Free(entry);
}


/** Release memory allocated for given entry

   \param[in]  list           pointer to the list
   \param[in]  payload_size   size of memory in bytes needed to hold the entry data

   \return value TAPI_OS_LockInit() call
 */
static IFX_return_t TAPI_TM_list_init(struct list *list, size_t payload_size)
{
   /* for first element next and prev point to first element if list is empty */
   list->first_element.next = &list->first_element;
   list->first_element.prev = &list->first_element;
   list->payload_size = payload_size;
   /* get the lock */
   return TAPI_OS_LockInit(&list->lock);
}


/** Start handling of given timeout entry

   \param[in]  context        timer context pointer
   \param[in]  timer_entry    timer entry to start
   \param[in]  timeout_time   timeout time for new timer entry
   \param[in]  bPeriodical    if IFX_TRUE, then timeout for event will be
                              checked periodically to execute the handler

   \return always IFX_SUCCESS as TAPI_TM_event_entry_start does
*/
static IFX_return_t TAPI_TM_timeout_event_start(struct TAPI_TM_Context *context,
              struct list_entry *timer_entry,
              time_t timeout_time, IFX_boolean_t bPeriodical)
{
   /** \todo add a check if timers are initialized */
   return TAPI_TM_event_entry_start(context, timer_entry, timeout_time, bPeriodical);
}


/** Start handling of given timeout entry

   \param[in]  context        timer context pointer
   \param[in]  timer_entry    timer entry to start
   \param[in]  timeout_time   timeout time for new timer entry
   \param[in]  bPeriodical    if IFX_TRUE, then timeout for event will be
                              checked periodically to execute the handler

   \return always IFX_SUCCESS
*/
static IFX_return_t TAPI_TM_event_entry_start(struct TAPI_TM_Context *context,
                   struct list_entry *timer_entry,
                   time_t timeout_time,
                   IFX_boolean_t bPeriodical)
{
   struct timeout_list_entry *timeout_entry_to_start;

   TAPI_OS_LockGet(&context->timeout_list.lock);
   timeout_entry_to_start = list_entry_data(timer_entry);
   timeout_entry_to_start->timeout.bPeriodical = bPeriodical;
   TAPI_TM_lockless_event_entry_start(context, timer_entry, timeout_time);
   TAPI_OS_LockRelease(&context->timeout_list.lock);

   return IFX_SUCCESS;
}


/** Add timeout event

   \param[in]  context        timer context pointer
   \param[in]  timer_entry    timer entry to start
   \param[in]  timeout_time   timeout time for new timer entry (in ms)

   \return always IFX_SUCCESS
*/
static IFX_return_t TAPI_TM_lockless_event_entry_start(struct TAPI_TM_Context *context,
                   struct list_entry *timer_entry,
                   time_t timeout_time)
{
   struct list_entry *current_entry;
   IFX_boolean_t added = IFX_FALSE;

   struct timeout_list_entry *timeout_entry_to_start = list_entry_data(timer_entry);
   /* get timout time to compare with current time read
      with IFXOS_ElapsedTimeMSecGet(0) */
   timeout_entry_to_start->timeout_time = IFXOS_ElapsedTimeMSecGet(0) + timeout_time;
   timeout_entry_to_start->time_in = timeout_time;
   foreach_list_entry(&context->timeout_list, current_entry)
   {
      struct timeout_list_entry *timeout_entry = list_entry_data(current_entry);
      if (timeout_entry->timeout_time >
          timeout_entry_to_start->timeout_time)
      {
         TAPI_TM_list_entry_add_before(&context->timeout_list, current_entry,
                     timer_entry);
         added = IFX_TRUE;
         break;
      }
   }

   if (!added)
   {
      TAPI_TM_list_entry_add_tail(&context->timeout_list, timer_entry);
   }
   return IFX_SUCCESS;
}


/** Add add new entry before entry that is already on the list

   \param[in]  list           the list - unused
   \param[in]  entry          entry from the list
   \param[in]  new_entry      entry to add
*/
static void TAPI_TM_list_entry_add_before(struct list *list,
            struct list_entry *entry,
            struct list_entry *new_entry)
{
   TAPI_UNUSED(list);

   entry->prev->next = new_entry;
   new_entry->prev = entry->prev;
   new_entry->next = entry;
   entry->prev = new_entry;
}


/** Add add new entry at the end of the list

   \param[in]  list           the list
   \param[in]  entry          entry to add
*/
static void TAPI_TM_list_entry_add_tail(struct list *list,
                   struct list_entry *new_entry)
{
   TAPI_TM_list_entry_add_after(list, list->first_element.prev, new_entry);
}


/** Add add new entry after entry that is already on the list

   \param[in]  list           the list - unused
   \param[in]  entry          entry from the list
   \param[in]  new_entry      entry to add
*/
static void TAPI_TM_list_entry_add_after(struct list *list,
           struct list_entry *entry,
           struct list_entry *new_entry)
{
   TAPI_UNUSED(list);

   entry->next->prev = new_entry;
   new_entry->next = entry->next;
   entry->next = new_entry;
   new_entry->prev = entry;
}

#endif /* __KERNEL__ */


/**
   Create a timer.

   \param  pTimerEntry  Function pointer to the call back function.
   \param  nArgument    Pointer to TAPI channel structure.

   \return
   Timer_ID  Pointer to internal timer structure.

   \remarks
   Initialize a task queue which will be scheduled once a timer interrupt occurs
   to execute the appropriate operation in a process context, process in which
   semaphores ... are allowed.
   Please notice that this task has to run under the keventd process, in which
   it can be executed thousands of times within a single timer tick.
*/
Timer_ID TAPI_Create_Timer(TIMER_ENTRY pTimerEntry, IFX_ulong_t nArgument)
{
#ifdef __KERNEL__
   /* allocate memory for the timer data structure */
   struct Timer_ID_s *pTimerData = kmalloc(sizeof (*pTimerData), GFP_KERNEL);
   if (pTimerData == IFX_NULL)
      return IFX_NULL;

   /* set function to be called after timer expires */
   pTimerData->pTimerEntry = pTimerEntry;
   pTimerData->nArgument = nArgument;
   pTimerData->bStopped = IFX_FALSE;
   hrtimer_setup(&pTimerData->Timer_HR, TAPI_timer_call_back, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

   /* Initialize Timer Task */
   INIT_WORK(&(pTimerData->timerTask), TAPI_tqueue);

   return (Timer_ID)pTimerData;

#else /* ! __KERNEL__ */
   if (IFX_TRUE != G_timers.bTimersInialized)
   {
      /* timeout thread needs to be started only once */
      G_timers.bTimersInialized = IFX_TRUE;
      TAPI_TM_timeout_init(&G_timers);
   }
   return (Timer_ID) TAPI_TM_timeout_event_create(&G_timers, pTimerEntry, nArgument);
#endif /* __KERNEL__ */
}


/**
   Sets a timer to the specified time and starts it. It can be choose if the
   timer starts periodically.

   \param  Timer_ID     Pointer to internal timer structure.
   \param  nTime        Time in ms.
   \param  bPeriodically Starts the timer periodically or not.
   \param  bRestart     Restart the timer or normal start.

   \return
   Returns an error code: IFX_TRUE / IFX_FALSE
*/
IFX_boolean_t TAPI_SetTime_Timer(Timer_ID Timer,
                                 IFX_uint32_t nTime,
                                 IFX_boolean_t bPeriodically,
                                 IFX_boolean_t bRestart)
{
#ifdef __KERNEL__
   struct Timer_ID_s *pTimerData = (struct Timer_ID_s *)Timer;

   if (pTimerData == IFX_NULL || nTime  > UINT_MAX / HZ)
      return IFX_FALSE;

   pTimerData->Periodical_Time = nTime;
   pTimerData->bPeriodical = bPeriodically;
   pTimerData->bStopped = IFX_FALSE;

   if (hrtimer_is_queued(&(pTimerData->Timer_HR)))
   {
      if (IFX_FALSE == bRestart)
         return IFX_TRUE;

      hrtimer_cancel(&(pTimerData->Timer_HR));
   }

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0))
   hrtimer_start(&pTimerData->Timer_HR,
                 ns_to_ktime(pTimerData->Periodical_Time * NSEC_PER_MSEC),
                 HRTIMER_MODE_REL);
#else
   hrtimer_start(&pTimerData->Timer_HR,
                 ms_to_ktime(pTimerData->Periodical_Time),
                 HRTIMER_MODE_REL);
#endif

#else /* !__KERNEL__ */
   /** \todo add a check if timers are initialized */
   if (bRestart == IFX_TRUE)
   {
      (void) TAPI_TM_timeout_event_stop(&G_timers, (struct list_entry *) Timer);
   }

   if (TAPI_TM_timeout_event_start(&G_timers,
              (struct list_entry *) Timer,
              (time_t) nTime,
              bPeriodically) != 0)
   {
      TRACE(TAPI_DXS, DBG_LEVEL_HIGH, ("TAPI_SetTime_Timer: failed to start timer\n"));
      return IFX_FALSE;
   }
#endif /* __KERNEL__ */

   return IFX_TRUE;
}


/**
   Stop a timer.

   \param Timer_ID Pointer to internal timer structure.

   \return Returns an error code: IFX_TRUE / IFX_FALSE
*/
IFX_boolean_t TAPI_Stop_Timer(Timer_ID Timer)
{
#ifdef __KERNEL__
   struct Timer_ID_s *pTimerData;

   if (Timer == IFX_NULL)
      return IFX_FALSE;

   pTimerData = (struct Timer_ID_s *)Timer;

   /* stop timer */
   pTimerData->bStopped = IFX_TRUE;
   /* prevent restart of driver */
   pTimerData->bPeriodical = IFX_FALSE;

   hrtimer_cancel(&(pTimerData->Timer_HR));

#else /* ! __KERNEL__ */
   if (Timer)
      (void) TAPI_TM_timeout_event_stop(&G_timers, (struct list_entry *) Timer);
   else
      return IFX_FALSE;
#endif /* __KERNEL__ */

   return IFX_TRUE;
}


/**
   Delete a timer.

   \param Timer_ID  Pointer to internal timer structure.

   \return
   Returns an error code: IFX_TRUE / IFX_FALSE
*/
IFX_boolean_t TAPI_Delete_Timer(Timer_ID Timer)
{
   if (Timer == IFX_NULL)
      return IFX_FALSE;

   TAPI_Stop_Timer(Timer);

   /* free memory */
#ifdef __KERNEL__
   kfree(Timer);
#else
   (void) TAPI_TM_timeout_event_remove(&G_timers, (struct list_entry *)Timer);
#endif /* __KERNEL__ */

   return IFX_TRUE;
}


/**
   Helper function to get a periodical timer.

   \param pWork Pointer to corresponding timer ID.

   \remarks
   This function will be executed in the process context, so to avoid
   scheduling in Interrupt Mode while working with semaphores etc...
   The task is always running under the keventd process and is also running
   very quickly. Even on a very heavily loaded system, the latency in the
   scheduler queue is quite small
*/
#ifdef __KERNEL__

static IFX_void_t TAPI_tqueue(struct work_struct *pWork)
{
   struct Timer_ID_s *pTimerData = (struct Timer_ID_s *)pWork;

   if (pTimerData->bStopped)
      return;

   /* Call TAPI Timer function */
   pTimerData->pTimerEntry(pTimerData, pTimerData->nArgument);

   if (pTimerData->bPeriodical)
   {
      if (hrtimer_is_queued(&(pTimerData->Timer_HR)))
         hrtimer_cancel(&(pTimerData->Timer_HR));

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0))
   hrtimer_start(&pTimerData->Timer_HR,
                 ns_to_ktime(pTimerData->Periodical_Time * NSEC_PER_MSEC),
                 HRTIMER_MODE_REL);
#else
   hrtimer_start(&pTimerData->Timer_HR,
                 ms_to_ktime(pTimerData->Periodical_Time),
                 HRTIMER_MODE_REL);
#endif
   }
}


/**
   Helper function to get a periodical timer.

   \param arg  Pointer to corresponding timer ID.
*/
static enum hrtimer_restart TAPI_timer_call_back(struct hrtimer *hr)
{
   struct Timer_ID_s *pTimerData = container_of(hr, struct Timer_ID_s, Timer_HR);

   queue_work(pTAPItimersWq, &(pTimerData->timerTask));
   return HRTIMER_NORESTART;
}
#endif /* __KERNEL__ */

#endif /* TAPI_HAVE_TIMERS */
