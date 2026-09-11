// SPDX-License-Identifier: GPL-2.0
/*
 * ISI (Skyworks-style Interchip Serial Interface) transport for EN75xx
 * SLICs, exposed as an ordinary Linux spi_controller.
 *
 * ISI multiplexes the ProSLIC control channel over the PCM bus the same
 * way ZSI does for the Microchip parts (see en75xx_zsi.c) -- the SLIC
 * only answers while the PCM engine is clocking PCLK, and the control
 * pins are shared with the PCM block, not a separate SPI bus. Driving
 * the chip as a plain SPI device on a generic controller leaves it
 * electrically mute.
 *
 * Unlike ZSI, this does not need a bespoke phandle API: ProSLIC's own
 * command-byte framing (0x60/0x20/0x80 | channel<<4, see
 * en75xx_slic_si3219x.c) already carries everything the register-level
 * protocol needs, so the only thing missing is a way to get those bytes
 * across the wire. Implementing that as a spi_controller means the
 * existing SPI-mode Si3219x driver works over ISI completely unchanged
 * -- only the "these compatibles need ISI, which does not exist yet"
 * refusal at the top of its probe() goes away.
 *
 * The wrapper lives at 0x1fbd1000 (+ id * 0x2000), the same physical
 * block ZSI uses; ISI and ZSI are alternate framings of one engine, not
 * separate hardware, so a board enables at most one of the two DT nodes
 * at that address.
 *
 * Register interface and bring-up sequence recovered from the vendor
 * spi.ko/pcm1.ko shipped in TP-Link's XX530v GPL release (an unstripped
 * ARM module) and verified live on EN7523 hardware (ProSLIC ID 0xaa read
 * back over this exact path). EN7528 shares the wrapper, the SYS SCU
 * interface-mode register and the SYS SCU reset bit at the same offsets
 * -- confirmed by decompiling that SoC's own vendor modules -- but has no
 * audio PLL block; see needs_audio_pll below.
 *
 * Chip-SCU clock-source/pinmux routing (set_gpio_clocksrc() in the
 * vendor's spi.ko/pcm1.ko) indexes a per-SoC chipScuReg[] table, so the
 * register offsets differ by SoC even though the algorithm is identical.
 * Both SoCs' table entries are now verified data, not inferred: EN7523's
 * from that SoC's own spi.ko/pcm1.ko, and EN7528's from three independent
 * binaries agreeing on both the algorithm and the raw chipScuReg bytes --
 * this exact TP-Link XC220-G3v board's own spi_si32192.ko and pcm1.ko
 * (chipScuReg entry 3, selected for GET_HIR()==0xb), plus the same
 * algorithm confirmed a third time in an unrelated AN7581/VB430v spi.ko.
 * See the en7523_isi_quirks/en7528_isi_quirks definitions below.
 *
 * EN7523 additionally sets one extra chip-SCU pinmux bit
 * (pinmux_extra_set) that set_gpio_clocksrc() itself does not touch --
 * observed in that SoC's own pcm1.ko init sequence, meaning unverified
 * even there (never established what it actually does, only that the
 * vendor writes it). No equivalent has been found for EN7528, so it is
 * not carried over by assumption; en7528_isi_quirks leaves it at 0.
 *
 * Only devNum 1 (a single ISI device) is wired up. A second device on
 * the same bus additionally ORs the chip-SCU DEV2 clock/pinmux bits into
 * the chip_scu_clksrc_reg/chip_scu_pinmux_reg writes in
 * en75xx_isi_spi_hw_init() below; nothing here has been tested with two
 * devices present.
 */

#include <linux/bits.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/reset.h>
#include <linux/spi/spi.h>

#include "en75xx_scu.h"

/*
 * Wrapper block (0x1fbd1000 + id * 0x2000), shared with en75xx_zsi.c.
 * CFG is listed for completeness but deliberately never written: the
 * vendor's ISI path leaves it at its reset value (0x19030303, bit 28
 * set) and only the ZSI/CSI paths touch it -- writing here would clear
 * that bit and corrupt the engine config before every transaction.
 */
