# SPDX-License-Identifier: GPL-2.0
#
# Out-of-tree build of the EN75xx voice stack.
#
#   make -C /path/to/linux M=$PWD modules
#
# Module split, and why:
#   en75xx-pcm     the TDM/DMA engine; everything else is a client
#   en75xx-voice   /dev/en75xx-fxsN and the line registry
#   en75xx-zsi     ZSI transport (Le9642 and friends)
#   en75xx-isi-spi ISI transport, exposed as a spi_controller (ProSLIC)
#   en75xx-slic-*  one per SLIC family

obj-m += en75xx-pcm.o
obj-m += en75xx-voice.o
obj-m += en75xx-zsi.o
obj-m += en75xx-isi-spi.o
obj-m += en75xx-slic-le9642.o
obj-m += en75xx-slic-si3219x.o

en75xx-pcm-objs            := en75xx_pcm.o
en75xx-voice-objs          := en75xx_voice_core.o
en75xx-zsi-objs            := en75xx_zsi.o
en75xx-isi-spi-objs        := en75xx_isi_spi.o
en75xx-slic-le9642-objs    := en75xx_slic_le9642.o

# The ProSLIC driver links the Skyworks API sources from vendor/.
PROSLIC_DIR := ../vendor/proslic
#
# The patch is NOT compiled in: it is loaded from
# en75xx/proslic/si3219x_<rev>[_<bom>].fw at probe time, which is what
# en75xx_proslic_fw.o provides. That keeps one module working across
# revisions and BOM variants instead of one build per variant.
#
en75xx-slic-si3219x-objs   := en75xx_slic_si3219x.o en75xx_proslic_fw.o \
	$(PROSLIC_DIR)/src/proslic.o \
	$(PROSLIC_DIR)/src/si_voice.o \
	$(PROSLIC_DIR)/src/si_voice_version.o \
	$(PROSLIC_DIR)/src/si3219x_intf.o \
	$(PROSLIC_DIR)/custom/si3219x_LCCB_constants.o

ccflags-y += -I$(src)/../include
ccflags-y += -I$(src)/$(PROSLIC_DIR)/inc
ccflags-y += -I$(src)/$(PROSLIC_DIR)/custom
ccflags-y += -DSI3219X -DPROSLIC_LINUX_KERNEL

# The vendor API predates a few warnings that are errors in modern trees.
CFLAGS_proslic.o                  += -Wno-unused-but-set-variable
CFLAGS_proslic.o                  += -Wno-missing-prototypes
CFLAGS_si3219x_intf.o             += -Wno-unused-but-set-variable
