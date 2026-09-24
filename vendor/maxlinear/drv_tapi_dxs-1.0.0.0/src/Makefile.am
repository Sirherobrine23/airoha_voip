#
#
#  Copyright 2020-2023 MaxLinear, Inc.
#
#  For licensing information, see the file 'LICENSE' in the root folder of
#  this software module.
#
#

if WITH_KERNEL_MODULE
bin_PROGRAMS = drv_tapi_dxs.ko
## dummy file required for distcheck target
else
lib_LIBRARIES = libdrvdxstapi.a
endif

# Common DUSLIC XS driver sources
drv_tapi_dxs_SOURCES = \
   dxs/drv_dxs_alm_aclm.c \
   dxs/drv_dxs_access.c \
   dxs/drv_dxs_mbx.c \
   dxs/drv_dxs_outbox.c \
   dxs/drv_dxs_fifo.c \
   dxs/drv_dxs_init.c \
   dxs/drv_dxs_irq.c \
   dxs/drv_dxs_bbd.c \
   dxs/drv_dxs_alm.c \
   dxs/drv_dxs_alm_lt.c \
   dxs/drv_dxs_ioctl.c \
   dxs/drv_dxs_dwld.c \
   dxs/drv_dxs_cid.c \
   dxs/drv_dxs_cid_sm.c \
   dxs/drv_dxs_pcm.c \
   dxs/drv_dxs_tg.c \
   dxs/drv_dxs_dtmf.c \
   dxs/drv_dxs_dtmf_sm.c \
   dxs/drv_dxs_event.c \
   dxs/drv_dxs_debug.c

# Sources from TAPI driver start
drv_tapi_dxs_SOURCES += \
   tapi/drv_tapi_dial.c \
   tapi/drv_tapi_event.c \
   tapi/drv_tapi_debug.c \
   tapi/drv_tapi_debug_buffer.c \
   tapi/drv_tapi_ioctl.c \
   tapi/drv_tapi_meter.c \
   tapi/drv_tapi_misc.c \
   tapi/drv_tapi_opcontrol.c \
   tapi/drv_tapi_ppd.c \
   tapi/drv_tapi_pcm.c \
   tapi/drv_tapi_ring.c \
   tapi/drv_tapi_tone.c \
   tapi/drv_tapi_cid.c \
   tapi/drv_tapi_init.c \
   lib/lib_bbd.c \
   lib/lib_bufferpool.c \
   lib/lib_fifo.c

if LINUX
drv_tapi_dxs_SOURCES += \
   tapi/drv_tapi_linux_procfs_phone_detection.c \
   tapi/drv_tapi_linux_procfs.c \
   tapi/drv_tapi_linux_threads.c \
   tapi/drv_tapi_linux_timers.c \
   tapi/drv_tapi_linux_kioctl.c \
   tapi/drv_tapi_linux_ioctl.c \
   tapi/drv_tapi_linux_lldev_registration.c \
   tapi/drv_tapi_linux_reboot.c \
   tapi/drv_tapi_linux.c \
   tapi/drv_tapi_compat.c

if !IFXOS_ENABLED
drv_tapi_dxs_SOURCES += \
   os_calls/ifxos_linux_copy_user_space_drv.c \
   os_calls/ifxos_linux_event_drv.c \
   os_calls/ifxos_linux_lock_drv.c \
   os_calls/ifxos_linux_memory_alloc_drv.c \
   os_calls/ifxos_linux_mutex_drv.c \
   os_calls/ifxos_linux_select_drv.c \
   os_calls/ifxos_linux_thread_drv.c \
   os_calls/ifxos_linux_time_drv.c \
   os_calls/ifxos_debug.c
endif

else # LINUX FALSE

if WIN32
drv_tapi_dxs_SOURCES += \
   tapi/drv_tapi_win.c
endif WIN32

endif # LINUX

# Sources from TAPI driver end

if WITH_KERNEL_MODULE
drv_tapi_dxs_SOURCES += \
   dxs/drv_dxs_linux_irq.c \
   dxs/drv_dxs_linux_spi.c \
   dxs/drv_dxs_linux.c

DXS_SPEC_DIST = dxs/drv_dxs_dev_io.c
else
# library sources
libdrvdxstapi_a_SOURCES = $(drv_tapi_dxs_SOURCES) \
                           dxs/drv_dxs_dev_io.c
DXS_SPEC_DIST = dxs/drv_dxs_linux.c

endif

