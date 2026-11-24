# Roadmap — Audio (ADSP/LPASS + PM660L) — lavender

> Estado al **2026-11-21**. Fuente de detalle: [`docs/AUDIO.md`](AUDIO.md) y
> [`docs/AUDIO-BRIEF.md`](AUDIO-BRIEF.md) (agente audio, serie de parches `0500`–`0599`,
> driver `drivers/audio/lavender_audio.c`). Este archivo es el tablero de avance:
> ~~tachado = verificado en el teléfono~~, `⬜ PENDIENTE` = falta.
> Convención: un archivo por función clave (`WIFI-ROADMAP.md`, `AUDIO-ROADMAP.md`, …).

## Objetivo final

Sonido real en iOS 16.7.16: altavoz (y auriculares) por el camino del board —
`ASM stream → ADM COPP → AFE INT4_MI2S_RX → LPASS digital codec/SWR → PM660L codec →
speaker` — expuesto al userland como `Speaker` vía un plug-in HAL
(`/System/Library/Audio/Plug-Ins/hal` + `/dev/lavender-audio`), con micrófono
(`INT3_MI2S_TX`) después.

**Hallazgo clave:** el board **no tiene WCD9335** — el DTBO vivo desactiva `slim_aud`
(y los nodos `tasha`/`tavil`). La tarjeta habilitada es `qcom,sdm660-asoc-snd` con el
códec analógico del **PM660L** (SPMI `0xf000`), el códec digital **LPASS**
(`0x152c0000`) + maestro **SoundWire** (`0x152c1000`) y el **ADSP** (Hexagon,
PAS id 1, carve-out `0x92a00000` + 30 MiB). No hay que portar WCD9335 ni tocar SLIMbus.

## Panorama

| Fase | Alcance | Estado |
|---|---|---|
| F0 | Inventario del camino de audio (DTBO, códecs, vendor XML, ADSP) | ✅ COMPLETA |
| F1 | Sondas y power-up del ADSP (`lavaud=1..3`) | ✅ COMPLETA |
| F2 | Carga PAS del ADSP + SMP2P READY (`lavaud=4..6`, parches 0503–0508) | ✅ COMPLETA (AUTH 0/0, ADSP arranca, 429+kick; GLINK sigue en F3) |
| F3 | GLINK `lpass` + APR (`0509`/`0509b`) | ✅ COMPLETA (VERSION/OPEN + APR responde `0x110e8`) |
| F4 | Códec analógico PM660L por SPMI (`0510`) | ⬜ PENDIENTE |
| F5 | Códec digital LPASS + SWR + AFE `INT4_MI2S_RX` (`0511`) | ✅ AFE COMPLETO (clock/config/start status 0) |
| F6 | ASM/ADM + tono de prueba 1 kHz (`0512`) | ⬜ PENDIENTE |
| F7 | iOS: `/dev/lavender-audio` + plug-in HAL (`0513`) | ⬜ PENDIENTE |
| F8 | Micrófono, jack/MBHC, volumen, suspend (`0514`+) | ⬜ PENDIENTE |

---

## F0 — Inventario del camino de audio — ✅ COMPLETA

- ~~DTBO vivo desactiva `slim_aud` ⇒ **no hay WCD9335** que portar (la primera
  suposición del port era incorrecta)~~
- ~~Tarjeta habilitada `qcom,sdm660-asoc-snd`: PM660L analog codec (SPMI SID 3,
  `0xf000`), LPASS digital codec `0x152c0000` + SoundWire master `0x152c1000`,
  WSA881x no montados (`qcom,wsa-disable`)~~
- ~~Mapeo del vendor (`audio_platform_info_intcodec.xml`): speaker `INT4_MI2S_RX`,
  auriculares `INT0_MI2S_RX`, mic `INT3_MI2S_TX`; ruta speaker
  `INT4 → RX4/RX5 → COMP → Spkr BOOST → SWR DAC_Port`~~
