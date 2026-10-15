/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _EN75XX_DXS_KERNEL_H
#define _EN75XX_DXS_KERNEL_H
#include <linux/types.h>
struct device_node;
int en75xx_dxs_index(struct device_node *np);
int en75xx_dxs_basic_init(unsigned int index);
int en75xx_dxs_kernel_version(unsigned int index, u32 *hw, u32 *fw, unsigned int *channels);
int en75xx_dxs_kernel_firmware(unsigned int index, const u8 *data, size_t size);
int en75xx_dxs_kernel_bbd(unsigned int index, const u8 *data, size_t size);
#endif
