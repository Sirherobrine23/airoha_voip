"""Compile the actual DT-to-TAPI settings function against a small DT mock."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class DxsPcmSettings(unittest.TestCase):
    def test_board_timing_and_rejected_properties(self):
        source = (ROOT / 'src/en75xx_slic_dxs.c').read_text()
        start = source.index('static int dxs_pcm_settings(')
        function = source[start:source.index('\n}', start) + 2]
        harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
typedef uint32_t u32;
typedef struct {
    unsigned nSlopeTX, nSlopeRX, nOffsetTX, nOffsetRX;
    unsigned nOpMode, nDrive, nTsSync, nDCLFreq;
} IFX_TAPI_PCM_IF_CFG_t;
struct device_node { int unused; };
struct device { struct device_node *of_node; };
struct en75xx_dxs { struct device *dev; IFX_TAPI_PCM_IF_CFG_t pcm_cfg; };
#define IFX_TAPI_PCM_IF_MODE_SLAVE 1
#define IFX_TAPI_PCM_IF_DRIVE_ENTIRE 0
#define IFX_TAPI_PCM_IF_TS_SYNC_FRAME 1
#define IFX_TAPI_PCM_IF_DCLFREQ_1024 1
#define IFX_TAPI_PCM_IF_DCLFREQ_2048 3
#define IFX_TAPI_PCM_IF_DCLFREQ_4096 4
#define IFX_TAPI_PCM_IF_DCLFREQ_8192 5
#define dev_info(...) ((void)0)
static const char *names[] = {"clock-frequency", "maxlinear,pcm-tx-edge",
    "maxlinear,pcm-rx-edge", "maxlinear,pcm-tx-offset", "maxlinear,pcm-rx-offset"};
static u32 values[5];
static unsigned present, malformed;
static int index_of(const char *name) {
    for (int i = 0; i < 5; i++) if (!strcmp(name, names[i])) return i;
    assert(0); return -1;
}
static const void *of_find_property(struct device_node *np, const char *name, void *len) {
    (void)np; (void)len;
    return present & (1u << index_of(name)) ? names : NULL;
}
static int of_property_read_u32(struct device_node *np, const char *name, u32 *v) {
    (void)np;
    int i = index_of(name);
    if (!(present & (1u << i)) || (malformed & (1u << i))) return -EINVAL;
    *v = values[i]; return 0;
}
static void reset(struct en75xx_dxs *dxs) {
    memset(&dxs->pcm_cfg, 0, sizeof(dxs->pcm_cfg));
    present = 31; malformed = 0;
    values[0] = 2048000; values[1] = 0; values[2] = 1;
    values[3] = 7; values[4] = 1;
}
'''
        harness += function + r'''
int main(void) {
    struct device_node np = {0}; struct device dev = {&np};
    struct en75xx_dxs dxs = {.dev = &dev};
    reset(&dxs); assert(dxs_pcm_settings(&dxs) == 0);
    assert(dxs.pcm_cfg.nDCLFreq == IFX_TAPI_PCM_IF_DCLFREQ_2048);
    assert(dxs.pcm_cfg.nSlopeTX == 0 && dxs.pcm_cfg.nSlopeRX == 1);
    assert(dxs.pcm_cfg.nOffsetTX == 7 && dxs.pcm_cfg.nOffsetRX == 1);
    assert(dxs.pcm_cfg.nOpMode == IFX_TAPI_PCM_IF_MODE_SLAVE);
    assert(dxs.pcm_cfg.nDrive == IFX_TAPI_PCM_IF_DRIVE_ENTIRE);
    for (int i = 0; i < 3; i++) {
        reset(&dxs); present &= ~(1u << i);
        assert(dxs_pcm_settings(&dxs) == -EINVAL);
    }
    for (int i = 1; i < 5; i++) {
        reset(&dxs); values[i] = i < 3 ? 2 : 8;
        assert(dxs_pcm_settings(&dxs) == -EINVAL);
    }
    for (int i = 0; i < 5; i++) {
        reset(&dxs); malformed |= 1u << i;
        assert(dxs_pcm_settings(&dxs) == -EINVAL);
    }
    reset(&dxs); values[0] = 2000000;
    assert(dxs_pcm_settings(&dxs) == -EINVAL);
    reset(&dxs); present = 7;
    assert(dxs_pcm_settings(&dxs) == 0);
    assert(dxs.pcm_cfg.nOffsetTX == 0 && dxs.pcm_cfg.nOffsetRX == 0);
    const u32 hz[] = {1024000, 2048000, 4096000, 8192000};
    const unsigned selectors[] = {1, 3, 4, 5};
    for (int i = 0; i < 4; i++) {
        reset(&dxs); values[0] = hz[i];
        assert(dxs_pcm_settings(&dxs) == 0 && dxs.pcm_cfg.nDCLFreq == selectors[i]);
    }
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='dxs-pcm-settings-') as tmp:
            binary = Path(tmp) / 'test'
            subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-x', 'c', '-', '-o', str(binary)],
                           input=harness, text=True, check=True)
            subprocess.run([str(binary)], check=True, timeout=5)


if __name__ == '__main__':
    unittest.main()
