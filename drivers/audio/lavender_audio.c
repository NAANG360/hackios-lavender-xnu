/*
 * lavender: audio platform probe, A0..A6. Boot-arg gated.
 *
 * Device log (0505 kernel, lavaud=3): the LPASS ADSP GDSC is collapsed
 * (`0x00000001`) and voting the SMMU branch first leaves `80000001` (EN, OFF);
 * 0507 fixes the order: GDSC first, then the SMMU branch. The same log exposed
 * two loader bugs fixed here (A6): the MDT hash segment is identified by
 * p_flags (phdr+24, bits 26:24 == 2), not by p_type and not by phdr+28, and a
 * PAS call can return success in x0 with a non-zero SCM status in x1. The
 * phone's hash table is the vendor 40-byte header plus 32-byte digests (opaque
 * to the AP) and the MDT is header+phdrs+hash even though the hash phdr's
 * p_offset says otherwise, so INIT_IMAGE gets that block assembled explicitly.
 * The image also ends in a pure-BSS phdr (filesz 0, memsz 0x23a000) that the
 * old walk skipped; it is now loaded (zeroed) and included in MEM_SETUP.
 * Device log (0508 kernel, lavaud=4): INIT_IMAGE and MEM_SETUP both returned
 * 0, B02/B03 copied, and then the thread wedged writing B04 at the 0x92a20000
 * boundary while the rest of the system stayed up. The audio handoff warns
 * that only byte stores are safe on this carve-out, so the copy now uses
 * byte stores; lavaudprobe=1 also reads the carve-out first and prints the
 * read/copy point every 256 KiB, and lavaudcopy=0 skips the copy entirely to
 * test whether the bootloader/TZ left the firmware preloaded.
 * Device log (0508b kernel, lavaud=5 with lavaudcopy=0): AUTH_AND_RESET
 * returned 0/0 and the ADSP started (Q6SS out of reset, SMP2P 443 and GLINK
 * 478/480 published). The next boot (lavaud=6, lavaudforce=1, copy still off)
 * got `AUTH_AND_RESET ret 0xffffffc3`: the running ADSP had modified the
 * carve-out (0x1df0000 changed from 00000000 to ffffffff) and TrustZone
 * refused the stale image. A copy is therefore required on every boot after
 * the first, but the copy path then wedged in flush_dcache64() after B22;
 * 0508c drops that flush (the carve-out is Device/VM_WIMG_IO and the byte
 * stores bypass the cache).
 *
 *   lavaud=1        run the read-only probe (A0)
 *   lavaud=2        also power up the LPASS ADSP domain if the bootloader
 *                   left it off, then read the Q6SS (A1)
 *   lavaud=3        also read adsp.mdt from the modem partition's FAT16,
 *                   parse its ELF/hash table and check every segment blob
 *                   against the ADSP carve-out (A2, read only)
 *   lavaud=4        also load the firmware through TrustZone PAS (A3):
 *                   INIT_IMAGE, MEM_SETUP, copy the segments, AUTH_AND_RESET
 *   lavaud=5        also wait for the ADSP and inventory its SMEM/SMP2P items
 *                   and the GLINK fifo candidates (A4, read only)
 *   lavaud=6        also create the AP SMP2P item 429, kick the ADSP and wait
 *                   for item 443's slave-kernel READY (A5, first SMEM write)
 *   lavaud=7        also bring up GLINK lpass (descriptor 478, TX 479, RX 480),
 *                   open the apr_audio_svc channel and send the APR core query
 *   lavaud=8        also read the PM660L analog codec over SPMI (SID 3,
 *                   peripheral 0xf0) read-only
 *   lavaud=9        also test the guarded SPMI write path on the PM660L DIG
 *                   spare register 0xf070 (write/read-back/restore)
 *   lavaud=10       also configure and start the AFE INT4_MI2S_RX port 0x1036
 *                   over APR (I2S 48 kHz/16-bit/SD0, then DEVICE_START)
 *   lavaud=11       also open an ADM playback device path (COPP) to 0x1036
 *   lavaud=12       also open the ASM stream, route it to the COPP, set the
 *                   media format and run it
 *   lavaudforce=1   A3: load even if SMEM shows an ADSP from an earlier run
 *   lavaudcopy=0    A3: skip the segment copy (test a preloaded carve-out)
 *   lavaudprobe=1   A3: read the carve-out and print copy progress every 256K
 *   lavauddelay=S   seconds to wait after boot before it (default 25), so the
 *                   console ring flusher and the USB live log are up
 *
 * The phone does NOT have a WCD9335: the live DTBO disables the SLIMbus audio
 * controller ("slim_aud") and the enabled sound card is qcom,sdm660-asoc-snd
 * with the PM660L analog codec (SPMI 0xf000) plus the LPASS digital codec
 * (0x152c0000) and its SoundWire master (0x152c1000). The DSP owns the data
 * path; before writing any of it this probe answers the questions the ladder
 * needs (work/briefs/audio-sdm660-brief.md):
 *
 *   - did the bootloader leave the LPASS ADSP power domain and the SMMU vote
 *     on (GCC 0x7d034 / 0x7d014)?
 *   - if the domain is on: what does the ADSP Q6SS (LPASS 0x15700000) say?
 *     reset, clock mux, power and BHS state (the DSP may already be running)
 *   - what does the LPASS Q6 SMMU (0x05100000) have mapped?
 *   - what do the LPASS digital codec, AVTIMER, MCLK and LPI pinctrl show?
 *   - is the SMEM TOC intact and does the apps<->ADSP partition still hold an
 *     item from an earlier run? The vendor smem.h for lavender fixes the
 *     SMP2P ids (apps->adsp 429, adsp->apps 443) and GLINK 478/479/480; the
 *     dump confirms them and shows the v1 packed header layout.
 *   - what does the APCS IPC register (0x17911008) show for the ADSP bits?
 *
 * It READS ONLY at lavaud=1 and lavaud=3. A clock-gated LPASS block is not
 * touched: if the ADSP domain is off, the Q6SS, codec and SMMU sections are
 * skipped instead of risking a bus hang. lavaud=2 clears the LPASS ADSP GDSC's
 * SW_COLLAPSE (after voting the LPASS SMMU clock) and re-reads the Q6SS; if the
 * domain does not come on it is put back and the rest is skipped. lavaud=3
 * adds the ADSP firmware dry run from the modem FAT16: the MDT is parsed and
 * every segment is checked against the carve-out, but nothing is written and
 * TrustZone is not called. Every access is announced 2 s in advance so the log
 * leaves the machine even if the access kills it.
 *
 * Register meanings: Linux gcc-sdm660.c, subsys-pil-tz.c and pil-q6v5.c, the
 * SDM660 DTB and the SMEM partition layout already used by the topaz port,
 * read as behaviour references (values only, no code copied).
 */
#if defined(ARM64_BOARD_CONFIG_LAVENDER)

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <kern/clock.h>
#include <kern/thread.h>
#include <kern/sched_prim.h>
#include <machine/machine_routines.h>
#include <pexpert/pexpert.h>
#include <libkern/libkern.h>
#include <vm/memory_types.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>
#include <arm/caches_internal.h>

extern vm_offset_t ml_io_map(vm_offset_t, vm_size_t);
/* osfmk/arm/machine_routines.h, not exported to the BSD layer: 0177 already
 * uses it for the framebuffer, because io_submap is only 8 MiB. */
extern vm_offset_t ml_io_map_unmappable(vm_offset_t, vm_size_t, uint32_t);
extern int santoni_emmc_read(uint64_t lba, uint32_t count, void *buf);
/* SPMI observer read path shared with SweetBattery (extended to SID 3). */
extern int sweet_spmi_init(void);
extern int sweet_spmi_read(unsigned sid, unsigned addr, uint8_t *out);
/* 0510: lavender arbiter v3 path (SID 3 = PM660L analog codec). */
extern int sweet_spmi_lavender_read(unsigned sid, unsigned addr,
    uint8_t *out);
extern int sweet_spmi_lavender_write(unsigned sid, unsigned addr,
    uint8_t val);
extern ppnum_t pmap_find_phys(pmap_t pmap, addr64_t va);

#define GCC_PA              0x00100000u
#define GCC_SIZE            0x94000u
#define Q6SS_PA             0x15700000u
#define Q6SS_SIZE           0x00001000u
#define LPASS_SMMU_PA       0x05100000u
#define LPASS_SMMU_SIZE     0x00040000u
#define LPASS_CODEC_PA      0x152c0000u
#define LPASS_CODEC_SIZE    0x00002000u
#define LPASS_PINCTRL_PA    0x15070000u
#define LPASS_PINCTRL_SIZE  0x00001000u
#define LPASS_MCLK_PA       0x15020000u
#define LPASS_MCLK_SIZE     0x00001000u
#define LPASS_AVTIMER_PA    0x150f7000u
#define LPASS_AVTIMER_SIZE  0x00001000u
#define SMEM_PA             0x86000000u
#define SMEM_SIZE           0x00200000u
#define APCS_IPC_PA         0x17911000u
#define APCS_IPC_SIZE       0x00001000u

/* The phone's modem partition is FAT16 at GPT LBA 786432 (same layout the
 * modem driver validates: 512 B sectors, 32 sectors/cluster, 1 reserved,
 * 2 FATs, 512 root entries, 49 FAT sectors). */
#define MODEM_LBA           786432ull
#define FAT_SECTOR          512u
#define FAT_SPC             32u
#define FAT_ROOT_ENTRIES    512u
#define FAT_SIZE_SECTORS    49u
#define FAT_CLUSTER_BYTES   (FAT_SPC * FAT_SECTOR)
#define FAT_ROOT_LBA        (MODEM_LBA + 1 + 2 * FAT_SIZE_SECTORS)
#define FAT_DATA_LBA        (FAT_ROOT_LBA + \
    (FAT_ROOT_ENTRIES * 32 / FAT_SECTOR))
#define FAT_MAX_CLUSTER     0xfff8u
#define ADSP_CARVE_PA       0x92a00000u
#define ADSP_CARVE_SIZE     0x01e00000u
#define ADSP_MDT_MAX        (256u << 10)
#define ADSP_HASH_MAX       (32u << 10)

/* ELF32 program header fields the MDT shares with a normal ELF (vendor
 * peripheral-loader.c: segment_is_hash()/segment_is_loadable()). */
#define ELF_PT_LOAD         1u
#define MDT_TYPE_MASK       (7u << 24)
#define MDT_TYPE_HASH       (2u << 24)
#define MDT_RELOCATABLE     (1u << 27)

/* TrustZone PAS (same command numbers as the GPU zap path, 0241). */
#define SCM_FN(svc, cmd)    (0x42000000u | ((uint32_t)(svc) << 8) | \
    (uint32_t)(cmd))
#define SCM_SVC_PIL         0x02u
#define PIL_INIT_IMAGE      0x01u
#define PIL_MEM_SETUP       0x02u
#define PIL_AUTH_RESET      0x05u
#define SCM_ARG_RW          2u
#define PAS_ID_ADSP         1u

struct elf32_phdr {
	uint32_t p_type, p_offset, p_vaddr, p_paddr;
	uint32_t p_filesz, p_memsz, p_flags, p_align;
} __attribute__((packed));

/* GCC byte offsets from GCC_PA (gcc-sdm660.c) */
#define GCC_LPASS_SMMU_CBCR     0x7d014u  /* hlos1_vote_lpass_adsp_smmu_clk */
#define GCC_LPASS_ADSP_GDSCR    0x7d034u  /* hlos1_vote_lpass_adsp_gdsc */
#define CBCR_EN                 (1u << 0)
#define CBCR_OFF                (1u << 31)
#define GDSC_PWR_ON             (1u << 31)
#define GDSC_SW_COLLAPSE        (1u << 0)

/* QDSP6SS byte offsets from Q6SS_PA (pil-q6v5.c) */
#define Q6SS_RESET              0x014u
#define Q6SS_GFMUX_CTL          0x020u
#define Q6SS_PWR_CTL            0x030u
#define Q6SS_MEM_PWR_CTL        0x034u
#define Q6SS_XO_CBCR            0x038u
#define Q6SS_SLEEP              0x03cu
#define Q6SS_BHS_STATUS         0x078u
#define Q6SS_MEM_PWR_CTL2       0x0b0u
#define Q6V62_BHS_STATUS        0x0c4u

/* SMMUv2 GR0 offsets (arm-smmu.c) */
#define SMMU_SCR0               0x0000u
#define SMMU_SACR               0x0010u
#define SMMU_IDR0               0x0020u
#define SMMU_IDR1               0x0024u
#define SMMU_IDR7               0x003cu
#define SMMU_SGFAR              0x0040u
#define SMMU_SGFSR              0x0048u
#define SMMU_SMR(n)             (0x0800u + ((n) << 2))
#define SMMU_S2CR(n)            (0x0c00u + ((n) << 2))
#define SMMU_SMR_VALID          (1u << 31)
#define SMMU_MAX_SMRS           128

/* SMEM v12 layout, as already used by the topaz port (TopazModem.cpp). */
#define SMEM_TOC_OFF            (SMEM_SIZE - 0x1000u)
#define SMEM_TOC_ENTRIES        64u
#define SMEM_ADSP_PID           2u
#define SMEM_APPS_HOST          0u
#define SMEM_GLOBAL_HOST        0xfffeu
#define SMEM_ITEM_CANARY        0xa5a5u
#define SMP2P_MAGIC             0x504d5324u     /* "$SMP" */
#define SMP2P_MAX_ENTRY         16u

struct smem_toc_entry {
	uint32_t offset, size, flags;
	uint16_t host0, host1;
	uint32_t cacheline, rsvd[7];
} __attribute__((packed));

struct smem_toc {
	uint8_t  magic[4];
	uint32_t version, num_entries, rsvd[5];
} __attribute__((packed));

struct smem_part {
	uint8_t  magic[4];
	uint16_t host0, host1;
	uint32_t size, free_uncached, free_cached, rsvd[3];
} __attribute__((packed));

struct smem_item {
	uint16_t canary, item;
	uint32_t size;
	uint16_t pad_data, pad_hdr;
	uint32_t rsvd;
} __attribute__((packed));

/*
 * SDM660 SMP2P v1 item header (vendor smp2p_private.h). Note that this is NOT
 * the SM6225/AudioReach layout topaz uses: the three fields after the magic are
 * packed bitfields, not separate u16s. The next fields are 16 entries of
 * { char name[16]; u32 value; }.
 */
struct smp2p_hdr {
	uint32_t magic;
	uint32_t feature_version;   /* version(7:0) | features(31:8) */
	uint32_t rem_loc_proc_id;   /* remote(31:16) | local(15:0) */
	uint32_t valid_total_ent;   /* valid(31:16) | total(15:0) */
	uint32_t flags;
} __attribute__((packed));

struct smp2p_entry {
	char     name[16];
	uint32_t value;
} __attribute__((packed));

/* Vendor smem.h (lavender-q-oss) fixed ids for this SoC. */
#define SMEM_SMP2P_APPS_BASE    427u
#define SMEM_SMP2P_AUDIO_BASE   443u
#define SMEM_ITEM_SMP2P_OUT     (SMEM_SMP2P_APPS_BASE + SMEM_ADSP_PID) /* 429 */
#define SMEM_ITEM_SMP2P_IN      (SMEM_SMP2P_AUDIO_BASE + SMEM_APPS_HOST) /* 443 */
#define SMEM_ITEM_GLINK_DESC    478u
#define SMEM_ITEM_GLINK_FIFO0   479u
#define SMEM_ITEM_GLINK_FIFO1   480u

