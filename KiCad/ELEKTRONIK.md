# ELEKTRONIK – dokumentation av kretskorten

Underlag för framtida Claude-sessioner. **Källor:** `KiCad/` (schemana, plus `.kicad_pro`/`.kicad_pcb` för några kortfakta) för koppling, och databladen i `docs/datasheets/` för komponentbeteende (se avsnitt 14 för vilka). Firmware, `README.md` och `docs/` i övrigt har inte använts för faktapåståenden, med ett undantag: avsnitt 4.4 och 12 punkt 3 nämner hur firmware konfigurerar MCP-avbrotten, tydligt märkt.

**Vid konflikt mellan detta dokument och schemat gäller schemat för koppling; databladet gäller för komponentbeteende.** Se avsnitt 13 för hur man slår upp fakta direkt ur schemat.

Konventioner i dokumentet:

- Referensbeteckningar, pinnar och nätnamn är exakt som i schemat (`U5.X15`, `/DAC1`). `ref.pinne` är en komponents pinne enligt nätlistan; pinnummer anges där de behövs.
- Nätnamn med `/` framför är lokala nätetiketter i schemat. `{slash}` i SLIC-unit-nätnamn är KiCads kodning av `/` (nätet `/Ain_0{slash}4` heter `Ain_0/4` i schemat).
- Det som inte går att belägga ur `KiCad/` eller databladen står som **"Okänt – fråga Jonas"**.
- "Härlett" = slutsats dragen ur kopplingen, inte något som står i schemat eller i ett datablad.
- **"Enligt datablad:"** = faktum från ett datablad i `docs/datasheets/`, med filnamn och **PDF-sida** (sidan i PDF-läsaren; för alla datablad utom MT8870D är den lika med sidtrycket – MT8870D trycker "4-12" ff. på PDF-sida 2 ff.). Bara det som står efter "Enligt datablad:" kommer från databladet; resten är schemafakta.
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
| Y1 | Crystal, värde "3.579546 Hz" | Kristall för U9 OSC1/OSC2. Värdesträngen är ett skrivfel; rätt värde är 3,579545 MHz (avsnitt 8 och 12) |
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

Enligt datablad: `PCM5101APWR.PDF` är databladet för hela familjen PCM5100A/PCM5101A/PCM5102A (s. 1–2, tabell 1 på s. 2: skillnaden är bara dynamikområde/SNR/THD), så det gäller U10 (PCM5102APW). Stiftkopplingen ovan är den databladet föreskriver: pinnummer enligt tabell 2 (s. 3); SCK på GND väljer internt PLL-läge, där PCM5102A skapar sin systemklocka ur BCK (s. 12 och typkretsen figur 41, s. 27); FMT låg = I2S, FLT låg = normal filterlatens, DEMP låg = de-emphasis av, XSMT hög = ej muteat (tabell 2, s. 3; XSMT-timing s. 22). XSMT permanent på `+3V3` betyder att DAC:en aldrig mutas via den pinnen. Logiknivåer: ingångarna är 3,3 V-kompatibla (VIH min 0,7·DVDD, s. 4), alltså direkt OK från ESP32 på 3,3 V.

### 4.4 Avbrottsnät (ESP-unit V1.4 och SLIC-unit V1.3)

- **MCP-kretsarnas INTA och INTB är sammankopplade till ett nät per krets** (båda pinnarna på samma nät):
  - U3 INTA (pin 20) + INTB (pin 19) → `/MCP_MAIN_INT_PIN` → U2.IO18 (pin 11); R21 (10K) mot `+3V3`.
  - U4 INTA + INTB → `/MCP_MT8816_INT_PIN` → U2.IO17 (pin 10); inget motstånd.
  - SLIC-unit U1 INTA (pin 20) + INTB (pin 19) → `/MCP_SLIC_INT_1/2` → JP17 / JP18 (avsnitt 10).
