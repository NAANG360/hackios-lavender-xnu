# XNU patch series

`xnu/patches/` holds the **complete** series the lavender kernel is built from. CI hands it to
[`blacktop/darwin-xnu-build`](https://github.com/blacktop/darwin-xnu-build) on top of Apple's open-source `xnu-8796` (see `docs/BUILD.md`).

- `0001`–`0192` come from [`aLpHa-Git-69/ios-lavender`](https://github.com/aLpHa-Git-69/ios-lavender), commit `bc606b7`, which is the pinned base `83356567…` plus the 30 s `0192`. That is the series that built the lock-screen kernel. `ios-lavender` is shared with other devices, so it is not modified; this repository is now where lavender changes are made.
- `0019-stage-libpthread.sh`, `0020-stage-bearssl.sh` and `0026-stage-hfs.sh` stage sources the build needs.
- The series also carries the sweet/topaz/santoni/whyred history (`0003`–`0141`, `0148`, `0176`–`0188`). Lavender builds on those changes, so they stay in place even where the names say another board.

## Lavender patches

| Patch | Purpose | Status |
| --- | --- | --- |
| `0145`–`0147` | `LAVENDER` machine config, the framebuffer and the SDCC v5 root | On device |
| `0189`, `0190` | SDHCI exact PIO + cache state; GIC RWP wait and reset cleanup | On device |
| `0191-lavender-touch-stub-common-headers.patch` | Shared `santoni_touch_probe()` entry point built as an unsupported-board stub | On device (stub) |
| `0192-sdhci-wait-card-ready-after-write.patch` | Wait for DAT0 + `TRAN`/`READY_FOR_DATA` (CMD13) after every PIO write; the eMMC write fix | On device: reached the lock screen |
| `0200-lavender-usb-gadget.patch` | SDM660 USB device (DWC3) with the CDC-ECM network and live log; starts the input and USB drivers on lavender | On device (`sweetusb=1`): controller powered and clocked, in device mode; PHY powered down, so it needs `0210` |
| `0201-lavender-pmic-keys.patch` | Power / Volume Up / Volume Down through the SDM660 SPMI arbiter v3 | **On device**: all three keys reach `SWEET HID` (log `sweetlog-usbkeys-0202`) |
| `0210-lavender-usb-phy-power-and-hold.patch` | Power up the QUSB2 v1 PHY that ABL leaves powered down (`sweetusb>=2`); `sweetusbhold=1` USB test boot that stops before the root mount | On device: PHY PLL locks in 10 us, hold mode works; ep0 `SETEPCONFIG` still timed out |
| `0211-lavender-dwc3-core-bringup.patch` | Linux/bergamota DWC3 bring-up: UTMI clock as PIPE clock, core+PHY soft reset, HS quirks (`sweetusbfix` mask, default 7) | On device: no more endpoint timeout, core runs, one bus-reset event; link stays suspended |
| `0212-lavender-qusb2-utmi-mux.patch` | Release the QUSB2 UTMI test mux (`UTMI_CTRL2 0x40`) ABL leaves set, so the controller drives the D+ pull-up (`sweetusbutmi=0` keeps it) | **On device: enumerates on Windows at high speed** (`VID_1209&PID_0001`, CDC-ECM, no Windows driver) |
| `0213-lavender-usb-ncm.patch` | CDC-NCM function (default on lavender, `sweetusbncm=0` = ECM) so Windows binds its in-box UsbNcm driver; NTB16 framing; SPEED_CHANGE; bus reset handling | **On device**: Windows 11 UsbNcm adapter up, ping 70/70, live log |
| `0215-lavender-fb-swap-telemetry.patch` | Log the first `sweetfblog` commits and every change of the enabled layer set; the old `SWEET FB: swap` detail stopped at 16 presents and the counter only every 1000 commits, which made an idle lock screen look frozen | Kernel only; the lock-screen boot with this series is the test |
| `0216-lavender-rtc-calendar-base.patch` | Calendar base from `sweetrtc=<unix seconds>` when the platform clock answers 0 (the PM660 RTC is not read yet); the shim passes its build time | Kernel only; look for `SWEET RTC: calendar base` and a 2026 date on the lock screen |
| `0217-lavender-sweetnopb.patch` | `sweetnopb=1` makes the exec of `PosterBoard` fail with EPERM, to isolate the poster crash from the rest of the lock screen | Diagnostic, off by default |

| `0220-lavender-pm660-rtc.patch` | Skip XNU's 30 s wait for an `IORTC` resource (as XNU does for BCM2837) and feed the calendar from the PM660 RTC over SPMI (read-only; `sweetrtcoff=` when the raw count is not Unix time) | **On device**: the 30 s wait is gone (eMMC/USB at ~16 s instead of ~46 s); RTC read (raw `230219452`, not Unix time: offset measured in TWRP) |

| `0230-lavender-touch-spin-timing.patch` | Touch bit-bang I2C with real timing: `delay()` never spins on this port (CPUs registered with `powergate_latency` 0), so every half bit slept until a timer interrupt and a frame took seconds. Spin on the timebase (`sweetouchus=`), adaptive frame length/rate, timing self-test and 10 s stats | **On device: touch works** (home screen, icons), 0 I2C errors, ~14 ms idle read, up to ~65 ms full frame |
| `0231-lavender-touch-partial-frames.patch` | Read only up to the highest active slot (12 bytes for one finger instead of 65), idle poll 25 ms (`sweetouchidle=`), line-rise stats | Host syntax-checked; not yet run on the device |
| `0232-lavender-sdhci-dma-diag.patch` | Read-only SDHCI report at attach (CAPABILITIES, HOST_VERSION, clock/DLL/ADMA registers, bounce buffer address). Finding: this SDCC v5 core has **no SDMA** (CAPS bit 22), only ADMA2 (bit 19) with 64-bit addressing, which is why every SDMA attempt ended in `ADMAERR`. Optional timed tests behind `sweetemmc_speed=1` / `sweetemmc_dmatest=1` (off: they hold the controller lock and stall boot) | **On device** |
| `0233-lavender-sdhci-adma2-read.patch` | ADMA2 **read** path into the bounce buffer (32 KiB descriptors, cache maintenance, PIO fallback, off after 3 failures). Writes stay PIO (0192). On by default after a PIO-vs-ADMA2 self-test at two places of the root; `sweetemmc_dma=0` keeps PIO | **On device**: cold read 465 KB/s (PIO) -> ~16 MB/s; boot to the lock screen ~23 min -> ~2 min; dyld cache files hash-identical to the host under 4 concurrent readers |
| `0234-lavender-touch-real-panel-height.patch` | SweetHIDTouch takes the real panel size (1080x2340 from the boot video) instead of a hard-coded 2400: fixes the growing vertical offset and makes the bottom-edge band reachable (home/unlock swipe) | Pending device test |
| `0235-lavender-vsync-at-panel-rate.patch` | The vsync notification to CoreAnimation follows `sweetfb_hz` (every `fFlushTicks` ticks) instead of a fixed 60 Hz, so the CPU compositor (`CA::OGL::SW`, ~150 % of backboardd) no longer draws frames the panel never shows | Pending device test |
| `0236-lavender-gpu-probe.patch` | Adreno 512 bring-up ladder (`bsd/dev/lavender_gpu.c`), off unless `lavgpu=N`: 1 reads GCC/GPUCC state, 2 votes the GPU BIMC branches, 3 CX GDSC, 4 GX GDSC + core clocks + reads the RBBM/CP block. Each step is announced 2 s before the access. Project: `lavender-metal` | Pending device test |
| `0237-lavender-gpu-gx-root-clock.patch` | lavgpu step 4: force the GFX3D RCG root on (XO) before the GX GDSC, 3 s poll, collapse GX again on failure. Run `lavgpu=4` of 0236 left the GX GDSC at `08222000` (never PWR_ON): the DT marks it `force-enable-root-clk` | Pending device test |
| `0238-lavender-gpu-ldo-bhs.patch` | lavgpu: read-only dump of the GFX LDO/head-switch block (`0x0506e000`) in step 1; step 4 puts it in BHS mode under GDSC control before the GX GDSC (restored on failure). With 0237 alone the GX still stayed at `08222000` | Pending device test |
| `0239-lavender-gpu-gx-reset-clamp.patch` | lavgpu step 4: before clearing the GX GDSC's SW_COLLAPSE, pulse the GX block reset (GPUCC+0x1090) and the AON/GMEM reset and release the GMEM clamp (GPUCC+0x130), the order the vendor GDSC driver uses. 0237 and 0238 left the GX at `08222000`; the LDO block was already in BHS state (`pwrswitch_ctrl 80000038`) | Pending device test |
| `0240-lavender-gpu-scm-probe.patch` | lavgpu=5: SCM questions, no state change: INFO is_call_available(PIL IS_SUPPORTED) and PIL IS_SUPPORTED for PAS id 13 (GPU zap), 4 (modem, reference) and 99 (contrast). Same SMC64 convention as 0113. Runs after the GX power-up of 0239 (`lavgpu=4`: GX on at 11 ms, GPU block read, no hang) | Pending device test |
| `0241-lavender-gpu-zap-load.patch` | lavgpu=6: loads the GPU zap shader through TrustZone PIL (PAS 13): reads `a512_zap.mdt`/`.b02` from `/usr/share/firmware/lavender/gpu/`, INIT_IMAGE, MEM_SETUP, copy, AUTH_AND_RESET, each announced and stopped at the first non-zero ret; re-reads SECVID_TRUST_CNTL afterwards. Also logs `a530_pm4.fw` size | Device: INIT_IMAGE, MEM_SETUP and AUTH_AND_RESET all ret 0; SECVID_TRUST_CNTL stays 1 until the CP runs (expected) |
| `0243-lavender-gpu-smmu-probe.patch` | lavgpu=7: read-only look at the GPU kgsl SMMU (0x05040000, SMMUv2 layout): global sCR0/IDR*, stream mapping groups 0-3, context banks 0-3 (SCTLR, TTBR0, TCR, FSR, FAR). Nothing written. Runs after the zap load | Device: SMMU active (sCR0 00200626), S2CR0..3 = fault (reset default), CB0 disabled; reading CB1 hangs the reading thread, so 0244 reads CB0 and CB4 only |
| `0249-lavender-gpu-smmu-bypass.patch` | lavgpu=8: reads CB0 and CB4 only, then programs the GPU stream: SMR0 valid for ID 0 and S2CR0 type BYPASS (CP fetches by physical address), read back to confirm it stuck | Device: SMR0 took (80000000), but S2CR0 bypass came back as fault (0x20000): this SMMU refuses bypass, so 0245 translates |
| `0250-lavender-gpu-smmu-stage1.patch` | lavgpu=9: context bank 4 (gfx3d_user) gets a stage 1 AArch64 identity table (four 1 GB blocks, VA = PA, normal WB), TCR/MAIR/TTBR0/CBAR/CBA2R programmed, TLB invalidated, SCTLR enabled; stream 0 mapped to it (SMR0 valid ID 0, S2CR0 = CB4). Reads back each register | Device: all CB4 registers read back as written, SCTLR 67, stream 0 -> CB4, FSR 0 |
| `0251-lavender-gpu-ucode-ring.patch` | lavgpu=10: a530_pm4.fw / a530_pfp.fw (minus first dword) into contiguous buffers, CP_PFP/ME_INSTR_BASE, 32 KB ring + rptr shadow, CP_RB_CNTL/BASE/RPTR_ADDR written and read back. CP stays halted | Device: microcode and ring registers read back as written (CP_PFP_ME_CNTL 303 = halted before) |
| `0252-lavender-gpu-cp-start.patch` | lavgpu=11: minimal CP/RBBM setup (64-bit address modes, SECVID TSB off, ROQ/MEQ thresholds), clears the PFP/ME halt, then ring submissions each announced and stopped at the first the CP does not consume: CP_ME_INIT, CP_SET_SECURE_MODE(0) (SECVID_TRUST_CNTL should go 0), CP_MEM_WRITE of a fence word read back by the CPU | Device: ME_INIT consumed, SET_SECURE_MODE(0) took SECVID_TRUST_CNTL 1->0, CP_MEM_WRITE fence cafe0001 written by the GPU |
| `0253-lavender-gpu-fence-reset.patch` | lavgpu=12: 16 fences with different values, a deliberate CP hang (short CP_NOP swallows the next packet), RBBM software reset, CP bring-up again (zap stays loaded), and a fence that must land after the reset | Device: 16/16 fences, deliberate hang, RBBM_SW_RESET + re-bring-up, fence lands again (M2 done) |
| `0300-lavender-gpu-compute.patch` | lavgpu=13: first compute dispatch. Hand-assembled ir3 shader (mov imm, stg, end; A5xx encodings checked against Mesa disassembler vectors) stores 0xc0de0001 to memory; 64-bit address-mode and UCHE registers, cs_program/launch_grid register order from Gallium fd5, CP_LOAD_STATE4 shader preload, CP_EXEC_CS; GPU reset and bring-up again if the word does not land | Device: CP consumed all 86 dwords, no hang, reset recovery works, but the store did not land (see 0301) |
| `0301-lavender-gpu-compute-flush.patch` | lavgpu=13 again: UCHE flush and CP_EVENT_WRITE CACHE_FLUSH_TS after the dispatch, in case the SP store sits in the write-back UCHE; logs the event word | Device: SHADER WROTE IT, 0xc0de0001 stored by the SP right after the dispatch (M3 first compute on the real GPU) |
| `0302-lavender-gpu-compute-buffers.patch` | lavgpu=14/15: shader with an input and an output buffer (ldg, add.s, stg; encodings from Mesa ir3 disasm vectors) | Superseded by 0303-0308 |
| `0303-lavender-gpu-compute-copy.patch` | split the buffer test: 14 = copy (ldg+stg), 15 = ldg+add+stg | See 0308 |
| `0304-lavender-gpu-compute-sentinel.patch` | 0xdeadbeef in the output before the dispatch, input readback | See 0308 |
| `0305-lavender-gpu-compute-regfootprint.patch` | SP_CS_CTRL_REG0 register footprint 4 for shaders that use r0..r3 | See 0308 |
| `0306-lavender-gpu-compute-gpu-written-input.patch` | copy test that loads a word the GPU stored itself (lavgpu=16) | Device: copy works |
| `0307-lavender-gpu-smmu-all-streams.patch` | tries SMR0 mask 0x7fff (the mask bits are not writable, SMR0 stays 80000000); SMMU fault status printed around the dispatch | Device: no SMMU faults seen |
| `0308-lavender-gpu-compute-repeat-add.patch` | lavgpu=15: copy x3 then out = in + 0x11111111 x3 | Device: 7 of 7 dispatches correct (c0de0001, 00abcdef x3, 11bcdf00 x3) |
| `0309-lavender-gpu-compute-64-threads.patch` | lavgpu=17: one workgroup of 64 threads, local id delivered in r4.x (HLSQ_CS_CNTL_0 LOCALIDREGID), ldg.a/stg.a with a thread-indexed address; probe dispatch then out[tid] = in[tid] + 0x11111111 twice, each output word compared with the host-computed value | Device: 3 of 3 dispatches, 64 of 64 outputs correct each (M3 done) |
| `0310-lavender-gpu-blit2d.patch` | lavgpu=18 (M4.0): a 2D blit through the A5xx 2D core (RB_2D_*, CP_BLIT), a 64 byte R8 copy, register order from Mesa fd5's blitter | Device: consumed, nothing copied (colour cache not flushed) |
| `0311-lavender-gpu-blit2d-ccu-flush.patch` | adds CP_EVENT_WRITE PC_CCU_FLUSH_COLOR_TS after the blit | Device: 64 of 64 bytes copied |
| `0312-lavender-gpu-blit2d-rgba.patch` | lavgpu=19 (M4.1): 2D copy of a 16x8 rectangle between two RGBA8 linear 32x16 images (box at (4,2) to (8,4)), every pixel checked and the rest of the destination left untouched | Device: 128 of 128 box pixels right, 384 of 384 outside untouched |
| `0313-lavender-gpu-blit2d-fill.patch` | lavgpu=20 (M4.2): three register variants of a 2D solid fill (CP_BLIT op FILL with RB_2D_SRC_SOLID_DW0..3; control word 0x86000000 or 0; integer or float components) | Device: consumed, nothing written by any variant (parked; clears will be a 3D draw) |
| `0314-lavender-gpu-first-3d-draw.patch` | lavgpu=21 (M4.3): first 3D draw. Sysmem TRILIST triangle with hand-assembled VS/FS into a 64x64 RGBA8 linear target (Mesa a5xx sysmem order: CCU invalidate, RB_CCU_CNTL 0x10000000 after the WFI, BYPASS, CCU colour+depth flush at the end). Key fixes found on device: CP_SET_RENDER_MODE needs 5 dwords, RB_MRT_PITCH/ARRAY_PITCH are shr 6 | Device: 351 of 351 inside pixels have the colour, 3316 of 3316 outside untouched |
| `0315-lavender-gpu-texture-sample.patch` | lavgpu=22 (M4.4): FS samples a 4x4 RGBA8 linear texture (hand-assembled cat5 `sam`, constant coords, nearest, clamp to edge); sampler and TEX_CONST loaded with CP_LOAD_STATE4 SB4_FS_TEX; linear pitch 256 with PITCHALIGN 2 | Device: every triangle pixel = texel (2,1) ff905077, outside untouched |
| `0316-lavender-gpu-varying-texcoords.patch` | lavgpu=23 (M4.5): VS passes a uv attribute (second VFD fetch) through the VPC; FS gets it with `bary.f` using IJ_PERSP_PIXEL (GRAS_CNTL + RB_RENDER_CONTROL0 + HLSQ_CONTROL_3) and feeds `sam`; lavgpu=24 is the (u,v)-as-colour debug step. The uv must not live in registers the VS marker tail clobbers (r1) | Device: every triangle pixel is the right texel ((x-16)>>3, (y-16)>>3), 213/213 inside, outside untouched |
| `0317-lavender-gpu-blend.patch` | lavgpu=25 (M4.6): colour blending in the RB: RB_MRT_CONTROL BLEND|BLEND2, RB_MRT_BLEND_CONTROL factors/opcodes, RB_BLEND_CNTL and SP_BLEND_CNTL enable bit 0. Factor enum: ZERO 0, ONE 1, SRC_ALPHA 6, ONE_MINUS_SRC_ALPHA 7, DST_COLOR 8, ONE_MINUS_DST_COLOR 9 | Device: SRC_ALPHA/1-SRC_ALPHA, ONE/ZERO and ONE/ONE all 351/351 over a known background, outside untouched |
| `0318-lavender-gpu-on-screen.patch` | lavgpu=26 (M4.7): the GPU's textured+varying triangle and the blended triangle are copied 4x into the real scanout framebuffer (PE_current_console base, ml_io_map_unmappable) for 30 s; the RB cannot render into the scanout directly because its 4320-byte stride is not a multiple of 64 | Device: both pictures visible on the panel (photo), channel order a8r8g8b8 correct |
| `0319-lavender-gpu-ioservice.patch` | lavgpu=30/31 (M5.0): the GPU as an IOKit service `LavenderGPU` (new iokit/Kernel/arm/LavenderGPU.cpp + files.arm64 line) with a user client: info, buffer new/free (physically contiguous below 4 GB, mapped to the client), submit of PM4 words (synchronous, fence by CP_MEM_WRITE) and submit by reference from a client buffer; lavgpu=31 adds an in-kernel selftest. No address validation (bring-up) | Device: selftest PASSED and the userland client (info, buffer, map, 8-byte and 6 KB streams, free) PASSED. Note: IOKit's large struct-input path hung the phone, so long streams go by reference |
| `0320-lavender-gpu-metal-props.patch` | lavgpu=30/31 (M5.1): selector 5 of the `LavenderGPU` service puts (1) or removes (0) `MetalPluginName`=LavenderMetal and `MetalPluginClassName`=LavenderMetalDevice on the existing empty `IOAcceleratorES` service (patch 0138), so Metal finds the plug-in bundle only while a client asks | Device: `MTLCreateSystemDefaultDevice()` in a test program returns `LavenderMetalDevice` (bundle in /System/Library/Extensions, userland in lavender-metal); the device is a stub that logs what Metal asks it |
| `0500-lavender-audio-probe.patch` | SDM660 audio platform probe (`bsd/dev/lavender_audio.c`), off unless `lavaud=N`: read-only GCC LPASS GDSC/vote, Q6SS, LPASS SMMU, digital codec/MCLK/AVTIMER/pinctrl, SMEM TOC + apps<->ADSP partition item walk (SMP2P/GLINK ids), APCS IPC. The phone has no WCD9335: the DTBO disables `slim_aud`; see `docs/AUDIO.md` | **Device run** (0505 kernel, `lavaud=3`); CI `44927793793` green |
| `0501-lavender-audio-power.patch` | `lavaud=2`: votes the LPASS SMMU clock (`0x7d014`), clears the LPASS ADSP GDSC's `SW_COLLAPSE` (`0x7d034`), polls `PWR_ON`, then re-reads the Q6SS; restores the collapse and stops if the domain never comes on. No firmware, no DSP start | **Kernel CI build green** (run `44928664860`); pending device test |
| `0502-lavender-audio-adsp-firmware.patch` | `lavaud=3`: reads `ADSP.MDT` from the modem FAT16 (the same partition/layout the modem driver uses), parses the ELF program headers and the MDT hash table, and checks every `adsp.bNN` blob's size and load range against the ADSP carve-out (`0x92a00000`, 30 MiB). Read-only dry run: nothing is written and TrustZone is not called | **Device run** at `lavaud=3`: 20 segments, 29,018,084 bytes, 0 bad; the hash parser is corrected by 0508 |
| `0503-lavender-audio-pas-load.patch` | `lavaud=4`: refuses to start the ADSP over an earlier run's SMEM state (`lavaudforce=1` overrides), copies the metadata into a contiguous buffer, runs PAS `INIT_IMAGE`, `MEM_SETUP`, copies every segment into the carve-out, flushes the cache and runs `AUTH_AND_RESET` (PAS id 1). Stops at the first non-zero SCM return and never touches the region after auth | **Kernel CI build green** (run `44933827597`); pending device test |
| `0504-lavender-audio-adsp-inventory.patch` | `lavaud=5`: after the PAS start, waits up to 3 s for the ADSP's SMP2P item and dumps the apps↔ADSP partition (item ids/sizes, `$SMP` local/remote pids and entry values) plus the 16 KiB GLINK fifo candidates and the Q6SS state. Read-only; those firmware-specific ids are the input for the real SMP2P/GLINK step | Written; host tests green; pending device test |
| `0505-lavender-audio-smp2p-v1-layout.patch` | `lavaud=5`: the SDM660 SMP2P header is the vendor v1 **packed** form (`feature_version`, `rem_loc_proc_id`, `valid_total_ent`), not the SM6225 layout the first probe used. Decodes it correctly and prints the fixed ids from the vendor `smem.h`: AP→ADSP 429, ADSP→AP 443, GLINK 478/479/480 | Written; host tests green; pending device test |
| `0506-lavender-audio-smp2p-out.patch` | `lavaud=6`: creates the AP SMP2P item 429 in the apps↔ADSP partition (SMEM hwlock 3 at TCSR `0x1f40000`, v1 header, `master-kernel`/`sleepstate`), kicks the ADSP on APCS bit 10 (`0x17911008`, mask `0x400`) and waits up to 5 s for item 443's `slave-kernel` READY bit. The first SMEM write of the series | Written; host tests green; pending device test |
| `0507-lavender-audio-power-order.patch` | Power-order fix from the first device log: at `lavaud=3` the LPASS ADSP GDSC read `0x00000001` (collapsed) and voting the SMMU branch first left it at `80000001` (EN, OFF). Un-collapse the GDSC first (200 ms poll), then vote the branch clock; the old GDSC-first failure message is preserved | **Kernel CI build green** (run `44938672875`); pending device test (`lavaud=3`) |
| `0508-lavender-audio-pas-metadata.patch` | PAS metadata/hash fixes from the same log and the PC copy of `NON-HLOS.bin`: find the MDT hash phdr by `p_flags` at phdr+24 (index 1, `PT_NULL`, 0x02200000, 7176 bytes; the old code read `p_align` at +28); assemble ELF header + phdrs + hash for `INIT_IMAGE` (the vendor MDT is exactly 788+7176 despite the hash phdr's `p_offset`); load and zero the `B22` BSS phdr (`filesz 0`, `memsz 0x23a000`) the old walk skipped; require the SCM x1 result as well as x0; check the ELF entry; never read the Q6SS after auth when the LPASS domain reported off | **Device-verified** through `INIT_IMAGE`/`MEM_SETUP` (`lavaud=4`, `ret 0 res 0`); the copy wedged at B04/0x92a20000 |
| `0508b-lavender-audio-carveout-copy.patch` | Copy diagnostics from the `lavaud=4` hang: `audio_io_copy`/`audio_io_zero` use byte stores (the audio handoff says only byte stores are safe on this carve-out); `lavaudprobe=1` reads nine carve-out offsets and prints the read/copy point every 256 KiB; `lavaudcopy=0` skips the segment copy to test whether the bootloader/TZ left the firmware preloaded | **Device-verified**: byte copy passed B04 and loaded B02..B22; the carve-out probe read the ABL-preloaded headers |
| `0508c-lavender-audio-no-carve-flush.patch` | Drop `flush_dcache64(carve-out, 31 MB, TRUE)` after the copy: the carve-out is `no-map` and Device-nGnRnE, `phystokv_range()` can return a zero-length run and spin. The metadata buffer flush stays | **Device-verified** (AUTH reached) |
| `0508d-lavender-audio-smp2p-cached.patch` | Cache the ADSP SMP2P 443 pointer/length before the 429 kick and read it directly after the kick instead of rescanning the partition; bound the scan to 64 items; print the cached header and raw wait state; 3 s for the ADSP to fill the entries | **Device-verified**: `vt 00020010` (2 valid) after the kick; the wait itself still stalls |
| `0508e-lavender-audio-smp2p-dump.patch` | Dump the cached 443 entries with `audio_smp2p_item` right after the header and mark the wait loop/calls, to separate a stalled entry read from a stalled wait | Written; CI `44983350862` green |
| `0509-lavender-audio-glink-apr.patch` | F3 `lavaud=7`: port the SMEM-native GLINK transport from the WLAN 0715/0716 work — descriptor 478, RX 480, allocate TX 479, ring indices, VERSION on APCS bit 9, open `apr_audio_svc`, intents, APR core `GET_FWK_VERSION` | **Device-verified**: VERSION_ACK, OPEN_ACK rcid 1, 8 remote intents, APR response `0x110e8` |
| `0509b-lavender-audio-glink-intents.patch` | The ADSP's `CMD_INTENT` uses `p1=2` (its local channel), not our rcid, so the old `p1==rcid` filter dropped every intent. Accept the pairs, log the raw INTENT, and send `RX_INTENT_REQ` for one 20-byte intent if none arrive | **Device-verified**: 8 remote intents and APR query sent |

Audio work takes `0500`–`0599`, the GPU series continues at `0300`+, and `0700`–`0707` is the
WLAN/modem block.

## Changing the series

```bash
# after adding or editing a patch
python3 tools/extract_drivers.py      # refresh drivers/ mirrors (generated, never edited by hand)
bash tests/run_tests.sh               # patch structure, SDHCI/GPT/log/GIC host tests, shipped-code checks
bash tests/host/prepare-tree.sh /tmp/lavender-tree
bash tests/host/run.sh /tmp/lavender-tree   # USB/keys drivers on simulated registers
```

A push to `main` that touches `xnu/patches/**`, `resources/**` or the workflow builds the kernel in CI. `.gitattributes` keeps patches LF; a CRLF checkout makes `git apply` reject them.

## 0192: the 30 s vs 1 s timeout

The working patch uses `CACHE_FLUSH_TIMEOUT_NS` (**30 s**). A 1 s variant using `CMD6_BUSY_TIMEOUT_NS` was built as `0192b` (CI run `44636855966`, kernel `2a6b5421…`) and **rejected**: it stopped at `cc_fips_test` because some writes need more than one second. Do not shorten the timeout without a card-specific measurement.

## 0200: USB device

The base tree already had `sweet_usb.c` (patches `0072`–`0077`). It compiled for `LAVENDER`, but with the SM7150 addresses. It also never ran: its only caller is `sweet_ufs_init()`, and that function does not run on an eMMC phone.

- Addresses come from the phone's DTB (`tools/fdtdump.py` on `lavender_dtb.bin`): DWC3 core `0x0a800000`, Qualcomm wrapper (QSCRATCH) `0x0a8f8800`, QUSB2 **v1** PHY `0x0c012000`, GCC `0x00100000`. The GDSC and branch clock offsets come from Linux `gcc-sdm660.c` (`usb_30_gdsc` `0xf004`, master `0xf008`, sleep `0xf00c`, mock_utmi `0xf010`, cfg_noc_usb3_axi `0x5018`, aggre2_usb3_axi `0xf03c`, ahb2phy `0x6a004`, rx0_usb2_clkref `0x88018`).
- Boot-arg `sweetusb`:
  - `1` (default): read-only. DWC3 and the PHY are read only if the GCC says the block is powered and clocked.
  - `2`: starts the CDC-ECM gadget. The phone is `ue0` 192.168.7.2/24 and sends the console as UDP to 192.168.7.1:5140.
  - `3` (experimental): first turns on the USB30 GDSC and the seven branch clocks from the GCC, then does what `2` does. Lavender boots the shim from `recovery`, not with `fastboot boot`, so ABL may leave USB off. No RCG, reset, PMIC regulator or PHY tuning writes.
- If ABL left the core in host or OTG mode, it is put in device mode before the device soft reset.
- The live log reads the `santoni_sdhci.c` console ring through new readers, `santoni_log_copy()` and `santoni_log_position()`. The `sweet_ufs.c` ring stays empty on lavender.
- `sweet_keys_init()` and `sweet_usb_probe()` are called from `IOFindBSDRoot`, after the eMMC probe. Both are idempotent.

## 0201: buttons

`sweet_keys.c` was the topaz driver (SM6225, arbiter v5). From the DTB, SDM660 has a v2/v3 arbiter:

- core `0x0800f000`, observer `0x09400000`, cnfg `0x0800a000`, ee 0;
- the APID table is at core + `0x800`, there is no APID count register (512 APIDs), and each observer channel is at obsrvr + `0x8000 * apid`.

| Key | Source | Bit |
| --- | --- | --- |
| Power | PM660 (SID 0) PON `0x800`, KPDPWR | `PON_INT_RT_STS` bit 0 |
| Volume Down | PM660 PON, RESIN | `PON_INT_RT_STS` bit 1 |
| Volume Up | PM660L (SID 2) GPIO7 = `gpio@c600`, active low | `INT_RT_STS` (`0xc610`) bit 0 |

The driver is still read-only: the only MMIO write is the EXT_READL command. If the GPIO is not an enabled input, Volume Up is withheld and the PMIC is left unchanged. `sweetkeys=0` disables the driver. The `camera_*` entries in the DTB are reference-board leftovers (lavender has no camera key) and are not polled.

## Tests

- `tests/run_tests.sh` covers SDHCI (`0192`), GPT, the log ring, GIC, the payload tools, the workflow command lines and the patch structure.
- `tests/host/` compiles the real patched `sweet_usb.c`/`sweet_keys.c` against simulated registers.

See `tests/README.md`.

## 0191 note

The touch entry point is still a stub. It compiles as an unsupported-board stub so the kernel builds for lavender while the Novatek driver is missing. It is not a working touch driver.
