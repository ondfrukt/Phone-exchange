# CLAUDE.md

Lokal telefonväxel för gamla fasta telefoner (pulsval och DTMF) byggd på ESP32-S3. All logik och switching sker i växeln, telefonerna är omodifierade. Hobby-/lärprojekt — ägaren (Jonas) lär sig programmering och elektronik genom projektet, så förklara *varför* när du föreslår något icke-uppenbart.

## Arbetssätt: issues först

Arbete med Claude planeras som **GitHub-issues** innan något implementeras.

- Nya idéer, buggar och förändringar → använd skillet `/issue` för att analysera och skriva en issue. Implementera inte direkt om Jonas inte uttryckligen ber om det.
- Varje issue-utkast granskas av agenten `issue-reviewer` (`.claude/agents/`) och måste vara GODKÄNT av den innan det visas för Jonas och publiceras.
- När du ombeds jobba på en issue (`#N`): läs hela issuen inklusive kommentarer, följ planen och acceptanskriterierna. Om koden har ändrats så att planen inte längre stämmer — stanna och säg det i stället för att improvisera.
- Fyll aldrig i luckor i en issue med antaganden om hårdvara (pinnar, adresser, timing, spänningar). Fråga.

## Färdigrapportering av en implementerad issue

När du är klar med en issue, avsluta med en rapport som innehåller:

1. **Gjort** — kort, per planens steg; avvikelser från planen och varför.
2. **Verifierat** — vad som är byggverifierat, och vad som återstår för Jonas att testa på hårdvaran (med stegen från issuens *Verifiering*).
3. **Granskning av issuen** — hämtat från issuens sektion *Granskningslogg*:
   - antal iterationer tills granskaren var nöjd
   - vad granskaren hittade, kort i punktform (blockerande fynd och vad som gjordes)
   - om något avgjordes av Jonas i stället för av granskaren
   - saknas loggen: skriv det, i stället för att anta något

Syftet är att Jonas ska kunna se om issue-granskaren är lagom — varken överdriven eller för slapp. Lägg därför till en egen mening: **hade granskarens fynd verkligen betydelse för implementationen?** Vilka punkter hjälpte, och märkte du brister i issuen under implementationen som granskaren missade?

## Git

- `main` = stabil, `develop` = integrationsgren. PR:er går feature-gren → `develop` → `main`.
- Gren för en issue: `issue-<N>-<kort-slug>` från `develop`. PR mot `develop`, skriv `Closes #N` i beskrivningen.
- Committa/pusha bara när Jonas ber om det.
- `gh` (GitHub CLI) används för issues/PR. Repo: `ondfrukt/Phone-exchange`.

## Bygga och ladda upp

PlatformIO, Arduino-ramverket, `gnu++17`. `pio` ligger inte i PATH — använd full sökväg:

```
"$USERPROFILE/.platformio/penv/Scripts/pio.exe" run                      # bygg (env devkitc)
"$USERPROFILE/.platformio/penv/Scripts/pio.exe" run -e wroom             # bygg för wroom-kortet
"$USERPROFILE/.platformio/penv/Scripts/pio.exe" run -e devkitc_ota -t upload    # OTA firmware
"$USERPROFILE/.platformio/penv/Scripts/pio.exe" run -e devkitc_ota -t uploadfs  # OTA webb-UI (data/)
```

- Det finns inga automatiska tester. Verifiering = att det bygger + manuell test på hårdvaran, som bara Jonas kan göra. Säg tydligt vad som är byggverifierat och vad som kräver hårdvarutest.
- Ladda inte upp till enheten (USB eller OTA) utan att Jonas ber om det.
- Ändringar i `data/` kräver `uploadfs`, inte bara firmware-upload.

## Arkitektur

`main.cpp` gör bara `app.begin()` / `app.loop()`. `src/app/App.{h,cpp}` äger alla objekt och kopplar ihop dem via konstruktor-injektion (referenser). Ingen RTOS-tråd per tjänst — allt körs kooperativt i `App::update()` i en fast ordning. **Inget får blockera** (inga `delay()`, inga långa loopar); använd `millis()`-baserade tillstånd.

| Mapp | Innehåll |
|------|----------|
| `include/config.h` | Pinnar, I2C-adresser, MCP23017-pinmappning (`cfg::`) |
| `src/drivers/` | Hårdvarunära: MCP23017, MT8816 (kopplingsmatris), AD9833 (tongenerator), PCM5102 (I2S-ljud), DTMF-mux, `InterruptManager` |
| `src/services/` | Telefonilogik: `LineHandler` (en linjes tillstånd), `LineManager` (äger 8 linjer, status-callbacks), `LineAction` (reagerar på statusbyten), `SHKService` (lur av/på + pulsval), `ToneReader` (MT8870 DTMF), `RingGenerator`, `ToneGenerator`, `ConnectionHandler`, `AudioPlayer` |
| `src/net/` | WiFi, provisioning, async webbserver, MQTT |
| `src/settings/` | `Settings`-singleton, persisteras i NVS (`Preferences`) |
| `src/model/Types.h` | `LineStatus`, `HookStatus`, `ToneId`, `RingState` m.fl. |
| `data/` | Webb-UI (LittleFS): `index.html`, `settings.html`, `app.js`, `settings.js`, `style.css` |
| `docs/` | Datablad-bilder, drawio-diagram (blockschema, statusflöde, SHK-logik) |
| `KiCad/` | Kretskort (ESP-unit, SLIC-unit) |

Kärnflöde: hårdvaruavbrott från MCP23017 → `InterruptManager` köar → `SHKService`/`ToneReader` tolkar → `LineManager::setStatus()` → callback → `LineAction` styr toner, ring, kopplingsmatris och MQTT.

## Konventioner och fallgropar

- **Ny inställning** berör alltid hela kedjan: `settings.h` (fält) → `Settings.cpp` (default, `load()`, `save()`, ev. bumpa `kVersion`) → `WebServer.cpp` (JSON ut + parameter in med intervallkontroll) → `data/settings.html` + `data/settings.js`.
- Debugloggning styrs per tjänst av `settings.debugXxLevel` (0/1/2). Följ samma mönster; logga via `Serial` och/eller `util::UIConsole`.
- Linjeindex 0–7. Aktiva linjer styrs av `Settings.activeLinesMask`.
- SHK-störningar under ringning är ett känt problem med en medveten workaround — läs avsnittet *Challenges* i [README.md](README.md) innan du ändrar `SHKService` eller `RingGenerator`.
- Elektroniken (kort, pinnar, MCP-tabeller, MT8816-matris, SLIC, jumprar) är dokumenterad i [KiCad/ELEKTRONIK.md](KiCad/ELEKTRONIK.md) — läs den före ändringar som rör pinnar, MCP, matris, SLIC, toner eller kretskort. Vid konflikt gäller schemat för elektrik.
- README:er under `src/` och `docs/` kan vara inaktuella (t.ex. defaultvärden). Koden är sanningen.
- Kommentarer i koden blandar svenska och engelska; följ den omgivande filens stil.
