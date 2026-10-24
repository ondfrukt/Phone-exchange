# CODEMAP — var ligger vad?

Kodkarta för snabb navigering: *fil + funktion*, inte hur det fungerar. Inga radnummer (funktionsnamn är stabilare).
Uppdatera kartan när filer/funktioner flyttas eller nya tjänster/endpoints läggs till. Koden är sanningen vid konflikt.

Allt körs kooperativt i `App::update()` (`src/app/App.cpp`) i denna ordning: `InterruptManager::collectInterrupts` → `WifiClient::loop` → `Provisioning::loop` → `WebServer::update` → `MqttClient::loop` → `LineAction::update` → `SHKService::update` → `ToneReader::update` → `RingGenerator::update` → `ToneGenerator::update` → `Functions::update`. Objekt skapas och kopplas i `App`-konstruktorn, uppstart i `App::begin()`.

## 1. Uppgift → fil/funktion

| Jag vill… | Titta i |
|-----------|---------|
| Ny inställning | `src/settings/settings.h` (fält, `kVersion`) → `Settings.cpp` (`resetDefaults`, `load`, `save`) → `src/net/WebServer.cpp` (`/api/settings/*` GET/POST) → `data/settings.html` + `data/settings.js` |
| Ny REST-endpoint | `WebServer::setupApiRoutes_` i `src/net/WebServer.cpp` |
| Nytt SSE-event | `WebServer::initSse_`, `sendFullStatusSse`, `sendDebugSse`, `sendToneGeneratorSse`, `sendActiveMaskSse`; klienten i `data/app.js` |
| Ny MQTT-topic / payload | `MqttClient::publishLineStatus`, `publishFullSnapshot`, `makeTopic_`; JSON i `src/util/StatusSerializer.cpp` |
| Nytt statusbeteende (vad som händer vid status X) | `LineAction::action` (switch per status); övergångar enligt sektion 2 |
| Ny `LineStatus` / `ToneId` / `RingState` | `src/model/Types.h`, sedan `LineAction::action` och `StatusSerializer` |
| Ändra tidsgränser per status (ready, dialing, ringing …) | `timer_*` i `settings.h`; startas av `LineManager::setLineTimer`; utlöses i `LineAction::timerExpiredCheck` → `timerExpired` |
| Ändra pulsvalstider / hook-filter | `SHKService` (`updateHookFilter_`, `updatePulseDetector_`, `pulseFalling_`, `pulseRising_`, `emitDigitAndReset_`); fält `hookStableMs`, `pulseLowMaxMs`, `globalPulseTimeoutMs` m.fl. i `settings.h` |
| Ändra DTMF-läsning | `ToneReader::update`, `readDtmfNibble`, `decodeDtmf`; `dtmf*`-fält i `settings.h` |
| Ändra ringsignal (längd, paus, iterationer) | `RingGenerator::generateRingSignal`, `update`, `stopRingingLine`; `ring*`-fält i `settings.h` |
| Ny ton / ändra tonföljd (ring-, upptagetton …) | `ToneGenerator::getSequence_`, `startTone`; styrs från `LineAction::startToneGenForStatus`; AD9833 i `AD9833Driver` |
| Koppla ihop/koppla bort linjer eller ljudkälla | `ConnectionHandler::connectLines`, `disconnectLines`, `connectAudioToLine`; matrisen i `MT8816Driver` |
| Ny pinne / I2C-adress / MCP-pinne | `include/config.h` (`cfg::ESP_PINS`, `cfg::mcp`, `cfg::mt8816`, `cfg::TMUX4051`); elektrik i `KiCad/ELEKTRONIK.md` |
| Läsa/skriva MCP23017 | `src/drivers/MCPDriver.*`; avbrott via `InterruptManager` |
| Linjedata (nummer, namn, aktiv, timers) | `LineHandler` (state per linje), `LineManager` (`setPhoneNumber`, `setLineName`, `searchPhoneNumber`, `syncLineActive`) |
| Aktiva linjer | `Settings.activeLinesMask`; `WebServer::setLineActiveBit_`, `toggleLineActiveBit_`; `Settings::adjustActiveLines` |
| Funktionsknapp, status-LED:ar, omstart, testring | `src/util/Functions.cpp` (`update`, `updateStatusLeds_`, `restartDevice`, `testRing`) |
| WiFi, uppkoppling, hostname, tid | `src/net/WifiClient.cpp` (`begin`, `loop`, `connect_`, `syncTime_`) |
| Provisioning (AP/captive portal), factory reset | `src/net/Provisioning.cpp` (`begin`, `loop`, `factoryReset`) |
| OTA | `ArduinoOTA` i `WifiClient::startOta_` (`handle()` i `WifiClient::loop`); upload via `pio run -e devkitc_ota` (se CLAUDE.md) |
| Webb-UI | `data/` (`index.html`+`app.js`, `settings.html`+`settings.js`, `style.css`); kräver `uploadfs` |
| Debugloggning / konsol i UI | `settings.debugXxLevel`; `src/util/UIConsole.*` (skickas som SSE `console`) |

