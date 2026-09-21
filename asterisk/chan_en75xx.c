/*
 * chan_en75xx -- Asterisk channel driver for EcoNet/Airoha EN75xx FXS lines.
 *
 * Each /dev/en75xx-fxsN is one analog subscriber line. The kernel side
 * already owns everything that is timing critical: hook debounce, the
 * ring cadence and its gaps, G.711 companding on the wire. What is left
 * here is the analog telephone state machine that chan_dahdi provides
 * for DAHDI spans -- dial tone, digit collection, ring, answer, hangup --
 * without pulling DAHDI in.
 *
 * Structure, deliberately close to chan_dahdi so the behaviour is
 * familiar:
 *
 *   monitor thread   polls every line for EPOLLPRI (hook change)
 *   ss_thread        one per originating call: dial tone, collect
 *                    digits, launch the PBX ("simple switch")
 *   channel thread   Asterisk's own, does read/write on the same fd
 *
 * The monitor and the channel thread share one fd on purpose: the
 * monitor only ever polls POLLPRI and issues ioctls, the channel thread
 * only ever reads and writes audio.
 *
 * This file is licensed GPLv2, matching Asterisk.
 */

/*** MODULEINFO
	<support_level>extended</support_level>
 ***/

#include "asterisk.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "asterisk/channel.h"
#include "asterisk/config.h"
#include "asterisk/module.h"
#include "asterisk/pbx.h"
#include "asterisk/lock.h"
#include "asterisk/utils.h"
#include "asterisk/callerid.h"
#include "asterisk/causes.h"
#include "asterisk/cli.h"
#include "asterisk/dsp.h"
#include "asterisk/format_cache.h"
#include "asterisk/indications.h"
#include "asterisk/musiconhold.h"
#include "asterisk/say.h"
#include "asterisk/stringfields.h"

#include "en75xx_voice.h"	/* the driver UAPI, copied in at build time */

#define CONFIG_FILE		"en75xx.conf"
#define EN75XX_MAX_LINES	8
#define EN75XX_FRAME_BYTES	EN75XX_VOICE_FRAME_BYTES	/* 160 = 10 ms */
#define EN75XX_FRAME_SAMPLES	EN75XX_VOICE_FRAME_SAMPLES	/* 80 */

/* Digit collection, same knobs chan_dahdi exposes. */
#define FIRST_DIGIT_TIMEOUT	16000
#define INTER_DIGIT_TIMEOUT	8000
#define MATCH_DIGIT_TIMEOUT	3000

enum en75xx_state {
	EN75XX_IDLE = 0,
	EN75XX_DIALING,		/* off-hook, collecting digits */
	EN75XX_RINGING,		/* we are ringing the phone */
	EN75XX_UP,
};

struct en75xx_pvt {
	int fd;
	int index;
	char device[64];
	char slic[32];

	ast_mutex_t lock;
	struct ast_channel *owner;
	enum en75xx_state state;
	int offhook;
	int ringing;

	/* 400 Hz dial-tone notch filter state for DTMF detection during dialing */
	int16_t notch_x1, notch_x2;
	int16_t notch_y1, notch_y2;

	struct ast_dsp *dsp;
	struct ast_frame frame;
	unsigned char buf[AST_FRIENDLY_OFFSET + EN75XX_FRAME_BYTES];
	/* short reads are normal; a frame is assembled here */
	unsigned char partial[EN75XX_FRAME_BYTES];
	size_t partial_len;

	/* config */
	char context[AST_MAX_CONTEXT];
	char exten[AST_MAX_EXTENSION];
	char cid_num[AST_MAX_EXTENSION];
	char cid_name[AST_MAX_EXTENSION];
	char language[MAX_LANGUAGE];
	char mohinterpret[MAX_MUSICCLASS];
	unsigned int immediate:1;
	unsigned int hwdtmf:1;		/* trust the SLIC for DTMF */
	unsigned int relaxdtmf:1;	/* relaxed DTMF detection rules */
	unsigned int play_dialtone:1;	/* play dial tone on off-hook */
	unsigned int hwtone:1;		/* let the SLIC generate the dial tone */
	unsigned int tone_playing:1;	/* a SLIC oscillator is running */
	unsigned int inuse:1;
	unsigned int caps;		/* EN75XX_VOICE_CAP_* from the driver */
	int dialtone_volume;		/* software dial tone amplitude */
	int callprogress_volume;	/* software call progress tone amplitude */
	int dialtone_freq1;		/* hardware dial tone, Hz */
	int dialtone_freq2;		/* second oscillator, 0 for a pure tone */
	int dialtone_level;		/* hardware dial tone level, dBm */
};

static struct en75xx_pvt lines[EN75XX_MAX_LINES];
static int num_lines;

static pthread_t monitor_thread = AST_PTHREADT_NULL;
static int monitor_stop;
static ast_mutex_t monlock = AST_MUTEX_INIT_VALUE;

static struct ast_format_cap *en75xx_tech_cap;

static struct ast_channel *en75xx_request(const char *type,
	struct ast_format_cap *cap, const struct ast_assigned_ids *assignedids,
	const struct ast_channel *requestor, const char *data, int *cause);
static int en75xx_call(struct ast_channel *ast, const char *dest, int timeout);
static int en75xx_hangup(struct ast_channel *ast);
static int en75xx_answer(struct ast_channel *ast);
static struct ast_frame *en75xx_read(struct ast_channel *ast);
static struct ast_frame *en75xx_exception(struct ast_channel *ast);
static int en75xx_write(struct ast_channel *ast, struct ast_frame *frame);
static int en75xx_indicate(struct ast_channel *ast, int condition,
			   const void *data, size_t datalen);
static int en75xx_fixup(struct ast_channel *old, struct ast_channel *new);
static int en75xx_digit_end(struct ast_channel *ast, char digit,
			    unsigned int duration);

