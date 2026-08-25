// SPDX-License-Identifier: GPL-2.0
/*
 * ZSI (Zarlink Serial Interface) transport for EN75xx SLICs.
 *
 * ZSI multiplexes the SLIC control channel over the PCM bus. Two
 * consequences drive the whole design of this file:
 *
 *  - The SoC must be in ZSI mode and the PCM engine must be clocking
 *    PCLK/FSYNC before the SLIC answers anything. The PCM *DMA* however
 *    must NOT be running for control transactions -- arming the rings
 *    stops the SLIC from replying.
 *
 *  - The wrapper's tx-ack only means "the wrapper sent the byte". The
 *    SLIC still has to clock it over the 8 kHz bus, so every byte needs
 *    an inter-byte gap of roughly 5 ms. Without it the SLIC replies
 *    garbage (0xf1). The gap lives here, in the driver, so that it
 *    cannot be skipped by a caller.
 *
 * Register map reverse-engineered from the vendor spi.ko
 * (ZSI_bytes_read/ZSI_bytes_write) and verified by live register poking.
 */

#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/slab.h>

#include "en75xx_zsi.h"

/* ZSI wrapper (0x1fbd1000 + id * 0x2000) */
#define ZSI_CFG				0x000
#define ZSI_TXRX			0x004
#define ZSI_CTL				0x008
#define  ZSI_CTL_READ_REQ		BIT(0)
#define  ZSI_CTL_TX_ACK			BIT(1)	/* write 1 to clear */
#define  ZSI_CTL_RX_READY		BIT(2)	/* write 1 to clear */
#define ZSI_RX				0x00c
#define ZSI_EN				0x010

#define ZSI_CFG_VAL			0x0f030313u	/* OEM stock value */
#define ZSI_EN_VAL			0x19

/* SYS block (0x1fb00000) */
#define SYS_CHIP_ID			0x064
#define SYS_IFACE_MODE			0x094
#define  SYS_IFACE_MODE_MASK		GENMASK(3, 0)
#define  SYS_IFACE_MODE_ZSI		0x5
#define SYS_RESET			0x834
#define  SYS_RESET_SLIC0		BIT(0)
#define  SYS_RESET_SLIC1		BIT(17)
/*
 * Bit 25 is the *SPI* reset. Toggling it here looks like it works and
 * then leaves the SLIC unreachable; the SLIC lines are bit 0 and 17.
 */

/* Chip SCU block (0x1fa20000) */
#define SCU_IOMUX_CONTROL1		0x104
#define  IOMUX1_SLIC_MASK		0x7d00
#define  IOMUX1_GPIO_ZSI_ISI		0x2000
#define SCU_PCM_CLK_DIV			0x0d4
#define SCU_PCM_CLK_OUT			0x0d8
#define SCU_PCM_CLK_SRC_SEL		0x148
#define  PCM_CLK_SRC_ZSI		0x1c

#define SCU_PCM_CLK_OUT_VAL		0x00a00301
#define SCU_PCM_CLK_DIV_VAL		0x00000008

#define ZSI_POLL_ITERS			20000	/* x 1 us => up to 20 ms */

/*
 * VP886 sub-command that clears the MPI buffer. A register read must
 * send it after the register opcode, otherwise the first byte returned
 * is a framing prefix (0x03) and the real data is shifted by one.
 */
#define VP886_NOOP			0x06
#define VP886_EC_REG_WRT		0x4a

struct en75xx_zsi {
	struct device		*dev;
	void __iomem		*base;
	void __iomem		*sys;
	void __iomem		*scu;
	struct mutex		lock;
	struct list_head	node;
	unsigned int		gap_us;
	bool			hw_ready;
};

static LIST_HEAD(en75xx_zsi_list);
static DEFINE_MUTEX(en75xx_zsi_list_lock);

