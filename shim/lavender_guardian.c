#include <stdint.h>
#include "console.h"
#include "lavender_guardian.h"

#define SPMI_CORE 0x0800f000UL
#define SPMI_CNFG 0x0800a000UL
#define SPMI_OBS  0x09400000UL
#define SPMI_CORE_SIZE 0x1000U
#define SPMI_VERSION 0x00
#define SPMI_V2_MIN 0x20010000U
#define SPMI_V3_MIN 0x30000000U
#define SPMI_V5_MIN 0x50000000U
#define SPMI_V7_MIN 0x70000000U
#define SPMI_V2_MAP 0x800U
#define SPMI_V5_MAP 0x900U
#define SPMI_OWNER 0x700U
#define SPMI_MAX_APIDS 512U
#define SPMI_CMD 0x00
#define SPMI_STATUS 0x08
#define SPMI_RDATA0 0x18
#define SPMI_DONE 0x01U
#define SPMI_ERROR 0x0eU
#define SPMI_PON_PPID 0x008U
#define SPMI_GPIO7_PPID 0x2c6U
#define PON_RT_STS 0x0810U
#define GPIO7_MODE 0xc640U
#define GPIO7_ENABLE 0xc646U
#define GPIO7_RT_STS 0xc610U
#define GPIO_ENABLE_BIT 0x80U
#define GPIO_STATUS_BIT 0x01U
#define PON_RESIN_N_SET 0x02U
#define PSCI_SYSTEM_RESET 0x84000009UL
#define PSCI_FEATURES 0x8400000aUL
#define QCOM_WDT_BASE 0x17817000UL
#define WDT0_RST 0x04U
#define WDT0_EN 0x08U
#define WDT0_BITE_TIME 0x14U
#define SPMI_OP_EXT_READL 1U
#define EARLY_TIMEOUT_SECONDS 60U
#define CHORD_SECONDS 3U
#define POLL_MS 10U

static inline uint32_t read32(uintptr_t a) { return *(volatile uint32_t *)a; }
static inline void write32(uintptr_t a, uint32_t v) { *(volatile uint32_t *)a = v; }
static uint64_t ticks(void)
{
	uint64_t v;
	__asm__ volatile("isb; mrs %0, cntpct_el0" : "=r"(v));
	return v;
}
static uint64_t frequency(void)
{
	uint64_t v;
	__asm__ volatile("mrs %0, cntfrq_el0" : "=r"(v));
	return v;
}
static void delay_ms(unsigned ms, uint64_t f)
{
	uint64_t until = ticks() + (f / 1000U) * ms;
	while (ticks() < until) __asm__ volatile("yield" ::: "memory");
}
static uint64_t smc_call(uint64_t fn, uint64_t a1, uint64_t a2, uint64_t a3)
{
	register uint64_t x0 __asm__("x0") = fn;
	register uint64_t x1 __asm__("x1") = a1;
	register uint64_t x2 __asm__("x2") = a2;
	register uint64_t x3 __asm__("x3") = a3;
	__asm__ volatile("smc #0"
		: "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3)
		:
		: "x4","x5","x6","x7","x8","x9","x10","x11",
		  "x12","x13","x14","x15","x16","x17","memory");
	return x0;
}

