dnl
dnl Option used to configure TAPI API version.
dnl If no argument is given to configure script (no -enable-tapi3 or -enable-tapi4)
dnl then default-api should be selected.
dnl -------------------------------------------------
dnl TAPI_VERSION_CHECK([DEFAULT-API], [ACTION-IF-TAPI4-ENABLED], [ACTION-IF-TAPI3-ENABLED])
dnl
dnl available values are:
dnl		--enable-tapi4
dnl		--enable-tapi3
dnl
AC_DEFUN([TAPI_VERSION_CHECK],
[
	AC_MSG_CHECKING(for TAPI API version)

	if test "${cached_tapi_version+set}" != set; then
		AC_ARG_ENABLE(tapi4,
			AS_HELP_STRING(
				[--enable-tapi4],
				[enable TAPI Version 4 interface (single device node) (disabled by default)]
			),
			[__enable_tapi4=$enableval],
			[]
		)

		AC_ARG_ENABLE(tapi3,
			AS_HELP_STRING(
				[--enable-tapi3],
				[enable TAPI Version 3 interface (multiple device node) (enabled by default)]
			),
			[__enable_tapi3=$enableval],
			[]
		)

		if test "$__enable_tapi4" = "yes" -o "$__enable_tapi4" = "enable"; then
			cached_tapi_version=TAPI_VERSION4
		fi

		if test "$__enable_tapi3" = "yes" -o "$__enable_tapi3" = "enable"; then
			if test "${cached_tapi_version+set}" != set; then
				cached_tapi_version=TAPI_VERSION3
			else
				AC_MSG_ERROR(
					[Only one of TAPI API can be enabled! Please select '--enable-tapi4' or '--enable-tapi3'.]
					)
			fi
		fi

		# Select API given as default parameter
		if test "${cached_tapi_version+set}" != set; then
			AC_MSG_NOTICE([Tapi API version was not given in configure string. Trying to selecting version given as default one.])
			if test x$1 != "xTAPI_VERSION3" -a x$1 != "xTAPI_VERSION4"; then
				AC_MSG_ERROR(
					[Allowed Tapi API versions are only TAPI_VERSION3 or TAPI_VERSION4 but given "$1"!]
					)
			fi
			cached_tapi_version=$1
		fi

		if test "${cached_tapi_version+set}" != set; then
			AC_MSG_ERROR(
				[The TAPI API interface should be defined! Please select '--enable-tapi4' or '--enable-tapi3' or set default version in configure.ac script.]
				)
		fi

		AC_MSG_RESULT([$cached_tapi_version] selected)

		echo "#define $cached_tapi_version" > $srcdir/include/drv_tapi_if_version.h

		unset __enable_tapi4 __enable_tapi3
	else
		AC_MSG_RESULT([$cached_tapi_version (cached)])
	fi

	if test "$cached_tapi_version" = TAPI_VERSION4; then
		ifelse([$2],,[:],[$2])
	else
		ifelse([$3],,[:],[$3])
	fi
])