static struct ast_channel_tech en75xx_tech = {
	.type = "EN75XX",
	.description = "EcoNet/Airoha EN75xx FXS",
	.requester = en75xx_request,
	.call = en75xx_call,
	.hangup = en75xx_hangup,
	.answer = en75xx_answer,
	.read = en75xx_read,
	.write = en75xx_write,
	.exception = en75xx_exception,
	.indicate = en75xx_indicate,
	.fixup = en75xx_fixup,
	.send_digit_end = en75xx_digit_end,
};

/*
 * Tones come from the channel's tone zone (indications.conf), so a
 * Brazilian zone gets Brazilian dial tone without touching this file.
 *
 * vol is passed straight through to ast_playtones_start() (0 -> its
 * own default, -8dBm full scale). Dial tone specifically is played
 * quieter than that (see ss_thread()) because it reflects across the
 * 2-wire hybrid into the mic RX stream for as long as it plays, and
 * at full scale that reflection is loud enough to defeat the Goertzel
 * DTMF detector on the first digit.
 */
static int play_tone_vol(struct ast_channel *chan, const char *name, int vol)
{
	struct ast_tone_zone_sound *ts;
	int res;

	ts = ast_get_indication_tone(ast_channel_zone(chan), name);
	if (!ts) {
		ast_log(LOG_WARNING, "no '%s' tone in the current zone\n", name);
		return -1;
	}

	/*
	 * When vol <= 0, Asterisk defaults to 7219 (-8 dBm).
	 * Calibrate unspecified call progress tones (busy, congestion) to a comfortable 5000 (-16 dBFS).
	 */
	if (vol <= 0)
		vol = 5000;

	ast_verb(2, "%s: play_tone '%s' [zone=%s, data='%s', vol=%d]\n",
		 ast_channel_name(chan), name,
		 ast_channel_zone(chan) ? ast_channel_zone(chan)->country : "default",
		 ts->data, vol);

	res = ast_playtones_start(chan, vol, ts->data, 0);
	ts = ast_tone_zone_sound_unref(ts);
	return res;
}

static int play_tone(struct ast_channel *chan, const char *name)
{
	struct en75xx_pvt *p = ast_channel_tech_pvt(chan);
	int vol = (p && p->callprogress_volume > 0) ? p->callprogress_volume : 800;
	return play_tone_vol(chan, name, vol);
}

/*
 * Dial tone. When the SLIC can play it from its own oscillators
 * (EN75XX_VOICE_CAP_TONE), that beats synthesising it here: a
 * continuous software tone has Asterisk building and writing a 10 ms
 * frame every 10 ms for as long as the handset sits off-hook and
 * undialled, which on this soft-float MIPS part was enough to starve
 * its own canary thread. The tone reaches the line the same way either
 * way, so it still reflects across the 2-wire hybrid and the notch in
 * en75xx_read() is still what makes the first digit detectable.
 */
static void start_dialtone(struct en75xx_pvt *p, struct ast_channel *chan)
{
	struct en75xx_voice_tone tone = {
		.freq1_hz = p->dialtone_freq1 > 0 ? p->dialtone_freq1 : 0,
		.freq2_hz = p->dialtone_freq2 > 0 ? p->dialtone_freq2 : 0,
		.level_dbm = p->dialtone_level < 0 ? p->dialtone_level : 0,
	};

	if (!p->play_dialtone) {
		ast_verb(2, "%s: play_dialtone disabled, line silent\n",
			 p->device);
		return;
	}

	if (p->hwtone && (p->caps & EN75XX_VOICE_CAP_TONE) &&
	    p->dialtone_freq1 > 0) {
		if (!ioctl(p->fd, EN75XX_VOICE_SET_TONE, &tone)) {
			p->tone_playing = 1;
			ast_verb(2, "%s: SLIC dial tone %u+%u Hz at %d dBm\n",
				 p->device, tone.freq1_hz, tone.freq2_hz,
				 tone.level_dbm);
			return;
		}
		ast_log(LOG_WARNING,
			"%s: SLIC dial tone failed (%s), using software\n",
			p->device, strerror(errno));
	}

	if (play_tone_vol(chan, "dial",
			  p->dialtone_volume > 0 ? p->dialtone_volume : 3500))
		ast_log(LOG_WARNING, "%s: no dial tone\n", p->device);
}

/*
 * Automatic level control tracks speech, and dial tone and DTMF are not
 * speech. Left running while digits are collected, the loop adapts down
 * against the continuous dial tone reflecting through the 2-wire hybrid
 * and buries the digits that follow. The vendor gates its own autogain
 * the same way -- it carries a per-line enable flag and is switched off
 * during fax negotiation. Suspend it for the switch, restore it once
 * there is a real conversation to level.
 */
static void line_set_alc(struct en75xx_pvt *p, int enable)
{
	uint32_t val = !!enable;

	if (ioctl(p->fd, EN75XX_VOICE_SET_ALC, &val) && errno != EOPNOTSUPP)
		ast_debug(2, "%s: SET_ALC %u failed: %s\n", p->device, val,
			  strerror(errno));
}

static void stop_dialtone(struct en75xx_pvt *p, struct ast_channel *chan)
{
	if (p->tone_playing) {
		struct en75xx_voice_tone silence = { 0 };

		ioctl(p->fd, EN75XX_VOICE_SET_TONE, &silence);
		p->tone_playing = 0;
	}
	if (chan)
		ast_playtones_stop(chan);
}

/* ------------------------------------------------------------------ */
/* Line primitives                                                     */
/* ------------------------------------------------------------------ */

