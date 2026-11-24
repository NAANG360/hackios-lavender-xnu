/*
 * Host test for bsd/dev/sweet_usb.c on LAVENDER (patch 0200): the real
 * driver source against fake SDM660 GCC, DWC3, QSCRATCH and QUSB2 v1
 * register files at the addresses of the phone's DTB.
 *
 * SCENARIO 1 (sweetusb=1, ABL left everything on): read-only. No GCC,
 *   DWC3, QSCRATCH or PHY register may change; the SM7150 addresses must
 *   never be mapped.
 * SCENARIO 2 (sweetusb=3, recovery boot left USB off and the core in OTG
 *   mode): GDSC and the seven branch clocks are enabled, the core is put in
 *   device mode, the VBUS session is forced in QSCRATCH, the device
 *   controller is soft-reset, given our event buffer, run, and the event
 *   and live-log threads are started.
 * SCENARIO 3 (sweetusb=1, ABL left USB off): nothing beyond GCC status is
 *   read and nothing is written.
 * SCENARIO 4 (0210, sweetusb=2, the state of the first lavender boot: USB
 *   on, QUSB2 powered down 0x23, PLL unlocked, TCSR clamp engaged): the
 *   clamp is released, POWER_DOWN cleared keeping 0x22, the PLL locks before
 *   RUN_STOP; sweetusbhold=1 then stays in its status loop.
 * SCENARIO 5 (0210, PHY supplies off: the PLL never locks): the wait is
 *   bounded; with no PHY clock SETEPCONFIG hangs, so nothing runs (0211
 *   model); sweetusbhold=0 returns at once.
 * SCENARIOS 4/5 also start from UTMI_CTRL2 = 0x40 (TEST_MUX_SEL, every lavender
 *   log) and expect 0212 to release it (0x00).
 * SCENARIO 7 (0213, CDC-NCM, as Windows UsbNcm drives it): descriptors,
 *   NTB parameters/format, configuration, SPEED_CHANGE then
 *   NETWORK_CONNECTION, an OUT NTB with two datagrams, a malformed one, IN
 *   NTBs (plus the 512-byte pad), a bus reset and reconfiguration.
 * SCENARIO 6 (0211 off, sweetusbfix=0): reproduces the device log of boot
 *   usbtest-0210: PHY locked, "ep0 cmd 0x1 timeout", no RUN_STOP.
 * SCENARIOS 4/2 also check the 0211 bring-up: UTMI clock as PIPE clock,
 *   core+PHY soft reset, HS quirks, then SETEPCONFIG completes.
 */
#include <setjmp.h>
#include "mock_mmio.h"

/*
 * 0221: the touch driver is not part of this host test; the USB driver only
 * needs the symbol to link. The kernel build uses the real definition from
 * bsd/dev/santoni_touch.c.
 */
void
santoni_touch_remote(const uint8_t *frame, uint32_t len)
{
	(void)frame;
	(void)len;
}

void mock_barrier(void);
#define MOCK_BARRIER(...) mock_barrier()

#define T_GCC_PA     0x00100000u
#define T_DWC3_PA    0x0a800000u
#define T_QS_PA      0x0a8f8800u
#define T_QUSB2_PA   0x0c012000u
#define T_TCSR_PAGE  0x01fcb000u
#define TCSR_CLAMP   (*mock_reg(T_TCSR_PAGE + 0x24c))
#define PHY(off)     (*mock_reg(T_QUSB2_PA + (off)))

static bool pll_can_lock = true;
static bool rs_after_lock = true;      /* RUN_STOP never set while the PLL is unlocked */
/*
 * 0211 model of the device: SETEPCONFIG writes the endpoint context RAM,
 * which runs from the PHY side; it only completes once the core went through
 * PHYSOFTRST with the PHY PLL locked (lavender boot usbtest-0210 and the
 * Poco F1: DEPSTARTCFG completes, SETEPCONFIG hangs). Other commands always
 * complete.
 */
static bool phy_synced;
static bool u2_in_reset;

