---
name: issue
description: Analysera en idé, bugg eller förändring i telefonväxel-repot och skriv en GitHub-issue som planerar en AI-insats — med kodanalys, plan, acceptanskriterier och testplan — så att en senare Claude-session kan implementera den utan att gissa. Använd när Jonas vill skapa en issue, planera en ändring, eller beskriver något som ska göras ("vi borde…", "det blir fel när…", "lägg till…").
---

# Skapa en AI-planerad issue

Målet är en issue som är **självbärande**: en Claude-session som bara läser issuen och repot ska kunna genomföra arbetet utan att gissa. All analys görs nu, inte vid implementationen. Du ändrar **ingen kod** i detta skill.

## 1. Förstå uppdraget

- Formulera om önskemålet med egna ord i 1–2 meningar och kontrollera att det stämmer.
- Klassa typ: `bug`, `enhancement` (ny funktion), `refactor`, `docs`, `hardware`.
- Om något avgörande är oklart (önskat beteende, vilken hårdvara/kortversion, prioritet) — fråga innan du analyserar. Små oklarheter samlar du i stället under *Öppna frågor*.

## 2. Analysera kodbasen

Läs koden på riktigt; skriv inget du inte har sett.

- Hitta var beteendet finns idag. Ange `fil:rad` för varje relevant ställe.
- Följ kedjan i båda riktningar: vem anropar, vad påverkas. Typiska kedjor i detta repo:
  - Linjestatus: `SHKService`/`ToneReader` → `LineManager::setStatus` → `LineAction`
  - Inställningar: `settings.h` → `Settings.cpp` (load/save/`kVersion`) → `WebServer.cpp` → `data/settings.*`
  - Hårdvara: `include/config.h` (pinnar/adresser) → driver → service
  - Status ut: `StatusSerializer`, `MqttClient`, webb-UI (`data/app.js`)
- Kontrollera timing- och blockeringsaspekter: allt körs kooperativt i `App::update()`.
- För buggar: formulera en hypotes om orsaken och vad som stöder den. Skilj på *bekräftat i koden* och *misstänkt*.
- Kolla om README/docs behöver uppdateras.
- Sök efter relaterade issues: `gh issue list --search "<nyckelord>" --state all`.

## 3. Skriv issuen

Skriv på svenska. Använd mallen nedan i denna ordning. Sektion 1–7 är obligatoriska (skriv "Inga" om en sektion är tom). Valfria sektioner tas med när de är relevanta.

```markdown
## Problemställning
<Vad ska uppnås eller vad är fel, och varför det spelar roll. Beteende sett utifrån (telefon/webb/MQTT).
För buggar: steg för att återskapa, förväntat vs faktiskt beteende, orsakshypotes märkt som bekräftad/misstänkt.>

## Berörd kod
<Hur det fungerar idag och var, med fil:rad. Hela kedjan: anropare, beroenden, UI/MQTT/NVS.>
- `src/...:123` — <roll idag, vad som ska ändras>
- `data/...` — <vad> (kräver `uploadfs`)

## Föreslagen lösning och plan
<Angreppssätt och varför just detta. Kort om alternativ som valts bort och skälet.>

1. <Konkret steg — vilken fil, vilken funktion, vad som ändras>
2. ...

## Acceptanskriterier
- [ ] <Observerbart, testbart kriterium>
- [ ] Bygger med `pio run` (och `pio run -e wroom` om relevant)
- [ ] <README/docs uppdaterade, om berört>

## Verifiering
**Byggverifiering (Claude):** <vad Claude kan kontrollera själv>
**Hårdvarutest (Jonas):**
1. <Steg på riktig hårdvara, t.ex. "lyft lur på linje 2, slå 3 med fingerskiva, …">
2. <Vilken debugnivå/serieutskrift som visar att det fungerar>

## Out of scope
<Vad som medvetet inte ska göras i denna issue.>

## Öppna frågor / Beslut
**Öppna frågor** (måste besvaras innan implementation):
- [ ] <Fråga som Jonas behöver besvara>

**Beslut:**
- <Avgjord fråga> → <beslut> — <motivering>

<!-- Valfria sektioner nedan -->

## Risker
<Timing, SHK-störningar, NVS-kompatibilitet (`kVersion`), webb-UI ur synk med firmware, minne/flash, påverkan på andra linjer.>

## Beroenden
- Blockeras av #N / Kräver kortversion <X>

## Referenser
- <Datablad, `docs/`-diagram, tidigare PR:er>

## Granskningslogg
<Fylls i av /issue efter steg 3b, inte av dig under analysen. Se steg 3b punkt 6.>
```