static int line_set_ring(struct en75xx_pvt *p, int on)
{
	struct en75xx_voice_ring ring = {
		.enable = !!on,
		/*
		 * Zero means "use the driver's cadence". Do not try to
		 * drive the cadence from here: the ring voltage corrupts
		 * the hook bit, and the kernel is what knows when the
		 * silent gaps are.
		 */
		.cadence_on_ms = 0,
		.cadence_off_ms = 0,
	};

	if (ioctl(p->fd, EN75XX_VOICE_SET_RING, &ring)) {
		ast_log(LOG_WARNING, "%s: ring %s failed: %s\n",
			p->device, on ? "on" : "off", strerror(errno));
		return -1;
	}
	p->ringing = !!on;
	return 0;
}

static int line_set_linefeed(struct en75xx_pvt *p, uint32_t lf)
{
	if (ioctl(p->fd, EN75XX_VOICE_SET_LINEFEED, &lf)) {
		ast_log(LOG_WARNING, "%s: linefeed %u failed: %s\n",
			p->device, lf, strerror(errno));
		return -1;
	}
	return 0;
}

static int line_get_state(struct en75xx_pvt *p,
			  struct en75xx_voice_line_state *st)
{
	if (ioctl(p->fd, EN75XX_VOICE_GET_STATE, st)) {
		ast_log(LOG_WARNING, "%s: GET_STATE failed: %s\n",
			p->device, strerror(errno));
		return -1;
	}
	return 0;
}

static void line_flush(struct en75xx_pvt *p)
{
	ioctl(p->fd, EN75XX_VOICE_FLUSH);
	p->partial_len = 0;
}

/* ------------------------------------------------------------------ */
/* Channel allocation                                                  */
/* ------------------------------------------------------------------ */

static struct ast_channel *en75xx_new(struct en75xx_pvt *p, int state,
				      const char *exten,
				      const struct ast_assigned_ids *ids,
				      const struct ast_channel *requestor)
{
	struct ast_channel *chan;
	struct ast_format_cap *caps;

	caps = ast_format_cap_alloc(AST_FORMAT_CAP_FLAG_DEFAULT);
	if (!caps)
		return NULL;

	chan = ast_channel_alloc(1, state, p->cid_num, p->cid_name, "",
				 exten, p->context, ids, requestor, 0,
				 "EN75XX/%d", p->index);
	if (!chan) {
		ao2_ref(caps, -1);
		return NULL;
	}

	ast_format_cap_append(caps, ast_format_slin, 0);
	ast_channel_nativeformats_set(chan, caps);
	ao2_ref(caps, -1);

	ast_channel_set_writeformat(chan, ast_format_slin);
	ast_channel_set_rawwriteformat(chan, ast_format_slin);
	ast_channel_set_readformat(chan, ast_format_slin);
	ast_channel_set_rawreadformat(chan, ast_format_slin);

	ast_channel_tech_set(chan, &en75xx_tech);
	ast_channel_tech_pvt_set(chan, p);
	ast_channel_set_fd(chan, 0, p->fd);

	if (!ast_strlen_zero(p->language))
		ast_channel_language_set(chan, p->language);
	if (!ast_strlen_zero(p->mohinterpret))
		ast_channel_musicclass_set(chan, p->mohinterpret);

	p->owner = chan;
	p->inuse = 1;

	ast_channel_unlock(chan);

	ast_module_ref(ast_module_info->self);
	return chan;
}

/* ------------------------------------------------------------------ */
/* Simple switch: dial tone, digit collection, PBX launch              */
/* ------------------------------------------------------------------ */

static void *ss_thread(void *data)
{
	struct ast_channel *chan = data;
	struct en75xx_pvt *p = ast_channel_tech_pvt(chan);
	char exten[AST_MAX_EXTENSION] = "";
	int len = 0, timeout = FIRST_DIGIT_TIMEOUT;
	int res;

	ast_verb(3, "%s: off-hook, collecting digits in context '%s'\n",
		 p->device, p->context);

	line_set_alc(p, 0);
	start_dialtone(p, chan);

	for (;;) {
		char digit;

		digit = ast_waitfordigit(chan, timeout);
		if (digit < 0) {		/* hangup or error */
			stop_dialtone(p, chan);
			line_set_alc(p, 1);
			ast_hangup(chan);
			return NULL;
		}
		if (digit == 0) {		/* timeout */
			if (!len)
				ast_verb(3, "%s: no digits dialled\n",
					 p->device);
			break;
		}

		if (len == 0)
			stop_dialtone(p, chan);

		if (len < (int)sizeof(exten) - 1) {
			exten[len++] = digit;
			exten[len] = '\0';
		}

		if (ast_exists_extension(chan, p->context, exten, 1, p->cid_num)) {
			/*
			 * Keep waiting briefly if a longer extension could
			 * still match, otherwise "1" would fire before "100".
			 */
			if (ast_matchmore_extension(chan, p->context, exten, 1,
						    p->cid_num)) {
				timeout = MATCH_DIGIT_TIMEOUT;
				continue;
			}
			break;
		}

		if (!ast_canmatch_extension(chan, p->context, exten, 1,
					    p->cid_num)) {
			ast_verb(3, "%s: '%s' matches nothing in '%s'\n",
				 p->device, exten, p->context);
			ast_indicate(chan, AST_CONTROL_CONGESTION);
			ast_safe_sleep(chan, 3000);
			line_set_alc(p, 1);
			ast_hangup(chan);
			return NULL;
		}

		timeout = INTER_DIGIT_TIMEOUT;
	}

	stop_dialtone(p, chan);

	if (!len || !ast_exists_extension(chan, p->context, exten, 1,
					  p->cid_num)) {
		ast_indicate(chan, AST_CONTROL_CONGESTION);
		ast_safe_sleep(chan, 3000);
		ast_hangup(chan);
		return NULL;
	}

	ast_channel_exten_set(chan, exten);
	ast_setstate(chan, AST_STATE_RING);
	line_set_alc(p, 1);
	line_flush(p);

	res = ast_pbx_run(chan);
	if (res)
		ast_log(LOG_WARNING, "%s: PBX exited with %d\n", p->device, res);

	return NULL;
}

