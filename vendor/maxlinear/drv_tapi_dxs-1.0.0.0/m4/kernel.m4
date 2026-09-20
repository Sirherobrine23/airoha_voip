dnl
dnl KERNEL_CHECK_CONFIG
dnl ----------------------------------------------------------
dnl
dnl Checks for kernel configuration
dnl specify --with-kernel-incl, --with-kernel-build
dnl
AC_DEFUN([KERNEL_CHECK_CONFIG],
[
	dnl check for kernel includes
	KERNEL_INCL_CHECK
	KERNEL_BUILD_CHECK

	if test -z "$ARCH" ; then
		[ARCH=`$CC -dumpmachine`]
	fi

	AC_MSG_CHECKING(for kernel architecture)
	if test -n "$ARCH" ; then
		[ ARCH=`echo $ARCH | sed -e s'/-.*//' \
			-e 's/i[3-9]86/i386/' \
			-e 's/mipsel/mips/' \
			-e 's/sh[234]/sh/' \
		`]
		AC_SUBST([KERNEL_ARCH],[$ARCH])
		AC_MSG_RESULT([$ARCH])
	else
		AC_MSG_ERROR([Kernel architecture not set!])
	fi
])


dnl
dnl Option used to check for valid path to the kernel includes
dnl -------------------------------------------------
dnl KERNEL_INCL_CHECK
dnl
dnl available values are:
dnl		--with-kernel-incl=<path>
dnl
AC_DEFUN([KERNEL_INCL_CHECK],
[
	AC_MSG_CHECKING(for kernel includes)

	if test "${cached_with_kernel_incl+set}" != set; then
		dnl Check for user given kernel includes path
		AC_ARG_WITH([kernel-incl],
			AS_HELP_STRING(
				[--with-kernel-incl@<:@=DIR@:>@],
				[target kernel sources path]
			),
			[__kernel_incl=$withval],
			[
				dnl Check for existance of deprecated option --enable-kernelincl
				AC_ARG_ENABLE([kernelincl],,
					[
						AC_MSG_WARN([deprecated configure option --enable-kernelincl used, use --with-kernel-incl instead])
						__kernel_incl=$enableval
					],
					[AC_MSG_ERROR([Set path to kernel includes using --with-kernel-incl@<:@=DIR@:>@])]
					)
			])
		AC_MSG_RESULT($__kernel_incl)

		dnl Verify if user given kernel includes path is valid
		if test -f $__kernel_incl/linux/kernel.h; then
			AC_SUBST([KERNEL_INCL_PATH],[$__kernel_incl])
			cached_with_kernel_incl=$__kernel_incl
		else
			AC_MSG_ERROR([Incorrect path to kernel includes '$__kernel_incl'])
		fi
	else
		AC_MSG_RESULT([$cached_with_kernel_incl (cached)])
	fi
])


dnl
dnl Option used to check for valid path to the kernel build path
dnl -------------------------------------------------
dnl KERNEL_BUILD_CHECK
dnl
dnl available values are:
dnl		--with-kernel-build=<path>
dnl
AC_DEFUN([KERNEL_BUILD_CHECK],
[
	AC_MSG_CHECKING(for kernel build path)

	if test "${cached_with_kernel_build+set}" != set; then

		dnl Check for user given kernel build path (used for Linux 2.6 only)
		AC_ARG_WITH([kernel-build],
			AS_HELP_STRING(
				[--with-kernel-build@<:@=DIR@:>@],
				[target kernel build path, applies to Linux 2.6 version, only]
			),
			[__with_kernel_build=$withval],
			[
            __with_kernel_build=ifelse([$1],,[$KERNEL_INCL_PATH/..],[$1])
            dnl Check for existance of depricated option --enable-kernelbuild
            AC_ARG_ENABLE([kernelbuild],,
            [
               AC_MSG_WARN([deprecated configure option --enable-kernelbuild, use --with-kernel-build instead])
               __with_kernel_build=$enableval
            ])
            dnl Check for existance of depricated option --enable-kernel-build
            AC_ARG_ENABLE([kernel-build],,
            [
               AC_MSG_WARN([deprecated configure option --enable-kernel-build, use --with-kernel-build instead])
               __with_kernel_build=$enableval
            ])
         ]
		)

		AC_MSG_RESULT($__with_kernel_build)

		if test -e $__with_kernel_build; then
                 dnl check if autoconf.h file is present
                 if test -r $__with_kernel_build/include/linux/autoconf.h; then
                  AC_SUBST([KERNEL_BUILD_PATH],[$__with_kernel_build])
                  cached_with_kernel_build=$__with_kernel_build
                 else
                  if test -r $__with_kernel_build/include/generated/autoconf.h; then
                   AC_SUBST([KERNEL_BUILD_PATH],[$__with_kernel_build])
                   cached_with_kernel_build=$__with_kernel_build
                  else
                   AC_MSG_ERROR([The kernel build directory is not valid or not configured!])
                  fi
                 fi
		else
			AC_MSG_ERROR([Incorrect path to kernel build directory])
		fi

		unset __with_kernel_build
	else
		AC_MSG_RESULT([$cached_with_kernel_build (cached)])
	fi
])


dnl
dnl Option used to configure build with or without kernel module
dnl -------------------------------------------------
dnl WITH_KERNEL_MODULE_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--with-kernel-module (default)
dnl		--without-kernel-module
dnl
AC_DEFUN([WITH_KERNEL_MODULE_CHECK],
[
	AC_MSG_CHECKING(for build with or without Kernel Module)
	AC_ARG_WITH(kernel-module,
		AS_HELP_STRING(
			[--with-kernel-module],
			[build with or without Kernel Module (with by default)]
		),
		[__with_kernel_module=$withval],
		[__with_kernel_module=ifelse([$1],,[yes],[$1])]
	)

	if test "$__with_kernel_module" = "yes" -o "$__with_kernel_module" = "1"; then
		AM_CONDITIONAL(WITH_KERNEL_MODULE, true)
		AC_SUBST([WITH_KERNEL_MODULE],[yes])
		AC_MSG_RESULT([with])

		ifelse([$2],,[:],[$2])

		KERNEL_CHECK_CONFIG
	else
		AM_CONDITIONAL(WITH_KERNEL_MODULE, false)
		AC_SUBST([WITH_KERNEL_MODULE],[no])
		AC_MSG_RESULT([without])

		ifelse([$3],,[:],[$3])
	fi

	unset __with_kernel_module
])


dnl
dnl Option used to enable Linux SMP support
dnl -------------------------------------------------------
dnl LINUX_SMP_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--disable-linux-smp (default)
dnl		--enable-linux-smp
dnl
AC_DEFUN([LINUX_SMP_CHECK],
[
	AC_MSG_CHECKING(for Linux SMP)
	AC_ARG_ENABLE(linux-smp,
		AS_HELP_STRING(
			[--enable-linux-smp],
			[enable Linux SMP support (disabled by default)]
		),
		[
			if test "x$enableval" = "xno"; then
				AC_MSG_RESULT([disabled])
			else
				AC_MSG_RESULT([enabled])
				AC_DEFINE([LINUX_SMP_SUPPORT],[1],[enable Linux SMP support])
			fi
		],
		[
			AC_MSG_RESULT([disabled(default)])
		]
	)
])