/* --- GLINK lpass (SMEM native) + APR, F3 ------------------------------- */
#define GLINK_TX_SIZE       0x4000u
#define GLINK_RX_MAX        0x4000u
#define GLINK_IRQ_BIT       9
#define GLINK_FEATURES      1u
#define GLINK_CHAN_NAME     "apr_audio_svc"
#define GLINK_LCID          1u
#define GLINK_OUR_INTENTS   8u
#define GLINK_MAX_INTENTS   8u
#define GLINK_INTENT_SIZE   0x1000u
#define GLINK_DATA_MAX      512u

#define GCMD_VERSION           0
#define GCMD_VERSION_ACK       1
#define GCMD_OPEN              2
#define GCMD_CLOSE             3
#define GCMD_OPEN_ACK          4
#define GCMD_INTENT            5
#define GCMD_RX_DONE           6
#define GCMD_RX_INTENT_REQ     7
#define GCMD_RX_INTENT_REQ_ACK 8
#define GCMD_TX_DATA           9
#define GCMD_CLOSE_ACK         11
#define GCMD_TX_DATA_CONT      12
#define GCMD_READ_NOTIF        13
#define GCMD_RX_DONE_W_REUSE   14
#define GCMD_SIGNALS           15

/* APR core service query: GET_FWK_VERSION (apr_audio-v2.h). */
#define APR_SVC_ADSP_CORE      0x3u
#define APR_DOMAIN_APPS        0x5u
#define APR_DOMAIN_ADSP        0x4u
#define APR_CMD_GET_FWK_VERSION    0x0001292Cu
#define APR_RSP_GET_FWK_VERSION    0x0001292Du

/* AFE service, F5: INT4 MI2S RX on the SDM660 sound card. */
#define APR_SVC_AFE                    4u
#define AFE_PORT_CMD_SET_PARAM_V2      0x000100EFu
#define AFE_PORT_CMD_DEVICE_START      0x000100E5u
#define AFE_SVC_CMD_SET_PARAM          0x000100F3u
#define AFE_MODULE_AUDIO_DEV_INTERFACE 0x0001020Cu
#define AFE_PARAM_ID_I2S_CONFIG        0x0001020Du
#define AFE_MODULE_CLOCK_SET           0x0001028Fu
#define AFE_PARAM_ID_CLOCK_SET         0x00010290u
#define AFE_CLK_INT4_MI2S_IBIT         0x00000113u
#define AFE_PORT_INT4_MI2S_RX          0x1036u

/* ADM, F6: the playback device path (COPP) to the AFE port. */
#define APR_SVC_ASM                    7u
#define APR_SVC_ADM                    8u
#define ADM_CMD_DEVICE_OPEN_V5         0x00010326u
#define ADM_CMDRSP_DEVICE_OPEN_V5      0x00010329u
#define ADM_PATH_PLAYBACK              1u
#define ADM_TOPOLOGY_NONE              0x00010312u
#define ADM_CMD_MATRIX_MAP_ROUTINGS_V5 0x00010325u
#define ADM_MATRIX_AUDIO_RX            0u
#define ASM_STREAM_CMD_OPEN_WRITE_V3   0x00010DB3u
#define ASM_DATA_CMD_MEDIA_FMT_UPDATE_V2 0x00010D98u
#define ASM_SESSION_CMD_RUN_V2         0x00010DAAu
#define ASM_TOPOLOGY_NONE              0x00010C68u
#define ASM_MEDIA_FMT_MULTI_CHANNEL_PCM_V2 0x00010DA5u
#define STREAM_PORT                    0x0101u

struct glink_msg {
	uint16_t cmd, p1;
	uint32_t p2;
} __attribute__((packed));

struct glink_data_hdr {
	struct glink_msg m;
	uint32_t chunk, left;
} __attribute__((packed));

struct glink_intent {
	uint32_t id, size;
	int used;
};

struct apr_hdr {
	uint16_t hdr_field, pkt_size;
	uint8_t src_svc, src_domain;
	uint16_t src_port;
	uint8_t dest_svc, dest_domain;
	uint16_t dest_port;
	uint32_t token, opcode;
} __attribute__((packed));

/* AFE_PARAM_ID_I2S_CONFIG payload (apr_audio-v2.h). */
struct afe_i2s_cfg {
	uint32_t version;
	uint16_t bit_width, channel_mode, mono_stereo, ws_src;
	uint32_t sample_rate;
	uint16_t data_format, reserved;
} __attribute__((packed));

/* AFE_PARAM_ID_CLOCK_SET payload. */
struct afe_clock_set {
	uint32_t version;
	uint32_t id;
	uint32_t hz;
	uint16_t attribute, root;
	uint32_t enable;
} __attribute__((packed));

/* SMEM's remote spinlock: TCSR mutex, lock 3 (vendor smem.c/hwspinlock). */
#define TCSR_MUTEX_PA           0x01f40000u
#define TCSR_MUTEX_SIZE         0x00004000u
#define SMEM_HWLOCK             3u
#define HWLOCK_APPS_ID          1u

static vm_offset_t gcc, q6ss, smmu, codec, pinctrl, mclk, avtimer, smem, apcs;
static vm_offset_t tcsr;
static int lpass_on;
static uint32_t audio_step;
static int audio_chan;
static uint8_t *fat_fat, *fat_root, *fat_image, *fat_bounce, *adsp_mdt;
static uint8_t *adsp_hash;
static uint32_t fat_image_len;
static int fat_ready;
static int audio_force;
static int audio_copy = 1;
static int audio_probe;
/* The ADSP's item 443 is cached before the 429 kick; after the kick the
 * partition scan can stall, and the item does not move once allocated. */
static uint8_t *smp2p_in;
static uint32_t smp2p_in_len;
static uint8_t *glink_desc, *glink_tx, *glink_rx;
static uint32_t glink_rx_len;
static uint16_t glink_rcid;
static int glink_ver_acked, glink_open_sent, glink_open_acked;
static int glink_intents_sent;
static uint32_t glink_next_intent = 1;
static struct glink_intent glink_ri[GLINK_MAX_INTENTS];
static unsigned glink_ri_count;
static unsigned glink_tx_msgs, glink_rx_msgs;
static unsigned glink_intent_log;
static uint32_t adm_status, adm_copp;

static inline uint32_t
audio_rd(vm_offset_t base, uint32_t off)
{
	uint32_t v = *(volatile uint32_t *)(base + off);

	__asm__ volatile("dsb ld" ::: "memory");
	return v;
}

static void
audio_wr(vm_offset_t base, uint32_t off, uint32_t val)
{
	*(volatile uint32_t *)(base + off) = val;
	__asm__ volatile("dsb sy" ::: "memory");
}

/*
 * The carve-out mapping is Device-nGnRnE. XNU's memset/memcpy store engine
 * emits STNP pairs, which are not safe on Device memory (the modem's 0247
 * lesson). The audio handoff also warns that only byte stores are safe on
 * this carve-out: the 64-bit version wedged the thread at the 0x92a20000
 * boundary (B02/B03 copied, B04 did not). Copy and zero one byte at a time.
 */
static void
audio_io_zero(vm_offset_t dst, uint32_t n)
{
	uint32_t i;

	for (i = 0; i < n; i++) {
		*(volatile uint8_t *)(dst + i) = 0;
	}
	__asm__ volatile("dsb sy" ::: "memory");
}

static void
audio_io_copy(vm_offset_t dst, const void *src, uint32_t n)
{
	const uint8_t *s = src;
	uint32_t i;

	for (i = 0; i < n; i++) {
		*(volatile uint8_t *)(dst + i) = s[i];
	}
	__asm__ volatile("dsb sy" ::: "memory");
}

static void
audio_wait(unsigned ms)
{
	uint64_t deadline;

	clock_interval_to_deadline(ms, NSEC_PER_MSEC, &deadline);
	(void)assert_wait_deadline(&audio_chan, THREAD_UNINT, deadline);
	thread_block(THREAD_CONTINUE_NULL);
}

/* Announce, then give the log 2 s to leave the machine. */
static void
audio_announce(const char *what)
{
	printf("SWEET AUDIO: about to %s\n", what);
	audio_wait(2000);
}

static void
audio_gcc(void)
{
	uint32_t cbcr, gdscr;

	audio_announce("read the GCC LPASS ADSP vote and power domain (read only)");
	cbcr = audio_rd(gcc, GCC_LPASS_SMMU_CBCR);
	gdscr = audio_rd(gcc, GCC_LPASS_ADSP_GDSCR);
	lpass_on = ((gdscr & GDSC_PWR_ON) != 0) && ((gdscr & GDSC_SW_COLLAPSE) == 0);
	printf("SWEET AUDIO: gcc lpass_smmu_cbcr %08x (en %u, %s)\n", cbcr,
	    cbcr & CBCR_EN, (cbcr & CBCR_OFF) ? "OFF" : "on");
	printf("SWEET AUDIO: gcc lpass_adsp_gdscr %08x (pwr_on %u, collapse %u)\n",
	    gdscr, (gdscr & GDSC_PWR_ON) != 0, (gdscr & GDSC_SW_COLLAPSE) != 0);
	printf("SWEET AUDIO: LPASS ADSP domain is %s\n", lpass_on ? "ON" : "OFF");
}

static int
audio_poll(vm_offset_t base, uint32_t off, uint32_t mask, uint32_t want,
    unsigned ms)
{
	for (unsigned i = 0; i < ms; i++) {
		if ((audio_rd(base, off) & mask) == want) {
			return 0;
		}
		audio_wait(1);
	}
	return ETIMEDOUT;
}

/*
 * A1: the LPASS ADSP power domain. The bootloader leaves it collapsed on some
 * boots; the GCC GDSC is the only LPASS power control the AP owns (the LPASS
 * internal clocks belong to the DSP). The SMMU vote clock is taken first
 * because the LPASS register window is not usable without it. If the GDSC
 * never reports PWR_ON, the collapse bit is put back and the caller skips
 * every read of the block.
 */
static int
audio_power_up(void)
{
	uint32_t v, gdscr;

	if (lpass_on) {
		printf("SWEET AUDIO: LPASS ADSP already on; nothing to write\n");
		return 0;
	}
	audio_announce("power up the LPASS ADSP domain (lavaud>=2)");

	/*
	 * Device result (0505 kernel): voting the SMMU branch first leaves it
	 * at 80000001 (EN set, OFF still set) because its GDSC is collapsed.
	 * Power the GDSC first, then vote the branch clock.
	 */
	gdscr = audio_rd(gcc, GCC_LPASS_ADSP_GDSCR);
	printf("SWEET AUDIO: adsp gdscr before %08x\n", gdscr);
	if (!(gdscr & GDSC_PWR_ON) || (gdscr & GDSC_SW_COLLAPSE)) {
		audio_wr(gcc, GCC_LPASS_ADSP_GDSCR, gdscr & ~GDSC_SW_COLLAPSE);
		if (audio_poll(gcc, GCC_LPASS_ADSP_GDSCR, GDSC_PWR_ON,
		    GDSC_PWR_ON, 200)) {
			gdscr = audio_rd(gcc, GCC_LPASS_ADSP_GDSCR);
			printf("SWEET AUDIO: adsp gdscr after %08x (STILL OFF; "
			    "CX rail or a voter is missing), putting the "
			    "collapse back\n", gdscr);
			audio_wr(gcc, GCC_LPASS_ADSP_GDSCR,
			    gdscr | GDSC_SW_COLLAPSE);
			return EIO;
		}
	}
	gdscr = audio_rd(gcc, GCC_LPASS_ADSP_GDSCR);
	printf("SWEET AUDIO: adsp gdscr after %08x (%s)\n", gdscr,
	    (gdscr & GDSC_PWR_ON) ? "on" : "still off");

	v = audio_rd(gcc, GCC_LPASS_SMMU_CBCR);
	printf("SWEET AUDIO: smmu cbcr before %08x\n", v);
	if (v & CBCR_OFF) {
		audio_wr(gcc, GCC_LPASS_SMMU_CBCR, v | CBCR_EN);
		if (audio_poll(gcc, GCC_LPASS_SMMU_CBCR, CBCR_OFF, 0, 100)) {
			v = audio_rd(gcc, GCC_LPASS_SMMU_CBCR);
			printf("SWEET AUDIO: smmu cbcr still off %08x\n", v);
		} else {
			printf("SWEET AUDIO: smmu cbcr after %08x (on)\n",
			    audio_rd(gcc, GCC_LPASS_SMMU_CBCR));
		}
	}
	gdscr = audio_rd(gcc, GCC_LPASS_ADSP_GDSCR);
	lpass_on = ((gdscr & GDSC_PWR_ON) != 0) &&
	    ((gdscr & GDSC_SW_COLLAPSE) == 0);
	return lpass_on ? 0 : EIO;
}

/* --- A2: adsp.mdt and the segment blobs in the modem FAT16 ---------------- */

static inline uint16_t
audio_le16(const uint8_t *p)
{
	return (uint16_t)(p[0] | (p[1] << 8));
}