static void start_outgoing_call(struct en75xx_pvt *p)
{
	struct ast_channel *chan;
	pthread_t tid;

	line_set_linefeed(p, EN75XX_VOICE_LINEFEED_ACTIVE);
	line_flush(p);

	chan = en75xx_new(p, AST_STATE_DOWN,
			  p->immediate ? p->exten : "s", NULL, NULL);
	if (!chan) {
		ast_log(LOG_WARNING, "%s: cannot allocate channel\n",
			p->device);
		return;
	}

	p->state = EN75XX_DIALING;
	p->notch_x1 = p->notch_x2 = 0;
	p->notch_y1 = p->notch_y2 = 0;

	if (p->immediate) {
		/* skip the switch entirely and go straight to the dialplan */
		ast_setstate(chan, AST_STATE_RING);
		if (ast_pbx_start(chan)) {
			ast_log(LOG_WARNING, "%s: ast_pbx_start failed\n",
				p->device);
			ast_hangup(chan);
		}
		return;
	}

	if (ast_pthread_create_detached(&tid, NULL, ss_thread, chan)) {
		ast_log(LOG_WARNING, "%s: cannot start switch thread\n",
			p->device);
		ast_hangup(chan);
	}
}

/* ------------------------------------------------------------------ */
/* Hook handling                                                       */
/* ------------------------------------------------------------------ */

static void handle_hook_change(struct en75xx_pvt *p, int offhook)
{
	ast_mutex_lock(&p->lock);

	if (offhook == p->offhook) {
		ast_mutex_unlock(&p->lock);
		return;
	}
	p->offhook = offhook;

	ast_debug(1, "%s: %s\n", p->device, offhook ? "off-hook" : "on-hook");

	if (offhook) {
		switch (p->state) {
		case EN75XX_RINGING:
			/* answered */
			line_set_ring(p, 0);
			p->state = EN75XX_UP;
			if (p->owner) {
				ast_queue_control(p->owner,
						  AST_CONTROL_ANSWER);
				ast_setstate(p->owner, AST_STATE_UP);
			}
			break;
		case EN75XX_IDLE:
			ast_mutex_unlock(&p->lock);
			start_outgoing_call(p);
			return;
		default:
			break;
		}
	} else {
		if (p->ringing)
			line_set_ring(p, 0);
		if (p->owner) {
			ast_queue_hangup(p->owner);
		} else {
			p->state = EN75XX_IDLE;
			line_set_linefeed(p, EN75XX_VOICE_LINEFEED_STANDBY);
		}
	}

	ast_mutex_unlock(&p->lock);
}

/* ------------------------------------------------------------------ */
/* Monitor thread                                                      */
/* ------------------------------------------------------------------ */

static void *monitor_loop(void *unused)
{
	struct pollfd pfd[EN75XX_MAX_LINES];

	while (!monitor_stop) {
		int i, res;

		for (i = 0; i < num_lines; i++) {
			pfd[i].fd = lines[i].fd;
			pfd[i].events = POLLPRI;
			pfd[i].revents = 0;
		}

		res = poll(pfd, num_lines, 500);
		if (res < 0) {
			if (errno == EINTR)
				continue;
			ast_log(LOG_WARNING, "monitor poll: %s\n",
				strerror(errno));
			usleep(100000);
			continue;
		}
		if (!res)
			continue;

		for (i = 0; i < num_lines; i++) {
			struct en75xx_voice_line_state st;

			if (!(pfd[i].revents & POLLPRI))
				continue;
			/*
			 * GET_STATE both reads the hook and acknowledges
			 * the event, so POLLPRI clears here.
			 */
			if (line_get_state(&lines[i], &st))
				continue;
			handle_hook_change(&lines[i],
					   st.hook == EN75XX_VOICE_OFFHOOK);
		}
	}

	return NULL;
}

static int monitor_start(void)
{
	int res = 0;

	ast_mutex_lock(&monlock);
	if (monitor_thread == AST_PTHREADT_NULL) {
		monitor_stop = 0;
		if (ast_pthread_create_background(&monitor_thread, NULL,
						  monitor_loop, NULL)) {
			ast_log(LOG_ERROR, "cannot start monitor thread\n");
			res = -1;
		}
	}
	ast_mutex_unlock(&monlock);
	return res;
}

static void monitor_shutdown(void)
{
	ast_mutex_lock(&monlock);
	monitor_stop = 1;
	if (monitor_thread != AST_PTHREADT_NULL) {
		pthread_join(monitor_thread, NULL);
		monitor_thread = AST_PTHREADT_NULL;
	}
	ast_mutex_unlock(&monlock);
}

/* ------------------------------------------------------------------ */
/* Channel technology callbacks                                        */
/* ------------------------------------------------------------------ */

static struct ast_channel *en75xx_request(const char *type,
	struct ast_format_cap *cap, const struct ast_assigned_ids *assignedids,
	const struct ast_channel *requestor, const char *data, int *cause)
{
	struct en75xx_pvt *p = NULL;
	struct ast_channel *chan;
	int i, want;

	if (ast_strlen_zero(data)) {
		ast_log(LOG_WARNING, "EN75XX requires a line number\n");
		*cause = AST_CAUSE_CHANNEL_UNACCEPTABLE;
		return NULL;
	}
	want = atoi(data);

	for (i = 0; i < num_lines; i++) {
		if (lines[i].index != want)
			continue;
		ast_mutex_lock(&lines[i].lock);
		if (lines[i].inuse) {
			ast_mutex_unlock(&lines[i].lock);
			*cause = AST_CAUSE_BUSY;
			return NULL;
		}
		p = &lines[i];
		break;
	}

	if (!p) {
		ast_log(LOG_WARNING, "EN75XX line %d not configured\n", want);
		*cause = AST_CAUSE_CHANNEL_UNACCEPTABLE;
		return NULL;
	}

	if (p->offhook) {
		/* the handset is already lifted */
		ast_mutex_unlock(&p->lock);
		*cause = AST_CAUSE_BUSY;
		return NULL;
	}

	chan = en75xx_new(p, AST_STATE_DOWN, "", assignedids, requestor);
	ast_mutex_unlock(&p->lock);

	if (!chan)
		*cause = AST_CAUSE_SWITCH_CONGESTION;

	return chan;
}

