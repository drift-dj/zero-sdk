# 1. System Overview & Hardware Architecture

Drift Zero is a standalone dual-deck DJ performance instrument running on an embedded dual-architecture platform. A host Linux system manages user interface rendering, database management, and media decoding, while an integrated ARM Cortex-M7 real-time coprocessor handles low-latency audio input/output (I/O) and hardware digital signal processing (DSP) over shared memory buffers at 44.1 kHz.

Zero operates in three primary UI environments:
1. **DJ Playback:** The live performance screen providing waveform visualizations, beatgrid alignment, looper status, and deck audio manipulation.
2. **Library Menu:** A searchable, key-aware SQLite database browser for tracks, playlists, and file imports.
3. **Control Panels:** Contextual modal overlays for audio routing (Soundcard/Mixer), USB mode configuration, direct recording, and system diagnostics.

## Front Panel Controls & Nomenclature

The physical console provides high-density, multifunction rotary and tactile controls.

```text
+-------------------------------------------------------------+
|  [TONE 1]        [TONE 2]        [TONE 3]     [ 1 EXT . 2 ] |
|  (Encoder+Btn)   (Encoder+Btn)   (Encoder+Btn)     (VOL)    |
|                                                             |
|  +---------------------------------------+   [DECK 1]       |
|  |             OLED DISPLAY              |   [ EXT  ]       |
|  |            (128 x 64 px)              |   [DECK 2]       |
|  +---------------------------------------+                  |
|                                                [NAV/SHIFT]  |
|      ( JOG WHEEL )                [CUE]                     |
|     (Encoder + Push)              [PLAY]                    |
|                                                             |
|  [DECK 1 FADER]      [CROSSFADER]       [DECK 2 FADER]      |
|        |                  |                    |            |
+-------------------------------------------------------------+
```

### 1. Rotary Encoders & Pushbuttons
* **Jog Wheel (`JOG`):** Primary optical encoder with integrated center pushbutton.
  * **Turn:** Nudges playing deck; scrubs stopped deck; scrolls menu lists.
  * **Press:** Selects active menu item; confirms modal dialogs.
  * **Long Press (Hold 0.5s):** Deploys or dismisses the Control Panels overlay (`ZDJ_UI_CONTROL_TOGGLE_PANEL`).
  * **Shift + Turn:** Needledrop fast-scan through loaded track timeline.
  * **Press + Turn:** Adjusts playback tempo in 0.1 BPM increments (adjusts lead tempo when Sync is enabled, or focused deck tempo when independent).
* **Tone Encoders (`TONE 1`, `TONE 2`, `TONE 3`):** Three rotary encoders with integrated push switches (`FN_1`, `FN_2`, `FN_3`). These control the active **Function Knob Page** (EQ, Trim/Cue, Filter, Sync, Looper).
* **Master Volume Rotary (`1 EXT . 2 VOL.` / `OUT`):** Dual-function analog matrix and cue attenuator.
  * **Turn:** Adjusts Main Master L/R Bus Volume.
  * **Press + Turn:** Adjusts Headphone Cue Bus Volume.
  * **Shift + Turn:** Adjusts Recording Bus Gain.

### 2. Transport & Navigation Buttons
* **Play / Pause (`PLAY`):**
  * **Press:** Starts or pauses playback on the focused deck.
  * **Shift + Press:** Toggles background session recording (`Start/Stop Recording`).
* **Cue (`CUE`):**
  * **Press & Hold:** Auditions audio from the active cuepoint.
  * **Release:** Instantly returns playhead to cuepoint and stops audio.
  * **Shift + Press:** Advances to the next stored cuepoint (`1` through `8`).
* **Nav / Shift (`NAV/SHIFT`):**
  * **Press:** Toggles Library Menu overlay.
  * **Hold:** Modifies buttons and encoders to secondary functions.
* **Deck Select Buttons (`DECK 1`, `EXT`, `DECK 2`):**
  * **Press:** Focuses the Playback UI and Function Knob assignments on Deck 1, Ext Deck, or Deck 2.
  * **Press Focused Deck:** Cycles forward through Function Knob Pages.
  * **Shift + Press Focused Deck:** Cycles backward through Function Knob Pages.
  * **Inside Control Panels:** Navigates left/right between panel tabs.

### 3. Faders
* **Deck 1 & Deck 2 Channel Faders:** Smooth linear faders controlling direct deck volume into the mix bus.
* **Crossfader:** Low-friction horizontal fader transitioning audio between Deck 1 and Deck 2 with software-selectable curves (fast cut or linear blend).

## Back Panel Connections & Power Architecture

```text
+-------------------------------------------------------------+
|  [POWER]   (OUT 1)   (OUT 2)   (IN 1)   (IN 2)   [ USB-C ]  |
+-------------------------------------------------------------+
```

* **Power Button (`POWER`):**
  * **Hold 1 Second:** Powers on console.
  * **Hold 6 Seconds:** Executes safe Linux system shutdown and powers off hardware.
* **Analog Audio Outputs (`OUTPUT 1`, `OUTPUT 2`):**
  * Dual 3.5 mm (1/8") Tip-Ring-Sleeve (TRS) stereo analog outputs.
  * Default routing: `OUTPUT 1` = Main Master Mix (L/R); `OUTPUT 2` = Headphone Monitor / Cue Bus.
* **Analog Audio Inputs (`INPUT 1`, `INPUT 2`):**
  * Dual 3.5 mm (1/8") TRS stereo line-level analog inputs routed to the ARM Cortex-M7 analog-to-digital converter (ADC) stage.
  * Signals route to the `EXT` Deck for live EQ/filtering or directly to the session recording bus.
* **USB-C Port:**
  * Dual-role USB 2.0 controller:
    * **USB Host Mode:** Supplies 5V bus power to external USB flash drives, SSDs, or class-compliant USB MIDI controllers.
    * **USB Gadget Mode:** Connects Zero to a host workstation as a USB Mass Storage drive, a 4-in/4-out USB audio interface, or a USB MIDI interface.

> [!IMPORTANT]
> **Interconnect Cables for Professional Audio Equipment**  
> Zero uses standard 3.5 mm (1/8") TRS stereo jacks. To interface with professional DJ mixers, audio interfaces, or active studio monitors, use a **3.5 mm TRS stereo to dual RCA male** or **3.5 mm TRS stereo to dual 1/4" TS male** breakout cable. Connecting a mono TS cable directly into Zero shorts the right audio channel to ground.

Connect power before attaching high-drain USB storage devices.

> [!WARNING]
> **Power Requirements & Brownout Prevention**  
> Drift Zero requires a stable 5V DC power supply capable of delivering at least 2.0A (10W). Powering the unit from unpowered USB hubs or low-current legacy USB-A ports can cause voltage drops during heavy bass transients, resulting in screen flickering or sudden hardware resets.
