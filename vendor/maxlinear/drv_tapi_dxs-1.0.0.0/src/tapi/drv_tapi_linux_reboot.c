#ifdef LINUX
#ifdef __KERNEL__

#include <drv_tapi_osmap.h>

#include <ifx_types.h>

#include <linux/reboot.h>
#include <linux/notifier.h>

/* ========================================================================== */
/*                           Function prototypes                              */
/* ========================================================================== */

static int dxs_reboot_notifier(struct notifier_block *self,
                               unsigned long event, void *data);

/* ========================================================================== */
/*                             Local variables                                */
/* ========================================================================== */

static IFX_boolean_t reboot_ongoing = IFX_FALSE;

static struct notifier_block TAPI_DXS_reboot_notifier = {
   .notifier_call = dxs_reboot_notifier
};

/* ========================================================================== */
/*                         Function implementation                            */
/* ========================================================================== */

/**
   Reboot notification callback.

   \param   self   pointer to notifier_block structure
   \param   event
   \param   data
   \return  NOTIFY_DONE
   \todo    Adapt to different kernel versions.
*/
static int dxs_reboot_notifier(struct notifier_block *self,
                               unsigned long event, void *data)
{
   reboot_ongoing = IFX_TRUE;
   printk(KERN_INFO "TAPIdxs reboot notifier called\n");
   return NOTIFY_DONE;
}


/**
   Register a callback for reboot notification.

   \param   pointer to notifier_block structure
   \return  none
   \todo    Adapt to different kernel versions.
*/
IFX_void_t TAPI_OS_RebootNotifierRegister()
{
   register_reboot_notifier(&TAPI_DXS_reboot_notifier);
}


/**
   Unregister a callback for reboot notification.

   \param   pointer to notifier_block structure
   \return  none
   \todo    Adapt to different kernel versions.
*/
IFX_void_t TAPI_OS_RebootNotifierUnRegister()
{
   unregister_reboot_notifier(&TAPI_DXS_reboot_notifier);
}


/**
   Check for ongoing reboot.

   \return
      IFX_TRUE  if reboot was triggered
      IFX_FALSE if not
*/
IFX_boolean_t TAPI_OS_isRebootOngoing()
{
   return reboot_ongoing;
}

#endif /* __KERNEL__ */
#endif /* LINUX */
