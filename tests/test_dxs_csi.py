"""Exercise the real DXS access functions against recorded OEM CSI framing."""
from test_review_fixes import ROOT, function, run_c
import unittest


class DxsCsi(unittest.TestCase):
    def test_wire_transactions(self):
        source = (ROOT / 'vendor/maxlinear/drv_tapi_dxs-1.0.0.0/src/dxs/drv_dxs_access.c').read_text()
        harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
typedef uint8_t IFX_uint8_t;
typedef uint16_t IFX_uint16_t;
typedef uint32_t IFX_uint32_t;
typedef int IFX_int32_t;
typedef bool IFX_boolean_t;
#define IFX_FALSE false
#define __KERNEL__ 1
struct device_node { const char *compatible; };
struct device { struct device_node *of_node; };
struct controller { struct device dev; };
struct spi { struct controller *controller; };
typedef struct { struct spi *pSpiDev; int nErr, nDevNr; } DXS_DEVICE_t;
static bool of_device_is_compatible(struct device_node *np, const char *s)
{ return np && !strcmp(np->compatible, s); }
#define SPI_MAXBYTES_SIZE 32
#define DXS_SPI_HDR_LENGTH 2
#define DXS_SPI_HDR_B0_W 0x40
#define DXS_SPI_HDR_B0_R 0x80
#define DXS_SPI_HDR_B0_RESERVED 0x3e
#define DXS_SPI_HDR_B1_I 1
#define DXS_HOST_DATA 0x14
#define DXS_statusOk 0
#define DXS_statusSpiAccErr 7
#define DXS_SPI_READ_HALF_DUPLEX
#define TAPI_ASSERT assert
#define SPI_CS_SET(dev,level) ((void)0)
#define IFX_LOW 0
#define IFX_HIGH 1
static int calls, fail_at, nt[8], nr[8];
static unsigned char tx[8][32];
static int dxs_spi_setup(DXS_DEVICE_t *d) { return 0; }
static int spi_ll_read_write(DXS_DEVICE_t *d, unsigned char *t, unsigned n,
                            unsigned char *r, unsigned m)
{
    int c = calls++;
    nt[c] = n; nr[c] = m; memcpy(tx[c], t, n);
    if (calls == fail_at) return DXS_statusSpiAccErr;
    if (m) {
        memset(r, 0xee, m);
        /* CSI has a leading byte, plain SPI does not. */
        unsigned off = strcmp(d->pSpiDev->controller->dev.of_node->compatible,
                              "plain-spi") != 0;
        for (unsigned i = off; i < m; i++) r[i] = 0x12 + i - off;
    }
    return 0;
}
'''
        for signature in ['void DXS_cpw2b (', 'static void DXS_cpdw2b (',
                          'void DXS_cpb2w (', 'void DXS_cpb2dw (',
                          'static IFX_boolean_t dxs_is_csi(',
                          '\nIFX_int32_t dxs_spi_write(', '\nIFX_int32_t dxs_spi_read(']:
            harness += '\n' + function(source, signature)
        harness += r'''
int main(void)
{
    struct device_node np = { "econet,en751221-csi-spi" };
    struct controller ctl = { { &np } };
    struct spi spi = { &ctl };
    DXS_DEVICE_t dev = { .pSpiDev = &spi };
    uint16_t words[4] = {0x1234, 0x5678, 0x9abc, 0xdef0};
    uint32_t mailbox[2] = {0x12345678, 0x9abcdef0};
    const unsigned char reg[] = {0x7e, 0x06, 0x12, 0x34, 0x12, 0x34};
    const unsigned char mb0[] = {0x7e, 0x14, 0x12, 0x34, 0x56, 0x78};
    const unsigned char mb1[] = {0x7e, 0x14, 0x9a, 0xbc, 0xde, 0xf0};
    for (int i = 0; i < 2; i++) {
        np.compatible = i ? "airoha,en7523-csi-spi" : "econet,en751221-csi-spi";
        calls = 0;
        assert(!dxs_spi_write(&dev, 6, words, 2));
        assert(calls == 1 && nt[0] == 6 && !memcmp(tx[0], reg, 6));
        calls = 0;
        assert(!dxs_spi_read(&dev, 6, words, 2));
        assert(calls == 1 && tx[0][0] == 0xbe && tx[0][1] == 6 && nr[0] == 10);
        assert(words[0] == 0x1213);
        calls = 0;
        assert(!dxs_spi_write(&dev, 0x14, (void *)mailbox, 8));
        assert(calls == 2 && nt[0] == 6 && nt[1] == 6);
        assert(!memcmp(tx[0], mb0, 6) && !memcmp(tx[1], mb1, 6));
        calls = 0;
        assert(!dxs_spi_read(&dev, 0x14, (void *)mailbox, 8));
        assert(calls == 2 && nr[0] == 10 && nr[1] == 10);
        assert(mailbox[0] == 0x12131415 && mailbox[1] == 0x12131415);
        mailbox[0] = 0x12345678; mailbox[1] = 0x9abcdef0;
        calls = 0; fail_at = 1;
        assert(dxs_spi_read(&dev, 0x14, (void *)mailbox, 8) == DXS_statusSpiAccErr);
        assert(calls == 1 && mailbox[0] == 0x12345678);
        calls = 0;
        assert(dxs_spi_write(&dev, 0x14, (void *)mailbox, 8) == DXS_statusSpiAccErr);
        assert(calls == 1); fail_at = 0;
        words[0] = 0x1234;
    }
    np.compatible = "plain-spi";
    calls = 0;
    assert(!dxs_spi_read(&dev, 6, words, 4));
    assert(calls == 1 && tx[0][1] == 7 && nr[0] == 4);
    assert(words[0] == 0x1213 && words[1] == 0x1415);
    calls = 0;
    assert(!dxs_spi_write(&dev, 6, words, 2));
    assert(calls == 1 && nt[0] == 4); /* no CSI padding */
    dev.pSpiDev = NULL;
    assert(dxs_spi_read(&dev, 6, words, 2) == DXS_statusSpiAccErr);
    assert(dxs_spi_write(&dev, 6, words, 2) == DXS_statusSpiAccErr);
    return 0;
}
'''
        run_c(harness)
