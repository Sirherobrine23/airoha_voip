"""Run the actual CSI/ISI hardware-init function against mocked SCU/MMIO."""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def block(source, signature, ending='\n}'):
    start = source.index(signature)
    return source[start:source.index(ending, start) + len(ending)]


class DxsRouting(unittest.TestCase):
    def test_single_device_csi_and_isi_routes(self):
        source = (ROOT / 'src/en75xx_isi_spi.c').read_text()
        defines = '\n'.join(line for line in source.splitlines() if re.match(
            r'#define\s+(EN752[38]_|SYS_IFACE_|ISI_REG_EN\b|ISI_EN_BITS\b)', line))
        harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
typedef uint32_t u32;
#define BIT(n) (1U << (n))
#define GENMASK(h,l) ((~0U >> (31-(h))) & (~0U << (l)))
struct en75xx_scu { u32 reg[256]; };
struct en75xx_isi_spi {
    void *dev, *base, *rst;
    struct en75xx_scu sys, scu;
};
static bool en7528_pcm_clk;
static unsigned writes, fail_at, resets, apll;
static int en75xx_scu_update(struct en75xx_scu *s, u32 off, u32 mask, u32 val)
{
    if (++writes == fail_at) return -EIO;
    s->reg[off / 4] = (s->reg[off / 4] & ~mask) | val;
    return 0;
}
static int en75xx_scu_write(struct en75xx_scu *s, u32 off, u32 val)
{ return en75xx_scu_update(s, off, ~0U, val); }
static int isi_start_apll(struct en75xx_isi_spi *s) { apll++; return 0; }
static int reset_control_reset(void *rst) { resets++; return 0; }
static u32 readl(void *ptr) { u32 v; memcpy(&v, ptr, 4); return v; }
static void writel(u32 value, void *ptr) { memcpy(ptr, &value, 4); }
#define dev_err_probe(d,ret,...) (ret)
#define dev_warn(...) ((void)0)
#define dev_info(...) ((void)0)
'''
        harness += defines + '\n'
        harness += block(source, 'struct en75xx_isi_quirks {', '\n};') + '\n'
        for mode in ('isi', 'csi'):
            harness += block(source, 'static const struct en75xx_isi_quirks en7523_' + mode + '_quirks = {', '\n};') + '\n'
        harness += block(source, 'static int en75xx_isi_spi_hw_init(') + '\n'
        harness += r'''
int main(void)
{
    const u32 seeds[] = {0, ~0U, 0xa5a5a5a5};
    for (unsigned mode = 0; mode < 2; mode++) {
        const struct en75xx_isi_quirks *q = mode ? &en7523_csi_quirks : &en7523_isi_quirks;
        for (unsigned i = 0; i < 3; i++) {
            struct en75xx_isi_spi s = {0};
            u32 mmio[8] = {0}, seed = seeds[i];
            s.base = mmio; s.rst = &s;
            s.scu.reg[0x214 / 4] = s.scu.reg[0x1d0 / 4] = seed;
            s.scu.reg[0x27c / 4] = seed; /* LED1 mapping register */
            s.sys.reg[0x94 / 4] = seed;
            writes = fail_at = resets = apll = 0;
            assert(!en75xx_isi_spi_hw_init(&s, q));
            /* Stock chipScuReg[3], CSI devNum=1 versus ISI devNum=1. */
            assert(s.scu.reg[0x214 / 4] == ((seed & ~0x003f3300U) | (mode ? 0x1100U : 0x1000U)));
            assert(s.scu.reg[0x1d0 / 4] == ((seed & ~0xc01U) | (mode ? 0xc01U : 1U)));
            assert(s.scu.reg[0x27c / 4] == seed);
            assert(s.sys.reg[0x94 / 4] == ((seed & ~15U) | (mode ? 5U : 15U)));
            assert(mmio[0x10 / 4] == 0x19 && resets == 1 && apll == 1);
            assert(!(s.scu.reg[0x214 / 4] & (BIT(13) | BIT(16) | BIT(9) | BIT(17))));
        }
    }
    for (unsigned failure = 1; failure <= 3; failure++) {
        struct en75xx_isi_spi s = {0};
        u32 mmio[8] = {0};
        s.base = mmio; s.rst = &s;
        writes = resets = 0; fail_at = failure;
        assert(en75xx_isi_spi_hw_init(&s, &en7523_csi_quirks) == -EIO);
        assert(resets == 0 && mmio[0x10 / 4] == 0);
    }
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='dxs-route-') as tmp:
            binary = Path(tmp) / 'test'
            subprocess.run(['cc', '-Wall', '-Werror', '-x', 'c', '-', '-o', str(binary)],
                           input=harness, text=True, check=True)
            subprocess.run([str(binary)], check=True, timeout=5)


if __name__ == '__main__':
    unittest.main()
