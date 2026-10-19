"""Check actual controller byte functions with a host MMIO model.

This validates the sequence and written values, not hardware register semantics.
Run: python3 -m unittest discover -s tests -p test_csi_status_ack.py -v
"""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


def function(source, name):
    start = source.index('static ', source.index(name) - 32)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


class CsiStatusAck(unittest.TestCase):
    def test_actual_byte_functions(self):
        source = (ROOT / 'src/en75xx_isi_spi.c').read_text()
        harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
typedef uint8_t u8;
typedef uint32_t u32;
#define ISI_REG_STATUS 8
#define ISI_REG_TXDATA 4
#define ISI_REG_RXDATA 12
#define ISI_STATUS_READ_START 1
#define ISI_STATUS_TX_DONE 2
#define ISI_STATUS_RX_READY 4
#define ISI_POLL_US 2
#define ISI_TIMEOUT_US 20000
struct en75xx_isi_spi { void *base; bool status_ack_rmw; u32 last_tx_status, last_ack; };
static unsigned char regs[32];
static u32 live_status, values[4];
static int writes, status_reads, timeout;
static u32 readl(void *p)
{
    if (p == regs + ISI_REG_STATUS) {
        status_reads++;
        return live_status;
    }
    assert(p == regs + ISI_REG_RXDATA);
    /* Change STATUS after the poll to require a fresh read. */
    live_status = 0x45;
    return 0x1ab;
}
static void writel(u32 value, void *p)
{
    if (p == regs + ISI_REG_TXDATA) { assert(value == 0x7e); return; }
    assert(p == regs + ISI_REG_STATUS);
    values[writes++] = value;
}
static int mock_poll(void *p, u32 *status)
{
    assert(p == regs + ISI_REG_STATUS);
    *status = 6;
    live_status = 0x26;
    return timeout ? -ETIMEDOUT : 0;
}
#define readl_poll_timeout_atomic(addr,status,cond,delay,bound) \
    mock_poll((addr), &(status))
'''
        for name in ('isi_ack_status(', 'isi_write_byte(', 'isi_read_byte('):
            harness += '\n' + function(source, name)
        harness += r'''
int main(void)
{
    for (int rmw = 0; rmw < 2; rmw++) {
        struct en75xx_isi_spi isi = {.base = regs, .status_ack_rmw = rmw};
        u8 value;
        writes = status_reads = timeout = 0;
        assert(isi_write_byte(&isi, 0x7e) == 0);
        assert(writes == 1 && values[0] == (rmw ? 0x26 : 2));
        assert(status_reads == rmw);
        writes = status_reads = 0; live_status = 0x20;
        assert(isi_read_byte(&isi, &value) == 0 && value == 0xab);
        assert(writes == 2 && values[0] == 0x21);
        assert(values[1] == (rmw ? 0x45 : 4));
        assert(status_reads == 1 + rmw);
        writes = status_reads = 0; timeout = 1;
        assert(isi_write_byte(&isi, 0x7e) == -ETIMEDOUT);
        assert(writes == 0 && status_reads == 0);
        writes = 0; value = 0xcc;
        assert(isi_read_byte(&isi, &value) == -ETIMEDOUT);
        assert(writes == 1 && value == 0xcc);
    }
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            src = Path(tmp) / 'ack.c'
            out = Path(tmp) / 'ack'
            src.write_text(harness)
            subprocess.run(['cc', '-std=gnu11', '-Wall', '-Wextra', '-Werror',
                            str(src), '-o', str(out)], check=True)
            subprocess.run([str(out)], check=True)

    def test_target_scope(self):
        source = (ROOT / 'src/en75xx_isi_spi.c').read_text()
        for target in ('en7523_csi', 'en7523_isi', 'en7528_isi', 'en751221_csi'):
            init = source.split(f'{target}_quirks = {{', 1)[1].split('};', 1)[0]
            self.assertEqual('.status_ack_rmw = true' in init, target == 'en7523_csi')
        self.assertIn('isi->status_ack_rmw = quirks->status_ack_rmw;', source)


if __name__ == '__main__':
    unittest.main()
