# Piano Generativo

Apri `index.html` nel browser: è un file unico, senza dipendenze.

I sorgenti sono in `src/`; dopo averli modificati rigenera la pagina con:

```
python3 build.py
```

- `src/engine.js` — armonia per sezioni, voicing, arrangiamento a strati, melodia, MIDI, ZIP
- `src/synth.js` — sintesi audio (pianoforte, Rhodes, basso, pad, batteria)
- `src/app.js` — generatore, vista arrangiamento, trasporto, esportazione
- `src/ex.js` — esercizi
- `src/style.css`, `src/index.tpl.html` — aspetto e struttura

## Batteria e basso

- **Batteria**: ogni genere ha stili con nome (es. reggae: one drop · rockers · steppers; dancehall: dembow · bashment; trap: classica · bouncy · drill · dark). Ogni colpo ha un'importanza (9 = ossatura … 3 = ghost): la densità decide quali colpi suonano, la stessa cifra dà la dinamica. Frase A A' A B con fill e build specifici del genere.
- **Basso**: personalità (tenuto, ottavi, ottave, walking, 808 con glide, segue la cassa, riddim, funk, arpeggiato, del genere) scelte da genere e mood.
- **Rigenera** (🎲 nella traccia): casuale, oppure "rigenera come…" uno stile scelto, che resta bloccato per quella traccia.
- **Vuoto/pieno**: pause dentro le battute (respiro su 2 battute), non battute intere tolte. La densità agisce anche sullo spessore degli accordi.
