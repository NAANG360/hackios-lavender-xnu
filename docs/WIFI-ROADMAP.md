# Roadmap — Driver WiFi (WCN3990 + modem MSS) — lavender

> Estado al **2026-11-22**. Progreso detallado en `work/briefs/wifi-wcn3990-brief.md`
> (agente WLAN, serie de parches `0700`–`0732`, branch `wlan-0706`). Este archivo es el
> tablero de avance: ~~tachado = verificado en el teléfono~~, `⬜ PENDIENTE` = falta.
> Convención: un archivo por función clave (`WIFI-ROADMAP.md`, y luego GPU/Audio/etc.).

## Objetivo final

Wi-Fi real en iOS 16.7.16 sobre el Redmi Note 7: scan, join WPA2, data path (DHCP,
`en0`) y visible en `wifid`/CoreWiFi/**Settings**. Hardware: WCN3990 detrás del modem
MSS (Q6) de SDM660 — el WiFi depende de arrancar el modem completo y hablarle por
GLINK/QRTR.

## Panorama

| Fase | Alcance | Estado |
|---|---|---|
| F0 | Inventario read-only (DTB, GPT, firmware, referencias) | ✅ COMPLETA |
| F1 | Arranque del modem (MBA/PIL/SCM/auth) | ✅ COMPLETA |
| F2 | GLINK/QRTR + host services + WLFW | 🔄 **EN CURSO** (host services completos; falta WLFW) |
| F3 | HTC/WMI/HTT + scan | ⬜ PENDIENTE |
| F4 | Assoc/WPA2 + data path + iOS (`en0`, Apple80211) | ⬜ PENDIENTE |

---

## F0 — Inventario read-only — ✅ COMPLETA

- ~~Nodo WLAN del DTB: `icnss@18800000` (WCN3990), sin SMMU de apps asignado hoy~~
- ~~SMMU ANOC2: 21 context banks, **0 SMR válidos** (`sCR0` SMMUEN=0) → sin traducción;
  GLINK/APCS no la necesitan; se re-evalúa en la etapa CE/DMA~~
- ~~Modem MSS `@4080000`: relojes, RMB, carve-outs (`modem_fw_region@8ac00000` +126 MiB,
  `pil_mba_region@94800000` 2 MiB)~~
- ~~GPT y particiones: firmware leído de la FAT16 de `modem` (LBA 786432) por
  `santoni_emmc_read`; `MBA.MBN`, `MODEM.MDT`+30 phdrs, `WLANMDSP.MBN` 3.86 MB~~
- ~~Bluetooth por SLIMbus (`slim@15240000`) — proyecto aparte, no UART HCI~~
- ~~Referencias downstream descargadas: `smem.c`/`glink_smem_native_xprt.c`
  (wahoo-4.4 y sdm660), `pil-msa.c`, `pil-q6v5.c`, `secure_buffer.c`, `gcc-sdm660.c`
  (`work/wlan/refs-gcc`)~~

## F1 — Arranque del modem (MBA/PIL/auth) — ✅ COMPLETA (parches 0700–0711)

- ~~`0700` probe de plataforma (`sweetwlan=1`)~~
- ~~`0701` relojes MSS (6 writes; bisección con `sweetmodemclk=N`)~~
- ~~`0702`–`0705` lectura de la FAT16 del `modem`: lock SDHCI (`santoni_sdhci_lock`),
  cadena FAT correcta (índice sin `fat_fat_off`), MBA.MBN cargado~~
- ~~`0706` carga de `MODEM.MDT` + segmentos (PIL order correcto: deassert restart antes
  de clocks)~~
- ~~`0707`–`0711` chunks de 2 MiB por SCM (`MEM_PROT_ASSIGN`, VMID MSS_MSA), ventanas
  IO map/unmap pareadas (`ml_io_map_unmappable`), IO alineado byte-a-byte hasta el
  límite de 8 bytes, blobs `02`–`29` (`status 00000003`, B12 21.5 MB y B17 13.7 MB por
  ventanas reutilizadas), 59 `reclaim chunk` → `MSS_MSA`~~
- ~~**AUTH COMPLETE**: `auth status 4 after 0 ms`, `auth complete, modem is running`,
  sistema vivo (~126 s). Log: `artifacts/logs/live-0711-auth-complete.txt`;
  CI `44929935655` (commit `b81de87`)~~

## F2 — GLINK/QRTR + host services + WLFW — 🔄 EN CURSO (parches 0712–0732)

### Logrado (verificado en el teléfono)

- ~~`0712`/`0713` probe SMEM v12: `$TOC` en `0x861ff000`, partición APPS/MODEM
  (`0000/0001`, off `11b000`, size `0x94000`), items 435 (SMP2P in) y 478 (desc)~~
- ~~Lección: el AP **no puede leer** particiones SMEM sin host APPS (cuelga el hilo)~~
- ~~`0714` SMP2P out: item 428 creado bajo hwlock **3** (`0x1f43000`, sfpb),
  doorbell APCS bit **14** → modem responde `slave-kernel 0x6` (READY|HANDOVER)~~
- ~~`0715`/`0716` GLINK UP: descriptor 478, TX 479 (uncached), RX 480 (**cached** —
  layout SDM660: header *después* de los datos, empaquetado desde el final), doorbell
  bit **15**, VERSION/VERSION_ACK, OPEN/OPEN_ACK, intents, QRTR HELLO, servicios
  `42:b401 servreg-notif` y `2b:1202 ssctl`~~
- ~~`0717`–`0720` **pd-mapper** en `0x4001` (servicio `0x40:0x101`):
  `servreg_loc_get_domain_list`; el formato correcto (verificado contra bergamota
  1115) es TLV 0x10 `u16 total_domains`, TLV 0x11 `u16 db_revision` y TLV 0x12
  `[u8 count][entries]`, **sin** bytes de validez (0727 revirtió el 0720)~~
- ~~`0728` **tabla que arranca wlan_pd**: `tms/pddump_disabled` y `tms/pdr_enabled`
  → **0 dominios**, `avs/audio` → `msm/adsp/audio_pd/74`, `kernel/elf_loader` y
  `wlan/fw` → `msm/modem/wlan_pd/180`; con la tabla vieja (1 dominio en pddump/pdr)
  el PDR del módem se quedaba parado~~
- ~~`0721` **tftp** (servicio `4096:1`, puerto `0x4002`): RRQ/WRQ, OACK,
  `blksize`/`tsize`, DATA/ACK con reintento; sirve el `IMAGE/` del FAT del `modem`
  (`WLANMDSP.MBN`, 3.859.712 B) sin cargarlo entero en RAM (`lavender_modem_fw_read`)~~
- ~~`0722`–`0726` **rmtfs** (servicio `14:1`, puerto `0x4000`): OPEN por path
  (`/boot/modem_fs1/2`, `_fsc`, `_fsg` → `modemst1/2`, `fsc`, `fsg`), CLOSE,
  RW_IOVEC, ALLOC_BUFF y GET_DEV_ERROR; GPT por `emmc_read`, partición EFS copiada a
  RAM (solo lectura del eMMC) y buffer de staging de 3 MiB del pool del kernel
  asignado a **HLOS+MSS** (`scm_mem_assign`, dst `{HLOS, MSS_MSA}`)~~
- ~~**Verificado en el teléfono (0725/0726):** `rmtfs buffer a3dc4000+300000 ready`,
  `announced rmtfs 14:1 port 4000`; el módem abre `modemst1` y `modemst2` (8 MiB
  copiados a RAM cada uno) y pide `ALLOC_BUFF 2 MiB`. El 0726 corrige el iovec: el
  `phys` del módem es la dirección **absoluta** dentro del staging, no un offset~~
- ~~`0729`/`0730` **flush de caché**: el staging es `kmem_alloc_contig` cacheable;
  sin `flush_dcache64` el módem leía ceros. La función recibe una **PA**, no un VA
  (el 0729 panickeó con `physokv_range: Illegal PA`). Con el flush en la PA, el
  módem lee 512 B de `modemst1`, 512 B de `modemst2` y **4094 sectores (~2 MiB) de
  `modemst2`**, y el EFS queda válido~~
- ~~**El PDR arranca (0730):** tras el EFS el módem pregunta `tms/pdr_enabled` → 0,
  anuncia ~13 servicios (incluido `ipa`) y abre `apr_apps2`, `apr_voice_svc` y
  `LOOPBACK_CTL_MPSS`~~
- ~~`0731`/`0732` **TFTP server check**: el módem sube
  `/readwrite/server_check.txt` por WRQ; se acepta en RAM (nunca al almacenamiento)
  y el OACK debe ecoar **todas** las opciones pedidas (`blksize`, `wsize`,
  `timeoutms`, `tsize`), o el cliente no envía los DATA~~

### ⬜ PENDIENTE

- [ ] Verificar el **0732** en el teléfono: server check completo (`got 5 bytes`),
      luego `avs/audio`/`kernel/elf_loader` y el `tftp RRQ .../wlanmdsp.mbn`
      (3.859.712 B).
- [ ] **WLFW QMI `0x45`**: handshake con el PD de WLAN (IND_REGISTER, MSA_INFO,
      MSA_READY, CAP, BDF_DOWNLOAD, CAL_REPORT, FW_READY; cliente `0x4b4e454c`,
      puerto `0x4003`; VMIDs 0x18/0x19 para las regiones MSA), entrega de
      `WLANMDSP.MBN` (3.86 MB) por chunks, CE rings/SRC/DST. Criterio: `wlfw 1` /
      `WLFW ready` en el `glink summary`.

## F3 — HTC/WMI/HTT + scan — ⬜ PENDIENTE

- [ ] HTC: mailbox/rings de control hacia el PD de WLAN
- [ ] WMI: servicios + `WMI_SCAN`
- [ ] HTT: data path básico
- [ ] **Criterio de éxito:** scan con SSIDs/BSSIDs reales en el log

## F4 — Assoc/WPA2 + data path + iOS — ⬜ PENDIENTE

- [ ] VDEV create/up/start, auth/assoc, 4-way handshake (topaz `0129`/`0130` como
      referencia)
- [ ] HTT TX/RX completo, `en0` Ethernet, DHCP
- [ ] `en0` + Apple80211 para que `wifid`/CoreWiFi/**Settings** lo vean
- [ ] **Criterio de éxito:** Settings lista redes, join WPA2, Safari HTTPS

---

## Trampas conocidas (no repetir)

1. **Device memory**: nada de `memset`/`memcpy` (STNP) ni stores no alineados — usar
   `io_zero()`/`io_copy()` de 0711 (byte a byte hasta el límite de 8 bytes).
2. **eMMC**: toda lectura de firmware pasa por `santoni_sdhci_lock` + retries.
3. **`ml_io_map()`/`ml_io_unmap()` no son un par** en XNU 8796: usar
   `ml_io_map_unmappable` + `ml_io_unmap` con bounds page-aligned.
4. **SMEM**: leer solo particiones con host APPS (`0`); los items cached se empacan
   desde el final con el header después del dato (SDM660).
5. **SMMU ANOC2**: sin SMRs válidos; no asumir traducción.
6. **Verificar el shim antes de interpretar un boot**: los boot-args deciden qué
   sonda corre (`sweetwlan`/`sweetmodem` vs `lavgpu`/`lavaud`).
7. **`MEM_PROT_ASSIGN` a una PA fija del DTB puede colgarse** con el Q6 ya vivo
   (0723/0724: `map ok` y nunca vuelve el SMC). El patrón que funciona es el de
   topaz: buffer `kmem_alloc_contig` del kernel y asignación a **HLOS + MSS_MSA**
   (0725).
8. **rmtfs iovec**: el `phys` que manda el módem es la dirección física **absoluta**
   del staging buffer (`ALLOC_BUFF`), no un offset; restar la PA base (0726).
9. **El USB del script muere** cuando el kernel espera READY en el bucle SMP2P
   (~`t=133`); no confundirlo con un cuelgue: la pantalla sigue y el diagnóstico
   debe leerse ahí. Aproximadamente 1 de cada 3 boots se queda en la auth del MBA;
   se recupera reiniciando.
10. **`flush_dcache64(addr, len, release)` recibe una PA**, no un VA: pasarle un VA
    de kmem panickea con `physokv_range: Illegal PA` (0729). Usar `pa + offset`.
11. **TFTP Qualcomm**: el OACK debe ecoar todas las opciones que pidió el cliente
    (`blksize`, `wsize`, `timeoutms`, `tsize`); un OACK parcial deja al módem sin
    enviar los DATA del server check (0731/0732).

## Coordinación entre agentes

- Serie WiFi/modem en branch **`wlan-0706`** (merge a `main` solo cuando F2 sea
  estable, explícitamente con el usuario).
- Rangos de parches: WiFi/modem **0700–0799**, GPU **0300+**, audio **0500+**,
  diagnóstico userland/crash **0900–1000**.
- Todos los shims llevan `sweetusb=2` para el live log (`usbnet-windows.ps1 -Log`).
