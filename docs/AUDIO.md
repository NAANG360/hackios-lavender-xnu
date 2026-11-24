# Audio

Status: **bring-up started (2026-11-21)**. No sound yet. Patches `0500` (read-only probe),
`0501` (LPASS/ADSP power-up), `0502` (ADSP firmware dry run), `0503` (TrustZone PAS load),
`0504` (post-PAS inventory), `0505` (SDM660 SMP2P v1 layout + vendor ids), `0506` (SMP2P
out 429 + kick), `0507` (power order), `0508` (PAS metadata/hash and SCM status) and
`0508b` (carve-out copy diagnostics) are written; `0507` run `44938672875` and `0508`
run `44942180169` are green. Two device boots have run:
the `0505` kernel with `lavaud=3` and the `0508` kernel with `lavaud=4`. The second one
proved the power fix and the loader:

- `0507` works: GDSC `00000001` -> `80000000` (on), SMMU CBCR `80000000` -> `00000001`
  (on), Q6SS/SMMU/codec readable.
- Dry run: hash phdr 1 (`PT_NULL`, 7176 B), 21 segments (incl. BSS `B22`),
  31,352,804 bytes, 0 bad; metadata 7964 B; image `92a00000..94800000`, entry in range.
- `PAS INIT_IMAGE` `ret 0 res 0`, `PAS MEM_SETUP` `ret 0 res 0`; then the segment copy
  wedged at **B04/0x92a20000** after B02 and B03, while the rest of iOS stayed alive
  (lockscreen). The handoff (`research/audio-refs/HANDOFF-AUDIO-F3F6.md`) warns that
  only byte stores are safe on this carve-out; `0508b` switches the copy to byte
  stores, adds the `lavaudprobe=1` carve-out read/progress and `lavaudcopy=0` to test
  a preloaded carve-out.

- **F3 (0509/0509b), `lavaud=7`:** GLINK `lpass` arriba — descriptor 478, RX 480,
  TX 479 allocado, `VERSION_ACK`, `OPEN_ACK apr_audio_svc` rcid 1 — y el ADSP
  respondió al `APR GET_FWK_VERSION` con `APR_BASIC_RSP_RESULT` (`0x110e8`,
  token 1, len 40). El `CMD_INTENT` del ADSP usa `p1=2` (su canal), no el rcid;
  el filtro viejo `p1==rcid` era el bug, corregido en 0509b. El status del basic
  response y la primera query AFE quedan como siguiente paso.

The full plan and the hardware evidence are in
[`docs/AUDIO-BRIEF.md`](AUDIO-BRIEF.md).

## What the phone actually has

The port's first assumption ("no codec driver, port the WCD9335") was wrong for this
board. The live DTBO (`artifacts/backup/dtbo.img`) disables the SLIMbus audio controller
(`slim_aud`), so the `tasha` (WCD9335) and `tavil` (WCD9341) nodes are off. The enabled
card is `qcom,sdm660-asoc-snd` with:

- **PM660L analog codec** at SPMI SID 3, base `0xf000` (`qcom,pmic-analog-codec`, with
  the speaker OCP/boost and MBHC blocks);
- **LPASS digital codec** at `0x152c0000` and its **SoundWire master** at `0x152c1000`
  (`msm-dig-codec`, `msm_sdw_codec`); the WSA881x amplifiers are not fitted
  (`qcom,wsa-disable`);
- **ADSP** (LPASS Hexagon) at `0x15700000`, TrustZone PAS id **1**, firmware `adsp`,
  carve-out `0x92a00000` + 30 MiB;
- the stock vendor image's `audio_platform_info_intcodec.xml` maps the speaker to AFE
  port **`INT4_MI2S_RX`**, headphones/handset to `INT0_MI2S_RX` and the built-in mic to
  `INT3_MI2S`; the speaker mixer path is `INT4_MI2S_RX -> RX4/RX5 -> COMP -> Spkr BOOST
  -> SWR DAC_Port` on the PM660L.

So the path is `ASM stream -> ADM COPP -> AFE INT4_MI2S_RX -> LPASS digital codec/SWR ->
PM660L codec -> speaker`. SLIMbus and WCD9335 are not part of it.

## iOS side

