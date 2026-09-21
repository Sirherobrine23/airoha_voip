// SPDX-License-Identifier: GPL-2.0
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include "include/uapi/linux/en75xx_voice.h"

static volatile sig_atomic_t stopped;
static void on_signal(int sig) { (void)sig; stopped = 1; }
static int64_t now_ms(void)
{
	struct timespec t;
	if (clock_gettime(CLOCK_MONOTONIC, &t)) { perror("clock_gettime"); exit(1); }
	return (int64_t)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}
static int state(int fd, struct en75xx_voice_line_state *s)
{
	if (ioctl(fd, EN75XX_VOICE_GET_STATE, s)) { perror("GET_STATE"); return -1; }
	if (s->faults) { fprintf(stderr, "SLIC faults=0x%x\n", s->faults); return -1; }
	return 0;
}
static int set_ring(int fd, unsigned enable)
{
	struct en75xx_voice_ring r = { .enable = enable,
		.cadence_on_ms = 1000, .cadence_off_ms = 4000 };
	if (ioctl(fd, EN75XX_VOICE_SET_RING, &r)) { perror("SET_RING"); return -1; }
	return 0;
}
static void stats(int fd, const struct en75xx_voice_stats *before)
{
	struct en75xx_voice_stats s;
	if (ioctl(fd, EN75XX_VOICE_GET_STATS, &s)) { perror("GET_STATS"); return; }
#define PRINT_DELTA(n) printf(#n "=%" PRIu64 " delta=%" PRIu64 "\n", \
	(uint64_t)s.n, (uint64_t)(s.n - before->n))
	PRINT_DELTA(rx_bytes); PRINT_DELTA(tx_bytes); PRINT_DELTA(rx_overruns);
	PRINT_DELTA(tx_underruns); PRINT_DELTA(hook_changes); PRINT_DELTA(dma_errors);
