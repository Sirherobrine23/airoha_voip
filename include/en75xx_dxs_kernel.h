/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _EN75XX_DXS_KERNEL_H
#define _EN75XX_DXS_KERNEL_H
#include <linux/types.h>
struct device_node;
extern bool en75xx_dxs_event_trace;
/* Backend-only diagnostics; does not consume mailbox data or interrupt latches. */
void en75xx_dxs_trace_snapshot(unsigned int index, const char *stage);
int en75xx_dxs_index(struct device_node *np);
int en75xx_dxs_basic_init(unsigned int index);
int en75xx_dxs_kernel_version(unsigned int index, u32 *hw, u32 *fw, unsigned int *channels);
int en75xx_dxs_kernel_firmware(unsigned int index, const u8 *data, size_t size);
int en75xx_dxs_kernel_bbd(unsigned int index, const u8 *data, size_t size);
/* Diagnostic only; leaves calibration pending and must not activate a line. */
int en75xx_dxs_kernel_bbd_prepare(unsigned int index, const u8 *data, size_t size);
#endif
