#!/usr/bin/env python3
"""Host regressions using the actual driver functions with mocked I/O."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]


def run_c(source):
    with tempfile.TemporaryDirectory(prefix='en75xx-test-') as tmp:
        binary = pathlib.Path(tmp) / 'test'
        subprocess.run(['cc', '-Wall', '-Werror', '-funsigned-char', '-pthread', '-x', 'c', '-',
                        '-o', str(binary)], input=source, text=True, check=True)
        subprocess.run([str(binary)], check=True, timeout=5)


class ReviewFixes(unittest.TestCase):
    def test_digit_error_on_unsigned_char_target(self):
        source = (ROOT / 'asterisk/chan_en75xx.c').read_text()
        switch = function(source, 'static void *ss_thread(')
        body = switch[switch.index('for (;;) {') + len('for (;;) {'):]
        body = body[:body.index('if (digit == 0)')]
        harness = r'''
#include <assert.h>
#include <stddef.h>
static int hungup;
#define ast_waitfordigit(chan,timeout) (-1)
#define stop_dialtone(p,chan) ((void)0)
#define line_set_alc(p,on) ((void)0)
#define ast_hangup(chan) (hungup++)
static void *check(void) {
'''
        harness += body + '\nreturn NULL; }\n'
        harness += 'int main(void) { check(); assert(hungup == 1); return 0; }\n'
        run_c(harness)

    def test_linefeed_errors_and_cadence(self):
        source = (ROOT / 'src/en75xx_slic_si3219x.c').read_text()
        harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
typedef unsigned char u8;
#define RC_NONE 0
#define RC_SPI_FAIL 8
#define LF_FWD_ACTIVE 1
#define LF_RINGING 4
#define PROSLIC_REG_LINEFEED 30
#define PROSLIC_REG_IRQEN1 22
#define PROSLIC_REG_AUTORD 80
struct work_struct { int unused; };
struct en75xx_si3219x {
    struct work_struct ring_work;
    int linefeed_lock;
    bool ring_enabled, ring_phase;
    unsigned int ring_on_ms, ring_off_ms;
};
static struct { u8 irqen1; } Si3219x_General_Configuration = { 0x80 };
static u8 regs[256];
static int operation, fail_at, queued, delay_ms;
#define mutex_lock(lock) ((void)(lock))
#define mutex_unlock(lock) ((void)(lock))
#define READ_ONCE(x) (x)
#define WRITE_ONCE(x,v) ((x) = (v))
#define to_delayed_work(w) (w)
#define container_of(w,t,m) ((t *)(w))
#define dev_warn_ratelimited(...) ((void)0)
#define system_wq 0
#define msecs_to_jiffies(ms) (ms)
#define max_t(t,a,b) ((t)(a) > (t)(b) ? (a) : (b))
#define cancel_delayed_work_sync(w) (queued = 0)
#define mod_delayed_work(q,w,ms) (queued++, delay_ms = (ms))
static int si3219x_read_reg_checked(struct en75xx_si3219x *s, u8 ch, u8 reg)
{ return ++operation == fail_at ? -EIO : regs[reg]; }
static int si3219x_write_reg(struct en75xx_si3219x *s, u8 ch, u8 reg, u8 val)
{ if (++operation == fail_at) return RC_SPI_FAIL; regs[reg] = val; return 0; }
static void reset(int lf, int failure)
{
    regs[PROSLIC_REG_LINEFEED] = lf;
    regs[PROSLIC_REG_AUTORD] = 0x2f;
    regs[PROSLIC_REG_IRQEN1] = 0x80;
    fail_at = failure; operation = queued = delay_ms = 0;
}
'''
        for signature in ('static int si3219x_set_linefeed(',
                          'static void en75xx_si3219x_ring_work(',
                          'static int en75xx_si3219x_ring('):
            harness += '\n' + function(source, signature)
        harness += r'''
int main(void)
{
    struct en75xx_si3219x s = { 0 };
    int i;
    reset(LF_FWD_ACTIVE, 0);
    assert(en75xx_si3219x_ring(&s, true, 1000, 4000) == 0);
    assert(s.ring_enabled && s.ring_phase && queued == 1 && delay_ms == 1000);
    assert(regs[PROSLIC_REG_LINEFEED] == LF_RINGING);
    assert(!(regs[PROSLIC_REG_IRQEN1] & 0x80));
    for (i = 1; i <= 4; i++) {
        reset(LF_FWD_ACTIVE, i);
        assert(en75xx_si3219x_ring(&s, true, 1000, 4000) < 0);
        assert(!s.ring_enabled && !queued);
    }
    for (i = 1; i <= 6; i++) {
        reset(LF_RINGING, i);
        assert(en75xx_si3219x_ring(&s, false, 0, 0) < 0);
        assert(!s.ring_enabled && !queued);
    }
    reset(LF_RINGING, 0);
    assert(en75xx_si3219x_ring(&s, false, 0, 0) == 0);
    assert(!s.ring_phase && regs[PROSLIC_REG_LINEFEED] == LF_FWD_ACTIVE);
    assert(regs[PROSLIC_REG_AUTORD] == 0x2f && regs[PROSLIC_REG_IRQEN1] == 0x80);
    reset(LF_RINGING, 0);
    assert(en75xx_si3219x_ring(&s, true, 1000, 4000) == 0);
    assert(s.ring_phase && delay_ms == 1000); /* already ringing is success */
    reset(0x41, 0);
    regs[PROSLIC_REG_IRQEN1] = 0;
    assert(si3219x_set_linefeed(&s, LF_FWD_ACTIVE) == -EAGAIN);
    assert(regs[PROSLIC_REG_AUTORD] == 0x2f && regs[PROSLIC_REG_IRQEN1] == 0);
    reset(LF_FWD_ACTIVE, 4);
    s.ring_enabled = true; s.ring_phase = false;
    en75xx_si3219x_ring_work(&s.ring_work);
    assert(!s.ring_phase && delay_ms == 20);
    fail_at = 0;
    en75xx_si3219x_ring_work(&s.ring_work);
    assert(s.ring_phase && delay_ms == 1000);
    s.ring_enabled = false; queued = 0;
    en75xx_si3219x_ring_work(&s.ring_work);
    assert(!queued);
    /* A board configuration with the VBAT interrupt disabled. */
    Si3219x_General_Configuration.irqen1 = 0;
    reset(LF_FWD_ACTIVE, 0);
    assert(si3219x_set_linefeed(&s, LF_RINGING) == 0);
    assert(operation == 2 && regs[PROSLIC_REG_IRQEN1] == 0x80);
    return 0;
}
'''
        run_c(harness)

    def test_hook_lock_contention_and_owner_change(self):
        source = (ROOT / 'asterisk/chan_en75xx.c').read_text()
        harness = r'''
#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
enum { EN75XX_IDLE, EN75XX_DIALING, EN75XX_RINGING, EN75XX_UP };
enum { AST_CONTROL_ANSWER, AST_STATE_UP, EN75XX_VOICE_LINEFEED_STANDBY };
struct ast_channel { pthread_mutex_t lock; int refs; };
struct en75xx_pvt {
    pthread_mutex_t lock;
    struct ast_channel *owner;
    int offhook, state, ringing;
};
static sem_t avoided, replaced;
static int queue_count, outgoing, force_contention;
#define ast_mutex_lock pthread_mutex_lock
#define ast_mutex_unlock pthread_mutex_unlock
#define ast_channel_trylock(c) pthread_mutex_trylock(&(c)->lock)
#define ast_channel_unlock(c) pthread_mutex_unlock(&(c)->lock)
#define ast_debug(...) ((void)0)
static struct ast_channel *ast_channel_ref(struct ast_channel *c)
{ c->refs++; return c; }
static void ast_channel_unref(struct ast_channel *c) { c->refs--; }
static void avoid(pthread_mutex_t *lock)
{
    pthread_mutex_unlock(lock);
    assert(force_contention);
    sem_post(&avoided);
    sem_wait(&replaced);
    pthread_mutex_lock(lock);
}
#define DEADLOCK_AVOIDANCE(lock) avoid(lock)
static void line_set_ring(struct en75xx_pvt *p, int v) { p->ringing = v; }
static void line_set_linefeed(struct en75xx_pvt *p, int v) {}
static void start_outgoing_call(struct en75xx_pvt *p) { outgoing++; }
static void ast_queue_control(struct ast_channel *c, int v)
{ pthread_mutex_lock(&c->lock); queue_count++; pthread_mutex_unlock(&c->lock); }
static void ast_setstate(struct ast_channel *c, int v) {}
static void ast_queue_hangup(struct ast_channel *c) { ast_queue_control(c, 0); }
'''
        harness += function(source, 'static void handle_hook_change(')
        harness += r'''
static void *monitor(void *arg) { handle_hook_change(arg, 0); return NULL; }
int main(void)
{
    pthread_mutexattr_t attr;
    struct ast_channel c = { .refs = 1 };
    struct en75xx_pvt p = { .owner = &c, .offhook = 1, .state = EN75XX_UP };
    pthread_t thread;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&c.lock, &attr);
    pthread_mutex_init(&p.lock, &attr);
    sem_init(&avoided, 0, 0); sem_init(&replaced, 0, 0);
    /* PBX holds channel, monitor tries line->channel; PBX needs line next. */
    force_contention = 1;
    pthread_mutex_lock(&c.lock);
    pthread_create(&thread, NULL, monitor, &p);
    sem_wait(&avoided);
    pthread_mutex_lock(&p.lock);
    p.owner = NULL; /* hangup clears the owner while monitor has yielded */
    pthread_mutex_unlock(&p.lock);
    pthread_mutex_unlock(&c.lock);
    sem_post(&replaced);
    pthread_join(thread, NULL);
    assert(queue_count == 0 && c.refs == 1 && p.state == EN75XX_IDLE);
    force_contention = 0;
    p.owner = &c; p.state = EN75XX_RINGING; p.ringing = 1;
    handle_hook_change(&p, 1);
    assert(queue_count == 1 && p.state == EN75XX_UP && !p.ringing && c.refs == 1);
    handle_hook_change(&p, 0);
    assert(queue_count == 2 && c.refs == 1);
    handle_hook_change(&p, 0); /* duplicate event */
    assert(queue_count == 2 && c.refs == 1);
    p.state = EN75XX_IDLE;
    handle_hook_change(&p, 1); /* incoming allocation already owns the line */
    assert(outgoing == 0);
    p.owner = NULL; p.offhook = 0;
    handle_hook_change(&p, 1);
    assert(outgoing == 1);
    assert(!pthread_mutex_trylock(&p.lock)); pthread_mutex_unlock(&p.lock);
    assert(!pthread_mutex_trylock(&c.lock)); pthread_mutex_unlock(&c.lock);
    return 0;
}
'''
        run_c(harness)

    def test_led_registration_modes(self):
        source = (ROOT / 'openwrt/package/asterisk-chan-en75xx/files/en75xx-fxs-led').read_text()
        ready = function(source, 'sip_ready()').replace('/usr/sbin/asterisk', 'mock_asterisk')
        prelude = '''
uci() { printf '%s' "$MODE"; }
mock_asterisk() {
    case "$2" in
        'pjsip show registrations') printf '%s\\n' "$REG"; return "$REG_STATUS" ;;
        'pjsip show contacts') printf '%s\\n' "$CONTACT" ;;
    esac
}
'''
        contact = ' Contact: phone/sip:phone@192.0.2.2 abc123 Avail 5.0'
        missing = "No such command 'pjsip show registrations'"
        cases = [
            ('', ' trunk/sip:server auth Registered', 0, '', 0),
            ('', 'No objects found.', 0, contact, 0),
            ('', missing, 0, contact, 0),
            ('outbound', missing, 0, contact, 1),
            ('outbound', 'No objects found.', 0, contact, 1),
            ('', ' trunk/sip:server auth Rejected', 0, contact, 1),
            ('', missing, 0, 'No objects found.', 1),
            ('', 'Unable to connect to remote asterisk', 1, contact, 1),
            ('contacts', '', 1, contact, 0),
            ('contacts', '', 0, contact.replace('Avail', 'Unavail'), 1),
        ]
        for mode, reg, status, contacts, expected in cases:
            with self.subTest(mode=mode, registration=reg, contact=contacts):
                result = subprocess.run(['sh', '-c', prelude + ready + '\nsip_ready'],
                                        env={'MODE': mode, 'REG': reg, 'REG_STATUS': str(status),
                                             'CONTACT': contacts, 'PATH': '/usr/bin:/bin'})
                self.assertEqual(result.returncode, expected)


if __name__ == '__main__':
    unittest.main()