static inline uint32_t
audio_le32(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
	    ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* The eMMC is shared with the root and the console flusher; retry like the
 * modem driver does instead of hammering the controller. */
static int
audio_emmc_read(uint64_t lba, uint32_t count, void *buf)
{
	int err = EIO;

	for (int attempt = 0; attempt < 5; attempt++) {
		err = santoni_emmc_read(lba, count, buf);
		if (err == 0) {
			return 0;
		}
		printf("SWEET AUDIO: eMMC read 0x%llx+%u failed %d (attempt "
		    "%d/5)\n", (unsigned long long)lba, count, err,
		    attempt + 1);
		audio_wait(100);
	}
	return err;
}

/* Build the 8.3 name of a FAT directory entry, without the trailing dot. */
static void
audio_fat_name(const uint8_t *e, char *out, size_t outlen)
{
	size_t n = 0;

	for (int i = 0; i < 8 && e[i] != ' '; i++) {
		if (n + 1 < outlen) {
			out[n++] = (char)e[i];
		}
	}
	if (e[8] != ' ' && n + 1 < outlen) {
		out[n++] = '.';
		for (int i = 8; i < 11 && e[i] != ' '; i++) {
			if (n + 1 < outlen) {
				out[n++] = (char)e[i];
			}
		}
	}
	out[n] = '\0';
}

static uint32_t
audio_fat_next(uint32_t cluster)
{
	if (cluster * 2 + 2 > 32u * 1024u) {
		return FAT_MAX_CLUSTER;
	}
	return audio_le16(fat_fat + cluster * 2);
}

static int
audio_fat_open(void)
{
	const uint8_t *b;
	uint32_t fat_bytes, cluster, off;
	int err;

	if (fat_ready) {
		return 0;
	}
	if (fat_fat == NULL) {
		if (kmem_alloc(kernel_map, (vm_offset_t *)&fat_fat,
		    32u * 1024u, KMA_DATA | KMA_ZERO,
		    VM_KERN_MEMORY_IOKIT) != KERN_SUCCESS ||
		    kmem_alloc(kernel_map, (vm_offset_t *)&fat_root,
		    FAT_CLUSTER_BYTES, KMA_DATA | KMA_ZERO,
		    VM_KERN_MEMORY_IOKIT) != KERN_SUCCESS ||
		    kmem_alloc(kernel_map, (vm_offset_t *)&fat_image,
		    FAT_CLUSTER_BYTES, KMA_DATA | KMA_ZERO,
		    VM_KERN_MEMORY_IOKIT) != KERN_SUCCESS ||
		    kmem_alloc(kernel_map, (vm_offset_t *)&fat_bounce,
		    FAT_CLUSTER_BYTES, KMA_DATA | KMA_ZERO,
		    VM_KERN_MEMORY_IOKIT) != KERN_SUCCESS) {
			printf("SWEET AUDIO: no memory for the FAT cache\n");
			return ENOMEM;
		}
	}
	audio_announce("read the modem FAT16 and its IMAGE/ directory");
	err = audio_emmc_read(MODEM_LBA, 1, fat_bounce);
	if (err) {
		return err;
	}
	b = fat_bounce;
	if (audio_le16(b + 0x0b) != FAT_SECTOR || b[0x0d] != FAT_SPC ||
	    audio_le16(b + 0x0e) != 1 || b[0x10] != 2 ||
	    audio_le16(b + 0x11) != FAT_ROOT_ENTRIES ||
	    audio_le16(b + 0x16) != FAT_SIZE_SECTORS) {
		printf("SWEET AUDIO: unexpected modem BPB (%u/%u/%u/%u/%u/%u)\n",
		    audio_le16(b + 0x0b), b[0x0d], audio_le16(b + 0x0e),
		    b[0x10], audio_le16(b + 0x11), audio_le16(b + 0x16));
		return EINVAL;
	}

	fat_bytes = FAT_SIZE_SECTORS * FAT_SECTOR;
	if (fat_bytes > 32u * 1024u) {
		return EINVAL;
	}
	err = audio_emmc_read(MODEM_LBA + 1, FAT_SIZE_SECTORS, fat_fat);
	if (err) {
		return err;
	}
	err = audio_emmc_read(FAT_ROOT_LBA, FAT_ROOT_ENTRIES * 32 / FAT_SECTOR,
	    fat_root);
	if (err) {
		return err;
	}

	cluster = 0;
	for (uint32_t i = 0; i < FAT_ROOT_ENTRIES; i++) {
		const uint8_t *e = fat_root + i * 32;
		char name[13];

		if (e[0] == 0x00) {
			break;
		}
		if (e[0] == 0xe5 || e[0x0b] == 0x0f) {
			continue;
		}
		audio_fat_name(e, name, sizeof(name));
		if (strcasecmp(name, "IMAGE") == 0 && (e[0x0b] & 0x10)) {
			cluster = audio_le16(e + 0x1a) |
			    (audio_le16(e + 0x14) << 16);
			break;
		}
	}
	if (cluster < 2 || cluster >= FAT_MAX_CLUSTER) {
		printf("SWEET AUDIO: IMAGE/ not found in the modem FAT\n");
		return ENOENT;
	}
	off = FAT_DATA_LBA + (cluster - 2) * FAT_SPC;
	err = audio_emmc_read(off, FAT_SPC, fat_image);
	if (err) {
		return err;
	}
	fat_image_len = FAT_CLUSTER_BYTES;
	fat_ready = 1;
	printf("SWEET AUDIO: modem FAT16 open, IMAGE/ cluster %u, %u bytes "
	    "cached\n", cluster, fat_image_len);
	return 0;
}

static int
audio_fat_lookup(const char *name, uint32_t *cluster, uint32_t *size)
{
	if (!fat_ready) {
		return ENOENT;
	}
	for (uint32_t i = 0; i < fat_image_len / 32; i++) {
		const uint8_t *e = fat_image + i * 32;
		char entry[13];

		if (e[0] == 0x00) {
			break;
		}
		if (e[0] == 0xe5 || e[0x0b] == 0x0f || (e[0x0b] & 0x10)) {
			continue;
		}
		audio_fat_name(e, entry, sizeof(entry));
		if (strcasecmp(entry, name) == 0) {
			*cluster = audio_le16(e + 0x1a) |
			    (audio_le16(e + 0x14) << 16);
			*size = audio_le32(e + 0x1c);
			return 0;
		}
	}
	return ENOENT;
}

static int
audio_fat_read(const char *name, void *dst, uint32_t max, uint32_t *out)
{
	uint32_t cluster, size, off = 0;
	int err;

	err = audio_fat_lookup(name, &cluster, &size);
	if (err) {
		printf("SWEET AUDIO: %s not found in IMAGE/\n", name);
		return err;
	}
	if (size > max) {
		printf("SWEET AUDIO: %s is %u bytes, max %u\n", name, size, max);
		return EINVAL;
	}
	while (off < size) {
		uint32_t n = MIN(size - off, FAT_CLUSTER_BYTES);

		err = audio_emmc_read(FAT_DATA_LBA + (cluster - 2) * FAT_SPC,
		    FAT_SPC, fat_bounce);
		if (err) {
			printf("SWEET AUDIO: %s read failed %d at %u\n", name,
			    err, off);
			return err;
		}
		memcpy((uint8_t *)dst + off, fat_bounce, n);
		off += n;
		if (off < size) {
			cluster = audio_fat_next(cluster);
			if (cluster < 2 || cluster >= FAT_MAX_CLUSTER) {
				printf("SWEET AUDIO: %s chain ends at %u/%u\n",
				    name, off, size);
				return EIO;
			}
		}
	}
	if (out != NULL) {
		*out = size;
	}
	return 0;
}

/* Read only the first n bytes of a blob: enough to prove the chain reads. */
static int
audio_fat_peek(const char *name, void *dst, uint32_t n, uint32_t *size)
{
	uint32_t cluster, fsize, off;
	int err;

	err = audio_fat_lookup(name, &cluster, &fsize);
	if (err) {
		printf("SWEET AUDIO: %s not found in IMAGE/\n", name);
		return err;
	}
	if (size != NULL) {
		*size = fsize;
	}
	if (fsize < n || n > FAT_CLUSTER_BYTES) {
		return EINVAL;
	}
	off = FAT_DATA_LBA + (cluster - 2) * FAT_SPC;
	err = audio_emmc_read(off, FAT_SPC, fat_bounce);
	if (err || !fat_ready) {
		return err ? err : EIO;
	}
	memcpy(dst, fat_bounce, n);
	return 0;
}

static int
audio_check_segment(const char *name, uint32_t paddr, uint32_t filesz,
    uint32_t memsz, unsigned *bad)
{
	uint8_t peek[8] = { 0 };
	uint32_t fat_size = 0;
	int err;

	if ((uint64_t)paddr < ADSP_CARVE_PA ||
	    (uint64_t)paddr + memsz > ADSP_CARVE_PA + ADSP_CARVE_SIZE ||
	    memsz < filesz) {
		printf("SWEET AUDIO: %s outside the carve-out: %08x+%08x "
		    "(file %u)\n", name, paddr, memsz, filesz);
		(*bad)++;
		return EINVAL;
	}
	if (filesz == 0) {
		printf("SWEET AUDIO: %-11s %08x memsz %06x bss (no blob)\n",
		    name, paddr, memsz);
		return 0;
	}
	err = audio_fat_peek(name, peek, sizeof(peek), &fat_size);
	if (err) {
		printf("SWEET AUDIO: %s: no readable blob (%d)\n", name, err);
		(*bad)++;
		return err;
	}
	printf("SWEET AUDIO: %-11s %08x memsz %06x file %06x fat %06x "
	    "first %08x\n", name, paddr, memsz, filesz, fat_size,
	    audio_le32(peek));
	if (fat_size != filesz) {
		printf("SWEET AUDIO: %s FAT size %u != filesz %u\n", name,
		    fat_size, filesz);
		(*bad)++;
		return EINVAL;
	}
	return 0;
}

/*
 * The MDT hash segment is a normal program header whose p_flags bits 26:24
 * are 2 (vendor peripheral-loader.c: segment_is_hash()). It is usually
 * PT_NULL, so looking only at p_type misses it; its p_flags live at phdr+24,
 * not phdr+28 (p_align). When p_offset escapes the MDT (the phone's
 * ADSP.MDT does this: the hash is a separate ADSP.Bnn blob), INIT_IMAGE
 * still needs the table physically after the program headers.
 */
struct audio_mdt_hash {
	uint32_t index;         /* program header index of the hash segment */
	uint32_t phoff;         /* ELF e_phoff */
	uint32_t phnum;         /* ELF e_phnum */
	uint32_t phdr_off;      /* hash phdr offset in the MDT */
	uint32_t table_off;     /* phoff + phnum*32: hash data when inline */
	uint32_t offset;        /* hash phdr p_offset (vendor, not trusted) */
	uint32_t length;        /* hash data length */
	int      external;      /* table is not the MDT tail */
};

static int
audio_mdt_hash(const uint8_t *mdt, uint32_t mdt_len,
    struct audio_mdt_hash *out)
{
	uint32_t phoff, phent, phnum, i;

	if (mdt_len < 0x34 || memcmp(mdt, "\177ELF", 4) != 0) {
		return EINVAL;
	}
	phoff = audio_le32(mdt + 28);
	phent = audio_le16(mdt + 42);
	phnum = audio_le16(mdt + 44);
	if (phent != 32 || phoff > mdt_len || phnum == 0 ||
	    phoff + (uint64_t)phnum * 32 > mdt_len) {
		return EINVAL;
	}
	for (i = 0; i < phnum; i++) {
		const uint8_t *ph = mdt + phoff + i * 32;

		if ((audio_le32(ph + 24) & MDT_TYPE_MASK) != MDT_TYPE_HASH) {
			continue;
		}
		out->index = i;
		out->phoff = phoff;
		out->phnum = phnum;
		out->phdr_off = phoff + i * 32;
		out->table_off = phoff + phnum * 32;
		out->offset = audio_le32(ph + 4);
		out->length = audio_le32(ph + 16);
		/* The vendor MDT is header+phdrs+hash even when the hash phdr's
		 * p_offset points somewhere else (p_offset is not trusted). */
		out->external = (uint64_t)out->table_off + out->length >
		    mdt_len;
		return 0;
	}
	return ENOENT;
}

static int
audio_mdt_hash_data(const uint8_t *mdt, uint32_t mdt_len,
    const struct audio_mdt_hash *h, const uint8_t **data)
{
	char name[16];
	uint32_t size = 0;
	int err;

	(void)mdt_len;
	if (!h->external) {
		*data = mdt + h->table_off;
		return 0;
	}
	if (h->length == 0 || h->length > ADSP_HASH_MAX) {
		printf("SWEET AUDIO: hash segment B%02u is %u bytes (max %u)\n",
		    h->index, h->length, ADSP_HASH_MAX);
		return EINVAL;
	}
	if (adsp_hash == NULL &&
	    kmem_alloc(kernel_map, (vm_offset_t *)&adsp_hash, ADSP_HASH_MAX,
	    KMA_DATA | KMA_ZERO, VM_KERN_MEMORY_IOKIT) != KERN_SUCCESS) {
		printf("SWEET AUDIO: no memory for the ADSP hash table\n");
		return ENOMEM;
	}
	snprintf(name, sizeof(name), "ADSP.B%02u", h->index);
	err = audio_fat_read(name, adsp_hash, ADSP_HASH_MAX, &size);
	if (err) {
		return err;
	}
	if (size != h->length) {
		printf("SWEET AUDIO: %s hash blob is %u bytes, phdr says %u\n",
		    name, size, h->length);
		return EINVAL;
	}
	*data = adsp_hash;
	return 0;
}

static int
audio_phdr_loadable(const uint8_t *ph)
{
	uint32_t flags = audio_le32(ph + 24);

	return audio_le32(ph) == ELF_PT_LOAD &&
	    (flags & MDT_TYPE_MASK) != MDT_TYPE_HASH &&
	    audio_le32(ph + 20) != 0;
}

/*
 * A2: read the ADSP metadata from the modem partition and validate it against
 * the reserved region. Nothing is written to the carve-out and TrustZone is
 * not called; this is the loader's dry run. The hash segment is only located
 * and validated here; its contents are opaque to the AP (the vendor table is
 * not the topaz 8+40*N format), and the blobs are named by phdr index, as
 * Linux does.
 */
static void
audio_firmware(void)
{
	const uint8_t *mdt, *hash = NULL;
	struct audio_mdt_hash mh;
	uint32_t mdt_len = 0, phoff, phnum, phent, i;
	uint32_t bad = 0, found = 0, reloc = 0;
	uint64_t total = 0;
	int err;

	memset(&mh, 0, sizeof(mh));
	if (audio_fat_open() != 0) {
		return;
	}
	if (adsp_mdt == NULL &&
	    kmem_alloc(kernel_map, (vm_offset_t *)&adsp_mdt, ADSP_MDT_MAX,
	    KMA_DATA | KMA_ZERO, VM_KERN_MEMORY_IOKIT) != KERN_SUCCESS) {
		printf("SWEET AUDIO: no memory for the ADSP MDT\n");
		return;
	}
	audio_announce("read ADSP.MDT from the modem FAT16");
	err = audio_fat_read("ADSP.MDT", adsp_mdt, ADSP_MDT_MAX, &mdt_len);
	if (err) {
		return;
	}
	mdt = adsp_mdt;
	if (mdt_len < 0x34 || memcmp(mdt, "\177ELF", 4) != 0) {
		printf("SWEET AUDIO: ADSP.MDT is not the expected ELF\n");
		return;
	}
	phoff = audio_le32(mdt + 28);
	phent = audio_le16(mdt + 42);
	phnum = audio_le16(mdt + 44);
	printf("SWEET AUDIO: ADSP.MDT %u bytes, entry %08x phoff %u phnum %u "
	    "phent %u\n", mdt_len, audio_le32(mdt + 24), phoff, phnum, phent);
	if (phent != 32 || phoff > mdt_len || phnum == 0 ||
	    phoff + (uint64_t)phnum * 32 > mdt_len) {
		printf("SWEET AUDIO: ADSP.MDT program headers are invalid\n");
		return;
	}
	if (audio_mdt_hash(mdt, mdt_len, &mh) == 0) {
		printf("SWEET AUDIO: ADSP.MDT hash phdr %u off %u len %u "
		    "memsz %u (%s)\n", mh.index, mh.offset, mh.length,
		    audio_le32(mdt + mh.phdr_off + 20),
		    mh.external ? "external" : "inline");
		err = audio_mdt_hash_data(mdt, mdt_len, &mh, &hash);
		if (err == 0 && mh.length >= 16) {
			printf("SWEET AUDIO: hash data %08x %08x %08x %08x "
			    "(vendor table, opaque to the AP)\n",
			    audio_le32(hash), audio_le32(hash + 4),
			    audio_le32(hash + 8), audio_le32(hash + 12));
		}
	} else {
		printf("SWEET AUDIO: ADSP.MDT has no hash segment\n");
	}
	for (i = 0; i < phnum; i++) {
		const uint8_t *ph = mdt + phoff + i * 32;
		char name[16];
		uint32_t flags, paddr, filesz, memsz;

		if (!audio_phdr_loadable(ph)) {
			continue;
		}
		flags = audio_le32(ph + 24);
		paddr = audio_le32(ph + 12);
		filesz = audio_le32(ph + 16);
		memsz = audio_le32(ph + 20);
		snprintf(name, sizeof(name), "ADSP.B%02u", i);
		if (flags & MDT_RELOCATABLE) {
			reloc++;
		}
		found++;
		total += memsz;
		(void)audio_check_segment(name, paddr, filesz, memsz, &bad);
	}
	printf("SWEET AUDIO: ADSP firmware dry run: %u segments, %llu bytes, "
	    "%u bad, %u relocatable\n", found, (unsigned long long)total, bad,
	    reloc);
	if (bad != 0) {
		printf("SWEET AUDIO: NOT loading ADSP (validation failed)\n");
	}
}

/* --- A3: TrustZone PAS load ---------------------------------------------- */

static uint64_t
audio_scm(uint32_t fn, uint64_t arginfo, uint64_t a, uint64_t b, uint64_t c,
    uint64_t *res1)
{
	register uint64_t x0 __asm__("x0") = fn, x1 __asm__("x1") = arginfo,
	    x2 __asm__("x2") = a, x3 __asm__("x3") = b, x4 __asm__("x4") = c,
	    x5 __asm__("x5") = 0, x6 __asm__("x6") = 0, x7 __asm__("x7") = 0;

	for (int n = 0; n < 1000; n++) {
		__asm__ volatile ("smc #0"
		    : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3), "+r"(x4),
		      "+r"(x5), "+r"(x6), "+r"(x7)
		    :
		    : "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",
		      "x16", "x17", "memory");
		if (x0 != 1) {
			break;
		}
		x0 = 1;
		x1 = arginfo;
		x2 = a;
		x3 = b;
		x4 = c;
		x5 = 0;
		x7 = 0;
	}
	if (res1 != NULL) {
		*res1 = x1;
	}
	return x0;
}