- **Observation (nivå):** U3 och U4 matas med `+5V` men deras avbrottsnät går direkt till 3,3 V-ESP:ns IO17/IO18 utan nivåväxlare. Om detta är ett problem beror på MCP:ns utgångskonfiguration, som inte syns i schemat; bedömning nedan och i avsnitt 12 punkt 3.
- **Enligt datablad (`MCP23017.pdf`):** INTA/INTB kan konfigureras som aktiv-hög, aktiv-låg eller öppen drain (tabell 1-1, s. 4; IOCON-bitarna ODR/INTPOL, s. 17–18). **Efter strömpåslag (IOCON = 0x00) är INT en push-pull-utgång, aktiv låg**, dvs. den driver `VDD` (här 5 V) som vilonivå (VOH min VDD−0,7 V vid 3 mA, s. 28). Med `IOCON.MIRROR = 0` är INTA och INTB oberoende utgångar (s. 17–18); att koppla ihop dem (som här) är bara tillåtet om båda alltid har samma nivå, dvs. `MIRROR = 1` (utgångarna OR:as internt, avsnitt 1.7.1.1) **eller** `ODR = 1` (öppen drain – då kan inget driva mot varandra). Annars kan en push-pull-utgång som är hög och en som är låg stå mot varandra. Det gäller U3, U4 och SLIC-unit U1.
- **Enligt firmware (inte schema/datablad, kan ändras):** `src/drivers/MCPDriver.cpp` skriver `IOCON = 0x44` (MIRROR=1, ODR=1, aktiv låg, öppen drain) för U3 (MAIN) och SLIC-korten, och `0x4A` för U4 (MT8816): MIRROR=1, INTPOL=1 (aktiv hög), **ODR=0 = push-pull**. Konsekvens, härlett ur databladet ovan: U3:s avbrottsnät är öppen drain mot R21 → `+3V3`, så högnivån är 3,3 V (säkert för IO18). U4 är push-pull aktiv hög och driver 5 V på IO17 när ett avbrott är aktivt – men U4:s alla använda pinnar är utgångar och databladet säger att utgångspinnar inte kan ge avbrott (avsnitt 1.7.4); firmware aktiverar heller inga avbrott på U4 (`GPINTEN` sätts bara för MAIN och SLIC). U4:s INT ligger alltså i vila på låg nivå. Mellan strömpåslag och att firmware skrivit IOCON är dock både U3 och U4 push-pull aktiv låg, dvs. 5 V kan ligga på IO18/IO17 en kort stund vid uppstart.
- **Gräns på ESP32-sidan, enligt datablad (`ESP32-S3-WROOM-1.PDF`):** VIH max = VDD+0,3 V (tabell 10, s. 14), dvs. 3,6 V vid 3,3 V matning; modulens datablad anger ingen egen absolut-maxgräns för IO-pinnar (tabell 8, s. 14, gäller bara matningen: max 3,6 V). 5 V på en IO-pinne ligger alltså utanför det databladet tillåter.
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

**Enligt datablad (`MCP23017.pdf`):** 7-bitars I2C-adress = `0100 A2 A1 A0`, dvs. 0x20 + (A2·4 + A1·2 + A0) (avsnitt 1.4, figur 1-2, s. 8). Adresspinnarna och ~RESET ska vara externt biaserade (tabell 1-1, s. 4) – uppfyllt här genom pull-ups/GND-koppling. Pinnummer (PDIP/SOIC/SSOP-kolumnen, tabell 1-1, s. 4; samma på alla tre kretsar): GPB0–7 = 1–8, SCL = 12, SDA = 13, A0/A1/A2 = 15/16/17, ~RESET = 18, INTB = 19, INTA = 20, GPA0–7 = 21–28 – allt stämmer med schemat. Adresserna nedan är alltså härledda ur kopplingen av A0–A2 enligt den formeln; att de är de som faktiskt svarar på bussen är inte kontrollerat (kräver hårdvarutest).

Avbrottsutgångarnas natur (push-pull/öppen drain, hopkoppling av INTA/INTB): se avsnitt 4.4.

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
- Alla 16 pinnar används. Ut/in enligt KS0835F-symbolens pinntyper (SHK = utgång på KS0835F ⇒ ingång på MCP; RM, FR, ~PD = ingångar på KS0835F ⇒ utgångar på MCP). **Enligt datablad (`KS0835F.pdf`, pinntabell s. 6) stämmer riktningarna:** F/R, RM och PD är ingångar på SLIC:en, SHK är dess utgång (hög = telefonen är avlyft). SHK ska avstuderas (debounce) 10 ms i styrprocessorn (s. 8). PD: låg nivå stänger av DC/DC-omvandlaren och databladet säger "DO NOT put logic (H) on this pin" (s. 6) – därför ligger dioderna D22–D25 (K mot MCP) i serie med ~PD: MCP-utgång låg drar ~PD låg via dioden, MCP-utgång hög spärras av dioden och ~PD blir flytande i stället för att drivas hög (typkretsen s. 7 har samma 1N4148/BAS16-diod). Att ~PD då är aktivt (ej avstängd) när det flyter är härlett ur typkretsen, inte uttryckt i texten.

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