EXTRA_DIST = \
   ../include/drv_dxs_errno.h \
   ../include/drv_dxs_strerrno.h \
   ../include/drv_dxs_io.h \
   ../include/drv_dxs_io_types.h \
   ../include/drv_dxs_dev_io.h \
   dxs/drv_dxs_version.h \
   dxs/drv_dxs_alm_aclm.h \
   dxs/drv_dxs_irq.h \
   dxs/drv_dxs_access.h \
   dxs/drv_dxs_mbx.h \
   dxs/drv_dxs_fifo.h \
   dxs/drv_dxs_bbd.h \
   dxs/drv_dxs_dwld.h \
   dxs/drv_dxs_tg.h \
   dxs/drv_dxs_dtmf.h \
   dxs/drv_dxs_dtmf_priv.h \
   dxs/drv_dxs_pcm.h \
   dxs/drv_dxs_pcm_priv.h \
   dxs/drv_dxs_alm.h \
   dxs/drv_dxs_alm_priv.h \
   dxs/drv_dxs_alm_lt.h \
   dxs/drv_dxs_cid.h \
   dxs/drv_dxs_cid_priv.h \
   dxs/drv_dxs_init.h \
   dxs/drv_dxs.h \
   dxs/drv_dxs_api.h \
   dxs/drv_dxs_fw_headers.h \
   dxs/drv_dxs_fw_cmd_sdd.h \
   dxs/drv_dxs_fw_cmd_eop.h \
   dxs/drv_dxs_debug.h \
   dxs/drv_dxs_ll_if_version.h \
   dxs/drv_dxs_outbox.h \
   dxs/drv_dxs_linux.h \
   dxs/drv_config_user.template.h \
   dxs/drv_config_user.easy3201.evs.h \
   dxs/drv_config_user.xrx500.h \
   dxs/drv_config_user.prx321_eva.h \
   dxs/drv_config_user.h \
   dxs/drv_config_user.lgb.h \
   dxs/drv_dxs_io_types_32.h \
   dxs/drv_dxs_alm_gr909.c \
   dxs/drv_dxs_alm_gr909.h

# EXTRA_DIST from TAPI driver
EXTRA_DIST += \
   ../include/drv_tapi_io.h \
   ../include/drv_tapi_io_indexes.h \
   ../include/drv_tapi_kio.h \
   ../include/drv_tapi_dev_io.h \
   ../include/drv_tapi_strerrno.h \
   ../include/drv_tapi_errno.h \
   ../include/drv_tapi_config.h \
   ../include/drv_tapi_osmap.h \
   ../include/drv_tapi_if_version.v3 \
   ../include/drv_tapi_if_version.v4 \
   ../include/drv_config_tapi_compatibility.h.in \
   ../include/lib_bbd.h \
   ../include/lib_bufferpool.h \
   ../include/lib_fifo.h \
   tapi/drv_tapi_osmap_local.h \
   tapi/drv_tapi.h \
   tapi/drv_tapi_ioctl.h \
   tapi/drv_tapi_ioctl_handlers.h \
   tapi/drv_tapi_compat.h \
   tapi/drv_tapi_api.h \
   tapi/drv_tapi_cid.h \
   tapi/drv_tapi_event.h \
   tapi/drv_tapi_ll_if_version.h \
   tapi/drv_tapi_ll_interface.h \
   tapi/drv_tapi_debug.h \
   tapi/drv_tapi_debug_buffer.h \
   tapi/drv_tapi_ppd.h \
   tapi/drv_tapi_ppd.c \
   tapi/drv_tapi_linux.h \
   tapi/drv_tapi_linux_ioctl.h \
   tapi/drv_tapi_linux_procfs.h \
   tapi/drv_tapi_linux_procfs_phone_detection.h \
   tapi/drv_tapi_version.h

NOT_USED_FOR_NOW = \
   tapi/drv_tapi_win.h \
   tapi/drv_tapi_vxworks.h \
   tapi/drv_tapi_vxworks.c \
   tapi/drv_tapi_vxworks_bsp.c

# From TAPI driver
if !IFXOS_ENABLED
IFXOS_LOCAL_SOURCE_FILES = \
   os_calls/ifxos_linux_event_drv.c \
   os_calls/ifxos_linux_copy_user_space_drv.c \
   os_calls/ifxos_linux_lock_drv.c \
   os_calls/ifxos_linux_memory_alloc_drv.c \
   os_calls/ifxos_linux_mutex_drv.c \
   os_calls/ifxos_linux_select_drv.c \
   os_calls/ifxos_linux_thread_drv.c \
   os_calls/ifxos_linux_time_drv.c \
   os_calls/ifxos_debug.c

