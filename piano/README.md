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
