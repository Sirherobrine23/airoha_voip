"""Actual transfer callback: completion counts and deferred trace emission."""
from dxs_trace_test_helpers import ROOT, function, run_c
import unittest

class CsiCalibrationTrace(unittest.TestCase):
    def test_controller_completion(self):
        source = (ROOT/'src/en75xx_isi_spi.c').read_text()
        harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
typedef uint8_t u8;
typedef uint32_t u32;
struct en75xx_isi_spi {
    void *base, *dev; int lock; bool csi, status_ack_rmw;
    u32 last_tx_status, last_ack;
    bool trace_header_valid;
    unsigned int trace_header_done;
    int trace_header_ret;
    u32 trace_header_status, trace_header_ack;
};
struct spi_controller { struct en75xx_isi_spi *isi; };
struct spi_device { int ignored; };
struct spi_transfer { const void *tx_buf; void *rx_buf; unsigned int len; };
static bool csi_trace, legacy_chan_sel, locked;
static int writes, fail_at, printed;
static unsigned int recorded_done[2];
static int recorded_ret[2];
static u32 recorded_status[2], recorded_ack[2];
static void info(void *dev, const char *fmt, ...)
{
    (void)dev; assert(!locked && printed < 2);
    va_list ap; va_start(ap,fmt);
    int len = va_arg(ap,int);
    const u8 *bytes = va_arg(ap,const u8 *);
    assert(len == 6 && bytes[0] == 0x7e && bytes[1] == 0x14);
    recorded_done[printed] = va_arg(ap,unsigned int);
    recorded_ret[printed] = va_arg(ap,int);
    recorded_status[printed] = va_arg(ap,u32);
    recorded_ack[printed] = va_arg(ap,u32);
    va_end(ap); printed++;
}
#define dev_info info
#define unlikely(x) (x)
#define ISI_REG_CHAN_SEL 0x14
#define spin_lock_irqsave(lock,flags) do { flags=0; locked=true; } while(0)
#define spin_unlock_irqrestore(lock,flags) do { (void)flags; locked=false; } while(0)
static void writel(u32 v,void *p) { (void)v;(void)p; }
static int spi_get_chipselect(struct spi_device *s,int n) { return n; }
static struct en75xx_isi_spi *spi_controller_get_devdata(struct spi_controller *s)
{ return s->isi; }
static void udelay(int n) { assert(n==10 && locked); }
static int isi_write_byte(struct en75xx_isi_spi *isi,u8 v)
{
    assert(locked); writes++;
    isi->last_tx_status=0x22; isi->last_ack=0x23;
    return writes==fail_at ? -ETIMEDOUT : 0;
}
static int isi_read_byte(struct en75xx_isi_spi *isi,u8 *v)
{ *v=0;return 0; }
'''
        for sig in ('static void en75xx_csi_trace_tx(',
                    'static int en75xx_isi_spi_transfer_one('):
            harness += '\n' + function(source,sig)
        harness += r'''
int main(void)
{
    const u8 head[]={0x7e,0x14,1,0,7,4}, payload[]={0x7e,0x14,0,6,0,0};
    const u8 other[]={0}; u8 rx;
    struct en75xx_isi_spi isi={.csi=true};
    struct spi_controller ctl={&isi}; struct spi_device spi={0};
    struct spi_transfer h={head,0,6},p={payload,0,6},o={other,0,1},r={0,&rx,1};
    csi_trace=true;
    assert(!en75xx_isi_spi_transfer_one(&ctl,&spi,&h));
    assert(printed==0 && isi.trace_header_valid); /* nothing between words */
    assert(!en75xx_isi_spi_transfer_one(&ctl,&spi,&p));
    assert(printed==2 && recorded_done[0]==6 && recorded_done[1]==6);
    assert(recorded_ret[0]==0 && recorded_ret[1]==0);
    assert(recorded_status[1]==0x22 && recorded_ack[1]==0x23);
    printed=writes=0; fail_at=9; /* byte 3 in the payload fails */
    assert(!en75xx_isi_spi_transfer_one(&ctl,&spi,&h));
    assert(en75xx_isi_spi_transfer_one(&ctl,&spi,&p)==-ETIMEDOUT);
    assert(printed==2 && recorded_done[1]==2 && recorded_ret[1]==-ETIMEDOUT);
    printed=writes=fail_at=0;
    assert(!en75xx_isi_spi_transfer_one(&ctl,&spi,&h));
    assert(!en75xx_isi_spi_transfer_one(&ctl,&spi,&o));
    assert(!en75xx_isi_spi_transfer_one(&ctl,&spi,&p)); assert(printed==0);
    assert(!en75xx_isi_spi_transfer_one(&ctl,&spi,&h));
    assert(!en75xx_isi_spi_transfer_one(&ctl,&spi,&r));
    assert(!en75xx_isi_spi_transfer_one(&ctl,&spi,&p)); assert(printed==0);
    csi_trace=false;
    assert(!en75xx_isi_spi_transfer_one(&ctl,&spi,&h));
    assert(!en75xx_isi_spi_transfer_one(&ctl,&spi,&p)); assert(printed==0);
    csi_trace=true; isi.csi=false;
    assert(!en75xx_isi_spi_transfer_one(&ctl,&spi,&h));
    assert(!en75xx_isi_spi_transfer_one(&ctl,&spi,&p)); assert(printed==0);
}
'''
        run_c(harness)