#define ISI_REG_CFG			0x000
#define ISI_REG_TXDATA			0x004
#define ISI_REG_STATUS			0x008
#define  ISI_STATUS_READ_START		BIT(0)
#define  ISI_STATUS_TX_DONE		BIT(1)
#define  ISI_STATUS_RX_READY		BIT(2)
#define ISI_REG_RXDATA			0x00c
#define ISI_REG_EN			0x010
#define  ISI_EN_BITS			0x19
#define ISI_REG_CHAN_SEL		0x014

/*
 * SYS SCU (0x1fb00000). The reset bit at 0x834 (matching en75xx_zsi.c's
 * SYS_RESET_SLIC0) is not poked directly here -- it goes through the
 * "isi" reset-controller line instead (devm_reset_control_get_optional_
 * exclusive() below), which is the same physical bit under mainline's
 * reset-controller abstraction.
 */
#define SYS_IFACE_MODE			0x094
#define  SYS_IFACE_MODE_MASK		GENMASK(3, 0)
#define  SYS_IFACE_MODE_ISI		0xf

/*
 * Chip SCU (0x1fa20000): SLIC clock source and pin routing, indexed from
 * chipScuReg[] -- see the file header for provenance. Offsets differ per
 * SoC (different table entry), the algorithm does not.
 */
#define EN7523_CHIP_SCU_CLKSRC		0x214
#define EN7523_CHIP_SCU_CLKSRC_MASK	0x003f3300u
#define EN7523_CHIP_SCU_GPIO_DEV1	0x00001000u
#define EN7523_CHIP_SCU_PINMUX		0x1d0
#define EN7523_CHIP_SCU_PINMUX_MASK	0x00000c00u
/* pcm1.ko init: 0x1d0 |= 1 (clock-out gate?) -- see pinmux_extra_set */
#define EN7523_CHIP_SCU_PINMUX_EXTRA	0x00000001u

/* chipScuReg entry 3 (GET_HIR()==0xb), from this exact board's own spi_si32192.ko/pcm1.ko */
#define EN7528_CHIP_SCU_CLKSRC		0x15c
#define EN7528_CHIP_SCU_CLKSRC_MASK	0x001f4000u
#define EN7528_CHIP_SCU_GPIO_DEV1	0x00080000u
#define EN7528_CHIP_SCU_PINMUX		0x130
#define EN7528_CHIP_SCU_PINMUX_MASK	0x00000c00u

/*
 * Audio PLL fractional synthesizer, chip SCU, EN7523 only. This is what
 * produces the 24.576 MHz PSCLK the SLIC needs on that SoC; EN7523's
 * generic peripheral clock tops out at 5 MHz and cannot reach it. Not
 * present on EN7528 -- see needs_audio_pll.
 */
#define CHIP_SCU_PLL_PROTECT		0x264
#define  CHIP_SCU_PLL_UNLOCK_KEY	0x80
#define CHIP_SCU_XTAL_SELECT		0x254
#define  CHIP_SCU_XTAL_25M		BIT(19)
#define CHIP_SCU_APLL_XTAL25		0x2d0
#define  CHIP_SCU_APLL_DIV_XTAL25	0x5681ecd4u
#define CHIP_SCU_APLL_XTAL20		0x2d4
#define  CHIP_SCU_APLL_DIV_XTAL20	0x6c226808u

#define ISI_POLL_US			2
#define ISI_TIMEOUT_US			20000

struct en75xx_isi_spi {
	struct device		*dev;
	void __iomem		*base;
	struct en75xx_scu	sys;
	struct en75xx_scu	scu;
	struct reset_control	*rst;
	struct clk		*slic_clk;
	spinlock_t		lock;
};

struct en75xx_isi_quirks {
	bool needs_audio_pll;
	/* chip_scu_clksrc_reg == 0 means "not known for this SoC yet" -- see
	 * en75xx_isi_spi_hw_init(), which warns and skips rather than guess. */
	u32 chip_scu_clksrc_reg;
	u32 chip_scu_clksrc_mask;
	u32 chip_scu_gpio_dev1;
	u32 chip_scu_pinmux_reg;
	u32 chip_scu_pinmux_mask;
	u32 pinmux_extra_set;	/* EN7523-only; see file header, 0 elsewhere */
};

