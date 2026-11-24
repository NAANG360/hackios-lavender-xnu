# audio on lavender (SDM660): what the hardware is, and the ladder

Status: research brief for the `0500`–`0599` series. Written 2026-11-21 after the
GPU (`0236`–`0253`) and Wi-Fi/modem (`0700`–`0706`) hard-path series. The phone's own
`lavaud=3` boot has since run the read-only probe and the firmware dry run; the device
facts below come from the phone's own device tree, its stock vendor image and the port's
existing drivers.

## 1. The premise was wrong: there is no WCD9335 and no SLIMbus audio

The repo's `docs/HARDWARE.md` / `docs/ROADMAP.md` said "no codec driver; the
PM660/PM660L audio path needs bring-up", which is right, but it is easy to read that
as "port the WCD9335". The phone's own DTBO says otherwise:

- `artifacts/backup/dtbo.img` (the live partition) contains exactly one overlay:
  `fragment@0` targeting `slim_aud` with `status = "disabled"`.
  `python3 tools/fdtdump.py` on `work/dtbo0.bin` shows it. So the SLIMbus audio
  controller at `0x151c0000` and both `tasha` (WCD9335) and `tavil` (WCD9341) nodes
  are **disabled on this board**. Do not bring up SLIMbus, and do not port
  `wcd9335.c`; there is no such codec.
- The enabled sound card in the same DTB (`/soc/sound`, `status = "okay"`) is
  `qcom,sdm660-asoc-snd`, model `sdm660-snd-card`, `qcom,wsa-disable`, and its
  codecs are `msm-stub-codec`, **`msm-dig-codec`**, **`analog-codec`**,
  `msm_sdw_codec` and `msm-ext-disp-audio-codec-rx`.
- `analog-codec` is `/soc/qcom,spmi@800f000/qcom,pm660l@3/analog-codec@f000`:
  compatible `qcom,pmic-analog-codec`, `reg = <0xf000 0x200>`, SPMI interrupts
  `spk_cnp_int`, `spk_clip_int`, `spk_ocp_int`, MBHC/HPH/ear OCP, supplies
  `cdc-vdda-cp`/`cdc-vdd-pa` (PM660L BOB) and `cdc-vdd-mic-bias`, MCLK 9.6 MHz.
  **The PM660L itself is the codec, including the speaker amplifier.** There is no
  external smart PA in the audio path.
- `msm-dig-codec` is `/soc/.../analog-codec@f000/msm-dig-codec`, `reg = <0x152c0000 0>`:
  the LPASS digital codec block. Its SoundWire master is
  `/soc/msm-sdw-codec@152c1000` with the `wsa881x_en` children; with
  `qcom,wsa-disable` those WSA amplifiers are not fitted, but the SoundWire link
  between the LPASS digital codec and the PM660L codec is what carries the PCM.
- Extra LPASS nodes: `lpi_pinctrl@15070000` (codec reset on LPI GPIO 24, MCLK on
  LPI GPIO 18), `audio_ext_clk` with `qcom,mclk-clk-reg = <0x15020018>`, and
  `avtimer@150f700c`.

## 2. The real audio map (lavender)