EXTRA_DIST += $(IFXOS_LOCAL_SOURCE_FILES)

IFXOS_LOCAL_HEADER_FILES = \
   ../include/ifx_types.h \
   ../include/os_calls/ifxos_linux_drv.h \
   ../include/os_calls/ifxos_memory_alloc.h \
   ../include/os_calls/ifxos_copy_user_space.h \
   ../include/os_calls/ifxos_debug.h \
   ../include/os_calls/ifxos_event.h \
   ../include/os_calls/ifxos_select.h \
   ../include/os_calls/ifxos_sys_show.h \
   ../include/os_calls/ifxos_sys_show_interface.h \
   ../include/os_calls/ifxos_mutex.h \
   ../include/os_calls/ifxos_lock.h \
   ../include/os_calls/ifxos_time.h \
   ../include/os_calls/ifxos_common.h \
   ../include/os_calls/ifxos_version.h \
   ../include/os_calls/ifxos_print.h \
   ../include/os_calls/ifxos_thread.h \
   ../include/os_calls/ifxos_interrupt.h \
   ../include/os_calls/ifxos_rt_if_check.h \
   ../include/os_calls/linux/ifxos_linux_common.h \
   ../include/os_calls/linux/ifxos_linux_event.h \
   ../include/os_calls/linux/ifxos_linux_interrupt.h \
   ../include/os_calls/linux/ifxos_linux_lock.h \
   ../include/os_calls/linux/ifxos_linux_memory_alloc.h \
   ../include/os_calls/linux/ifxos_linux_mutex.h \
   ../include/os_calls/linux/ifxos_linux_print.h \
   ../include/os_calls/linux/ifxos_linux_select.h \
   ../include/os_calls/linux/ifxos_linux_thread.h \
   ../include/os_calls/linux/ifxos_linux_time.h

EXTRA_DIST += $(IFXOS_LOCAL_HEADER_FILES)

endif # !IFXOS_ENABLED

nodist_EXTRA_drv_tapi_dxs_ko_SOURCES =

nodist_drv_tapi_dxs_ko_SOURCES = \
   ../include/drv_config_defines.h

INCLUDES_NO_KERNEL = \
   -I@abs_top_builddir@/include \
   -I@abs_top_srcdir@/include \
   -I@abs_top_srcdir@/src/dxs \
   -I@abs_top_srcdir@/src/tapi

if !IFXOS_ENABLED
INCLUDES_NO_KERNEL += -I@abs_top_srcdir@/include/os_calls
endif

if EVENT_LOGGER_DEBUG
INCLUDES_NO_KERNEL += -I@EVENT_LOGGER_INCL_PATH@
endif

AM_CPPFLAGS = $(INCLUDES_NO_KERNEL)

if WITH_KERNEL_MODULE
AM_CPPFLAGS += -I@KERNEL_INCL_PATH@
endif

if IFXOS_ENABLED
AM_CPPFLAGS += -I@IFXOS_INCL_PATH@
endif

# Additional flags for DUSLIC driver compiled as library
libdrvdxs_additional_cflags = \
   -DLINUX \
   -DRW_MULTI_THREAD \
   -D_REENTRANT \
   -D_THREAD_SAFE \
   -D_POSIX_SOURCE -D_GNU_SOURCE \
   -DIFXOS_USE_DEV_IO=1

if WARNINGS
AM_CFLAGS = -Wall -Wimplicit -Wreturn-type -Wunused \
            -Wswitch -Wcomment -Wparentheses -Wmissing-prototypes
else
AM_CFLAGS = -Wall
endif

# Use old standard to keep compatibility with "unknown" old toolchains.
# gnu is used to compile linux code parts without warnings.
AM_CFLAGS += -std=gnu90

if WITH_KERNEL_MODULE

if USER_EXTRA_CFLAGS_SET
AM_CFLAGS += @USER_EXTRA_CFLAGS@
endif

drv_tapi_dxs_CFLAGS = -D__LINUX__ -DLINUX -D__KERNEL__ \
                      -DMODULE -DEXPORT_SYMTAB $(filter-out -std=gnu90,$(AM_CFLAGS)) \
                      -DTAPI_HAVE_TIMERS \
                      -fno-common -fno-strict-overflow -fno-delete-null-pointer-checks -fwrapv \
                      -Wno-error

