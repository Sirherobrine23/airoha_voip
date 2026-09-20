#
#  Copyright 2014-2015 Lantiq Deutschland GmbH
#  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
#  Copyright 2016-2020 Intel Corporation.
#  Copyright 2020-2024 MaxLinear, Inc.
#
#  For licensing information, see the file 'LICENSE' in the root folder of
#  this software module.
#

AC_PREREQ(2.59)
AC_COPYRIGHT([Copyright 2024, MaxLinear, Inc.
For licensing information please see the file 'LICENSE' in the root folder of
this software module.])

AC_INIT([Standalone TAPI driver for DXS],[1.0.0.0],[MaxLinear, Inc.],[drv_tapi_dxs])
AC_CONFIG_SRCDIR(src/Makefile.am)

AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wno-portability tar-pax subdir-objects])

# save the configure arguments
# remove the install dir from the distcheck configure line to avoid that the
# installed binaries are deleted on distcheck. The regex will remove
# --prefix and all subsequent "non-space" chars from the configure options
# as well as the internal features.
# Note: The escaped \' are important.
CONFIGURE_OPTIONS=''
for c_arg in $ac_configure_args; do
case $c_arg in
\'--prefix=* | \'--enable-el_debug\' | \'--enable-eval\')
   #echo "skipping configure option for distcheck: $c_arg"
   ;;
*)
   CONFIGURE_OPTIONS="$CONFIGURE_OPTIONS $c_arg"
   ;;
esac
done
AC_SUBST(CONFIGURE_OPTIONS)

AC_CONFIG_HEADER([include/drv_config_defines.h include/drv_config_tapi_compatibility.h])

dnl enable debugging mode
DEBUG_CHECK

if [[ "$CFLAGS" == "" ]] ; then
   CFLAGS="-O2"
fi

AC_PROG_CC
AC_PROG_RANLIB

AH_TOP(
[/******************************************************************************

  Copyright 2023-2024 MaxLinear, Inc.

  For licensing information, see the file 'LICENSE' in the root folder of
  this software module.

******************************************************************************/

#ifndef _DRV_TAPI_DXS_CONFIG_DEFINES_H_
#define _DRV_TAPI_DXS_CONFIG_DEFINES_H_]
)

AH_BOTTOM([
#endif /* _DRV_TAPI_DXS_CONFIG_DEFINES_H_ */
])

AC_DEFINE_UNQUOTED([DRV_TAPI_CONFIGURE_STR], ["$ac_configure_args"], [configure options string])

## Target host specific compilation flags
case "${host}" in
   "")
      SYS=unknown
      ;;
   aarch64-*linux*)
      AC_SUBST([CROSS_COMPILE],[${host}-])
      SYS=linux
      ;;
   arm-*linux*)
      AC_SUBST([CROSS_COMPILE],[${host}-])
      SYS=linux
      ;;
   mips*-linux*)
      AC_SUBST([CROSS_COMPILE],[${host}-])
      SYS=linux
      ;;
   powerpc*-linux*)
      AC_SUBST([CROSS_COMPILE],[${host}-])
      SYS=linux
      ;;
   ppc_6xx)
      AC_SUBST([CROSS_COMPILE],[ppc_6xx-])
      SYS=linux
      ;;
   powerpc-linux)
      AC_SUBST([CROSS_COMPILE],[powerpc-linux-])
      SYS=linux
      ;;
   i[[3456789]]86-*-linux*)
      SYS=linux
      ;;
   i[[3456789]]86-*-mingw32*)
      SYS=mingw32
      ;;
   x86_*-linux*)
      SYS=linux
      ;;
   *)
      SYS="${host_os}"
      ;;
esac

AM_CONDITIONAL(LINUX, test "${SYS}" = "linux")
AM_CONDITIONAL(WIN32, test "${SYS}" = "mingw32")
AM_CONDITIONAL(BUILD_SHARED_LIB, test "${SYS}" = "mingw32")

AS_IF(
   [test "$SYS" = linux],
   [
      dnl check for Linux kernel support
      dnl Build of the Kernel modules under LINUX

      PROC_CHECK(TAPI_USE_PROC)
      LINUX_SMP_CHECK
   ],
   [AM_CONDITIONAL(WITH_KERNEL_MODULE, false)]
)

dnl Set project base directory path
DEFAULT_PRJ_BASE_PATH=''
AC_ARG_ENABLE(prjbase,
   AS_HELP_STRING(
      [--enable-prjbase=x],
      [set the project base directory path (default is ~/project)]),
   [
   if test -n $enableval; then
      AC_MSG_RESULT([set the project base directory path $enableval])
      DEFAULT_PRJ_BASE_PATH=$enableval
      AC_SUBST([PRJ_BASE_PATH],[$DEFAULT_PRJ_BASE_PATH])
   else
      AC_MSG_RESULT([Set the default project base directory path $DEFAULT_PRJ_BASE_PATH])
      AC_SUBST([PRJ_BASE_PATH],[$DEFAULT_PRJ_BASE_PATH])
   fi
   ],
   [AC_SUBST([PRJ_BASE_PATH],[$DEFAULT_PRJ_BASE_PATH])]
)

dnl Check if IFXOS should be used
IFXOS_ENABLE(
   [disable],
   [LIBIFXOS_CHECK_CONFIG
    LIBIFXOS_CHECK_CONFIG_INCL]
)


#
# Check TAPI features switches
#

dnl check whether TAPI API is set to V3 or V4
TAPI_VERSION_CHECK([TAPI_VERSION3])

dnl enable Linux kernelspace API support
TAPI_KERNEL_API_CHECK([disable], [TAPI])

