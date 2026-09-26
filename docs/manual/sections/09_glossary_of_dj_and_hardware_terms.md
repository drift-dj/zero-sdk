# 9. Glossary of DJ & Hardware Terms

### Beatgrid
A sequence of timestamped quarter-note intervals mapped to an audio track. Zero's software engine uses beatgrids to synchronize playback tempo, calculate phase offsets, and align automated loop boundaries.

### Camelot Wheel
A harmonic categorization system numbering the Circle of Fifths from 1 to 12 with letter designations A (Minor) and B (Major). Adjacent numbers and relative letter swaps indicate compatible musical keys that blend without harmonic dissonance.

### Crossfader
A horizontal slider transitioning audio between Deck 1 and Deck 2. Moving the fader fully to the left routes only Deck 1; moving it fully to the right routes only Deck 2. Center position mixes both decks simultaneously.

### Cue / PFL (Pre-Fader Listen)
An isolated monitoring bus routing pre-fader deck audio directly to headphones. Allows auditioning and phase alignment before opening the channel volume fader to the audience.

### Downbeat (Bar 1, Beat 1)
The primary rhythmic pulse initiating a musical measure or phrase. Zero anchors beatgrid calculations to the initial downbeat transient.

### Hotcue
A persistent digital bookmark mapped to an exact waveform timestamp. Triggering a hotcue jumps the playhead to the marker and immediately initiates playback.

### Isolator EQ
A 3-band equalizer providing full frequency band attenuation (-infinity dB) at minimum knob rotation. Allows complete removal of low, mid, or high bands without affecting neighboring frequencies.

### Needledrop
A navigation shortcut (`SHIFT + Turn Jog`) that moves the playback position in multi-bar intervals to scan track arrangements rapidly.

### Nudge (Pitch Bend)
A temporary increase or decrease in playback speed executed by rotating `JOG` while a deck is playing. Aligns transient phase without permanently modifying track tempo.

### Quantize
An execution timing assist that aligns manual performance triggers (such as loop engagement or hotcue strikes) to the nearest beatgrid interval.

### Slew Interpolation
Digital signal smoothing applied to fader and encoder parameters across audio buffer frames. Eliminates zipper noise and stepping artifacts during fast fader motions.

### Split-Cue
A headphone monitoring mode dividing the stereo output: the left channel monitors the pre-fader Cue bus, and the right channel monitors the master output mix.

### TSM (Time-Scale Modification)
A digital signal processing algorithm that alters audio playback speed (tempo) while maintaining original musical key and pitch.

### USB Host Mode vs. Gadget Mode
* **Host Mode:** Zero acts as the host controller, supplying 5V bus power to external flash drives, SSDs, and USB MIDI controllers.
* **Gadget Mode:** Zero acts as an accessory device attached to a workstation computer, providing USB Mass Storage drive access, a 4-in/4-out USB audio interface, or USB MIDI clock communication.