| Piece | Address / node | Notes |
| --- | --- | --- |
| ADSP (LPASS Hexagon, QDSP6) | `qcom,lpass@15700000`, PAS id **1** | `qcom,pil-tz-generic`, firmware `adsp`, SMEM id `0x1a7`, XO 19.2 MHz, `qcom,proxy-timeout-ms = 10000` |
| ADSP carve-out | `0x92a00000`, **30 MiB** (`0x1e00000`) | `/reserved-memory/adsp_fw_region@92a00000`; the modem carve-out ends at `0x92a00000` |
| ADSP Q6SS registers | `0x15700000` (`reg = <0x15700000 0x100>`) | reset `0x014`, GFMUX `0x020`, PWR `0x030`, sleep `0x03c`, XO CBCR `0x038`, BHS `0x078`, v62 BHS `0x0c4` |
| LPASS power/vote | GCC `0x7d034` GDSC `hlos1_vote_lpass_adsp`, `0x7d014` clock `hlos1_vote_lpass_adsp_smmu` | the only LPASS controls the GCC driver exposes on SDM660 |
| LPASS SMMU | `/soc/arm,smmu-lpass_q6@5100000` | SMMUv2 register layout at `0x05100000` |
| LPASS digital codec | `0x152c0000`; master `0x152c1000` | `msm-dig-codec` + `msm_sdw_codec` |
| Codec (PM660L) | SPMI SID **3**, base `0xf000`, 0x200 bytes | analog codec + MBHC + speaker OCP; supplies on BOB |
| SMEM | `0x86000000`, 2 MiB | TOC at `0x861ff000` (`$TOC`), partitions `$PRT` |
| APCS IPC | `0x17911000 + 0x8` | GLINK mask `0x200` (bit 9), SMP2P mask `0x400` (bit 10) |
| SMP2P/GLINK IRQs | ADSP SMP2P SPI `0x9e`, GLINK SPI `0x9d` | `qcom,smp2p-adsp@17911008`, `qcom,glink-smem-native-xprt-adsp` |
| SMP2P/GLINK SMEM items | APPS base **427**, AUDIO base **443**: AP→ADSP **429**, ADSP→AP **443**; GLINK **478/479/480** | vendor `include/soc/qcom/smem.h` in `lavender-q-oss` |
| Codec reset | LPI GPIO 24 (`lpi_pinctrl@15070000`) | not TLMM GPIO 64; node name is misleading |
| MCLK | LPI GPIO 18 func2, register `0x15020018` | 9.6 MHz (`0x927c00`) |
| Primary MI2S pins | TLMM GPIO 12/13/14/15 | `pri_mi2s_*`, used by the reference TDM node, **not** by lavender's speaker |

### AFE ports (from the stock vendor image, not guesswork)

`work/audio_platform_info_intcodec.xml`, extracted from
`vendor.img` (`work/gpu-fw/.../images/vendor.img`), says:

- headphones / handset / line: `INT0_MI2S_RX`;
- speaker, speaker_external_1/2, speaker_vbat: **`INT4_MI2S_RX`**;
- built-in mic: `INT3_MI2S` (internal codec) or `SLIMBUS_0` (external-codec variant);
- speaker mixer path (`/vendor/etc/mixer_paths*.xml`): `INT4_MI2S_RX Audio Mixer
  MultiMedia1`, `INT4_MI2S_RX Channels Two`, `RX4 MIX1 INP1: RX4`, `RX5 MIX1 INP1:
  RX5`, `RX4/RX5 Digital Volume 67`, `COMP1/COMP2 Switch`, `SpkrLeft/Right BOOST
  Switch`, `SpkrLeft/Right VISENSE Switch`, `SpkrLeft/Right SWR DAC_Port Switch`.
  That is the PM660L speaker path, fed by the LPASS SoundWire DAC port; no
  `TAS2557`/`TFA98xx`/`MAX98927`/`WSA881x` control appears.

Consequence: the audio data path is
`ASM stream -> ADM COPP -> AFE INT4_MI2S_RX -> LPASS digital codec/SoundWire ->
PM660L analog codec (SWR DAC + SPK boost) -> speaker`.
The AP configures the PM660L over SPMI and the LPASS digital codec/SWR block; the
ADSP runs the AFE port and the PCM flow. This is the same shape as Android's
`msm-pcm`/`q6asm`/`q6adm`/`q6afe` + `msm-analog-codec` + `msm-dig-codec` stack.

## 3. What iOS side needs

There is no Apple `IOAudio2` device on this port: the iOS 16 kernelcache has the
closed `AppleHDA`/`AppleAOPAudioController` stack and none of it is in the
open-source XNU build. The practical path, already proven by `ios-topaz`
(`userland/audio/audio_plugin.c`, `audio_plugin.plist`) and by HakiOS
(`hakios.audio.c`), is:

1. a kernel `/dev/<board>-audio` node: 48 kHz stereo S16_LE frames from the
   mediaserverd HAL plug-in;
2. a userland AudioServer plug-in in `/System/Library/Audio/Plug-Ins/HAL` that
   presents `Speaker` + `Hawking` (+ `Codec`, + `Actuator`) to the iOS virtual
   audio device and writes the mixed frames to that node;
3. the kernel takes those frames into the AFE path above.

The topaz plug-in is the template; only the device name and the `/dev` node change.
For a first audible milestone the kernel can play a fixed test tone without the
plug-in (like `hakios.sound.c` `sound=2/3`), which is how the GPU series proved
CP/RB before any Metal driver.

## 4. Reference material: what is reusable, what is not

| Source | Reusable | Not reusable |
| --- | --- | --- |
| `refs/ios-topaz/xnu/patches/0144-topaz-audio-adsp.patch` | TrustZone PAS sequence (`INIT_IMAGE 1`, `MEM_SETUP 2`, `AUTH_AND_RESET 5`), MDT/ELF segment loader, SMP2P item layout, GLINK channel/queue implementation, "never boot over stale SMEM state" rule | addresses (SM6225 `0x53800000` carve-out), GPR/AudioReach protocol (`APM GET_SPF_STATE`); the SM6225 SMP2P header layout is **not** SDM660's (see below) |
| `refs/ios-topaz/xnu/patches/0145`, `0146` | codec/stream step organisation, RPM vote helper (TopazRpm), tone test shape | WCD937x over SoundWire, sia8159 amp, SM6225 RPMh |
| `refs/ios-lavender/drivers/audio/hakios/hakios.adsp.c`, `hakios.sound.c` | **APR** channel (`apr_audio_svc`), ASM/ADM/AFE command shapes, internal-codec AFE port config, SMMU SID mask trick, test-tone/stream setup | SM6375/POCO X5 Pro addresses, WCD938x/SoundWire bolero macros, AW88261 amp, virtio-sound |
| this repo `0700`/`lavender_wlan.c`, `0701`–`0706`/`lavender_modem.c` | the read-only probe format (announce + 2 s), GCC branch/GDSC read, SCM call convention (`scm_call`), Q6SS/RMB state read, **FAT16 modem-partition firmware reader**, eMMC read lock/retries | MSS-specific reset (ADSP uses PAS/TZ, not the MSS restart) |
| this repo `0240`/`0241` GPU patches | working SCM PIL calls for PAS 13 (zap), firmware file read from the iOS root | PAS id 13, GPU addresses |
| `MiCode/Xiaomi_Kernel_OpenSource` branch `lavender-q-oss` | the phone's own `sound/soc/codecs/wcd9335.c` is not used, but `msm-analog-codec`-equivalent registers, `msm-dig-codec`, `msm_sdw_codec`, `msm-pcm-q6` and the `tfa98xx` tree are the vendor reference | it is Android 4.4 ASoC; not portable code |
| Linux `drivers/soc/qcom/subsys-pil-tz.c`, `pil-q6v5.c`, `smp2p.c`, `glink_smem_native_xprt.c` | exact PAS command numbers/arginfo, Q6SS offsets, SMP2P item layout, APCS kick semantics | none of it runs on XNU as-is |
| Linux `drivers/clk/qcom/gcc-sdm660.c` | `hlos1_vote_lpass_adsp` GDSC `0x7d034`, SMMU vote `0x7d014`, branch/GDSC bit meanings | LPASS internal clocks are not in the GCC |

## 5. The ladder (0500–0599, one patch at a time)

Each patch is boot-arg gated, off by default, and advances one layer. Numbers are
reserved as we go so another agent cannot collide (this series uses 0500+).