drv_tapi_dxs_ko_LDFLAGS = -r -nostdlib

endif WITH_KERNEL_MODULE

.PHONY: lint
lint:
# Notes:
#  * do not put path to linux header includes as it breaks checking the driver code.
#  * you can add -v flag to get more detailed info about error
#  * cppcheck_out directory is used to store results and as lint build dir to speed up
#    consecutive checks
#  * -j1 is used to get information about unused functions / variables
#  * even deeper analysis --max-ctu-depth=10 --check-level=exhaustive --force
	@mkdir -p @abs_top_srcdir@/cppcheck_out
	@cppcheck \
		--enable=all \
		--language=c --std=c99 --platform=unix64 \
		--max-configs=12 -j1 --force\
		-DLINUX -D__LINUX__ -DHAVE_CONFIG_H -D__LITTLE_ENDIAN \
		-D__KERNEL__ -DMODULE -DEXPORT_SYMTAB -DTAPI_HAVE_TIMERS -DTAPI_HL_DRIVER \
		-UIFXOS_ENABLED -UVXWORKS -UWIN32 -UDRV_TAPI_NAME \
		--suppressions-list=@abs_top_srcdir@/dev/cppcheck_suppressions_list.txt \
		--inconclusive \
		--cppcheck-build-dir=@abs_top_srcdir@/cppcheck_out/ \
		--checkers-report=@abs_top_srcdir@/cppcheck_out/cppcheck_checkers_report.txt \
		--output-file=@abs_top_srcdir@/cppcheck_out/cppcheck_results.txt \
		-I@abs_top_srcdir@/include \
		@abs_top_srcdir@/src/

install-exec-hook: $(bin_PROGRAMS)
	@for p in $(bin_PROGRAMS); do \
		$(STRIP) --strip-debug $(DESTDIR)$(bindir)/$$p; \
		echo "  Stripping symbols: $(STRIP) --strip-debug $(DESTDIR)$(bindir)/$$p"; \
	done

if WITH_KERNEL_MODULE

drv_tapi_dxs_OBJS  = "$(subst .c,.o, $(drv_tapi_dxs_SOURCES))"

drv_tapi_dxs.ko: $(drv_tapi_dxs_SOURCES) $(nodist_drv_tapi_dxs_ko_SOURCES) $(nodist_EXTRA_drv_tapi_dxs_SOURCES) $(EXTRA_DIST)
	@echo -e "Making Linux kernel object"
	@for f in $(drv_tapi_dxs_SOURCES) ; do \
		if test ! -e $(PWD)/$$f; then \
			echo "LN $$f"; \
			mkdir -p $(PWD)/`dirname $$f`/; \
			ln -s @abs_srcdir@/$$f $(PWD)/$$f; \
		fi; \
	done;

	@echo -e "# drv_tapi_dxs: Generated to build Linux kernel object" > $(PWD)/Kbuild
	@echo -e "obj-m := $(subst .ko,.o,$@)" >> $(PWD)/Kbuild
	@echo -e "$(subst .ko,,$@)-y := $(drv_tapi_dxs_OBJS)" >> $(PWD)/Kbuild
	@echo -e "ccflags-y := -DHAVE_CONFIG_H  $(drv_tapi_dxs_CFLAGS) $(AM_CPPFLAGS)" >> $(PWD)/Kbuild

#	Kernel Makefile exports LDFLAGS, which might pull-in wrong settings
	LDFLAGS= \
	$(MAKE) ARCH=@KERNEL_ARCH@ -C @KERNEL_BUILD_PATH@ O=@KERNEL_BUILD_PATH@ \
           M=$(PWD) KBUILD_EXTRA_SYMBOLS="$(strip @KBUILD_EXTRA_SYMBOLS@)" \
           CROSS_COMPILE=@CROSS_COMPILE@ modules

clean-generic:
	@echo "drv_tapi_dxs: Cleanup Linux kernel object build"
	@- find . -name ".*.cmd" | xargs rm -f
	@- find . -name "*.o" | xargs rm -f
	@- find . -name "*.mod.c" | xargs rm -f
	@- find . -name "*.mod" | xargs rm -f
	@- rm -f Module.symvers Module.symvers.temp Kbuild modules.order
	@- rm -rf .tmp_versions

else
# library flags
libdrvdxstapi_a_CFLAGS = \
	$(libdrvdxstapi_additional_cflags) \
	$(AM_CFLAGS)

endif WITH_KERNEL_MODULE_FALSE
