# Buggar att fixa

- **Event message queue overflow**
    - Får många felmeddelanden: `[2055072][E][AsyncEventSource.cpp:237] _queueMessage(): Event message queue overflow: discard message`
    - Justera köns storlek för att avhjälpa problemet
    - Skicka även bara event om det finns klienter anslutna

- **Hantering av interrupts från MCP**
    - Problem med flera funktioner på samma MCP
    - `handleMainInterrupt()` anropas av lyssnande funktioner som tömmer hela cashen av loggad info
    - Funktioner "nedströms" får inte den info de behöver
    - **Lösning:** Skapa en ny central funktion `HandleInterrupts()` som:
        - Tar över hantering av interrupts
        - Hanterar händelser på MCP
        - Kontaktar berörda funktioner som påverkas av förändringen

- **UI-problem med knappar**
    - Knapparna är ibland för stora och ibland för små
    - Alla knappar utgår från samma mall i CSS-filen
    - **Möjlig lösning:** Dela upp i flera CSS-varianter
