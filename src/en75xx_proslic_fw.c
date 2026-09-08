// SPDX-License-Identifier: GPL-2.0
/*
 * Loader for ProSLIC DSP patches shipped as firmware blobs.
 *
 * Two things the loader has to get right:
 *
 *  - Endianness. The blobs are little-endian so one set of files works
 *    on both the ARM and the big-endian MIPS parts, which means the
 *    arrays are converted into freshly allocated native-endian buffers
 *    rather than pointed at in place.
 *
 *  - Terminators. ProSLIC_LoadPatchData() walks patchData until it hits
 *    a zero, and ProSLIC_LoadSupportRAM() walks psRamAddr the same way.
 *    The converter keeps the terminator, and this checks that it is
 *    still there before handing the arrays to the API.
 */

#include <linux/crc32.h>
#include <linux/ctype.h>
#include <linux/firmware.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "en75xx_proslic_fw.h"

static int proslic_fw_parse(struct device *dev, const struct firmware *blob,
			    const char *name, const char *chipset,
			    char revision, const char *bom,
			    struct en75xx_proslic_fw *fw)
{
	const struct proslic_fw_hdr *hdr;
	const __le32 *src32;
	const __le16 *src16;
	unsigned int n_data, n_psdata, n_psaddr, n_entries, i;
	size_t expect;
	u32 crc;

	if (blob->size < PROSLIC_FW_HDR_SIZE) {
		dev_err(dev, "%s: truncated (%zu bytes)\n", name, blob->size);
		return -EINVAL;
	}

	hdr = (const struct proslic_fw_hdr *)blob->data;

	if (memcmp(hdr->magic, PROSLIC_FW_MAGIC, sizeof(hdr->magic))) {
		dev_err(dev, "%s: bad magic\n", name);
		return -EINVAL;
	}
	if (le16_to_cpu(hdr->version) != PROSLIC_FW_VERSION) {
		dev_err(dev, "%s: version %u, this driver speaks %u\n",
			name, le16_to_cpu(hdr->version), PROSLIC_FW_VERSION);
		return -EINVAL;
	}
	if (le16_to_cpu(hdr->hdr_size) != PROSLIC_FW_HDR_SIZE) {
		dev_err(dev, "%s: header is %u bytes, expected %u\n",
			name, le16_to_cpu(hdr->hdr_size), PROSLIC_FW_HDR_SIZE);
		return -EINVAL;
	}

	/* A filename (including the fallback name) does not identify a patch. */
	if (strnlen(hdr->chipset, sizeof(hdr->chipset)) != strlen(chipset) ||
	    strncasecmp(hdr->chipset, chipset, sizeof(hdr->chipset)) ||
	    strnlen(hdr->revision, sizeof(hdr->revision)) != 1 ||
	    tolower(hdr->revision[0]) != tolower(revision) ||
	    (bom && *bom &&
	     (strnlen(hdr->bom, sizeof(hdr->bom)) != strlen(bom) ||
	      strncasecmp(hdr->bom, bom, sizeof(hdr->bom))))) {
		dev_err(dev, "%s: patch chipset, revision or BOM mismatch\n", name);
		return -EINVAL;
	}

	n_data = le16_to_cpu(hdr->n_data);
	n_psdata = le16_to_cpu(hdr->n_psdata);
	n_psaddr = le16_to_cpu(hdr->n_psaddr);
	n_entries = le16_to_cpu(hdr->n_entries);

	if (!n_data || n_data > PROSLIC_FW_MAX_DATA ||
	    !n_psaddr || n_psaddr > PROSLIC_FW_MAX_PSRAM ||
	    n_psdata != n_psaddr ||
	    (n_entries != 8 && n_entries != PROSLIC_FW_NUM_ENTRIES)) {
		dev_err(dev,
			"%s: implausible sizes (data %u, ps %u/%u, entries %u)\n",
			name, n_data, n_psaddr, n_psdata, n_entries);
		return -EINVAL;
	}

	/* the jump table is always stored padded to 16 */
	expect = (size_t)n_data * 4 + (size_t)n_psdata * 4 +
		 (size_t)n_psaddr * 2 + PROSLIC_FW_NUM_ENTRIES * 2;

	if (blob->size - PROSLIC_FW_HDR_SIZE != expect) {
		dev_err(dev, "%s: payload is %zu bytes, header implies %zu\n",
			name, blob->size - PROSLIC_FW_HDR_SIZE, expect);
		return -EINVAL;
	}

	/* Match binascii.crc32() in tools/proslic-patch2fw.py. */
	crc = crc32(~0U, blob->data + PROSLIC_FW_HDR_SIZE, expect) ^ ~0U;
	if (crc != le32_to_cpu(hdr->crc32)) {
		dev_err(dev, "%s: CRC mismatch (%#010x != %#010x)\n",
			name, crc, le32_to_cpu(hdr->crc32));
		return -EINVAL;
	}