static int en75xx_call(struct ast_channel *ast, const char *dest, int timeout)
{
	struct en75xx_pvt *p = ast_channel_tech_pvt(ast);

	ast_mutex_lock(&p->lock);

	if (p->offhook) {
		ast_mutex_unlock(&p->lock);
		return -1;
	}

	line_flush(p);
	if (line_set_ring(p, 1)) {
		ast_mutex_unlock(&p->lock);
		return -1;
	}
	p->state = EN75XX_RINGING;

	ast_mutex_unlock(&p->lock);

	ast_setstate(ast, AST_STATE_RINGING);
	ast_queue_control(ast, AST_CONTROL_RINGING);
	return 0;
}

static int en75xx_answer(struct ast_channel *ast)
{
	struct en75xx_pvt *p = ast_channel_tech_pvt(ast);

	ast_mutex_lock(&p->lock);
	if (p->ringing)
		line_set_ring(p, 0);
	line_set_linefeed(p, EN75XX_VOICE_LINEFEED_ACTIVE);
	line_flush(p);
	p->state = EN75XX_UP;
	ast_mutex_unlock(&p->lock);

	ast_setstate(ast, AST_STATE_UP);
	return 0;
}

static int en75xx_hangup(struct ast_channel *ast)
{
	struct en75xx_pvt *p = ast_channel_tech_pvt(ast);

	if (!p)
		return 0;

	ast_mutex_lock(&p->lock);

	if (p->ringing)
		line_set_ring(p, 0);
	/* an oscillator left running would sing on into the next call */
	stop_dialtone(p, NULL);
	line_set_alc(p, 1);
	line_flush(p);

	/*
	 * If the handset is still lifted the subscriber has not hung up
	 * yet: hold the feed so they get silence rather than a dead
	 * line, and the next on-hook returns us to standby.
	 */
	line_set_linefeed(p, p->offhook ? EN75XX_VOICE_LINEFEED_ACTIVE
					: EN75XX_VOICE_LINEFEED_STANDBY);

	p->owner = NULL;
	p->inuse = 0;
	p->state = p->offhook ? EN75XX_DIALING : EN75XX_IDLE;

	ast_mutex_unlock(&p->lock);

	ast_channel_tech_pvt_set(ast, NULL);
	ast_module_unref(ast_module_info->self);

	ast_verb(3, "%s: hung up\n", p->device);
	return 0;
}

static struct ast_frame *en75xx_read(struct ast_channel *ast)
{
	struct en75xx_pvt *p = ast_channel_tech_pvt(ast);
	struct ast_frame *f = &p->frame;
	unsigned char *dst;
	ssize_t n;

	memset(f, 0, sizeof(*f));
	f->frametype = AST_FRAME_NULL;
	f->src = "EN75XX";

	/*
	 * The device hands out 10 ms frames but a read can still come
	 * back short, so a frame is assembled across reads rather than
	 * emitting ragged audio.
	 */
	dst = p->partial + p->partial_len;
	n = read(p->fd, dst, EN75XX_FRAME_BYTES - p->partial_len);
	if (n < 0) {
		if (errno == EAGAIN || errno == EINTR)
			return &ast_null_frame;
		ast_log(LOG_WARNING, "%s: read: %s\n", p->device,
			strerror(errno));
		return NULL;
	}
	if (n == 0)
		return &ast_null_frame;

	p->partial_len += n;
	if (p->partial_len < EN75XX_FRAME_BYTES)
		return &ast_null_frame;

	memcpy(p->buf + AST_FRIENDLY_OFFSET, p->partial, EN75XX_FRAME_BYTES);
	p->partial_len = 0;

	f->frametype = AST_FRAME_VOICE;
	f->subclass.format = ast_format_slin;
	f->datalen = EN75XX_FRAME_BYTES;
	f->samples = EN75XX_FRAME_SAMPLES;
	f->offset = AST_FRIENDLY_OFFSET;
	f->data.ptr = p->buf + AST_FRIENDLY_OFFSET;
	f->mallocd = 0;

	/*
	 * Software DTMF detection. The SLICs here do not report digits
	 * out of band, so the tones have to be pulled out of the audio;
	 * ast_dsp_process rewrites the frame into an AST_FRAME_DTMF
	 * when it finds one, which is what ast_waitfordigit consumes.
	 */
	if (p->dsp && !p->hwdtmf) {
		/*
		 * When off-hook and collecting digits (EN75XX_DIALING), Asterisk plays
		 * dial tone (400 Hz). The tone reflects across the 2-wire hybrid back into
		 * RX. Asterisk's Goertzel DTMF detector requires
		 * (E_row + E_col) > 42.0 * E_total. The 400 Hz reflection inflates
		 * E_total, preventing detection of the first dialed digit.
		 * Passing the audio to ast_dsp_process through a sharp 400 Hz biquad notch
		 * filter removes the reflected dial tone and restores DTMF detection.
		 */
		if (p->state == EN75XX_DIALING) {
			int16_t *samp = (int16_t *)(p->buf + AST_FRIENDLY_OFFSET);
			/* Q14 biquad coefficients for 400 Hz notch at 8000 Hz, Q=3.0 */
			const int32_t b0 = 15582, b1 = -29638, b2 = 15582;
			const int32_t a1 = -29638, a2 = 14779;
			int i;

			for (i = 0; i < EN75XX_FRAME_SAMPLES; i++) {
				int32_t x0 = samp[i];
				int32_t acc = b0 * x0 + b1 * (int32_t)p->notch_x1 + b2 * (int32_t)p->notch_x2
					    - a1 * (int32_t)p->notch_y1 - a2 * (int32_t)p->notch_y2;
				int32_t y0 = acc >> 14;
				if (y0 > 32767) y0 = 32767;
				else if (y0 < -32768) y0 = -32768;

				p->notch_x2 = p->notch_x1;
				p->notch_x1 = x0;
				p->notch_y2 = p->notch_y1;
				p->notch_y1 = y0;
				samp[i] = (int16_t)y0;
			}
		}

		f = ast_dsp_process(ast, p->dsp, f);

		if (f && (f->frametype == AST_FRAME_DTMF_END || f->frametype == AST_FRAME_DTMF_BEGIN)) {
			ast_verb(2, "%s: DTMF %s '%c'\n", p->device,
				 (f->frametype == AST_FRAME_DTMF_BEGIN) ? "begin" : "end",
				 f->subclass.integer);
		}
	}