**Enligt datablad (`MT8816AF1.pdf`, TQFP-44-kolumnen i pinntabellen s. 3–4 och pinfiguren s. 2):** alla pinnummer ovan stämmer (X0–X5 = 31–26, X6–X11 = 4–9, X12/X13 = 25/24, X14/X15 = 2/3, Y0–Y7 = 32/35/37/39/17/15/13/10, AX0–AX3 = 44/18/19/43, AY0–AY2 = 20/21/40, RESET 42, DATA 36, STROBE 14, CS 34, VDD 38, VSS 12, VEE 16, N.C. 1/11/22/23/33/41). Matrisen är 16 rader (X) × 8 kolumner (Y) (s. 5), så "X = Aout/DAC, Y = Ain" i tabellerna ovan är en korrekt läsning av chippets uppbyggnad. Skrivning (s. 5): ställ in adress (AX/AY) och DATA, sätt CS hög, pulsa STROBE hög→låg; DATA 1 = sluten, 0 = öppen; DATA måste vara stabil på STROBE:s fallande flank; RESET hög öppnar alla kontakter oberoende av CS och måste vara låg vid skrivning. Alla styrsignaler är aktivt höga. Matning: VDD 4,5–13,2 V (s. 7) – 5 V är inom området. VSS är digital jordreferens och VEE den negativa analoga matningen: **analogsignaler ska ligga mellan VEE och VDD** (s. 5 och 7; absolut max VDD+0,3 V). Här är VEE = VSS = GND, dvs. analogt område 0–5 V; se avsnitt 12 punkt 11. Digitala ingångsnivåer (VIL/VIH, s. 7): har inte bedömts – tabellen är otydlig och styrsignalerna kommer från MCP23017 på 5 V, inte direkt från ESP32.

---

## 7. Toner och ljud-in (ESP-unit V1.4)

- **AD9833 ×3** (U6, U7, U8; moduler med pinnarna 1 VCC, 2 DGND, 3 SDATA, 4 SCLK, 5 FSYNC, 6 AGND, 7 OUT): VCC = `+3V3`, DGND/AGND = GND, SPI-nät enligt avsnitt 4.2. **Enligt datablad (`AD9833SRMZ-EP-RL7.PDF`) – gäller chippet, inte modulens kortlayout:** AD9833 är ett 10-poligt MSOP-chip med pinnarna 1 COMP, 2 VDD, 3 CAP/2.5V, 4 DGND, 5 MCLK, 6 SDATA, 7 SCLK, 8 FSYNC, 9 AGND, 10 VOUT (s. 6). Modulens 7-poliga pinordning (1–7 ovan) går därför **inte** att belägga ur databladet – den följer inte chippets pinnummer; MCLK-källan (klockkrets på modulen) och COMP/CAP-kondensatorerna ligger på modulen. **Modulens pinordning och klocka: Okänt – fråga Jonas.** Det databladet kan bekräfta: signalnamnen och deras funktion (SDATA = seriell data, SCLK = klockan, data klockas in på fallande SCLK-flank, FSYNC aktivt låg = ny 16-bitars ord; s. 6), matning 2,3–5,5 V (s. 3, alltså `+3V3` OK), logikingångar VIH min 2,0 V vid 2,7–3,6 V matning (s. 3, alltså direkt OK från ESP32 på 3,3 V), och att FSYNC är aktivt låg stämmer med pull-ups R41–R43 mot `+3V3` (inaktiv i vila). VOUT har max 0,65 V och min 38 mV (s. 3); härlett: utsignalen ligger då över 0 V och passar MT8816:s analoga område (avsnitt 6). Utgångar: U6.OUT → `/DAC1` → U5.X15; U7.OUT → `/DAC2` → U5.X14; U8.OUT → `/DAC3` → U5.X13. Chip select: U6 `/CS1`, U7 `/CS2`, U8 `/CS3`.
- **PCM5102APW** (U10): I2S enligt 4.3. `OUTL` (pin 6) → `/D_OUTL` → U5.X12, C23 (2,2 nF → GND) och J2 pin 1. `OUTR` (pin 7) → `/D_OUTR` → C24 (2,2 nF → GND) och J2 pin 2. Stödkomponenter: C21 (CAPP–CAPM), C22 (VNEG–CPGND), C25 + C26 på LDOO (pin 18). **Enligt datablad (`PCM5101APWR.PDF`, typkrets figur 41, s. 27):** flygkondensatorn (2,2 µF mellan CAPP–CAPM) och VNEG-kondensatorn (2,2 µF mellan VNEG och **GND** via CPGND) stämmer med schemats C21/C22 till värde och plats. Figur 41 visar dessutom 0,1 µF på CPVDD, DVDD och LDOO samt 10 µF + 0,1 µF på AVDD; någon jämförelse av dessa avkopplingar mot schemat (C25/C26 på LDOO m.fl.) är inte gjord. **CPGND (pin 3, "Charge pump ground", tabell 2 s. 3) ska ligga på GND** – i schemat gör den inte det, se avsnitt 12 punkt 5. Utgången är ground-centrerad (±, 2,1 Vrms, s. 1–2), dvs. har en negativ halva – se avsnitt 12 punkt 11. Databladets rekommenderade utgångsfilter (figur 43, s. 28) är 470 Ω i serie + 2,2 nF mot GND per kanal; schemat har 2,2 nF (C23/C24) men **ingen serieresistor** på `/D_OUTL`/`/D_OUTR` enligt nätlistan (inga andra komponenter än C23, J2, U5.X12 på `/D_OUTL`). Det är en avvikelse från rekommendationen, inte nödvändigtvis ett fel.