static int isi_start_apll(struct en75xx_isi_spi *isi)
{
	u32 xtal, reg, div, v;
	int ret;

	ret = en75xx_scu_write(&isi->scu, CHIP_SCU_PLL_PROTECT,
			       CHIP_SCU_PLL_UNLOCK_KEY);
	if (ret)
		return ret;

	ret = en75xx_scu_read(&isi->scu, CHIP_SCU_XTAL_SELECT, &xtal);
	if (ret)
		return ret;

	if (xtal & CHIP_SCU_XTAL_25M) {
		reg = CHIP_SCU_APLL_XTAL25;
		div = CHIP_SCU_APLL_DIV_XTAL25;
	} else {
		reg = CHIP_SCU_APLL_XTAL20;
		div = CHIP_SCU_APLL_DIV_XTAL20;
	}

	ret = en75xx_scu_read(&isi->scu, reg, &v);
	if (!ret)
		ret = en75xx_scu_write(&isi->scu, reg, (v & 1) | div);
	/* toggle the load latch (bit 0 of 0x2d0) to commit the divider */
	if (!ret)
		ret = en75xx_scu_read(&isi->scu, CHIP_SCU_APLL_XTAL25, &v);
	if (!ret)
		ret = en75xx_scu_write(&isi->scu, CHIP_SCU_APLL_XTAL25, v ^ 1);
	if (!ret)
		ret = en75xx_scu_write(&isi->scu, CHIP_SCU_PLL_PROTECT, 0);

	if (ret)
		return ret;

	dev_dbg(isi->dev, "audio PLL programmed for %s crystal\n",
		(xtal & CHIP_SCU_XTAL_25M) ? "25 MHz" : "20 MHz");
	return 0;
}

/* ------------------------------------------------------------------ */
/* Byte level                                                          */
/* ------------------------------------------------------------------ */

static int isi_write_byte(struct en75xx_isi_spi *isi, u8 v)
{
	u32 status;

	writel(v, isi->base + ISI_REG_TXDATA);
	if (readl_poll_timeout_atomic(isi->base + ISI_REG_STATUS, status,
				      status & ISI_STATUS_TX_DONE,
				      ISI_POLL_US, ISI_TIMEOUT_US))
		return -ETIMEDOUT;
	writel(ISI_STATUS_TX_DONE, isi->base + ISI_REG_STATUS);
	return 0;
}

static int isi_read_byte(struct en75xx_isi_spi *isi, u8 *v)
{
	u32 status = readl(isi->base + ISI_REG_STATUS);

	writel(status | ISI_STATUS_READ_START, isi->base + ISI_REG_STATUS);
	if (readl_poll_timeout_atomic(isi->base + ISI_REG_STATUS, status,
				      status & ISI_STATUS_RX_READY,
				      ISI_POLL_US, ISI_TIMEOUT_US))
		return -ETIMEDOUT;
	*v = readl(isi->base + ISI_REG_RXDATA) & 0xff;
	writel(ISI_STATUS_RX_READY, isi->base + ISI_REG_STATUS);
	return 0;
}

/*
 * The vendor selects the channel exactly once, at the start of the
 * whole command-then-data transaction, and never touches it again
 * until the next one. spi_write_then_read() splits a register read
 * into a tx transfer (command + register) then an rx transfer (data),
 * so select only on the tx phase -- re-selecting before the read would
 * poke this register mid-transaction, which the vendor never does.
 */
static int en75xx_isi_spi_transfer_one(struct spi_controller *host,
				       struct spi_device *spi,
				       struct spi_transfer *xfer)
{
	struct en75xx_isi_spi *isi = spi_controller_get_devdata(host);
	unsigned long flags;
	unsigned int i;
	int ret = 0;

	spin_lock_irqsave(&isi->lock, flags);

