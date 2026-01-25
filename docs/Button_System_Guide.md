# Guide: Skapa egna knapptyper

En praktisk guide för hur du skapar och anpassar dina egna knappstilar i `style.css`.

## Grundstruktur

Alla knappar börjar med basklassen `.btn` som innehåller gemensam styling. Sen lägger du till varianter för olika utseenden.

```css
/* Basknapp - alla knappar börjar här */
.btn {
  /* Layout & Display */
  display: inline-flex;
  align-items: center;
  justify-content: center;
  
  /* Storlek & Spacing */
  padding: 8px 16px;           /* Ändra för större/mindre knappar */
  font-size: 0.9rem;           /* Textstorlek */
  
  /* Utseende */
  border-radius: 8px;          /* Rundade hörn */
  border: 1px solid var(--border);
  background: rgba(255, 255, 255, 0.04);
  color: var(--fg);
  
  /* Bredd */
  width: auto;                 /* auto = anpassar efter innehåll */
  min-width: fit-content;      /* Minst så bred som innehållet */
  
  /* Övrigt */
  cursor: pointer;
  transition: all 0.2s ease;   /* Mjuka övergångar */
}
```

## Parametrar du kan styra

### 1. Färger
```css
.btn-din-variant {
  /* Border färg */
  border-color: rgba(96, 165, 250, 0.4);
  
  /* Bakgrundsfärg (använd rgba för transparens) */
  background: rgba(96, 165, 250, 0.15);
  
  /* Textfärg */
  color: rgb(96, 165, 250);
}
```

**Användbara färger från ditt tema:**
- `var(--accent)` = #60a5fa (blå)
- `var(--ok)` = #16a34a (grön)
- `var(--no)` = #ef4444 (röd)
- `var(--fg)` = #e5e7eb (vit text)
- `var(--muted)` = #9ca3af (grå text)

### 2. Storlek
```css
.btn-din-storlek {
  font-size: 1rem;             /* Textstorlek */
  padding: 12px 24px;          /* Vertikalt Horisontellt */
  border-radius: 10px;         /* Rundning (större = mjukare) */
}
```

### 3. Bredd
```css
.btn-din-bredd {
  min-width: 100px;            /* Minsta bredd */
  max-width: 200px;            /* Största bredd */
  width: auto;                 /* auto / 100% / fast px-värde */
}
```

### 4. Hover-effekt
```css
.btn-din-variant:hover:not(:disabled) {
  background: rgba(96, 165, 250, 0.25);  /* Ljusare vid hover */
  border-color: rgba(96, 165, 250, 0.6);
  transform: translateY(-1px);            /* Lyft knappen lite */
}
```

## Mall för ny knappvariant

Kopiera denna mall till `style.css` och anpassa:

```css
/* Min nya knappvariant */
.btn-minvariant {
  /* Färger */
  border-color: rgba(255, 120, 60, 0.4);      /* Orange border */
  background: rgba(255, 120, 60, 0.15);       /* Orange bakgrund */
  color: rgb(255, 160, 120);                   /* Orange text */
}

/* Hover-effekt */
.btn-minvariant:hover:not(:disabled) {
  background: rgba(255, 120, 60, 0.25);       /* Ljusare vid hover */
  border-color: rgba(255, 120, 60, 0.6);      /* Tydligare border */
}
```

**Användning:**
```html
<button class="btn btn-minvariant">Min knapp</button>
```

## Praktiska exempel

### Exempel 1: Ljusblå info-knapp
```css
.btn-info {
  border-color: rgba(56, 189, 248, 0.4);
  background: rgba(56, 189, 248, 0.15);
  color: rgb(125, 211, 252);
}

.btn-info:hover:not(:disabled) {
  background: rgba(56, 189, 248, 0.25);
  border-color: rgba(56, 189, 248, 0.6);
}
```

### Exempel 2: Varnings-knapp (gul)
```css
.btn-warning {
  border-color: rgba(234, 179, 8, 0.4);
  background: rgba(234, 179, 8, 0.15);
  color: rgb(250, 204, 21);
}

.btn-warning:hover:not(:disabled) {
  background: rgba(234, 179, 8, 0.25);
  border-color: rgba(234, 179, 8, 0.6);
}
```

### Exempel 3: Kompakt knapp (mindre padding)
```css
.btn-tiny {
  font-size: 0.75rem;
  padding: 2px 8px;
  border-radius: 4px;
  min-width: 50px;
}
```

### Exempel 4: Bred action-knapp
```css
.btn-action {
  font-size: 1.1rem;
  font-weight: 600;
  padding: 14px 32px;
  min-width: 200px;
  border-radius: 12px;
}
```

## Färgverktyg

För att hitta bra rgba-värden:

1. **Välj en hex-färg** (t.ex. #60a5fa)
2. **Border:** rgba(96, 165, 250, 0.4) - 40% opacitet
3. **Bakgrund:** rgba(96, 165, 250, 0.15) - 15% opacitet  
4. **Hover bakgrund:** rgba(96, 165, 250, 0.25) - 25% opacitet
5. **Text:** rgb(96, 165, 250) - 100% opacitet

## Kombinera med breddklasser

Du kan använda dina egna knappar med befintliga breddklasser:

```html
<!-- Din knapp + compact bredd -->
<button class="btn btn-minvariant btn-compact">OK</button>

<!-- Din knapp + wide bredd -->
<button class="btn btn-minvariant btn-wide">Spara inställningar</button>

<!-- Din knapp + anpassad min/max -->
<button class="btn btn-minvariant btn-min-100 btn-max-250">Flexibel</button>
```

## Snabbreferens: Befintliga klasser

**Varianter:**
- `.btn-primary` - Blå (huvudåtgärd)
- `.btn-success` - Grön (bekräfta)
- `.btn-danger` - Röd (ta bort)
- `.btn-secondary` - Grå (sekundär)
- `.btn-ghost` - Transparent (diskret)

**Storlekar:**
- `.btn-sm` - Liten
- (ingen klass) - Medium
- `.btn-lg` - Stor

**Bredder:**
- `.btn-compact` (60-120px)
- `.btn-wide` (150-300px)
- `.btn-full` (100%)
- `.btn-min-XX` / `.btn-max-XX`

**Tillstånd:**
- `.working` - Visar "..." animation
- `disabled` (HTML attribut) - Inaktiverad

## Tips

1. **Testa i button-demo.html** - Lägg till din knapp där för att se den i action
2. **Använd rgba() för transparens** - Fungerar bättre med dark mode
3. **Håll opacity-värdena konsekventa** - Border ~0.4, Background ~0.15, Hover ~0.25
4. **Kopiera befintliga knappar** - Ändra bara färgvärdena för snabbast resultat