---

## 8. DTMF-mottagning (ESP-unit V1.4)

- **TMUX4051 (U11):** S0–S7 = `/Aout_0..7` (pin 13, 14, 15, 12, 1, 5, 2, 4); adress A0/A1/A2 (pin 11/10/9) ← U3 GPA0/GPA1/GPA2 (`/TM_A0..2`); D (pin 3) → `/DTMF`; EN (pin 6), VSS (7), GND (8) = GND; VDD (16) = `+5V`. TMUX är alltså en "lyssna på linje n"-väljare som ligger parallellt med X0–X7 på matrisen.
  - **Enligt datablad (`TMUX4051.pdf`):** alla pinnummer ovan stämmer (S0 13, S1 14, S2 15, S3 12, S4 1, S5 5, S6 2, S7 4, D 3, EN 6, VSS 7, GND 8, A2/A1/A0 = 9/10/11, VDD 16; tabell 5-1, s. 4). **EN är aktivt låg** ("När EN är hög är alla switchar av", s. 4) – EN på GND betyder alltså att multiplexern alltid är på. Adressering (tabell 8-1, s. 23): A2A1A0 = 000 → S0, 001 → S1, … 111 → S7, så `/TM_A0..2` = linjenumret 0–7 direkt. Matning: VDD−VSS ska vara 5–24 V (s. 8), alltså är 5 V (VSS = GND) precis lägsta tillåtna spänning; VSS är signalgolvet. Signalen på S/D ska ligga mellan VSS och VDD (s. 8); här är VSS = GND (0–5 V), se avsnitt 12 punkt 11. Adress-/EN-ingångar: VIH min 1,35 V, VIL max 0,8 V (s. 10) – MCP23017 på 5 V klarar det.
- **MT8870 (U9)** ingångskrets: `/DTMF` → C14 (0,1 µF) → `Net-(C14-Pad1)` → R30 (100K) → `Net-(U9-IN-)` (IN−, pin 2); R32 (100K) mellan IN− och GS (pin 3); IN+ (pin 1) kopplad till VREF (pin 4) via `Net-(U9-IN+)`.
- **Övrigt U9:** EST (16) ← R31 (390K) → ST/GT (17) ← C15 (0,1 µF) mot `+5V`; OSC1 (7) / OSC2 (8) ← Y1; TOE (10) = `+5V`; INH (5) = GND; VDD (18) = `+5V`; VSS (9) = GND; PWDN (6) ← U3.GPB2.
- **Utgångar:** Q1 (11) → U3.GPB7, Q2 (12) → GPB6, Q3 (13) → GPB5, Q4 (14) → GPB4, STD (15) → U3.GPB3.
- **Y1:** värde i schemat "3.579546 Hz" (skrivfel, se avsnitt 12), pin 1 → OSC1, pin 2 → OSC2.
- **Enligt datablad (`MT8870D.pdf`, 18-poligt SOIC = MT8870DS):** pinnummer stämmer med schemat (IN+ 1, IN− 2, GS 3, VRef 4, INH 5, PWDN 6, OSC1 7, OSC2 8, VSS 9, TOE 10, Q1–Q4 11–14, StD 15, ESt 16, St/GT 17, VDD 18; pinfigur och pinntabell s. 2). Kristall: **3,579545 MHz** (s. 2 och 5; typkrets figur 10, s. 10: "X-tal = 3.579545 MHz ±0,1 %"; tillåten klockfrekvens 3,5759–3,5831 MHz, s. 10) – schemats symbolvärde "3.579546 Hz" ska alltså läsas som 3,579545 MHz (komponentens egen artikelnummerlänk i schemat, Raltron AS-3.579545-18, pekar på samma värde). Singel-ändad ingång (figur 10, s. 10): signal → C1 (100 nF) → R1 (100 k) → IN−, R2 (100 k) mellan IN− och GS (förstärkning 1), IN+ till VRef – samma som schemats C14, R30, R32 och IN+–VRef. Steering (figur 10): R3 mellan ESt och St/GT, C2 (100 nF) från St/GT till VDD – samma uppbyggnad som R31/C15, men **R31 är 390 k, typkretsen har 300 k** (±1 %); ett större R ger längre skyddstid (tGTP/tGTA = R·C-baserad, s. 3–4), så detta är en avvikelse i värde, inte i koppling. Funktion: PWDN hög = avstängd (intern pull-down), INH hög blockerar A–D (intern pull-down; INH på GND = A–D tillåtna), TOE hög = utgångarna Q1–Q4 aktiva (intern pull-up; TOE på `+5V` är alltså alltid aktiv) (s. 2, tabell 1 s. 4). StD går hög när en ton registrerats; Q1–Q4 är giltiga då.

