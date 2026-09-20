dnl Option used to select an compiler warnings
dnl -------------------------------------------------
dnl WARNINGS_CHECK ([ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl available values are:
dnl		--enable-warnings
dnl		--disable-warnings (default)
dnl
AC_DEFUN([WARNINGS_CHECK],
[
	AC_MSG_CHECKING(for warnings)
	AC_ARG_ENABLE(warnings,
		AS_HELP_STRING(
			[--enable-warnings],
			[enable compiler warnings (GCC only)]
		),
		[
			if test $enableval = 'yes'; then
				AM_CONDITIONAL(WARNINGS, true)
				CFLAGS="$CFLAGS -Wall"

				ifelse([$1],,:,[$1])

				AC_MSG_RESULT([enabled])
			else
				AM_CONDITIONAL(WARNINGS, false)

				ifelse([$2],,:,[$2])

				AC_MSG_RESULT([disabled])
			fi
		],
		[
			AM_CONDITIONAL(WARNINGS, false)

			ifelse([$2],,:,[$2])

			AC_MSG_RESULT([disabled (default), enable with --enable-warnings])
		]
	)
])


dnl Option used to enable warnings as error
dnl -------------------------------------------------
dnl WERROR_CHECK ([ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-werror
dnl		--disable-werror (default)
dnl
AC_DEFUN([WERROR_CHECK],
[
	AC_MSG_CHECKING(for warnings as errors)
	AC_ARG_ENABLE(werror,
		AS_HELP_STRING(
			[--enable-werror],
			[enable warnings as error (disabled by default)]
		),
		[
			if test $enableval = 'yes'; then
				AM_CONDITIONAL(WERROR, true)
				CFLAGS="$CFLAGS -Werror"

				ifelse([$1],,:,[$1])

				AC_MSG_RESULT([enabled])
			else
				AM_CONDITIONAL(WERROR, false)

				ifelse([$2],,:,[$2])

				AC_MSG_RESULT([disabled])
			fi
		],
		[
			AM_CONDITIONAL(WERROR, false)

			ifelse([$2],,:,[$2])

			AC_MSG_RESULT([disabled (default), enable with --enable-werror])
		]
	)
])


dnl Option used to enable debugging messages
dnl -------------------------------------------------
dnl DEBUG_CHECK ([ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-debug
dnl		--disable-debug (default)
dnl
AC_DEFUN([DEBUG_CHECK],
[
	AC_MSG_CHECKING(for debugging messages)
	AC_ARG_ENABLE(debug,
		AS_HELP_STRING(
			[--enable-debug],
			[build in debugging symbols and enable debugging messages (disabled by default)]
		),
		[
			if test $enableval = 'yes'; then
				AC_DEFINE([DEBUG],[1],[Build in debugging symbols and enable debugging messages])
				AM_CONDITIONAL(DEBUG, true)
				CFLAGS=`echo $CFLAGS | sed -e 's#\-g[0-9]*##g'`
				CFLAGS=`echo $CFLAGS | sed -e 's#\-O[0-9s]*##g'`
				CFLAGS="$CFLAGS -O1 -g3"

				ifelse([$1],,:,[$1])

				AC_MSG_RESULT([enabled])
			else
				AM_CONDITIONAL(DEBUG, false)

				ifelse([$2],,:,[$2])

				AC_MSG_RESULT([disabled])
			fi
		],
		[
			AM_CONDITIONAL(DEBUG, false)

			ifelse([$2],,:,[$2])

			AC_MSG_RESULT([disabled (default), enable with --enable-debug])
		]
	)
])


dnl Option used to enable runtime traces
dnl -------------------------------------------------
dnl TRACE_CHECK ([ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-trace
dnl		--disable-trace (default)
dnl
AC_DEFUN([TRACE_CHECK],
[
	AC_MSG_CHECKING(for runtime traces)
	AC_ARG_ENABLE(trace,
		AS_HELP_STRING(
			[--enable-trace],
			[enable runtime traces (disabled by default)]
		),
		[
			if test $enableval = 'yes'; then
				AM_CONDITIONAL(ENABLE_TRACE, true)
				AC_DEFINE([ENABLE_TRACE],[1],[enable trace outputs in general])

				ifelse([$1],,:,[$1])

				AC_MSG_RESULT([enabled])
			else
				AM_CONDITIONAL(ENABLE_TRACE, false)

				ifelse([$2],,:,[$2])

				AC_MSG_RESULT([disabled])
			fi
		],
		[
			AM_CONDITIONAL(ENABLE_TRACE, false)

			ifelse([$2],,:,[$2])

			AC_MSG_RESULT([disabled (default), enable with --enable-trace])
		]
	)
])


dnl Option used to set additional (device specific) CFLAGS
dnl -------------------------------------------------
dnl
AC_DEFUN([WITH_CFLAGS_CHECK],
[
	AC_MSG_CHECKING(for additional CFLAGS)
	AC_ARG_WITH(cflags,
		AS_HELP_STRING(
			[--with-cflags=val],
			[pass additional (device specific) CFLAGS, not required for Linux 2.6]
		),
		[
			AC_SUBST([USER_EXTRA_CFLAGS],[$withval])
			AM_CONDITIONAL(USER_EXTRA_CFLAGS_SET, true)
			CFLAGS="$CFLAGS $withval"
			AC_MSG_RESULT([$withval])
		],
		[
			AM_CONDITIONAL(USER_EXTRA_CFLAGS_SET, false)
			AC_MSG_RESULT([not set (default), set with --with-cflags])
		]
	)
])


dnl
dnl Option used to enable/disable interrupts
dnl -------------------------------------------------
dnl INTERRUPTS_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-interrupts (default)
dnl		--disable-interrupts
dnl
AC_DEFUN([INTERRUPTS_CHECK],
[
	AC_MSG_CHECKING(for interrupts support)
	AC_ARG_ENABLE(interrupts,
		AS_HELP_STRING(
			[--enable-interrupts],
			[enable interrupts support (enabled by default)]
		),
		[__enable_interrupts=$enableval],
		[__enable_interrupts=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_interrupts" = "yes" -o "$__enable_interrupts" = "enable"; then
		ifelse([$2],,:,[$2])

		AC_MSG_RESULT([enabled])
	else
		ifelse([$3],,:,[$3])

		AC_MSG_RESULT([disabled])
	fi

	unset __enable_interrupts
])


dnl
dnl WITH_DRV_INCL_CHECK(
dnl 	[WITH-INCL], [DRV-NAME], [DRV-DIRS], [DRV-HEADER],
dnl 	[DEFAULT-PATH], [ACTION-IF-PRESENT], [ACTION-IF-MISSED]
dnl ----------------------------------------------------------
dnl
dnl Checks for [DRV-NAME]
dnl specify --with-[WITH-INCL]-incl
dnl If not supplied it checks for default and returns error when
dnl the header file was not found.
dnl
AC_DEFUN([WITH_DRV_INCL_CHECK],
[
	AC_MSG_CHECKING(for $2 includes)
	if test "${cached_found_$1_incl+set}" != set; then
		__want_$1_incl="no"
		__found_$1_incl="no"

		DRV_PATH_CHECK([__$1_default_incl],[$3])

		AC_ARG_WITH([$1-incl],
			AS_HELP_STRING(
				[--with-$1-incl@<:@=DIR@:>@],
				[Path to $2 includes.]
				[@<:@default=<basedir>/<drv_name>/include@:>@; ]
				[<drv_name> are list of @<:@$3@:>@; ]
				[<basedir> are configurable with '--with-drv-incl']
				),
			 [
				__with_$1_incl=$withval
				__want_$1_incl="yes"
			 ],
			[__with_$1_incl=ifelse([$5],,[$__$1_default_incl],[$5])]
		)

		DRV_HEADER_PATH_CHECK([__with_$1_incl], [$4],,
			[__found_$1_incl="yes"])

		AC_MSG_RESULT([$__with_$1_incl ($__found_$1_incl)])
		cached_found_$1_incl=$__found_$1_incl;
		cached_want_$1_incl=$__want_$1_incl;
		cached_with_$1_incl=$__with_$1_incl;

		unset __found_$1_incl __with_$1_incl __want_$1_incl
	else
		AC_MSG_RESULT([$cached_with_$1_incl ($cached_found_$1_incl) (cached)])
	fi

	if test "x$cached_found_$1_incl" == "xyes"; then
		ifelse([$6],,[:],[$6])
	else
		__msg="not found, please specify correct value using '--with-$1-incl'"

		if test "x$cached_want_$1_incl" == "xyes"; then
			AC_MSG_ERROR([$__msg])
		fi

		AC_MSG_WARN([$__msg])

		unset __msg

		ifelse([$7],,[:],[$7])
	fi

])dnl


dnl MAX_DEVICES_CHECK(
dnl  [1-NAME] [2-VARIABLE])
dnl ----------------------------------------------------------
dnl
dnlSet number of max devices
dnl specify --with-max-devices
dnl
dnl
AC_DEFUN([MAX_DEVICES_CHECK],
[
   MAX_DEVICES=1
   dnl set the maximum number of devices supported
   AC_MSG_CHECKING(for maximum number of devices supported)
   AC_ARG_WITH(max-devices,
       AS_HELP_STRING(
           [--with-max-devices[=VAL]],
           [maximum $1 devices to support (1 by default)]
       ),
       [
          if test "$withval" = yes; then
             AC_MSG_ERROR([Please provide a value for the maximum devices]);
          fi
          AC_MSG_RESULT([$withval device(s)])
          MAX_DEVICES=$withval
       ],
       [
          AC_MSG_RESULT([1 device (default), set max devices with --with-max-devices=val])
       ]
   )
   dnl make sure this is defined even if option is not given!
   AC_DEFINE_UNQUOTED([$2],[$MAX_DEVICES],[Maximum $1 devices to support])
])


dnl PROC_CHECK(
dnl   [1-VARIABLE])
dnl ----------------------------------------------------------
dnl
dnl enable use of proc filesystem entries
dnl specify --enable-proc (enabled by default)
dnl
AC_DEFUN([PROC_CHECK],
[
   dnl enable use of proc filesystem entries
   AC_MSG_CHECKING(for use of proc filesystem entries)
   AC_ARG_ENABLE(proc,
       AS_HELP_STRING(
           [--enable-proc],
           [enable use of proc filesystem entries (disabled by default)]
       ),
       [
           if test $enableval = 'yes'; then
               AC_MSG_RESULT(enabled (Linux only))
               AC_DEFINE([$1],[1],[enable use of proc filesystem entries (Linux only)])
           fi
       ],
       [
           AC_MSG_RESULT(disabled)
       ]
   )
])

