/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ProSLIC DSP patches as firmware blobs.
 *
 * The Skyworks API expects its patches as compiled-in C arrays, which
 * means every chipset, revision and BOM variant is linked into the
 * module whether the board uses it or not. tools/proslic-patch2fw.py
 * turns those C files into blobs; this loads one back.
 */
#ifndef _EN75XX_PROSLIC_FW_H
#define _EN75XX_PROSLIC_FW_H

#include <linux/device.h>
#include <linux/types.h>

#include "proslic.h"

#define PROSLIC_FW_MAGIC	"PROSLICP"
#define PROSLIC_FW_VERSION	2
#define PROSLIC_FW_HDR_SIZE	56

/* Mirrors proslic.h; the loader refuses anything larger. */
#define PROSLIC_FW_MAX_DATA	1024
#define PROSLIC_FW_MAX_PSRAM	128
#define PROSLIC_FW_NUM_ENTRIES	16

/*
 * On-disk header. Field order follows the draft in the proslic_drivers2
 * branch (serial, then the four sizes, then the arrays); the magic,
 * version, CRC and identification are what make it safe to load blindly.
 * Everything is little-endian regardless of the host.
 */
struct proslic_fw_hdr {
	u8	magic[8];
	__le16	version;
	__le16	hdr_size;
	__le32	serial;
	__le16	n_data;		/* patchData words, includes terminator   */
	__le16	n_psdata;	/* psRamData words                        */
	__le16	n_psaddr;	/* psRamAddr words, includes terminator   */
	__le16	n_entries;	/* real jump-table entries: 8 or 16       */
	__le32	crc32;		/* of everything after this header        */
	char	chipset[8];	/* "si3219x"                              */
	char	revision[4];	/* "A"                                    */
	char	bom[12];	/* "LCQC", "FB", "BB", "TSS", "TSS_ISO"   */
	__le32	reserved;
} __packed;

struct en75xx_proslic_fw {
	ramData	*data;
	uInt16	*entries;
	uInt16	*ps_addr;
	ramData	*ps_data;
	u32	serial;
	char	name[64];
};

/* Fixed names consumed by the vendor Si3219x API during ProSLIC_Init(). */
extern proslicPatch si3219xPatchRevALCQC;
extern proslicPatch RevAPatch;

/*
 * Loads en75xx/proslic/<chipset>_<rev>_<bom>.fw, falling back to
 * <chipset>_<rev>.fw when @bom is NULL or the BOM-specific blob is
 * absent. @revision is a letter: 'A', 'B', 'C'.
 */
int en75xx_proslic_fw_load(struct device *dev, const char *chipset,
			   char revision, const char *bom,
			   struct en75xx_proslic_fw *fw);

void en75xx_proslic_fw_free(struct en75xx_proslic_fw *fw);

/* Fills a proslicPatch that the Skyworks API can consume. */
void en75xx_proslic_fw_to_patch(const struct en75xx_proslic_fw *fw,
				proslicPatch *patch);

#endif /* _EN75XX_PROSLIC_FW_H */
