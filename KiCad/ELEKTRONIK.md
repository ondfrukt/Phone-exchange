# ELEKTRONIK – dokumentation av kretskorten

Underlag för framtida Claude-sessioner. **Källa: enbart `KiCad/`** (schemana, plus `.kicad_pro`/`.kicad_pcb` för några kortfakta). Firmware, `README.md` och `docs/` har inte använts.

**Vid konflikt mellan detta dokument och schemat gäller schemat.** Se avsnitt 13 för hur man slår upp fakta direkt ur schemat.

Konventioner i dokumentet:

- Referensbeteckningar, pinnar och nätnamn är exakt som i schemat (`U5.X15`, `/DAC1`). `ref.pinne` är en komponents pinne enligt nätlistan; pinnummer anges där de behövs.
- Nätnamn med `/` framför är lokala nätetiketter i schemat. `{slash}` i SLIC-unit-nätnamn är KiCads kodning av `/` (nätet `/Ain_0{slash}4` heter `Ain_0/4` i schemat).
- Det som inte går att belägga ur `KiCad/` står som **"Okänt – fråga Jonas"**.
- "Härlett" = slutsats dragen ur kopplingen (eller ur MCP23017-standarden), inte något som står i schemat.
- Pinntyper (in/ut) kommer från symbolernas pinntyp i schemat.

---

## 1. Översikt

Systemet är en telefonväxel byggd av två korttyper:

| Kort | Mapp | Rev. enligt filer | Senaste backup-zip |
|------|------|-------------------|--------------------|
| ESP-unit (huvudkort) | `KiCad/Exchange ESP-unit S3 V1.4/` | Schema: titel "Phone exchange - ESP-Unit", datum 2025-08-16, rev 1. PCB: datum 2025-08-17, rev 1.1. 2 kopparlager. | 2026-04-11 |
| SLIC-unit (linjekort) | `KiCad/Exchange SLIC-unit V1.3/` | Schemat har inget titelblock. 2 kopparlager. | 2026-02-07 |

Arbetsfilerna (inte backup-zip:arna) har använts och antas vara aktuella (Jonas bekräftar).

- **ESP-unit** bär ESP32-S3 (U2), två MCP23017 (U3 allmän I/O, U4 styr kopplingsmatrisen), kopplingsmatrisen MT8816 (U5), tre AD9833-tongeneratorer (U6–U8), DTMF-mottagare MT8870 (U9) med analog multiplexer TMUX4051 (U11), ljud-DAC PCM5102 (U10) samt matning (jack J1 / USB-C J7, LM1117 till 3,3 V).
- **SLIC-unit** har **4 linjer** per kort: en KS0835F (U2–U5) per linje, en MCP23017 (U1) för linjesignalerna, skydd (polyfuse, tyristorer, TVS, dioder) och skruvplint J3 mot telefonerna.
- **Koppling mellan korten:** ESP-unit **J4** (2×13, `PinSocket`) ↔ SLIC-unit **J2** (2×13, `PinHeader`). Pinnumren motsvarar varandra (SCL pin 1, SDA pin 3, GND pin 2/4/6/8, +5V pin 9/10, `Ain`/`Aout` pin 11–26 på båda korten) – härlett att kontakterna kopplas 1:1; kablaget/kortstapling står inte i schemat.
- **8 linjer = 2 SLIC-units.** Ett SLIC-kort är antingen "linje 0–3" (JP1–8 stängda) eller "linje 4–7" (JP9–16 stängda), se avsnitt 10. ESP-unit har nät `Ain_0..7`/`Aout_0..7` för 8 linjer.
- **Spänningsdomäner:** ESP32 och AD9833/PCM5102 går på 3,3 V (`+3V3`); MCP23017, MT8816, MT8870, TMUX4051 och SLIC-sidan på 5 V (`+5V`). I2C och SLIC-avbrotten nivåväxlas med BSS138 (avsnitt 4).

---

## 2. Komponentlista

### 2.1 ESP-unit V1.4

