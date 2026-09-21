#
# en75xx-fxs-bench-test: standalone ioctl/PCM exerciser for /dev/en75xx-fxsN.
#
# Single source file, no configure step; built directly against the UAPI
# header bundled with this package (see include/uapi/linux/en75xx_voice.h).
#
include $(TOPDIR)/rules.mk

PKG_NAME:=en75xx-fxs-bench-test
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0-only
PKG_MAINTAINER:=Sirherobrine23

include $(INCLUDE_DIR)/package.mk

define Package/en75xx-fxs-bench-test
  SUBMENU:=Voice over IP
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=EN75xx FXS bench/soak test tool
  URL:=https://github.com/Sirherobrine23
  DEPENDS:=+kmod-en75xx-pcm
endef

define Package/en75xx-fxs-bench-test/description
 Standalone ioctl/PCM exerciser for /dev/en75xx-fxsN: hook and ring
 control, and tone/record/play over the PCM path, with running stats
 (bytes, over/underruns, hook changes, DMA errors). Meant for bench
 bring-up and soak testing outside of Asterisk; stop Asterisk first,
 since both would open the same device.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./en75xx-test.c $(PKG_BUILD_DIR)/
	$(CP) ./include $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) \
		-o $(PKG_BUILD_DIR)/en75xx-test \
		$(PKG_BUILD_DIR)/en75xx-test.c -lm
endef

define Package/en75xx-fxs-bench-test/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/en75xx-test $(1)/usr/bin/en75xx-test
endef

$(eval $(call BuildPackage,en75xx-fxs-bench-test))