---

## 9. SLIC-unit per linje (V1.3)

**KS0835F-pinnar** (U2–U5): 1 RING, 2 TIP, 3 F/R (in), 4 RM (in), 5 SHK (ut), 6 Audio_in (in), 7 Audio_out (ut), 8 N.C. (oansluten), 9 GND, 10 +VDC (`+5V`), 11 ~PD (in).

**Enligt datablad (`KS0835F.pdf`):** pinfunktion och riktning enligt pinntabellen (s. 6) – 1 RING, 2 TIP, 3 F/R (in), 4 RM (in), 5 SHK (ut), 6 VR (= schemats Audio_in, "Audio In Negative": analog insignal från codec som kommer ut på Tip/Ring), 7 VX (= Audio_out, analog signal från Tip/Ring till codec), 8 NC ("Do not connect"), 9 GND, 10 +VDC (3,3–5 V; matning 3,2–5,5 V, s. 2 och 4), 11 PD (låg = avstängd) – **stämmer helt med schemat**. Databladet kallar pin 11 "PD" (aktivt låg avstängning), schemat ~PD. Ringning (s. 7): sätt RM hög, växla F/R med önskad frekvens och kadens; databladet anger 20–25 Hz och en 75 V-batterinivå under ringning, ringsignalen är 130 Vp-p; exempelkadensen i figuren är 1 s ring / 4 s paus. F/R-polaritet: pinntabellen (s. 6) säger att hög F/R *vänder* polariteten, avsnittet "Polarity Reversal" (s. 8) säger tvärtom ("F/R hög = Forward") – **databladet är internt motsägande**, så logiknivåns betydelse för enbart polaritet är oviss; för ringning spelar det ingen roll eftersom pinnen växlas. SHK-signalen ska debounce:as 10 ms (s. 8). Under ringning känner SLIC:en av slingströmmen (ring-trip) och visar avlyft på SHK (s. 8).

