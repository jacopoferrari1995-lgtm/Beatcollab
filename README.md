# Jackpot plugins (AU)

- **Jackpot Pad**: drum sampler a 16 pad (note da C1), velocity a curve, tastiera del computer come Ableton (Z/X ottava, C/V velocity), registratore con quantizzazione, swing e strength, drag MIDI.

- **Jackpot 60**: sampler/sequencer stile MPC (64 pad, 8 sequenze, chop, resample, drum bus, delay, riverbero, loop con BPM automatici).
- **Jackpot Tape**: tape stop, start, spinback, half speed, reverse, stutter, ripeti zona, glitch, gate, coreografia fino a 8 battute, BPM dall'host.

I `.component` si scaricano dalla pagina Releases.

Installazione: copia i `.component` in `~/Library/Audio/Plug-Ins/Components`, poi nel Terminale:

    xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Jackpot*.component

Build: CMake + JUCE 8.0.4 (clonato in `JUCE/`), workflow in `.github/workflows/build-mac.yml`.
