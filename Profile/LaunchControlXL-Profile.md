# BEATSTATION x Launch Control XL (original, with pots)

BEATSTATION uses the Launch Control XL's **factory User templates**, the same as DRONESTATION: nothing to load onto the controller.

**User template 1 plays the pattern. User template 2 shapes the sound.** Hold **User** and press button **1** or **2** to switch.

Put BEATSTATION on a Software Instrument track and select or record-arm it so it receives the controller. The top right corner of the plugin shows every MIDI message it receives.

## User template 1: the pattern

The knobs and faders edit **8 steps at a time** (one column per step). The amber bar under each lane shows which 8.

| Control | Does |
|---|---|
| Knob row 1 | PUNCH step pitch |
| Knob row 2 | PUNCH step velocity (fully left = no note) |
| Knob row 3 | WASH step pitch |
| Faders | WASH step velocity (fully down = no note) |

Pitches snap to the SCALE you've chosen.

## User template 2: the sound

| Row | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| Knob row 1 (PUNCH) | DROP 1 | SLOPE 1 | FM | NOISE | CUTOFF | RES | GRIT | DECAY |
| Knob row 2 (WASH) | DROP 1 | SLOPE 1 | FM | NOISE | CUTOFF | RES | GRIT | DECAY |
| Knob row 3 | TEMPER | BIT DRIVE | BOOST | MUTATE ORDER | MUTATE PITCH | MUTATE VEL | SHUFFLE | GAIN |
| Faders | PUNCH LEVEL | WASH LEVEL | PUNCH F.ENV | WASH F.ENV | PUNCH ATTACK | WASH ATTACK | WIDTH | BOOST FREQ |

## Buttons (both templates)

| Button | Does |
|---|---|
| Track Focus 1 to 4 | Page: steps 1-8, 9-16, 17-24, 25-32 |
| Track Focus 5 | RANDOM PUNCH |
| Track Focus 6 | RANDOM WASH |
| Track Focus 7 | CLEAR PUNCH |
| Track Focus 8 | CLEAR WASH |
| Track Control 1 | Mute PUNCH |
| Track Control 2 | Mute WASH |

## Good to know

- The pots are absolute. After switching page or template, a knob jumps to wherever the physical knob sits the first time you move it.
- If your User templates have been customised, make fresh ones in Novation Components (a new template starts at factory defaults): MIDI channel 1 into User slot 1, channel 2 into slot 2.
- If Logic grabs the controller for its own mixer, remove the Launch Control XL from Logic's Control Surfaces setup.