**Typkretsen (s. 7) jämfört med schemat:** typkretsen har en TVS (P6KE82/P6KE75, D1), en diodbrygga eller 1N4004-dioder (D2), PTC-säkringar i serie på båda ledarna (JK250-120U, valfria), 1 MΩ "extern DC-biasresistor" (R1) tvärs över linjen, tyristorskydd P3100SC (D6–D8, valfria) och ett 1N4148/BAS16 på PD, 0,1 µF på Audio_in/Audio_out, samt 0,1 µF + 1000 µF/10 V på matningen. Schemat har **samma byggstenar** (polyfuse F, 1 MΩ R6–R9, tyristorer RV, 0,1 µF C3–C10, diod i PD) men: (a) dioderna D2–D17 (M4) är kopplade på ett eget sätt på RING-noden (två mot GND, två mot en TVS) i stället för en brygga över både TIP och RING; (b) TVS:en är **P6KE51A** i stället för databladets P6KE82/P6KE75 – 51 V är lägre än batterinivån 75 V under ringning (databladet s. 3, 7), men eftersom TVS:en ligger bakom dioderna är det inte klart om detta ger problem – **kan ej avgöras utan TVS-datablad (inte i `docs/datasheets/`) och mätning, Okänt – fråga Jonas**; (c) de polariserade kondensatorerna på `+5V` är 10 µF (C15–C18, avsnitt 2.2), inte databladets 1000 µF/10 V per SLIC – databladets typkrets visar 1000 µF/10 V, och matningsströmmen per linje är hög: typiskt 40 mA (5 V, på-lur), **270 mA (5 V, avlyft)** och 180 mA under ringning, vid 3,3 V 50/320/220 mA (s. 4) – fyra linjer avlyfta ger alltså ca 1 A från `+5V` via J2 pin 9/10. Om 10 µF räcker som lokal buffert är inte bedömt; flaggas som observation i avsnitt 12 punkt 12.

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
2. **Y1 värde "3.579546 Hz"** – **utfall: skrivfel, bekräftat mot datablad.** Rätt värde är 3,579545 MHz: MT8870D-databladet anger 3,579545 MHz ±0,1 % (`MT8870D.pdf` s. 2, 5 och figur 10 s. 10), och komponentens egen artikelnummerlänk i schemat (Raltron AS-3.579545-18) har samma värde. Förslag (görs inte här): rätta symbolvärdet i schemat.
3. **MCP-avbrott 5 V → 3,3 V:** U3/U4 (matade från `+5V`) går direkt till U2.IO18 / IO17 utan nivåväxlare (avsnitt 4.4). **Utfall: ofarligt i nuvarande firmware under drift, men beror på programvara och har ett uppstartsfönster.**
   - Enligt datablad är INT efter strömpåslag push-pull aktiv låg och driver då ~5 V (`MCP23017.pdf` s. 4, 17–18, 28), vilket överskrider ESP32:s VIH max VDD+0,3 V = 3,6 V (`ESP32-S3-WROOM-1.PDF` tabell 10, s. 14).
   - Firmware (`src/drivers/MCPDriver.cpp`) sätter U3 till öppen drain mot R21 → `+3V3` (högnivå 3,3 V, säkert), och U4 till push-pull aktiv hög utan att några avbrott är aktiverade (vila = låg, 0 V). Under drift kommer alltså aldrig 5 V till IO17/IO18.
   - **Kvarvarande risk:** (a) mellan strömpåslag och att firmware skrivit IOCON ligger U3/U4:s INT push-pull aktiv låg = ~5 V på IO18/IO17; (b) om någon ändrar firmware så att U4:s avbrott aktiveras får IO17 5 V när det är aktivt; (c) en MCP som tappar sin konfiguration (t.ex. brownout/reset på bara MCP) faller tillbaka till push-pull. Hur länge/ofta (a) inträffar och om ESP32-pinnen tål det: kan ej avgöras ur databladen (ingen absolut-maxgräns för IO-pinnar i modulens datablad). Tillräckligt för en egen issue om Jonas vill ha det robust (nivåväxlare eller serieresistor).
   - **Hopkoppling av INTA/INTB:** tillåten endast om `MIRROR=1` eller `ODR=1` (avsnitt 4.4); firmware sätter båda för U3/SLIC och `MIRROR=1` för U4. Utan det skulle utgångarna kunna stå mot varandra.
