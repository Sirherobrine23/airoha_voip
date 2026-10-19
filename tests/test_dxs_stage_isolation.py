"""Run actual C stage boundaries/parser with mocked transport and clocks."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
BACKEND = ROOT / 'vendor/maxlinear/drv_tapi_dxs-1.0.0.0/src/dxs/drv_dxs_bbd.c'


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]


def run_c(source):
    with tempfile.TemporaryDirectory(prefix='dxs-isolation-') as tmp:
        binary = Path(tmp) / 'test'
        subprocess.run(['cc', '-Wall', '-Werror', '-x', 'c', '-', '-o', str(binary)],
                       input=source, text=True, check=True)
        subprocess.run([str(binary)], check=True, timeout=5)


class DxsStageIsolation(unittest.TestCase):
    def test_stage_boundaries_and_firmware_loss(self):
        source = (ROOT / 'src/en75xx_slic_dxs.c').read_text()
        functions = '\n'.join(function(source, name) for name in (
            'static int dxs_boot_firmware(', 'static int dxs_verify_firmware(',
            'static int dxs_wait_stage('))
        probe = source[source.index('static int en75xx_dxs_probe('):]
        begin = probe.index('\tret = dxs_boot_firmware(dxs, fw);')
        end = probe.index('\tdev_info(dev, "DXS BBD accepted:')
        boundary = probe[begin:end]
        harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
typedef uint32_t u32;
struct firmware { const unsigned char *data; size_t size; };
struct en75xx_dxs { void *dev, *handle; unsigned index, count; u32 loaded_fw; bool started; };
static unsigned stage_wait_ms = 1000;
static u32 live_fw;
static int reads, sleeps, reset_on_sleep, reset_at_start, fail_read;
static int starts, prepares, calibrates, completed, cleanup, released, voice_path, bbd_error;
#define dev_info(...) ((void)0)
#define dev_err(...) ((void)0)
#define IFX_TAPI_DEV_START 1
static int en75xx_dxs_kernel_version(unsigned i, u32 *hw, u32 *fw, unsigned *count)
{ reads++; if (reads == fail_read) return -ETIMEDOUT; *hw=12; *fw=live_fw; *count=1; return 0; }
static int en75xx_dxs_kernel_firmware(unsigned i, const unsigned char *data, size_t size)
{ live_fw=0x02010100; return 0; }
static void msleep(unsigned ms)
{ assert(ms == stage_wait_ms); sleeps++; if(sleeps == reset_on_sleep) live_fw=0x02000000; }
static int dxs_ioctl(struct en75xx_dxs *d, void *h, unsigned cmd, void *arg)
{ assert(cmd == IFX_TAPI_DEV_START); starts++; if(reset_at_start) live_fw=0x02000000; return 0; }
static int en75xx_dxs_kernel_bbd_prepare(unsigned i, const unsigned char *data, size_t size)
{ prepares++; return bbd_error; }
static int en75xx_dxs_kernel_bbd(unsigned i, const unsigned char *data, size_t size)
{ calibrates++; return bbd_error; }
static void platform_set_drvdata(void *p, struct en75xx_dxs *d) { completed++; }
static void release_firmware(const struct firmware *f) { released++; }
'''
        harness += functions
        harness += r'''
static int run_stage(struct en75xx_dxs *dxs, int mode)
{
    bool voice=mode==3, start_only=mode==1, bbd_only=mode==2;
    const struct firmware image={(void *)1, 498}, *fw=&image, *bbd=&image;
    void *dev=0, *pdev=0;
    int start=0, ret;
    u32 hw_version, fw_version;
    unsigned fxs_count;
    (void)dev;
'''
        harness += boundary
        harness += r'''
    voice_path++;
    return 0;
fail:
    cleanup++;
    return ret;
}
static void reset(struct en75xx_dxs *d)
{
    *d=(struct en75xx_dxs){.count=1};
    live_fw=0x02000000;
    reads=sleeps=reset_on_sleep=reset_at_start=fail_read=0;
    starts=prepares=calibrates=completed=cleanup=released=voice_path=bbd_error=0;
}
int main(void)
{
    struct en75xx_dxs d;
    reset(&d); assert(run_stage(&d,0)==0);
    assert(reads==3 && sleeps==1 && !starts && !prepares && !calibrates && completed==1);
    reset(&d); assert(run_stage(&d,1)==0);
    assert(starts==1 && sleeps==1 && !prepares && !calibrates && completed==1);
    reset(&d); assert(run_stage(&d,2)==0);
    assert(starts==1 && sleeps==2 && prepares==1 && !calibrates && !voice_path && released==2);
    reset(&d); assert(run_stage(&d,3)==0);
    assert(starts==1 && !sleeps && !prepares && calibrates==1 && voice_path==1 && !completed);
    reset(&d); reset_on_sleep=1;
    assert(run_stage(&d,0)==-EIO && cleanup==1 && !starts && !completed);
    reset(&d); reset_at_start=1;
    assert(run_stage(&d,3)==-EIO && !calibrates && !prepares && !voice_path);
    reset(&d); reset_on_sleep=1;
    assert(run_stage(&d,2)==-EIO && !prepares && !calibrates && !completed);
    reset(&d); reset_on_sleep=2;
    assert(run_stage(&d,2)==-EIO && prepares==1 && !calibrates && !completed);
    reset(&d); fail_read=3;
    assert(run_stage(&d,1)==-ETIMEDOUT && !completed);
    reset(&d); bbd_error=-EIO;
    assert(run_stage(&d,3)==-EIO && reads==4 && !voice_path && !completed);
    return 0;
}
'''
        run_c(harness)

    def test_bbd_prepare_keeps_calibration_pending_and_voice_requires_it(self):
        source = BACKEND.read_text()
        functions = '\n'.join(function(source, name) for name in (
            'static IFX_int32_t dxs_bbd_verify_firmware(',
            'static IFX_int32_t dxs_bbd_download('))
        harness = r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define __KERNEL__ 1
typedef int IFX_int32_t;
typedef unsigned IFX_uint32_t;
typedef unsigned char IFX_uint8_t;
typedef void IFX_void_t;
typedef int IFX_boolean_t;
#define IFX_NULL NULL
#define IFX_TRUE 1
#define IFX_FALSE 0
#define DXS_statusOk 0
#define DXS_statusErr 9
#define DXS_statusParam 10
#define DXS_statusInvalCh 11
#define DXS_statusNoMem 12
#define DXS_statusBbdCorrupt 13
#define DXS_statusBbdBlockErr 14
#define DXS_statusAutomaticCalibrationFailed 15
#define DXS_MAX_CH_NR 2
#define DXS_MAX_BBD_FIRMWARE_SIZE 4096
#define BBD_DXS_MAGIC 0x44585321
#define BBD_DXS_BLOCK_MAXDWNLD 1
#define DXS_SUCCESS(ret) ((ret)==0)
#define RETURN_STATUS(ret,info) return (ret)
#define RETURN_DEVSTATUS(ret,info) return (ret)
#define pr_err(...) ((void)0)
#define pr_info(...) ((void)0)
#define tapi_debug_buffer_add_user_entry(...) ((void)0)
#define TAPI_OS_Malloc malloc
#define TAPI_OS_Free free
#define TAPI_OS_CpyUsr2Kern memcpy
typedef struct { int bCalibrationNeeded; } ALM;
typedef struct dev DXS_DEVICE_t;
typedef struct { unsigned nChannel; ALM *pALM; DXS_DEVICE_t *pParent; } DXS_CHANNEL_t;
struct dev {
    unsigned nChannel, nDevNr, nFwRev;
    DXS_CHANNEL_t pChannel[2];
    struct { unsigned nALI; } caps;
};
typedef struct { unsigned char *buf; unsigned size; } DXS_BBD_Download_t;
typedef struct { unsigned char *buf; unsigned size; } bbd_format_t;
typedef struct { unsigned identifier, tag, index; unsigned char *pData; unsigned size; } bbd_block_t;
typedef int bbd_error_t;
#define BBD_INTG_ERR_INVALID 1
#define BBD_INTG_OK 0
static const unsigned DXS_CH_BBD_Blocks[]={0x1001};
static int reads, blocks, calibrations, read_error, calibration_error, integrity_error;
static unsigned live_fw;
static int DXS_ReadFwVersion(DXS_DEVICE_t *d)
{ reads++; if(read_error) return read_error; d->nFwRev=live_fw; return 0; }
static int bbd_check_integrity(bbd_format_t *b, unsigned magic) { return integrity_error; }
static void bbd_get_block(bbd_format_t *b, bbd_block_t *block)
{ block->pData=b->buf; block->size=b->size; }
static int dxs_bbd_BlockHandler(DXS_CHANNEL_t *ch, bbd_block_t *block)
{ blocks++; ch->pALM->bCalibrationNeeded=1; return 0; }
static int DXS_ALM_Calibration(DXS_CHANNEL_t *ch)
{ calibrations++; if(calibration_error) return calibration_error; ch->pALM->bCalibrationNeeded=0; return 0; }
'''
        harness += functions
        harness += r'''
static void reset(DXS_DEVICE_t *d, ALM *alm, unsigned count)
{
    memset(d,0,sizeof(*d)); memset(alm,0,2*sizeof(*alm));
    reads=blocks=calibrations=read_error=calibration_error=integrity_error=0;
    d->nFwRev=live_fw=0x02010300; d->caps.nALI=count;
    for(unsigned i=0;i<count;i++)
        d->pChannel[i]=(DXS_CHANNEL_t){.nChannel=i+1,.pALM=&alm[i],.pParent=d};
}
int main(void)
{
    DXS_DEVICE_t d;
    ALM alm[2]; unsigned char data[16]={0};
    DXS_BBD_Download_t bbd={data,sizeof(data)};
    reset(&d,alm,1);
    assert(dxs_bbd_download((void *)&d,&bbd,1,0)==0);
    assert(blocks==1 && reads==1 && !calibrations && alm[0].bCalibrationNeeded);
    reset(&d,alm,1);
    assert(dxs_bbd_download((void *)&d,&bbd,1,1)==0);
    assert(reads==1 && calibrations==1 && !alm[0].bCalibrationNeeded);
    reset(&d,alm,2);
    assert(dxs_bbd_download((void *)&d,&bbd,1,1)==0 && calibrations==2);
    reset(&d,alm,1); live_fw=0x02000000;
    assert(dxs_bbd_download((void *)&d,&bbd,1,1)==DXS_statusErr);
    assert(reads==1 && !calibrations && alm[0].bCalibrationNeeded);
    reset(&d,alm,1); read_error=22;
    assert(dxs_bbd_download((void *)&d,&bbd,1,1)==22 && !calibrations);
    reset(&d,alm,1); calibration_error=23;
    assert(dxs_bbd_download((void *)&d,&bbd,1,1)==DXS_statusAutomaticCalibrationFailed);
    reset(&d,alm,1); integrity_error=1;
    assert(dxs_bbd_download((void *)&d,&bbd,1,0)==DXS_statusBbdCorrupt && !blocks);
    reset(&d,alm,1);
    assert(dxs_bbd_download(&d.pChannel[0],&bbd,1,1)==0 && calibrations==1);
    reset(&d,alm,1);
    assert(dxs_bbd_download((void *)&d,&bbd,0,1)==0 && !reads && calibrations==1);
    return 0;
}
'''
        run_c(harness)


if __name__ == '__main__':
    unittest.main()