- ~~ADSP: PAS id 1, carve-out `0x92a00000` + 30 MiB; firmware `ADSP.MDT` leíble de la
  FAT16 del `modem` (y en el PC en `work/wlan/fw/NON-HLOS.bin`)~~
- ~~Plan iOS: plug-in HAL de userland (topaz `refs/ios-topaz/userland/audio/audio_plugin.c`
  como plantilla) + nodo `/dev/lavender-audio` — no hay `IOAudio2` portado~~

## F1 — Sondas y power-up del ADSP (`lavaud=1..3`) — ✅ COMPLETA (parches 0500–0502, 0505, 0507)

- ~~`0500` sonda read-only: GCC LPASS GDSC/votos, Q6SS, SMMU, códec digital, SMEM TOC +
  partición apps↔ADSP, APCS IPC (descubre los IDs reales de SMP2P/GLINK del firmware)~~
- ~~Boot de device (`0505` kernel, `lavaud=3`, log `work/wlan/audio-0505-lavaud3-2026-11-21.txt`):
  sonda completa sin colgarse; **ADSP apagado tras el ABL** (`gdscr 00000001`)~~
- ~~Hallazgo del orden de encendido: votar el branch SMMU primero queda en `80000001`
  (EN con OFF) y nunca toca el GDSC → **`0507` invierte: GDSC primero, después CBCR**
  (CI verde `44938672875`)~~
- ~~`0502` dry-run del firmware: `ADSP.MDT` 7964 B / 23 phdrs → **20 blobs B02..B21,
  29.018.084 bytes, 0 mal**, todos dentro del carve-out; hash no detectado por el parser
  viejo → `0508` lo arregla~~
- ~~SMEM: partición apps↔ADSP vacía (`used 0x20`), APCS IPC 0 — no hay corrida previa~~

## F2 — Carga PAS del ADSP + SMP2P READY (`lavaud=4..6`) — 🔄 EN CURSO (0503–0506, 0508, 0508b)

### Verificado en el teléfono (2026-11-21, kernel `0508`, `lavaud=4`)

- ~~`0507` funciona: GDSC `00000001`→`80000000` (on), CBCR `80000000`→`00000001`
  (on), Q6SS/SMMU/códec legibles~~
- ~~Dry run: hash phdr 1, 21 segmentos (con BSS `B22`), 31.352.804 B, 0 mal;
  metadata 7964 B; imagen `92a00000..94800000`, entry en rango~~
- ~~`PAS INIT_IMAGE` `ret 0 res 0` y `PAS MEM_SETUP` `ret 0 res 0`~~
- ~~Copia byte a byte (`0508b`): pasó B04 y copió los 21 segmentos (B02..B22,
  BSS incluido); el wedge de `0x92a20000` era el store de 64 bits~~
- ~~La sonda del carve-out leyó `0x0 = 5800c00e` (B02) y `0x20000 = a09dc001`
  (B04): el **ABL ya precarga el firmware**, la copia es innecesaria~~
- ~~`lavaud=5` con `lavaudcopy=0`: `AUTH_AND_RESET ret 0 res 0`; Q6SS
  `reset 00000004 → 00000001`, `pwr 030c0000 → 00000001`; el **ADSP arrancó**~~
- ~~Ítem SMP2P **443** presente (`local 2 remote 0 total 16 valid 0`), GLINK
  **478** (descriptor) y **480** (fifo1) presentes; 429/479 aún no (el AP no creó
  su lado)~~
- ⚠️ `lavaud=6 force copy=0`: `AUTH ret 0xffffffc3`; la corrida anterior del ADSP
  ensució el carve-out (`0x1df0000 ffffffff`). Hay que **copiar en cada boot**
  después del primero. `0508c` quita el `flush_dcache64` del carve-out que
  colgaba; el próximo shim es `lavaud=6 lavaudcopy=1 lavaudforce=1`.

### Logrado (escrito y CI verde; falta el boot que lo verifique)