4. **R19/R20** går från `+3V3` till 5 V-sidan av SLIC-avbrotten (inte till `..._3V`-näten), parallellt med pull-ups mot `+5V` (avsnitt 4.4). **Utfall: kan ej avgöras (avsikt okänd), men ofarligt som kopplat.** Vilonivån på 5 V-sidan blir en spänningsdelning mellan två 10 k-pull-ups, ca 4,15 V (härlett). SLIC-kortets INT-utgång är enligt firmware öppen drain, så den drar bara ner nätet, och 4,15 V ligger under MCP23017:s 5 V-matning (absolut max VDD+0,6 V, `MCP23017.pdf` s. 27). Nätet går bara till BSS138-nivåväxlarens drain (källan ligger på 3,3 V-sidan med egen pull-up, gate = `+3V3`, så transistorn är avstängd i vila – härlett ur kopplingen, BSS138-datablad finns inte i `docs/datasheets/`). Effekten av R19/R20 är alltså bara en något lägre vilonivå och lite extra ström när MCP drar ner nätet. Avsikten med att pull-upen går till `+3V3` på 5 V-sidan förblir **Okänt – fråga Jonas**.
5. **PCM5102 CPGND (U10 pin 3):** nätet `Net-(U10-CPGND)` har bara C22 och U10.3 – pinnen är inte ansluten till GND i schemat (kontrollerat mot ny nätlista) och inte heller i PCB-filen (pad 3 ligger på nätet `Net-(U10-CPGND)`, alla andra GND-pinnar på `GND`). **Utfall: fel mot datablad.** PCM510xA-databladet beskriver pin 3 som "Charge pump ground" (`PCM5101APWR.PDF` tabell 2, s. 3) och typkretsen (figur 41, s. 27) visar CPGND ihopkopplad med GND, med VNEG-kondensatorn (2,2 µF) mellan VNEG och GND. Med CPGND flytande saknar den inbyggda laddningspumpen sin jordreferens, och den negativa matningen VNEG (ska vara −3,3 V) blir osäker – ljudutgången kan bli förvrängd, tyst eller instabil. (Hur chippet beter sig i detalj är inte beskrivet i databladet; slutsatsen att kopplingen är fel bygger på att databladets typkrets och pinnbeskrivning kräver GND.) Rättning kräver ändring av schema och PCB och ligger utanför den här issuen.
6. **`GNDPWR`** på SLIC-unit är skilt från `GND` i schemat; D2–D17 (diodskydden) går mot `GND`, tyristorerna mot `GNDPWR` (avsnitt 3.2 och 9). Fysisk koppling: Okänt – fråga Jonas.
7. **Oanslutna pinnar:** ESP-unit: U2 IO3/6/7/8/15/16/21/38/45/46 + RXD0/TXD0, U3.GPB0, U4.GPA4–7 och GPB7, U5 X8–X11 och N.C., J5 pin 3, J6 pin 9, J7 SBU1/SBU2. SLIC-unit: J3 pin 1, KS0835F pin 8 (N.C.) på U2–U5. **Utfall för strapping-pinnarna (`ESP32-S3-WROOM-1.PDF` s. 11–13):** modulen har fyra strapping-pinnar: IO0, IO3, IO45 och IO46. Av de oanslutna är IO3, IO45 och IO46 strapping.
   - **IO0** är ansluten (R7 pull-up + SW2) och därmed definierad: hög = SPI-boot, SW2 nedtryckt = download-läge (tabell 5, s. 12). Inte oansluten.
   - **IO45** (väljer VDD_SPI-spänning): irrelevant för den här modulen. N16R2 har ESP32-S3R2 som är fabriksinställd till 3,3 V (tabell 6, s. 13) – strapping-värdet används då inte. **Ofarligt.**
   - **IO46** (boot-läge/ROM-logg): har intern pull-down och får sitta oansluten (s. 11); kombinationen IO46 = 1 och IO0 = 0 är ogiltig (tabell 5 not 1, s. 12) – IO46 flyter låg, så den uppstår inte. **Ofarligt.**
   - **IO3** (JTAG-källa och ROM-meddelanden på UART0): har **ingen** intern pull – databladet säger att den är flytande som standard och att dess värde "styrs av extern krets som inte får vara högimpedant" (s. 11). Effekten är begränsad: JTAG-källan bestäms bara av IO3 när eFuse `EFUSE_STRAP_JTAG_SEL` är satt (tabell 4–5, s. 12); utan det används USB Serial/JTAG-kontrollern oavsett (J7 på IO19/IO20). **Kan ej avgöras ur databladet** om eFuse-värdet är satt på just den här modulen; flaggas som en lös ände, men sannolikt ofarlig för normal drift. Förslag: om J6 (JTAG på IO39–42) ska användas på det sättet, dra IO3 till en definierad nivå.
   - IO35–IO37 (I2S) är fria på N16R2: bara moduler med OSPI-PSRAM (R8) använder dem internt (fotnot b, s. 11); N16R2 har quad-PSRAM (R2). **Stämmer.**