	return f;
}

static struct ast_frame *en75xx_exception(struct ast_channel *ast)
{
	/*
	 * Exception flag is raised on POLLPRI (hook change event).
	 * Do not consume the event here via line_get_state(), as monitor_loop
	 * owns hook state transitions. Simply return null frame to suppress
	 * Asterisk core's "no exception handler" warning.
	 */
	return &ast_null_frame;
}

static int en75xx_write(struct ast_channel *ast, struct ast_frame *frame)
{
	struct en75xx_pvt *p = ast_channel_tech_pvt(ast);
	const unsigned char *data;
	size_t left;

	if (frame->frametype != AST_FRAME_VOICE)
		return 0;

	if (ast_format_cmp(frame->subclass.format, ast_format_slin) !=
	    AST_FORMAT_CMP_EQUAL) {
		ast_log(LOG_WARNING, "%s: cannot write format %s\n",
			p->device, ast_format_get_name(frame->subclass.format));
		return -1;
	}

	if (!frame->datalen)
		return 0;

	data = frame->data.ptr;
	left = frame->datalen;

	while (left) {
		ssize_t n = write(p->fd, data, left);

		if (n < 0) {
			if (errno == EAGAIN) {
				/* TX fifo full: drop, do not stall the bridge */
				ast_debug(3, "%s: tx overflow, %zu dropped\n",
					  p->device, left);
				return 0;
			}
			if (errno == EINTR)
				continue;
			ast_log(LOG_WARNING, "%s: write: %s\n", p->device,
				strerror(errno));
			return -1;
		}
		data += n;
		left -= n;
	}

	return 0;
}

static int en75xx_indicate(struct ast_channel *ast, int condition,
			   const void *data, size_t datalen)
{
	struct en75xx_pvt *p = ast_channel_tech_pvt(ast);
	int res = 0;

	switch (condition) {
	case AST_CONTROL_BUSY:
		res = play_tone(ast, "busy");
		break;
	case AST_CONTROL_CONGESTION:
		res = play_tone(ast, "congestion");
		break;
	case AST_CONTROL_RINGING:
		res = play_tone(ast, "ring");
		break;
	case AST_CONTROL_PROGRESS:
	case AST_CONTROL_PROCEEDING:
	case AST_CONTROL_VIDUPDATE:
	case AST_CONTROL_SRCUPDATE:
	case AST_CONTROL_SRCCHANGE:
		break;
	case AST_CONTROL_HOLD:
		ast_moh_start(ast, data, p->mohinterpret);
		break;
	case AST_CONTROL_UNHOLD:
		ast_moh_stop(ast);
		break;
	case -1:
		ast_playtones_stop(ast);
		break;
	default:
		ast_debug(2, "%s: unhandled indication %d\n", p->device,
			  condition);
		res = -1;
		break;
	}

	return res;
}

static int en75xx_digit_end(struct ast_channel *ast, char digit,
			    unsigned int duration)
{
	/*
	 * Digits toward the subscriber are generated as audio by
	 * Asterisk's tone code; there is no out-of-band path on the
	 * SLIC, so nothing to do here.
	 */
	return 0;
}

static int en75xx_fixup(struct ast_channel *old, struct ast_channel *new)
{
	struct en75xx_pvt *p = ast_channel_tech_pvt(new);

	if (!p)
		return -1;

	ast_mutex_lock(&p->lock);
	if (p->owner == old)
		p->owner = new;
	ast_mutex_unlock(&p->lock);
	return 0;
}

/* ------------------------------------------------------------------ */
/* Configuration                                                       */
/* ------------------------------------------------------------------ */

static void line_close(struct en75xx_pvt *p)
{
	if (p->dsp) {
		ast_dsp_free(p->dsp);
		p->dsp = NULL;
	}
	if (p->fd >= 0) {
		close(p->fd);
		p->fd = -1;
	}
}

