# Jackpot plugins (AU)

- **Jackpot Pad**: drum sampler a 16 pad. Schermo a pagine (Kit, Mixer, Sample, Velocity, EQ, Filtro, Comp, FX, Master) con 8 Q-link contestuali, 8 kit di fabbrica per stile, salvataggio kit utente (Musica/Jackpot Pad/Kits), catena per pad (filtro, EQ 3 bande, comp soft/medium/hard, drive, lo-fi), mandate a reverb e delay a tempo, bus comp e clip, sync BPM con Ableton (repitch o stretch), registratore con quantize/swing/strength e drag MIDI.

- **Jackpot 60**: sampler/sequencer stile MPC (64 pad, 8 sequenze, chop, resample, drum bus, delay, riverbero, loop con BPM automatici).
- **Jackpot Tape**: tape stop, start, spinback, half speed, reverse, stutter, ripeti zona, glitch, gate, coreografia fino a 8 battute, BPM dall'host.

I `.component` si scaricano dalla pagina Releases.

Installazione: copia i `.component` in `~/Library/Audio/Plug-Ins/Components`, poi nel Terminale:

    xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Jackpot*.component

Build: CMake + JUCE 8.0.4 (clonato in `JUCE/`), workflow in `.github/workflows/build-mac.yml`.