8. **Stale nätklassmönster i `.kicad_pro` (ESP-unit):** mönster som `/MCP1_INT`, `/MCP2_INT`, `/MCP1_INT_3V`, `/MCP2_INT_3V`, `/Test_button`, `/LED_L0..L7`, `/Audio_SLIC` matchar inga nät i dagens schema (äldre namn). `/WiFi_LED` ligger i klassen `Audio` medan `/MQTT_LED` ligger i `Signal`. Nätklasser (ESP-unit): Default, Audio, Power, Signal (spårbredd 0,2 mm; Power 0,5 mm). SLIC-unit har även klassen Tip/Ring (0,3 mm) och Power med 0,4 mm clearance.
9. **Duplicerade/olika pull-ups:** SDA_3V/SCL_3V har två pull-ups var med olika värden (4,7K ‖ 10K ≈ 3,2 kΩ). Avsikt: Okänt.
10. **SLIC-unit saknar titelblock** i schemat (ingen revision/datum att läsa ut).
11. **Analogt signalområde i MT8816 (U5) och TMUX4051 (U11), upptäckt vid databladsgranskningen – utfall: misstänkt problem, kan ej avgöras utan mätning.** Enligt datablad ska analogsignaler ligga mellan VEE och VDD (`MT8816AF1.pdf` s. 5, 7; absolut max VDD+0,3 V) respektive VSS och VDD (`TMUX4051.pdf` s. 8). I schemat är VEE/VSS = GND och VDD = `+5V`, dvs. 0–5 V, och ingen DC-bias syns i nätlistan på `/Aout_n`, `/Ain_n`, `/D_OUTL` eller `/DAC1–3`. Linjeljudet går via 0,1 µF-kondensatorer (SLIC-unit C3–C10, ger AC-koppling utan bias) och PCM5102A ger en ground-centrerad utsignal ±2,1 Vrms, direkt på U5.X12 (`PCM5101APWR.PDF` s. 1–2). En signal som svänger under 0 V går då utanför det databladen anger. Vad KS0835F:s VX-utgång har för DC-nivå, och hur U5:s ingångsskydd reagerar i praktiken, står inte i databladen (**Okänt**). AD9833-utgången (0–0,65 V) är inte ett problem. Mät `Aout_n` och `/D_OUTL` med oscilloskop innan slutsats dras.
12. **KS0835F-matning och TVS, observationer från databladsjämförelsen** (`KS0835F.pdf` s. 3–4, 7): (a) typkretsen har 1000 µF/10 V på +VDC per SLIC, schemat 10 µF-kondensatorer (C15–C18) – och strömmen är 270 mA/linje avlyft; (b) TVS P6KE51A (D18–D21) mot databladets P6KE82/75 och 75 V batterinivå under ringning – hur dioderna D2–D17 ändrar bilden kan inte avgöras utan TVS-datablad. Dessa är observationer; om de är fel i praktiken avgörs av test (ringning på flera linjer, spänningsfall på `+5V`).

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

---

## 14. Datablad som använts

Alla i `docs/datasheets/`. "Enligt datablad:" i dokumentet syftar på dessa; sidnummer är PDF-sidor (avsnitt "Konventioner" överst). Kontrollen gjordes mot nätlistor exporterade med `kicad-cli` (avsnitt 13) och mot `.kicad_pcb` för CPGND.

| Fil | Komponent i schemat | Täcker rätt variant? | Använd för |
|-----|---------------------|----------------------|------------|
| `MCP23017.pdf` | U3, U4 (ESP-unit), U1 (SLIC-unit) | Ja (MCP23017/MCP23S17, DS21952B) | Pinnummer, I2C-adress, INT-utgångar (avsnitt 4.4, 5, 12) |
| `KS0835F.pdf` | U2–U5 (SLIC-unit) | Ja (KS0835F, V1.3) | Pinfunktion/riktning, typkrets, ringning (avsnitt 5.3, 9, 12) |
| `PCM5101APWR.PDF` | U10 (PCM5102APW) | Ja – databladet SLAS859A gäller PCM5100A/5101A/5102A | Pinnar, CPGND, typkrets, utgångsfilter (avsnitt 4.3, 7, 12) |
| `MT8816AF1.pdf` | U5 | Ja (MT8816AF1 = 44-poligt TQFP) | Pinnar, skrivprotokoll, matning/analogt område (avsnitt 6, 12) |
| `MT8870D.pdf` | U9 | Ja (MT8870D, 18-poligt SOIC) | Pinnar, kristall, typkrets (avsnitt 8, 12) |
| `TMUX4051.pdf` | U11 | Ja | Pinnar, EN-polaritet, adressering (avsnitt 8) |
| `AD9833SRMZ-EP-RL7.PDF` | U6–U8 (AD9833-moduler) | **Bara chippet**, inte modulen | Funktion/nivåer; modulens pinordning är **Okänd** (avsnitt 7) |
| `ESP32-S3-WROOM-1.PDF` | U2 (N16R2) | Ja (N16R2 finns i ordertabellen) | Modulpinnar, strapping, VIH (avsnitt 4.4, 11, 12) |

Inget datablad finns för LM1117, BSS138, PMEG4050EP, TVS-dioderna, P3100SC, MF-R025 och 1N4148WS; fakta om dem är oförändrade (schema eller härlett).

**ESP32-pinnarna i avsnitt 4 och 11** (IO-nummer ↔ modulpinne) kontrollerades mot `ESP32-S3-WROOM-1.PDF` tabell 3 (s. 10–11) och stämmer: IO0 27, IO1 39, IO2 38, IO4 4, IO5 5, IO9 17, IO10 18, IO11 19, IO12 20, IO13 21, IO14 22, IO17 10, IO18 11, IO19 13, IO20 14, IO35 28, IO36 29, IO37 30, IO47 24, IO48 25. (IO39–IO42 = pin 32–35 anges inte i avsnitten ovan.)