static int
audio_contig(vm_size_t size, vm_offset_t *va, uint64_t *pa)
{
	ppnum_t pn;

	if (kmem_alloc_contig(kernel_map, va, size, PAGE_MASK, 0, 0,
	    KMA_DATA | KMA_ZERO, VM_KERN_MEMORY_IOKIT) != KERN_SUCCESS) {
		return ENOMEM;
	}
	pn = pmap_find_phys(kernel_pmap, (addr64_t)*va);
	if (pn == 0) {
		return EIO;
	}
	*pa = ptoa_64(pn);
	return 0;
}

/* Stream one segment blob into the uncached carve-out mapping. */
static int
audio_fat_blob_to_va(const char *name, vm_offset_t dst, uint32_t want)
{
	uint32_t cluster, size, off = 0;
	int err;

	err = audio_fat_lookup(name, &cluster, &size);
	if (err) {
		printf("SWEET AUDIO: %s not found in IMAGE/\n", name);
		return err;
	}
	if (size != want) {
		printf("SWEET AUDIO: %s is %u bytes, expected %u\n", name, size,
		    want);
		return EINVAL;
	}
	while (off < size) {
		uint32_t n = MIN(size - off, FAT_CLUSTER_BYTES);

		if (audio_probe && (off & 0x3ffff) == 0) {
			printf("SWEET AUDIO: %s %u/%u read cluster %u\n", name,
			    off, size, cluster);
			audio_wait(100);
		}
		err = audio_emmc_read(FAT_DATA_LBA + (cluster - 2) * FAT_SPC,
		    FAT_SPC, fat_bounce);
		if (err) {
			printf("SWEET AUDIO: %s read failed %d at %u\n", name,
			    err, off);
			return err;
		}
		if (audio_probe && (off & 0x3ffff) == 0) {
			printf("SWEET AUDIO: %s %u/%u copy to %08x\n", name,
			    off, size, (uint32_t)(dst + off));
			audio_wait(100);
		}
		audio_io_copy(dst + off, fat_bounce, n);
		off += n;
		if (off < size) {
			cluster = audio_fat_next(cluster);
			if (cluster < 2 || cluster >= FAT_MAX_CLUSTER) {
				printf("SWEET AUDIO: %s chain ends at %u/%u\n",
				    name, off, size);
				return EIO;
			}
		}
	}
	return 0;
}

/*
 * The ADSP/Windows lesson: never start the DSP over SMEM state from an earlier
 * run. If the apps<->ADSP partition already holds a $SMP item, refuse unless
 * lavaudforce=1 (the item is printed so the operator can decide).
 */
static int
audio_adsp_stale(void)
{
	const struct smem_toc *toc = (const struct smem_toc *)
	    (smem + SMEM_TOC_OFF);
	const struct smem_toc_entry *te = (const struct smem_toc_entry *)
	    (toc + 1);

	if (memcmp(toc->magic, "$TOC", 4) != 0 || toc->num_entries >
	    SMEM_TOC_ENTRIES) {
		return 0;
	}
	for (uint32_t i = 0; i < toc->num_entries; i++) {
		const struct smem_part *p;
		uint8_t *e, *end;

		if (te[i].offset == 0 || te[i].host0 != SMEM_APPS_HOST ||
		    te[i].host1 != SMEM_ADSP_PID) {
			continue;
		}
		p = (const struct smem_part *)(smem + te[i].offset);
		if (memcmp(p->magic, "$PRT", 4) != 0 ||
		    p->free_uncached > p->size) {
			return 0;
		}
		e = (uint8_t *)p + sizeof(*p);
		end = (uint8_t *)p + p->free_uncached;
		while (e + sizeof(struct smem_item) <= end) {
			const struct smem_item *h = (const struct smem_item *)e;
			uint32_t magic;

			if (h->canary != SMEM_ITEM_CANARY) {
				break;
			}
			if (h->pad_data <= h->size && h->size != 0) {
				memcpy(&magic, e + sizeof(*h) + h->pad_hdr, 4);
				if (magic == SMP2P_MAGIC) {
					printf("SWEET AUDIO: ADSP SMEM item %u "
					    "from an earlier run\n",
					    (unsigned)h->item);
					return 1;
				}
			}
			e += sizeof(*h) + h->pad_hdr + h->size;
		}
	}
	return 0;
}

/* Walk the MDT entries once, calling cb for every loadable segment. The hash
 * segment is opaque to the AP; Linux loads every PT_LOAD phdr that is not the
 * hash and names the blob after the phdr index. */
typedef int (*audio_mdt_seg_cb)(const char *name, uint32_t paddr,
    uint32_t filesz, uint32_t memsz, void *arg);

static int
audio_mdt_walk(const uint8_t *mdt, uint32_t mdt_len, audio_mdt_seg_cb cb,
    void *arg)
{
	uint32_t phoff, phnum, phent, i;
	int err;

	if (mdt_len < 0x34 || memcmp(mdt, "\177ELF", 4) != 0) {
		return EINVAL;
	}
	phoff = audio_le32(mdt + 28);
	phent = audio_le16(mdt + 42);
	phnum = audio_le16(mdt + 44);
	if (phent != 32 || phoff > mdt_len || phnum == 0 ||
	    phoff + (uint64_t)phnum * 32 > mdt_len) {
		return EINVAL;
	}
	for (i = 0; i < phnum; i++) {
		const uint8_t *ph = mdt + phoff + i * 32;
		char name[16];
		uint32_t paddr, filesz, memsz;

		if (!audio_phdr_loadable(ph)) {
			continue;
		}
		paddr = audio_le32(ph + 12);
		filesz = audio_le32(ph + 16);
		memsz = audio_le32(ph + 20);
		snprintf(name, sizeof(name), "ADSP.B%02u", i);
		err = cb(name, paddr, filesz, memsz, arg);
		if (err) {
			return err;
		}
	}
	return 0;
}

struct pas_range {
	uint32_t start, end;
	uint32_t bad;
	uint32_t entry;
	int entry_ok;
};

static int
audio_range_cb(const char *name, uint32_t paddr, uint32_t filesz,
    uint32_t memsz, void *arg)
{
	struct pas_range *r = arg;

	(void)name;
	if ((uint64_t)paddr < ADSP_CARVE_PA ||
	    (uint64_t)paddr + memsz > ADSP_CARVE_PA + ADSP_CARVE_SIZE ||
	    memsz < filesz) {
		r->bad++;
		return EINVAL;
	}
	if (paddr < r->start) {
		r->start = paddr;
	}
	if (paddr + memsz > r->end) {
		r->end = paddr + memsz;
	}
	if (r->entry >= paddr && r->entry < paddr + memsz) {
		r->entry_ok = 1;
	}
	return 0;
}

struct pas_copy {
	vm_offset_t region;
	uint32_t start;
	uint32_t bad;
};

static int
audio_copy_cb(const char *name, uint32_t paddr, uint32_t filesz,
    uint32_t memsz, void *arg)
{
	struct pas_copy *c = arg;
	int err;

	if (filesz != 0) {
		err = audio_fat_blob_to_va(name,
		    c->region + (paddr - c->start), filesz);
		if (err) {
			c->bad++;
			return err;
		}
	}
	if (memsz > filesz) {
		audio_io_zero(c->region + (paddr - c->start) + filesz,
		    memsz - filesz);
	}
	printf("SWEET AUDIO: loaded %-11s %08x filesz %06x memsz %06x\n",
	    name, paddr, filesz, memsz);
	return 0;
}

/*
 * A3: copy the metadata and the segments into the ADSP carve-out and let
 * TrustZone authenticate and start the DSP. Every SCM call is announced and
 * the sequence stops at the first non-zero return; nothing is touched after
 * AUTH_AND_RESET (the region belongs to the secure side then).
 */
/*
 * Read-only probe of the carve-out mapping, gated by lavaudprobe=1. If a
 * read wedges, the preceding line names the address; if the bootloader or
 * TZ left the firmware preloaded, these words show the blob headers. The
 * audio handoff says the ADSP firmware may already be mapped from the
 * carve-out, so this decides whether the copy is needed at all.
 */
static void
audio_carve_probe(vm_offset_t region)
{
	static const uint32_t offs[] = {
		0x000000, 0x010000, 0x01f000, 0x020000, 0x021000,
		0x030000, 0x800000, 0x1000000, 0x1df0000,
	};

	audio_announce("read the ADSP carve-out at nine offsets "
	    "(lavaudprobe=1)");
	for (uint32_t i = 0; i < sizeof(offs) / sizeof(offs[0]); i++) {
		printf("SWEET AUDIO: carve %08x about to read\n", offs[i]);
		audio_wait(100);
		printf("SWEET AUDIO: carve %08x = %08x\n", offs[i],
		    audio_rd(region, offs[i]));
	}
}


static void
audio_pas_load(void)
{
	struct pas_range range = { 0xffffffffu, 0, 0, 0, 0 };
	struct audio_mdt_hash mh;
	const uint8_t *hash = NULL;
	struct pas_copy copy;
	vm_offset_t meta_va = 0, region_va = 0;
	uint64_t meta_pa = 0, res = 0, ret;
	uint32_t mdt_len = 0, hsz = 0, meta_size = 0;
	int err;

	memset(&mh, 0, sizeof(mh));
	if (audio_step < 4) {
		return;
	}
	if (smem == 0) {
		printf("SWEET AUDIO: no SMEM mapping; refusing PAS\n");
		return;
	}
	if (audio_adsp_stale()) {
		if (!audio_force) {
			printf("SWEET AUDIO: refusing to start the ADSP over an "
			    "earlier run (lavaudforce=1 overrides)\n");
			return;
		}
		printf("SWEET AUDIO: lavaudforce=1, continuing over stale "
		    "SMEM\n");
	}
	if (audio_fat_open() != 0) {
		return;
	}
	if (adsp_mdt == NULL &&
	    kmem_alloc(kernel_map, (vm_offset_t *)&adsp_mdt, ADSP_MDT_MAX,
	    KMA_DATA | KMA_ZERO, VM_KERN_MEMORY_IOKIT) != KERN_SUCCESS) {
		printf("SWEET AUDIO: no memory for the ADSP MDT\n");
		return;
	}
	audio_announce("read ADSP.MDT for the PAS load");
	if (audio_fat_read("ADSP.MDT", adsp_mdt, ADSP_MDT_MAX, &mdt_len)) {
		return;
	}
	if (audio_mdt_hash(adsp_mdt, mdt_len, &mh) != 0) {
		printf("SWEET AUDIO: ADSP.MDT has no hash segment; refusing "
		    "PAS\n");
		return;
	}
	err = audio_mdt_hash_data(adsp_mdt, mdt_len, &mh, &hash);
	if (err) {
		printf("SWEET AUDIO: ADSP hash table is not readable (%d); "
		    "refusing PAS\n", err);
		return;
	}
	hsz = mh.table_off;
	/* The vendor metadata block is ELF header + program headers + hash
	 * table; the MDT is exactly that concatenation on this firmware even
	 * though the hash phdr's p_offset says otherwise. */
	meta_size = hsz + mh.length;
	if (mh.length == 0 || (uint64_t)hsz + mh.length > ADSP_MDT_MAX) {
		printf("SWEET AUDIO: ADSP metadata %u+%u does not fit; "
		    "refusing PAS\n", hsz, mh.length);
		return;
	}
	printf("SWEET AUDIO: ADSP metadata %u bytes (phdrs %u + hash B%02u "
	    "%u, %s)\n", meta_size, hsz, mh.index, mh.length,
	    mh.external ? "external" : "inline");
	range.entry = audio_le32(adsp_mdt + 24);
	if (audio_mdt_walk(adsp_mdt, mdt_len, audio_range_cb, &range) ||
	    range.bad != 0) {
		printf("SWEET AUDIO: segment ranges are not usable; refusing "
		    "PAS\n");
		return;
	}
	if (range.end <= range.start) {
		printf("SWEET AUDIO: empty ADSP image; refusing PAS\n");
		return;
	}
	range.start &= ~0xfffu;
	range.end = (range.end + 0xfffu) & ~0xfffu;
	printf("SWEET AUDIO: ADSP image %08x..%08x (%u bytes), entry %08x "
	    "%s\n", range.start, range.end, range.end - range.start,
	    range.entry, range.entry_ok ? "in range" : "OUTSIDE");
	if (!range.entry_ok) {
		printf("SWEET AUDIO: entry is outside every segment; refusing "
		    "PAS\n");
		return;
	}

	if (audio_contig((meta_size + 0xfffu) & ~0xfffu, &meta_va, &meta_pa)) {
		printf("SWEET AUDIO: no contiguous MDT buffer\n");
		return;
	}
	/* INIT_IMAGE wants the ELF header + program headers + hash table as
	 * one contiguous block (vendor pil_init_image_trusted). */
	memcpy((void *)meta_va, adsp_mdt, hsz);
	memcpy((void *)(meta_va + hsz), hash, mh.length);
	flush_dcache64((addr64_t)meta_pa, meta_size, TRUE);

	region_va = ml_io_map_unmappable(range.start, range.end - range.start,
	    VM_WIMG_IO);
	if (region_va == 0) {
		printf("SWEET AUDIO: ADSP carve-out map failed\n");
		return;
	}

	audio_announce("SCM PIL INIT_IMAGE (ADSP metadata)");
	ret = audio_scm(SCM_FN(SCM_SVC_PIL, PIL_INIT_IMAGE),
	    2 | (SCM_ARG_RW << 6), PAS_ID_ADSP, meta_pa, 0, &res);
	printf("SWEET AUDIO: PAS init_image: ret %llx res %llx\n",
	    (unsigned long long)ret, (unsigned long long)res);
	if (ret != 0 || res != 0) {
		return;
	}
	audio_announce("SCM PIL MEM_SETUP (ADSP carve-out)");
	ret = audio_scm(SCM_FN(SCM_SVC_PIL, PIL_MEM_SETUP), 3, PAS_ID_ADSP,
	    range.start, range.end - range.start, &res);
	printf("SWEET AUDIO: PAS mem_setup: ret %llx res %llx\n",
	    (unsigned long long)ret, (unsigned long long)res);
	if (ret != 0 || res != 0) {
		return;
	}

	audio_carve_probe(region_va);
	if (!audio_copy) {
		printf("SWEET AUDIO: skipping the segment copy (lavaudcopy=0); "
		    "trusting the carve-out as-is\n");
	} else {
		copy.region = region_va;
		copy.start = range.start;
		copy.bad = 0;
		audio_announce("copy every ADSP segment into the carve-out");
		err = audio_mdt_walk(adsp_mdt, mdt_len, audio_copy_cb, &copy);
		if (err || copy.bad != 0) {
			printf("SWEET AUDIO: segment copy failed (%d, %u bad); "
			    "not authenticating\n", err, copy.bad);
			return;
		}
		/*
		 * The carve-out is mapped Device-nGnRnE (VM_WIMG_IO); the byte
		 * stores bypass the cache, so no cache maintenance is needed.
		 * Do NOT flush_dcache64() this range: it is no-map, so
		 * phystokv_range() can return a zero-length run and spin (the
		 * lavaud=4 copy boot wedged here after loading B22).
		 */
	}

	audio_announce("SCM PIL AUTH_AND_RESET (start the ADSP)");
	ret = audio_scm(SCM_FN(SCM_SVC_PIL, PIL_AUTH_RESET), 1, PAS_ID_ADSP,
	    0, 0, &res);
	printf("SWEET AUDIO: PAS auth_and_reset: ret %llx res %llx\n",
	    (unsigned long long)ret, (unsigned long long)res);
	if (ret != 0 || res != 0) {
		printf("SWEET AUDIO: TrustZone refused the ADSP image\n");
		return;
	}
	audio_wait(100);
	if (q6ss != 0 && lpass_on) {
		printf("SWEET AUDIO: after auth: q6 reset %08x pwr %08x "
		    "bhs62 %08x\n", audio_rd(q6ss, Q6SS_RESET),
		    audio_rd(q6ss, Q6SS_PWR_CTL),
		    audio_rd(q6ss, Q6V62_BHS_STATUS));
	} else {
		printf("SWEET AUDIO: not reading the Q6SS after auth (LPASS "
		    "domain reported off)\n");
	}
	printf("SWEET AUDIO: ADSP started; next step is SMP2P/GLINK\n");
}

