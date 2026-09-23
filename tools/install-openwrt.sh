#!/bin/sh
# SPDX-License-Identifier: GPL-2.0
set -eu

if [ "$#" -ne 1 ]; then
	echo "usage: $0 /path/to/openwrt" >&2
	exit 2
fi

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
openwrt_dir=$1

if [ ! -f "$openwrt_dir/rules.mk" ] || [ ! -d "$openwrt_dir/package" ]; then
	echo "not an OpenWrt source tree: $openwrt_dir" >&2
	exit 1
fi

kernel_pkg="$openwrt_dir/package/kernel/en75xx-voip"
asterisk_pkg="$openwrt_dir/package/network/services/asterisk-chan-en75xx"
bench_pkg="$openwrt_dir/package/utils/en75xx-fxs-bench-test"

mkdir -p "$kernel_pkg" "$asterisk_pkg" "$bench_pkg"
cp -a "$project_dir/openwrt/package/kernel/en75xx-voip/." "$kernel_pkg/"
cp -a "$project_dir/src" "$project_dir/include" "$project_dir/vendor" \
	"$project_dir/firmware" "$kernel_pkg/"
cp -a "$project_dir/openwrt/package/asterisk-chan-en75xx/." "$asterisk_pkg/"
cp -a "$project_dir/openwrt/package/en75xx-fxs-bench-test/." "$bench_pkg/"

echo "Installed EN75xx kernel, Asterisk, and bench-test packages into $openwrt_dir"
