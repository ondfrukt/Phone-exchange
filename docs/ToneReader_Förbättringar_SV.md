# Förbättringar av ToneReader-klassen

## Sammanfattning
ToneReader-klassen har förbättrats för att bättre filtrera bort skräpsignaler och brus som tidigare felaktigt detekterades som DTMF-toner. Tre nya konfigurerbara parametrar har lagts till i Settings-systemet för att möjliggöra finjustering av tondetekteringen.

## Nya konfigurationsparametrar

### 1. `dtmfDebounceMs` (Standard: 150ms)
**Syfte:** Förhindrar dubbeldetektering av samma siffra inom en kort tidsperiod.

**Hur det fungerar:** Efter att en siffra har detekterats och bearbetats kommer samma siffra att ignoreras om den detekteras igen inom detta tidsfönster.

**När du ska justera:**
- **Öka** om du får dubbla siffror (t.ex. trycker "5" en gång men får "55")
- **Minska** om legitima snabba sekvenser av samma siffra missas

**Rekommenderat område:** 100-300ms

---

### 2. `dtmfMinToneDurationMs` (Standard: 40ms)
**Syfte:** Filtrerar bort mycket korta elektriska störningar eller transienter som triggar MT8870 men inte är giltiga DTMF-toner.

**Hur det fungerar:** Systemet mäter hur länge STD-signalen (Steering Data) från MT8870 förblir hög. Om den går låg innan denna minimala varaktighet uppnås, avvisas tonen som ogiltig.

**När du ska justera:**
- **Öka** om du får slumpmässiga sifferdetekteringar från elektriskt brus eller störningar
- **Minska** om legitima mycket snabba knapptryckningar inte detekteras (ovanligt)

**Rekommenderat område:** 30-60ms

**Obs:** MT8870-databladet specificerar 40ms som typisk minimal tonvaraktighet för tillförlitlig DTMF-detektering.

---

### 3. `dtmfStdStableMs` (Standard: 5ms)
**Syfte:** Säkerställer att STD-signalen har stabiliserats innan DTMF-koden läses från Q1-Q4-pinnarna.

**Hur det fungerar:** När STD-signalen övergår från LÅG till HÖG (stigande flank), väntar systemet denna varaktighet innan det läser 4-bitars koden. Detta förhindrar att instabila värden läses under signalens stigtid.

**När du ska justera:**
- **Öka** om du får felaktiga siffror detekterade (t.ex. trycker "5" men får "8" eller andra fel siffror)
- **Minska** om legitima toner missas (osannolikt med standardvärdet)

**Rekommenderat område:** 3-10ms

---

## Hur filtreringen fungerar tillsammans

ToneReader implementerar nu en trestegs filtreringsprocess:

### Steg 1: Detektering av stigande flank
När STD-signalen går HÖG, markerar systemet det som en "väntande" ton men bearbetar den inte omedelbart.

### Steg 2: Stabilitetskontroll (`dtmfStdStableMs`)
Efter den konfigurerade stabilitetsperioden, om STD fortfarande är HÖG, läser systemet Q1-Q4-pinnarna och avkodar DTMF-siffran.

### Steg 3: Validering av varaktighet och debounce
- När STD går LÅG, beräknas den totala tonvaraktigheten
- Om varaktighet < `dtmfMinToneDurationMs`, avvisas tonen
- Om samma siffra detekterades < `dtmfDebounceMs` sedan, avvisas den som en dubblett
- Annars accepteras siffran och läggs till de uppringda siffrorna

## Felsökning av vanliga problem

### Problem: Får slumpmässiga siffror när ingen telefon används
**Lösning:** Öka `dtmfMinToneDurationMs` till 50-60ms för att bättre filtrera elektriskt brus.

### Problem: Att trycka på en siffra en gång resulterar i flera detekteringar
**Lösning:** Öka `dtmfDebounceMs` till 200-250ms.

### Problem: Får ibland fel siffror (t.ex. trycker 5 ger 7)
**Lösning:** Öka `dtmfStdStableMs` till 7-10ms för att ge mer tid för MT8870-utgången att stabiliseras.

### Problem: Snabb uppringning registrerar inte alla siffror
**Lösning:** Minska `dtmfDebounceMs` till 100-120ms (men inte för lågt eller du får dubbletter).

## Konfigurering via webbgränssnittet

De nya inställningarna kan justeras genom telefonväxelns webbgränssnitt:
1. Navigera till http://phoneexchange.local/
2. Gå till Settings-sektionen
3. Leta efter "DTMF/ToneReader-inställningar"
4. Justera värdena efter behov
5. Spara ändringar

## Konfigurering programmatiskt

Inställningarna är tillgängliga via Settings singleton:

```cpp
Settings& settings = Settings::instance();

// Justera DTMF-inställningar
settings.dtmfDebounceMs = 200;        // 200ms debounce
settings.dtmfMinToneDurationMs = 45;  // 45ms minimal ton
settings.dtmfStdStableMs = 7;         // 7ms stabilitetskontroll

// Spara till icke-flyktigt minne
settings.save();
```

## Debug-loggning

För att se detaljerad information om tondetektering och filtrering, öka ToneReaders debug-nivå:

```cpp
settings.debugTRLevel = 2;  // Maximal verbositet
```

Debug-nivåer:
- **0**: Ingen debug-utmatning
- **1**: Grundläggande händelser (flanker, accepterade/avvisade toner, varningar)
- **2**: Detaljerad debugging (STD-signaldetaljer, debounce-kontroller, råa GPIO-värden)

## Teknisk bakgrund

MT8870 är ett DTMF-avkodningschip som:
- Tar emot ljudsignaler från telefonlinjer
- Detekterar DTMF (tonvals) frekvenser
- Ger ut en 4-bitars kod på Q1-Q4-pinnar som representerar siffran
- Höjer STD-pinnen HÖG när en giltig ton detekteras

Förbättringarna implementerar korrekt signalkonditionering och validering för att säkerställa att endast äkta DTMF-toner från telefonknappsatser registreras, medan elektriskt brus, överhörning och andra skräpsignaler som kan trigga MT8870 avvisas.

## Vad har ändrats i koden

**I settings.h:**
- Lagt till tre nya konfigurerbara parametrar för DTMF-detektering

**I Settings.cpp:**
- Lagt till standardvärden för de nya parametrarna
- Lagt till laddning och sparning av parametrarna till NVS (non-volatile storage)

**I ToneReader.h:**
- Tagit bort hårdkodad `DTMF_DEBOUNCE_MS`-konstant
- Lagt till tillståndsvariabler för STD-signalstabilitetsspårning

**I ToneReader.cpp:**
- Implementerat fördröjd bearbetning av stigande flanker
- Lagt till kontroll av minimal tonvaraktighet
- Använder nu konfigurerbara parametrar från Settings istället för hårdkodade värden
- Förbättrad loggning för debugging

Dessa förbättringar bör avsevärt minska falska DTMF-detekteringar från brus och skräpsignaler samtidigt som korrekt detektering av legitima toner bibehålls.