iOS 16 on this port has no `IOAudio2` device; Apple's audio kernel drivers are closed and
not in the open-source XNU build. As in the topaz port, the user-visible device will be a
userland AudioServer plug-in in `/System/Library/Audio/Plug-Ins/HAL` presenting
`Speaker` (and the `Hawking` companion), writing 48 kHz stereo S16_LE frames to a kernel
`/dev/lavender-audio` node. The kernel then feeds the AFE path above. The topaz plug-in
(`refs/ios-topaz/userland/audio/audio_plugin.c`) is the template.

## Ladder (patches 0500–0599)

| Patch | What | State |
| --- | --- | --- |
| `0500` | read-only probe: GCC LPASS GDSC/vote, Q6SS, LPASS SMMU, digital codec/MCLK/AVTIMER/pinctrl, SMEM TOC + ADSP partition items, APCS IPC | **device run** (`0505` kernel, `lavaud=3`); CI `44927793793` green |
| `0501` | LPASS/ADSP power-up (SMMU vote, GDSC un-collapse, re-read Q6SS); restores the collapse on failure | device run reached this path, but the old order returned at the SMMU CBCR before the GDSC; retested by `0507` |
| `0502` | read `adsp.mdt` + `adsp.bNN` from the modem FAT16, parse the ELF/hash table and check every segment against the `0x92a00000` carve-out (read-only dry run) | **device run** (`lavaud=3`): 20 blobs, 29,018,084 bytes, 0 bad; `0508` adds the 0x23a000 BSS phdr and fixes the hash parser |
| `0503` | TrustZone PAS load (`lavaud=4`): refuse over stale SMEM, `INIT_IMAGE`, `MEM_SETUP`, copy the segments into `0x92a00000`, `AUTH_AND_RESET`, then read the Q6SS; `lavaudforce=1` overrides the stale check | **device run**: INIT_IMAGE and MEM_SETUP `ret 0 res 0`; copy wedged at B04/0x92a20000 (0508b) |
| `0504` | read-only inventory after PAS (`lavaud=5`): wait up to 3 s for the ADSP's SMP2P item, dump the apps↔ADSP partition items, `$SMP` headers/entries and the GLINK fifo candidates | written; pending device boot |
| `0505` | fix the SDM660 SMP2P **v1 packed header** (`feature_version`/`rem_loc_proc_id`/`valid_total_ent`, not the SM6225 layout) and print the vendor ids: AP→ADSP 429, ADSP→AP 443, GLINK 478/479/480 | written; the pre-PAS partition dump from `lavaud=3` is empty (no entries yet) |
| `0506` | `lavaud=6`: creates the AP SMP2P item 429 in the apps↔ADSP partition (hwlock 3, v1 header, `master-kernel`/`sleepstate`), kicks the ADSP on APCS bit 10 and waits up to 5 s for item 443's `slave-kernel` READY. First SMEM write of the series | written; pending device boot |
| `0507` | power-order fix from the first device log: the LPASS ADSP GDSC must be un-collapsed **before** voting the SMMU branch; the old order left the branch at `80000001` (EN, OFF). GDSC first, then CBCR, with the old failure text preserved | **device-verified**: GDSC `80000000` and CBCR `00000001` |
| `0508` | PAS metadata/hash fixes from that log: find the hash phdr by `p_flags` at phdr+24 (`PT_NULL`, 7176 bytes); assemble header+phdrs+hash for `INIT_IMAGE` (the MDT is that concatenation even though `p_offset` lies); load and zero the `B22` BSS phdr (`filesz 0`, 0x23a000); require SCM x1 status; check the ELF entry; never read the Q6SS when the LPASS domain reported off | **device-verified** through MEM_SETUP; CI run `44942180169` green |
| `0508b` | carve-out copy diagnostics from the `lavaud=4` hang: byte-wise `audio_io_copy`/`audio_io_zero` (the handoff says only byte stores are safe), `lavaudprobe=1` reads nine carve-out offsets and prints each 256 KiB read/copy point, `lavaudcopy=0` skips the segment copy to test a preloaded carve-out | written; pending kernel CI and device boot |
| `0509` | GLINK `lpass` edge 478/479/480, channel open/version and APR `apr_audio_svc` | planned |
| `0510` | PM660L codec over SPMI: add an isolated lavender SPMI arbiter v3 path (`0x0800f000`, observer `0x09400000 + 0x8000*apid`) and read the PM660L (SID 3, peripheral `0xf0`) ID/revision registers at `0xf000` (`lavaud=8`, read-only). Supplies, reset, MCLK, HP/SPK/MBHC come next | **device-verified read-only**: `0xF004=0x23` (digital PERPH_TYPE), `0xF005=0x04`; all reads return 0 |
| `0511` | LPASS digital codec + SWR master + AFE `INT4_MI2S_RX` port | planned |
| `0512` | ASM stream + ADM COPP to `INT4_MI2S_RX`, 1 kHz test tone | planned |
| `0513` | `/dev/lavender-audio` + the HAL plug-in (`Speaker`/`Hawking`/`Codec`) | planned |
| `0514`+ | microphone (`INT3_MI2S_TX`/ADC), jack/MBHC, volume, suspend | planned |

