/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ZSI (Zarlink Serial Interface) transport for EN75xx SLICs.
 *
 * ZSI multiplexes the SLIC control channel over the PCM bus: the SLIC
 * only answers while the PCM engine is clocking PCLK/FSYNC, and the
 * control "MISO" pin is shared with the PCM block. Driving the chip as
 * a plain SPI device leaves it electrically mute (MISO idle-high 0xff).
 *
 * The wrapper lives at 0x1fbd1000 (+ id * 0x2000).
 */
#ifndef _EN75XX_ZSI_H
#define _EN75XX_ZSI_H

#include <linux/device.h>
#include <linux/types.h>

struct en75xx_zsi;

struct en75xx_zsi *en75xx_zsi_get(struct device *dev, const char *phandle_name);
void en75xx_zsi_put(struct en75xx_zsi *zsi);

/* Bring the SoC into ZSI mode: route, pinmux, PCM clock, wrapper enable. */
int en75xx_zsi_hw_init(struct en75xx_zsi *zsi);

/* Pulse the reset associated with this ZSI/SLIC path. */
int en75xx_zsi_slic_reset(struct en75xx_zsi *zsi);

/*
 * Stream one raw MPI section. Each opcode self-delimits: the SLIC
 * parses the stream, so callers just hand over the bytes.
 */
int en75xx_zsi_write(struct en75xx_zsi *zsi, const u8 *buf, size_t len);

/*
 * Select an EC (channel) and read @len bytes from register @reg.
 * Pass ec = 0 to skip the channel selection for device-level registers.
 */
int en75xx_zsi_read_reg(struct en75xx_zsi *zsi, u8 ec, u8 reg,
			u8 *buf, size_t len);

/* Select an EC and write @len data bytes to register @reg. */
int en75xx_zsi_write_reg(struct en75xx_zsi *zsi, u8 ec, u8 reg,
			 const u8 *buf, size_t len);

#endif /* _EN75XX_ZSI_H */
