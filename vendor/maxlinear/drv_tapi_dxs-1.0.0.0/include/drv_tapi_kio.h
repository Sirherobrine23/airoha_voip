#ifndef _IFX_TAPI_KERNEL_API_H_
#define _IFX_TAPI_KERNEL_API_H_

/******************************************************************************

  Copyright 2014 Lantiq Deutschland GmbH
  Copyright 2023 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#ifdef __cplusplus
   extern "C" {
#endif

/* Group definitions for Doxygen */
/** \defgroup ETHSW_KERNELAPI TAPI Linux Kernel Interface
    This chapter describes the entire interface to access and
    configure the services of the TAPI module
    within the Linux kernel space. */

/**@{*/

/** Definition of the device handle that is retrieved during
    the \ref ifx_tapi_kopen call. This handle is used to access the
    device while calling \ref ifx_tapi_kioctl. */
typedef void* IFX_TAPI_KIO_HANDLE;

/** Definition for an invalid device handle, returned by
    \ifx_tapi_kopen in case of failure. */
#define IFX_TAPI_KIO_FD_INVALID     0

/**
   Request a device handle for a dedicated device. The
   device is identified by the given device name (e.g.
   "dxs10"). The device name follows the same naming convention
   as given by the Linux file system device node.
   The device handle is the return value of this function. This
   handle is used to access the device parameter and features
   while calling \ref ifx_tapi_kioctl. Please call the function
   \ref ifx_tapi_kclose to release a device handle that is not needed anymore.

   \param name  Pointer to the device name of the requested device.

   \remarks The client kernel module should check the function return value.
   A returned zero indicates that the resource allocation failed.

   \return Return the device handle in case the requested device is available.
   It returns \ref IFX_TAPI_KIO_FD_INVALID in case the device
   does not exist or is blocked by another application.
*/
extern IFX_TAPI_KIO_HANDLE ifx_tapi_kopen(char *name);


/**
   Calls the TAPI HL implementation with the given command and the
   parameter argument. The called device is identified by the
   given device handle. This handle was previously requested by
   calling \ref ifx_tapi_kopen.

   \param handle   device handle, given by \ref ifx_tapi_kopen.
   \param command  TAPI command to perform.
   \param arg      Command arguments. This argument is basically a
                   reference to the command parameter structure.

   \remarks The commands and arguments are the same as normally used over
   the Linux ioctl interface from user space.

   \return Return value as follows:
   - IFX_SUCCESS: if successful
   - An error code in case an error occurred.
*/
extern int ifx_tapi_kioctl(IFX_TAPI_KIO_HANDLE handle,
                           unsigned int command,
                           void *arg);


/**
   Releases a device handle which was previously
   allocated by \ref ifx_tapi_kopen.

   \param handle  device handle, given by \ref ifx_tapi_kopen.

   \return Return value as follows:
   - IFX_SUCCESS: if successful
   - An error code in case an error occurred.
*/
extern int ifx_tapi_kclose(IFX_TAPI_KIO_HANDLE handle);

/**@}*/

#ifdef __cplusplus
   }
#endif

#endif /* _IFX_TAPI_KERNEL_API_H_ */
