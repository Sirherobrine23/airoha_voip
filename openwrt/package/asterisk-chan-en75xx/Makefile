#
# chan_en75xx: Asterisk channel driver for EN75xx FXS lines.
#
# Built out of tree against the asterisk package's headers, so it does
# not need a patched Asterisk source tree.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=asterisk-chan-en75xx
PKG_RELEASE:=2
PKG_LICENSE:=GPL-2.0-only
PKG_MAINTAINER:=Sirherobrine23

PKG_BUILD_DEPENDS:=asterisk

include $(INCLUDE_DIR)/package.mk

define Package/asterisk-chan-en75xx
  SUBMENU:=Telephony
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Asterisk channel driver for EN75xx FXS lines
  URL:=https://github.com/Sirherobrine23
  DEPENDS:=+asterisk +kmod-en75xx-pcm +asterisk-bridge-simple
endef

define Package/asterisk-chan-en75xx/description
 Exposes each /dev/en75xx-fxsN as an Asterisk channel of type EN75XX.
 Handles dial tone, DTMF collection, ring, answer and hangup, so an
 ordinary analog telephone behaves like one. The timing-critical parts
 (hook polling, ring cadence, G.711 companding) stay in the kernel.
endef

define Package/asterisk-chan-en75xx/conffiles
/etc/asterisk/en75xx.conf
endef

ASTERISK_DIR:=$(STAGING_DIR)/usr/include/asterisk
# The UAPI header travels with the module so the two cannot drift apart.
# Prefer the copy made by tools/install-openwrt.sh; when this repository is
# consumed directly as a src-link feed, fall back to the project-root header.
EN75XX_VOIP_PROJECT_DIR:=$(realpath $(CURDIR)/../../..)
EN75XX_VOIP_INSTALLED_UAPI:=$(TOPDIR)/package/kernel/en75xx-voip/include/uapi/linux
UAPI_DIR:=$(if $(wildcard $(EN75XX_VOIP_INSTALLED_UAPI)/en75xx_voice.h),$(EN75XX_VOIP_INSTALLED_UAPI),$(EN75XX_VOIP_PROJECT_DIR)/include/uapi/linux)

TARGET_CFLAGS += \
	-I$(STAGING_DIR)/usr/include \
	-I$(UAPI_DIR) \
	-fPIC -D_GNU_SOURCE -DAST_MODULE=\"chan_en75xx\" \
	-DAST_MODULE_SELF_SYM=__internal_chan_en75xx_self
TARGET_LDFLAGS += -lm

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/chan_en75xx.c $(PKG_BUILD_DIR)/
	$(CP) ./src/*.sample $(PKG_BUILD_DIR)/
	$(CP) $(UAPI_DIR)/en75xx_voice.h $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) -shared -o \
		$(PKG_BUILD_DIR)/chan_en75xx.so \
		$(PKG_BUILD_DIR)/chan_en75xx.c $(TARGET_LDFLAGS)
endef

define Package/asterisk-chan-en75xx/install
	$(INSTALL_DIR) $(1)/etc/init.d $(1)/etc/hotplug.d/misc $(1)/usr/sbin
	$(INSTALL_BIN) ./files/en75xx-fxs-perms.init $(1)/etc/init.d/en75xx-fxs-perms
	$(INSTALL_BIN) ./files/en75xx-fxs-led.init $(1)/etc/init.d/en75xx-fxs-led
	$(INSTALL_BIN) ./files/en75xx-fxs-led $(1)/usr/sbin/en75xx-fxs-led
	$(INSTALL_DATA) ./files/50-en75xx-fxs $(1)/etc/hotplug.d/misc/50-en75xx-fxs
	$(INSTALL_DIR) $(1)/usr/lib/asterisk/modules
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/chan_en75xx.so \
		$(1)/usr/lib/asterisk/modules/
	$(INSTALL_DIR) $(1)/etc/asterisk
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/en75xx.conf.sample \
		$(1)/etc/asterisk/en75xx.conf
endef

$(eval $(call BuildPackage,asterisk-chan-en75xx))
