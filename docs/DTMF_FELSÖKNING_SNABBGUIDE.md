# Snabbguide: Felsökning av DTMF-signaler

## Problem: Inga DTMF-signaler detekteras

### Steg-för-steg felsökning

#### Steg 1: Aktivera grundläggande debugging
I `src/app/App.cpp` i `begin()` funktionen, lägg till efter `settings.resetDefaults();`:

```cpp
settings.debugTRLevel = 1;   // ToneReader debug nivå 1
settings.debugMCPLevel = 1;  // MCP interrupt debug nivå 1
```

#### Steg 2: Öppna Serial Monitor
- Anslut USB-kabel till ESP32
- Öppna Serial Monitor i PlatformIO (baudrate: 115200)
- Tryck på Reset-knappen på ESP32

#### Steg 3: Kontrollera startup-meddelanden
Leta efter:
```
MCP init:
 - MCP_MAIN   hittad
MCP INT: MCP_MAIN interrupt attached to ESP32 pin 18
```

Om "MCP_MAIN saknas" visas → Kontrollera I2C-anslutningar

#### Steg 4: Tryck på en DTMF-knapp
Lyssna på DTMF-ton i telefonen och kolla Serial Monitor.

**Om du ser**:
```
MCP_MAIN: Total interrupts fired: X  (där X ökar)
```
✅ **Bra!** Interrupts fungerar → Gå till Steg 5

**Om inget händer**:
❌ Inga interrupts når ESP32:
1. Kontrollera att MT8870 STD är ansluten till MCP_MAIN pin 11 (GPB3)
2. Kontrollera att MCP_MAIN INT är ansluten till ESP32 pin 18
3. Kontrollera att MT8870 får rätt spänning (5V)
4. Kontrollera att DTMF-ljud når MT8870 input

#### Steg 5: Detaljerad interrupt-debugging
Om interrupts triggas men inga siffror detekteras, öka debug-nivån:

```cpp
settings.debugTRLevel = 2;   // Mer detaljerad output
settings.debugMCPLevel = 2;
```

Nu ska du se:
```
DTMF: STD interrupt detected - addr=0x27 pin=11 level=HIGH
DTMF: Rising edge detected
DTMF: MT8870 pins - Q4=X Q3=X Q2=X Q1=X => nibble=0xX
DTMF: DECODED - nibble=0xX => char='X'
```

**Kontrollera**:
- Kommer interrupt från rätt pin (11)?
- Är Q1-Q4 värdena rimliga? (0-15)
- Dekoderas rätt tecken?

#### Steg 6: GPIO-läsningsproblem
Om inga Q1-Q4 värden visas eller de är alltid 0, kontrollera:

```cpp
settings.debugTRLevel = 2;
```

Leta efter:
```
DTMF: Raw GPIOAB=0bXXXXXXXXXXXXXXXX
```

- Ändras dessa värden när du trycker DTMF-knappar?
  - **NEJ** → MT8870 Q1-Q4 är inte korrekt anslutna till MCP_MAIN
  - **JA** → Kontrollera pin-mappning i config.h

## Debug-nivåer snabbreferens

| Nivå | Användning | Output |
|------|-----------|---------|
| 0 | Normal drift | Ingen debug |
| 1 | Grundläggande | Interrupts, decoded digits, fel |
| 2 | Detaljerad | GPIO states, edges, debounce |
| 3 | Full | Allt + periodiska kontroller |

## Pin-mappning (från config.h)

```
MT8870 → MCP_MAIN:
  STD (pin 15) → GPB3 (pin 11)
  Q1  (pin 11) → GPB7 (pin 15) - LSB
  Q2  (pin 10) → GPB6 (pin 14)
  Q3  (pin 9)  → GPB5 (pin 13)
  Q4  (pin 8)  → GPB4 (pin 12) - MSB

MCP_MAIN → ESP32:
  INT → GPIO 18
```

## Vanliga problem och lösningar

### Problem: "Failed to read GPIO from MCP_MAIN"
**Lösning**: 
- Kontrollera I2C-bussen (SDA/SCL)
- Kontrollera MCP_MAIN adress (0x27)
- Lägg till fördröjning mellan läsningar

### Problem: "No lastLineReady available"
**Lösning**:
- Telefonen måste vara off-hook först
- Linjen måste vara i "ready" state
- Kontrollera LineManager status

### Problem: Fel siffror detekteras
**Lösning**:
- Kontrollera Q1-Q4 anslutningar
- Verifiera dtmf_map_ i ToneReader.h mot MT8870 datablad
- Kontrollera för korskopplingar i ledningar

### Problem: Siffror detekteras flera gånger
**Lösning**:
- Öka DTMF_DEBOUNCE_MS (standard: 150ms)
- Leta efter "Duplicate ignored" meddelanden
- Justera värdet tills dubbletter försvinner

### Problem: Vissa knapptryckningar misslyckas (intermittent)
**Symptom**: Ibland detekteras ingen siffra när man trycker på en knapp, speciellt vid snabba tryckningar.

**Förklaring**: Systemet har automatisk recovery-mekanism för att hantera snabba knapptryckningar där STD-signalen går LOW→HIGH→LOW så snabbt att rising edge interrupt förloras.

**Debug-output vid recovery (Level 1+)**:
```
DTMF: RECOVERY - Detected LOW but never saw HIGH transition, attempting to read nibble
```
eller
```
DTMF: RECOVERY - Rising edge was detected but not processed, attempting read on falling edge
```

**Åtgärd**:
- Om recovery-meddelanden visas och siffror detekteras korrekt → Allt fungerar som det ska
- Om siffror fortfarande missas:
  1. Kontrollera MT8870 hold-time
  2. Verifiera I2C-bussens hastighet
  3. Kontrollera för EMI/störningar

## Mer information

Se [DTMF_DEBUGGING.md](DTMF_DEBUGGING.md) för fullständig dokumentation.