| Ref | Värde / typ | Roll |
|-----|-------------|------|
| U1 | LM1117T-3.3 (TO-220) | Spänningsregulator `+5V` (VI) → `+3V3` (VO) |
| U2 | ESP32-S3-WROOM-1-N16R2 | Mikrokontroller |
| U3 | MCP23017 | Allmän I/O-expander (MAIN): MT8870-utgångar, TMUX-adress, knapp, EX-kontakt |
| U4 | MCP23017 | I/O-expander som styr MT8816 |
| U5 | MT8816AF1 | 16×8 analog kopplingsmatris |
| U6, U7, U8 | AD9833-module ×3 | Tongeneratorer (SPI), utgång `/DAC1`, `/DAC2`, `/DAC3` |
| U9 | MT8870DSR1 | DTMF-mottagare |
| Y1 | Crystal, värde "3.579546 Hz" | Kristall för U9 OSC1/OSC2 (värdet är sannolikt skrivfel, se avsnitt 12) |
| U10 | PCM5102APW | I2S-DAC (ljud ut) |
| U11 | TMUX4051 | 8:1 analog multiplexer för DTMF-lyssning |
| D1 BLUE, D2 RED, D3 YELLOW | LED 0805 | Se avsnitt 11 |
| D8 RED | LED 0805 | LED från `+5V` via R4 (1K) – sannolikt strömindikering (härlett) |
| D4, D5 | PMEG4050EP | Dioder från J1 resp. USB VBUS till `+5V` (ORing) |
| D6 | SMBJ5.0A | TVS `+5V` ↔ GND |
| D7 | SMLVT3V3 | TVS `+3V3` ↔ GND |
| D17 | SS110F (Schottky) | A=`+3V3`, K=`+5V` (se avsnitt 3) |
| J1 | Jack-DC | Strömingång (spänning: Okänt – fråga Jonas) |
| J2 | Conn_02x02 | Ljudutgång: pin 1 `/D_OUTL`, pin 2 `/D_OUTR`, pin 3–4 GND |
| J4 | Conn_02x13 socket | Till SLIC-unit J2 |
| J5 | Conn_01x06 | I2C-utgång (5 V-sidan) + matning |
| J6 | Conn_02x05 | JTAG |
| J7 | USB-C (USB2.0, 16P) | USB: matning + D+/D− |
| J8 | Conn_02x03 | EX1–EX5 + GND |
| SW1 / SW2 / SW3 | Tryckknappar | "Function Button" / "Boot button" / "Reset button" |
| Q1, Q2 | BSS138 | Nivåväxlare I2C (SDA, SCL) |
| 2 st `Q?` | BSS138 | Nivåväxlare SLIC-avbrott (ej annoterade, se 4.4 och 12) |
| TP1–TP4 | Testpunkter | TP1 `+5V`, TP2 `+3V3`, TP3 GND, TP4 `Net-(D5-A)` (USB VBUS) |
| H1–H4 | Monteringshål (M3), anslutna till GND | |
| C1, C4, C11, C13, C18, C19, C28, C32 | 0,1 µF | Avkoppling `+5V`/`+3V3` mot GND (C4, C11, C13, C32 på `+5V`; C1, C18, C19, C28 på `+3V3`) |
| C2, C20, C27 | 10 µF | Avkoppling `+3V3` mot GND |
| C9, C10, C12 | 10 µF (polariserade) | Avkoppling `+5V` mot GND |
| C3 | 47 µF (pol.) | `+5V` mot GND |
| C17 | 47 µF (pol.) | `+3V3` mot GND |
| C5 (0,1 µF), C6 (10 µF) | | `Net-(D4-A)` (J1-sidan) mot GND |
| C7 (1 µF) | | `Net-(U2-EN)` mot GND |
| C8 (0,1 µF) | | `Net-(C8-Pad2)` (SW3/R8) mot GND |
| C14 (0,1 µF) | | AC-koppling `/DTMF` → `Net-(C14-Pad1)` → R30 |
| C15 (0,1 µF) | | `+5V` ↔ `Net-(U9-ST/GT)` |
| C16 (0,1 µF) | | `Net-(U2-IO0)` mot GND |
| C21 (2,2 µF) | | U10.CAPP ↔ U10.CAPM |
| C22 (2,2 µF) | | U10.VNEG ↔ U10.CPGND |
| C23, C24 (2,2 nF) | | `/D_OUTL` resp. `/D_OUTR` mot GND |
| C25 (10 µF), C26 (0,1 µF) | | U10.LDOO mot GND |
| R1, R2, R5 (100 Ω) | | LED-serieresistorer D1, D2, D3 |
| R4 (1K) | | LED-serieresistor D8 (från `+5V`) |
| R3, R25 (5,1K) | | CC1 resp. CC2 på USB-C mot GND |
| R6 (10K) | | Pull-up `Net-(U2-EN)` → `+3V3` |
| R7 (10K) | | Pull-up `Net-(U2-IO0)` → `+3V3` |
| R8 (0 Ω) | | `Net-(U2-EN)` ↔ `Net-(C8-Pad2)` (till SW3) |
| R9 (4,7K), R10 (10K) | | Pull-ups `/SDA_3V` → `+3V3` |
| R12 (10K), R18 (4,7K) | | Pull-ups `/SCL_3V` → `+3V3` |
| R11 (10K), R13 (10K) | | Pull-ups `/SDA` resp. `/SCL` → `+5V` |
| R14 (10K) | | `Net-(U4-A0)` → `+5V` (sätter U4 A0 hög) |
| R15, R16, R22 (22 Ω) | | Serie i I2S: `/BCK`, `/DIN`, `/LRCK` |
| R17 (10K), R29 (10K) | | Pull-up ~RESET U4 resp. U3 → `+5V` |
| R19, R20 (10K) | | `+3V3` ↔ `/MCP_SLIC_INT_1` resp. `/MCP_SLIC_INT_2` (5 V-sidan! se 4.4) |
| R21 (10K) | | `+3V3` ↔ `/MCP_MAIN_INT_PIN` |
| R30 (100K), R32 (100K), R31 (390K) | | Ingångs-/återkopplingsnät kring MT8870 (se avsnitt 8) |
| R41, R42, R43 (10K) | | Pull-ups `+3V3` ↔ `/CS2`, `/CS3`, `/CS1` |
| 4 st `R?` (10K) | | Pull-ups i SLIC-avbrottens nivåväxlare (ej annoterade, se 4.4 och 12) |

### 2.2 SLIC-unit V1.3

| Ref | Värde / typ | Roll |
|-----|-------------|------|
| U1 | MCP23017 | I/O-expander för SHK/RM/FR/PD, 4 linjer |
| U2, U3, U4, U5 | KS0835F | Linjekrets (SLIC), en per linje (lokal linje 0, 1, 2, 3) |
| F1–F8 | MF-R025 (polyfuse) | Serie i RING/TIP mot J3 (F1,F3,F5,F7 = RING; F2,F4,F6,F8 = TIP) |
| RV1–RV12 | P3100SC | Tyristorskydd, 3 per linje (se avsnitt 9) |
| D18–D21 | P6KE51A | TVS, en per linje |
| D2–D17 | M4 (SM4004) | 4 dioder per linje på RING-noden (se avsnitt 9) |
| D22–D25 | 1N4148WS | I serie med ~PD per linje |
| D1 | LED "POWER LED" | Strömindikering via R5 (1k) från `+5V` |
| R1 (10k) | | Pull-up ~RESET U1 → `+5V` |
| R2, R3, R4 (10k) | | Pull-up A2, A1, A0 på U1 → `+5V` (se JP19–21) |
| R5 (1k) | | LED-resistor D1 |
| R6–R9 (1M) | | Tvärs över TIP/RING per linje (R6=linje 0 … R9=linje 3) |
| C1 (0,1 µF), C2 (10 µF pol.) | | Avkoppling `+5V` mot GND |
| C11–C14 (0,1 µF), C15–C18 (10 µF pol.) | | Avkoppling `+5V` mot GND |
| C3–C6 (0,1 µF) | | `Audio_in` per linje (C3=linje 0 … C6=linje 3) |
| C7–C10 (0,1 µF) | | `Audio_out` per linje (C7=linje 0 … C10=linje 3) |
| JP1–JP21 | SolderJumper_2_Open | Se avsnitt 10 |
| J2 | Conn_02x13 header | Till ESP-unit J4 |
| J3 | Screw_Terminal_01x10 | Mot telefonerna (avsnitt 9) |
| TP1 `+5V`, TP2 GND | Testpunkter | |
| H1–H4 | Monteringshål, anslutna till GND | |

---

## 3. Strömförsörjning och skydd

### 3.1 ESP-unit V1.4

