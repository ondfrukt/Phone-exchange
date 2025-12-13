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
    - När en ringsignal genereras till linje 0. 