static void __attribute__((noreturn)) reset_now(const char *why)
{
	con_skip();
	con_str("RESET", why);

	/* Use the firmware reset service before touching unverified MMIO. */
	int64_t feature = (int64_t)smc_call(PSCI_FEATURES, PSCI_SYSTEM_RESET, 0, 0);
	con_hex("PSCI RESET FEAT", (uint64_t)feature);
	if (feature >= 0) {
		con_str("RESET", "PSCI system reset");
		__asm__ volatile("dsb sy; isb" ::: "memory");
		uint64_t ret = smc_call(PSCI_SYSTEM_RESET, 0, 0, 0);
		con_hex("PSCI RET", ret);
	}

	/*
	 * SDM660's qcom,msm-watchdog is driven by watchdog_v2.c. Its
	 * msm_trigger_wdog_bite() path programs BITE_TIME=1, then resets
	 * the counter. CNTFRQ_EL0 is the architectural timer rate, not the
	 * watchdog clock; never derive watchdog ticks from it.
	 * Enable the block and request the driver's one-tick bite sequence.
	 * No MMIO readback: a stalled peripheral read must not trap here.
	 */
	con_str("RESET", "watchdog one-tick bite fallback");
	write32(QCOM_WDT_BASE + WDT0_EN, 0);
	write32(QCOM_WDT_BASE + WDT0_RST, 1);
	write32(QCOM_WDT_BASE + WDT0_BITE_TIME, 1);
	__asm__ volatile("dsb sy" ::: "memory");
	write32(QCOM_WDT_BASE + WDT0_EN, 1);
	write32(QCOM_WDT_BASE + WDT0_RST, 1);
	__asm__ volatile("dsb sy; isb" ::: "memory");
	for (;;) __asm__ volatile("wfe");
}
struct spmi {
	uintptr_t obs;
	uint32_t stride;
	uint32_t pon_apid;
	uint32_t gpio_apid;
	uint32_t pon_sid;
	uint32_t gpio_sid;
};
static int find_apid(uint32_t ppid, uint32_t n, uint32_t mapoff,
                     uint32_t owner_filter, uint32_t *out)
{
    int found=0;
    uint32_t selected=0;
    for(uint32_t i=0;i<n;i++) {
        uint32_t m=read32(SPMI_CORE+mapoff+i*4U);
        if(!m || ((m>>8)&0xfffU)!=ppid) continue;
        if(owner_filter) {
            uint32_t owner=read32(SPMI_CNFG+SPMI_OWNER+i*4U)&7U;
            if(owner!=0U) continue;
        }
        selected=i;
        found=1;
    }
    if(found) *out=selected;
    return found?0:-1;
}
static int discover(struct spmi *s)
{
    uint32_t ver=read32(SPMI_CORE+SPMI_VERSION);
    uint32_t mapoff, n, owner_filter=0;
    if(ver<SPMI_V2_MIN) return -1;
    if(ver<SPMI_V5_MIN) {
        mapoff=SPMI_V2_MAP;
        n=(SPMI_CORE_SIZE-mapoff)/4U;
        s->stride=0x8000U;
    } else {
        mapoff=SPMI_V5_MAP;
        n=(SPMI_CORE_SIZE-mapoff)/4U;
        s->stride=0x80U;
        owner_filter=1U;
    }
    if(!n || n>SPMI_MAX_APIDS) return -1;
    if(find_apid(SPMI_PON_PPID,n,mapoff,owner_filter,&s->pon_apid)) return -1;
    if(find_apid(SPMI_GPIO7_PPID,n,mapoff,owner_filter,&s->gpio_apid)) return -1;
    s->obs=SPMI_OBS;
    s->pon_sid=(SPMI_PON_PPID>>8)&0xfU;
    s->gpio_sid=(SPMI_GPIO7_PPID>>8)&0xfU;
    con_hex("SPMI VER",ver);
    con_hex2("SPMI IDs","PON=",s->pon_apid,"GPIO7=",s->gpio_apid);
    return 0;
}
static int read_byte(const struct spmi *s,uint32_t apid,uint32_t sid,
                     uint16_t addr,uint8_t *out)
{
	uintptr_t c=s->obs+(uintptr_t)s->stride*apid;
	/* PMIC-arb command format: opcode, SID, 8-bit peripheral address,
	 * 8-bit register offset, byte count. Keep the full 16-bit address. */
	uint32_t cmd=(SPMI_OP_EXT_READL<<27)|((sid&0xfU)<<20)|
	             (((uint32_t)(addr>>8)&0xffU)<<12)|
	             (((uint32_t)addr&0xffU)<<4);
	uint64_t f=frequency(), deadline=ticks()+f/1000U+1U;
	uint32_t st;
	write32(c+SPMI_CMD,cmd);
	__asm__ volatile("dsb sy" ::: "memory");
	for(;;) {
		st=read32(c+SPMI_STATUS);
		if(st&SPMI_ERROR) return -1;
		if(st&SPMI_DONE) break;
		if(ticks()>=deadline) return -1;
	}
	*out=(uint8_t)(read32(c+SPMI_RDATA0)&0xffU);
	return 0;
}
static int chord(const struct spmi *s,uint8_t *up_out,uint8_t *down_out)
{
	uint8_t mode,en,up,down;
	if(read_byte(s,s->gpio_apid,s->gpio_sid,GPIO7_MODE,&mode) ||
	   read_byte(s,s->gpio_apid,s->gpio_sid,GPIO7_ENABLE,&en) ||
	   read_byte(s,s->gpio_apid,s->gpio_sid,GPIO7_RT_STS,&up) ||
	   read_byte(s,s->pon_apid,s->pon_sid,PON_RT_STS,&down)) return -1;
	*up_out=up;
	*down_out=down;
	/* Fail closed: GPIO7 must already be an enabled digital input. */
	if((mode&0x7fU)!=0U || !(en&GPIO_ENABLE_BIT)) return -1;
	/* GPIO7 has pull-up (active-low); PON RESIN_N_SET reports VolDown. */
	return (!(up&GPIO_STATUS_BIT) && (down&PON_RESIN_N_SET)) ? 1 : 0;
}
void lavender_guardian_run(void)
{
	struct spmi s;
	uint64_t f=frequency(), started=ticks(), progress=started, chord_since=0, probe_at=started;
	uint32_t marker_last=0xffffffffU;
	uint8_t last_up=0xffU,last_down=0xffU;
	volatile uint32_t *cell=(volatile uint32_t *)0;
	int ready=0;
	if(!f) reset_now("timer unavailable");
	for(;;) {
		uint64_t now=ticks();
		uintptr_t pa=lavender_guardian_cell_pa;
		if(pa && !cell) { cell=(volatile uint32_t *)pa; marker_last=*cell; progress=now; con_hex("CELL PA",pa); con_hex("GUARDMK",marker_last); }
		else if(cell) {
			uint32_t marker=*cell;
			if(marker!=marker_last) { marker_last=marker; progress=now; }
			/* Stage 20 ends the early timeout only after key guardian is ready. */
			if(now-progress>f*EARLY_TIMEOUT_SECONDS) {
				uint64_t age=(now-progress)/f;
				if(!(marker&0x80000000U)) {
					/* XNU may have cleared the shared framebuffer. Repaint
					 * only for a genuine pre-stage-20 stall. */
					con_clear(RED);
					con_str("LAVENDER","XNU EARLY BOOT STALLED");
					con_hex("LAST STAGE",marker);
					con_hex("STALL SECS",age);
					reset_now("XNU early boot stalled");
				} else if(!ready) {
					con_clear(RED);
					con_str("LAVENDER","KEY GUARDIAN UNAVAILABLE");
					con_hex("LAST STAGE",marker);
					con_hex("WAIT SECS",age);
					reset_now("key guardian unavailable");
				} else {
					/* Stage 20 intentionally ends marker updates. Do not
					 * misreport later kernel startup as an early stall. */
					progress=now;
				}
			}
		} else if(now-started>f*EARLY_TIMEOUT_SECONDS) {
			con_clear(RED);
			con_str("LAVENDER","XNU HANDOFF NOT SEEN");
			reset_now("XNU handoff did not start");
		}
		/*
		 * Do not touch the PMIC/SPMI block while XNU is still in its
		 * fragile pre-MMU/arm_init path. These are direct MMIO transactions;
		 * an unresponsive peripheral must not freeze the independent A53
		 * progress watchdog before its 60-second deadline.
		 */
		if(cell && (marker_last & 0x80000000U) && !ready && now>=probe_at) {
			ready=(discover(&s)==0);
			probe_at=now+5U*f;
		}
		if(ready) {
			uint8_t up=0,down=0;
			int pressed=chord(&s,&up,&down);
			if(pressed>=0 && (up!=last_up || down!=last_down)) {
				con_hex2("KEYS","GPIO7=",up,"PON=",down);
				last_up=up; last_down=down;
			}
			if(pressed==1) {
				if(!chord_since) { chord_since=now; con_str("KEYS","both pressed; hold 3s"); }
				else if(now-chord_since>=f*CHORD_SECONDS)
					reset_now("VolUp+VolDown held 3 seconds");
			} else {
				chord_since=0;
				if(pressed<0) {
					ready=0;
					probe_at=now+f;
				}
			}
		}
		delay_ms(POLL_MS,f);
	}
}
