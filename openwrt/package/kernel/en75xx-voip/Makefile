#
# EN75xx voice stack (PCM/TDM + FXS SLIC drivers) for OpenWrt.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=en75xx-voip
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0-only
PKG_MAINTAINER:=Sirherobrine23

include $(INCLUDE_DIR)/package.mk

define KernelPackage/en75xx-pcm
  SUBMENU:=Voice over IP
  TITLE:=EcoNet/Airoha EN75xx PCM/TDM controller
  DEPENDS:=@TARGET_econet||TARGET_airoha
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
  TITLE:=Skyworks Si3218x/Si3219x ProSLIC FXS (SPI)
  DEPENDS:=+kmod-en75xx-pcm
  FILES:=$(PKG_BUILD_DIR)/en75xx-slic-si3219x.ko
  AUTOLOAD:=$(call AutoLoad,51,en75xx-slic-si3219x)
endef

define KernelPackage/en75xx-slic-si3219x/description
 ProSLIC FXS driver built on the Skyworks ProSLIC API. Covers the
 Si32192 used on the TP-Link XX230v and XC220-G3v boards.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/. $(PKG_BUILD_DIR)/
	$(CP) ./include $(PKG_BUILD_DIR)/../include
	$(CP) ./vendor $(PKG_BUILD_DIR)/../vendor
endef

define Build/Compile
	+$(KERNEL_MAKE) M=$(PKG_BUILD_DIR) modules
endef

$(eval $(call KernelPackage,en75xx-pcm))
$(eval $(call KernelPackage,en75xx-slic-le9642))
$(eval $(call KernelPackage,en75xx-slic-si3219x))
