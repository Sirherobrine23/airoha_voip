#
# Firmware blobs for the EN75xx voice stack.
#
# Included from the kmod Makefile; kept separate so the blob list does
# not clutter the module definitions.
#
define Package/en75xx-proslic-firmware
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=ProSLIC DSP patches for EN75xx FXS
  DEPENDS:=+kmod-en75xx-slic-si3219x
endef

define Package/en75xx-proslic-firmware/description
 DSP patch images for the Skyworks ProSLIC parts, converted from the
 ProSLIC API C sources by tools/proslic-patch2fw.py. Install only the
 blob your board needs; the driver picks it by chipset, revision and
 BOM variant.
endef

define Package/en75xx-proslic-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware/en75xx/proslic
	$(INSTALL_DATA) ./firmware/proslic/*.fw \
		$(1)/lib/firmware/en75xx/proslic/
endef

define Package/en75xx-dxs-firmware
  SECTION:=firmware
  CATEGORY:=Firmware
  TITLE:=MaxLinear DUSLIC-XS firmware
endef

define Package/en75xx-dxs-firmware/description
 PRAM patch and BBD coefficient blobs for the PEF32001/PEF32002. The
 PRAM patch is optional -- without it the chip runs its ROM firmware --
 but the BBD is not.
endef

define Package/en75xx-dxs-firmware/install
	$(INSTALL_DIR) $(1)/lib/firmware/airoha-voice
	$(INSTALL_DATA) ./firmware/dxs/DXS_FW.bin $(1)/lib/firmware/airoha-voice/
	$(INSTALL_DATA) ./firmware/dxs/DXS_BBD.bin $(1)/lib/firmware/airoha-voice/
	$(INSTALL_DIR) $(1)/lib/firmware/voice
	$(LN) /lib/firmware/airoha-voice/DXS_FW.bin \
		$(1)/lib/firmware/voice/dxs_firmware.bin
	$(LN) /lib/firmware/airoha-voice/DXS_BBD.bin \
		$(1)/lib/firmware/voice/dxs_bbd.bin
endef