static unsigned int zsi_gap_us = 5000;
module_param(zsi_gap_us, uint, 0644);
MODULE_PARM_DESC(zsi_gap_us,
	"inter-byte gap in us (default 5000; below ~2000 the SLIC mis-parses "
	"profile streams)");

/* ------------------------------------------------------------------ */
/* Byte level                                                          */
/* ------------------------------------------------------------------ */

static int zsi_poll_set(struct en75xx_zsi *zsi, u32 bit)
{
	unsigned int i;

	for (i = 0; i < ZSI_POLL_ITERS; i++) {
		if (readl(zsi->base + ZSI_CTL) & bit)
			return 0;
		udelay(1);
	}
	return -ETIMEDOUT;
}

static int zsi_write_byte(struct en75xx_zsi *zsi, u8 v)
{
	int ret;

	writel(v, zsi->base + ZSI_TXRX);
	ret = zsi_poll_set(zsi, ZSI_CTL_TX_ACK);
	if (ret) {
		dev_err_ratelimited(zsi->dev, "ZSI tx timeout (0x%02x)\n", v);
		return ret;
	}
	writel(ZSI_CTL_TX_ACK, zsi->base + ZSI_CTL);	/* W1C */

	/* the SLIC still has to clock this byte over the 8 kHz PCM bus */
	usleep_range(zsi->gap_us, zsi->gap_us + 1000);
	return 0;
}

static int zsi_read_byte(struct en75xx_zsi *zsi, u8 *v)
{
	int ret;

	writel(ZSI_CTL_READ_REQ, zsi->base + ZSI_CTL);
	ret = zsi_poll_set(zsi, ZSI_CTL_RX_READY);
	if (ret) {
		dev_err_ratelimited(zsi->dev, "ZSI rx timeout\n");
		return ret;
	}

	*v = readl(zsi->base + ZSI_RX) & 0xff;
	writel(ZSI_CTL_RX_READY, zsi->base + ZSI_CTL);	/* W1C */

	usleep_range(zsi->gap_us, zsi->gap_us + 1000);
	return 0;
}

/* ------------------------------------------------------------------ */
/* Hardware bring-up                                                   */
/* ------------------------------------------------------------------ */

int en75xx_zsi_hw_init(struct en75xx_zsi *zsi)
{
	u32 v;

	if (!zsi->sys || !zsi->scu)
		return -ENODEV;

	mutex_lock(&zsi->lock);

	/* interface route = ZSI */
	v = readl(zsi->sys + SYS_IFACE_MODE);
	v &= ~SYS_IFACE_MODE_MASK;
	v |= SYS_IFACE_MODE_ZSI;
	writel(v, zsi->sys + SYS_IFACE_MODE);

	/* pinmux: clear the SLIC field, select GPIO_ZSI_ISI */
	v = readl(zsi->scu + SCU_IOMUX_CONTROL1);
	v &= ~IOMUX1_SLIC_MASK;
	v |= IOMUX1_GPIO_ZSI_ISI;
	writel(v, zsi->scu + SCU_IOMUX_CONTROL1);

	/* PCM clock source = ZSI */
	v = readl(zsi->scu + SCU_PCM_CLK_SRC_SEL);
	writel(v | PCM_CLK_SRC_ZSI, zsi->scu + SCU_PCM_CLK_SRC_SEL);

	/* PCLK/FSYNC must run for the SLIC to answer at all */
	writel(SCU_PCM_CLK_DIV_VAL, zsi->scu + SCU_PCM_CLK_DIV);
	writel(SCU_PCM_CLK_OUT_VAL, zsi->scu + SCU_PCM_CLK_OUT);

	/* wrapper config + enable */
	writel(ZSI_CFG_VAL, zsi->base + ZSI_CFG);
	writel(readl(zsi->base + ZSI_EN) | ZSI_EN_VAL, zsi->base + ZSI_EN);

	zsi->hw_ready = true;
	mutex_unlock(&zsi->lock);

	dev_dbg(zsi->dev, "ZSI mode selected, wrapper enabled\n");
	return 0;
}
EXPORT_SYMBOL_GPL(en75xx_zsi_hw_init);