	fw->data = kcalloc(n_data + 1, sizeof(*fw->data), GFP_KERNEL);
	fw->ps_data = kcalloc(n_psdata + 1, sizeof(*fw->ps_data), GFP_KERNEL);
	fw->ps_addr = kcalloc(n_psaddr + 1, sizeof(*fw->ps_addr), GFP_KERNEL);
	fw->entries = kcalloc(PROSLIC_FW_NUM_ENTRIES, sizeof(*fw->entries),
			      GFP_KERNEL);
	if (!fw->data || !fw->ps_data || !fw->ps_addr || !fw->entries) {
		en75xx_proslic_fw_free(fw);
		return -ENOMEM;
	}

	src32 = (const __le32 *)(blob->data + PROSLIC_FW_HDR_SIZE);
	for (i = 0; i < n_data; i++)
		fw->data[i] = le32_to_cpu(src32[i]);

	src32 += n_data;
	for (i = 0; i < n_psdata; i++)
		fw->ps_data[i] = le32_to_cpu(src32[i]);

	src16 = (const __le16 *)(src32 + n_psdata);
	for (i = 0; i < n_psaddr; i++)
		fw->ps_addr[i] = le16_to_cpu(src16[i]);

	src16 += n_psaddr;
	for (i = 0; i < PROSLIC_FW_NUM_ENTRIES; i++)
		fw->entries[i] = le16_to_cpu(src16[i]);

	if (fw->data[n_data - 1] != 0 || fw->ps_addr[n_psaddr - 1] != 0) {
		dev_err(dev, "%s: arrays are not zero-terminated\n", name);
		en75xx_proslic_fw_free(fw);
		return -EINVAL;
	}

	fw->serial = le32_to_cpu(hdr->serial);
	strscpy(fw->name, name, sizeof(fw->name));

	dev_info(dev,
		 "%s: %.8s rev %.4s bom %.12s, serial %#010x, %u patch words\n",
		 name, hdr->chipset, hdr->revision,
		 hdr->bom[0] ? hdr->bom : "-", fw->serial, n_data - 1);
	return 0;
}

int en75xx_proslic_fw_load(struct device *dev, const char *chipset,
			   char revision, const char *bom,
			   struct en75xx_proslic_fw *fw)
{
	const struct firmware *blob;
	char name[64];
	int ret;

	memset(fw, 0, sizeof(*fw));

	if (bom && *bom) {
		char lower[16];
		unsigned int i;

		for (i = 0; i < sizeof(lower) - 1 && bom[i]; i++)
			lower[i] = tolower(bom[i]);
		lower[i] = '\0';

		snprintf(name, sizeof(name), "en75xx/proslic/%s_%c_%s.fw",
			 chipset, tolower(revision), lower);
		ret = request_firmware(&blob, name, dev);
		if (!ret)
			goto parse;

		dev_dbg(dev, "%s not present, trying without the BOM\n", name);
	}

	snprintf(name, sizeof(name), "en75xx/proslic/%s_%c.fw",
		 chipset, tolower(revision));
	ret = request_firmware(&blob, name, dev);
	if (ret) {
		dev_err(dev, "no patch firmware for %s rev %c: %d\n",
			chipset, revision, ret);
		return ret;
	}

parse:
	ret = proslic_fw_parse(dev, blob, name, chipset, revision, bom, fw);
	release_firmware(blob);
	return ret;
}
EXPORT_SYMBOL_GPL(en75xx_proslic_fw_load);

void en75xx_proslic_fw_free(struct en75xx_proslic_fw *fw)
{
	if (!fw)
		return;

	kfree(fw->data);
	kfree(fw->ps_data);
	kfree(fw->ps_addr);
	kfree(fw->entries);
	memset(fw, 0, sizeof(*fw));
}
EXPORT_SYMBOL_GPL(en75xx_proslic_fw_free);

void en75xx_proslic_fw_to_patch(const struct en75xx_proslic_fw *fw,
				proslicPatch *patch)
{
	/*
	 * proslicPatch declares patchSerial const, so the whole struct is
	 * built here and copied in rather than assigned field by field.
	 */
	const proslicPatch built = {
		fw->data,
		fw->entries,
		fw->serial,
		fw->ps_addr,
		fw->ps_data,
	};

	memcpy(patch, &built, sizeof(*patch));
}
EXPORT_SYMBOL_GPL(en75xx_proslic_fw_to_patch);

/*
 * The API resolves its patches through fixed symbol names -- for the
 * Si3219x, SI3219X_PATCH_A maps to si3219xPatchRevALCQC and
 * SI3219X_PATCH_A_DEFAULT to RevAPatch. Those symbols are normally
 * provided by a compiled-in patch .c file. Defining them here, empty,
 * lets the driver fill them from firmware before ProSLIC_Init() runs.
 *
 * They are deliberately not const: the API only ever reads them, and
 * a const definition would land in .rodata where it could not be
 * populated at probe time.
 */
proslicPatch si3219xPatchRevALCQC;
EXPORT_SYMBOL_GPL(si3219xPatchRevALCQC);
proslicPatch si3218xPatchRevALCQC;
EXPORT_SYMBOL_GPL(si3218xPatchRevALCQC);
proslicPatch RevAPatch;
EXPORT_SYMBOL_GPL(RevAPatch);

MODULE_DESCRIPTION("ProSLIC patch firmware loader");
MODULE_LICENSE("GPL");
