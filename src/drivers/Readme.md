I drivers läggs funktioner som startar upp saker och ting. Exemelvis definitioner av pins (om de ska vara output/inputs) uppstart av MCP-enheter. All hårdvaru init-funktiner dvs


Så här blir flödet för att läsa av HOOKPINS
1. Hook ändras (du lyfter eller lägger på luren).
2. MCP skickar en signal på INT-pinnen → “något har hänt”.
3. Din ESP32 ser detta och sätter en flagga (t.ex. intFlag = true;).
(Viktigt: själva avbrottet ska bara sätta en flagga, inget mer.)
4. I din vanliga loop() kollar du om intFlag är satt.
- Om ja: läs av läget på alla hook-pinnar och spara det.
5. Kör debounce:
- Om en pin ändrades: vänta t.ex. 30 ms.
- Är den fortfarande i samma läge efter 30 ms? → Då bestäm att den verkligen ändrats.