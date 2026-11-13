#include <stdint.h>
#include "console.h"
#include "lavender_guardian.h"

#define SPMI_CORE 0x0800f000UL
#define SPMI_CNFG 0x0800a000UL
#define SPMI_OBS  0x09400000UL
#define SPMI_VERSION 0x00
#define SPMI_FEATURES 0x04
#define SPMI_V5_MAP 0x900
#define SPMI_V7_MAP 0x2000
#define SPMI_OWNER 0x700
#define SPMI_COUNT_MASK 0x7ffU
#define SPMI_MAX_APIDS 1024U
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
static void __attribute__((noreturn)) reset_now(const char *why)
{
	register uint64_t x0 __asm__("x0") = PSCI_SYSTEM_RESET;
	register uint64_t x1 __asm__("x1") = 0;
	register uint64_t x2 __asm__("x2") = 0;
	register uint64_t x3 __asm__("x3") = 0;
	con_skip();
	con_str("LAVENDER RESET", why);
	__asm__ volatile("dsb sy; isb; smc #0"
		: "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3)
		:
		: "x4","x5","x6","x7","x8","x9","x10","x11",
		  "x12","x13","x14","x15","x16","x17","memory");
	for (;;) __asm__ volatile("wfe");
}
struct spmi {
	uintptr_t obs;
	uint32_t stride;
	uint32_t pon_apid;
	uint32_t gpio_apid;
};
static int find_apid(uint32_t ppid, uint32_t n, uint32_t mapoff, uint32_t *out)
{
	int found=0;
	uint32_t selected=0;
	for(uint32_t i=0;i<n;i++) {
		uint32_t m=read32(SPMI_CORE+mapoff+i*4U);
		if(!m || ((m>>8)&0xfffU)!=ppid) continue;
		uint32_t owner=read32(SPMI_CNFG+SPMI_OWNER+i*4U)&7U;
		/* Select the last mapping owned by HLOS EE 0, as Linux does. */
		if(owner==0U) { selected=i; found=1; }
	}
	if(found) *out=selected;
	return found?0:-1;
}
static int discover(struct spmi *s)
{
	uint32_t ver=read32(SPMI_CORE+SPMI_VERSION);
	uint32_t n=read32(SPMI_CORE+SPMI_FEATURES)&SPMI_COUNT_MASK;
	uint32_t mapoff;
	if((ver&0xff000000U)==0x50000000U) {
		mapoff=SPMI_V5_MAP; s->stride=0x80U;
	} else if((ver&0xff000000U)==0x70000000U) {
		mapoff=SPMI_V7_MAP; s->stride=0x20U;
	} else return -1;
	if(!n || n>SPMI_MAX_APIDS) return -1;
	if(find_apid(SPMI_PON_PPID,n,mapoff,&s->pon_apid)) return -1;
	if(find_apid(SPMI_GPIO7_PPID,n,mapoff,&s->gpio_apid)) return -1;
	s->obs=SPMI_OBS;
	return 0;
}
static int read_byte(const struct spmi *s,uint32_t apid,uint16_t addr,uint8_t *out)
{
	uintptr_t c=s->obs+(uintptr_t)s->stride*apid;
	uint32_t cmd=(1U<<27)|((uint32_t)(addr&0xffU)<<4);
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
static int chord(const struct spmi *s)
{
	uint8_t mode,en,up,down;
	if(read_byte(s,s->gpio_apid,GPIO7_MODE,&mode) ||
	   read_byte(s,s->gpio_apid,GPIO7_ENABLE,&en) ||
	   read_byte(s,s->gpio_apid,GPIO7_RT_STS,&up) ||
	   read_byte(s,s->pon_apid,PON_RT_STS,&down)) return -1;
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
	volatile uint32_t *cell=(volatile uint32_t *)0;
	int ready=0;
	if(!f) reset_now("timer unavailable");
	for(;;) {
		uint64_t now=ticks();
		uintptr_t pa=lavender_guardian_cell_pa;
		if(pa && !cell) { cell=(volatile uint32_t *)pa; marker_last=*cell; progress=now; }
		else if(cell) {
			uint32_t marker=*cell;
			if(marker!=marker_last) { marker_last=marker; progress=now; }
			/* Stage 20 sets bit 31 after the early-kernel startup guard completes. */
			if(!(marker&0x80000000U) && now-progress>f*EARLY_TIMEOUT_SECONDS)
				reset_now("XNU early boot stalled");
		} else if(now-started>f*EARLY_TIMEOUT_SECONDS) {
			reset_now("XNU handoff did not start");
		}
		if(cell && !ready && now>=probe_at) {
			ready=(discover(&s)==0);
			probe_at=now+f;
		}
		if(ready) {
			int pressed=chord(&s);
			if(pressed==1) {
				if(!chord_since) chord_since=now;
				else if(now-chord_since>=f*CHORD_SECONDS)
					reset_now("VolUp+VolDown held 3 seconds");
			} else chord_since=0;
		}
		delay_ms(POLL_MS,f);
	}
}
