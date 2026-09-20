# LIBIFXOS_CHECK_CONFIG ([DEFAULT-INCL-PATH], [DEFAULT-LIB-PATH])
# ----------------------------------------------------------
# Checks for lib_ifxos 
# specify --with-ifxos-lib 
# and --with-ifxos-incl 
# If not supplied error results.
AC_DEFUN([LIBIFXOS_CHECK_CONFIG],
[
   dnl Determine user choice for ifxos library
   AC_ARG_WITH([ifxos-lib],
      AS_HELP_STRING(
         [--with-ifxos-lib@<:@=DIR@:>@],
         [include ifxos @<:@default=yes@:>@. DIR is the path to the ifxos library.]),
   [
      _want_libifxos_lib="yes"
      _libifxos_lib_with=$withval
   ])

   dnl Determine user choice for ifxos includes
   AC_ARG_WITH([ifxos-incl],
      AS_HELP_STRING(
         [--with-ifxos-incl@<:@=DIR@:>@],
         [include ifxos @<:@default='value from --with-ifxos-incl'@:>@. DIR is the path to the ifxos which contains the include dir.]),
   [
      _want_libifxos_incl="yes"
      _libifxos_incl_with=$withval
   ])

   dnl Set global choice
   if test "x$_want_libifxos_lib" != "xno" -a "x$_want_libifxos_incl" != "xno"; then
      want_libifxos="yes"
   elif test "x$_want_libifxos_lib" == "xno" -a "x$_want_libifxos_incl" != "xno"; then
      AC_MSG_ERROR([Path to the ifxos library is mandatory])
   elif test "x$_want_libifxos_lib" != "xno" -a "x$_want_libifxos_incl" == "xno"; then
      AC_MSG_ERROR([Path to the ifxos library is mandatory])
   fi
    
   found_libifxos="no"
   
	dnl Save current statement
	__saved_CFLAGS=$CFLAGS

	CFLAGS="$CFLAGS -I$_libifxos_incl_with"

   dnl Check path to the ifxos
   AC_MSG_CHECKING(for ifxos lib)
   if test -f $_libifxos_lib_with/libifxos.a -o -f $_libifxos_lib_with/libifxos.dll.a; then
		AC_MSG_RESULT($_libifxos_lib_with)
	else
      AC_MSG_ERROR([Incorrect path to the ifxos library '$_libifxos_lib_with'])
   fi

   dnl Check path to the ifxos includes
   AC_MSG_CHECKING(for ifxos includes)
   if test -e $_libifxos_incl_with/ifxos_version.h; then
      AC_COMPILE_IFELSE(
            [AC_LANG_PROGRAM([[#include "ifxos_version.h"]],
                             [[return;]])],
            [AC_MSG_RESULT([$_libifxos_incl_with])],
            [AC_MSG_ERROR([Invalid ifxos_version.h file in '$_libifxos_incl_with'])]
         )
   else
      AC_MSG_ERROR([No ifxos_version.h file in '$_libifxos_incl_with'])
   fi

   IFXOS_INCL_PATH=$_libifxos_incl_with
   IFXOS_LIBRARY_PATH=$_libifxos_lib_with

   found_libifxos="yes"
   AC_DEFINE(HAVE_LIBIFXOS,1,[Define to 1 if ifxos should be enabled.])

   AC_SUBST(IFXOS_INCL_PATH)
   AC_SUBST(IFXOS_LIBRARY_PATH)

	dnl Restore saved statement
	CFLAGS=$__saved_CFLAGS

   unset _want_libifxos_incl _libifxos_incl_with
   unset _want_libifxos_lib _libifxos_lib_with 

])dnl


# LIBIFXOS_CHECK_CONFIG_INCL ([DEFAULT-INCL-PATH])
# ----------------------------------------------------------
#
# Checks for lib_ifxos includes
# specify --with-ifxos-incl 
# If not supplied error results.
#
AC_DEFUN([LIBIFXOS_CHECK_CONFIG_INCL],
[
   dnl Determine user choice for ifxos includes
   AC_ARG_WITH([ifxos-incl],
      AS_HELP_STRING(
         [--with-ifxos-incl@<:@=DIR@:>@],
         [include ifxos @<:@default='value from --with-ifxos-incl'@:>@. DIR is the path to the ifxos which contains the include dir.]),
   [
      _set_libifxos_incl="yes"
      _libifxos_incl_path=$withval
   ])
   
   dnl Set global choice
   if test "x$_set_libifxos_incl" == "xno"; then
      AC_MSG_ERROR([Path to the ifxos library includes is mandatory])
   fi
   
   if test ! -r $_libifxos_incl_path/ifx_types.h; then
      AC_MSG_ERROR([The lib_ifxos include directory is not valid! '$_libifxos_incl_path'])
   fi
   
	dnl Save current statement
	__saved_CFLAGS=$CFLAGS

	CFLAGS="$CFLAGS -I$_libifxos_incl_path"
   
   dnl Check path to the ifxos includes
   AC_MSG_CHECKING(for ifxos includes)
   if test -e $_libifxos_incl_path/ifxos_version.h; then
      AC_COMPILE_IFELSE(
            [AC_LANG_PROGRAM([[#include "ifxos_version.h"]],
                             [[return;]])],
            [AC_MSG_RESULT([$_libifxos_incl_path])],
            [AC_MSG_ERROR([Invalid ifxos_version.h file in '$_libifxos_incl_path'])]
         )
   else
      AC_MSG_ERROR([No ifxos_version.h file in '$_libifxos_incl_path'])
   fi

   AC_SUBST([IFXOS_INCL_PATH], [$_libifxos_incl_path])

	dnl Restore saved statement
	CFLAGS=$__saved_CFLAGS

   unset _set_libifxos_incl _libifxos_incl_path

])dnl


dnl
dnl Option used to enable/disable ifxos support.
dnl -------------------------------------------------
dnl IFXOS_ENABLE([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-ifxos
dnl		--disable-ifxos
dnl
AC_DEFUN([IFXOS_ENABLE],
[
	if test "$1" = "disable"; then
		__default_disable="(default)"
		__default_enable=""
	else
		__default_disable=""
		__default_enable="(default)"
	fi

	AC_MSG_CHECKING(for ifxos)
	AC_ARG_ENABLE(ifxos,
		AS_HELP_STRING(
			[--enable-ifxos],
			[enable/disable ifxos ($1d by default)]
		),
		[__enable_ifxos=$enableval],
		[__enable_ifxos=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_ifxos" = "yes" -o "$__enable_ifxos" = "enable"; then
		AC_MSG_RESULT([enabled $__default_enable])
		AC_DEFINE([IFXOS_ENABLED], [1], [Use IFXOS library])
		AM_CONDITIONAL(IFXOS_ENABLED, true)
		ifelse([$2],,:,[$2])
	else
		AC_MSG_RESULT([disabled $__default_disable])
		AM_CONDITIONAL(IFXOS_ENABLED, false)
		ifelse([$3],,:,[$3])
	fi

	unset __enable_ifxos
])
