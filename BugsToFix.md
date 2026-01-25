# Buggar att fixa

## Webserver
- [ ] Knapparna är ibland för stora och ibland för små - behöver ses över
- [ ] Lägg till fler settings-inställningar

## SHKHandler

## ToneDialing

## ToneReader
- [ ] Allt för ofta reagerar inte koden vid en knapptryckning via DTMF, oavsett hur lång tid knappen trycks in.

## LineAction

## MCPDriver

## RingGenerator
- [ ] Störningar i SHK-linje vid ringsignal (workaround implementerad)
  - Kan inte lyfta luren exakt när en ringning togglar
  - Behöver eventuellt permanent lösning på sikt

## Settings
- [ ] Felmeddelande vid ändring av settings i WebUI
  ```
  [733579][E][Preferences.cpp:202] putUInt(): nvs_set_u32 fail: timerPulsDialing KEY_TOO_LONG
  [733587][E][Preferences. cpp:202] putUInt(): nvs_set_u32 fail: timerToneDialing KEY_TOO_LONG
  [733596][E][Preferences.cpp:202] putUInt(): nvs_set_u32 fail: timerDisconnected KEY_TOO_LONG
  ```

## Övrigt
- [ ] ESP Panikar när man:
  - Ringer 
  - Sker efter att ringsignalerna är klara och status ska återställas