dnl enable normal TAPI driver compatibility (keep same ioctl API parameters)
CLASSIC_TAPI_COMPATIBILITY_CHECK([enable])

dnl enable TAPI debug buffer feature (can be used to debug SPI traffic)
TAPI_DEBUG_BUFFER_CHECK(
   [disable],
   [TAPI_DEBUG_BUFFER_NUM_OF_ENTRIES_CHECK])

#
# Check DXS misc settings
#

DXS_DCDC_HW_CHECK

MAX_DEVICES_CHECK(DUSLIC XS, DXS_MAX_DEVICES)

TAPI_LL_DIRECT_CHIP_ACCESS_CHECK([disable], DXS)


#
# Basic POTS features
#

dnl enable TAPI pulse dial/hook support and hook state machine
TAPI_DIAL_CHECK([enable], [TAPI_HSM_CHECK([enable])])

dnl PCM channel support
TAPI_PCM_CHECK([enable])

dnl check TAPI Calibration support
TAPI_CALIBRATION_CHECK([enable])


#
# Extended POTS features
#

dnl check if extended POTS features are enabled as a group
TAPI_EXTENDED_POTS_FEATURES_CHECK(
   [disable],
   [
      TAPI_DTMF_CHECK([enable])
      TAPI_RINGENGINE_CHECK([enable])
      TAPI_TONE_GENERATOR_CHECK([enable], [TAPI])
      TAPI_CID_CHECK([enable])
      TAPI_PHONE_DET_CHECK([enable])
      TAPI_RING_ADAPTIVE_BITTIME_CHECK([enable])
   ],
   [
      AS_ECHO("checking if user set extended POTS feature explicitly...")
      TAPI_DTMF_CHECK([disable])
      TAPI_RINGENGINE_CHECK([disable])
      TAPI_TONE_GENERATOR_CHECK([disable], [TAPI])
      TAPI_CID_CHECK([disable])
      TAPI_PHONE_DET_CHECK([disable])
      TAPI_RING_ADAPTIVE_BITTIME_CHECK([disable])
   ]
)


#
# Linetesting features
#

dnl check if linetesting POTS features are enabled as a group
TAPI_LINETESTING_POTS_FEATURES_CHECK(
   [disable],
   [
      TAPI_CAP_MEASUREMENT_CHECK([enable], TAPI)
      TAPI_NLT_CHECK([enable])
      TAPI_CONT_MEAS_CHECK([enable])
   ],
   [
      AS_ECHO("checking if user set linetesting POTS feature explicitly...")
      TAPI_CAP_MEASUREMENT_CHECK([disable], TAPI)
      TAPI_NLT_CHECK([disable])
      TAPI_CONT_MEAS_CHECK([disable])
   ]
)


#
# Rarely used extended features
#

TAPI_MWL_CHECK([disable])
TAPI_METERING_CHECK([disable])
TAPI_TONE_GENERATOR_LEVEL_COMPENSATION_CHECK([disable], DXS)

#
# Misc compilation checks
#

dnl enable kernel module
WITH_KERNEL_MODULE_CHECK(
   [yes], # default
   [INTERRUPTS_CHECK(
      [enable], # default
      [AC_DEFINE([DXS_HAVE_INTERRUPTS],[1],[enable interrupts])]
   )]
)

dnl Enable debug traces hidden behind ENABLE_TRACE symbol
TRACE_CHECK

dnl set kernel include and build directories
KERNEL_CHECK_CONFIG

dnl enable compiler warnings
WARNINGS_CHECK

dnl enable warnings as error
WERROR_CHECK

dnl check if strict permissions are enabled
TAPI_STRICT_PERMISSIONS_CHECK([disable])

dnl enable event logger debugging
EVENT_LOGGER_CHECK_CONFIG

AS_IF([test "$SYS" = mingw32], [EVENT_LOGGER_LIB_CHECK])

dnl set additional (device specific) CFLAGS
WITH_CFLAGS_CHECK

dnl Check for SPI 8-bit mode
DXS_SPI_8BIT_CHECK

AC_DEFINE_UNQUOTED([WITH_CLFAGS_CONFIGURE_OPTION],
                   [$CFLAGS],
                   [environment and device specific CFLAGS passed to configure])

AS_IF([test -n "$KBUILD_EXTRA_SYMBOLS"],
   [AC_SUBST([KBUILD_EXTRA_SYMBOLS],[$KBUILD_EXTRA_SYMBOLS])])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])

AC_OUTPUT

AS_ECHO("----------------------------------------------------------------------
Standalone TAPI for DXS configuration:
   Source code location:          ${srcdir}
   Cross compile:                 ${CROSS_COMPILE}
   Compiler:                      ${CC}
   Compiler flags:                ${CFLAGS}
   Host System Type:              ${host}
   Install path:                  ${prefix}

   TAPI API version               ${cached_tapi_version}
   Linux kernel include path:     ${KERNEL_INCL_PATH}
   Linux kernel build path:       ${KERNEL_BUILD_PATH}")

AS_IF([test "$EVENT_LOGGER_DEBUG_TRUE" = ""],
   [
AS_ECHO(
"   Event logger includes path:    ${EVENT_LOGGER_INCL_PATH}
   Event logger library path:     ${EVENT_LOGGER_LIBRARY_PATH}")],
   [AS_ECHO("   Event logger usage disabled.")])

AS_IF([test "$IFXOS_ENABLED_TRUE" = ""],
   [
AS_ECHO(
"   IFXOS includes path:           ${IFXOS_INCL_PATH}
   IFXOS library path:            ${IFXOS_LIBRARY_PATH}")],
   [AS_ECHO("   IFXOS usage disabled.")])

AS_ECHO("----------------------------------------------------------------------")
