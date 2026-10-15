#!/bin/sh
# Build vendor TAPI first, then the EN75xx stack with its exported symbols.
set -eu
: "${KERNEL_DIR:?set KERNEL_DIR to a configured/prepared kernel tree}"
repo=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build=${DXS_BUILD_DIR:-"$repo/build/dxs"}
mkdir -p "$build"
cd "$build"
"$repo/vendor/maxlinear/drv_tapi_dxs-1.0.0.0/configure" \
	--with-kernel-incl="$KERNEL_DIR/include" \
	--with-kernel-build="$KERNEL_DIR" \
	--with-kernel-module --enable-tapi3 --enable-kernel-api \
	--enable-dcdc-hw=CIBB12 --with-max-devices=2 "$@"
make -j"${JOBS:-4}" ACLOCAL=true AUTOCONF=true AUTOHEADER=true AUTOMAKE=true
make -C "$KERNEL_DIR" M="$repo/src" \
	EN75XX_DXS_BUILD="$build" \
	KBUILD_EXTRA_SYMBOLS="$build/src/Module.symvers" -j"${JOBS:-4}" modules
