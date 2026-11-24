/*
 * Host test for bsd/dev/sweet_keys.c on LAVENDER (patch 0201): the real
 * driver source against a fake SDM660 SPMI arbiter v3 (core 0x0800f000,
 * cnfg 0x0800a000, observer 0x09400000 + 0x8000 * apid) with PM660 PON
 * (PPID 0x008) and PM660L GPIO7 (PPID 0x2c6).
 *
 * Checks: version gate, APID lookup (duplicate PPID entries prefer the
 * apps-owned one), which observer pages get mapped, that the driver only
 * ever writes the EXT_READL command word, and the key decoding of the
 * polling thread (power, volume-up active low, volume-down) into
 * sweet_hid_key() events.
 */
#include <setjmp.h>
#include "mock_mmio.h"

void mock_barrier(void);
#define MOCK_BARRIER(...) mock_barrier()

#define CORE_PA  0x0800f000u
#define CNFG_PA  0x0800a000u
#define OBS_PA   0x09400000u

#define PON_APID_OTHER  5       /* PPID 0x008 owned by another EE */
#define PON_APID        9       /* PPID 0x008 owned by apps (EE 0) */
#define GPIO_APID       300     /* PPID 0x2c6 */

/* Fake PMIC state */
static uint8_t pon_rt;          /* PON_INT_RT_STS: bit0 KPDPWR, bit1 RESIN */
/* 0220: PM660 RTC (PPID 0x060): 32-bit seconds, RTC_CTRL bit 7 enable */
#define RTC_APID        33
static uint32_t rtc_secs = 0x6abc1234u;
static uint8_t rtc_ctrl = 0x80;
static int rtc_tick_after_reads = -1;   /* >= 0: the counter carries after N byte reads */
static unsigned rtc_byte_reads;
static uint8_t gpio_rt = 1;     /* GPIO7 INT_RT_STS bit0: 1 = released (active low) */
static uint8_t gpio_mode = 0x00, gpio_en = 0x80, gpio_subtype = 0x10;
static unsigned bad_writes, reads_done;

static uint8_t
pmic_read(unsigned apid, unsigned addr)
{
	if (apid == PON_APID) {
		return addr == 0x10 ? pon_rt : 0;
	}
	if (apid == GPIO_APID) {
		switch (addr) {
		case 0x10: return gpio_rt;
		case 0x05: return gpio_subtype;
		case 0x40: return gpio_mode;
		case 0x46: return gpio_en;
		}
	}
	if (apid == RTC_APID) {
		if (addr == 0x46) {
			return rtc_ctrl;
		}
		if (addr >= 0x48 && addr <= 0x4b) {
			if (rtc_tick_after_reads >= 0 && (int)rtc_byte_reads++ == rtc_tick_after_reads) {
				rtc_secs += 1;          /* 0x...ff -> carry into byte 1 mid-read */
			}
			return (uint8_t)(rtc_secs >> (8 * (addr - 0x48)));
		}
	}
	return 0xee;
}

/* Hardware reacts to a command at the barrier that follows its write. */
void
mock_barrier(void)
{
	for (unsigned apid = 0; apid < 512; apid++) {
		struct mock_region *r = mock_find(OBS_PA + 0x8000u * apid, 0x1000);
		if (r == NULL || r->pa != OBS_PA + 0x8000u * apid) {
			continue;
		}
		uint32_t *ch = (uint32_t *)r->host;
		if (ch[0] == 0) {
			continue;
		}
		uint32_t cmd = ch[0];
		if ((cmd >> 27) != 1 || (cmd & 7u) != 0 || (cmd & ~((0x1fu << 27) | (0xffu << 4) | 7u))) {
			bad_writes++;
		}
		ch[0x18 / 4] = pmic_read(apid, (cmd >> 4) & 0xffu);
		ch[0x08 / 4] = 1;       /* done, no error */
		ch[0] = 0;
		reads_done++;
	}
}

void
delay(int usec)
{
	(void)usec;
}

/* HID events */
static int hid_events[32][2];
static unsigned hid_nevents;

void sweet_hid_key(int key, int down);
void
sweet_hid_key(int key, int down)
{
	hid_events[hid_nevents][0] = key;
	hid_events[hid_nevents][1] = down;
	hid_nevents++;
}

/* The polling thread: thread_block() advances the fake PMIC per tick and
 * leaves the infinite loop after the last step. */
static jmp_buf loop_exit;
static unsigned tick;

wait_result_t
thread_block(thread_continue_t continuation)
{
	(void)continuation;
	switch (++tick) {
	case 1: pon_rt = 1; break;              /* power down */
	case 2: pon_rt = 0; break;              /* power up */
	case 3: gpio_rt = 0; break;             /* volume-up down */
	case 4: gpio_rt = 1; pon_rt = 2; break; /* volume-up up, volume-down down */
	case 5: pon_rt = 0; break;              /* volume-down up */
	default: longjmp(loop_exit, 1);
	}
	return 0;
}

#include KEYS_SOURCE

static void
setup_arbiter(void)
{
	uint32_t *core = (uint32_t *)mock_add_region(CORE_PA, 0x1000);
	uint32_t *cnfg = (uint32_t *)mock_add_region(CNFG_PA, 0x1000);
	core[0] = 0x30000000u;                          /* v3 */
	core[(0x800 + 4 * PON_APID_OTHER) / 4] = 0x008u << 8;
	cnfg[(0x700 + 4 * PON_APID_OTHER) / 4] = 2;     /* owned by EE 2 */
	core[(0x800 + 4 * PON_APID) / 4] = 0x008u << 8;
	cnfg[(0x700 + 4 * PON_APID) / 4] = 0;           /* apps */
	core[(0x800 + 4 * GPIO_APID) / 4] = 0x2c6u << 8;
	core[(0x800 + 4 * 7) / 4] = 0x0c6u << 8;        /* PM660 GPIO7 (SID 0): must not match */
	mock_add_region(OBS_PA + 0x8000u * PON_APID_OTHER, 0x1000);
	mock_add_region(OBS_PA + 0x8000u * PON_APID, 0x1000);
	mock_add_region(OBS_PA + 0x8000u * GPIO_APID, 0x1000);
	mock_add_region(OBS_PA + 0x8000u * 7, 0x1000);
	core[(0x800 + 4 * RTC_APID) / 4] = 0x060u << 8;   /* PM660 rtc_rw@6000 */
	mock_add_region(OBS_PA + 0x8000u * RTC_APID, 0x1000);
}

