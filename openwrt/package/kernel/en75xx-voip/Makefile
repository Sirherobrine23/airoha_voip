#
# EN75xx voice stack (PCM/TDM + FXS SLIC drivers) for OpenWrt.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=en75xx-voip
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0-only
PKG_MAINTAINER:=Sirherobrine23

include $(INCLUDE_DIR)/kernel.mk
include $(INCLUDE_DIR)/package.mk

# The package can be consumed in two ways:
#  1. tools/install-openwrt.sh copies src/include/vendor/firmware here; or
#  2. the repository itself is used as an OpenWrt src-link feed.
#
# In the second case GNU make resolves the package/feeds symlink before setting
# CURDIR, so walking four directories up from this Makefile reaches the project
# root (openwrt/package/kernel/en75xx-voip -> project root).
EN75XX_VOIP_SOURCE_DIR:=$(if $(wildcard $(CURDIR)/src/Makefile),$(CURDIR),$(realpath $(CURDIR)/../../../..))

include ./firmware.mk

define KernelPackage/en75xx-lec
  SUBMENU:=Voice over IP
  TITLE:=OSLEC line echo canceller
  DEPENDS:=@TARGET_econet||TARGET_airoha
  FILES:=$(PKG_BUILD_DIR)/en75xx-lec.ko
  AUTOLOAD:=$(call AutoLoad,49,en75xx-lec)
endef

define KernelPackage/en75xx-pcm
  SUBMENU:=Voice over IP
  TITLE:=EcoNet/Airoha EN75xx PCM/TDM controller
  DEPENDS:=@TARGET_econet||TARGET_airoha +kmod-en75xx-lec
  FILES:= \
	$(PKG_BUILD_DIR)/en75xx-pcm.ko \
	$(PKG_BUILD_DIR)/en75xx-voice.ko
  AUTOLOAD:=$(call AutoLoad,50,en75xx-pcm en75xx-voice)
endef

define KernelPackage/en75xx-pcm/description
 TDM engine behind the FXS ports on EN751221, EN7528 and EN7523, plus
 the /dev/en75xx-fxsN character device that carries 8 kHz 16-bit linear
 audio to userspace.
endef

define KernelPackage/en75xx-slic-le9642
  SUBMENU:=Voice over IP
  TITLE:=Microsemi Le9641/Le9642 FXS (ZSI)
  DEPENDS:=+kmod-en75xx-pcm
  FILES:= \
	$(PKG_BUILD_DIR)/en75xx-zsi.ko \
	$(PKG_BUILD_DIR)/en75xx-slic-le9642.ko
  AUTOLOAD:=$(call AutoLoad,51,en75xx-zsi en75xx-slic-le9642)
endef

define KernelPackage/en75xx-slic-le9642/description
 Le9641/Le9642 miSLIC driven over ZSI. Loads the OEM raw MPI profiles,
 runs the codec in G.711 and exposes hook, ring and line feed.
endef

define KernelPackage/en75xx-slic-si3219x
  SUBMENU:=Voice over IP
  TITLE:=Skyworks Si3218x/Si3219x ProSLIC FXS (SPI/ISI)
  DEPENDS:=+kmod-en75xx-pcm
  FILES:= \
	$(PKG_BUILD_DIR)/en75xx-isi-spi.ko \
	$(PKG_BUILD_DIR)/en75xx-slic-si3219x.ko
  AUTOLOAD:=$(call AutoLoad,51,en75xx-isi-spi en75xx-slic-si3219x)
endef

define KernelPackage/en75xx-slic-si3219x/description
 ProSLIC FXS driver built on the Skyworks ProSLIC API, plus the ISI
 transport (en75xx-isi-spi, an spi_controller) that Si32192/Si32193
 boards need underneath it -- Si32184/Si3228x boards on a real SPI bus
 don't need it, but loading it costs nothing since it only binds to its
 own DT nodes. Covers the Si32192 used on the TP-Link XX230v and
 XC220-G3v boards.
endef

define KernelPackage/en75xx-slic-dxs-cibb12
  SUBMENU:=Voice over IP
  TITLE:=MaxLinear PEF32001/PEF32002 TAPI (CIBB12)
  DEPENDS:=+kmod-en75xx-pcm +en75xx-dxs-firmware
  FILES:=$(PKG_BUILD_DIR)/dxs-build/src/drv_tapi_dxs.ko
endef

define KernelPackage/en75xx-slic-dxs-cibb12/description
 MaxLinear standalone TAPI driver for the PEF32001/DXS101 and
 PEF32002/DXS102, compiled specifically for the CIBB12 converter topology.
 It is intentionally not autoloaded: firmware, BBD, reset polarity and PCM
 slots must be selected from the board design before starting the line.
endef

# The blobs live next to the sources so the converter output and the
# packaged firmware cannot drift apart.
define Build/Prepare
	test -f $(EN75XX_VOIP_SOURCE_DIR)/src/Makefile || \
		{ echo "EN75xx VoIP source tree not found: $(EN75XX_VOIP_SOURCE_DIR)" >&2; exit 1; }
	mkdir -p $(PKG_BUILD_DIR) $(PKG_BUILD_DIR)/../include $(PKG_BUILD_DIR)/../vendor
	$(CP) $(EN75XX_VOIP_SOURCE_DIR)/src/. $(PKG_BUILD_DIR)/
	$(CP) $(EN75XX_VOIP_SOURCE_DIR)/include/. $(PKG_BUILD_DIR)/../include/
	$(CP) $(EN75XX_VOIP_SOURCE_DIR)/vendor/. $(PKG_BUILD_DIR)/../vendor/
endef

DXS_SRC_DIR:=$(PKG_BUILD_DIR)/../vendor/maxlinear/drv_tapi_dxs-1.0.0.0
DXS_BUILD_DIR:=$(PKG_BUILD_DIR)/dxs-build
DXS_SELECTED:=$(CONFIG_PACKAGE_kmod-en75xx-slic-dxs-cibb12)

define Build/Configure
	$(call Build/Configure/Default)
	$(if $(DXS_SELECTED),mkdir -p $(DXS_BUILD_DIR))
	$(if $(DXS_SELECTED),(cd $(DXS_BUILD_DIR); \
		$(TARGET_CONFIGURE_OPTS) \
		ARCH=$(LINUX_KARCH) \
		$(DXS_SRC_DIR)/configure \
			--build=$(GNU_HOST_NAME) \
			--host=$(GNU_TARGET_NAME) \
			--target=$(GNU_TARGET_NAME) \
			--with-kernel-incl=$(LINUX_DIR) \
			--with-kernel-build=$(LINUX_DIR) \
			--with-kernel-module \
			--enable-tapi3 \
			--enable-kernel-api \
			--enable-dcdc-hw=CIBB12 \
			--with-max-devices=4))
endef

define Build/Compile
	+$(KERNEL_MAKE) M=$(PKG_BUILD_DIR) modules
	$(if $(DXS_SELECTED),$(MAKE) $(PKG_JOBS) -C $(DXS_BUILD_DIR))
endef

$(eval $(call KernelPackage,en75xx-lec))
$(eval $(call KernelPackage,en75xx-pcm))
$(eval $(call KernelPackage,en75xx-slic-le9642))
$(eval $(call KernelPackage,en75xx-slic-si3219x))
$(eval $(call KernelPackage,en75xx-slic-dxs-cibb12))
$(eval $(call BuildPackage,en75xx-proslic-firmware))
$(eval $(call BuildPackage,en75xx-dxs-firmware))
