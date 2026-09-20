#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
grep -q "AC_PROG_RANLIB" $DIR/../configure.in
if [ $? -eq 0 ]
then
   sed -i 's/AC_PROG_RANLIB/LT_PREREQ([2.2.6])\nLT_INIT([win32-dll])/' $DIR/../configure.in
   cat <<EOF >> $DIR/../src/Makefile.am

if BUILD_SHARED_LIB
lib_LTLIBRARIES = libdrvtapi.la
libdrvtapi_la_SOURCES = \$(drv_tapi_common_SOURCES) \\
	drv_tapi_dev_io.c
libdrvtapi_la_CFLAGS =
libdrvtapi_la_LDFLAGS = -version-info 0:0:0
libdrvtapi_la_LIBADD = -L@IFXOS_LIBRARY_PATH@ -lifxos
if LINUX
libdrvtapi_la_CFLAGS += \$(libdrvtapi_a_CFLAGS)
else
if WIN32
libdrvtapi_la_CFLAGS += -DTAPI_LIBRARY -DTAPI_HAVE_TIMERS \\
	\$(AM_CFLAGS) \\
	\$(libdrvtapi_additional_cflags)
libdrvtapi_la_LDFLAGS += -no-undefined
if EVENT_LOGGER_DEBUG
libdrvtapi_la_LIBADD += @EVENT_LOGGER_LIBRARY_PATH@/libdrveventlogger.la
endif EVENT_LOGGER_DEBUG
endif WIN32
endif LINUX_FALSE
libdrvtapi_la_CFLAGS += \$(INCLUDES)
endif BUILD_SHARED_LIB
EOF
fi
