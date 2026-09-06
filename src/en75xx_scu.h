/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Access helper for the two shared control blocks these SoCs expose.
 *
 *   NP SCU    0x1fb00000   reset, interface mode
 *   chip SCU  0x1fa20000   pinmux, PCM clock
 *
 * Neither belongs to the voice stack: upstream models them as syscons
 * (`airoha,chip-scu` / `airoha,en7523-scu`, both with the "syscon"
 * fallback compatible) and several drivers reference them by phandle.
 * The vendor trees instead put raw ranges in each consumer's reg
 * property.
 *
 * Both styles work here. A regmap from a phandle is preferred because
 * it serialises against the other users of the block; a raw mapping is
 * the fallback, and is deliberately not an exclusive request, since the
 * SCU node maps the same addresses.
 */
#ifndef _EN75XX_SCU_H
#define _EN75XX_SCU_H

#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

struct en75xx_scu {
	struct regmap	*map;
	void __iomem	*base;
};

static inline bool en75xx_scu_valid(const struct en75xx_scu *scu)
{
	return scu->map || scu->base;
}

static inline int en75xx_scu_read(const struct en75xx_scu *scu, u32 off,
				  u32 *val)
{
	if (scu->map)
		return regmap_read(scu->map, off, val);
	if (scu->base) {
		*val = readl(scu->base + off);
		return 0;
	}

	return -ENODEV;
}

static inline int en75xx_scu_write(const struct en75xx_scu *scu, u32 off,
				   u32 val)
{
	if (scu->map)
		return regmap_write(scu->map, off, val);
	if (scu->base) {
		writel(val, scu->base + off);
		return 0;
	}

	return -ENODEV;
}

static inline int en75xx_scu_update(const struct en75xx_scu *scu, u32 off,
				    u32 mask, u32 val)
{
	if (scu->map)
		return regmap_update_bits(scu->map, off, mask, val);
	if (scu->base) {
		writel((readl(scu->base + off) & ~mask) | (val & mask),
		       scu->base + off);
		return 0;
	}

	return -ENODEV;
}

/*
 * Resolve one block: try the syscon phandle first, then a named reg
 * range. Returns 0 when neither is present -- callers decide whether
 * that is fatal, because a board that only needs the PCM engine and
 * leaves reset to the bootloader is a legitimate configuration.
 */
static inline int en75xx_scu_get(struct platform_device *pdev,
				 struct en75xx_scu *scu,
				 const char *phandle, const char *reg_name)
{
	struct device *dev = &pdev->dev;
	struct resource *res;

	memset(scu, 0, sizeof(*scu));

	if (phandle && dev->of_node &&
	    of_property_present(dev->of_node, phandle)) {
		scu->map = syscon_regmap_lookup_by_phandle(dev->of_node,
							   phandle);
		if (IS_ERR(scu->map)) {
			int ret = PTR_ERR(scu->map);

			scu->map = NULL;
			return dev_err_probe(dev, ret,
					     "cannot map %s\n", phandle);
		}
		return 0;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, reg_name);
	if (!res)
		return 0;

	/*
	 * devm_ioremap, not devm_ioremap_resource: the SCU node already
	 * covers these addresses, and an exclusive request would make
	 * whichever driver probes second fail with -EBUSY.
	 */
	scu->base = devm_ioremap(dev, res->start, resource_size(res));
	if (!scu->base)
		return -ENOMEM;

	return 0;
}

#endif /* _EN75XX_SCU_H */