- **Ingångar:** J1 (Jack-DC, pin 1 → `Net-(D4-A)`, pin 2 → GND) och J7 USB-C (VBUS-pinnar A4/A9/B4/B9 → `Net-(D5-A)`, GND-pinnar → GND). **J1:s matningsspänning och polaritet: Okänt – fråga Jonas.** TVS-valet (D6 SMBJ5.0A på `+5V`) antyder 5 V (härlett, inte belagt).
- **ORing:** D4 (A=`Net-(D4-A)`/J1, K=`+5V`) och D5 (A=`Net-(D5-A)`/VBUS, K=`+5V`) – bägge PMEG4050EP. `+5V` matas alltså av den högsta av J1 och USB.
- **`+5V` → `+3V3`:** U1 LM1117T-3.3 (VI=`+5V` pin 3, VO=`+3V3` pin 2, GND pin 1).
- **D17 SS110F:** A=`+3V3`, K=`+5V`, dvs. parallellt med U1 med anoden på utgången och katoden på ingången. Härlett: skyddsdiod som hindrar att `+3V3` hamnar över `+5V` (t.ex. när `+5V` försvinner) – roll ej beskriven i schemat.
- **Skydd:** D6 SMBJ5.0A (TVS, A1=`+5V`, A2=GND), D7 SMLVT3V3 (TVS, A1=`+3V3`, A2=GND).
- **Matade från `+3V3`:** U2, U6–U8 (VCC), U10 (AVDD, DVDD, CPVDD, XSMT), pull-ups R6, R7, R9, R10, R12, R18, R19, R20, R21, R41–R43, gate på Q1/Q2/`Q?`, J5 pin 5, J6 pin 2.
- **Matade från `+5V`:** U3, U4 (VDD), U5 (VDD pin 38), U9 (VDD, TOE), U11 (VDD), J4 pin 9/10, J5 pin 4, pull-ups R11, R13, R14, R17, R29.
- **Testpunkter:** TP1 `+5V`, TP2 `+3V3`, TP3 GND, TP4 USB VBUS (`Net-(D5-A)`).
- Strömbegränsning/säkring på ingångarna: ingen i schemat.

### 3.2 SLIC-unit V1.3

- **`+5V`** kommer in på J2 pin 9/10 och matar U1 (VDD) samt U2–U5 (`+VDC`, pin 10) och pull-up-motstånden R1–R5. GND på J2 pin 2/4/6/8.
- **`GNDPWR`** är ett eget nät med endast J3 pin 2 och åtta tyristorer (RV2, RV3, RV5, RV6, RV8, RV9, RV11, RV12). I schemat finns **ingen koppling mellan `GNDPWR` och `GND`**. **Vad `GNDPWR` fysiskt är kopplat till (J3 pin 2) och om nätet binds till GND på PCB:n: Okänt – fråga Jonas.**
- **Linjeskydd per linje** (detaljer i avsnitt 9): polyfuse (F) i TIP och RING, tyristorskydd (RV), TVS (D18–D21), dioder (D2–D17) och 1 MΩ tvärs över TIP/RING.
- J3 pin 1 är oansluten (`unconnected-(J3-Pin_1-Pad1)`).

---

## 4. Bussar och signalnät

### 4.1 I2C (ESP-unit V1.4)

- **ESP-sida (3,3 V):** `/SDA_3V` = U2.IO9 (pin 17), `/SCL_3V` = U2.IO10 (pin 18). Pull-ups mot `+3V3`: SDA_3V R9 (4,7K) ‖ R10 (10K); SCL_3V R12 (10K) ‖ R18 (4,7K).
- **Nivåväxling:** Q1 (SDA): G=`+3V3`, S=`/SDA_3V`, D=`/SDA`. Q2 (SCL): G=`+3V3`, S=`/SCL_3V`, D=`/SCL`.
- **5 V-sida:** `/SDA` (pull-up R11 10K → `+5V`) och `/SCL` (R13 10K → `+5V`) går till U3 (SDA pin 13, SCL pin 12), U4 (samma pinnar), J4 pin 3 (SDA) / pin 1 (SCL) och J5 pin 1 (SDA) / pin 2 (SCL).
- **Kretsar på 5 V-bussen:** U3, U4 (ESP-unit), U1 (SLIC-unit, SDA pin 13, SCL pin 12 via J2 pin 3 / pin 1). Adresser: avsnitt 5.

### 4.2 SPI mot AD9833 (ESP-unit V1.4)

| Nät | ESP-pinne | AD9833-pinne (U6, U7, U8) |
|-----|-----------|---------------------------|
| `/SCLK` | U2.IO12 (pin 20) | SCLK (pin 4) på alla tre |
| `/MOSI` | U2.IO1 (pin 39) | SDATA (pin 3) på alla tre |
| `/CS1` | U2.IO5 (pin 5) | U6.FSYNC (pin 5) |
| `/CS2` | U2.IO4 (pin 4) | U7.FSYNC |
| `/CS3` | U2.IO2 (pin 38) | U8.FSYNC |

`/CS1`, `/CS2`, `/CS3` har pull-ups (R43, R41, R42, 10K) mot `+3V3`. Ingen MISO.

### 4.3 I2S mot PCM5102 (ESP-unit V1.4)

| Nät | ESP-pinne | Serieresistor | PCM5102 (U10) |
|-----|-----------|---------------|---------------|
| `/BCK` | U2.IO37 (pin 30) | R15 22 Ω | BCK (pin 13) via `Net-(U10-BCK)` |
| `/DIN` | U2.IO36 (pin 29) | R16 22 Ω | DIN (pin 14) via `Net-(U10-DIN)` |
| `/LRCK` | U2.IO35 (pin 28) | R22 22 Ω | LRCK (pin 15) via `Net-(U10-LRCK)` |

SCK (pin 12), FMT (16), DEMP (10), FLT (11) ligger på GND; XSMT (17) på `+3V3`. Ingen MCLK från ESP.

### 4.4 Avbrottsnät (ESP-unit V1.4 och SLIC-unit V1.3)

- **MCP-kretsarnas INTA och INTB är sammankopplade till ett nät per krets** (båda pinnarna på samma nät):
  - U3 INTA (pin 20) + INTB (pin 19) → `/MCP_MAIN_INT_PIN` → U2.IO18 (pin 11); R21 (10K) mot `+3V3`.
  - U4 INTA + INTB → `/MCP_MT8816_INT_PIN` → U2.IO17 (pin 10); inget motstånd.
  - SLIC-unit U1 INTA (pin 20) + INTB (pin 19) → `/MCP_SLIC_INT_1/2` → JP17 / JP18 (avsnitt 10).