- ~~`0503` carga por TrustZone (`lavaud=4`): rechaza SMEM con corrida previa,
  `INIT_IMAGE`/`MEM_SETUP`/copia de segmentos/`AUTH_AND_RESET`, relectura Q6SS~~
- ~~`0508` endurece lo que el log expuso: hash phdr por `p_flags` en phdr+24
  (`PT_NULL`, 7176 B), `INIT_IMAGE` = cabecera+phdrs+hash (el `p_offset` miente: el
  archivo son exactamente 788 + 7176), carga y zero del BSS `B22` (`filesz 0`,
  `0x23a000`), exige el estado SCM x1 y la entry del ELF (CI verde `44942180169`)~~
- ~~`0504` inventario post-PAS read-only (`lavaud=5`): espera ítem SMP2P del ADSP y
  dumpea la partición~~
- ~~`0505` layout **v1 packed** del SMP2P de SDM660 (distinto del de SM6225) + IDs del
  vendor: AP→ADSP **429**, ADSP→AP **443**, GLINK 478/479/480~~
- ~~`0506` (`lavaud=6`): crea el ítem SMP2P out **429** (hwlock 3, header v1
  `master-kernel`/`sleepstate`), patea el ADSP por APCS bit **10** y espera hasta 5 s el
  `slave-kernel` READY del ítem 443 — primera escritura SMEM de la serie~~

### ⬜ PENDIENTE (bloqueo actual)

- [ ] **Boot `lavaud=6 lavaudcopy=0 lavaudforce=1`**: crear el ítem SMP2P 429,
      patear APCS bit 10 y esperar `slave-kernel` en 443. Ojo: el 443 de la
      corrida anterior queda en SMEM, por eso `lavaudforce=1`.
- [ ] Si `slave-kernel` nunca aparece: revisar el layout/ítem 429 (el 443 v1
      `valid 0` puede requerir que el AP cree 429 primero) y el APCS kick.
- [ ] Boot `lavaud=5`: inventario post-PAS (ítems SMP2P que publica el ADSP al despertar).
- [ ] Boot `lavaud=6`: `slave-kernel 443` en READY tras el kick — espejo exacto del
      `0x6` que ya logró el agente WLAN con el modem.
- [ ] Si el ADSP no arranca: revisar rechazo por SMEM con corrida previa (usar
      `lavaudforce=1` como diagnóstico) y el estado x1 del SCM.

## F3 — GLINK `lpass` + APR — ✅ COMPLETA (`0509`/`0509b`)

- ~~Canal GLINK 478/479/480: descriptor 478, RX 480 (16 KiB, uncached), TX 479
  allocado por el AP en `0x4330`~~
- ~~`VERSION`/`VERSION_ACK` (ADSP features 7), `OPEN apr_audio_svc` rcid 1~~
- ~~Intents: el ADSP manda `CMD_INTENT` con `p1=2` (su canal), no el rcid; 8
  intents de 0x200/0x400. El filtro `p1==rcid` era el bug (0509b)~~
- ~~APR vivo: `GET_FWK_VERSION` (token 1) respondido con `APR_BASIC_RSP_RESULT`
  `0x000110e8`, len 40. Falta decodificar el status y mandar la primera query AFE~~
- [ ] Siguiente dentro de F3: imprimir el payload del basic response y una query
  AFE (`0x1036`/INT4) para confirmar el servicio de audio

## F4 — Códec analógico PM660L por SPMI — 🔄 EN CURSO (read-only OK)

- ~~Identificación por SPMI: arbiter lavender v3 (`0x0800f000`, observer
  `0x09400000+0x8000*apid`), **SID 3** (`pm660l@3/analog-codec@f000`)~~
- ~~`0xF004 = 0x23` (digital PERPH_TYPE, POR 0x23) y `0xF005 = 0x04` (subtipo);
  `0xF100/0xF101` analógicos en 0. El códec responde.~~
