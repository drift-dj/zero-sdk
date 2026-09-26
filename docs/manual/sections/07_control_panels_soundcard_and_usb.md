# 7. Control Panels: Recording, Soundcard Routing & USB Subsystems

The Control Panels provide system configuration, input/output (I/O) signal routing, session recording, and diagnostic utilities.

## Deploying & Navigating Control Panels

* **Deploy / Dismiss Panels:** Long-press `JOG` (hold for 0.5s) from any screen to toggle the Control Panel overlay (`ZDJ_UI_CONTROL_TOGGLE_PANEL`).
* **Switch Panels:** Press `DECK 1` (Previous Panel) or `DECK 2` (Next Panel) to navigate horizontally between panel tabs.
* **Select & Adjust:** Turn `JOG` to highlight parameters, and press `JOG` to modify values or deploy submenus.

```text
[ RECORDING ] <-> [ SOUNDCARD / MIXER ] <-> [ FILE BROWSER ] <-> [ USB SETUP ] <-> [ SETTINGS ]
```

## 1. Recording Control Panel (`ZDJ_UI_PANEL_RECORDING`)

The recording engine captures uncompressed 32-bit floating-point, 44.1 kHz stereo WAV audio directly to internal flash storage (`zdj_audio_record_node.c`).

```text
+=============================================================+
| [PANEL: RECORDING]    STATUS: [RECORDING]     TIME: 00:42:15|
| L: [||||||||||||||||||....] -3.2 dB (PEAK)    REC GAIN: -2dB|
| R: [||||||||||||||||||....] -3.2 dB (PEAK)   (SHIFT+VOL=ADJ)|
| FILE: /media/internal/recordings/recording_001.wav          |
| INPUT: >[ MAIN BUS LR ]                                     |
+=============================================================+
```

### Telemetry & Metadata Specifications
* **Active Status:** Displays elapsed session time, real-time peak VU level meters, and the destination filename (`recording_XXX.wav`).
* **Storage Location:** Written to `/media/internal/recordings/`.
* **Database Cataloging:** When recording finalizes, the file is automatically registered in the SQLite library with artist `"Zero Recording"` and title `"recording_XXX"`.
* **Sync Grid Inheritance:** If Sync is engaged during recording, the WAV file automatically inherits the master deck's BPM and beatgrid metadata (`zdj_audio_record_node.c:345`).
* **Record Headroom Gain:** Hold `NAV/SHIFT` and turn `VOL` to adjust digital recording gain (-12 dB to +12 dB). Because audio is stored in 32-bit float, internal mixing cannot clip digitally before writing to disk.

### Procedure: Recording a Live DJ Set

1. Long-press `JOG` for 0.5s. The Control Panels overlay displays.
2. Press `DECK 1` or `DECK 2` until the `RECORDING` tab displays.
3. Turn `VOL` while holding `NAV/SHIFT` to calibrate record bus gain. The peak VU meters reflect input amplitude.
4. Press `TONE 1` (or press `SHIFT + PLAY` from the live performance view). Recording commences, the `[REC]` badge illuminates, and the session timer starts.
5. Press `SHIFT + PLAY` when the set concludes. The audio engine cleanly flushes memory buffers, closes the WAV header, and logs the file to the library.

## 2. Soundcard / Mixer Panel (`ZDJ_UI_PANEL_SOUNDCARD`)

Governs the low-level ALSA audio crossbar and DSP routing matrix (`zdj_soundcard_panel.c`). The panel renders as a horizontally scrolling channel-strip interface.

```text
+=============================================================+
| [PANEL: SOUNDCARD]    ROUTING MATRIX          PRESET: [CLUB]|
| [D1]  [EXT]  [D2]  |  [AUX0]  [CV0]  [MAIN]  [CUE]  [OUT 1] |
| -2dB   0dB   -6dB  |   0dB     PPQN   -1dB   -3dB   STEREO  |
| (JOG: SCROLL CHANNELS) (TONE 1: GAIN) (TONE 2: PAN/ROUTING) |
+=============================================================+
```

### Channel Strips
* **Decks:** Deck 1 (`DECK_1_PREFADE`), Ext Deck (`DECK_EXT_PREFADE`), Deck 2 (`DECK_2_PREFADE`).
* **Auxiliary Busses:** Aux 0 through Aux 3 for sub-mixing and external effects routing.
* **Control Voltage (CV) Busses:** CV 0 through CV 3 for analog clock synchronization (1, 2, or 4 Pulses Per Quarter Note / PPQN).
* **Busses:** Main Bus (`MAIN_BUS`), Headphone Cue Bus (`CUE_BUS`), Record Bus (`RECORD_BUS`).
* **Hardware Ports:** Output 1, Output 2, Input 1, Input 2, and USB I/O.