- **Observation (nivå):** U3 och U4 matas med `+5V` men deras avbrottsnät går direkt till 3,3 V-ESP:ns IO17/IO18 utan nivåväxlare. Inte nödvändigtvis fel (beror på MCP:ns utgångskonfiguration, som inte syns i schemat).
- **SLIC-avbrott, väg in till ESP:** J4 pin 7 = `/MCP_SLIC_INT_1`, J4 pin 5 = `/MCP_SLIC_INT_2` → nivåväxlare → `/MCP_SLIC_INT_1_3V` → U2.IO11 (pin 19), `/MCP_SLIC_INT_2_3V` → U2.IO14 (pin 22).
- **De sex oannoterade delarna (2 st `Q?`, 4 st `R?`) är verkliga komponenter** (Jonas bekräftat: ännu utan löpnummer). I nätlistan slås alla `Q?` ihop till en komponent och alla `R?` till en. Därför har de skiljts åt genom att läsa symbolerna i `.kicad_sch` (en tillfällig omnumrerad kopia exporterades; originalet är orört). Resultat, med symbolernas positioner i `Exchange ESP-unit S3.kicad_sch` (mm):

  | Del (position) | Roll | Kopplingar |
  |----------------|------|------------|
  | `Q?` (334.01, 217.17) BSS138 | Nivåväxlare för **INT_1** | G=`+3V3`, D=`/MCP_SLIC_INT_1`, S=`/MCP_SLIC_INT_1_3V` |
  | `R?` (326.39, 214.63) 10K | Pull-up 5 V-sidan INT_1 | `+5V` ↔ `/MCP_SLIC_INT_1` |
  | `R?` (341.63, 214.63) 10K | Pull-up 3,3 V-sidan INT_1 | `+3V3` ↔ `/MCP_SLIC_INT_1_3V` |
  | `Q?` (334.01, 240.03) BSS138 | Nivåväxlare för **INT_2** | G=`+3V3`, D=`/MCP_SLIC_INT_2`, S=`/MCP_SLIC_INT_2_3V` |
  | `R?` (326.39, 236.22) 10K | Pull-up 5 V-sidan INT_2 | `+5V` ↔ `/MCP_SLIC_INT_2` |
  | `R?` (341.63, 236.22) 10K | Pull-up 3,3 V-sidan INT_2 | `+3V3` ↔ `/MCP_SLIC_INT_2_3V` |

  Tvetydigheten är därmed löst; ingen "oklart" kvarstår för vilken del som hör till vilket avbrott.
- **Avvikelse mot förväntad uppbyggnad (observation):** R19 (10K) går från `+3V3` till `/MCP_SLIC_INT_1` och R20 (10K) från `+3V3` till `/MCP_SLIC_INT_2` – alltså till **5 V-sidans** nät, inte till `..._3V`. 5 V-sidan har därmed pull-up till både `+5V` (`R?`) och `+3V3` (R19/R20); vilonivån blir en spänningsdelning (härlett: ca 4,15 V om ingen annan last drar). Roll/avsikt: **Okänt – fråga Jonas.**

### 4.5 JTAG och USB (ESP-unit V1.4)

- **JTAG J6:** pin 1 `/TMS` (U2.IO42), pin 2 `+3V3`, pin 3 `/TCK` (U2.IO39), pin 4/6/8/10 GND, pin 5 `/TDO` (U2.IO40), pin 7 `/TDI` (U2.IO41), pin 9 oansluten.
- **USB-C J7:** `/D+` (A6, B6) → U2.IO20 (pin 14); `/D-` (A7, B7) → U2.IO19 (pin 13). CC1 → R3 (5,1K) → GND, CC2 → R25 (5,1K) → GND. SBU1/SBU2 oanslutna. Skärm S1 → GND.
- **Reset/boot:** `Net-(U2-EN)`: R6 (10K → `+3V3`), C7 (1 µF → GND), R8 (0 Ω) → `Net-(C8-Pad2)` med C8 (0,1 µF → GND) och SW3 (→ GND). `Net-(U2-IO0)`: R7 (10K → `+3V3`), C16 (0,1 µF → GND), SW2 (→ GND).

---

## 5. MCP23017-tabeller

I2C-adress härleds ur MCP23017-standarden 0x20 + (A2·4 + A1·2 + A0) – **härledd, inte verifierad mot något annat än A0–A2-kopplingen**. Pinne → pinnummer (samma på alla tre kretsar): GPA0–7 = 21–28, GPB0–7 = 1–8; A0/A1/A2 = pin 15/16/17; ~RESET = pin 18; INTB = 19, INTA = 20.

### 5.1 ESP-unit U3 (MAIN)

- A0, A1, A2 → alla GND ⇒ härledd adress **0x20**. ~RESET via R29 (10K) mot `+5V` (ingen reset-signal från ESP).

| Pinne | Nät | Ansluten till | Riktning (som kopplat) |
|-------|-----|----------------|-------------------------|
| GPA0 | `/TM_A0` | U11.A0 (pin 11) | Ut (till TMUX) |
| GPA1 | `/TM_A1` | U11.A1 (pin 10) | Ut |
| GPA2 | `/TM_A2` | U11.A2 (pin 9) | Ut |
| GPA3 | `/EX5` | J8 pin 5 | Okänt – fråga Jonas (EX-kontakt, riktning styrs av vad som kopplas dit) |
| GPA4 | `/EX3` | J8 pin 3 | Okänt – fråga Jonas |
| GPA5 | `/EX4` | J8 pin 4 | Okänt – fråga Jonas |
| GPA6 | `/EX1` | J8 pin 1 | Okänt – fråga Jonas |
| GPA7 | `/EX2` | J8 pin 2 | Okänt – fråga Jonas |
| GPB0 | `unconnected-(U3-GPB0-Pad1)` | – | Oanvänd |
| GPB1 | `/Function_button` | SW1 pin 2 (SW1 pin 1 → GND) | In (knapp mot GND; ingen extern pull-up i schemat) |
| GPB2 | `/PWDN_MT8870` | U9.PWDN (pin 6) | Ut |
| GPB3 | `/STD` | U9.STD (pin 15, utgång på MT8870) | In |
| GPB4 | `/Q4` | U9.Q4 (pin 14, utgång) | In |
| GPB5 | `/Q3` | U9.Q3 (pin 13, utgång) | In |
| GPB6 | `/Q2` | U9.Q2 (pin 12, utgång) | In |
| GPB7 | `/Q1` | U9.Q1 (pin 11, utgång) | In |