| Patch | What | Boot arg | Risk / gate |
| --- | --- | --- | --- |
| `0500` | read-only platform probe: GCC LPASS GDSC/vote, Q6SS registers, LPASS SMMU, digital codec/AVTIMER/MCLK, SMEM TOC + ADSP partition items (discovers SMP2P/GLINK item IDs), APCS IPC | `lavaud=1` | reading a clock-gated LPASS block can hang the bus: skip the block unless the LPASS GDSC reports on |
| `0501` | LPASS/ADSP power-up: vote `hlos1_vote_lpass_adsp_smmu`, un-collapse the LPASS ADSP GDSC, wait for PWR_ON, re-read Q6SS; no firmware | `lavaud=2` | if the GDSC never comes on, stop (do not touch Q6SS) |
| `0502` | ADSP firmware read: FAT16 `image/adsp.mdt` + `adsp.bNN` from the modem partition, MDT hash-table parse, size/range check against `0x92a00000..0x94800000`; no TZ call yet | `lavaud=3` | read-only; validates the loader before any write |
| `0503` | TrustZone PAS load: copy segments, `INIT_IMAGE`, `MEM_SETUP`, `AUTH_AND_RESET` (PAS 1), wait for ADSP PBL/err-ready | `lavaud=4` | never if SMEM shows an earlier run; `adsp_stop=1` shuts down first |
| `0504` | post-PAS inventory: wait for the ADSP SMP2P item, dump the apps↔ADSP partition and the GLINK candidates (read only) | `lavaud=5` | only after 0503 says the DSP is up |
| `0505` | fix the SMP2P **v1 packed header** (SDM660 vendor layout) and print the fixed ids 429/443/478/479/480 (read only) | `lavaud=5` | none; the 0500/0504 parser used the SM6225 layout and would have mis-decoded this SoC |
| `0506` | SMP2P out item 429 + `slave-kernel` READY, GLINK `lpass` edge 478/479/480 (first SMEM writes) | `lavaud=6` | only with the exact ids and after the inventory |
| `0507` | power-order fix from the first device log: un-collapse the LPASS ADSP GDSC before voting the SMMU branch; the old order returned at `80000001` (EN, OFF) | `lavaud=3` | if the GDSC still does not report PWR_ON, the CX rail/voter is missing |
| `0508` | PAS metadata/hash fixes from the same log: hash phdr by p_flags at phdr+24 (PT_NULL), external `ADSP.Bnn` hash blob, header+phdrs+hash for `INIT_IMAGE`, SCM x1 status, entry/BSS checks, no Q6SS read when the domain is off | `lavaud=4` | refuses PAS without a readable hash or with an entry outside every segment |
| `0509` | GLINK `lpass` edge and APR `apr_audio_svc`: enumerate AFE/ASM/ADM services; no audio yet | `lavaud=7` | polling only, no interrupts; needs a READY ADSP |
| `0510` | PM660L codec bring-up over SPMI: ID/revision, supplies, codec reset (LPI GPIO 24), MCLK 9.6 MHz, MBHC/HPH/SPK blocks; read back | `lavaud=8` | codec writes after supplies are verified; keep `cdc-vdd-pa`/BOB state |
| `0511` | LPASS digital codec + SWR master + AFE `INT4_MI2S_RX` port: bit clock/frame sync, SWR frame shape, port started | `lavaud=9` | must not touch LPASS registers while the DSP owns the block; check 0509 first |
| `0512` | ASM stream + ADM COPP to `INT4_MI2S_RX`; 1 kHz stereo tone, speakers muted/enabled per mixer sequence | `lavaud=10` | audible; keep volume at minimum, monitor codec OCP |
| `0513` | `/dev/lavender-audio` sink and the topaz-derived HAL plug-in (Speaker/Hawking/Codec); mediaserverd frames | `lavaud=11` | userland install on the root image; no Apple binaries |
| `0514`+ | microphone (INT3_MI2S_TX/ADC), jack detection/MBHC, volume, suspend | | after the speaker path works |