	if (xfer->tx_buf) {
		const u8 *tx = xfer->tx_buf;

		writel(spi_get_chipselect(spi, 0), isi->base + ISI_REG_CHAN_SEL);
		for (i = 0; i < xfer->len; i++) {
			ret = isi_write_byte(isi, tx[i]);
			if (ret)
				break;
		}
	} else if (xfer->rx_buf) {
		u8 *rx = xfer->rx_buf;

		for (i = 0; i < xfer->len; i++) {
			ret = isi_read_byte(isi, &rx[i]);
			if (ret)
				break;
		}
	}

	spin_unlock_irqrestore(&isi->lock, flags);
	return ret;
}

/* ------------------------------------------------------------------ */
/* Hardware bring-up                                                   */
/* ------------------------------------------------------------------ */

static int en75xx_isi_spi_hw_init(struct en75xx_isi_spi *isi,
				  const struct en75xx_isi_quirks *quirks)
{
	int ret;

	/*
	 * Route the SLIC clock and pins first -- this is step one of the
	 * vendor's own bring-up, ahead of any reset or interface-mode
	 * write. PSCLK first: nothing on the ISI link works without it.
	 */
	if (quirks->needs_audio_pll) {
		ret = isi_start_apll(isi);
		if (ret)
			return ret;
	}

	if (quirks->chip_scu_clksrc_reg) {
		/*
		 * ISI branch of set_gpio_clocksrc(): write(clksrc_reg,
		 * (read & ~clksrc_mask) | gpio_dev1); write(pinmux_reg,
		 * read & ~pinmux_mask), the latter ORed with the SoC's
		 * extra bit (0 except on EN7523) since this driver only
		 * ever takes the ISI path.
		 */
		ret = en75xx_scu_update(&isi->scu, quirks->chip_scu_clksrc_reg,
					quirks->chip_scu_clksrc_mask,
					quirks->chip_scu_gpio_dev1);
		if (!ret)
			ret = en75xx_scu_update(&isi->scu, quirks->chip_scu_pinmux_reg,
						quirks->chip_scu_pinmux_mask | quirks->pinmux_extra_set,
						quirks->pinmux_extra_set);
		if (ret)
			return dev_err_probe(isi->dev, ret,
					     "cannot route the SLIC clock source\n");
	} else {
		dev_warn(isi->dev,
			"chip-SCU clock/pin routing not implemented for this SoC "
			"(chipScuReg layout not decoded); relying on whatever the "
			"bootloader/vendor firmware already left in place\n");
	}

	/*
	 * Program the SLIC interface mode. Without this the SCU nibble
	 * stays at its reset value of plain SPI framing, and the SLIC
	 * never answers however correct the byte-level transfers are.
	 */
	ret = en75xx_scu_update(&isi->sys, SYS_IFACE_MODE, SYS_IFACE_MODE_MASK,
				SYS_IFACE_MODE_ISI);
	if (ret)
		return dev_err_probe(isi->dev, ret,
				     "cannot set the SLIC interface mode\n");

	/*
	 * Pulse the ISI block out of reset -- the vendor does this
	 * immediately after setting the interface mode above.
	 */
	if (isi->rst) {
		ret = reset_control_reset(isi->rst);
		if (ret)
			return dev_err_probe(isi->dev, ret,
					     "cannot pulse the ISI reset\n");
	}

	/*
	 * Finally enable the wrapper -- meaningful only once the interface
	 * nibble is set, so it has to stay after the block above.
	 */
	writel(readl(isi->base + ISI_REG_EN) | ISI_EN_BITS,
	       isi->base + ISI_REG_EN);

	dev_info(isi->dev, "ISI mode selected, wrapper enabled%s\n",
		 quirks->needs_audio_pll ? " (audio PLL programmed)" : "");
	return 0;
}

