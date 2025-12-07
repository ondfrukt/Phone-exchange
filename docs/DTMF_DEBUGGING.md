# DTMF Debugging Guide

## Overview

This document describes how to use the new debug levels for troubleshooting DTMF signal detection in the Phone Exchange system.

## Debug-nivåer

Systemet använder tre debug-nivåer som kan ställas in via `debugTRLevel` (ToneReader) och `debugMCPLevel` (MCPDriver) i Settings:

### Level 0 (Standard - Ingen debug)
Ingen debug-utskrift. Används för normal drift.

### Level 1 (Grundläggande debugging)
Visar viktiga händelser i DTMF-detekteringen:
- **STD interrupt detection**: När MT8870 STD-pinnen ändras
- **DTMF nibble läsning**: Q1-Q4 pin-tillstånd från MT8870
- **Dekodade värden**: Vilken siffra/tecken som detekterades
- **Linjetilldelning**: Vilken telefonlinje som tar emot signalen
- **Fel och varningar**: Problem med att läsa GPIO eller saknade linjer
- **Interrupt-räknare**: Rapporteras var 5:e sekund för MCP_MAIN

**Användning**: Grundläggande felsökning när DTMF inte fungerar alls.

```cpp
settings.debugTRLevel = 1;  // ToneReader debug
settings.debugMCPLevel = 1; // MCP interrupt debug
```

### Level 2 (Detaljerad debugging)
Inkluderar allt från Level 1 plus:
- **Alla interrupt-händelser**: Även de som inte är STD-relaterade
- **Edge detection**: Rising/falling edge detaljer
- **Råa GPIO-läsningar**: Binärt format av GPIOAB-register
- **Debounce-kontroller**: Information om dubblettfiltrering
- **INTF/INTCAP register**: Värden från MCP23017
- **Falling edge**: När STD går från HIGH till LOW

**Användning**: När signaler detekteras men inte tolkas korrekt.

```cpp
settings.debugTRLevel = 2;
settings.debugMCPLevel = 2;
```

### Level 3 (Omfattande debugging)
Inkluderar allt från Level 1 och 2 plus:
- **Periodisk STD pin-kontroll**: Visar STD-tillstånd varje sekund
- **Main loop heartbeat**: Visar att huvudloopen körs var 10:e sekund

**Användning**: När inga interrupts alls triggas eller för att bekräfta att systemet körs.

```cpp
settings.debugTRLevel = 3;
settings.debugMCPLevel = 2;
```

## Typiska felsökningsscenarier

### Scenario 1: Inga DTMF-signaler detekteras alls

**Steg 1**: Aktivera Level 1 debugging
```cpp
settings.debugTRLevel = 1;
settings.debugMCPLevel = 1;
```

**Vad man letar efter**:
- Visas "MCP_MAIN: Total interrupts fired" med ökande värde?
  - **JA**: ESP32 tar emot interrupts → Gå till Scenario 2
  - **NEJ**: Inga interrupts når ESP32 → Kontrollera hårdvara

**Hårdvarukontroller**:
1. Kontrollera att MCP_MAIN INT-pinnen (ESP32 pin 18) är korrekt ansluten
2. Kontrollera att MT8870 STD-pinnen är ansluten till MCP_MAIN pin 11 (GPB3)
3. Kontrollera att MT8870 får rätt spänning
4. Kontrollera att DTMF-signalen når MT8870:s input

### Scenario 2: Interrupts detekteras men inga DTMF-värden

**Steg 1**: Öka till Level 2 för att se interrupt-detaljer
```cpp
settings.debugTRLevel = 2;
settings.debugMCPLevel = 2;
```

**Vad man letar efter**:
- Visas "DTMF: STD interrupt detected"?
  - **JA**: STD-interrupts fungerar → Kontrollera GPIO-läsning
  - **NEJ**: Interrupts är från fel pin → Kontrollera konfiguration

**Om STD-interrupts fungerar**:
- Visas "DTMF: MT8870 pins - Q4=X Q3=X Q2=X Q1=X"?
  - **JA**: GPIO läses korrekt → Kontrollera nibble-tolkning
  - **NEJ**: GPIO-läsning misslyckas → Kontrollera I2C-buss