static const uint32_t cbcr_offs[7] = { 0xf008, 0xf00c, 0xf010, 0x5018, 0xf03c, 0x6a004, 0x88018 };

#define GDSCR      (*mock_reg(T_GCC_PA + 0xf004))
#define DWC(off)   (*mock_reg(T_DWC3_PA + (off)))

/* Fake hardware reaction points */
static void
hw_step(void)
{
	if (!(GDSCR & 1u)) {
		GDSCR |= 1u << 31;                      /* collapse released -> power on */
	}
	for (unsigned i = 0; i < 7; i++) {
		uint32_t *c = mock_reg(T_GCC_PA + cbcr_offs[i]);
		if (*c & 1u) {
			*c &= ~(1u << 31);              /* enabled -> running */
		}
	}
	if (DWC(0xc704) & (1u << 30)) {
		DWC(0xc704) &= ~(1u << 30);             /* soft reset completes */
	}
	if (DWC(0xc704) & (1u << 31)) {
		DWC(0xc70c) &= ~(1u << 22);             /* run -> not halted */
	}
	if (DWC(0xc200) & (1u << 31)) {
		u2_in_reset = true;
	} else if (u2_in_reset) {
		u2_in_reset = false;
		phy_synced = (PHY(0x38) & (1u << 5)) != 0;  /* PHYSOFTRST released with the PLL locked */
	}
	for (unsigned ep = 0; ep < 8; ep++) {
		uint32_t cmd = DWC(0xc80c + 0x10 * ep);
		if ((cmd & 0xfu) == 1u && !phy_synced) {
			continue;                       /* SETEPCONFIG hangs without the PHY clock */
		}
		DWC(0xc80c + 0x10 * ep) &= ~(1u << 10); /* endpoint command done */
	}
	/* QUSB2 v1: 8-bit registers, read back in every byte lane */
	uint32_t pd = PHY(0xb4) & 0xffu;
	if (PHY(0xb4) != pd * 0x01010101u) {
		PHY(0xb4) = pd * 0x01010101u;   /* a byte write shows in every lane */
	}
	if (pll_can_lock && !(pd & 1u) && (TCSR_CLAMP & 1u) && !(PHY(0x38) & (1u << 5))) {
		PHY(0x38) |= 0x20202020u;       /* PLL_LOCK (bit 5) */
	}
	if ((DWC(0xc704) & (1u << 31)) && !(PHY(0x38) & (1u << 5))) {
		rs_after_lock = false;
	}
}

void
mock_barrier(void)
{
	hw_step();
}

void
delay(int usec)
{
	(void)usec;
	hw_step();
}

static jmp_buf hold_exit;
static unsigned blocks, block_limit;

wait_result_t
thread_block(thread_continue_t continuation)
{
	(void)continuation;
	if (block_limit && ++blocks >= block_limit) {
		longjmp(hold_exit, 1);
	}
	return 0;
}

/* Memory and cache */
vm_map_t kernel_map;
pmap_t kernel_pmap;
static uint8_t dma_area[0x4000] __attribute__((aligned(0x4000)));
#define DMA_PN     0x123456u

kern_return_t
kmem_alloc_contig(vm_map_t map, vm_offset_t *addr, vm_size_t size, vm_offset_t mask,
    ppnum_t max_pnum, ppnum_t pnum_mask, int flags, int tag)
{
	(void)map; (void)mask; (void)max_pnum; (void)pnum_mask; (void)flags; (void)tag;
	if (size > sizeof(dma_area)) {
		return 1;
	}
	*addr = (vm_offset_t)dma_area;
	return KERN_SUCCESS;
}

ppnum_t
pmap_find_phys(pmap_t pmap, addr64_t va)
{
	(void)pmap;
	return va == (addr64_t)(vm_offset_t)dma_area ? DMA_PN : 0;
}

void
flush_dcache64(addr64_t pa, unsigned len, boolean_t phys)
{
	(void)pa; (void)len; (void)phys;
}

