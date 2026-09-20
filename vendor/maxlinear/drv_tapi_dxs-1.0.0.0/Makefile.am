#
#  Copyright 2013-2015 Lantiq Deutschland GmbH
#  Copyright 2015-2016 Lantiq Beteiligungs-GmbH & Co.KG
#  Copyright 2016-2020 Intel Corporation.
#  Copyright 2020-2024 MaxLinear, Inc.
#
#  For licensing information, see the file 'LICENSE' in the root folder of
#  this software module.

#

AUTOMAKE_OPTIONS = foreign 1.7 nostdinc
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

# set distcheck options - see configure.ac for list of modifications
DISTCHECK_CONFIGURE_FLAGS=@CONFIGURE_OPTIONS@
DISTCLEANFILES = src/drv_dxs_dxs.mod \
				 src/dxs/drv_config_user.h \
				 src/modules.order \
				 dev

drv_tapi_dxs_docdir = ${docdir}

drv_tapi_dxs_doc_DATA = \
	README.md\
	LICENSE

EXTRA_DIST = $(drv_tapi_dxs_doc_DATA) \
	ChangeLog \
	doc/dxs/doxyconfig \
	doc/dxs/lantiq,duslicxs.txt \
	doc/tapi/doxyconfig_v3.dxy \
	doc/tapi/doxyconfig_v4.dxy \
	doc/footer.html \
	doc/header.html \
	doc/MaxLinear-logo.png \
	doc/stylesheet.css \
	script/tapi/addLibtool.sh \
	drv_tapi_dxs_version.h

if LINUX
if WITH_KERNEL_MODULE
EXT = .ko
endif
endif

# Make subdirectories inside tar.gz writable for everyone
# (required for Linux kbuild)
dist-hook:
	find $(distdir) -type d -exec chmod 777 {} \; -print

# Create self extracting linux distribution
distcheck-hook:
	cp $(distdir)/src/dxs/drv_config_user.template.h $(distdir)/_build/drv_config_user.h
	@echo "Make subdirectories writable... "; \
	find $(distdir) -type d -exec chmod a+w {} \; -print
	@echo -n "Checking for the line ending style... "; \
	failure_list="`find $(distdir) \( -path '$(distdir)/doc' \) -prune -o -type f -exec grep -UIqsc $$'\r$$' {} \; -printf '\tDOS line ending style: %p\n'`"; \
	if test -z "$$failure_list"; then \
		echo "Ok"; \
	else \
		echo "FAIL"; \
		echo "$$failure_list"; \
		echo; \
		exit -1; \
	fi

clean-generic:
	@- rm -f Module.symvers.*
	@- rm -rf tmp/

.PHONY: lint
lint:
	@target=lint; \
    list='$(SUBDIRS)'; \
    for subdir in $$list; do \
       echo "Making $$target in $$subdir"; \
       if test "$$subdir" = "."; then \
          dot_seen=yes; \
          local_target="$$target-am"; \
       else \
          local_target="$$target"; \
       fi; \
       (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
       || eval $$failcom; \
     done;

.PHONY: errno
errno: errno-tapi errno-dxs

.PHONY: errno-dxs
errno-dxs:
	@top_srcdir@/script/dxs/updateErrno.pl @top_srcdir@/include/drv_dxs_errno.h @top_srcdir@/include/drv_dxs_strerrno.h @top_srcdir@/src/dxs

.PHONY: errno-tapi
errno-tapi:
	@top_srcdir@/script/tapi/updateErrno.pl @top_srcdir@/include/drv_tapi_errno.h @top_srcdir@/include/drv_tapi_strerrno.h @top_srcdir@/src/tapi

.PHONY: style
style:
	find @top_srcdir@/src -type f -name '*.[hc]' -exec @top_srcdir@/script/tapi/checkStyle.pl {} \;

.PHONY: checkstack
checkstack:
	( \
		if test -z "$(host_alias)" ; then \
			objdump -d src/drv_tapi_dxs$(EXT) | @top_srcdir@/script/tapi/checkstack.pl ; \
		else \
			# Add path to toolchain to $PATH in case of not found binary \
			# export PATH=<path to toolchain>:$path \
			$(host_alias)-objdump -d src/drv_tapi_dxs$(EXT) | @top_srcdir@/script/tapi/checkstack.pl $(host_alias) ; \
		fi ; \
	)

.PHONY: examples
examples:
	@@top_srcdir@/script/tapi/checkExamples.pl @top_srcdir@/src/tapi