Avbrott: INTA+INTB → `/MCP_MAIN_INT_PIN` → U2.IO18.

### 5.2 ESP-unit U4 (MT8816-styrning)

- A0 → `Net-(U4-A0)` → R14 (10K) → `+5V` (hög); A1, A2 → GND ⇒ härledd adress **0x21**. ~RESET via R17 (10K) mot `+5V`.

| Pinne | Nät | MT8816 (U5) | Riktning |
|-------|-----|-------------|----------|
| GPA0 | `Net-(U4-GPA0)` | RESET (pin 42) | Ut |
| GPA1 | `Net-(U4-GPA1)` | DATA (pin 36) | Ut |
| GPA2 | `Net-(U4-GPA2)` | STROBE (pin 14) | Ut |
| GPA3 | `Net-(U4-GPA3)` | CS (pin 34) | Ut |
| GPA4–GPA7 | `unconnected-(U4-GPA4..7)` | – | Oanvända |
| GPB0 | `Net-(U4-GPB0)` | AX0 (pin 44) | Ut |
| GPB1 | `Net-(U4-GPB1)` | AX1 (pin 18) | Ut |
| GPB2 | `Net-(U4-GPB2)` | AX2 (pin 19) | Ut |
| GPB3 | `Net-(U4-GPB3)` | AX3 (pin 43) | Ut |
| GPB4 | `Net-(U4-GPB4)` | AY0 (pin 20) | Ut |
| GPB5 | `Net-(U4-GPB5)` | AY1 (pin 21) | Ut |
| GPB6 | `Net-(U4-GPB6)` | AY2 (pin 40) | Ut |
| GPB7 | `unconnected-(U4-GPB7-Pad8)` | – | Oanvänd |

Avbrott: INTA+INTB → `/MCP_MT8816_INT_PIN` → U2.IO17.

### 5.3 SLIC-unit U1

- A2/A1/A0 hålls höga av R2/R3/R4 (10k mot `+5V`) och dras låga av stängda JP19/JP20/JP21 (se avsnitt 10). Standardläge (alla jumprar öppna) ⇒ härledd adress **0x27**. ~RESET via R1 (10k) mot `+5V`.
- Alla 16 pinnar används. Ut/in enligt KS0835F-symbolens pinntyper (SHK = utgång på KS0835F ⇒ ingång på MCP; RM, FR, ~PD = ingångar på KS0835F ⇒ utgångar på MCP).

| Pinne | Nät (lokal linje) | KS0835F-pinne | Riktning (MCP) |
|-------|-------------------|---------------|-----------------|
| GPA0 | `/PD_0/4` | D22 (K) → U2.~PD | Ut |
| GPA1 | `/PD_1/5` | D23 (K) → U3.~PD | Ut |
| GPA2 | `/FR_1/5` | U3.F/R | Ut |
| GPA3 | `/RM_1/5` | U3.RM | Ut |
| GPA4 | `/SHK_1/5` | U3.SHK | In |
| GPA5 | `/SHK_0/4` | U2.SHK | In |
| GPA6 | `/RM_0/4` | U2.RM | Ut |
| GPA7 | `/FR_0/4` | U2.F/R | Ut |
| GPB0 | `/SHK_2/6` | U4.SHK | In |
| GPB1 | `/RM_2/6` | U4.RM | Ut |
| GPB2 | `/FR_2/6` | U4.F/R | Ut |
| GPB3 | `/SHK_3/7` | U5.SHK | In |
| GPB4 | `/RM_3/7` | U5.RM | Ut |
| GPB5 | `/FR_3/7` | U5.F/R | Ut |
| GPB6 | `/PD_2/6` | D24 (K) → U4.~PD | Ut |
| GPB7 | `/PD_3/7` | D25 (K) → U5.~PD | Ut |

Avbrott: INTA+INTB → `/MCP_SLIC_INT_1/2` → JP17 / JP18.

---

## 6. Ljudvägen: MT8816 (ESP-unit V1.4, U5)

`Aout_n` (linjens ljud **ut** från SLIC, dvs. KS0835F `Audio_out`) ligger på **X-sidan**; `Ain_n` (ljud **in** till linjen, KS0835F `Audio_in`) ligger på **Y-sidan**.

| X-ingång (pin) | Nät | Ansluten till |
|----------------|-----|----------------|
| X0 (31) – X5 (26) | `/Aout_0` … `/Aout_5` | J4 pin 20, 22, 24, 26, 19, 21; TMUX U11 S0…S5 |
| X6 (4), X7 (5) | `/Aout_6`, `/Aout_7` | J4 pin 23, 25; U11 S6, S7 |
| X8 (6) – X11 (9) | `unconnected-(U5-X8..X11)` | Oanslutna |
| X12 (25) | `/D_OUTL` | U10.OUTL (pin 6), C23, J2 pin 1 |
| X13 (24) | `/DAC3` | U8.OUT |
| X14 (2) | `/DAC2` | U7.OUT |
| X15 (3) | `/DAC1` | U6.OUT |

Detalj `Aout_n` → X/TMUX-S/J4: `Aout_0` X0 (31) / S0 (13) / J4.20; `Aout_1` X1 (30) / S1 (14) / J4.22; `Aout_2` X2 (29) / S2 (15) / J4.24; `Aout_3` X3 (28) / S3 (12) / J4.26; `Aout_4` X4 (27) / S4 (1) / J4.19; `Aout_5` X5 (26) / S5 (5) / J4.21; `Aout_6` X6 (4) / S6 (2) / J4.23; `Aout_7` X7 (5) / S7 (4) / J4.25.

| Y-ingång (pin) | Nät | J4-pinne |
|----------------|-----|----------|
| Y0 (32) | `/Ain_0` | 12 |
| Y1 (35) | `/Ain_1` | 14 |
| Y2 (37) | `/Ain_2` | 16 |
| Y3 (39) | `/Ain_3` | 18 |
| Y4 (17) | `/Ain_4` | 11 |
| Y5 (15) | `/Ain_5` | 13 |
| Y6 (13) | `/Ain_6` | 15 |
| Y7 (10) | `/Ain_7` | 17 |

`/D_OUTR` (U10.OUTR) går **inte** till matrisen, bara till J2 pin 2 och C24.