static int line_open(struct en75xx_pvt *p)
{
	struct en75xx_voice_info info;

	p->fd = open(p->device, O_RDWR | O_NONBLOCK);
	if (p->fd < 0) {
		ast_log(LOG_ERROR, "cannot open %s: %s\n", p->device,
			strerror(errno));
		return -1;
	}

	if (ioctl(p->fd, EN75XX_VOICE_GET_INFO, &info)) {
		ast_log(LOG_ERROR, "%s: GET_INFO failed: %s\n", p->device,
			strerror(errno));
		goto err;
	}

	if (info.abi_version != EN75XX_VOICE_ABI_VERSION) {
		ast_log(LOG_ERROR,
			"%s: ABI %u, this module was built for %u\n",
			p->device, info.abi_version,
			EN75XX_VOICE_ABI_VERSION);
		goto err;
	}
	if (info.sample_rate != EN75XX_VOICE_RATE ||
	    info.sample_bits != EN75XX_VOICE_SAMPLE_BITS) {
		ast_log(LOG_ERROR, "%s: unexpected format %u Hz / %u bit\n",
			p->device, info.sample_rate, info.sample_bits);
		goto err;
	}
	p->caps = info.capabilities;

	ast_copy_string(p->slic, info.slic, sizeof(p->slic));

	if (!p->hwdtmf) {
		int digitmode = DSP_DIGITMODE_DTMF;
		if (p->relaxdtmf)
			digitmode |= DSP_DIGITMODE_RELAXDTMF;
		p->dsp = ast_dsp_new();
		if (!p->dsp) {
			ast_log(LOG_ERROR, "%s: cannot allocate DSP\n",
				p->device);
			goto err;
		}
		ast_dsp_set_features(p->dsp, DSP_FEATURE_DIGIT_DETECT);
		ast_dsp_set_digitmode(p->dsp, digitmode);
	}

	line_set_linefeed(p, EN75XX_VOICE_LINEFEED_STANDBY);

	ast_verb(2, "EN75XX line %d on %s (SLIC %s)\n", p->index, p->device,
		 p->slic);
	return 0;

err:
	line_close(p);
	return -1;
}

static int load_config(int reload)
{
	struct ast_config *cfg;
	struct ast_flags flags = { reload ? CONFIG_FLAG_FILEUNCHANGED : 0 };
	struct ast_variable *v;
	char *cat;

	cfg = ast_config_load(CONFIG_FILE, flags);
	if (cfg == CONFIG_STATUS_FILEUNCHANGED)
		return 0;
	if (!cfg || cfg == CONFIG_STATUS_FILEINVALID) {
		ast_log(LOG_ERROR, "cannot load " CONFIG_FILE "\n");
		return -1;
	}

	num_lines = 0;

	for (cat = ast_category_browse(cfg, NULL); cat;
	     cat = ast_category_browse(cfg, cat)) {
		struct en75xx_pvt *p;

		if (!strcasecmp(cat, "general"))
			continue;
		if (strncasecmp(cat, "line", 4)) {
			ast_log(LOG_WARNING, "ignoring section [%s]\n", cat);
			continue;
		}
		if (num_lines >= EN75XX_MAX_LINES) {
			ast_log(LOG_WARNING, "too many lines, [%s] ignored\n",
				cat);
			continue;
		}

		p = &lines[num_lines];
		memset(p, 0, sizeof(*p));
		ast_mutex_init(&p->lock);
		p->fd = -1;
		p->index = atoi(cat + 4);
		snprintf(p->device, sizeof(p->device), "/dev/en75xx-fxs%d",
			 p->index);
		ast_copy_string(p->context, "default", sizeof(p->context));
		ast_copy_string(p->exten, "s", sizeof(p->exten));
		p->relaxdtmf = 1;
		p->play_dialtone = 1;
		p->hwtone = 1;
		p->dialtone_volume = 3500;
		p->callprogress_volume = 5000;
		/*
		 * 400 Hz continuous is the Indian dial tone, matching the
		 * country=in zone the init script writes into
		 * indications.conf; -18 dBm is the level Silicon Labs' own
		 * tone presets use.
		 */
		p->dialtone_freq1 = 400;
		p->dialtone_freq2 = 0;
		p->dialtone_level = -18;

		for (v = ast_variable_browse(cfg, cat); v; v = v->next) {
			if (!strcasecmp(v->name, "device"))
				ast_copy_string(p->device, v->value,
						sizeof(p->device));
			else if (!strcasecmp(v->name, "context"))
				ast_copy_string(p->context, v->value,
						sizeof(p->context));
			else if (!strcasecmp(v->name, "extension"))
				ast_copy_string(p->exten, v->value,
						sizeof(p->exten));
			else if (!strcasecmp(v->name, "callerid"))
				ast_callerid_split(v->value, p->cid_name,
						   sizeof(p->cid_name),
						   p->cid_num,
						   sizeof(p->cid_num));
			else if (!strcasecmp(v->name, "language"))
				ast_copy_string(p->language, v->value,
						sizeof(p->language));
			else if (!strcasecmp(v->name, "mohinterpret"))
				ast_copy_string(p->mohinterpret, v->value,
						sizeof(p->mohinterpret));
			else if (!strcasecmp(v->name, "immediate"))
				p->immediate = ast_true(v->value);
			else if (!strcasecmp(v->name, "hardware_dtmf"))
				p->hwdtmf = ast_true(v->value);
			else if (!strcasecmp(v->name, "relaxdtmf"))
				p->relaxdtmf = ast_true(v->value);
			else if (!strcasecmp(v->name, "play_dialtone"))
				p->play_dialtone = ast_true(v->value);
			else if (!strcasecmp(v->name, "dialtone_volume"))
				p->dialtone_volume = atoi(v->value);
			else if (!strcasecmp(v->name, "callprogress_volume"))
				p->callprogress_volume = atoi(v->value);
			else if (!strcasecmp(v->name, "hardware_tones"))
				p->hwtone = ast_true(v->value);
			else if (!strcasecmp(v->name, "dialtone_freq1"))
				p->dialtone_freq1 = atoi(v->value);
			else if (!strcasecmp(v->name, "dialtone_freq2"))
				p->dialtone_freq2 = atoi(v->value);
			else if (!strcasecmp(v->name, "dialtone_level"))
				p->dialtone_level = atoi(v->value);
			else
				ast_log(LOG_WARNING,
					"[%s]: unknown option '%s'\n", cat,
					v->name);
		}

		if (line_open(p)) {
			ast_mutex_destroy(&p->lock);
			continue;
		}

		num_lines++;
	}

	ast_config_destroy(cfg);

	if (!num_lines) {
		ast_log(LOG_ERROR, "no usable FXS lines\n");
		return -1;
	}

	return 0;
}

