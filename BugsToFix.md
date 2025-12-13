# Buggar att fixa

- **Event message queue overflow**
    - Får många felmeddelanden: `[2055072][E][AsyncEventSource.cpp:237] _queueMessage(): Event message queue overflow: discard message`
    - Justera köns storlek för att avhjälpa problemet
    - Skicka även bara event om det finns klienter anslutna

- **UI-problem med knappar**
    - Knapparna är ibland för stora och ibland för små
    - Alla knappar utgår från samma mall i CSS-filen
    - **Möjlig lösning:** Dela upp i flera CSS-varianter


- **Statusändringar vid ringning** ✅ FIXED
    - När en ringsignal genereras uppstår falska SHK-pinändringar på grund av elektriskt brus från FR-pin toggling
    - Lösning: Ökad hookStableMs från 50ms till 100ms och hookStableConsec från 2 till 25 läsningar
    - Detta kräver att SHK-signalen är stabil i både tid (100ms) och antal på varandra följande läsningar (25 × 2ms = 50ms) innan en hook-statusändring accepteras
    - Filtrerar brus från FR-pin toggling (25ms period) samtidigt som äkta hook-lyft detekteras inom ~100ms