## 2. Statusövergångar: status ← vem sätter den

Alla `LineManager::setStatus` (`src/services/LineManager.cpp`). Anrop utanför `LineAction` är markerade fetstil.

| Status | Sätts av |
|--------|----------|
| Ready | `LineAction::hookStatusCangeCheck` — lur av på Idle-linje |
| Connected | `hookStatusCangeCheck` — lur av på Incoming-linje (båda parter) |
| Idle | `hookStatusCangeCheck` — lur på (alla fall, även `outgoingTo` vid Ringing); `LineAction::timerExpired` (Incoming → Idle); **`WebServer.cpp` `/api/ring/stop`** |
| Disconnected | `hookStatusCangeCheck` (motparten när Connected läggs på); `timerExpired` (Ringing → Disconnected) |
| **PulseDialing** | **`SHKService::pulseRising_` (Ready → PulseDialing)** |
| **ToneDialing** | **`ToneReader::update` (första siffran)** |
| Ringing | `timerExpired` (vid dialtimeout när nummer hittats och linjen är ledig) |
| Incoming | `timerExpired` (på den anropade linjen, samtidigt som Ringing); **`WebServer.cpp` `/api/ring/test`** |
| Busy | `timerExpired` (anropad linje ej Idle) |
| Fail | `timerExpired` (egen nummer, inaktiv linje eller okänt nummer) |
| Timeout | `timerExpired` (från Ready, Busy, Fail, Disconnected) |
| Abandoned | `timerExpired` (från Timeout) |

- Hook-flaggan sätts i `SHKService::setStableHook` → `lineManager_.lineHookChangeFlag`. `HookStatus::Off` = lur av (lyft lur), `On` = lur på.
- Flaggor/ordning: `LineAction::update` kör `hookStatusCangeCheck` → `statusChangeCheck` (anropar `action`) → `timerExpiredCheck`. Flaggor: `lineHookChangeFlag`, `lineStatusChangeFlag`, `activeTimersMask` (`LineManager.h`).
- Raden `//lineManager_.setStatus(...)` i `SHKService::setStableHook` är bortkommenterad och räknas inte.

## 3. Datavägar

- **Avbrott in:** MCP23017 → `InterruptManager::collectInterrupts` (kö) → `pollEvent` / `pollEventByAddress`. Konsumenter: `SHKService::update` (SLIC1/SLIC2), `ToneReader` (STD), `Functions::update` (funktionsknapp).
- **Status ut:** `LineManager::setStatus` → callbacks (`addStatusChangedCallback`) → `MqttClient` (`begin`) och `WebServer::setupLineManagerCallback_` (SSE `lineStatus`). `LineAction::action` publicerar dessutom direkt via `MqttClient::publishLineStatus`.
- **SSE-event** (`WebServer.cpp`): `console`, `lineStatus`, `debug`, `toneGen`, `activeMask`, samt ett namnlöst fullstatus-event (`sendFullStatusSse`).
- **REST → inställningar:** `WebServer::setupApiRoutes_` → `Settings` (fält) → `Settings::save` → NVS (`Preferences`); inläsning vid start i `Settings::load`.
- **Reaktion på status:** `LineAction::action` → toner (`startToneGenForStatus`, `turnOffToneGenIfUsed`), ring (`RingGenerator`), koppling (`ConnectionHandler` → `MT8816Driver`), MQTT.

## 4. Fallgropar

- **SHK under ringning:** filter och tvingad omläsning efter ringburst i `SHKService::update` (kollar `RingGenerator::lineStates_`, `RingState::RingToggling`/`RingPause`). Läs README *Challenges* före ändring av `SHKService` eller `RingGenerator`.
- **Inget får blockera:** inga `delay()`/långa loopar, allt körs i `App::update()`.
- **`AudioPlayer`** (`src/services/AudioPlayer.*`) är inte kopplad — instansieras inte i `App`.
- **Filnamn:** `settings.h` (gemener) men `Settings.cpp` (versal).
- **Stavning i kod:** `hookStatusCangeCheck`, `timer_incomming` är felstavade men korrekta namn — grep:a med dem.
- **Linjeindex** är 0–7; aktiva linjer styrs av `Settings.activeLinesMask`.
- **`src/ota/`** är tom; OTA sker via `ArduinoOTA` i `WifiClient`.

Se även: `docs/StatusflowChart.drawio.svg` (statusflöde), `KiCad/ELEKTRONIK.md` (elektronik), `README.md` *Challenges*.