/* --- A4: read-only inventory of what the ADSP published after PAS --------- */

static void audio_smem_part(const char *tag,
    const struct smem_toc_entry *te);

static int
audio_adsp_smp2p_present(void)
{
	const struct smem_toc *toc = (const struct smem_toc *)
	    (smem + SMEM_TOC_OFF);
	const struct smem_toc_entry *te = (const struct smem_toc_entry *)
	    (toc + 1);

	if (smem == 0 || memcmp(toc->magic, "$TOC", 4) != 0 ||
	    toc->num_entries > SMEM_TOC_ENTRIES) {
		return 0;
	}
	for (uint32_t i = 0; i < toc->num_entries; i++) {
		const struct smem_part *p;
		uint8_t *e, *end;

		if (te[i].offset == 0 || te[i].host0 != SMEM_APPS_HOST ||
		    te[i].host1 != SMEM_ADSP_PID) {
			continue;
		}
		p = (const struct smem_part *)(smem + te[i].offset);
		if (memcmp(p->magic, "$PRT", 4) != 0 ||
		    p->free_uncached > p->size) {
			return 0;
		}
		e = (uint8_t *)p + sizeof(*p);
		end = (uint8_t *)p + p->free_uncached;
		while (e + sizeof(struct smem_item) <= end) {
			const struct smem_item *h = (const struct smem_item *)e;
			uint32_t magic;

			if (h->canary != SMEM_ITEM_CANARY) {
				break;
			}
			if (h->pad_data <= h->size && h->size != 0) {
				memcpy(&magic, e + sizeof(*h) + h->pad_hdr, 4);
				if (magic == SMP2P_MAGIC) {
					return 1;
				}
			}
			e += sizeof(*h) + h->pad_hdr + h->size;
		}
	}
	return 0;
}

/*
 * A4: after AUTH_AND_RESET the ADSP creates its own SMP2P item and, once the
 * AP side exists, marks slave-kernel ready. This step only reads: it waits up
 * to 3 s for the item, then dumps the whole apps<->ADSP partition (item ids,
 * sizes, $SMP headers and entry values) and the GLINK candidates. The vendor
 * smem.h for this phone gives the ids: AP 429, ADSP 443, GLINK 478/479/480;
 * the dump confirms them and shows the slave-kernel state.
 */
static int
audio_item_present(uint16_t want)
{
	const struct smem_toc *toc = (const struct smem_toc *)
	    (smem + SMEM_TOC_OFF);
	const struct smem_toc_entry *te = (const struct smem_toc_entry *)
	    (toc + 1);

	if (smem == 0 || memcmp(toc->magic, "$TOC", 4) != 0 ||
	    toc->num_entries > SMEM_TOC_ENTRIES) {
		return 0;
	}
	for (uint32_t i = 0; i < toc->num_entries; i++) {
		const struct smem_part *p;
		uint8_t *e, *end;

		if (te[i].offset == 0 || te[i].host0 != SMEM_APPS_HOST ||
		    te[i].host1 != SMEM_ADSP_PID) {
			continue;
		}
		p = (const struct smem_part *)(smem + te[i].offset);
		if (memcmp(p->magic, "$PRT", 4) != 0 ||
		    p->free_uncached > p->size) {
			return 0;
		}
		e = (uint8_t *)p + sizeof(*p);
		end = (uint8_t *)p + p->free_uncached;
		while (e + sizeof(struct smem_item) <= end) {
			const struct smem_item *h = (const struct smem_item *)e;

			if (h->canary != SMEM_ITEM_CANARY) {
				break;
			}
			if (h->item == want) {
				return 1;
			}
			e += sizeof(*h) + h->pad_hdr + h->size;
		}
	}
	return 0;
}

static void
audio_adsp_inventory(void)
{
	const struct smem_toc *toc = (const struct smem_toc *)
	    (smem + SMEM_TOC_OFF);
	const struct smem_toc_entry *te = (const struct smem_toc_entry *)
	    (toc + 1);
	uint32_t found = 0;
	int present = 0;

	if (audio_step < 5) {
		return;
	}
	if (smem == 0) {
		printf("SWEET AUDIO: no SMEM mapping; no ADSP inventory\n");
		return;
	}
	audio_announce("wait for the ADSP SMP2P item after PAS (read only)");
	for (int i = 0; i < 30; i++) {
		if (audio_adsp_smp2p_present()) {
			present = 1;
			break;
		}
		audio_wait(100);
	}
	printf("SWEET AUDIO: ADSP SMP2P item %s after 3 s\n",
	    present ? "present" : "absent");
	if (memcmp(toc->magic, "$TOC", 4) != 0 ||
	    toc->num_entries > SMEM_TOC_ENTRIES) {
		printf("SWEET AUDIO: no SMEM TOC; no inventory\n");
		return;
	}
	for (uint32_t i = 0; i < toc->num_entries; i++) {
		if (te[i].offset == 0 || te[i].host0 != SMEM_APPS_HOST ||
		    te[i].host1 != SMEM_ADSP_PID) {
			continue;
		}
		audio_smem_part("adsp-after-start", &te[i]);
		found++;
	}
	printf("SWEET AUDIO: ADSP inventory: %u partition(s)\n", found);
	printf("SWEET AUDIO: ids: out %u %s, in %u %s, glink %u/%u/%u %u/%u/%u\n",
	    SMEM_ITEM_SMP2P_OUT,
	    audio_item_present(SMEM_ITEM_SMP2P_OUT) ? "yes" : "no",
	    SMEM_ITEM_SMP2P_IN,
	    audio_item_present(SMEM_ITEM_SMP2P_IN) ? "yes" : "no",
	    SMEM_ITEM_GLINK_DESC, SMEM_ITEM_GLINK_FIFO0,
	    SMEM_ITEM_GLINK_FIFO1,
	    audio_item_present(SMEM_ITEM_GLINK_DESC),
	    audio_item_present(SMEM_ITEM_GLINK_FIFO0),
	    audio_item_present(SMEM_ITEM_GLINK_FIFO1));
	if (q6ss != 0 && lpass_on) {
		printf("SWEET AUDIO: q6 after start: reset %08x pwr %08x "
		    "bhs62 %08x\n", audio_rd(q6ss, Q6SS_RESET),
		    audio_rd(q6ss, Q6SS_PWR_CTL),
		    audio_rd(q6ss, Q6V62_BHS_STATUS));
	}
	if (!present) {
		printf("SWEET AUDIO: the ADSP did not publish SMP2P; A5 must "
		    "create the AP-side item and kick\n");
	}
}

/* --- A5: SMP2P out 429 + kick + slave-kernel ------------------------------ */

static struct smem_part *
audio_smem_find_part(void)
{
	const struct smem_toc *toc = (const struct smem_toc *)
	    (smem + SMEM_TOC_OFF);
	const struct smem_toc_entry *te = (const struct smem_toc_entry *)
	    (toc + 1);

	if (smem == 0 || memcmp(toc->magic, "$TOC", 4) != 0 ||
	    toc->num_entries > SMEM_TOC_ENTRIES) {
		return NULL;
	}
	for (uint32_t i = 0; i < toc->num_entries; i++) {
		struct smem_part *p;

		if (te[i].offset == 0 || te[i].host0 != SMEM_APPS_HOST ||
		    te[i].host1 != SMEM_ADSP_PID) {
			continue;
		}
		p = (struct smem_part *)(smem + te[i].offset);
		if (memcmp(p->magic, "$PRT", 4) == 0 &&
		    p->free_uncached <= p->size) {
			return p;
		}
	}
	return NULL;
}

static void *
audio_smem_get_item(uint16_t want, uint32_t *payload)
{
	struct smem_part *p = audio_smem_find_part();
	uint8_t *e, *end;
	uint32_t n = 0;

	if (p == NULL) {
		return NULL;
	}
	e = (uint8_t *)p + sizeof(*p);
	end = (uint8_t *)p + p->free_uncached;
	while (e + sizeof(struct smem_item) <= end) {
		const struct smem_item *h = (const struct smem_item *)e;

		if (++n > 64) {
			printf("SWEET AUDIO: SMEM item scan did not terminate\n");
			break;
		}
		if (h->canary != SMEM_ITEM_CANARY) {
			break;
		}
		if (h->item == want) {
			if (payload != NULL) {
				*payload = h->size - h->pad_data;
			}
			return e + sizeof(*h) + h->pad_hdr;
		}
		e += sizeof(*h) + h->pad_hdr + h->size;
	}
	return NULL;
}

static int
audio_smem_lock(void)
{
	if (tcsr == 0) {
		return 0;
	}
	for (int i = 0; i < 1000; i++) {
		audio_wr(tcsr, SMEM_HWLOCK * 0x1000, HWLOCK_APPS_ID);
		if ((audio_rd(tcsr, SMEM_HWLOCK * 0x1000) & 0xffffu) ==
		    HWLOCK_APPS_ID) {
			return 1;
		}
		audio_wait(1);
	}
	printf("SWEET AUDIO: SMEM hwlock busy (owner %08x)\n",
	    audio_rd(tcsr, SMEM_HWLOCK * 0x1000));
	return 0;
}

static void
audio_smem_unlock(void)
{
	if (tcsr != 0) {
		audio_wr(tcsr, SMEM_HWLOCK * 0x1000, 0);
	}
}

/*
 * qcom_smem_alloc_private(): append a private entry at offset_free_uncached,
 * under the SMEM hwlock. The item must not exist yet; existing items are
 * returned as-is.
 */
static void *
audio_smem_alloc_item(uint16_t item, uint32_t size)
{
	struct smem_part *p = audio_smem_find_part();
	struct smem_item *h;
	uint32_t aligned, need;
	uint8_t *data = NULL;

	if (p == NULL) {
		printf("SWEET AUDIO: no apps<->adsp SMEM partition\n");
		return NULL;
	}
	aligned = (size + 7u) & ~7u;
	need = sizeof(*h) + aligned;
	if (!audio_smem_lock()) {
		return NULL;
	}
	/* re-check under the lock; another host may have allocated it */
	data = audio_smem_get_item(item, NULL);
	if (data != NULL) {
		audio_smem_unlock();
		printf("SWEET AUDIO: SMEM item %u already exists, reusing\n",
		    (unsigned)item);
		return data;
	}
	if (p->free_uncached + need >= p->free_cached) {
		audio_smem_unlock();
		printf("SWEET AUDIO: SMEM partition full (%05x + %x >= %05x)\n",
		    p->free_uncached, need, p->free_cached);
		return NULL;
	}
	h = (struct smem_item *)((uint8_t *)p + p->free_uncached);
	h->canary = SMEM_ITEM_CANARY;
	h->item = item;
	h->size = aligned;
	h->pad_data = (uint16_t)(aligned - size);
	h->pad_hdr = 0;
	h->rsvd = 0;
	data = (uint8_t *)h + sizeof(*h);
	audio_io_zero((vm_offset_t)data, aligned);
	__asm__ volatile("dsb sy" ::: "memory");
	p->free_uncached += need;
	__asm__ volatile("dsb sy" ::: "memory");
	audio_smem_unlock();
	printf("SWEET AUDIO: SMEM item %u allocated (%u bytes at %05x)\n",
	    (unsigned)item, size, (uint32_t)((uint8_t *)h - (uint8_t *)p));
	return data;
}

static void
audio_smp2p_out_init(void)
{
	struct smp2p_hdr h;
	struct smp2p_entry e;
	void *data;
	uint32_t payload = 0;

	if (audio_step < 6) {
		return;
	}
	if (smem == 0) {
		printf("SWEET AUDIO: no SMEM mapping; no SMP2P out\n");
		return;
	}
	if (tcsr == 0) {
		tcsr = ml_io_map(TCSR_MUTEX_PA, TCSR_MUTEX_SIZE);
		if (tcsr == 0) {
			printf("SWEET AUDIO: TCSR mutex map failed; no SMP2P "
			    "out\n");
			return;
		}
	}
	audio_announce("create the AP SMP2P item 429 (first SMEM write)");
	data = audio_smem_alloc_item(SMEM_ITEM_SMP2P_OUT,
	    sizeof(struct smp2p_hdr) +
	    SMP2P_MAX_ENTRY * sizeof(struct smp2p_entry));
	if (data == NULL) {
		return;
	}
	(void)payload;
	memset(&h, 0, sizeof(h));
	h.magic = SMP2P_MAGIC;
	h.feature_version = 1;                  /* version 1, no features */
	h.rem_loc_proc_id = (uint32_t)SMEM_ADSP_PID << 16;  /* local 0 */
	h.valid_total_ent = (2u << 16) | 16u;   /* 2 valid of 16 */
	h.flags = 0;
	audio_io_copy((vm_offset_t)data, &h, sizeof(h));
	memset(&e, 0, sizeof(e));
	strlcpy(e.name, "master-kernel", sizeof(e.name));
	e.value = 0;
	audio_io_copy((vm_offset_t)data + sizeof(h), &e, sizeof(e));
	memset(&e, 0, sizeof(e));
	strlcpy(e.name, "sleepstate", sizeof(e.name));
	e.value = 1;
	audio_io_copy((vm_offset_t)data + sizeof(h) + sizeof(e), &e,
	    sizeof(e));

	smp2p_in = audio_smem_get_item(SMEM_ITEM_SMP2P_IN, &smp2p_in_len);
	printf("SWEET AUDIO: cached ADSP item 443: %s payload %u magic %08x\n",
	    smp2p_in != NULL ? "yes" : "no", smp2p_in_len,
	    smp2p_in != NULL ? audio_le32(smp2p_in) : 0);
	printf("SWEET AUDIO: smp2p out item %u ready; kicking APCS bit 10\n",
	    SMEM_ITEM_SMP2P_OUT);
	audio_wr(apcs, 0x008, 0x400u);
}

static uint32_t
audio_smp2p_in_value(const char *name, int *found)
{
	const uint8_t *data = smp2p_in;
	const struct smp2p_hdr *h;
	uint32_t fv, rl, vt, valid;

	if (found != NULL) {
		*found = 0;
	}
	if (data == NULL || smp2p_in_len < sizeof(*h)) {
		/* Fallback for a boot that did not cache it yet. */
		data = audio_smem_get_item(SMEM_ITEM_SMP2P_IN, NULL);
	}
	if (data == NULL) {
		return 0;
	}
	h = (const struct smp2p_hdr *)data;
	if (h->magic != SMP2P_MAGIC) {
		return 0;
	}
	fv = h->feature_version;
	rl = h->rem_loc_proc_id;
	vt = h->valid_total_ent;
	(void)fv;
	(void)rl;
	valid = vt >> 16;
	for (uint32_t i = 0; i < valid && i < SMP2P_MAX_ENTRY; i++) {
		const struct smp2p_entry *e = (const struct smp2p_entry *)
		    (data + sizeof(*h) + i * sizeof(*e));
		char entry[sizeof(e->name) + 1];

		memcpy(entry, e->name, sizeof(e->name));
		entry[sizeof(e->name)] = '\0';
		if (strcmp(entry, name) == 0) {
			if (found != NULL) {
				*found = 1;
			}
			return e->value;
		}
	}
	return 0;
}