**Styrpinnar (alla insignaler på U5, från U4 – avsnitt 5.2):** RESET (42) ← U4.GPA0, DATA (36) ← GPA1, STROBE (14) ← GPA2, CS (34) ← GPA3, AX0 (44) ← GPB0, AX1 (18) ← GPB1, AX2 (19) ← GPB2, AX3 (43) ← GPB3, AY0 (20) ← GPB4, AY1 (21) ← GPB5, AY2 (40) ← GPB6. **Matning:** VDD (38) = `+5V`; VSS (12) och VEE (16) = GND. N.C.-pinnar (1, 11, 22, 23, 33, 41) oanslutna.

---

## 7. Toner och ljud-in (ESP-unit V1.4)

- **AD9833 ×3** (U6, U7, U8; moduler med pinnarna 1 VCC, 2 DGND, 3 SDATA, 4 SCLK, 5 FSYNC, 6 AGND, 7 OUT): VCC = `+3V3`, DGND/AGND = GND, SPI-nät enligt avsnitt 4.2. Utgångar: U6.OUT → `/DAC1` → U5.X15; U7.OUT → `/DAC2` → U5.X14; U8.OUT → `/DAC3` → U5.X13. Chip select: U6 `/CS1`, U7 `/CS2`, U8 `/CS3`.
- **PCM5102APW** (U10): I2S enligt 4.3. `OUTL` (pin 6) → `/D_OUTL` → U5.X12, C23 (2,2 nF → GND) och J2 pin 1. `OUTR` (pin 7) → `/D_OUTR` → C24 (2,2 nF → GND) och J2 pin 2. Stödkomponenter: C21 (CAPP–CAPM), C22 (VNEG–CPGND), C25 + C26 på LDOO (pin 18). Observation om CPGND i avsnitt 12.

---

## 8. DTMF-mottagning (ESP-unit V1.4)

- **TMUX4051 (U11):** S0–S7 = `/Aout_0..7` (pin 13, 14, 15, 12, 1, 5, 2, 4); adress A0/A1/A2 (pin 11/10/9) ← U3 GPA0/GPA1/GPA2 (`/TM_A0..2`); D (pin 3) → `/DTMF`; EN (pin 6), VSS (7), GND (8) = GND; VDD (16) = `+5V`. TMUX är alltså en "lyssna på linje n"-väljare som ligger parallellt med X0–X7 på matrisen.
- **MT8870 (U9)** ingångskrets: `/DTMF` → C14 (0,1 µF) → `Net-(C14-Pad1)` → R30 (100K) → `Net-(U9-IN-)` (IN−, pin 2); R32 (100K) mellan IN− och GS (pin 3); IN+ (pin 1) kopplad till VREF (pin 4) via `Net-(U9-IN+)`.
- **Övrigt U9:** EST (16) ← R31 (390K) → ST/GT (17) ← C15 (0,1 µF) mot `+5V`; OSC1 (7) / OSC2 (8) ← Y1; TOE (10) = `+5V`; INH (5) = GND; VDD (18) = `+5V`; VSS (9) = GND; PWDN (6) ← U3.GPB2.
- **Utgångar:** Q1 (11) → U3.GPB7, Q2 (12) → GPB6, Q3 (13) → GPB5, Q4 (14) → GPB4, STD (15) → U3.GPB3.
- **Y1:** värde "3.579546 Hz" (se avsnitt 12), pin 1 → OSC1, pin 2 → OSC2.

---

## 9. SLIC-unit per linje (V1.3)

**KS0835F-pinnar** (U2–U5): 1 RING, 2 TIP, 3 F/R (in), 4 RM (in), 5 SHK (ut), 6 Audio_in (in), 7 Audio_out (ut), 8 N.C. (oansluten), 9 GND, 10 +VDC (`+5V`), 11 ~PD (in).

| Lokal linje | 0 (ESP-linje 0 eller 4) | 1 (1 / 5) | 2 (2 / 6) | 3 (3 / 7) |
|-------------|-------------------------|-----------|-----------|-----------|
| KS0835F | U2 | U3 | U4 | U5 |
| SHK → MCP U1 | GPA5 (pin 26) | GPA4 (pin 25) | GPB0 (pin 1) | GPB3 (pin 4) |
| RM ← MCP U1 | GPA6 (27) | GPA3 (24) | GPB1 (2) | GPB4 (5) |
| FR ← MCP U1 | GPA7 (28) | GPA2 (23) | GPB2 (3) | GPB5 (6) |
| PD ← MCP U1 | GPA0 (21) | GPA1 (22) | GPB6 (7) | GPB7 (8) |
| PD-diod (K mot MCP) | D22 | D23 | D24 | D25 |
| J3 RING | pin 10 | pin 8 | pin 6 | pin 4 |
| J3 TIP | pin 9 | pin 7 | pin 5 | pin 3 |
| Polyfuse RING / TIP | F1 / F2 | F3 / F4 | F5 / F6 | F7 / F8 |
| Nät RING / TIP (kortutgång) | `/RING_0/4` / `/TIP_0/4` | `/RING_1/5` / `/TIP_1/5` | `/RING_2/6` / `/TIP_2/6` | `/RING_3/7` / `/TIP_3/7` |
| TIP–RING-motstånd | R6 (1M) | R7 (1M) | R8 (1M) | R9 (1M) |
| Tyristorer RING–TIP / TIP–GNDPWR / RING–GNDPWR | RV1 / RV2 / RV3 | RV4 / RV5 / RV6 | RV7 / RV8 / RV9 | RV10 / RV11 / RV12 |
| Dioder på RING-noden | D2, D3 (A=GND); D4, D5 (K → TVS) | D6, D8 (A=GND); D7, D9 (K → TVS) | D10, D11 (A=GND); D12, D13 (K → TVS) | D14, D16 (A=GND); D15, D17 (K → TVS) |
| TVS | D18 | D19 | D20 | D21 |
| Audio_in via kondensator | C3 → `Ain_0/4` | C4 → `Ain_1/5` | C5 → `Ain_2/6` | C6 → `Ain_3/7` |
| Audio_out via kondensator | C7 → `Aout_0/4` | C8 → `Aout_1/5` | C9 → `Aout_2/6` | C10 → `Aout_3/7` |

