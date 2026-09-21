// SPDX-License-Identifier: GPL-2.0
#include <assert.h>
#include <stdarg.h>
#define ioctl fake_ioctl
#define main app_main
#include "../en75xx-test.c"
#undef main
#undef ioctl
static int scenario, queries, starts, stops;
int fake_ioctl(int fd, unsigned long request, ...)
{
	va_list ap;
	void *arg;
	(void)fd;
	va_start(ap, request); arg = va_arg(ap, void *); va_end(ap);
	if (request == EN75XX_VOICE_GET_STATE) {
		struct en75xx_voice_line_state *s = arg;
		memset(s, 0, sizeof(*s)); queries++;
		if (scenario == 1 || queries > 1) s->hook = EN75XX_VOICE_OFFHOOK;
		if (scenario == 3 && queries > 1) s->faults = 1;
		return 0;
	}
	if (request == EN75XX_VOICE_SET_RING) {
		struct en75xx_voice_ring *r = arg;
		if (r->enable) {
			starts++;
			assert(r->cadence_on_ms == 1000 && r->cadence_off_ms == 4000);
			if (scenario == 2) { errno = EIO; return -1; }
		} else stops++;
		return 0;
	}
	errno = ENOTTY; return -1;
}
int main(void)
{
	unsigned char frame[160]; unsigned phase = 0;
	const int expected[8] = {0,1448,2048,1448,0,-1448,-2048,-1448};
	tone_frame(frame, &phase);
	assert(phase == 0);
	for (unsigned i = 0; i < 80; i++) {
		int v = frame[2*i] | ((unsigned)frame[2*i+1] << 8);
		if (v >= 32768) v -= 65536;
		assert(v == expected[i % 8]);
	}
	for (scenario = 0; scenario < 4; scenario++) {
		int r;
		queries = starts = stops = 0;
		r = controls(3, "ring", 1);
		assert(r == (scenario ? -1 : 0));
		assert(starts == (scenario == 1 ? 0 : 1));
		assert(stops == (scenario == 1 ? 0 : 1));
	}
	puts("PASS: tone packing, on-hook gate, pickup stop, failed start cleanup, fault cleanup");
	return 0;
}