### Output Port Modes
* **`OUTPUT 1` Assignment:**
  * **Master Stereo (Default):** Primary front-of-house sound system feed (L/R).
  * **Booth Stereo:** Independent secondary monitor feed with dedicated gain trim.
* **`OUTPUT 2` Assignment:**
  * **Headphone Cue (Stereo Default):** Dedicated stereo cue bus with pre-fader listen (PFL) mixing.
  * **Split-Cue Mode (Mono L/R):** Left headphone ear receives the cue bus (PFL); right headphone ear receives the master house mix.

### Procedure: Enabling Split-Cue Monitoring

1. Long-press `JOG` to deploy the Control Panels.
2. Press `DECK 2` until the `SOUNDCARD` panel displays.
3. Turn `JOG` horizontally to highlight the `OUTPUT 2` strip and press `JOG`. The output options menu displays.
4. Turn `JOG` to `Routing Mode` and press `JOG` to select `Split-Cue (Mono L/R)`.
5. Put on headphones. Left ear monitors active PFL channels; right ear monitors the live master bus.
6. Long-press `JOG` to return to DJ Playback.

## 3. File Browser Panel (`ZDJ_UI_PANEL_BROWSER`)

Provides direct navigation of local Linux file partitions, internal storage `/media/internal/`, and external USB devices `/media/usb/`.

* **Audio Auditioning:** Highlight an audio file (`.wav`, `.mp3`) and press `TONE 2`. Audio plays in headphones via the cue bus without interrupting playing decks.
* **File Operations:** Copy, move, or delete files, and stage system `.pkg` update packages.

## 4. USB Setup Panel (`ZDJ_UI_PANEL_USB`)

Configures USB controller operation:

* **USB Host Mode:** Zero acts as the host controller, supplying 5V bus power to attached flash drives, SSDs, and USB MIDI keyboards.
* **USB Gadget Mode:** Zero acts as a peripheral connected to a workstation computer:
  * **Drive Mode (Mass Storage):** Mounts internal storage as an external drive volume.
  * **USB Audio Mode:** Operates as a class-compliant 4-in / 4-out USB audio interface (44.1 kHz, 24-bit PCM) for recording into DAWs.
  * **USB MIDI Mode:** Sends and receives MIDI clock pulses for external hardware sequencer synchronization.

## 5. Settings & Developer Panel (`ZDJ_UI_PANEL_SETTINGS`)

### UI Settings
* **Function Knob Pages:** Toggle visibility or customize order for knob pages (`EQ`, `TRIM`, `FILTER`, `SYNC`, `LOOP`).
* **Library Annotations:** Toggle BPM, Key, and Camelot compatibility badges.
* **Display Contrast:** Calibrate OLED brightness.

### System Settings
* **OS Updates:** Installs signed firmware update archives (`.pkg`).
* **Granular Reset Dialogs:**
  * **Reset Library Database:** Clears SQLite track tables and resets library indices without deleting audio files on disk.
  * **Reset Soundcard DB:** Restores factory audio routing matrices and presets.
  * **Full System Reset:** Returns all settings, soundcard profiles, and UI configurations to factory state.

### Developer & Diagnostics Submenu (`zdj_settings_developer_panel.c`)
*Added in System Version 0.6.0 (PR #33):*
* **Configurable Crash Dialog:** Toggles an on-screen crash notification widget upon unexpected process restart, preventing silent reboots.
* **Crash Log Viewer:** Displays backtraces and core dump information from `/var/log/zdj_crash.log`.
* **DSP Performance Telemetry:** Displays real-time CPU load, audio ring buffer margin, and memory utilization.

## Failure Modes & Troubleshooting

| Condition | Symptom | Root Cause | Corrective Action |
|---|---|---|---|
| Recording stops unexpectedly | Session terminates mid-set | Internal storage partition reached capacity | Verify available disk space in `Settings > System Info` before long recording sessions. |
| Split-Cue delivers identical audio to both ears | Both headphone sides play master mix | PFL not engaged on incoming deck, or Output 2 set to Stereo | Go to `TRIM/CUE` knob page and press `TONE 2` to engage PFL on the auditioning deck. |
| USB Audio not visible in DAW | Workstation audio settings do not list Zero | USB controller is in Host Mode instead of Gadget Audio Mode | Go to `Control Panels > USB Setup`, select `Gadget Mode`, and set profile to `USB Audio`. |
