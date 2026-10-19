"""Exercise the actual DXS ROM/firmware stage with mocked backend calls."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class DxsFirmwareStage(unittest.TestCase):
    def test_boot_sequence_and_failures(self):
        source = (ROOT / 'src/en75xx_slic_dxs.c').read_text()
        a = source.index('static int dxs_boot_firmware(')
        function = source[a:source.index('\n}', a) + 2]
        harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
typedef uint32_t u32;
struct firmware { const unsigned char *data; size_t size; };
struct en75xx_dxs { void *dev; unsigned int index, count; u32 loaded_fw; };
#define dev_info(...) ((void)0)
#define dev_err(...) ((void)0)
static int sequence[4], seq, versions, downloads, fail_version, fail_download;
static unsigned detected[2];
static int en75xx_dxs_kernel_version(unsigned index, u32 *hw, u32 *fw, unsigned *count)
{
    assert(index == 0);
    sequence[seq++] = 1;
    versions++;
    if (versions == fail_version) return -EIO;
    *hw = 0x12; *fw = versions == 1 ? 0x02010000 : 0x02010300;
    *count = detected[versions - 1];
    return 0;
}
static int en75xx_dxs_kernel_firmware(unsigned index, const unsigned char *data, size_t size)
{
    assert(index == 0 && data && size == 4);
    sequence[seq++] = 2; downloads++;
    return fail_download ? -ETIMEDOUT : 0;
}
static void reset(void)
{
    seq = versions = downloads = fail_version = fail_download = 0;
    detected[0] = detected[1] = 1;
}
'''
        harness += function + r'''
int main(void)
{
    const unsigned char data[4] = {1,2,3,4};
    const struct firmware fw = {data, 4};
    struct en75xx_dxs dxs = {.index = 0, .count = 1};
    reset();
    assert(dxs_boot_firmware(&dxs, &fw) == 0);
    assert(seq == 3 && sequence[0] == 1 && sequence[1] == 2 && sequence[2] == 1);
    assert(versions == 2 && downloads == 1);
    reset(); fail_version = 1;
    assert(dxs_boot_firmware(&dxs, &fw) == -EIO && downloads == 0 && versions == 1);
    reset(); detected[0] = 2;
    assert(dxs_boot_firmware(&dxs, &fw) == -ENODEV && downloads == 0);
    reset(); fail_download = 1;
    assert(dxs_boot_firmware(&dxs, &fw) == -ETIMEDOUT && versions == 1 && downloads == 1);
    reset(); fail_version = 2;
    assert(dxs_boot_firmware(&dxs, &fw) == -EIO && versions == 2 && downloads == 1);
    reset(); detected[1] = 2;
    assert(dxs_boot_firmware(&dxs, &fw) == -ENODEV && versions == 2);
    reset(); dxs.count = 2; detected[0] = detected[1] = 2;
    assert(dxs_boot_firmware(&dxs, &fw) == 0);
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='dxs-fw-stage-') as tmp:
            binary = Path(tmp) / 'test'
            subprocess.run(['cc', '-Wall', '-Werror', '-x', 'c', '-', '-o', str(binary)],
                           input=harness, text=True, check=True)
            subprocess.run([str(binary)], check=True, timeout=5)


if __name__ == '__main__':
    unittest.main()