#undef PRINT_DELTA
}
/* Explicit little-endian encoding, independent of the build host. */
static void tone_frame(unsigned char *buf, unsigned *phase)
{
	unsigned i;
	for (i = 0; i < EN75XX_VOICE_FRAME_SAMPLES; i++) {
		int16_t sample = (int16_t)lrint(2048.0 * sin(2.0 * 3.141592653589793 * *phase / 8.0));
		uint16_t u = (uint16_t)sample;
		buf[2*i] = u & 255; buf[2*i+1] = u >> 8;
		*phase = (*phase + 1) % 8; /* 1 kHz at 8 kHz, about -24 dBFS peak */
	}
}
static int controls(int fd, const char *mode, unsigned seconds)
{
	struct en75xx_voice_line_state s;
	int last = -1, ret = 0;
	int ringing = !strcmp(mode, "ring");
	int64_t deadline = now_ms() + seconds * 1000;
	if (state(fd, &s)) return -1;
	if (ringing && s.hook != EN75XX_VOICE_ONHOOK) {
		fprintf(stderr, "Ring refused: handset must be on-hook.\n"); return -1;
	}
	if (ringing && set_ring(fd, 1)) {
		/* The start ioctl may have failed after a partial operation. */
		set_ring(fd, 0); return -1;
	}
	while (!stopped && now_ms() < deadline) {
		struct timespec pause = { .tv_nsec = 100000000 };
		if (state(fd, &s)) { ret = -1; break; }
		if ((int)s.hook != last) {
			printf("hook=%s ringing(requested)=%u linefeed(cached)=%u\n",
			       s.hook ? "off-hook" : "on-hook", s.ringing, s.linefeed);
			last = (int)s.hook;
		}
		if (ringing && s.hook == EN75XX_VOICE_OFFHOOK) {
			puts("Pickup detected; stopping ring."); break;
		}
		nanosleep(&pause, NULL);
	}
	if (ringing && set_ring(fd, 0)) ret = -1;
	return ret;
}
static int audio(int fd, const char *mode, unsigned seconds, const char *path)
{
	unsigned char tx[EN75XX_VOICE_FRAME_BYTES], rx[4096];
	struct en75xx_voice_line_state s;
	FILE *fp = NULL;
	unsigned phase = 0, peak = 0;
	size_t offset = sizeof(tx), length = sizeof(tx);
	uint64_t got = 0, sent = 0, samples = 0, nonzero = 0;
	long double square_sum = 0;
	int record = !strcmp(mode, "record"), play = !strcmp(mode, "play");
	int ret = -1, eof = 0;
	int64_t deadline, next_hook, last_rx, last_tx;
	if (state(fd, &s)) return -1;
	if (s.hook != EN75XX_VOICE_OFFHOOK) {
		fprintf(stderr, "Audio refused: lift the handset first.\n"); return -1;
	}
	if (path) {
		/* Do not silently replace an earlier recording. */
		if (record) {
			int f = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
			if (f < 0) { perror(path); return -1; }
			fp = fdopen(f, "wb");
			if (!fp) { perror("fdopen"); close(f); return -1; }
		} else {
			fp = fopen(path, "rb");
			if (!fp) { perror(path); return -1; }
		}
	}
	if (ioctl(fd, EN75XX_VOICE_FLUSH)) { perror("FLUSH"); goto out; }
	deadline = now_ms() + seconds * 1000;
	next_hook = last_rx = last_tx = now_ms();
	while (!stopped && now_ms() < deadline) {
		struct pollfd p = { .fd = fd, .events = POLLIN | POLLOUT };
		int64_t now = now_ms();
		ssize_t n;
		if (now >= next_hook) {
			if (state(fd, &s)) goto out;
			if (s.hook == EN75XX_VOICE_ONHOOK) { puts("On-hook; stopping audio."); break; }
			next_hook = now + 100;
		}
		if (now - last_rx > 2000 || now - last_tx > 2000) {
			fprintf(stderr, "No %s progress for 2 s: check PCM/DMA/IRQ.\n",
				now - last_rx > 2000 ? "RX" : "TX"); goto out;
		}
		if (offset == length) {
			offset = 0; length = sizeof(tx);
			memset(tx, 0, sizeof(tx));
			if (play && !eof) {
				size_t nr = fread(tx, 1, sizeof(tx), fp);
				if (ferror(fp)) { perror("fread"); goto out; }
				if (nr & 1) { fprintf(stderr, "Input has an incomplete S16_LE sample.\n"); goto out; }
				if (nr < sizeof(tx)) eof = 1;
				/* Pad with silence and continue draining RX until duration expires. */
			} else if (!record && !play) {
				tone_frame(tx, &phase);
			}
		}
		if (poll(&p, 1, 50) < 0) {
			if (errno == EINTR) continue;
			perror("poll"); goto out;
		}
		if (p.revents & (POLLERR | POLLHUP | POLLNVAL)) {
			fprintf(stderr, "Device poll error: 0x%x\n", p.revents); goto out;
		}
		if (p.revents & POLLIN) {
			n = read(fd, rx, sizeof(rx));
			if (n < 0 && errno != EAGAIN && errno != EINTR) { perror("read"); goto out; }
			if (n > 0) {
				ssize_t i;
				if (n & 1) { fprintf(stderr, "Odd PCM read length\n"); goto out; }
				got += n; last_rx = now_ms();
				for (i = 0; i < n; i += 2) {
					int v = rx[i] | ((unsigned)rx[i+1] << 8);
					unsigned magnitude;
					if (v >= 32768) v -= 65536;
					magnitude = v < 0 ? -v : v;
					if (magnitude > peak) peak = magnitude;
					square_sum += (long double)v * v; samples++; nonzero += !!v;
				}
				if (record && fwrite(rx, 1, n, fp) != (size_t)n) { perror("fwrite"); goto out; }
			}
		}
		if (p.revents & POLLOUT) {
			n = write(fd, tx + offset, length - offset);
			if (n < 0 && errno != EAGAIN && errno != EINTR) { perror("write"); goto out; }
			if (n > 0) {
				if (n & 1) { fprintf(stderr, "Odd PCM write length\n"); goto out; }
				offset += n; sent += n; last_tx = now_ms();
			}
		}
	}
	ret = 0;
out:
	printf("userspace: RX=%" PRIu64 " TX=%" PRIu64 " bytes; RX peak=%u RMS=%.1f nonzero=%" PRIu64 "/%" PRIu64 "\n",
	       got, sent, peak, samples ? sqrt((double)(square_sum / samples)) : 0.0, nonzero, samples);
	if (fp && fclose(fp)) { perror("fclose"); ret = -1; }
	return ret;
}
static void usage(const char *name)
{
	fprintf(stderr, "Usage: %s DEVICE info\n"
		"       %s DEVICE {hook|ring|tone} SECONDS\n"
		"       %s DEVICE {record|play} SECONDS FILE.s16le\n"
		"SECONDS: 1..120 (ring: 1..15); ring cadence 1000/4000 ms.\n"
		"All opens start PCM DMA with ABI v1. Stop Asterisk before testing.\n",
		name, name, name);
}
int main(int argc, char **argv)
{
	struct en75xx_voice_info info;
	struct en75xx_voice_stats before = {0};
	struct sigaction sa = { .sa_handler = on_signal };
	unsigned seconds = 0;
	char *end;
	long value;
	int fd, ret, is_info, is_control, is_audio;
	if (argc < 3) { usage(argv[0]); return 2; }
	is_info = !strcmp(argv[2], "info");
	is_control = !strcmp(argv[2], "hook") || !strcmp(argv[2], "ring");
	is_audio = !strcmp(argv[2], "tone") || !strcmp(argv[2], "record") || !strcmp(argv[2], "play");
	if ((!is_info && !is_control && !is_audio) ||
	    argc != (is_info ? 3 : (!strcmp(argv[2], "record") || !strcmp(argv[2], "play")) ? 5 : 4)) {
		usage(argv[0]); return 2;
	}
	if (!is_info) {
		errno = 0; value = strtol(argv[3], &end, 10);
		if (errno || !*argv[3] || *end || value < 1 || value > (!strcmp(argv[2], "ring") ? 15 : 120)) {
			usage(argv[0]); return 2;
		}
		seconds = value;
	}
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL); sigaction(SIGTERM, &sa, NULL); sigaction(SIGHUP, &sa, NULL);
	setvbuf(stdout, NULL, _IOLBF, 0);
	fd = open(argv[1], O_RDWR | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0) { perror(argv[1]); return 1; }
	ret = -1;
	if (ioctl(fd, EN75XX_VOICE_GET_INFO, &info)) { perror("GET_INFO"); goto out; }
	printf("SLIC=%.*s ABI=%u channel=%u rate=%u bits=%u frame=%u caps=0x%x\n",
	       32, info.slic, info.abi_version, info.pcm_channel, info.sample_rate,
	       info.sample_bits, info.frame_samples, info.capabilities);
	if (info.abi_version != 1 || info.sample_rate != 8000 || info.sample_bits != 16 || info.frame_samples != 80) {
		fprintf(stderr, "Unsupported ABI/audio format\n"); goto out;
	}
	if (ioctl(fd, EN75XX_VOICE_GET_STATS, &before)) { perror("GET_STATS"); goto out; }
	if (is_info) {
		struct en75xx_voice_line_state s;
		ret = state(fd, &s);
		if (!ret) printf("hook=%u ringing(requested)=%u linefeed(cached)=%u faults=0x%x\n",
				s.hook, s.ringing, s.linefeed, s.faults);
	} else if (is_control) ret = controls(fd, argv[2], seconds);
	else ret = audio(fd, argv[2], seconds, argc == 5 ? argv[4] : NULL);
	stats(fd, &before);
out:
	/* release() is a second ring-stop path; SIGKILL cannot run our ioctl. */
	if (close(fd)) { perror("close"); ret = -1; }
	return ret ? 1 : stopped ? 130 : 0;
}