static void audio_smp2p_item(const uint8_t *data, uint32_t len);

static void
audio_smp2p_wait_ready(void)
{
	uint32_t last = 0xffffffffu;
	int found = 0;

	if (audio_step < 6) {
		return;
	}
	audio_announce("wait for ADSP item 443 and slave-kernel READY");
	if (smp2p_in != NULL) {
		const struct smp2p_hdr *h = (const struct smp2p_hdr *)smp2p_in;

		printf("SWEET AUDIO: cached 443 hdr magic %08x fv %08x rl %08x "
		    "vt %08x\n", h->magic, h->feature_version,
		    h->rem_loc_proc_id, h->valid_total_ent);
		printf("SWEET AUDIO: 443 entries before the wait:\n");
		audio_smp2p_item(smp2p_in, smp2p_in_len);
		printf("SWEET AUDIO: 443 entry dump done\n");
	}
	/* The ADSP only fills the 443 entries after it has seen the AP's 429. */
	audio_wait(3000);
	printf("SWEET AUDIO: 443 wait loop start\n");
	for (int i = 0; i < 50; i++) {
		uint32_t v;

		if (i < 2) {
			printf("SWEET AUDIO: 443 value call %d\n", i);
		}
		v = audio_smp2p_in_value("slave-kernel", &found);

		if (audio_probe && (i % 5) == 0) {
			printf("SWEET AUDIO: 443 wait t=%d found %d v %08x\n",
			    i, found, v);
		}
		if (found && v != last) {
			last = v;
			printf("SWEET AUDIO: smp2p in 443 slave-kernel %08x "
			    "(t=%d00 ms)\n", v, i);
		}
		if (found && (v & 1u)) {
			printf("SWEET AUDIO: ADSP READY (slave-kernel %08x)\n",
			    v);
			return;
		}
		audio_wait(100);
	}
	if (found) {
		printf("SWEET AUDIO: ADSP item 443 present, slave-kernel %08x "
		    "never READY\n", last);
	} else {
		printf("SWEET AUDIO: ADSP item 443 never appeared\n");
	}
}

/* --- F3: GLINK lpass (SMEM native) and the APR channel ------------------- */

static void
glink_mb(void)
{
	__asm__ volatile("dsb sy" ::: "memory");
}

static uint32_t
glink_desc_rd(uint32_t i)
{
	return audio_rd((vm_offset_t)glink_desc, i * 4);
}

static void
glink_desc_wr(uint32_t i, uint32_t v)
{
	audio_wr((vm_offset_t)glink_desc, i * 4, v);
}

static uint32_t
glink_rx_avail(void)
{
	uint32_t h, t;

	if (glink_rx == NULL) {
		return 0;
	}
	h = glink_desc_rd(3);
	t = glink_desc_rd(2);
	return h >= t ? h - t : glink_rx_len - t + h;
}

static void
glink_rx_peek(uint32_t off, void *dst, uint32_t n)
{
	uint8_t *d = dst;
	uint32_t t = glink_desc_rd(2) + off;
	uint32_t i;

	if (t >= glink_rx_len) {
		t -= glink_rx_len;
	}
	for (i = 0; i < n; i++) {
		d[i] = glink_rx[t];
		if (++t == glink_rx_len) {
			t = 0;
		}
	}
}

static void
glink_rx_advance(uint32_t n)
{
	uint32_t t = glink_desc_rd(2) + n;

	if (t >= glink_rx_len) {
		t -= glink_rx_len;
	}
	glink_mb();
	glink_desc_wr(2, t);
}

static uint32_t
glink_tx_put(uint32_t head, const void *src, uint32_t n)
{
	const uint8_t *s = src;
	uint32_t n1 = MIN(n, GLINK_TX_SIZE - head);

	audio_io_copy((vm_offset_t)(glink_tx + head), s, n1);
	if (n > n1) {
		audio_io_copy((vm_offset_t)glink_tx, s + n1, n - n1);
	}
	head += n;
	return head >= GLINK_TX_SIZE ? head - GLINK_TX_SIZE : head;
}

static int
glink_tx_write(const void *hdr, uint32_t hlen, const void *data,
    uint32_t dlen)
{
	uint32_t head = 0, tail = 0, avail = 0, need;
	int n;

	if (glink_desc == NULL || glink_tx == NULL || apcs == 0) {
		return 0;
	}
	need = (hlen + dlen + 7) & ~7u;
	for (n = 0; n < 1000; n++) {
		head = glink_desc_rd(1);
		tail = glink_desc_rd(0);
		avail = tail <= head ? GLINK_TX_SIZE - head + tail :
		    tail - head;
		avail = avail < 8 ? 0 : avail - 8;
		if (avail >= need) {
			break;
		}
		audio_wait(1);
	}
	if (n == 1000) {
		printf("SWEET AUDIO: glink TX full (head %x tail %x)\n", head,
		    tail);
		return 0;
	}
	head = glink_tx_put(head, hdr, hlen);
	if (dlen != 0) {
		head = glink_tx_put(head, data, dlen);
	}
	head = (head + 7) & ~7u;
	if (head >= GLINK_TX_SIZE) {
		head -= GLINK_TX_SIZE;
	}
	glink_mb();
	glink_desc_wr(1, head);
	glink_mb();
	audio_wr(apcs, 0x008, 1u << GLINK_IRQ_BIT);
	glink_mb();
	glink_tx_msgs++;
	return 1;
}

static void
glink_cmd(uint16_t cmd, uint16_t p1, uint32_t p2)
{
	struct glink_msg m = { cmd, p1, p2 };

	(void)glink_tx_write(&m, sizeof(m), NULL, 0);
}

static void
glink_open(void)
{
	struct {
		struct glink_msg m;
		char name[16];
	} __attribute__((packed)) req;

	memset(&req, 0, sizeof(req));
	req.m.cmd = GCMD_OPEN;
	req.m.p1 = GLINK_LCID;
	req.m.p2 = sizeof(GLINK_CHAN_NAME);
	memcpy(req.name, GLINK_CHAN_NAME, sizeof(GLINK_CHAN_NAME));
	(void)glink_tx_write(&req, sizeof(struct glink_msg) +
	    sizeof(GLINK_CHAN_NAME), NULL, 0);
	glink_open_sent = 1;
}

static void
glink_intents(uint32_t count, uint32_t size)
{
	struct {
		struct glink_msg m;
		uint32_t pair[2 * GLINK_OUR_INTENTS];
	} __attribute__((packed)) m;

	memset(&m, 0, sizeof(m));
	m.m.cmd = GCMD_INTENT;
	m.m.p1 = GLINK_LCID;
	m.m.p2 = count;
	for (uint32_t i = 0; i < count && i < GLINK_OUR_INTENTS; i++) {
		m.pair[2 * i] = size;
		m.pair[2 * i + 1] = glink_next_intent++;
	}
	(void)glink_tx_write(&m, sizeof(struct glink_msg) + 8 * count, NULL,
	    0);
}

static int
glink_send_apr_pkt(uint8_t dest_svc, uint16_t src_port, uint16_t dest_port,
    uint32_t opcode,
    const void *payload, uint32_t payload_len, uint32_t token)
{
	struct apr_hdr a;
	struct glink_data_hdr h;
	uint8_t buf[GLINK_DATA_MAX];
	unsigned i;

	if (sizeof(a) + payload_len > sizeof(buf)) {
		return 0;
	}
	memset(&a, 0, sizeof(a));
	a.hdr_field = (uint16_t)((2u << 8) | (5u << 4) | 0u);
	a.pkt_size = (uint16_t)(sizeof(a) + payload_len);
	a.src_svc = dest_svc;
	a.src_domain = APR_DOMAIN_APPS;
	a.src_port = src_port;
	a.dest_svc = dest_svc;
	a.dest_domain = APR_DOMAIN_ADSP;
	a.dest_port = dest_port;
	a.token = token;
	a.opcode = opcode;
	memcpy(buf, &a, sizeof(a));
	if (payload_len != 0) {
		memcpy(buf + sizeof(a), payload, payload_len);
	}

	for (i = 0; i < glink_ri_count; i++) {
		if (!glink_ri[i].used &&
		    glink_ri[i].size >= sizeof(a) + payload_len) {
			break;
		}
	}
	if (i == glink_ri_count) {
		printf("SWEET AUDIO: no remote GLINK intent for APR svc %u "
		    "opcode %08x (%u)\n", dest_svc, opcode, glink_ri_count);
		return 0;
	}
	h.m.cmd = GCMD_TX_DATA;
	h.m.p1 = GLINK_LCID;
	h.m.p2 = glink_ri[i].id;
	h.chunk = (uint32_t)(sizeof(a) + payload_len);
	h.left = 0;
	glink_ri[i].used = 1;
	if (!glink_tx_write(&h, sizeof(h), buf, h.chunk)) {
		glink_ri[i].used = 0;
		return 0;
	}
	printf("SWEET AUDIO: APR -> svc %u opcode %08x len %u intent %u\n",
	    dest_svc, opcode, h.chunk, glink_ri[i].id);
	return 1;
}

static int
glink_send_apr(void)
{
	return glink_send_apr_pkt(APR_SVC_ADSP_CORE, 1, 0, APR_CMD_GET_FWK_VERSION,
	    NULL, 0, 1);
}

static int
glink_rx_one(void)
{
	struct glink_msg m;
	uint32_t avail = glink_rx_avail();
	uint32_t len;

	if (avail < sizeof(m)) {
		return 0;
	}
	glink_rx_peek(0, &m, sizeof(m));
	switch (m.cmd) {
	case GCMD_VERSION:
		printf("SWEET AUDIO: glink VERSION %u features %x\n", m.p1,
		    m.p2);
		glink_cmd(GCMD_VERSION_ACK, 1, GLINK_FEATURES);
		glink_rx_advance(8);
		break;
	case GCMD_VERSION_ACK:
		printf("SWEET AUDIO: glink VERSION_ACK %u features %x\n", m.p1,
		    m.p2);
		glink_ver_acked = 1;
		glink_rx_advance(8);
		break;
	case GCMD_OPEN_ACK:
		printf("SWEET AUDIO: glink OPEN_ACK \"%s\" rcid %u\n",
		    GLINK_CHAN_NAME, m.p1);
		glink_rcid = (uint16_t)m.p1;
		glink_open_acked = 1;
		glink_rx_advance(8);
		break;
	case GCMD_OPEN:
		glink_cmd(GCMD_OPEN_ACK, (uint16_t)m.p1, 0);
		glink_rx_advance(8);
		break;
	case GCMD_CLOSE:
		printf("SWEET AUDIO: glink CLOSE from ADSP\n");
		glink_cmd(GCMD_CLOSE_ACK, (uint16_t)m.p1, 0);
		glink_open_acked = 0;
		glink_rx_advance(8);
		break;
	case GCMD_INTENT:
		len = (8 + 8 * m.p2 + 7) & ~7u;
		if (avail < len) {
			return 0;
		}
		if (glink_intent_log < 6 || m.p2 != 0) {
			uint8_t raw[16];

			glink_rx_peek(0, raw, sizeof(raw));
			printf("SWEET AUDIO: glink INTENT p1 %u p2 %u raw %08x "
			    "%08x %08x %08x\n", m.p1, m.p2,
			    audio_le32(raw), audio_le32(raw + 4),
			    audio_le32(raw + 8), audio_le32(raw + 12));
			glink_intent_log++;
		}
		for (uint32_t i = 0; i < m.p2; i++) {
			uint32_t pair[2];

			glink_rx_peek(8 + 8 * i, pair, sizeof(pair));
			if (glink_ri_count < GLINK_MAX_INTENTS) {
				glink_ri[glink_ri_count].size = pair[0];
				glink_ri[glink_ri_count].id = pair[1];
				glink_ri[glink_ri_count].used = 0;
				glink_ri_count++;
			}
		}
		if (glink_ri_count != 0) {
			printf("SWEET AUDIO: glink got %u remote intents\n",
			    glink_ri_count);
		}
		glink_rx_advance(len);
		break;
	case GCMD_RX_INTENT_REQ:
		glink_intents(1, (m.p2 > GLINK_INTENT_SIZE ? m.p2 : GLINK_INTENT_SIZE));
		glink_cmd(GCMD_RX_INTENT_REQ_ACK, GLINK_LCID, 1);
		glink_rx_advance(8);
		break;
	case GCMD_RX_INTENT_REQ_ACK:
		glink_rx_advance(8);
		break;
	case GCMD_TX_DATA:
	case GCMD_TX_DATA_CONT: {
		struct glink_data_hdr h;
		uint8_t buf[GLINK_DATA_MAX];

		if (avail < sizeof(h)) {
			return 0;
		}
		glink_rx_peek(0, &h, sizeof(h));
		len = h.chunk;
		if (len > sizeof(buf)) {
			len = sizeof(buf);
		}
		if (avail < sizeof(h) + h.chunk) {
			return 0;
		}
		glink_rx_peek(sizeof(h), buf, len);
		if (len >= sizeof(struct apr_hdr)) {
			const struct apr_hdr *a =
			    (const struct apr_hdr *)(const void *)buf;

			printf("SWEET AUDIO: APR rx src %u/%u dst %u/%u "
			    "opcode %08x token %u len %u\n", a->src_svc,
			    a->src_domain, a->dest_svc, a->dest_domain,
			    a->opcode, a->token, len);
			{
				uint32_t hdr_bytes =
				    ((a->hdr_field >> 4) & 0x0fu) * 4u;

				if ((a->opcode == 0x000110E8u ||
				    a->opcode == ADM_CMDRSP_DEVICE_OPEN_V5) &&
				    len >= hdr_bytes + 8) {
					const uint8_t *p = buf + hdr_bytes;

					printf("SWEET AUDIO: APR rsp %08x "
					    "payload %08x %08x\n", a->opcode,
					    audio_le32(p), audio_le32(p + 4));
					if (a->opcode ==
					    ADM_CMDRSP_DEVICE_OPEN_V5) {
						adm_status = audio_le32(p);
						adm_copp = audio_le32(p + 4) &
						    0xffffu;
						printf("SWEET AUDIO: ADM "
						    "status %u copp %x\n",
						    adm_status, adm_copp);
					}
				}
				printf("SWEET AUDIO: APR raw len %u pkt %u "
				    "hdr %04x chunk %u:", len, a->pkt_size,
				    a->hdr_field, h.chunk);
				for (uint32_t bi = 0; bi < len && bi < 48;
				    bi++) {
					printf(" %02x", buf[bi]);
				}
				printf("\n");
			}
		} else {
			printf("SWEET AUDIO: glink rx data len %u (no APR)\n",
			    len);
		}
		glink_cmd(GCMD_RX_DONE, GLINK_LCID, m.p2);
		glink_rx_advance((sizeof(h) + h.chunk + 7) & ~7u);
		glink_rx_msgs++;
		break;
	}
	case GCMD_CLOSE_ACK:
	case GCMD_SIGNALS:
	case GCMD_READ_NOTIF:
	default:
		glink_rx_advance(8);
		break;
	}
	return 1;
}