dnl
dnl Option used to enable TAPI NLT/GR909
dnl -------------------------------------------------
dnl TAPI_NLT_CHECK([ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-nlt
dnl		--disable-nlt (default)
dnl
AC_DEFUN([TAPI_NLT_CHECK],
[
	AC_MSG_CHECKING(for TAPI NLT/GR909 services)
	AC_ARG_ENABLE(nlt,
		AS_HELP_STRING(
			[--enable-nlt],
			[enable TAPI Network Line Testing(NLT) services - including GR909 (disabled by default)]
		),
		[__enable_tapi_nlt=$enableval],
		[__enable_tapi_nlt=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_nlt" = "yes" -o "$__enable_tapi_nlt" = "enable"; then
		AC_DEFINE([TAPI_NLT],[1],[enable TAPI Network Line Testing services])
		AC_DEFINE([TAPI_GR909],[1],[enable TAPI GR909 support])
		AM_CONDITIONAL(WITH_NLT, true)
		AM_CONDITIONAL(WITH_GR909, true)
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AM_CONDITIONAL(WITH_NLT, false)
		AM_CONDITIONAL(WITH_GR909, false)
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_nlt
])


dnl Option used to enable capacitance measurement
dnl -------------------------------------------------
dnl TAPI_CAP_MEASUREMENT_CHECK([1-DEFAULT-ACTION], [2-DEVICE-NAME]
dnl [3-ACTION-IF-ENABLED], [4-ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-cap-measurement
dnl		--disable-cap-measurement (default)
dnl
AC_DEFUN([TAPI_CAP_MEASUREMENT_CHECK],
[
	AC_MSG_CHECKING(for capacitance measurement services)
	AC_ARG_ENABLE(cap-measurement,
		AS_HELP_STRING(
			[--enable-cap-measurement],
			[enable capacitance measurement services (disabled by default)]
		),
		[__enable_cap_meas=$enableval],
		[__enable_cap_meas=ifelse([$1],,[no],[$1])]
	)

	if test "x$__enable_cap_meas" = "xyes" -o "x$__enable_cap_meas" = "xenable"; then
		AC_MSG_RESULT([enabled for $2])
		AC_DEFINE([$2_CAPACITANCE_MEASUREMENT_SUPPORT],[1],
			[enable capacitance measurement services])
		ifelse([$3],,[:],[$3])
	else
		AC_MSG_RESULT([disabled for $2])
		ifelse([$4],,[:],[$4])
	fi

	unset __enable_cap_meas
])


dnl Option used to enable direct chip access for LL driver
dnl -------------------------------------------------
dnl TAPI_LL_DIRECT_CHIP_ACCESS_CHECK([1-DEFAULT-ACTION], [2-DEVICE-NAME]
dnl [3-ACTION-IF-ENABLED], [4-ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-direct-chip-access
dnl		--disable-direct-chip-access (default)
dnl
AC_DEFUN([TAPI_LL_DIRECT_CHIP_ACCESS_CHECK],
[
	AC_MSG_CHECKING(for direct chip access support)
	AC_ARG_ENABLE(direct-chip-access,
		AS_HELP_STRING(
			[--enable-direct-chip-access],
			[enable direct chip access support (disabled by default)]
		),
		[__enable_direct_chip_access=$enableval],
		[__enable_direct_chip_access=ifelse([$1],,[no],[$1])]
	)

	if test "x$__enable_direct_chip_access" = "xyes" -o "x$__enable_direct_chip_access" = "xenable"; then
		AC_MSG_RESULT([enabled])

		AC_DEFINE([$2_DIRECT_CHIP_ACCESS_SUPPORT],[1],[enable direct chip access support])

		ifelse([$3],,[:],[$3])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$4],,[:],[$4])
	fi

	unset __enable_direct_chip_access
])


dnl Option used to enable Kernel API for LL driver
dnl -------------------------------------------------
dnl TAPI_KERNEL_API_CHECK([1-DEFAULT-ACTION], [2-DEVICE-NAME]
dnl [3-ACTION-IF-ENABLED], [4-ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-kernel-api
dnl		--disable-kernel-api (default)
dnl
AC_DEFUN([TAPI_KERNEL_API_CHECK],
[
	AC_MSG_CHECKING(for Kernel API support)
	AC_ARG_ENABLE(kernel-api,
		AS_HELP_STRING(
			[--enable-kernel-api],
			[enable Kernel API (disabled by default)]
		),
		[__enable_kernel_api=$enableval],
		[__enable_kernel_api=ifelse([$1],,[no],[$1])]
	)

	if test "x$__enable_kernel_api" = "xyes" -o "x$__enable_kernel_api" = "xenable"; then
		AC_MSG_RESULT([enabled])
		AC_DEFINE([$2_KERNEL_API_SUPPORT],[1],[enable Kernel API])
		ifelse([$3],,[:],[$3])
	else
		AC_MSG_RESULT([disabled])
		ifelse([$4],,[:],[$4])
	fi

	unset __enable_kernel_api
])


dnl Option used to enable tone generator feature for LL driver
dnl -------------------------------------------------
dnl TAPI_TONE_GENERATOR_CHECK([1-DEFAULT-ACTION], [2-DEVICE-NAME]
dnl [3-ACTION-IF-ENABLED], [4-ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-tone-generator (default)
dnl		--disable-tone-generator
dnl
AC_DEFUN([TAPI_TONE_GENERATOR_CHECK],
[
	AC_MSG_CHECKING(for tone generator support)
	AC_ARG_ENABLE(tone-generator,
		AS_HELP_STRING(
			[--enable-tone-generator],
			[enable tone generator (enabled by default)]
		),
		[__enable_tone_generator=$enableval],
		[__enable_tone_generator=ifelse([$1],,[yes],[$1])]
	)

	if test "x$__enable_tone_generator" = "xyes" -o "x$__enable_tone_generator" = "xenable"; then
		AC_MSG_RESULT([enabled])

		AC_DEFINE([$2_TONE_GENERATOR_SUPPORT],[1],[enable tone generator feature])

		ifelse([$3],,[:],[$3])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$4],,[:],[$4])
	fi

	unset __enable_tone_generator
])


dnl Option used to enable TAPI DTMF
dnl -------------------------------------------------
dnl TAPI_DTMF_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-dtmf (default)
dnl		--disable-dtmf
dnl
AC_DEFUN([TAPI_DTMF_CHECK],
[
	AC_MSG_CHECKING(for TAPI DTMF support)
	AC_ARG_ENABLE(dtmf,
		AS_HELP_STRING(
			[--enable-dtmf],
			[enable TAPI DTMF support (enabled by default)]
		),
		[__enable_tapi_dtmf=$enableval],
		[__enable_tapi_dtmf=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_dtmf" = "yes" -o "$__enable_tapi_dtmf" = "enable"; then
		AC_DEFINE([TAPI_DTMF],[1],[enable TAPI DTMF support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_dtmf
])


dnl
dnl Option used to enable TAPI CID
dnl -------------------------------------------------
dnl TAPI_CID_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-cid
dnl		--disable-cid (default)
dnl
AC_DEFUN([TAPI_CID_CHECK],
[
	AC_MSG_CHECKING(for TAPI CID support)
	AC_ARG_ENABLE(cid,
		AS_HELP_STRING(
			[--enable-cid],
			[enable TAPI CID support (disabled by default)]
		),
		[__enable_tapi_cid=$enableval],
		[__enable_tapi_cid=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_cid" = "yes" -o "$__enable_tapi_cid" = "enable"; then
		AC_DEFINE([TAPI_CID],[1],[enable TAPI CID support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_cid
])


dnl
dnl Option used to enable TAPI Hook state machine
dnl -------------------------------------------------
dnl TAPI_HSM_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-hsm (default)
dnl		--disable-hsm
dnl
AC_DEFUN([TAPI_HSM_CHECK],
[
	AC_MSG_CHECKING(for TAPI Hook state machine)
	AC_ARG_ENABLE(hsm,
		AS_HELP_STRING(
         [--enable-hsm],
         [enable TAPI Hook state machine (enabled by default)]
		),
		[__enable_tapi_hsm=$enableval],
		[__enable_tapi_hsm=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_hsm" = "yes" -o "$__enable_tapi_hsm" = "enable"; then
		AC_DEFINE([TAPI_HOOKSTATE],[1],[enable TAPI Hook state machine])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_hsm
])


dnl
dnl Option used to enable TAPI Metering feature
dnl -------------------------------------------------
dnl TAPI_METERING_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-metering
dnl		--disable-metering (default)
dnl
AC_DEFUN([TAPI_METERING_CHECK],
[
	AC_MSG_CHECKING(for TAPI METERING support)
	AC_ARG_ENABLE(metering,
		AS_HELP_STRING(
         [--enable-metering],
         [enable TAPI metering support (disabled by default)]
		),
		[__enable_tapi_metering=$enableval],
		[__enable_tapi_metering=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_metering" = "yes" -o "$__enable_tapi_metering" = "enable"; then
		AC_DEFINE([TAPI_METERING],[1],[enable TAPI METERING support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_metering
])


dnl
dnl Option used to enable TAPI Analog Line Continuous Measurement
dnl -------------------------------------------------
dnl TAPI_CONT_MEAS_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-cont-measurement
dnl		--disable-cont-measurement (default)
dnl
AC_DEFUN([TAPI_CONT_MEAS_CHECK],
[
	AC_MSG_CHECKING(for Analog Line Continuous Measurement)
	AC_ARG_ENABLE(cont-measurement,
		AS_HELP_STRING(
			[--enable-cont-measurement],
			[enable TAPI Analog Line Continuous Measurement (disabled by default)]
		),
		[__enable_tapi_cont_meas=$enableval],
		[__enable_tapi_cont_meas=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_cont_meas" = "yes" -o "$__enable_tapi_cont_meas" = "enable"; then
		AC_DEFINE([TAPI_CONT_MEASUREMENT],[1],[enable TAPI Analog Line Continuous Measurement])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_cont_meas
])


dnl
dnl Option used to enable TAPI FXS Phone Detection support
dnl -------------------------------------------------
dnl TAPI_PHONE_DET_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-phone-detection
dnl		--disable-phone-detection (default)
dnl
AC_DEFUN([TAPI_PHONE_DET_CHECK],
[
	AC_MSG_CHECKING(for FXS Phone Detection support)
	AC_ARG_ENABLE(phone-detection,
		AS_HELP_STRING(
			[--enable-phone-detection],
			[enable TAPI FXS Phone Detection support (disabled by default)]
		),
		[__enable_tapi_phone_det=$enableval],
		[__enable_tapi_phone_det=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_phone_det" = "yes" -o "$__enable_tapi_phone_det" = "enable"; then
		AC_DEFINE([TAPI_PHONE_DETECTION],[1],[enable FXS Phone Detection support])
		AM_CONDITIONAL(TAPI_PHONE_DETECTION, true)
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])
		AM_CONDITIONAL(TAPI_PHONE_DETECTION, false)

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_phone_det
])


dnl
dnl Option used to enable PCM channel support
dnl -------------------------------------------------
dnl TAPI_PCM_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-pcm (default)
dnl		--disable-pcm
dnl
AC_DEFUN([TAPI_PCM_CHECK],
[
	AC_MSG_CHECKING(for PCM support)

	if test "${cached_enable_tapi_pcm+set}" != set; then
		AC_ARG_ENABLE(pcm,
			AS_HELP_STRING(
				[--enable-pcm],
				[enable PCM channel support (enabled by default)]
			),
			[__enable_tapi_pcm=$enableval],
			[__enable_tapi_pcm=ifelse([$1],,[yes],[$1])]
		)

		if test "$__enable_tapi_pcm" = "yes" -o "$__enable_tapi_pcm" = "enable"; then
			cached_enable_tapi_pcm=enabled
			AC_DEFINE([TAPI_PCM_SUPPORT],[1],[enable PCM channel support])
			AM_CONDITIONAL(TAPI_PCM_SUPPORT, true)
			AC_MSG_RESULT([enabled])

		else
			cached_enable_tapi_pcm=disabled
			AM_CONDITIONAL(TAPI_PCM_SUPPORT, false)
			AC_MSG_RESULT([disabled])
		fi

		unset __enable_tapi_pcm
	else
		AC_MSG_RESULT([$cached_enable_tapi_pcm (cached)])
	fi

	if test "$cached_enable_tapi_pcm" = enabled; then
		ifelse([$2],,[:],[$2])
	else
		ifelse([$3],,[:],[$3])
	fi
])


dnl Option used to enable MWL support
dnl -------------------------------------------------
dnl TAPI_MWL_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-mwl (default)
dnl		--disable-mwl
dnl
AC_DEFUN([TAPI_MWL_CHECK],
[
	AC_MSG_CHECKING(for TAPI MWL support)
	AC_ARG_ENABLE(mwl,
		AS_HELP_STRING(
			[--enable-mwl],
			[enable MWL support (disabled by default)]
		),
		[__enable_tapi_mwl=$enableval],
		[__enable_tapi_mwl=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_mwl" = "yes" -o "$__enable_tapi_mwl" = "enable"; then
		AC_DEFINE([TAPI_MWL],[1],[enable TAPI MWL support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_mwl
])


dnl Option used to enable Calibration support
dnl -------------------------------------------------
dnl TAPI_CALIBRATION_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-calibration (default)
dnl		--disable-calibration
dnl
AC_DEFUN([TAPI_CALIBRATION_CHECK],
[
	AC_MSG_CHECKING(for TAPI Calibration support)
	AC_ARG_ENABLE(calibration,
		AS_HELP_STRING(
			[--enable-calibration],
			[enable Calibration support (enabled by default)]
		),
		[__enable_tapi_calibration=$enableval],
		[__enable_tapi_calibration=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_calibration" = "yes" -o "$__enable_tapi_calibration" = "enable"; then
		AC_DEFINE([TAPI_CALIBRATION],[1],[enable TAPI Calibration support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_calibration
])


dnl Option used to enable the ring-engine for cadenced ringing
dnl -------------------------------------------------
dnl TAPI_RINGENGINE_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-ring-engine (default)
dnl		--disable-ring-engine
dnl
AC_DEFUN([TAPI_RINGENGINE_CHECK],
[
	AC_MSG_CHECKING(for TAPI ring engine support)
	AC_ARG_ENABLE(ring-engine,
		AS_HELP_STRING(
			[--enable-ring-engine],
			[enable ring engine support (enabled by default)]
		),
		[__enable_tapi_ringengine=$enableval],
		[__enable_tapi_ringengine=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_ringengine" = "yes" -o "$__enable_tapi_ringengine" = "enable"; then
		AC_DEFINE([TAPI_RING_ENGINE],[1],[enable TAPI ring engine support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_ringengine
])


dnl Option used to enable pulse dial/hook support
dnl -------------------------------------------------
dnl TAPI_DIAL_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-dial (default)
dnl		--disable-dial
dnl
AC_DEFUN([TAPI_DIAL_CHECK],
[
	AC_MSG_CHECKING(for TAPI Pulse dial support)
	AC_ARG_ENABLE(dial,
		AS_HELP_STRING(
			[--enable-dial],
			[enable pulse dial/hook support (enabled by default)]
		),
		[__enable_tapi_dial=$enableval],
		[__enable_tapi_dial=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_tapi_dial" = "yes" -o "$__enable_tapi_dial" = "enable"; then
		AC_DEFINE([TAPI_DIAL],[1],[enable TAPI pulse dial/hook support])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_dial
])


dnl
dnl Option used to set strict permissions on HL and LL driver file interfaces
dnl -------------------------------------------------------------------------
dnl TAPI_STRICT_PERMISSIONS_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-strict-permissions
dnl		--disable-strict-permissions (default)
dnl
AC_DEFUN([TAPI_STRICT_PERMISSIONS_CHECK],
[
	AC_MSG_CHECKING(for strict permissions)
	AC_ARG_ENABLE(strict-permissions,
		AS_HELP_STRING(
         [--enable-strict-permissions],
         [set strict permissions on IO files (disabled by default)]
		),
		[__enable_strict_perm=$enableval],
		[__enable_strict_perm=ifelse([$1],,[no],[$1])]
	)
	if test "$__enable_strict_perm" = "yes" -o "$__enable_strict_perm" = "enable"; then
		AC_DEFINE([TAPI_STRICT_PERMISSIONS],[1],[strict io file permissions])
		AM_CONDITIONAL(TAPI_STRICT_PERMISSIONS, true)
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])
		AM_CONDITIONAL(TAPI_STRICT_PERMISSIONS, false)

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_strict_perm
])


dnl
dnl Option used to change the ring cadence to adapt the time per bit
dnl to the length of a ring period instead of fixed 50ms steps
dnl -------------------------------------------------
dnl TAPI_RING_ADAPTIVE_BITTIME_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-ring-adaptive-bittime
dnl		--disable-ring-adaptive-bittime (default)
dnl
AC_DEFUN([TAPI_RING_ADAPTIVE_BITTIME_CHECK],
[
	AC_MSG_CHECKING(for TAPI ring cadence bit duration)
	AC_ARG_ENABLE(ring-adaptive-bittime,
		AS_HELP_STRING(
			[--enable-ring-adaptive-bittime],
			[adapt the time per bit to the configured ring period (disabled by default)]
		),
		[__enable_ring_adaptive_bittime=$enableval],
		[__enable_ring_adaptive_bittime=ifelse([$1],,[no],[$1])]
	)
	if test "$__enable_ring_adaptive_bittime" = "yes" -o "$__enable_ring_adaptive_bittime" = "enable"; then
		AC_DEFINE([TAPI_RING_ADAPTIVE_BITTIME],[1],[adapt the duration for each bit in the ring cadence to the configured ring-period])
		AC_MSG_RESULT([adapt to ring period])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([fixed 50 ms per bit])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_ring_adaptive_bittime
])


dnl
dnl Option used to enable usage of POTS extended features
dnl -------------------------------------------------
dnl TAPI_EXTENDED_POTS_FEATURES_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-extended-features
dnl		--disable-extended-features (default)
dnl
AC_DEFUN([TAPI_EXTENDED_POTS_FEATURES_CHECK],
[
	AC_MSG_CHECKING(for group of TAPI extended POTS features)
	AC_ARG_ENABLE(extended-features,
		AS_HELP_STRING(
			[--enable-extended-features],
			[enable extended POTS features (disabled by default)]
		),
		[__enable_tapi_extended_features=$enableval],
		[__enable_tapi_extended_features=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_extended_features" = "yes" -o "$__enable_tapi_extended_features" = "enable"; then
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_extended_features
])


dnl
dnl Option used to enable usage of POTS linetesting features
dnl -------------------------------------------------
dnl TAPI_LINETESTING_POTS_FEATURES_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-linetesting-features
dnl		--disable-linetesting-features (default)
dnl
AC_DEFUN([TAPI_LINETESTING_POTS_FEATURES_CHECK],
[
	AC_MSG_CHECKING(for group of TAPI linetesting POTS features)
	AC_ARG_ENABLE(linetesting-features,
		AS_HELP_STRING(
			[--enable-linetesting-features],
			[enable linetesting POTS features (disabled by default)]
		),
		[__enable_tapi_linetesting_features=$enableval],
		[__enable_tapi_linetesting_features=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_linetesting_features" = "yes" -o "$__enable_tapi_linetesting_features" = "enable"; then
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_linetesting_features
])


dnl
dnl Option used to keep compatibility with classic TAPI driver ioctl API
dnl -------------------------------------------------
dnl CLASSIC_TAPI_COMPATIBILITY_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-classic-tapi-compat (default)
dnl		--disable-classic-tapi-compat
dnl
AC_DEFUN([CLASSIC_TAPI_COMPATIBILITY_CHECK],
[
	AC_MSG_CHECKING(for classic TAPI compatibility)
	AC_ARG_ENABLE(classic-tapi-compat,
		AS_HELP_STRING(
			[--enable-classic-tapi-compat],
			[enable classic TAPI driver compatibility (enabled by default)]
		),
		[__enable_classic_tapi_compatibility=$enableval],
		[__enable_classic_tapi_compatibility=ifelse([$1],,[yes],[$1])]
	)

	if test "$__enable_classic_tapi_compatibility" = "yes" -o "$__enable_classic_tapi_compatibility" = "enable"; then
		AC_DEFINE([CLASSIC_TAPI_COMPATIBLE],[1],[keep TAPI ioctl API compatible with classic TAPI driver])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_classic_tapi_compatibility
])


dnl
dnl Option to enable TAPI debug buffer which can be used to trace e.g. SPI traffic 
dnl or user events.
dnl -------------------------------------------------
dnl TAPI_DEBUG_BUFFER_CHECK([DEFAULT-ACTION], [ACTION-IF-ENABLED], [ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-tapi-debug-buffer
dnl		--disable-tapi-debug-buffer (default)
dnl
AC_DEFUN([TAPI_DEBUG_BUFFER_CHECK],
[
	AC_MSG_CHECKING(for TAPI debug buffer)
	AC_ARG_ENABLE(tapi-debug-buffer,
		AS_HELP_STRING(
			[--enable-tapi-debug-buffer],
			[enable TAPI debug buffer (disabled by default)]
		),
		[__enable_tapi_debug_buffer=$enableval],
		[__enable_tapi_debug_buffer=ifelse([$1],,[no],[$1])]
	)

	if test "$__enable_tapi_debug_buffer" = "yes" -o "$__enable_tapi_debug_buffer" = "enable"; then
		AC_DEFINE([ENABLE_TAPI_DEBUG_BUFFER],[1],[Enable TAPI debug buffer feature])
		AC_MSG_RESULT([enabled])

		ifelse([$2],,[:],[$2])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$3],,[:],[$3])
	fi

	unset __enable_tapi_debug_buffer
])


dnl TAPI_DEBUG_BUFFER_NUM_OF_ENTRIES_CHECK
dnl ----------------------------------------------------------
dnl
dnl Set number of max entries in TAPI debug buffer (buffer capacity)
dnl specify --with-tapi-debug-buf-entries
dnl
dnl
AC_DEFUN([TAPI_DEBUG_BUFFER_NUM_OF_ENTRIES_CHECK],
[
   TAPI_DEBUG_BUFFER_NUM_OF_ENTRIES=1024

   dnl set the maximum number of entries in TAPI debug buffer
   AC_MSG_CHECKING(for maximum number of entries in TAPI debug buffer)
   AC_ARG_WITH(tapi-debug-buf-entries,
       AS_HELP_STRING(
           [--with-tapi-debug-buf-entries[=VAL]],
           [maximum number of TAPI debug buffer entries ($TAPI_DEBUG_BUFFER_NUM_OF_ENTRIES by default)]
       ),
       [
          if test "$withval" = "yes"; then
             AC_MSG_ERROR([Please provide a value for the maximum number of entries in TAPI debug buffer]);
          fi
          AC_MSG_RESULT([$withval buffer entries(s)])
          TAPI_DEBUG_BUFFER_NUM_OF_ENTRIES=$withval
       ],
       [
          AC_MSG_RESULT([$TAPI_DEBUG_BUFFER_NUM_OF_ENTRIES buffer entries (default), can be changed --with-tapi-debug-buf-entries=val])
       ]
   )
   dnl make sure this is defined even if option is not given!
   AC_DEFINE_UNQUOTED([TAPI_DEBUG_BUFFER_NUM_OF_ENTRIES],[$TAPI_DEBUG_BUFFER_NUM_OF_ENTRIES],[Maximum TAPI debug buffer entries])
])


dnl Option used to enable tone generator level compensation
dnl -------------------------------------------------------
dnl TAPI_TONE_GENERATOR_LEVEL_COMPENSATION_CHECK([1-DEFAULT-ACTION],
dnl [2-DEVICE-NAME], dnl [3-ACTION-IF-ENABLED], [4-ACTION-IF-DISABLED])
dnl
dnl available values are:
dnl		--enable-tone-generator-level-compensation
dnl		--disable-tone-generator-level-compensation (default)
dnl
AC_DEFUN([TAPI_TONE_GENERATOR_LEVEL_COMPENSATION_CHECK],
[
	AC_MSG_CHECKING(for compensation of tone generator levels)
	AC_ARG_ENABLE(tone-generator-level-compensation,
		AS_HELP_STRING(
			[--enable-tone-generator-level-compensation],
			[Enable compensation of tone generator levels]
		),
		[__enable_tone_generator_level_compensation=$enableval],
		[__enable_tone_generator_level_compensation=ifelse([$1],,[no],[$1])]
	)

	if test "x$__enable_tone_generator_level_compensation" = "xyes" -o "x$__enable_tone_generator_level_compensation" = "xenable"; then
		AC_MSG_RESULT([enabled])

		AC_DEFINE([$2_TONE_GENERATOR_LEVEL_COMPENSATION],[1],[enable compensation of tone levels with analog line gains])

		ifelse([$3],,[:],[$3])
	else
		AC_MSG_RESULT([disabled])

		ifelse([$4],,[:],[$4])
	fi

	unset __enable_tone_generator_level_compensation
])