### Scenario 3: DTMF detekteras men fel värden

**Steg 1**: Använd Level 2 för att se rådata
```cpp
settings.debugTRLevel = 2;
```

**Vad man letar efter**:
- Jämför "Raw GPIOAB" med förväntade värden
- Kontrollera att Q1-Q4 mappningen är korrekt i config.h:
  ```cpp
  constexpr uint8_t Q1 = 15;  // LSB
  constexpr uint8_t Q2 = 14;
  constexpr uint8_t Q3 = 13;
  constexpr uint8_t Q4 = 12;  // MSB
  ```
- Verifiera `dtmf_map_` i ToneReader.h mot MT8870 datablad

### Scenario 4: Ibland fungerar det, ibland inte

**Steg 1**: Aktivera debounce-debugging
```cpp
settings.debugTRLevel = 2;
```

**Vad man letar efter**:
- "DTMF: Duplicate ignored (debouncing)" meddelanden
  - För många? → Öka `DTMF_DEBOUNCE_MS` (standard 150ms)
  - För få? → Minska `DTMF_DEBOUNCE_MS`
- Edge detection problem → Kontrollera lastStdLevel_ uppdatering

## Startup-meddelanden

Vid systemstart visas nu följande information:

```
MCP init:
 - MCP_MAIN   hittad/saknas
 - MCP_SLIC1  hittad/saknas
 - MCP_SLIC2  hittad/saknas
 - MCP_MT8816 hittad/saknas
MCP INT: Configuring MCP_MAIN interrupts...
MCP INT: Configuring FUNCTION_BUTTON on pin 9
MCP INT: Configuring MT8870 STD on pin 11 (compare-to-DEFVAL mode)
MCP INT: GPINTENA=0bXXXXXXXX GPINTENB=0bXXXXXXXX
MCP INT: INTCONA=0bXXXXXXXX INTCONB=0bXXXXXXXX
MCP INT: MCP_MAIN interrupt attached to ESP32 pin 18
MCP INT: MCP_SLIC1 interrupt attached to ESP32 pin 11
MCP INT: MCP_SLIC2 interrupt attached to ESP32 pin 14
MCP init: All interrupts configured successfully
```

Detta bekräftar att:
1. MCP-enheter detekteras korrekt
2. Interrupts är konfigurerade för rätt pins
3. ESP32 interrupt-pins är kopplade

## Exempel på normal DTMF-detektering (Level 1)

När allt fungerar korrekt ser utskriften ut så här när man trycker på siffra "5":

```
DTMF: STD interrupt detected - addr=0x27 pin=11 level=HIGH
DTMF: Rising edge detected - attempting to read DTMF nibble
DTMF: Successfully read nibble=0x5
DTMF: MT8870 pins - Q4=0 Q3=1 Q2=0 Q1=1 => nibble=0b0101 (0x5)
DTMF: DECODED - nibble=0x5 => char='5'
DTMF: Added to line 0 digit='5' dialedDigits="5"
```

## Tips för effektiv debugging

1. **Börja med låg nivå**: Starta alltid med Level 1 och öka vid behov
2. **En sak i taget**: Aktivera bara en debug-kategori åt gången
3. **Använd Serial Monitor**: 115200 baud (se platformio.ini)
4. **Dokumentera**: Anteckna vad som fungerar/inte fungerar på olika nivåer
5. **Kontrollera hårdvaran först**: Innan du skriver kod, verifiera anslutningar

## Prestandapåverkan

- **Level 1**: Minimal påverkan, kan användas kontinuerligt
- **Level 2**: Måttlig påverkan, mycket text skrivs ut
- **Level 3**: Hög påverkan, kan sakta ner systemet märkbart

## Relaterade filer

- `src/services/ToneReader.cpp` - DTMF-detektering
- `src/drivers/MCPDriver.cpp` - MCP23017 interrupt-hantering
- `include/config.h` - Pin-mappningar
- `src/settings/settings.h` - Debug-nivå inställningar