static void
audio_glink_apr(void)
{
	uint32_t len = 0, ms;

	if (audio_step < 7) {
		return;
	}
	if (smem == 0 || apcs == 0 || tcsr == 0) {
		printf("SWEET AUDIO: GLINK needs SMEM/APCS/TCSR; skipping\n");
		return;
	}
	audio_announce("map the ADSP GLINK descriptor and FIFOs (F3)");
	glink_desc = audio_smem_get_item(SMEM_ITEM_GLINK_DESC, &len);
	if (glink_desc == NULL || len < 32) {
		printf("SWEET AUDIO: no GLINK descriptor item 478\n");
		return;
	}
	glink_rx = audio_smem_get_item(SMEM_ITEM_GLINK_FIFO1, &glink_rx_len);
	if (glink_rx == NULL || glink_rx_len < 64) {
		printf("SWEET AUDIO: no GLINK RX FIFO item 480\n");
		return;
	}
	glink_tx = audio_smem_alloc_item(SMEM_ITEM_GLINK_FIFO0, GLINK_TX_SIZE);
	if (glink_tx == NULL) {
		printf("SWEET AUDIO: no GLINK TX FIFO item 479\n");
		return;
	}
	audio_io_zero((vm_offset_t)glink_tx, GLINK_TX_SIZE);
	printf("SWEET AUDIO: glink desc %08x %08x %08x %08x rx %u\n",
	    glink_desc_rd(0), glink_desc_rd(1), glink_desc_rd(2),
	    glink_desc_rd(3), glink_rx_len);
	glink_desc_wr(1, 0);            /* tx head, ours */
	glink_desc_wr(2, 0);            /* rx tail, ours */
	glink_mb();

	printf("SWEET AUDIO: glink VERSION (features %x), APCS bit %u\n",
	    GLINK_FEATURES, GLINK_IRQ_BIT);
	glink_cmd(GCMD_VERSION, 1, GLINK_FEATURES);
	for (ms = 0; ms < 2000 && !glink_ver_acked; ms += 10) {
		while (glink_rx_one()) {
		}
		audio_wait(10);
	}
	printf("SWEET AUDIO: glink version %s after %u ms\n",
	    glink_ver_acked ? "acked" : "NOT acked", ms);
	if (!glink_ver_acked) {
		return;
	}

	glink_open();
	for (ms = 0; ms < 2000 && !glink_open_acked; ms += 10) {
		while (glink_rx_one()) {
		}
		audio_wait(10);
	}
	printf("SWEET AUDIO: glink %s open %s after %u ms (rcid %u)\n",
	    GLINK_CHAN_NAME, glink_open_acked ? "acked" : "NOT acked", ms,
	    glink_rcid);
	if (!glink_open_acked) {
		return;
	}

	glink_intents(GLINK_OUR_INTENTS, GLINK_INTENT_SIZE);
	glink_intents_sent = 1;
	printf("SWEET AUDIO: glink %u intents of %u sent\n",
	    GLINK_OUR_INTENTS, GLINK_INTENT_SIZE);
	for (ms = 0; ms < 1000 && glink_ri_count == 0; ms += 10) {
		while (glink_rx_one()) {
		}
		audio_wait(10);
	}
	printf("SWEET AUDIO: glink remote intents %u after %u ms\n",
	    glink_ri_count, ms);
	if (glink_ri_count == 0) {
		printf("SWEET AUDIO: requesting one remote intent of %u\n",
		    (unsigned)sizeof(struct apr_hdr));
		glink_cmd(GCMD_RX_INTENT_REQ, GLINK_LCID,
		    sizeof(struct apr_hdr));
		for (ms = 0; ms < 1000 && glink_ri_count == 0; ms += 10) {
			while (glink_rx_one()) {
			}
			audio_wait(10);
		}
		printf("SWEET AUDIO: glink remote intents %u after the "
		    "request (%u ms)\n", glink_ri_count, ms);
	}
	if (glink_ri_count == 0) {
		return;
	}
	if (!glink_send_apr()) {
		return;
	}
	for (ms = 0; ms < 2000; ms += 10) {
		while (glink_rx_one()) {
		}
		audio_wait(10);
	}
	printf("SWEET AUDIO: glink apr probe done: tx %u rx %u\n",
	    glink_tx_msgs, glink_rx_msgs);
}

/*
 * F5: AFE INT4_MI2S_RX. SET_PARAM_V2 carries the I2S configuration (48 kHz,
 * 16-bit, SD0, stereo, internal WS), then DEVICE_START starts the port.
 */
static void
audio_afe_port(void)
{
	struct {
		uint32_t payload_size;
		uint32_t mem[3];
		uint32_t module, param;
		uint16_t size, reserved;
		struct afe_clock_set clk;
	} __attribute__((packed)) clk_pkt;
	struct {
		uint16_t port_id, payload_size;
		uint32_t mem[3];
		uint32_t module, param;
		uint16_t size, reserved;
		struct afe_i2s_cfg cfg;
	} __attribute__((packed)) pkt;
	struct {
		uint16_t port_id, reserved;
	} __attribute__((packed)) start = { AFE_PORT_INT4_MI2S_RX, 0 };
	uint32_t ms;

	if (audio_step < 10) {
		return;
	}
	audio_announce("AFE INT4_MI2S_RX (0x1036) clock, I2S config and start");

	/* The MI2S bit clock: 48 kHz * 16 bits * 2 channels = 1.536 MHz. */
	memset(&clk_pkt, 0, sizeof(clk_pkt));
	clk_pkt.payload_size = 12 + sizeof(clk_pkt.clk);
	clk_pkt.module = AFE_MODULE_CLOCK_SET;
	clk_pkt.param = AFE_PARAM_ID_CLOCK_SET;
	clk_pkt.size = sizeof(clk_pkt.clk);
	clk_pkt.clk.version = 1;
	clk_pkt.clk.id = AFE_CLK_INT4_MI2S_IBIT;
	clk_pkt.clk.hz = 48000 * 16 * 2;
	clk_pkt.clk.attribute = 4;      /* INVERT_COUPLE_NO */
	clk_pkt.clk.root = 0;
	clk_pkt.clk.enable = 1;
	(void)glink_send_apr_pkt(APR_SVC_AFE, 1, 0, AFE_SVC_CMD_SET_PARAM, &clk_pkt,
	    sizeof(clk_pkt), 2);
	for (ms = 0; ms < 1000; ms += 10) {
		while (glink_rx_one()) {
		}
		audio_wait(10);
	}
	printf("SWEET AUDIO: AFE MI2S clock 0x113 at %u Hz sent\n", 1536000u);

	memset(&pkt, 0, sizeof(pkt));
	pkt.port_id = AFE_PORT_INT4_MI2S_RX;
	pkt.module = AFE_MODULE_AUDIO_DEV_INTERFACE;
	pkt.param = AFE_PARAM_ID_I2S_CONFIG;
	pkt.size = sizeof(pkt.cfg);
	pkt.payload_size = 12 + sizeof(pkt.cfg);
	pkt.cfg.version = 1;
	pkt.cfg.bit_width = 16;
	pkt.cfg.channel_mode = 1;       /* AFE_PORT_I2S_SD0 */
	pkt.cfg.mono_stereo = 1;        /* AFE_PORT_I2S_STEREO */
	pkt.cfg.ws_src = 1;             /* internal */
	pkt.cfg.sample_rate = 48000;

	if (!glink_send_apr_pkt(APR_SVC_AFE, 1, 0, AFE_PORT_CMD_SET_PARAM_V2,
	    &pkt, sizeof(pkt), 3)) {
		printf("SWEET AUDIO: AFE config not sent\n");
		return;
	}
	for (ms = 0; ms < 1000; ms += 10) {
		while (glink_rx_one()) {
		}
		audio_wait(10);
	}
	printf("SWEET AUDIO: AFE 0x1036 I2S config sent (16-bit 48k SD0)\n");

	if (!glink_send_apr_pkt(APR_SVC_AFE, 1, 0, AFE_PORT_CMD_DEVICE_START,
	    &start, sizeof(start), 4)) {
		printf("SWEET AUDIO: AFE start not sent\n");
		return;
	}
	for (ms = 0; ms < 2000; ms += 10) {
		while (glink_rx_one()) {
		}
		audio_wait(10);
	}
	printf("SWEET AUDIO: AFE 0x1036 start sent (tx %u rx %u)\n",
	    glink_tx_msgs, glink_rx_msgs);
}

/*
 * F6 control plane, step 1: ask ADM for a playback device path (COPP) to the
 * AFE port. The response is ADM_CMDRSP_DEVICE_OPEN_V5 and carries the COPP in
 * its first word (the raw dump below shows the payload).
 */
static void
audio_adm_open(void)
{
	struct {
		uint16_t flags, mode, endpoint1, endpoint2;
		uint32_t topology;
		uint16_t channels, bits;
		uint32_t rate;
		uint8_t map[8];
	} __attribute__((packed)) open;
	uint32_t ms;

	if (audio_step < 11) {
		return;
	}
	audio_announce("ADM device open (COPP) for INT4_MI2S_RX 0x1036");
	memset(&open, 0, sizeof(open));
	open.mode = ADM_PATH_PLAYBACK;
	open.endpoint1 = AFE_PORT_INT4_MI2S_RX;
	open.endpoint2 = 0xffff;
	open.topology = ADM_TOPOLOGY_NONE;
	open.channels = 2;
	open.bits = 24;
	open.rate = 48000;
	open.map[0] = 1;                /* PCM_CHANNEL_FL */
	open.map[1] = 2;                /* PCM_CHANNEL_FR */
	if (!glink_send_apr_pkt(APR_SVC_ADM, 0, 0, ADM_CMD_DEVICE_OPEN_V5,
	    &open, sizeof(open), 5)) {
		printf("SWEET AUDIO: ADM open not sent\n");
		return;
	}
	for (ms = 0; ms < 1500; ms += 10) {
		while (glink_rx_one()) {
		}
		audio_wait(10);
	}
	printf("SWEET AUDIO: ADM open for 0x1036 sent (tx %u rx %u)\n",
	    glink_tx_msgs, glink_rx_msgs);
}

/*
 * F6 control plane, step 2: open the ASM write stream, route session 1 to the
 * ADM COPP, set the 16-bit 48 kHz stereo format and run it. No buffer yet.
 */
static void
audio_asm_route(void)
{
	struct {
		uint32_t flags;
		uint16_t sink, bits;
		uint32_t topology, format;
	} __attribute__((packed)) open = {
		0, 0, 16, ASM_TOPOLOGY_NONE,
		ASM_MEDIA_FMT_MULTI_CHANNEL_PCM_V2
	};
	struct {
		uint32_t matrix, sessions;
		uint16_t session, copps;
		uint16_t copp[2];
	} __attribute__((packed)) route = {
		ADM_MATRIX_AUDIO_RX, 1, 1, 1, { 0, 0 }
	};
	struct {
		uint32_t size;
		uint16_t channels, bits;
		uint32_t rate;
		uint16_t is_signed, reserved;
		uint8_t map[8];
	} __attribute__((packed)) fmt = {
		20, 2, 16, 48000, 1, 0, { 1, 2 }
	};
	uint32_t run[3] = { 0, 0, 0 };
	uint32_t ms;

	if (audio_step < 12) {
		return;
	}
	audio_announce("ASM stream open, ADM route, media format and run");
	route.copp[0] = (uint16_t)adm_copp;
	(void)glink_send_apr_pkt(APR_SVC_ASM, STREAM_PORT, STREAM_PORT,
	    ASM_STREAM_CMD_OPEN_WRITE_V3, &open, sizeof(open), 6);
	for (ms = 0; ms < 300; ms += 10) {
		while (glink_rx_one()) {
		}
		audio_wait(10);
	}
	(void)glink_send_apr_pkt(APR_SVC_ADM, 0, 0,
	    ADM_CMD_MATRIX_MAP_ROUTINGS_V5, &route, sizeof(route), 7);
	for (ms = 0; ms < 300; ms += 10) {
		while (glink_rx_one()) {
		}
		audio_wait(10);
	}
	(void)glink_send_apr_pkt(APR_SVC_ASM, STREAM_PORT, STREAM_PORT,
	    ASM_DATA_CMD_MEDIA_FMT_UPDATE_V2, &fmt, sizeof(fmt), 8);
	for (ms = 0; ms < 300; ms += 10) {
		while (glink_rx_one()) {
		}
		audio_wait(10);
	}
	(void)glink_send_apr_pkt(APR_SVC_ASM, STREAM_PORT, STREAM_PORT,
	    ASM_SESSION_CMD_RUN_V2, run, sizeof(run), 9);
	for (ms = 0; ms < 1000; ms += 10) {
		while (glink_rx_one()) {
		}
		audio_wait(10);
	}
	printf("SWEET AUDIO: ASM/ADM route done (copp %x, tx %u rx %u)\n",
	    adm_copp, glink_tx_msgs, glink_rx_msgs);
}

/*
 * F4: PM660L analog codec read-only probe. The codec is the SPMI device at
 * SID 3, base 0xf000 (vendor msm-analog-cdc.c); the lavender observer read
 * path is extended for that peripheral in sweet_spmi.c (0510).
 */
static void
audio_pm660l(void)
{
	static const struct {
		uint16_t reg;
		const char *name;
	} regs[] = {
		{ 0x0000, "DIG_REV1" }, { 0x0001, "DIG_REV2" },
		{ 0x0004, "DIG_TYPE" }, { 0x0005, "DIG_SUBTYPE" },
		{ 0x0046, "DIG_RST_CTL" }, { 0x0048, "DIG_TOP_CLK" },
		{ 0x0049, "DIG_ANA_CLK" }, { 0x004A, "DIG_DIG_CLK" },
		{ 0x0100, "ANA_REV1" }, { 0x0101, "ANA_REV2" },
		{ 0x0102, "ANA_REV3" }, { 0x0103, "ANA_REV4" },
		{ 0x0104, "ANA_TYPE" }, { 0x0105, "ANA_SUBTYPE" },
		{ 0x0140, "MICB_1_EN" }, { 0x0144, "MICB_2_EN" },
		{ 0x0193, "RX_BIAS_DAC" }, { 0x0197, "HPH_CNP_EN" },
		{ 0x01B0, "SPKR_DAC_CTL" }, { 0x01B5, "SPKR_PWRSTG" },
		{ 0x01B6, "SPKR_DRV_MISC" }, { 0x01C0, "CURRENT_LIMIT" },
		{ 0x01C1, "OUTPUT_VOLTAGE" }, { 0x01C3, "BOOST_EN_CTL" },
	};

	if (audio_step < 8) {
		return;
	}
	audio_announce("read the PM660L analog codec over SPMI (SID 3, 0xf000)");
	for (uint32_t i = 0; i < sizeof(regs) / sizeof(regs[0]); i++) {
		uint8_t v = 0;
		int err = sweet_spmi_lavender_read(3, 0xf000u + regs[i].reg,
		    &v);

		printf("SWEET AUDIO: PM660L %-14s [%04x] = %02x (%d)\n",
		    regs[i].name, regs[i].reg, v, err);
		audio_wait(20);
	}
}

/*
 * F4 write path test: the PM660L DIG spare registers (0xf070..0xf072, POR 0)
 * are the only writes the guarded SPMI helper allows for now. Write 0x5a,
 * read it back, then restore the old value and read it again.
 */
static void
audio_pm660l_write(void)
{
	uint8_t orig = 0, after = 0, restored = 0;
	int err;

	if (audio_step < 9) {
		return;
	}
	audio_announce("PM660L spare write/read-back/restore (SID 3 0xf070)");
	err = sweet_spmi_lavender_read(3, 0xf070u, &orig);
	printf("SWEET AUDIO: PM660L spare read %02x (%d)\n", orig, err);
	if (err != 0) {
		return;
	}
	err = sweet_spmi_lavender_write(3, 0xf070u, 0x5au);
	printf("SWEET AUDIO: PM660L spare write 5a (%d)\n", err);
	(void)sweet_spmi_lavender_read(3, 0xf070u, &after);
	err = sweet_spmi_lavender_write(3, 0xf070u, orig);
	printf("SWEET AUDIO: PM660L spare restore %02x (%d)\n", orig, err);
	(void)sweet_spmi_lavender_read(3, 0xf070u, &restored);
	printf("SWEET AUDIO: PM660L spare readback %02x restore %02x\n",
	    after, restored);
}

