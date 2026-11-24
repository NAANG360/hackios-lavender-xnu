/*
 * tests/test_log_stamp.c
 *
 * Host test for patch 0202: "[  SSS.mmm] " uptime stamp at the start of
 * every santonilog ring line. The functions below are the patch's code,
 * compiled against host stand-ins for os_atomic_* and mach_absolute_time().
 */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#define SANTONI_LOG_SIZE (4u << 20)
#define os_atomic_add_orig(p, v, o) __atomic_fetch_add(p, v, __ATOMIC_RELAXED)
#define os_atomic_load(p, o)        __atomic_load_n(p, __ATOMIC_RELAXED)
#define os_atomic_store(p, v, o)    __atomic_store_n(p, v, __ATOMIC_RELAXED)
#define _Atomic

static char santoni_log_ring[SANTONI_LOG_SIZE];
static uint64_t santoni_log_total;
static uint64_t fake_now;
static uint64_t mach_absolute_time(void) { return fake_now; }


/* Called for all console output from any context: never blocks */
void santoni_log_append(const char *c, int size);

/*
 * 0194: every ring line starts with "[  SSS.mmm] " (uptime). The second
 * lavender boot took ~20 min and the log could not say where: 654 flushes,
 * no times. The stamp is CNTVCT / 19.2 MHz (SDM660 arch timer, the ADT
 * timebase-frequency; QEMU's 62.5 MHz reads 3.26x too fast). Integer math
 * on mach_absolute_time() only: absolutetime_to_nanoseconds() divides by
 * rtclock state that is still zero for the first console lines.
 * The line-start flag is shared without a lock; two CPUs racing can drop
 * or double a stamp, never corrupt the ring.
 */
#define SANTONI_LOG_TIMEBASE_HZ   19200000ull
#define SANTONI_LOG_STAMP_LEN     12            /* "[" 5 "." 3 "] " */

static _Atomic bool santoni_log_bol = true;

static void
santoni_log_put(const char *c, int size)
{
	uint64_t at = os_atomic_add_orig(&santoni_log_total, (uint64_t)size,
	    relaxed);

	for (int i = 0; i < size; i++) {
		santoni_log_ring[(at + (uint64_t)i) % SANTONI_LOG_SIZE] = c[i];
	}
}

static void
santoni_log_stamp(void)
{
	uint64_t ms = mach_absolute_time() / (SANTONI_LOG_TIMEBASE_HZ / 1000);
	uint64_t s = ms / 1000;
	char b[24];
	int n = sizeof(b);

	b[--n] = ' ';
	b[--n] = ']';
	for (int k = 0; k < 3; k++) {
		b[--n] = (char)('0' + ms % 10);
		ms /= 10;
	}
	b[--n] = '.';
	do {
		b[--n] = (char)('0' + s % 10);
		s /= 10;
	} while (s != 0 && n > 1);
	while ((int)sizeof(b) - n < SANTONI_LOG_STAMP_LEN - 1) {
		b[--n] = ' ';
	}
	b[--n] = '[';
	santoni_log_put(b + n, (int)sizeof(b) - n);
}

void
santoni_log_append(const char *c, int size)
{
	int i = 0;

	while (i < size) {
		int j = i;

		if (os_atomic_load(&santoni_log_bol, relaxed)) {
			os_atomic_store(&santoni_log_bol, false, relaxed);
			santoni_log_stamp();
		}
		while (j < size && c[j] != '\n') {
			j++;
		}
		if (j < size) {
			j++;            /* keep the '\n' with its line */
		}
		santoni_log_put(c + i, j - i);
		if (c[j - 1] == '\n') {
			os_atomic_store(&santoni_log_bol, true, relaxed);
		}
		i = j;
	}
}

int
main(void)
{
	static const char want[] =
	    "[    0.000] boot\n"
	    "[    0.000] second line\n"
	    "[  754.021] third\n"
	    "[  754.021] \n"
	    "[  754.021] end\n"
	    "[123456.000] big\n";

	santoni_log_append("x", 0);                 /* size 0: nothing, no stamp */
	fake_now = 0;
	santoni_log_append("boot\nsec", 8);         /* line split across calls */
	fake_now = 19200000ull * 754 + 19200 * 21;  /* 754.021 s at 19.2 MHz */
	santoni_log_append("ond line\n", 9);
	santoni_log_append("third\n\nend", 10);     /* empty line gets a stamp */
	fake_now = 19200000ull * 123456;
	santoni_log_append("\n", 1);
	santoni_log_append("big\n", 4);             /* > 5 digit seconds widen */

	assert(santoni_log_total == sizeof(want) - 1);
	assert(memcmp(santoni_log_ring, want, sizeof(want) - 1) == 0);
	printf("test_log_stamp: ok\n");
	return 0;
}