`0500` is deliberately the same shape as `0236` (GPU step 1) and `0700` (WLAN F0b):
it only reads, announces every access 2 s ahead, and logs enough to decide the next
step without another blind boot.

## 6. What the first device run answered

The `0505` kernel with `lavaud=3` ran the probe and the firmware dry run to the end
without a hang (`work/wlan/audio-0505-lavaud3-2026-11-21.txt`):

1. **Is the LPASS ADSP power domain on after ABL?** No: `0x7d034` read `00000001`
   (SW_COLLAPSE 1, PWR_ON 0), and the SMMU vote `0x7d014` read `80000000` (off).
   The old order voted SMMU first, which left it at `80000001` (EN set, OFF still
   set) and returned before touching the GDSC. `0507` reverses the order.
2. **Is the ADSP already running?** No. The apps↔ADSP partition is empty (`used
   0x20`, no `$SMP` item) and APCS IPC read 0. There is no earlier run to shut down.
3. **What do the fixed ids look like?** The SMEM TOC is v1 with 7 partitions and the
   vendor layout is the v1 packed SMP2P header, as 0505 implements. The ids are
   confirmed from the vendor `smem.h`; the post-PAS inventory that shows their values
   still needs a running ADSP.
4. **Is the Q6SS out of reset / does the GDSC stay on?** The Q6SS was deliberately not
   read while the domain was off. The GDSC is still collapsed after ABL.
5. **What is the LPASS SMMU state?** Not read: its vote clock was off, and 0508 keeps
   the rule that the block is only touched after the GDSC and the branch report on.
6. **Does APCS show stale ADSP bits?** No: `00000000` (GLINK bit 9 and SMP2P bit 10
   both clear).

The dry run also read `ADSP.MDT` (7964 bytes, 23 phdrs) from the modem FAT16 and found
`ADSP.B02..B21` (20 blobs, 29,018,084 bytes, 0 bad). The same firmware is on this PC at
`work/wlan/fw/NON-HLOS.bin`, which confirms the details the loader needs:

- the hash phdr is index 1, `PT_NULL`, `p_flags 0x02200000`, 7176 bytes, and the MDT is
  exactly 788 (ELF header + 23 phdrs) + 7176 (hash data): the vendor MDT is the
  concatenation, and the hash phdr's `p_offset` (4096) is not where the table lives;
- the table is the vendor 40-byte header + 32-byte digests — **not** topaz's
  `8 + 40*N` entry format — so it is opaque to the AP and only TrustZone needs it;
- phdr 22 is a pure-BSS segment (`filesz 0`, `memsz 0x23a000`) that ends exactly at
  `0x94800000`; the old walk skipped it, so MEM_SETUP did not cover the ADSP BSS.

The hash parser had two bugs the log exposed: it read `p_align` at phdr+28 instead of
`p_flags` at phdr+24, and it required `p_type == PT_LOAD` although the hash phdr is
`PT_NULL`. 0508 fixes both, assembles the metadata block for `INIT_IMAGE`, loads the BSS
segment, checks the SCM x1 result, and validates the ELF entry address.

## 7. Safety

- Read-only first, always. A wrong read of a powered-down LPASS block on this SoC
  can take the bus down without a panic; every access is announced and spaced.
- `0x92a00000` is the ADSP carve-out and `0x94800000..0x94a00000` is the MBA
  region; the modem series already proved the MSS carve-outs are usable from XNU
  (MBA alive). Do not write until the firmware loader's range check passes.
- Do not boot the ADSP over SMEM state from an earlier run: shut down (PAS 4) or
  refuse, exactly as the Windows topaz driver learned.
- The PM660L codec shares the PMIC with the keys/RTC; codec writes go through the
  existing SPMI arbiter path and only after the supplies (`BOB`, `cdc-vdd-pa`) are
  read as on.
- Tests: `tests/run_tests.sh` (patch structure + mirrors) before every commit; the
  kernel build is a CI run, then a shim and a device boot with `sweetusb=2` so the
  live log reaches `192.168.7.1:5140`.