static void
audio_q6ss(void)
{
	if (!lpass_on) {
		printf("SWEET AUDIO: LPASS off; not touching the Q6SS at %08x\n",
		    Q6SS_PA);
		return;
	}
	audio_announce("read the ADSP Q6SS registers (domain is on)");
	printf("SWEET AUDIO: q6 reset %08x gfmux %08x pwr %08x mem_pwr %08x\n",
	    audio_rd(q6ss, Q6SS_RESET), audio_rd(q6ss, Q6SS_GFMUX_CTL),
	    audio_rd(q6ss, Q6SS_PWR_CTL), audio_rd(q6ss, Q6SS_MEM_PWR_CTL));
	printf("SWEET AUDIO: q6 xo_cbcr %08x sleep %08x bhs %08x bhs62 %08x "
	    "mem_pwr2 %08x\n", audio_rd(q6ss, Q6SS_XO_CBCR),
	    audio_rd(q6ss, Q6SS_SLEEP), audio_rd(q6ss, Q6SS_BHS_STATUS),
	    audio_rd(q6ss, Q6V62_BHS_STATUS),
	    audio_rd(q6ss, Q6SS_MEM_PWR_CTL2));
}

static const char *
audio_smmu_type(uint32_t s2cr)
{
	switch ((s2cr >> 16) & 3u) {
	case 0:
		return "translate";
	case 1:
		return "bypass";
	case 2:
		return "fault";
	default:
		return "reserved";
	}
}

static void
audio_lpass_smmu(void)
{
	uint32_t cbcr = audio_rd(gcc, GCC_LPASS_SMMU_CBCR);

	if (cbcr & CBCR_OFF) {
		printf("SWEET AUDIO: LPASS SMMU vote clock is off; not reading "
		    "0x%08x\n", LPASS_SMMU_PA);
		return;
	}
	audio_announce("read the LPASS Q6 SMMU global registers");
	printf("SWEET AUDIO: lpass smmu sCR0 %08x sACR %08x IDR0 %08x IDR1 %08x "
	    "IDR7 %08x\n", audio_rd(smmu, SMMU_SCR0), audio_rd(smmu, SMMU_SACR),
	    audio_rd(smmu, SMMU_IDR0), audio_rd(smmu, SMMU_IDR1),
	    audio_rd(smmu, SMMU_IDR7));
	printf("SWEET AUDIO: lpass smmu sGFAR %08x sGFSR %08x\n",
	    audio_rd(smmu, SMMU_SGFAR), audio_rd(smmu, SMMU_SGFSR));
	for (uint32_t i = 0; i < 16; i++) {
		uint32_t smr = audio_rd(smmu, SMMU_SMR(i));
		uint32_t s2cr;

		if (!(smr & SMMU_SMR_VALID)) {
			continue;
		}
		s2cr = audio_rd(smmu, SMMU_S2CR(i));
		printf("SWEET AUDIO: lpass SMR[%u] %08x sid %04x -> S2CR %08x "
		    "%s cb %u\n", i, smr, smr & 0x7fffu, s2cr,
		    audio_smmu_type(s2cr), s2cr & 0xffu);
	}
}

static void
audio_lpass_blocks(void)
{
	if (!lpass_on) {
		printf("SWEET AUDIO: LPASS off; skipping codec/pinctrl/MCLK/"
		    "AVTIMER reads\n");
		return;
	}
	audio_announce("read the LPASS digital codec and SoundWire block");
	printf("SWEET AUDIO: codec %08x %08x %08x %08x swr %08x %08x %08x %08x\n",
	    audio_rd(codec, 0x000), audio_rd(codec, 0x004),
	    audio_rd(codec, 0x008), audio_rd(codec, 0x00c),
	    audio_rd(codec, 0x1000), audio_rd(codec, 0x1004),
	    audio_rd(codec, 0x1008), audio_rd(codec, 0x100c));
	audio_announce("read the LPASS LPI pinctrl, MCLK and AVTIMER registers");
	printf("SWEET AUDIO: lpi pinctrl %08x %08x %08x %08x\n",
	    audio_rd(pinctrl, 0x000), audio_rd(pinctrl, 0x004),
	    audio_rd(pinctrl, 0x008), audio_rd(pinctrl, 0x00c));
	printf("SWEET AUDIO: mclk_reg %08x, avtimer %08x %08x\n",
	    audio_rd(mclk, 0x018), audio_rd(avtimer, 0x00c),
	    audio_rd(avtimer, 0x010));
}

static void
audio_smp2p_item(const uint8_t *data, uint32_t len)
{
	const struct smp2p_hdr *h = (const struct smp2p_hdr *)data;
	uint32_t fv, rl, vt, version, features, local, remote, total, valid;

	if (len < sizeof(*h)) {
		return;
	}
	fv = h->feature_version;
	rl = h->rem_loc_proc_id;
	vt = h->valid_total_ent;
	version = fv & 0xffu;
	features = fv >> 8;
	local = rl & 0xffffu;
	remote = rl >> 16;
	total = vt & 0xffffu;
	valid = vt >> 16;
	printf("SWEET AUDIO: smp2p v%u features %06x local %u remote %u total "
	    "%u valid %u flags %08x\n", version, features, local, remote,
	    total, valid, h->flags);
	for (uint32_t i = 0; i < valid && i < SMP2P_MAX_ENTRY &&
	    sizeof(*h) + (i + 1) * sizeof(struct smp2p_entry) <= len; i++) {
		const struct smp2p_entry *e =
		    (const struct smp2p_entry *)(data + sizeof(*h) +
		    i * sizeof(*e));
		char name[sizeof(e->name) + 1];

		memcpy(name, e->name, sizeof(e->name));
		name[sizeof(e->name)] = '\0';
		printf("SWEET AUDIO:   entry %u '%s' = %08x\n", i, name,
		    e->value);
	}
}

static void
audio_smem_part(const char *tag, const struct smem_toc_entry *te)
{
	vm_offset_t base = smem + te->offset;
	const struct smem_part *p = (const struct smem_part *)base;
	uint32_t used = 0, items = 0;

	if (te->size < sizeof(*p) || memcmp(p->magic, "$PRT", 4) != 0) {
		printf("SWEET AUDIO: %s %u/%u @%05x size %05x: no $PRT header\n",
		    tag, (unsigned)te->host0, (unsigned)te->host1, te->offset,
		    te->size);
		return;
	}
	printf("SWEET AUDIO: %s %u/%u @%05x size %05x used %05x cached %05x\n",
	    tag, (unsigned)p->host0, (unsigned)p->host1, te->offset, p->size,
	    p->free_uncached, p->free_cached);
	if (p->free_uncached > p->size || p->free_uncached < sizeof(*p)) {
		printf("SWEET AUDIO: %s: bad free pointer %05x\n", tag,
		    p->free_uncached);
		return;
	}
	while (used + sizeof(struct smem_item) <= p->free_uncached) {
		const struct smem_item *h = (const struct smem_item *)
		    (base + sizeof(*p) + used);
		const uint8_t *data;
		uint32_t payload, step, magic;

		if (h->canary != SMEM_ITEM_CANARY) {
			printf("SWEET AUDIO: %s: bad canary %04x at %05x\n", tag,
			    (unsigned)h->canary, used);
			return;
		}
		if (h->pad_data > h->size || h->size == 0) {
			printf("SWEET AUDIO: %s: bad item %u size %u pad %u\n",
			    tag, (unsigned)h->item, h->size,
			    (unsigned)h->pad_data);
			return;
		}
		payload = h->size - h->pad_data;
		step = sizeof(*h) + h->pad_hdr + h->size;
		data = (const uint8_t *)h + sizeof(*h) + h->pad_hdr;
		{
			const char *what = NULL;

			switch (h->item) {
			case SMEM_ITEM_SMP2P_OUT:
				what = " smp2p apps->adsp";
				break;
			case SMEM_ITEM_SMP2P_IN:
				what = " smp2p adsp->apps";
				break;
			case SMEM_ITEM_GLINK_DESC:
				what = " glink descriptor";
				break;
			case SMEM_ITEM_GLINK_FIFO0:
				what = " glink fifo0";
				break;
			case SMEM_ITEM_GLINK_FIFO1:
				what = " glink fifo1";
				break;
			}
			printf("SWEET AUDIO: %s item %u%s size %u payload %u\n",
			    tag, (unsigned)h->item, what != NULL ? what : "",
			    h->size, payload);
		}
		memcpy(&magic, data, sizeof(magic));
		if (payload >= sizeof(struct smp2p_hdr) &&
		    magic == SMP2P_MAGIC) {
			audio_smp2p_item(data, payload);
		}
		items++;
		used += step;
		if (items >= 32) {
			printf("SWEET AUDIO: %s: stopped after 32 items\n", tag);
			return;
		}
	}
}

static void
audio_smem(void)
{
	const struct smem_toc *toc = (const struct smem_toc *)
	    (smem + SMEM_TOC_OFF);
	const struct smem_toc_entry *te = (const struct smem_toc_entry *)
	    (toc + 1);
	uint32_t found = 0;

	audio_announce("read the SMEM header and partition table (read only)");
	printf("SWEET AUDIO: smem header %08x %08x %08x %08x\n",
	    audio_rd(smem, 0x00), audio_rd(smem, 0x04), audio_rd(smem, 0x08),
	    audio_rd(smem, 0x0c));
	if (memcmp(toc->magic, "$TOC", 4) != 0 || toc->num_entries >
	    SMEM_TOC_ENTRIES) {
		printf("SWEET AUDIO: no SMEM $TOC at %08x (magic %c%c%c%c, "
		    "entries %u)\n", SMEM_PA + SMEM_TOC_OFF, toc->magic[0],
		    toc->magic[1], toc->magic[2], toc->magic[3],
		    toc->num_entries);
		return;
	}
	printf("SWEET AUDIO: smem TOC v%u, %u entries\n", toc->version,
	    toc->num_entries);
	for (uint32_t i = 0; i < toc->num_entries; i++) {
		const struct smem_toc_entry *e = &te[i];

		if (e->offset == 0 || e->size == 0 || e->offset >= SMEM_SIZE) {
			continue;
		}
		found++;
		if (e->host0 == SMEM_APPS_HOST && e->host1 == SMEM_ADSP_PID) {
			audio_smem_part("apps<->adsp", e);
		} else {
			printf("SWEET AUDIO: smem part %u/%u @%05x size %05x\n",
			    (unsigned)e->host0, (unsigned)e->host1, e->offset,
			    e->size);
		}
	}
	printf("SWEET AUDIO: smem TOC holds %u partitions\n", found);
}

static void
audio_apcs(void)
{
	audio_announce("read the APCS IPC register for the ADSP bits");
	printf("SWEET AUDIO: apcs ipc %08x (glink bit9 %u, smp2p bit10 %u)\n",
	    audio_rd(apcs, 0x008), (audio_rd(apcs, 0x008) >> 9) & 1u,
	    (audio_rd(apcs, 0x008) >> 10) & 1u);
}

static void
lavender_audio_thread(__unused void *arg, __unused wait_result_t wr)
{
	uint32_t delay_s = 25;

	(void)PE_parse_boot_argn("lavauddelay", &delay_s, sizeof(delay_s));
	if (delay_s > 300) {
		delay_s = 300;
	}
	printf("SWEET AUDIO: A0/A1 probe (lavaud=%u) in %u s\n", audio_step,
	    delay_s);
	audio_wait(delay_s * 1000);

	gcc = ml_io_map(GCC_PA, GCC_SIZE);
	if (gcc == 0) {
		printf("SWEET AUDIO: GCC map failed\n");
		return;
	}
	audio_gcc();

	if (audio_step >= 2) {
		(void)audio_power_up();
	}

	q6ss = ml_io_map(Q6SS_PA, Q6SS_SIZE);
	if (q6ss != 0) {
		audio_q6ss();
	} else if (lpass_on) {
		printf("SWEET AUDIO: Q6SS map failed\n");
	}

	smmu = ml_io_map_unmappable(LPASS_SMMU_PA, LPASS_SMMU_SIZE,
	    VM_WIMG_IO);
	if (smmu != 0) {
		audio_lpass_smmu();
	} else {
		printf("SWEET AUDIO: LPASS SMMU map failed\n");
	}

	codec = ml_io_map(LPASS_CODEC_PA, LPASS_CODEC_SIZE);
	pinctrl = ml_io_map(LPASS_PINCTRL_PA, LPASS_PINCTRL_SIZE);
	mclk = ml_io_map(LPASS_MCLK_PA, LPASS_MCLK_SIZE);
	avtimer = ml_io_map(LPASS_AVTIMER_PA, LPASS_AVTIMER_SIZE);
	if (codec != 0 && pinctrl != 0 && mclk != 0 && avtimer != 0) {
		audio_lpass_blocks();
	} else {
		printf("SWEET AUDIO: LPASS block map failed (codec %d pinctrl %d "
		    "mclk %d avtimer %d)\n", codec != 0, pinctrl != 0, mclk != 0,
		    avtimer != 0);
	}

	smem = ml_io_map_unmappable(SMEM_PA, SMEM_SIZE, VM_WIMG_IO);
	if (smem != 0) {
		audio_smem();
	} else {
		printf("SWEET AUDIO: SMEM map failed\n");
	}

	apcs = ml_io_map(APCS_IPC_PA, APCS_IPC_SIZE);
	if (apcs != 0) {
		audio_apcs();
	} else {
		printf("SWEET AUDIO: APCS map failed\n");
	}

	if (audio_step >= 3) {
		audio_firmware();
	}
	if (audio_step >= 4) {
		audio_pas_load();
	}
	if (audio_step >= 5) {
		audio_adsp_inventory();
	}
	if (audio_step >= 6) {
		audio_smp2p_out_init();
	}
	if (audio_step == 6) {
		audio_smp2p_wait_ready();
	}
	if (audio_step >= 7) {
		/* The ADSP only fills 443 after the 429 kick; give it a moment
		 * and go straight to GLINK (the wait loop is diagnostic only). */
		audio_wait(3000);
		audio_glink_apr();
	}
	if (audio_step >= 8) {
		audio_pm660l();
	}
	if (audio_step >= 9) {
		audio_pm660l_write();
	}
	if (audio_step >= 10) {
		audio_afe_port();
	}
	if (audio_step >= 11) {
		audio_adm_open();
	}
	if (audio_step >= 12) {
		audio_asm_route();
	}

	printf("SWEET AUDIO: probe done (lavaud=%u)\n", audio_step);
}

int
lavender_audio_probe(void)
{
	static int started;
	uint32_t step = 0, force = 0, copy = 1, probe = 0;
	thread_t thread;

	if (started ||
	    !PE_parse_boot_argn("lavaud", &step, sizeof(step)) || step == 0) {
		return 0;
	}
	started = 1;
	audio_step = step;
	(void)PE_parse_boot_argn("lavaudforce", &force, sizeof(force));
	audio_force = force != 0;
	(void)PE_parse_boot_argn("lavaudcopy", &copy, sizeof(copy));
	audio_copy = copy != 0;
	(void)PE_parse_boot_argn("lavaudprobe", &probe, sizeof(probe));
	audio_probe = probe != 0;
	if (kernel_thread_start(lavender_audio_thread, NULL, &thread) ==
	    KERN_SUCCESS) {
		thread_deallocate(thread);
	}
	return 0;
}

#else /* !ARM64_BOARD_CONFIG_LAVENDER */

int lavender_audio_probe(void);
int
lavender_audio_probe(void)
{
	return 0;
}

#endif