Riktlinjer:
- Titel: kort, imperativ, svenska, utan prefix. Ex: "Ignorera SHK-pulser under ringpaus på inaktiva linjer".
- Planen ska vara så konkret att stegen går att bocka av, men inte innehålla färdig kod — kodsnuttar bara när de förtydligar ett gränssnitt eller en datastruktur.
- Gissa aldrig hårdvarufakta (pinnar, adresser, spänningar, timing från datablad). Lägg dem under *Öppna frågor* om de inte står i koden.
- När Jonas besvarar en öppen fråga: flytta den till *Beslut* med svar och motivering, så att den inte tas upp igen.
- Om uppdraget är för stort för en PR: föreslå uppdelning i flera issues och skriv dem var för sig, med beroenden angivna ("Blockeras av #N").

## 3b. Granskning av issue-reviewer (obligatorisk)

Du får inte gå vidare till steg 4 förrän granskaren har gett utslaget **GODKÄND**.

1. Spara utkastet (titel + brödtext) som markdown i scratchpad-katalogen.
2. Starta agenten `issue-reviewer` med Agent-verktyget. Ge den sökvägen till utkastet och en kort sammanfattning av uppdraget som Jonas beskrev det (granskaren har inte sett konversationen). Om Jonas redan har besvarat frågor eller fattat beslut, skicka med dem så att granskaren inte flaggar dem som gissningar.
3. Vid **UNDERKÄND**: åtgärda varje blockerande punkt i utkastet — gör ny analys i koden vid behov. Håller du inte med om en punkt, skriv ner varför. Skicka det uppdaterade utkastet till *samma* granskare (SendMessage) med en lista över vad som ändrats och dina motiveringar för punkter du inte åtgärdat.
4. Upprepa tills GODKÄND, högst 3 omgångar. Är granskaren fortfarande inte nöjd efter 3 omgångar, eller hänger en blockerande punkt på information bara Jonas har: stanna och lägg fram de kvarstående punkterna för Jonas. Jonas beslut ersätter granskarens godkännande.
5. Ta med granskarens icke-blockerande förslag som du valt att inte genomföra, och alla punkter markerade `[TILL JONAS]`, när du visar utkastet för Jonas.

6. Skriv in **Granskningslogg** som sista sektion i issuen (alltid, även vid godkännande direkt). Den läses senare av implementationssessionen och återges i färdigrapporten, så håll den kort och sakligt:
   ```markdown
   ## Granskningslogg
   - Iterationer: <antal granskningsomgångar tills GODKÄND, t.ex. 2>
   - Utslag per omgång: 1: UNDERKÄND (3 blockerande), 2: GODKÄND
   - Blockerande fynd och åtgärd:
     - <kort fynd> → <åtgärdat / avvisat med skäl>
   - Förslag (ej blockerande): <antal> varav <antal> genomförda
   - [TILL JONAS]: <kort, eller "inga">
   - Avgjort av Jonas utan godkännande från granskaren: <ja/nej>
   ```
   Loggen ska vara fakta om vad granskaren sa, inte din bedömning av den.

Förslag behöver du inte genomföra. Gör det bara om de är snabba och tydligt förbättrar issuen — annars blir issuen bara längre.

## 4. Granska med Jonas och skapa

1. Visa titel, etiketter och hela issue-texten. Lyft särskilt fram *Öppna frågor*, och ange granskningsutfallet (antal omgångar, vad som ändrades, kvarstående förslag).
2. Skapa issuen först efter Jonas godkännande (det är en publicering på GitHub).
3. Skriv brödtexten till en temporär fil i scratchpad-katalogen och skapa:
   ```
   gh issue create --repo ondfrukt/Phone-exchange --title "<titel>" --body-file <fil> --label "ai-planned" --label "<typ>"
   ```
   Om en etikett saknas (`gh label list`), fråga om den ska skapas (`gh label create ai-planned --color 5319E7 --description "Analyserad och planerad för AI-insats"`).
4. Återge issue-länken.

Om `gh` saknas eller inte är inloggat: säg det (installera med `winget install GitHub.cli`, logga in med `gh auth login`) och spara issuen som markdown i scratchpad-katalogen så att Jonas kan klistra in den.