/* Locks and networking: succeed, record nothing beyond the interface. */
static lck_grp_t the_grp;
static lck_mtx_t the_mtx;
lck_grp_t *lck_grp_alloc_init(const char *n, void *a) { (void)n; (void)a; return &the_grp; }
lck_mtx_t *lck_mtx_alloc_init(lck_grp_t *g, void *a) { (void)g; (void)a; return &the_mtx; }
void lck_mtx_lock(lck_mtx_t *m) { (void)m; }
void lck_mtx_unlock(lck_mtx_t *m) { (void)m; }
const uint8_t etherbroadcastaddr[ETHER_ADDR_LEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
int ether_demux(void) { return 0; }
int ether_add_proto(void) { return 0; }
int ether_del_proto(void) { return 0; }
int ether_check_multi(void) { return 0; }
int ether_frameout_extended(void) { return 0; }
static int the_ifnet;
static unsigned ifnet_attached, ifaddr_set;
errno_t ifnet_allocate_extended(const struct ifnet_init_eparams *i, ifnet_t *ifp) { (void)i; *ifp = (ifnet_t)&the_ifnet; return 0; }
errno_t ifnet_attach(ifnet_t ifp, const void *ll) { (void)ifp; (void)ll; ifnet_attached++; return 0; }
errno_t ifnet_set_addrlen(ifnet_t ifp, int len) { (void)ifp; (void)len; return 0; }
errno_t ifnet_set_hdrlen(ifnet_t ifp, int len) { (void)ifp; (void)len; return 0; }
errno_t ifnet_set_mtu(ifnet_t ifp, int mtu) { (void)ifp; (void)mtu; return 0; }
errno_t ifnet_set_baudrate(ifnet_t ifp, uint64_t b) { (void)ifp; (void)b; return 0; }
errno_t ifnet_set_flags(ifnet_t ifp, uint32_t f, uint32_t m) { (void)ifp; (void)f; (void)m; return 0; }
errno_t ifnet_set_lladdr(ifnet_t ifp, const void *ll, size_t len) { (void)ifp; (void)ll; (void)len; return 0; }
errno_t ifnet_input(ifnet_t ifp, mbuf_t m, const struct ifnet_stat_increment_param *s);
errno_t ifnet_stat_increment_out(ifnet_t ifp, uint32_t p, uint32_t b, uint32_t e) { (void)ifp; (void)p; (void)b; (void)e; return 0; }
errno_t proto_plumb(int pf, ifnet_t ifp) { (void)pf; (void)ifp; return 0; }
static int the_socket;
errno_t sock_socket(int d, int t, int p, void *cb, void *c, socket_t *so) { (void)d; (void)t; (void)p; (void)cb; (void)c; *so = (socket_t)&the_socket; return 0; }
errno_t sock_ioctl(socket_t so, unsigned long req, void *arg) { (void)so; (void)arg; if (req == SIOCAIFADDR) { ifaddr_set++; } return 0; }
errno_t sock_send(socket_t so, const struct msghdr *msg, int flags, size_t *sent) { (void)so; (void)msg; (void)flags; *sent = 0; return 0; }
void sock_close(socket_t so) { (void)so; }
/* 0213: working mbufs: one 2 KiB cluster each, a data offset, a packet length */
struct mock_mbuf { uint8_t buf[2048]; size_t off, len, pktlen; };
static unsigned mbufs_live;
errno_t mbuf_gethdr(int how, int type, mbuf_t *m) { (void)how; (void)type; *m = calloc(1, sizeof(struct mock_mbuf)); mbufs_live++; return 0; }
errno_t mbuf_mclget(int how, int type, mbuf_t *m) { (void)how; (void)type; (void)m; return 0; }
int mbuf_flags(mbuf_t m) { (void)m; return MBUF_EXT; }
size_t mbuf_maxlen(mbuf_t m) { (void)m; return 2048; }
void *mbuf_data(mbuf_t m) { return m->buf + m->off; }
size_t mbuf_len(mbuf_t m) { return m->len; }
errno_t mbuf_setlen(mbuf_t m, size_t len) { m->len = len; return 0; }
errno_t mbuf_pkthdr_setlen(mbuf_t m, size_t len) { m->pktlen = len; return 0; }
size_t mbuf_pkthdr_len(mbuf_t m) { return m->pktlen; }
errno_t mbuf_pkthdr_setrcvif(mbuf_t m, ifnet_t ifp) { (void)m; (void)ifp; return 0; }
errno_t mbuf_pkthdr_setheader(mbuf_t m, void *h) { (void)m; (void)h; return 0; }
void mbuf_pkthdr_adjustlen(mbuf_t m, int a) { m->pktlen += a; }
errno_t mbuf_setdata(mbuf_t m, void *d, size_t len) { m->off = (size_t)((uint8_t *)d - m->buf); m->len = len; return 0; }
errno_t mbuf_copydata(mbuf_t m, size_t off, size_t len, void *out) { memcpy(out, m->buf + m->off + off, len); return 0; }
void mbuf_freem(mbuf_t m) { if (m) { free(m); mbufs_live--; } }
/* received frames: full length (header + payload) and the first payload byte */
static unsigned rx_count;
static size_t rx_len[8];
static uint8_t rx_first[8];

/* The SYSTEM console ring readers added to santoni_sdhci.c by 0200. */
static unsigned santoni_reader_calls;
uint32_t santoni_log_copy(uint64_t *pos, char *out, uint32_t max);
uint64_t santoni_log_position(void);
uint32_t santoni_log_copy(uint64_t *pos, char *out, uint32_t max) { (void)pos; (void)out; (void)max; santoni_reader_calls++; return 0; }
uint64_t santoni_log_position(void) { santoni_reader_calls++; return 0; }

#include USB_SOURCE

errno_t
ifnet_input(ifnet_t ifp, mbuf_t m, const struct ifnet_stat_increment_param *s)
{
	(void)ifp; (void)s;
	if (rx_count < 8) {
		rx_len[rx_count] = m->pktlen + ETHER_HDR_LEN;
		rx_first[rx_count] = m->buf[m->off];
	}
	rx_count++;
	mbuf_freem(m);
	return 0;
}

/* 0213: drive ep0 like a host: a SETUP packet, then the SETUP XferComplete event */
static uint32_t
host_setup(uint8_t type, uint8_t req, uint16_t value, uint16_t index, uint16_t length)
{
	uint8_t *st = (uint8_t *)(ud.dma + UD_SETUP);
	st[0] = type; st[1] = req;
	st[2] = (uint8_t)value; st[3] = (uint8_t)(value >> 8);
	st[4] = (uint8_t)index; st[5] = (uint8_t)(index >> 8);
	st[6] = (uint8_t)length; st[7] = (uint8_t)(length >> 8);
	ud.state = EP0_SETUP;
	ud_event((0u << 1) | (1u << 6));                /* ep0 OUT XferComplete */
	volatile uint32_t *t = (volatile uint32_t *)(ud.dma + UD_TRB_IN);
	return t[2];                                    /* IN data stage length */
}

static void
put16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
static void
put32(uint8_t *p, uint32_t v) { put16(p, (uint16_t)v); put16(p + 2, (uint16_t)(v >> 16)); }

static uint8_t *gcc_mem, *dwc_mem, *qs_mem, *phy_mem;

static void
setup(bool powered, uint32_t gctl)
{
	gcc_mem = mock_add_region(T_GCC_PA, 0x94000);
	dwc_mem = mock_add_region(T_DWC3_PA, 0xd000);
	qs_mem = mock_add_region(T_QS_PA, 0x100);
	phy_mem = mock_add_region(T_QUSB2_PA, 0x400);
	mock_add_region(T_TCSR_PAGE, 0x1000);
	TCSR_CLAMP = 1;
	GDSCR = powered ? (1u << 31) : 1u;              /* SW_COLLAPSE set when off */
	for (unsigned i = 0; i < 7; i++) {
		*mock_reg(T_GCC_PA + cbcr_offs[i]) = powered ? 1u : (1u << 31);
	}
	DWC(0xc120) = 0x5533290a;                       /* GSNPSID */
	DWC(0xc110) = gctl;
	DWC(0xc70c) = 1u << 22;                         /* halted */
	*mock_reg(T_QUSB2_PA + 0x38) = 0x20202020u;       /* PLL locked */
	*mock_reg(T_QUSB2_PA + 0xb4) = 0x22222222u;       /* powered */
}

int
main(void)
{
#if SCENARIO == 1
	setup(true, 0x00112000u);
	uint8_t *snap_gcc = malloc(0x94000), *snap_dwc = malloc(0xd000);
	uint8_t snap_qs[0x100], snap_phy[0x400];
	memcpy(snap_gcc, gcc_mem, 0x94000);
	memcpy(snap_dwc, dwc_mem, 0xd000);
	memcpy(snap_qs, qs_mem, sizeof(snap_qs));
	memcpy(snap_phy, phy_mem, sizeof(snap_phy));
	mock_bootarg_sweetusb = 1;
	sweet_usb_probe();
	CHECK(memcmp(snap_gcc, gcc_mem, 0x94000) == 0, "sweetusb=1 wrote a GCC register");
	CHECK(memcmp(snap_dwc, dwc_mem, 0xd000) == 0, "sweetusb=1 wrote a DWC3 register");
	CHECK(memcmp(snap_qs, qs_mem, sizeof(snap_qs)) == 0, "sweetusb=1 wrote QSCRATCH");
	CHECK(memcmp(snap_phy, phy_mem, sizeof(snap_phy)) == 0, "sweetusb=1 wrote the PHY");
	CHECK(mock_find(T_DWC3_PA, 4)->maps == 1, "DWC3 not read");
	CHECK(mock_find(T_QUSB2_PA, 4)->maps == 1, "QUSB2 not read");
	CHECK(mock_nthreads == 0, "sweetusb=1 started threads");
	sweet_usb_probe();
	CHECK(mock_find(T_GCC_PA, 4)->maps == 1, "second probe ran again");
#elif SCENARIO == 2
	setup(false, 0x00113000u);                      /* PrtCapDir = 3 (OTG) */
	mock_bootarg_sweetusb = 3;
	sweet_usb_probe();
	CHECK(GDSCR & (1u << 31), "GDSC not powered (0x%08x)", GDSCR);
	CHECK(!(GDSCR & 1u), "SW_COLLAPSE still set");
	for (unsigned i = 0; i < 7; i++) {
		uint32_t c = *mock_reg(T_GCC_PA + cbcr_offs[i]);
		CHECK((c & 1u) && !(c & (1u << 31)), "clock 0x%x not running (0x%08x)", cbcr_offs[i], c);
	}
	CHECK(((DWC(0xc110) >> 12) & 3u) == 2u, "GCTL not in device mode (0x%08x)", DWC(0xc110));
	CHECK((DWC(0xc110) & ~(3u << 12)) == (0x00113000u & ~(3u << 12)), "other GCTL bits changed");
	CHECK(*mock_reg(T_QS_PA + 0x10) & ((1u << 20) | (1u << 28)), "VBUS override not set");
	CHECK(*mock_reg(T_QS_PA + 0x30) & (1u << 24), "LANE0_PWR_PRESENT not set");
	CHECK(DWC(0xc704) & (1u << 31), "RUN_STOP not set");
	CHECK(!(DWC(0xc70c) & (1u << 22)), "controller still halted");
	uint64_t evt = ((uint64_t)DWC(0xc404) << 32) | DWC(0xc400);
	CHECK(evt == ptoa_64(DMA_PN), "event buffer 0x%llx, want 0x%llx",
	    (unsigned long long)evt, (unsigned long long)ptoa_64(DMA_PN));
	CHECK(DWC(0xc720) == 0x3, "DALEPENA 0x%x", DWC(0xc720));
	CHECK(ifnet_attached == 1 && ifaddr_set == 1, "ue0 not created/configured");
	CHECK(mock_nthreads == 2, "threads %u, want event + live-log", mock_nthreads);
#elif SCENARIO == 3
	setup(false, 0x00112000u);
	uint8_t *snap_gcc = malloc(0x94000);
	memcpy(snap_gcc, gcc_mem, 0x94000);
	mock_bootarg_sweetusb = 1;
	sweet_usb_probe();
	CHECK(memcmp(snap_gcc, gcc_mem, 0x94000) == 0, "sweetusb=1 wrote a GCC register");
	CHECK(mock_find(T_DWC3_PA, 4)->maps == 0, "unclocked DWC3 was mapped");
	CHECK(mock_find(T_QUSB2_PA, 4)->maps == 0, "PHY mapped with the controller off");
#elif SCENARIO == 7
	/* 0213: CDC-NCM function (default on LAVENDER), driven like Windows UsbNcm */
	setup(true, 0x00102000u);
	PHY(0x38) = 0x10101010u;
	PHY(0xb4) = 0x23232323u;
	mock_bootarg_sweetusb = 2;
	sweet_usb_probe();
	CHECK(ud.ncm, "NCM not selected by default on LAVENDER");
	CHECK(DWC(0xc704) & (1u << 31), "RUN_STOP not set");
	uint8_t *data = (uint8_t *)(ud.dma + UD_DATA);
	uint32_t n = host_setup(0x80, 6, 0x0200, 0, 255);
	CHECK(n == 86 && data[2] == 86 && data[4] == 2, "config descriptor len %u", n);
	CHECK(data[9 + 5] == 0x02 && data[9 + 6] == 0x0d, "comm interface not CDC NCM (%02x/%02x)", data[14], data[15]);
	CHECK(data[41] == 6 && data[42] == 0x24 && data[43] == 0x1a, "NCM functional descriptor missing");
	CHECK(data[54 + 5] == 0x0a && data[54 + 7] == 0x01 && data[63 + 7] == 0x01, "data interface protocol not NTB");
	n = host_setup(0x80, 6, 0x0302, 0x0409, 255);
	CHECK(n == 2 + 2 * 19 && data[2] == 'S', "product string len %u", n);
	n = host_setup(0xa1, 0x80, 0, 0, 28);
	CHECK(n == 28 && data[0] == 28 && data[2] == 1 && data[4] == 0x00 && data[5] == 0x08 && data[8] == 4 &&
	    data[16] == 0x00 && data[17] == 0x08, "GET_NTB_PARAMETERS");
	host_setup(0x21, 0x84, 0, 0, 0);
	CHECK(ud.state == EP0_WAIT_STATUS2, "SET_NTB_FORMAT(16) not accepted");
	host_setup(0x21, 0x84, 1, 0, 0);
	CHECK(ud.state == EP0_SETUP, "SET_NTB_FORMAT(32) not stalled");
	host_setup(0x00, 9, 1, 0, 0);
	CHECK(ud.configured, "SET_CONFIGURATION(1) did not configure");
	host_setup(0x01, 11, 1, 1, 0);
	uint8_t *note = (uint8_t *)(ud.dma + UD_NOTIFY);
	CHECK(ud.notify_stage == 1 && note[1] == 0x2a, "first notification 0x%02x, want SPEED_CHANGE", note[1]);
	ud_event((EP_INT << 1) | (1u << 6));
	CHECK(ud.notify_stage == 2 && note[1] == 0x00 && note[2] == 1, "second notification not NETWORK_CONNECTION");

	/* OUT NTB16 with two datagrams (64 and 90 bytes) */
	uint8_t *b = (uint8_t *)(ud.dma + UD_RX);
	memset(b, 0, 0x800);
	put32(b, 0x484d434eu); put16(b + 4, 12); put16(b + 6, 7); put16(b + 8, 32 + 64 + 90); put16(b + 10, 12);
	put32(b + 12, 0x304d434eu); put16(b + 16, 20); put16(b + 18, 0);
	put16(b + 20, 32); put16(b + 22, 64); put16(b + 24, 96); put16(b + 26, 90); put16(b + 28, 0); put16(b + 30, 0);
	b[32 + 14] = 0x45; b[96 + 14] = 0x46;
	volatile uint32_t *rt = (volatile uint32_t *)(ud.dma + UD_TRB_RX);
	rt[2] = 0x800 - (32 + 64 + 90);
	ud_event((EP_RX << 1) | (1u << 6));
	CHECK(rx_count == 2 && rx_len[0] == 64 && rx_len[1] == 90 && rx_first[0] == 0x45 && rx_first[1] == 0x46,
	    "NTB rx: %u frames (%zu, %zu)", rx_count, rx_len[0], rx_len[1]);
	CHECK(ud.xfer_active[EP_RX], "RX not re-armed");
	/* malformed: datagram past the block */
	put16(b + 22, 1000);
	rt[2] = 0x800 - (32 + 64 + 90);
	ud_event((EP_RX << 1) | (1u << 6));
	CHECK(ud.ntb_bad == 1 && rx_count == 2, "malformed NTB accepted (bad %u, rx %u)", ud.ntb_bad, rx_count);

	/* IN: one 100-byte frame, then a 484-byte one (28 + 484 = 512 -> padded) */
	mbuf_t m = NULL;
	mbuf_gethdr(0, 0, &m);
	memset(m->buf, 0xab, 100); m->len = m->pktlen = 100;
	ud_if_output(ud.ifp, m);
	uint8_t *tx = (uint8_t *)(ud.dma + UD_TX);
	volatile uint32_t *tt = (volatile uint32_t *)(ud.dma + UD_TRB_TX);
	CHECK(ud_rd32(tx) == 0x484d434eu && ud_rd16(tx + 4) == 12 && ud_rd16(tx + 8) == 128 && ud_rd16(tx + 10) == 12,
	    "NTH16 %08x len %u", ud_rd32(tx), ud_rd16(tx + 8));
	CHECK(ud_rd32(tx + 12) == 0x304d434eu && ud_rd16(tx + 16) == 16 && ud_rd16(tx + 20) == 28 &&
	    ud_rd16(tx + 22) == 100 && ud_rd16(tx + 24) == 0 && tx[28] == 0xab, "NDP16");
	CHECK((tt[2] & 0xffffff) == 128, "TX TRB length %u", tt[2] & 0xffffff);
	ud_event((EP_TX << 1) | (1u << 6));
	mbuf_gethdr(0, 0, &m);
	m->len = m->pktlen = 484;
	ud_if_output(ud.ifp, m);
	CHECK(ud_rd16(tx + 8) == 513 && (tt[2] & 0xffffff) == 513 && ud_rd16(tx + 6) == 1, "512-byte NTB not padded");
	ud_event((EP_TX << 1) | (1u << 6));

	/* host reset: the function must come back unconfigured, ep0 in SETUP */
	ud_event(0x00000101);
	CHECK(!ud.configured && ud.resets >= 1 && (DWC(0xc720) & ~3u) == 0 && ud.state == EP0_SETUP,
	    "bus reset: configured %d resets %u DALEPENA 0x%x state %d", ud.configured, ud.resets, DWC(0xc720), ud.state);
	host_setup(0x00, 9, 1, 0, 0);
	CHECK(ud.configured, "not reconfigured after a bus reset");
	CHECK(mbufs_live == 0, "%u mbufs leaked", mbufs_live);
#elif SCENARIO == 6
	setup(true, 0x00102000u);
	PHY(0x38) = 0x10101010u;
	PHY(0xb4) = 0x23232323u;
	DWC(0xc200) = 0x00002500u;                      /* GUSB2PHYCFG as logged */
	mock_bootarg_sweetusb = 2;
	mock_bootarg_usbfix = 0;
	sweet_usb_probe();
	CHECK(PHY(0x38) & (1u << 5), "PLL not locked");
	CHECK(!(DWC(0xc704) & (1u << 31)), "RUN_STOP set although SETEPCONFIG hung");
	CHECK(DWC(0xc80c) & (1u << 10), "ep0 SETEPCONFIG did not hang in the model");
	CHECK((*mock_reg(T_QS_PA + 0x08) & 0x109u) == 0, "GENERAL_CFG touched with sweetusbfix=0");
#elif SCENARIO == 4 || SCENARIO == 5
	setup(true, 0x00102000u);                       /* as logged: device mode, halted */
	PHY(0x38) = 0x10101010u;                        /* PLL_STATUS without lock */
	PHY(0x80) = 0xf8f8f8f8u;
	PHY(0xb4) = 0x23232323u;                        /* CLAMP_N_EN|FREEZIO_N|POWER_DOWN */
	PHY(0xc4) = 0x40404040u;                        /* UTMI_CTRL2: TEST_MUX_SEL, as logged */
	TCSR_CLAMP = 0;
	pll_can_lock = (SCENARIO == 4);
	mock_bootarg_sweetusb = 2;
	sweet_usb_probe();
	CHECK(TCSR_CLAMP == 1, "TCSR clamp not released (0x%08x)", TCSR_CLAMP);
	CHECK((PHY(0xb4) & 0xffu) == 0x22, "POWERDOWN 0x%02x, want 0x22", PHY(0xb4) & 0xffu);
	CHECK((PHY(0x80) & 0xffu) == 0xf8, "PORT_TUNE1 changed");
	CHECK((PHY(0xc4) & 0xffu) == 0, "UTMI_CTRL2 0x%02x: test mux not released (0212)", PHY(0xc4) & 0xffu);
#if SCENARIO == 4
	CHECK(DWC(0xc704) & (1u << 31), "RUN_STOP not set");
	CHECK(mock_nthreads == 2, "threads %u, want event + live-log", mock_nthreads);
#else
	/* no PHY clock: SETEPCONFIG hangs even after 0211, nothing is run */
	CHECK(!(DWC(0xc704) & (1u << 31)), "RUN_STOP set without a PHY clock");
	CHECK(mock_nthreads == 0, "threads %u started without a PHY clock", mock_nthreads);
#endif
	/* 0211 (default mask 7) */
	CHECK((*mock_reg(T_QS_PA + 0x08) & 0x109u) == 0x009u, "GENERAL_CFG 0x%08x: want UTMI as PIPE, CLK_DIS clear",
	    *mock_reg(T_QS_PA + 0x08));
	CHECK((DWC(0xc11c) & ((1u << 17) | (1u << 26))) == ((1u << 17) | (1u << 26)), "GUCTL1 quirks 0x%08x", DWC(0xc11c));
	CHECK(!(DWC(0xc200) & ((1u << 6) | (1u << 8) | (1u << 31))), "GUSB2PHYCFG 0x%08x", DWC(0xc200));
	CHECK(!(DWC(0xc2c0) & (1u << 31)), "GUSB3PIPECTL still in PHYSOFTRST");
	CHECK(!(DWC(0xc110) & (1u << 11)) && ((DWC(0xc110) >> 12) & 3u) == 2u && !(DWC(0xc110) & (3u << 6)),
	    "GCTL 0x%08x after core reset", DWC(0xc110));
	CHECK(!(DWC(0xc704) & ((1u << 10) | (1u << 12))), "INITU1/U2ENA still set");
#if SCENARIO == 4
	CHECK(PHY(0x38) & (1u << 5), "PLL not locked");
	CHECK(rs_after_lock, "RUN_STOP was set before the PLL locked");
	mock_bootarg_usbhold = 1;
	block_limit = 3;
	if (setjmp(hold_exit) == 0) {
		sweet_usb_hold();
		CHECK(0, "sweet_usb_hold returned with sweetusbhold=1");
	}
	CHECK(blocks == 3, "hold loop blocked %u times", blocks);
#else
	CHECK(!(PHY(0x38) & (1u << 5)), "PLL locked without supplies?");
	mock_bootarg_usbhold = 0;
	block_limit = 1;
	if (setjmp(hold_exit) == 0) {
		sweet_usb_hold();               /* must return without blocking */
	} else {
		CHECK(0, "sweetusbhold=0 blocked");
	}
#endif
#endif
	if (mock_failures) {
		fprintf(stderr, "test_lavender_usb scenario %d: %u failure(s)\n", SCENARIO, mock_failures);
		return 1;
	}
	printf("test_lavender_usb scenario %d: OK\n", SCENARIO);
	return 0;
}