int
main(void)
{
	setup_arbiter();
	sweet_keys_init();

	CHECK(keys_pon_apid == PON_APID, "PON apid %u, want %u (apps-owned entry)", keys_pon_apid, PON_APID);
	CHECK(keys_gpio_apid == GPIO_APID, "GPIO apid %u, want %u", keys_gpio_apid, GPIO_APID);
	CHECK(mock_pa_of(keys_pon_chan) == OBS_PA + 0x8000u * PON_APID, "PON channel PA 0x%llx",
	    (unsigned long long)mock_pa_of(keys_pon_chan));
	CHECK(mock_pa_of(keys_gpio_chan) == OBS_PA + 0x8000u * GPIO_APID, "GPIO channel PA 0x%llx",
	    (unsigned long long)mock_pa_of(keys_gpio_chan));
	CHECK(mock_find(OBS_PA + 0x8000u * PON_APID_OTHER, 4)->maps == 0, "mapped another EE's channel");
	CHECK(mock_find(OBS_PA + 0x8000u * 7, 4)->maps == 0, "mapped PM660 GPIO7 instead of PM660L");
	CHECK(mock_nthreads == 1, "polling thread not started (%u)", mock_nthreads);
	CHECK(reads_done == 5, "init reads %u, want 5", reads_done);

	if (setjmp(loop_exit) == 0) {
		keys_thread(NULL, 0);
	}
	static const int want[][2] = { {0, 1}, {0, 0}, {1, 1}, {1, 0}, {2, 1}, {2, 0} };
	CHECK(hid_nevents == 6, "HID events %u, want 6", hid_nevents);
	for (unsigned i = 0; i < 6 && i < hid_nevents; i++) {
		CHECK(hid_events[i][0] == want[i][0] && hid_events[i][1] == want[i][1],
		    "event %u = key %d down %d, want key %d down %d", i,
		    hid_events[i][0], hid_events[i][1], want[i][0], want[i][1]);
	}
	CHECK(bad_writes == 0, "%u writes that are not EXT_READL", bad_writes);

	/* Second call is a no-op. */
	unsigned before = reads_done;
	sweet_keys_init();
	CHECK(reads_done == before && mock_nthreads == 1, "sweet_keys_init not idempotent");

	/* A GPIO configured as output must be withheld, not reconfigured. */
	keys_started = false;
	mock_nthreads = 0;
	gpio_mode = 0x01;               /* LV/MV subtype: dir 1 = output */
	sweet_keys_init();
	CHECK(keys_gpio_chan == 0 && keys_gpio_apid == KEYS_NO_APID, "output GPIO not withheld");
	CHECK(keys_pon_chan != 0, "PON lost when GPIO withheld");

	/* 0220: PM660 RTC -> calendar */
	{
		uint32_t raw = 0;
		uint8_t ctrl = 0;
		CHECK(lavender_rtc_read(&raw, &ctrl) && raw == 0x6abc1234u && ctrl == 0x80,
		    "RTC read 0x%08x ctrl 0x%02x", raw, ctrl);
		CHECK(lavender_rtc_gmt() == 0x6abc1234L, "plausible raw not used as is");
		rtc_secs = 12345u;                      /* not Unix time, no offset: fall back */
		CHECK(lavender_rtc_gmt() == 0, "implausible raw used without sweetrtcoff");
		mock_bootarg_rtcoff_set = 1;
		mock_bootarg_rtcoff = 1700000000 - 12345;
		CHECK(lavender_rtc_gmt() == 1700000000L, "sweetrtcoff not applied");
		mock_bootarg_rtcoff = -5;
		rtc_secs = 0x6abc1234u;
		CHECK(lavender_rtc_gmt() == 0x6abc1234L - 5, "negative sweetrtcoff");
		mock_bootarg_rtcoff_set = 0;
		/* carry from byte 0 into byte 1 between the byte reads: re-read, consistent value */
		rtc_secs = 0x6abc12ffu;
		rtc_byte_reads = 0;
		rtc_tick_after_reads = 1;               /* after byte 0, before byte 1 */
		CHECK(lavender_rtc_read(&raw, &ctrl) && raw == 0x6abc1300u, "torn read 0x%08x", raw);
		rtc_tick_after_reads = -1;
		rtc_ctrl = 0x00;                        /* RTC disabled */
		CHECK(lavender_rtc_gmt() == 0, "disabled RTC used");
		rtc_ctrl = 0x80;
		CHECK(bad_writes == 0, "%u writes that are not EXT_READL", bad_writes);
	}

	/* A v5 arbiter is not this layout. */
	keys_started = false;
	mock_nthreads = 0;
	*mock_reg(CORE_PA) = 0x50000000u;
	sweet_keys_init();
	CHECK(mock_nthreads == 0, "v5 arbiter accepted by the v3 driver");

	if (mock_failures) {
		fprintf(stderr, "test_lavender_keys: %u failure(s)\n", mock_failures);
		return 1;
	}
	printf("test_lavender_keys: OK\n");
	return 0;
}