void en75xx_zsi_slic_reset(struct en75xx_zsi *zsi)
{
	u32 mask = SYS_RESET_SLIC0 | SYS_RESET_SLIC1;
	u32 v;

	if (!zsi->sys)
		return;

	mutex_lock(&zsi->lock);
	v = readl(zsi->sys + SYS_RESET);
	writel(v & ~mask, zsi->sys + SYS_RESET);
	usleep_range(5000, 6000);
	writel(v | mask, zsi->sys + SYS_RESET);
	usleep_range(5000, 6000);
	writel(v & ~mask, zsi->sys + SYS_RESET);
	usleep_range(20000, 25000);

	/* the reset drops the wrapper enable, put it back */
	writel(readl(zsi->base + ZSI_EN) | ZSI_EN_VAL, zsi->base + ZSI_EN);
	mutex_unlock(&zsi->lock);
}
EXPORT_SYMBOL_GPL(en75xx_zsi_slic_reset);

/* ------------------------------------------------------------------ */
/* MPI level                                                           */
/* ------------------------------------------------------------------ */

int en75xx_zsi_write(struct en75xx_zsi *zsi, const u8 *buf, size_t len)
{
	size_t i;
	int ret = 0;

	if (!zsi->hw_ready)
		return -EIO;

	mutex_lock(&zsi->lock);
	for (i = 0; i < len; i++) {
		ret = zsi_write_byte(zsi, buf[i]);
		if (ret)
			break;
	}
	mutex_unlock(&zsi->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(en75xx_zsi_write);

static int zsi_select_ec_locked(struct en75xx_zsi *zsi, u8 ec)
{
	int ret;

	if (!ec)
		return 0;

	ret = zsi_write_byte(zsi, VP886_EC_REG_WRT);
	if (ret)
		return ret;
	return zsi_write_byte(zsi, ec);
}

int en75xx_zsi_read_reg(struct en75xx_zsi *zsi, u8 ec, u8 reg,
			u8 *buf, size_t len)
{
	size_t i;
	int ret;

	if (!zsi->hw_ready)
		return -EIO;

	mutex_lock(&zsi->lock);

	ret = zsi_select_ec_locked(zsi, ec);
	if (ret)
		goto out;

	ret = zsi_write_byte(zsi, reg);
	if (ret)
		goto out;

	/* aligns the read framing; without it every byte is off by one */
	ret = zsi_write_byte(zsi, VP886_NOOP);
	if (ret)
		goto out;

	for (i = 0; i < len; i++) {
		ret = zsi_read_byte(zsi, &buf[i]);
		if (ret)
			goto out;
	}
out:
	mutex_unlock(&zsi->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(en75xx_zsi_read_reg);

int en75xx_zsi_write_reg(struct en75xx_zsi *zsi, u8 ec, u8 reg,
			 const u8 *buf, size_t len)
{
	size_t i;
	int ret;

	if (!zsi->hw_ready)
		return -EIO;

	mutex_lock(&zsi->lock);

	ret = zsi_select_ec_locked(zsi, ec);
	if (ret)
		goto out;

	ret = zsi_write_byte(zsi, reg);
	if (ret)
		goto out;

	for (i = 0; i < len; i++) {
		ret = zsi_write_byte(zsi, buf[i]);
		if (ret)
			goto out;
	}
out:
	mutex_unlock(&zsi->lock);
	return ret;
}
EXPORT_SYMBOL_GPL(en75xx_zsi_write_reg);

/* ------------------------------------------------------------------ */
/* Lookup and platform driver                                          */
/* ------------------------------------------------------------------ */

struct en75xx_zsi *en75xx_zsi_get(struct device *dev, const char *phandle_name)
{
	struct device_node *np;
	struct en75xx_zsi *zsi, *found = NULL;

	np = of_parse_phandle(dev->of_node, phandle_name, 0);
	if (!np)
		return ERR_PTR(-ENODEV);

	mutex_lock(&en75xx_zsi_list_lock);
	list_for_each_entry(zsi, &en75xx_zsi_list, node) {
		if (zsi->dev->of_node == np) {
			found = zsi;
			break;
		}
	}
	mutex_unlock(&en75xx_zsi_list_lock);
	of_node_put(np);

	if (!found)
		return ERR_PTR(-EPROBE_DEFER);

	get_device(found->dev);
	return found;
}
EXPORT_SYMBOL_GPL(en75xx_zsi_get);

void en75xx_zsi_put(struct en75xx_zsi *zsi)
{
	if (!IS_ERR_OR_NULL(zsi))
		put_device(zsi->dev);
}
EXPORT_SYMBOL_GPL(en75xx_zsi_put);

static void __iomem *zsi_map_named(struct platform_device *pdev,
				   const char *name)
{
	struct resource *res;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	if (!res)
		return NULL;
	return devm_ioremap_resource(&pdev->dev, res);
}

static int en75xx_zsi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct en75xx_zsi *zsi;

	zsi = devm_kzalloc(dev, sizeof(*zsi), GFP_KERNEL);
	if (!zsi)
		return -ENOMEM;

	zsi->dev = dev;
	mutex_init(&zsi->lock);
	INIT_LIST_HEAD(&zsi->node);
	zsi->gap_us = zsi_gap_us;

	zsi->base = devm_platform_ioremap_resource_byname(pdev, "zsi");
	if (IS_ERR(zsi->base))
		zsi->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(zsi->base))
		return PTR_ERR(zsi->base);

	zsi->sys = zsi_map_named(pdev, "sys");
	if (IS_ERR(zsi->sys))
		return PTR_ERR(zsi->sys);
	zsi->scu = zsi_map_named(pdev, "scu");
	if (IS_ERR(zsi->scu))
		return PTR_ERR(zsi->scu);

	if (!zsi->sys || !zsi->scu) {
		dev_err(dev, "need both the 'sys' and 'scu' reg ranges\n");
		return -EINVAL;
	}

	device_property_read_u32(dev, "airoha,zsi-gap-us", &zsi->gap_us);
	if (zsi->gap_us < 500)
		zsi->gap_us = 500;

	mutex_lock(&en75xx_zsi_list_lock);
	list_add_tail(&zsi->node, &en75xx_zsi_list);
	mutex_unlock(&en75xx_zsi_list_lock);

	platform_set_drvdata(pdev, zsi);
	dev_info(dev, "ZSI transport ready (gap %u us)\n", zsi->gap_us);

	of_platform_populate(dev->of_node, NULL, NULL, dev);
	return 0;
}

static void en75xx_zsi_remove(struct platform_device *pdev)
{
	struct en75xx_zsi *zsi = platform_get_drvdata(pdev);

	of_platform_depopulate(&pdev->dev);
	mutex_lock(&en75xx_zsi_list_lock);
	list_del_init(&zsi->node);
	mutex_unlock(&en75xx_zsi_list_lock);
}

static const struct of_device_id en75xx_zsi_of_match[] = {
	{ .compatible = "econet,en751221-zsi" },
	{ .compatible = "econet,en75xx-zsi" },
	{ }
};
MODULE_DEVICE_TABLE(of, en75xx_zsi_of_match);

static struct platform_driver en75xx_zsi_driver = {
	.probe = en75xx_zsi_probe,
	.remove = en75xx_zsi_remove,
	.driver = {
		.name = "en75xx-zsi",
		.of_match_table = en75xx_zsi_of_match,
	},
};
module_platform_driver(en75xx_zsi_driver);

MODULE_DESCRIPTION("EcoNet/Airoha EN75xx ZSI SLIC transport");
MODULE_LICENSE("GPL");