## Running the probe

Build the shim with the audio boot args and boot it like any other test image:

```
lavaud=1 lavauddelay=25    # read-only probe
lavaud=2 lavauddelay=25    # probe + LPASS ADSP power-up
lavaud=3 lavauddelay=25    # + adsp.mdt/adsp.bNN dry run from the modem FAT16
lavaud=4 lavauddelay=25    # + TrustZone PAS load and ADSP start
lavaud=5 lavauddelay=25    # + post-PAS SMEM/SMP2P inventory (read only)
lavaud=6 lavauddelay=25    # + create SMP2P out 429, kick the ADSP, wait READY
```

`lavaud=0` (or absent) keeps the driver silent. The thread waits 25 s after the eMMC
probe so the persistent log and the USB live log are up, then announces each access 2 s
in advance. Expected lines in `SANTONILOG1` / the UDP live log:

```
SWEET AUDIO: A0/A1 probe (lavaud=N) in 25 s
SWEET AUDIO: gcc lpass_smmu_cbcr ... lpass_adsp_gdscr ...
SWEET AUDIO: LPASS ADSP domain is ON|OFF
SWEET AUDIO: adsp gdscr before/after ...       (lavaud>=2, only if off)
SWEET AUDIO: smmu cbcr before/after ...        (lavaud>=2, only if off; after the GDSC)
SWEET AUDIO: q6 reset ...                      (domain on)
SWEET AUDIO: lpass smmu sCR0 ...
SWEET AUDIO: codec ... swr ...
SWEET AUDIO: smem TOC ... apps<->adsp ... item ... (SMP2P/GLINK ids)
SWEET AUDIO: apcs ipc ...
SWEET AUDIO: ADSP.MDT ... hash phdr N at off+len (inline|ADSP.Bnn blob)
SWEET AUDIO: ADSP firmware dry run: N segments, ... bad, N relocatable  (lavaud>=3)
SWEET AUDIO: ADSP metadata N bytes ... ADSP image start..end, entry ...  (lavaud>=4)
SWEET AUDIO: PAS init_image/mem_setup/auth_and_reset: ret ... res ...
SWEET AUDIO: probe done (lavaud=N)
```

At `lavaud=1`, if the domain is off after ABL, the probe deliberately skips the Q6SS,
codec and SMMU reads: a read of a gated LPASS block on this SoC can hang the bus.
`lavaud=2` is the step that turns the domain on and then re-reads them; if the GDSC never
reports `PWR_ON`, the collapse bit is put back and the rest is skipped. `0507` powers the
GDSC before voting the SMMU branch; `0508` composes the metadata for `INIT_IMAGE`, refuses
to start PAS without a hash table or with an entry outside every segment, checks the SCM
x1 result as well as x0, and does not touch the Q6SS after auth when the LPASS domain
reported off.

## References

- Hardware facts and the full ladder: [`docs/AUDIO-BRIEF.md`](AUDIO-BRIEF.md)
- Reusable ADSP/GLINK/APR shape: `refs/ios-topaz/xnu/patches/0144`–`0146`,
  `refs/ios-lavender/drivers/audio/hakios/`
- TrustZone PIL sequence: `xnu/patches/0241` (GPU zap) and Linux
  `drivers/soc/qcom/subsys-pil-tz.c`
- Vendor codec reference: `MiCode/Xiaomi_Kernel_OpenSource`, branch `lavender-q-oss`,
  `sound/soc/codecs/` (`wcd9335` is not used; the PM660L driver and `msm-dig-codec`
  are the ones to read)
