# Buggar att fixa

- **Event message queue overflow**
    - Får många felmeddelanden: `[2055072][E][AsyncEventSource.cpp:237] _queueMessage(): Event message queue overflow: discard message`
    - Justera köns storlek för att avhjälpa problemet
    - Skicka även bara event om det finns klienter anslutna

- **UI-problem med knappar**
    - Knapparna är ibland för stora och ibland för små
    - Alla knappar utgår från samma mall i CSS-filen
    - **Möjlig lösning:** Dela upp i flera CSS-varianter

- **Statusändringar vid ringning**
    - När en ringsignal genereras till linje 0 får jag även in singaler in i min SHK-pin på samma linje. testat och samma beteende sker på både linje 0 och 1.
    - kanske kan det delade jordplanen spela roll men samtidigt har jag bara problemet på den linje som jag ringer till enskilt.

- **~~Krachar när webserverDebug ändras~~ FIXED**
    - ~~När debugnivån på webserver ändras krachar espn och startar om~~
    - Fixed: Removed infinite recursion in console sink callback that was triggered when debugWSLevel >= 2

- **Settings ändringar för ringsignaler ändras inte när man trycker på knappen**
    - man får bara följande felmeddelanden:
[733579][E][Preferences.cpp:202] putUInt(): nvs_set_u32 fail: timerPulsDialing KEY_TOO_LONG
[733587][E][Preferences.cpp:202] putUInt(): nvs_set_u32 fail: timerToneDialing KEY_TOO_LONG
[733596][E][Preferences.cpp:202] putUInt(): nvs_set_u32 fail: timerDisconnected KEY_TOO_LONG
