"""Compile the actual CSI selection and post-firmware HOST_CFG programming."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
VENDOR = ROOT / 'vendor/maxlinear/drv_tapi_dxs-1.0.0.0/src/dxs'


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]


class RuntimeHostConfig(unittest.TestCase):
    def test_csi_normal_spi_and_register_failures(self):
        access = (VENDOR / 'drv_dxs_access.c').read_text()
        init = (VENDOR / 'drv_dxs_init.c').read_text()
        helpers = '\n'.join(function(access, sig) for sig in (
            'static IFX_boolean_t dxs_is_csi(', 'IFX_uint16_t DXS_RuntimeHostConfig('))
        pos = init.index('const IFX_uint16_t nRegWrite = DXS_RuntimeHostConfig(')
        start = init.rfind('\n   {', 0, pos)
        end = init.index('\n   }\n\n   RETURN_DEVSTATUS', pos) + len('\n   }')
        block = init[start:end]
        harness = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uint16_t IFX_uint16_t;
typedef uint32_t IFX_vuint32_t;
typedef int IFX_boolean_t;
#define IFX_TRUE 1
#define IFX_FALSE 0
#define __KERNEL__ 1
#include "drv_dxs.h"
struct device_node { const char *compatible; };
struct device { struct device_node *of_node; };
struct controller { struct device dev; };
struct spi_device { struct controller *controller; };
typedef struct { struct spi_device *pSpiDev; unsigned nDevNr; } DXS_DEVICE_t;
static int of_device_is_compatible(struct device_node *n, const char *s)
{ return n && !strcmp(n->compatible,s); }
#define DXS_statusOk 0
#define DXS_statusReadErr 10
#define tapi_debug_buffer_add_user_entry(...) ((void)0)
#define pr_info(...) ((void)0)
static int writes, reads, write_error, read_error;
static unsigned last_write, returned;
static int DXS_RegWrite(DXS_DEVICE_t *d, unsigned reg, unsigned value)
{ assert(reg==6); writes++; last_write=value; return write_error; }
static int DXS_RegRead(DXS_DEVICE_t *d, unsigned reg, IFX_uint16_t *value)
{ assert(reg==6); reads++; *value=returned; return read_error; }
'''
        harness += helpers + '\nstatic int program(DXS_DEVICE_t *pDev)\n{\nint ret;\n'
        harness += block + '\nreturn ret;\n}\n'
        harness += r'''
static void reset(unsigned value)
{ writes=reads=write_error=read_error=0; returned=value; }
int main(void)
{
    struct device_node node={"airoha,en7523-csi-spi"};
    struct controller ctl={.dev={.of_node=&node}};
    struct spi_device spi={.controller=&ctl};
    DXS_DEVICE_t d={.pSpiDev=&spi};
    reset(7); assert(program(&d)==0 && last_write==7 && writes==1 && reads==1);
    node.compatible="econet,en751221-csi-spi";
    assert(dxs_is_csi(&d) && DXS_RuntimeHostConfig(&d)==6);
    node.compatible="airoha,en7523-isi-spi";
    reset(6); assert(program(&d)==0 && last_write==6);
    ctl.dev.of_node=0; assert(DXS_RuntimeHostConfig(&d)==6);
    d.pSpiDev=0; assert(DXS_RuntimeHostConfig(&d)==6);
    d.pSpiDev=&spi; ctl.dev.of_node=&node; node.compatible="airoha,en7523-csi-spi";
    reset(7); write_error=23; assert(program(&d)==23 && !reads);
    reset(7); read_error=24; assert(program(&d)==24 && reads==1);
    reset(6); assert(program(&d)==DXS_statusReadErr);
    reset(0x107); assert(program(&d)==0);
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='dxs-hostcfg-') as tmp:
            binary = Path(tmp) / 'test'
            subprocess.run(['cc', '-Wall', '-Werror', '-I', str(VENDOR),
                            '-x', 'c', '-', '-o', str(binary)],
                           input=harness, text=True, check=True)
            subprocess.run([str(binary)], check=True, timeout=5)


if __name__ == '__main__':
    unittest.main()