static int en75xx_isi_spi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct en75xx_isi_quirks *quirks;
	struct en75xx_isi_spi *isi;
	struct spi_controller *host;
	int ret;

	quirks = of_device_get_match_data(dev);
	if (!quirks)
		return -ENODEV;

	host = devm_spi_alloc_host(dev, sizeof(*isi));
	if (!host)
		return -ENOMEM;

	isi = spi_controller_get_devdata(host);
	isi->dev = dev;
	spin_lock_init(&isi->lock);

	isi->base = devm_platform_ioremap_resource_byname(pdev, "isi");
	if (IS_ERR(isi->base))
		isi->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(isi->base))
		return PTR_ERR(isi->base);

	ret = en75xx_scu_get(pdev, &isi->sys, "airoha,scu", "sys");
	if (ret)
		return ret;
	ret = en75xx_scu_get(pdev, &isi->scu, "airoha,chip-scu", "scu");
	if (ret)
		return ret;
	if (!en75xx_scu_valid(&isi->sys) || !en75xx_scu_valid(&isi->scu))
		return dev_err_probe(dev, -EINVAL,
				     "need both airoha,scu and airoha,chip-scu\n");

	isi->rst = devm_reset_control_get_optional_exclusive(dev, "isi");
	if (IS_ERR(isi->rst))
		return dev_err_probe(dev, PTR_ERR(isi->rst),
				     "cannot get the ISI reset\n");

	isi->slic_clk = devm_clk_get_optional_enabled(dev, "slic");
	if (IS_ERR(isi->slic_clk))
		return dev_err_probe(dev, PTR_ERR(isi->slic_clk),
				     "cannot enable the SLIC clock\n");

	ret = en75xx_isi_spi_hw_init(isi, quirks);
	if (ret)
		return ret;

	host->dev.of_node = dev->of_node;
	host->bus_num = -1;
	host->mode_bits = SPI_CPOL | SPI_CPHA;
	host->bits_per_word_mask = SPI_BPW_MASK(8);
	host->flags = SPI_CONTROLLER_HALF_DUPLEX;
	host->use_gpio_descriptors = true;
	host->transfer_one = en75xx_isi_spi_transfer_one;

	ret = devm_spi_register_controller(dev, host);
	if (ret)
		return dev_err_probe(dev, ret, "cannot register SPI controller\n");

	dev_info(dev, "ISI SPI controller ready\n");
	return 0;
}

static const struct en75xx_isi_quirks en7523_isi_quirks = {
	.needs_audio_pll = true,
	.chip_scu_clksrc_reg = EN7523_CHIP_SCU_CLKSRC,
	.chip_scu_clksrc_mask = EN7523_CHIP_SCU_CLKSRC_MASK,
	.chip_scu_gpio_dev1 = EN7523_CHIP_SCU_GPIO_DEV1,
	.chip_scu_pinmux_reg = EN7523_CHIP_SCU_PINMUX,
	.chip_scu_pinmux_mask = EN7523_CHIP_SCU_PINMUX_MASK,
	.pinmux_extra_set = EN7523_CHIP_SCU_PINMUX_EXTRA,
};

static const struct en75xx_isi_quirks en7528_isi_quirks = {
	.needs_audio_pll = false,
	.chip_scu_clksrc_reg = EN7528_CHIP_SCU_CLKSRC,
	.chip_scu_clksrc_mask = EN7528_CHIP_SCU_CLKSRC_MASK,
	.chip_scu_gpio_dev1 = EN7528_CHIP_SCU_GPIO_DEV1,
	.chip_scu_pinmux_reg = EN7528_CHIP_SCU_PINMUX,
	.chip_scu_pinmux_mask = EN7528_CHIP_SCU_PINMUX_MASK,
};

static const struct of_device_id en75xx_isi_spi_of_match[] = {
	{ .compatible = "airoha,en7523-isi-spi", .data = &en7523_isi_quirks },
	{ .compatible = "econet,en7528-isi-spi", .data = &en7528_isi_quirks },
	{ }
};
MODULE_DEVICE_TABLE(of, en75xx_isi_spi_of_match);

static struct platform_driver en75xx_isi_spi_driver = {
	.probe = en75xx_isi_spi_probe,
	.driver = {
		.name = "en75xx-isi-spi",
		.of_match_table = en75xx_isi_spi_of_match,
	},
};
module_platform_driver(en75xx_isi_spi_driver);

MODULE_DESCRIPTION("EcoNet/Airoha EN75xx ISI SLIC transport (spi_controller)");
MODULE_LICENSE("GPL");
