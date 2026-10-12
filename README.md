# BEATSTATION

**manners! DSP** rhythm machine. AU and VST3, macOS universal (Apple Silicon and Intel). DRONESTATION's sibling.

Two voices, two lanes, one clock. **PUNCH** hits up front, **WASH** rolls behind it. Each voice is a monophonic percussion and bass engine: two oscillators and noise, each with a pitch drop, FM, ring and cross-ring modulation, a metallic hi-hat circuit, and a gritty multimode filter. The sequencer's pitch and velocity modulate the sound, so one voice can sound like a whole kit. Inspired by the hybrid drum and bass idea behind Bram Bos's Ruismaker Noir.

## Playing it

- **Press play in Logic.** BEATSTATION follows Logic's tempo and transport.
- **Lanes:** click to place a note, drag up and down for pitch, left and right for velocity (circle size). Click a note to remove it; right-click also removes. Each lane has its own STEPS (1 to 32) and step RATE, so the lanes can drift against each other.
- **ENV: HIT or WAVE.** HIT is percussive. WAVE swells and decays slowly, and each new hit pushes the swell higher rather than restarting it, so a pattern becomes rolling waves.
- **TEMPER:** centre plays exactly what you programmed. Left calms it down (filters, softens, removes grit and crunch). Right pushes it into chaos (grit, FM, bit crushing, random pitch drops and more MUTATE).
- **MUTATE:** chances that a step plays another step's note, shifts pitch, or changes velocity, live, without changing the stored pattern.
- **MOD MATRIX:** how each step's pitch and velocity change the drop, FM, noise, oscillator balance, cutoff and levels.
- **Presets** save the sound and both patterns.
- **Launch Control XL:** see `Profile/LaunchControlXL-Profile.md`.

## Install

1. Unzip `BEATSTATION-AU.zip`.
2. Drag `BEATSTATION.component` into `/Library/Audio/Plug-Ins/Components`.
3. In Terminal: `sudo xattr -dr com.apple.quarantine /Library/Audio/Plug-Ins/Components/BEATSTATION.component`
4. In Logic: Software Instrument track, then AU Instruments > manners! DSP > BEATSTATION.