/* ------------------------------------------------------------------ */
/* CLI                                                                 */
/* ------------------------------------------------------------------ */

static const char *state_name(enum en75xx_state s)
{
	switch (s) {
	case EN75XX_IDLE:	return "idle";
	case EN75XX_DIALING:	return "dialing";
	case EN75XX_RINGING:	return "ringing";
	case EN75XX_UP:		return "up";
	}
	return "?";
}

static char *cli_show_lines(struct ast_cli_entry *e, int cmd,
			    struct ast_cli_args *a)
{
	int i;

	switch (cmd) {
	case CLI_INIT:
		e->command = "en75xx show lines";
		e->usage = "Usage: en75xx show lines\n"
			   "       List the configured FXS lines.\n";
		return NULL;
	case CLI_GENERATE:
		return NULL;
	}

	ast_cli(a->fd, "%-6s %-22s %-10s %-8s %-8s %s\n",
		"Line", "Device", "SLIC", "Hook", "State", "Context");

	for (i = 0; i < num_lines; i++) {
		struct en75xx_pvt *p = &lines[i];
		struct en75xx_voice_stats st = { 0 };

		ioctl(p->fd, EN75XX_VOICE_GET_STATS, &st);

		ast_cli(a->fd, "%-6d %-22s %-10s %-8s %-8s %s\n",
			p->index, p->device, p->slic,
			p->offhook ? "off" : "on",
			state_name(p->state), p->context);
		ast_cli(a->fd,
			"       rx %llu B  tx %llu B  overruns %llu  "
			"underruns %llu  dma errors %llu\n",
			(unsigned long long)st.rx_bytes,
			(unsigned long long)st.tx_bytes,
			(unsigned long long)st.rx_overruns,
			(unsigned long long)st.tx_underruns,
			(unsigned long long)st.dma_errors);
	}

	return CLI_SUCCESS;
}

static char *cli_ring(struct ast_cli_entry *e, int cmd,
		      struct ast_cli_args *a)
{
	int i, want, on;

	switch (cmd) {
	case CLI_INIT:
		e->command = "en75xx ring";
		e->usage = "Usage: en75xx ring <line> on|off\n"
			   "       Ring a line directly, for bench testing.\n";
		return NULL;
	case CLI_GENERATE:
		return NULL;
	}

	if (a->argc != 4)
		return CLI_SHOWUSAGE;

	want = atoi(a->argv[2]);
	on = !strcasecmp(a->argv[3], "on");

	for (i = 0; i < num_lines; i++) {
		if (lines[i].index != want)
			continue;
		ast_mutex_lock(&lines[i].lock);
		line_set_ring(&lines[i], on);
		ast_mutex_unlock(&lines[i].lock);
		ast_cli(a->fd, "line %d ring %s\n", want, on ? "on" : "off");
		return CLI_SUCCESS;
	}

	ast_cli(a->fd, "no such line: %d\n", want);
	return CLI_FAILURE;
}

static struct ast_cli_entry cli_en75xx[] = {
	AST_CLI_DEFINE(cli_show_lines, "List EN75XX FXS lines"),
	AST_CLI_DEFINE(cli_ring, "Ring an EN75XX FXS line"),
};

/* ------------------------------------------------------------------ */
/* Module                                                              */
/* ------------------------------------------------------------------ */

static int unload_module(void)
{
	int i;

	monitor_shutdown();

	ast_channel_unregister(&en75xx_tech);
	ast_cli_unregister_multiple(cli_en75xx, ARRAY_LEN(cli_en75xx));

	for (i = 0; i < num_lines; i++) {
		struct en75xx_pvt *p = &lines[i];

		ast_mutex_lock(&p->lock);
		if (p->owner)
			ast_softhangup(p->owner, AST_SOFTHANGUP_APPUNLOAD);
		if (p->ringing)
			line_set_ring(p, 0);
		line_set_linefeed(p, EN75XX_VOICE_LINEFEED_OPEN);
		line_close(p);
		ast_mutex_unlock(&p->lock);
		ast_mutex_destroy(&p->lock);
	}
	num_lines = 0;

	ao2_cleanup(en75xx_tech_cap);
	en75xx_tech_cap = NULL;
	en75xx_tech.capabilities = NULL;

	return 0;
}

static int load_module(void)
{
	en75xx_tech_cap = ast_format_cap_alloc(AST_FORMAT_CAP_FLAG_DEFAULT);
	if (!en75xx_tech_cap)
		return AST_MODULE_LOAD_DECLINE;

	ast_format_cap_append(en75xx_tech_cap, ast_format_slin, 0);
	en75xx_tech.capabilities = en75xx_tech_cap;

	if (load_config(0)) {
		ao2_cleanup(en75xx_tech_cap);
		en75xx_tech_cap = NULL;
		return AST_MODULE_LOAD_DECLINE;
	}

	if (ast_channel_register(&en75xx_tech)) {
		ast_log(LOG_ERROR, "cannot register channel type EN75XX\n");
		unload_module();
		return AST_MODULE_LOAD_DECLINE;
	}

	ast_cli_register_multiple(cli_en75xx, ARRAY_LEN(cli_en75xx));

	if (monitor_start()) {
		unload_module();
		return AST_MODULE_LOAD_DECLINE;
	}

	return AST_MODULE_LOAD_SUCCESS;
}

static int reload(void)
{
	ast_log(LOG_NOTICE, "chan_en75xx does not support reload; "
		"unload and load again\n");
	return 0;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_LOAD_ORDER,
	"EcoNet/Airoha EN75xx FXS Channel Driver",
	.support_level = AST_MODULE_SUPPORT_EXTENDED,
	.load = load_module,
	.unload = unload_module,
	.reload = reload,
	.load_pri = AST_MODPRI_CHANNEL_DRIVER,
);