Vägen TIP/RING: KS0835F TIP (pin 2) → F(TIP) → `/TIP_n` → J3; KS0835F RING (pin 1) → F(RING) → `/RING_n` → J3. Mellan KS0835F och polyfuse (alltså på kretssidan): R(1M) mellan RING och TIP; RV (RING–TIP); RV (TIP–`GNDPWR`); RV (RING–`GNDPWR`). Dioderna sitter på RING-noden (kretssidan): två parallella dioder med A på GND och K på noden (D2+D3 för linje 0) samt två med A på noden och K på en TVS-nod (D4+D5, TVS D18 till GND). TIP-sidan har ingen sådan diodkoppling.

Ljud: `Audio_in`/`Audio_out` (KS0835F pin 6/7) → 0,1 µF-kondensator → nät `Ain_n/n+4` / `Aout_n/n+4` → solder-jumper (avsnitt 10) → J2.

**J3 (skruvplint, 10 poler):** pin 1 oansluten, pin 2 `GNDPWR`, pin 3 TIP_3/7, 4 RING_3/7, 5 TIP_2/6, 6 RING_2/6, 7 TIP_1/5, 8 RING_1/5, 9 TIP_0/4, 10 RING_0/4.

---

## 10. Solder jumpers och inställningar (SLIC-unit V1.3)

Alla 21 jumprar är `SolderJumper_2_Open` (öppna som standard enligt symbolen). A/B är symbolens pinnar.

| Jumper | Kopplar | Till J2-pinne | Hör till |
|--------|---------|---------------|----------|
| JP1 | `Ain_0/4` | 12 | linje 0 |
| JP2 | `Ain_1/5` | 14 | linje 1 |
| JP3 | `Ain_2/6` | 16 | linje 2 |
| JP4 | `Ain_3/7` | 18 | linje 3 |
| JP5 | `Aout_0/4` | 20 | linje 0 |
| JP6 | `Aout_1/5` | 22 | linje 1 |
| JP7 | `Aout_2/6` | 24 | linje 2 |
| JP8 | `Aout_3/7` | 26 | linje 3 |
| JP9 | `Ain_0/4` | 11 | linje 4 |
| JP10 | `Ain_1/5` | 13 | linje 5 |
| JP11 | `Ain_2/6` | 15 | linje 6 |
| JP12 | `Ain_3/7` | 17 | linje 7 |
| JP13 | `Aout_0/4` | 19 | linje 4 |
| JP14 | `Aout_1/5` | 21 | linje 5 |
| JP15 | `Aout_2/6` | 23 | linje 6 |
| JP16 | `Aout_3/7` | 25 | linje 7 |
| JP17 | `/MCP_SLIC_INT_1/2` | 7 (`MCP_SLIC_INT_1`) | INT-kanal 1 |
| JP18 | `/MCP_SLIC_INT_1/2` | 5 (`MCP_SLIC_INT_2`) | INT-kanal 2 |
| JP19 | U1.A2 (pin 17) → GND när sluten | – | adressbit A2 |
| JP20 | U1.A1 (pin 16) → GND när sluten | – | adressbit A1 |
| JP21 | U1.A0 (pin 15) → GND när sluten | – | adressbit A0 |

**Konfigurera ett SLIC-kort:**

- **Linje 0–3:** stäng JP1–JP8. **Linje 4–7:** stäng JP9–JP16. Stäng inte båda grupperna på samma kort (härlett: det skulle koppla ihop linje n och n+4 på ESP-sidan, t.ex. `Ain_0` med `Ain_4`).
- **INT-kanal:** stäng JP17 (till J2 pin 7 = `MCP_SLIC_INT_1`) eller JP18 (till pin 5 = `MCP_SLIC_INT_2`). Två SLIC-kort behöver olika kanaler om båda ska rapportera avbrott (härlett: annars delas ett nät).
- **I2C-adress:** härledd adress = 0x20 + (A2·4 + A1·2 + A0), där en *stängd* jumper ger 0 (GND) och en *öppen* ger 1 (pull-up R2/R3/R4). Alla öppna ⇒ 0x27; alla stängda ⇒ 0x20.
  - Exempel: "linje 4–7 med adress 0x23" ⇒ stäng JP9–JP16 och **JP19** (A2=0), lämna JP20 och JP21 öppna (A1=1, A0=1) ⇒ 0x23.
- Vilka adresser/INT-kanaler som faktiskt används i drift: **Okänt – fråga Jonas.**

---

## 11. Övrigt på ESP-unit V1.4

| Del | Koppling |
|-----|----------|
| D1 BLUE | `/WiFi_LED` = U2.IO47 (pin 24) → R1 (100 Ω) → D1 → GND |
| D2 RED | `/MQTT_LED` = U2.IO48 (pin 25) → R2 (100 Ω) → D2 → GND |
| D3 YELLOW | `/Hook_Status_LED` = U2.IO13 (pin 21) → R5 (100 Ω) → D3 → GND |
| D8 RED | `+5V` → R4 (1K) → D8 → GND |
| SW1 | `/Function_button` = U3.GPB1; andra polen GND |
| SW2 | `Net-(U2-IO0)` (U2.IO0, pin 27); andra polen GND |
| SW3 | Reset via R8 mot U2.EN, andra polen GND |
| J8 EX1–EX5 | pin 1 `/EX1` (U3.GPA6), 2 `/EX2` (GPA7), 3 `/EX3` (GPA4), 4 `/EX4` (GPA5), 5 `/EX5` (GPA3), 6 GND |
| J2 | pin 1 `/D_OUTL`, pin 2 `/D_OUTR`, pin 3–4 GND |
| J5 | pin 1 `/SDA`, 2 `/SCL`, 3 oansluten, 4 `+5V`, 5 `+3V3`, 6 GND |
| J6 | JTAG, avsnitt 4.5 |
| J4 | pin 1 `/SCL`, 2 GND, 3 `/SDA`, 4 GND, 5 `/MCP_SLIC_INT_2`, 6 GND, 7 `/MCP_SLIC_INT_1`, 8 GND, 9 och 10 `+5V`, 11 `Ain_4`, 12 `Ain_0`, 13 `Ain_5`, 14 `Ain_1`, 15 `Ain_6`, 16 `Ain_2`, 17 `Ain_7`, 18 `Ain_3`, 19 `Aout_4`, 20 `Aout_0`, 21 `Aout_5`, 22 `Aout_1`, 23 `Aout_6`, 24 `Aout_2`, 25 `Aout_7`, 26 `Aout_3` |
| TP1–TP4 | `+5V`, `+3V3`, GND, USB VBUS |

