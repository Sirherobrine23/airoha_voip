"""Compile actual polling/IRQ observation paths with mocked register access."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
IRQ = ROOT / 'vendor/maxlinear/drv_tapi_dxs-1.0.0.0/src/dxs/drv_dxs_irq.c'


def function(source, signature):
    start = source.index(signature)
    # Skip the earlier prototype of DXS_interrupt_handler.
    while source.find(';', start) < source.find('{', start):
        start = source.index(signature, start + len(signature))
    return source[start:source.index('\n}', start) + 2]


class HostObserver(unittest.TestCase):
    def test_progress_and_register_snapshots(self):
        source = IRQ.read_text()
        a = source.index('struct dxs_trace_progress {')
        progress = source[a:source.index('\n#else\n#define DXS_TRACE_COUNT', a)]
        functions = '\n'.join(function(source, sig) for sig in (
            'enum IFX_irqreturn_t DXS_interrupt_handler(',
            'static IFX_void_t dxs_polling_mode_timer ('))
        harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#define __KERNEL__ 1
typedef int IFX_int32_t;
typedef unsigned short IFX_uint16_t;
typedef unsigned char IFX_uint8_t;
typedef unsigned long IFX_ulong_t;
typedef void IFX_void_t;
typedef int Timer_ID;
typedef struct { int counter; } atomic_t;
static void atomic_inc(atomic_t *a) { a->counter++; }
static int atomic_read(const atomic_t *a) { return a->counter; }
enum IFX_irqreturn_t { IFX_IRQ_NONE, IFX_IRQ_HANDLED };
typedef struct { unsigned nDevNr, nDevState, nEventHandlingMode, nFwRev; void *pChannel; } DXS_DEVICE_t;
/* The real handler accesses pChannel[0].pTapiCh. */
struct channel { void *pTapiCh; };
typedef struct { unsigned nDevNr, nDevState, nEventHandlingMode, nFwRev; struct channel *pChannel; } ActualDevice;
#define DXS_DEVICE_t ActualDevice
typedef struct { int id; } IFX_TAPI_EVENT_t;
#define IFX_NULL NULL
#define DXS_MAX_DEVICES 2
#define DXS_statusOk 0
#define DS_SPI_ACTIVE 0x80
#define DS_BASIC_INIT 4
#define DXS_EVENT_STOP 0
#define DXS_EVENT_POLLING 2
#define DXS_HOST_CFG 0x06
#define DXS_HOST_IEN1 0x08
#define DXS_HOST_IEN2 0x0a
#define DXS_HOST_INT1 0x10
#define DXS_HOST_INT2 0x12
#define DXS_HOST_LEN 0x18
#define DXS_HOST_BINF 0x24
#define DXS_REG_IEN1_ERR 1
#define DXS_REG_IEN1_OBX_RDY 0x800
#define DXS_REG_IEN2_OBX_UFL 0x200
#define DXS_REG_IEN2_IBX_OFL 0x100
#define IFX_TAPI_EVENT_FAULT_FW_CBO_UF 1
#define IFX_TAPI_EVENT_FAULT_FW_CBI_OF 2
#define IFX_TAPI_EVENT_FAULT_HW_SPI_ACCESS 3
#define IFX_TAPI_MODULE_TYPE_NONE 0
#define DXS_TAPI_EVENT_MODULE_SET(...) ((void)0)
#define TAPI_UNUSED(x) ((void)(x))
static void trace_log(const char *format, ...) { (void)format; }
#define pr_info(...) trace_log(__VA_ARGS__)
#define pr_err(...) trace_log(__VA_ARGS__)
#define pr_info_once(...) ((void)0)
static bool en75xx_dxs_event_trace;
static struct channel channel;
static ActualDevice device;
static unsigned offsets[100], nreads, int1;
static int int_error, reg_error, dispatches, outbox_calls, rebooting;
static int DXS_GetDevice(unsigned i, ActualDevice **d)
{ if(i) return 1; *d=&device; return 0; }
static int DXS_RegRead(ActualDevice *d, unsigned offset, IFX_uint16_t *value)
{
    assert(nreads<100); offsets[nreads++]=offset;
    *value=offset==DXS_HOST_INT1 ? int1 : 7;
    return offset==DXS_HOST_INT1 ? int_error : reg_error;
}
static int DXS_RegReadMulti(ActualDevice *d, unsigned offset, IFX_uint16_t *value, unsigned count)
{ assert(offset==DXS_HOST_INT1 && count==2); value[1]=0; return DXS_RegRead(d,offset,value); }
static int DXS_RegWrite(ActualDevice *d, unsigned offset, unsigned value)
{ assert(offset==DXS_HOST_INT2); return 0; }
static int DXS_outbox_handler(ActualDevice *d) { outbox_calls++; return 0; }
static void IFX_TAPI_Event_Dispatch(void *ch, void *event) { dispatches++; }
static int TAPI_OS_isRebootOngoing(void) { return rebooting; }
'''
        harness += progress + '\n' + functions
        harness += r'''
static void reset(void)
{
    memset(dxs_progress,0,sizeof(dxs_progress));
    device=(ActualDevice){.nDevNr=0,.nDevState=0x84,.nEventHandlingMode=2,
                          .nFwRev=0x02010100,.pChannel=&channel};
    en75xx_dxs_event_trace=true;
    nreads=int1=int_error=reg_error=dispatches=outbox_calls=rebooting=0;
}
int main(void)
{
    const unsigned expected[]={6,8,10,24,36};
    reset(); en75xx_dxs_trace_snapshot(0,"before");
    assert(nreads==5 && !memcmp(offsets,expected,sizeof(expected)));
    nreads=0; reg_error=22;
    en75xx_dxs_trace_snapshot(0,"failed reads");
    assert(nreads==5 && !memcmp(offsets,expected,sizeof(expected)));
    reset(); en75xx_dxs_trace_snapshot(2,"invalid"); assert(!nreads);
    en75xx_dxs_event_trace=false;
    en75xx_dxs_trace_snapshot(0,"disabled"); assert(!nreads);
    reset();
    for(int i=0;i<3;i++) dxs_polling_mode_timer(0,0);
    assert(atomic_read(&dxs_progress[0].poll)==3);
    assert(atomic_read(&dxs_progress[0].eligible)==3);
    assert(atomic_read(&dxs_progress[0].handler)==3);
    assert(atomic_read(&dxs_progress[0].int_reads)==3);
    assert(atomic_read(&dxs_progress[0].done)==3 && !outbox_calls);
    device.nEventHandlingMode=DXS_EVENT_STOP;
    dxs_polling_mode_timer(0,0);
    assert(atomic_read(&dxs_progress[0].poll)==4 && atomic_read(&dxs_progress[0].eligible)==3);
    reset(); int1=DXS_REG_IEN1_OBX_RDY;
    dxs_polling_mode_timer(0,0);
    assert(atomic_read(&dxs_progress[0].outbox_enter)==1);
    assert(atomic_read(&dxs_progress[0].outbox_done)==1 && outbox_calls==1);
    reset(); int_error=23; dxs_polling_mode_timer(0,0);
    assert(atomic_read(&dxs_progress[0].int_reads)==1 && dispatches==1 && !outbox_calls);
    reset(); device.nDevState=DS_BASIC_INIT;
    dxs_polling_mode_timer(0,0);
    assert(atomic_read(&dxs_progress[0].eligible)==1 && !atomic_read(&dxs_progress[0].handler));
    reset(); rebooting=1; dxs_polling_mode_timer(0,0); assert(!nreads);
    reset(); en75xx_dxs_event_trace=false; dxs_polling_mode_timer(0,0);
    assert(nreads==1 && !atomic_read(&dxs_progress[0].poll));
    return 0;
}
'''
        with tempfile.TemporaryDirectory(prefix='dxs-host-observer-') as tmp:
            for mode in ([], ['-DDXS_SPI_8BIT_ACCESS=1']):
                binary = Path(tmp) / ('test8' if mode else 'test16')
                subprocess.run(['cc', '-Wall', '-Werror', '-Wno-unused-function', *mode,
                                '-x', 'c', '-', '-o', str(binary)],
                               input=harness, text=True, check=True)
                subprocess.run([str(binary)], check=True, timeout=5)


if __name__ == '__main__':
    unittest.main()