- [ ] Leer `0xF104/0xF105` (tipo/subtipo analógico) y las revisiones completas
- [ ] Supplies, reset (LPI GPIO24), MCLK 9.6 MHz y rutas RX4/RX5 → COMP →
      Spkr BOOST → SWR DAC_Port. **Requiere un camino de escritura SPMI**
      (el observer actual es read-only); diseñarlo con cuidado.
- [ ] **Criterio de éxito:** rutas del códec programadas y speaker path listo

## F5 — Códec digital LPASS + SWR + AFE — ✅ AFE COMPLETO (`0511`/`0511b`)

- [x] AFE `INT4_MI2S_RX` (0x1036): `SET_PARAM_V2` I2S 48 kHz/16-bit/SD0 y
      `DEVICE_START` → **status 0**.
- [x] Clock MI2S `0x113` (`INT4_MI2S_IBIT`) 1.536 MHz → **status 0**.
- [ ] LPASS digital codec `0x152c0000` + maestro SoundWire `0x152c1000`:
      revisar si el path PM660L los necesita o si el AFE ya cubre el I2S.
- [x] **Criterio de éxito:** el AFE acepta el puerto (clock/config/start status 0).

## F6 — ASM/ADM + tono de prueba — ⬜ PENDIENTE (`0512`)

- [ ] ASM stream + ADM COPP → `INT4_MI2S_RX`
- [ ] **Criterio de éxito: tono de 1 kHz audible en el altavoz** — primer sonido real

## F7 — iOS: `/dev/lavender-audio` + plug-in HAL — ⬜ PENDIENTE (`0513`)

- [ ] Nodo kernel `/dev/lavender-audio` (48 kHz stereo S16_LE)
- [ ] Plug-in HAL en `/System/Library/Audio/Plug-Ins/HAL` presentando `Speaker` +
      `Hawking` (plantilla topaz `audio_plugin.c`)
- [ ] **Criterio de éxito:** volumen desde iOS suena por el speaker

## F8 — Micrófono y resto — ⬜ PENDIENTE (`0514`+)

- [ ] Capture path `INT3_MI2S_TX`/ADC (micrófono interno)
- [ ] Jack/MBHC (detección por el PM660L), volumen, suspend/resume

---

## Trampas conocidas (no repetir)

1. **No portar WCD9335 ni tocar SLIMbus** — el DTBO del board los desactiva; el camino
   es PM660L + LPASS/SWR.
2. **Orden de power-up: GDSC (`0x7d034`) antes que el branch SMMU (`0x7d014`)** — el
   orden viejo votaba el branch y volvía con `EN|OFF` sin tocar el GDSC (0507).
3. **Hash del firmware ADSP**: phdr índice 1, `PT_NULL`, `p_flags 0x02200000`, 7176 B;
   `p_offset` dice 4096 pero el archivo son 788 + 7176 (cabecera+phdrs+hash) —
   `INIT_IMAGE` es esa concatenación.
4. **El phdr `B22` es BSS puro** (`filesz 0`, `memsz 0x23a000`, termina en
   `0x94800000`): hay que zeroearlo, no cargarlo (el walk viejo lo salteaba).
5. **SMP2P de SDM660 es layout v1 packed** (`feature_version`/`rem_loc_proc_id`/
   `valid_total_ent`) — no el de SM6225; IDs 429/443/478-480 (los mismos que el modem).
6. **La tabla de hash del vendor es opaca a la AP** (40 B de header + digests de 32):
   validar longitudes, no interpretar entradas.
7. Exigir el **estado x1 del SCM** y la **entry del ELF** antes de declarar PAS OK
   (0508); rechazar arranques sobre SMEM con corrida previa.

## Coordinación entre agentes

- Serie audio en parches **0500–0599** (agente audio, commits directos a `main` con
  CI verde; el boot de verificación se anuncia en STATUS.md).
- Rangos: WiFi/modem **0700–0799** (branch `wlan-0706`), GPU **0300+**, diagnóstico
  userland/crash **0900–1000**.
- Boot args: `lavaud=1..6 lavauddelay=25`; el live log por `sweetusb=2`
  (`usbnet-windows.ps1 -Log`).
