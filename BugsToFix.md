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
    - Lösning: Kontextmedveten filtrering med olika tröskelvärden beroende på om linjen ringer
      - Normal drift: `hookStableMs = 50ms`, `hookStableConsec = 10` läsningar
      - Under ringning: `hookStableMsDuringRing = 150ms` (6× FR toggle-period på 25ms)
    - SHKService detekterar nu om en linje aktivt ringer via RingGenerator och använder strängare filtrering
    - Filtrerar brus från FR-pin toggling samtidigt som äkta hook-lyft detekteras inom ~150ms under ringning