**ESP32-pinnar som används** (U2, nätlistan): IO0 boot/SW2, IO1 `/MOSI`, IO2 `/CS3`, IO4 `/CS2`, IO5 `/CS1`, IO9 `/SDA_3V`, IO10 `/SCL_3V`, IO11 `/MCP_SLIC_INT_1_3V`, IO12 `/SCLK`, IO13 `/Hook_Status_LED`, IO14 `/MCP_SLIC_INT_2_3V`, IO17 `/MCP_MT8816_INT_PIN`, IO18 `/MCP_MAIN_INT_PIN`, IO19 `/D-`, IO20 `/D+`, IO35 `/LRCK`, IO36 `/DIN`, IO37 `/BCK`, IO39 `/TCK`, IO40 `/TDO`, IO41 `/TDI`, IO42 `/TMS`, IO47 `/WiFi_LED`, IO48 `/MQTT_LED`. **Oanslutna:** IO3, IO6, IO7, IO8, IO15, IO16, IO21, IO38, IO45, IO46, RXD0, TXD0.

---

## 12. Kända avvikelser i schemat

Rättas inte här, bara flaggas.

1. **Annotationsvarning:** `kicad-cli` ger "schematic has annotation errors". Orsak: ESP-unit har 2 symboler `Q?` (BSS138) och 4 symboler `R?` (10K) utan löpnummer. De är verkliga delar i SLIC-avbrottens nivåväxling (avsnitt 4.4), ej ritfel. Nätlistan slår ihop dem; läs symbolerna ur `.kicad_sch` för att skilja dem åt.
2. **Y1 värde "3.579546 Hz"** – sannolikt skrivfel för MHz (en MT8870-kristall är normalt 3,579545 MHz; databladsuppgift utanför `KiCad/`).
3. **MCP-avbrott 5 V → 3,3 V:** U3/U4 (matade från `+5V`) går direkt till U2.IO18 / IO17 utan nivåväxlare (avsnitt 4.4). Observation.
4. **R19/R20** går från `+3V3` till 5 V-sidan av SLIC-avbrotten (inte till `..._3V`-näten), parallellt med pull-ups mot `+5V` (avsnitt 4.4). Avsikt: Okänt – fråga Jonas.
5. **PCM5102 CPGND (U10 pin 3):** nätet `Net-(U10-CPGND)` har bara C22 och U10.3 – pinnen är inte ansluten till GND i schemat. Kan vara ett fel eller avsiktligt; verifiera mot PCM5102A-databladet (utanför `KiCad/`).
6. **`GNDPWR`** på SLIC-unit är skilt från `GND` i schemat; D2–D17 (diodskydden) går mot `GND`, tyristorerna mot `GNDPWR` (avsnitt 3.2 och 9). Fysisk koppling: Okänt – fråga Jonas.
7. **Oanslutna pinnar:** ESP-unit: U2 IO3/6/7/8/15/16/21/38/45/46 + RXD0/TXD0, U3.GPB0, U4.GPA4–7 och GPB7, U5 X8–X11 och N.C., J5 pin 3, J6 pin 9, J7 SBU1/SBU2. SLIC-unit: J3 pin 1, KS0835F pin 8 (N.C.) på U2–U5.
8. **Stale nätklassmönster i `.kicad_pro` (ESP-unit):** mönster som `/MCP1_INT`, `/MCP2_INT`, `/MCP1_INT_3V`, `/MCP2_INT_3V`, `/Test_button`, `/LED_L0..L7`, `/Audio_SLIC` matchar inga nät i dagens schema (äldre namn). `/WiFi_LED` ligger i klassen `Audio` medan `/MQTT_LED` ligger i `Signal`. Nätklasser (ESP-unit): Default, Audio, Power, Signal (spårbredd 0,2 mm; Power 0,5 mm). SLIC-unit har även klassen Tip/Ring (0,3 mm) och Power med 0,4 mm clearance.
9. **Duplicerade/olika pull-ups:** SDA_3V/SCL_3V har två pull-ups var med olika värden (4,7K ‖ 10K ≈ 3,2 kΩ). Avsikt: Okänt.
10. **SLIC-unit saknar titelblock** i schemat (ingen revision/datum att läsa ut).

---

## 13. Så verifierar du mot schemat

Dokumentet kan ligga efter schemat – **vid konflikt gäller schemat.**

1. Exportera en nätlista (KiCad 9; ger varningen om annotationsfel för ESP-unit):

   ```
   "C:\Program Files\KiCad\9.0\bin\kicad-cli.exe" sch export netlist --format kicadsexpr -o ut.net "KiCad/Exchange ESP-unit S3 V1.4/Exchange ESP-unit S3.kicad_sch"
   ```

   Gör likadant för `KiCad/Exchange SLIC-unit V1.3/Exchange SLIC-unit.kicad_sch`. Checka inte in nätlistorna.

2. Slå upp ett nät eller en komponents pinnar med ett litet Python-skript (kör med `python -I`):

   ```python
   import re, sys
   def parse(t):
       tok = re.findall(r'"(?:\\.|[^"\\])*"|\(|\)|[^\s()]+', t); i = 0
       def rd():
           nonlocal i
           x = tok[i]; i += 1
           if x == '(':
               l = []
               while tok[i] != ')': l.append(rd())
               i += 1
               return l
           return x[1:-1] if x.startswith('"') else x
       return rd()
   def sub(l, n): return [x for x in l if isinstance(x, list) and x and x[0] == n]
   t = parse(open(sys.argv[1], encoding='utf8').read())
   for n in sub(sub(t, 'nets')[0], 'net'):
       name = sub(n, 'name')[0][1]
       nodes = [f"{sub(x,'ref')[0][1]}.{sub(x,'pin')[0][1]}({(sub(x,'pinfunction') or [[0,'']])[0][1]})" for x in sub(n, 'node')]
       if len(sys.argv) < 3 or sys.argv[2] in name or any(sys.argv[2] in s for s in nodes):
           print(name, '::', ' '.join(nodes))
   ```

   Exempel: `python -I nl.py ut.net DAC2` ger `/DAC2 :: U5.2(X14) U7.7(OUT)`.

3. För de oannoterade `Q?`/`R?` på ESP-unit: kopiera `.kicad_sch` till en temporär mapp, ge de sex symbolerna unika referenser (t.ex. Q91, Q92, R91–R94, både `(property "Reference" …)` och `(reference …)` under `(instances …)`) och exportera nätlistan från kopian. Originalet ska inte ändras.
