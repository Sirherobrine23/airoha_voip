#!/usr/bin/env python3
"""Host regression tests compiling the actual kernel firmware parser with shims.

Requires a C compiler and zlib development files. No module or hardware access.
The shims exercise the on-disk parser, not Linux firmware lookup or SPI I/O.
"""
import binascii
import ctypes
import pathlib
import struct
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

SHIMS = r'''
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <zlib.h>
typedef uint8_t u8;
typedef uint16_t __le16, uInt16;
typedef uint32_t __le32, u32, ramData;
struct device { int unused; };
struct firmware { size_t size; const unsigned char *data; };
typedef struct {
 const ramData *patchData;
 const uInt16 *patchEntries;
 const u32 patchSerial;
 const uInt16 *psRamAddr;
 const ramData *psRamData;
} proslicPatch;
#define __packed __attribute__((packed))
#define GFP_KERNEL 0
#define kcalloc(n, s, f) calloc(n, s)
#define kfree(p) free(p)
#define dev_err(...) ((void)0)
#define dev_dbg(...) ((void)0)
#define dev_info(...) ((void)0)
#define EXPORT_SYMBOL_GPL(...)
#define MODULE_DESCRIPTION(...)
#define MODULE_LICENSE(...)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define le16_to_cpu(x) (x)
#define le32_to_cpu(x) (x)
#else
#define le16_to_cpu(x) __builtin_bswap16(x)
#define le32_to_cpu(x) __builtin_bswap32(x)
#endif
#define strscpy(d, s, n) snprintf(d, n, "%s", s)
static int request_firmware(const struct firmware **f, const char *n,
                           struct device *d) { return -ENOENT; }
static void release_firmware(const struct firmware *f) {}
/* Linux crc32() exposes the uncomplemented accumulator, unlike zlib. */
static u32 kernel_crc32(u32 seed, const unsigned char *p, size_t n)
{ return (u32)crc32(seed ^ 0xffffffffU, p, n) ^ 0xffffffffU; }
#define crc32 kernel_crc32
'''

WRAPPER = r'''
int check_blob(const unsigned char *data, size_t size)
{
 struct firmware blob = { size, data };
 struct en75xx_proslic_fw fw = {0};
 struct device dev = {0};
 int ret = proslic_fw_parse(&dev, &blob, "test.fw", "si3219x", 'A', "lcqc", &fw);
 en75xx_proslic_fw_free(&fw);
 return ret;
}
'''


def strip_includes(text):
    return '\n'.join(line for line in text.splitlines() if not line.startswith('#include'))


class FirmwareParserTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        path = pathlib.Path(cls.temp.name)
        source = SHIMS + strip_includes((ROOT / 'src/en75xx_proslic_fw.h').read_text())
        source += strip_includes((ROOT / 'src/en75xx_proslic_fw.c').read_text()) + WRAPPER
        (path / 'parser.c').write_text(source)
        subprocess.run(['cc', '-shared', '-fPIC', '-O1', '-g', '-fsanitize=undefined',
                        str(path / 'parser.c'), '-lz', '-o', str(path / 'parser.so')], check=True)
        cls.lib = ctypes.CDLL(str(path / 'parser.so'))
        cls.lib.check_blob.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
        cls.lib.check_blob.restype = ctypes.c_int
        subprocess.run(['python3', str(ROOT / 'tools/proslic-patch2fw.py'), '-o', str(path / 'fw'),
                        str(ROOT / 'vendor/proslic/patch_files/si3219x_patch_A_2017MAY25.c')],
                       check=True, stdout=subprocess.DEVNULL)
        cls.valid = (path / 'fw/si3219x_a_lcqc.fw').read_bytes()

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def parse(self, data):
        buf = ctypes.create_string_buffer(bytes(data))
        return self.lib.check_blob(buf, len(data))

    def test_converter_output_is_accepted(self):
        self.assertEqual(self.parse(self.valid), 0)

    def test_header_identity_is_enforced(self):
        for offset, replacement in [(28, b'si3218x\0'), (36, b'B\0\0\0'),
                                    (40, b'FB\0' + bytes(9)), (40, bytes(12))]:
            with self.subTest(offset=offset, replacement=replacement):
                blob = bytearray(self.valid)
                blob[offset:offset + len(replacement)] = replacement
                self.assertLess(self.parse(blob), 0)

    def test_zero_support_ram_count_is_rejected(self):
        blob = bytearray(self.valid[:56])
        n_data = struct.unpack_from('<H', blob, 16)[0]
        struct.pack_into('<HH', blob, 18, 0, 0)
        blob += self.valid[56:56 + n_data * 4] + self.valid[-32:]
        struct.pack_into('<I', blob, 24, binascii.crc32(blob[56:]))
        self.assertLess(self.parse(blob), 0)

    def test_crc_corruption_is_rejected(self):
        blob = bytearray(self.valid)
        blob[56] ^= 1
        self.assertLess(self.parse(blob), 0)

    def test_truncation_is_rejected(self):
        for size in (0, 55, 56, len(self.valid) - 1):
            with self.subTest(size=size):
                self.assertLess(self.parse(self.valid[:size]), 0)

    def test_missing_terminators_are_rejected(self):
        n_data, n_psdata, n_psaddr = struct.unpack_from('<HHH', self.valid, 16)
        for offset in (56 + (n_data - 1) * 4,
                       56 + n_data * 4 + n_psdata * 4 + (n_psaddr - 1) * 2):
            with self.subTest(offset=offset):
                blob = bytearray(self.valid)
                blob[offset] = 1
                struct.pack_into('<I', blob, 24, binascii.crc32(blob[56:]))
                self.assertLess(self.parse(blob), 0)


if __name__ == '__main__':
    unittest.main()
