# 3. Function Knob Pages & Audio Processing

The three rotary encoders at the top of the console (`TONE 1`, `TONE 2`, `TONE 3`) dynamically control deck digital signal processing (DSP) functions according to the active **Function Knob Page**.

## Page Navigation & Deck Assignment

* **Deck Selection:** Press `DECK 1`, `EXT`, or `DECK 2` to bind the Function Knobs to that deck.
* **Cycling Pages Forward:** Press the active Deck Select button to advance through that deck's available Function Knob Pages.
* **Cycling Pages Backward:** Hold `NAV/SHIFT` and press the active Deck Select button to reverse the cycle.
* **Reordering Pages:** Go to `Control Panels > Settings > UI Settings > Knob Pages` to show, hide, or reorder available pages.

## Function Knob Page Matrix

### 1. 3-Band Isolator Equalizer (`EQ`)

Provides 3-band isolator equalization with total band kill (-inf) and +6 dB boost.

![EQ Isolator Bass Swap OLED Animation](assets/bass_swap_animated.gif)

```text
+=============================================================+
| [D1] EQ ISOLATOR      SWAP: [D1 <-> D2]       PAGE: [1/5]   |
| LOW:  [-12dB]  [||||......] (TONE 1: TURN=ADJ / PRESS+TURN) |
| MID:  [  0dB]  [||||||....] (TONE 2: TURN)                  |
| HIGH: [ +2dB]  [||||||||..] (TONE 3: TURN)                  |
+=============================================================+
```

* **`TONE 1` (Turn):** Adjusts Low EQ (-inf to +6 dB).
* **`TONE 1` (Press + Turn):** **Bass Swap:** Continuously adjusts Deck 1 and Deck 2 Low EQ in inverse directions. Rotating clockwise attenuates Deck 1 low frequencies while proportionally boosting Deck 2 low frequencies (`zdj_soundcard_deck.c`).
* **`TONE 2` (Turn):** Adjusts Mid EQ (-inf to +6 dB).
* **`TONE 3` (Turn):** Adjusts High EQ (-inf to +6 dB).

#### Procedure: Low-Frequency Bass Swap Transition

1. Raise `DECK 2 FADER` to full volume with Deck 2 Low EQ set to minimum (-inf). Deck 2 mids and highs become audible over Deck 1.
2. Press and rotate `TONE 1` clockwise on Beat 1 of a musical phrase. Deck 1 low frequencies attenuate while Deck 2 low frequencies increase in inverse proportion.
3. Lower `DECK 1 FADER` to zero. Deck 2 now carries the full frequency spectrum of the mix.

### 2. Deck Gain & Headphone Monitoring (`TRIM/CUE`)

Manages pre-fader channel amplification and headphone cue bus routing.

* **`TONE 1` (Turn):** Adjusts Deck Pre-Fader Gain (-12 dB to +12 dB).
* **`TONE 2` (Press):** Toggles Pre-Fader Listen (PFL / Cue Mute) for this deck on the headphone monitor bus.
* **`TONE 2` (Turn):** Adjusts headphone cue balance for this deck.

### 3. Bipolar Resonant Filter (`FILTER`)

```text
+=============================================================+
| [D1] BIPOLAR FILTER   STATE: [HPF ACTIVE]     PAGE: [3/5]   |
| CUTOFF: [4.2 kHz]     [.........||||] (TONE 1: TURN/PRESS=R)|
| RESONANCE: [ 45%]     [|||||........] (TONE 2: TURN)        |
| MODE: 24dB/OCT        SWEEP: HIGH-PASS                      |
+=============================================================+
```

A dual-topology 24 dB/octave sweep filter combining Low-Pass and High-Pass operation on a single control:
* **`TONE 1` (Turn):** Sweeps cutoff frequency:
  * **Counter-Clockwise:** Low-Pass Filter (LPF). Attenuates high frequencies progressively.
  * **Center (12 O'Clock):** Filter bypassed.
  * **Clockwise:** High-Pass Filter (HPF). Attenuates low and mid frequencies progressively.
* **`TONE 1` (Press):** Resets filter cutoff immediately to center bypass.
* **`TONE 2` (Turn):** Adjusts filter resonance (Q factor).

### 4. Beat Sync Engine (`SYNC`)

Aligns tempo and phase to the session master clock using beatgrid reference anchors:
* **`TONE 1` (Press):** Toggles Sync active. The follower deck's tempo and transient phase lock to the master lead deck.
* **`TONE 1` (Turn):** Adjusts phase offset against the master clock.

### 5. Quantized Looper & Beat Skip (`LOOP/SKIP`)

![Performance Looper & Stutter Roll Animation](assets/looper_stutter_animated.gif)

```text
+=============================================================+
| [D1] LOOPER ACTIVE    [ 4 BEATS ]             PAGE: [5/5]   |
| LOOP RANGE: BAR 16.1 -> 17.1 (SNAP: ON) (TONE 1: PRESS=EXIT)|
| QUANTIZE: [ 1 BEAT ]  MOVE: [+- 1 BAR]  (TONE 2: MOVE/QUANT)|
| BEAT SKIP: [+- 1 BEAT]                  (TONE 3: SKIP)      |
+=============================================================+
```

Controls loop capture, boundary modulation, and position jumping:

| Control | State: Loop Inactive (`LOOP OFF`) | State: Loop Active (`LOOP ON`) |
|---|---|---|
| **`TONE 1` (Press)** | **Engage Loop:** Captures loop of selected length at playhead. | **Exit Loop:** Releases loop boundary and resumes linear playback. |
| **`TONE 1` (Turn)** | **Set Default Length:** Selects length (1/16, 1/8, 1/4, 1/2, 1, 2, 4, 8, 16, 32 beats). | **Resize Active Loop:** Dynamically doubles or halves the active loop length. |
| **`TONE 2` (Turn)** | No action | **Move Loop:** Shifts active loop boundaries forward or backward by 1 bar. |
| **`TONE 2` (Press)** | **Toggle Quantize:** Enforces beatgrid snap. | **Toggle Quantize:** Enforces beatgrid snap. |
| **`TONE 2` (Press + Turn)** | Adjusts Quantize grid resolution (1/4, 1/2, 1 beat). | Adjusts Quantize grid resolution (1/4, 1/2, 1 beat). |
| **`TONE 3` (Turn)** | **Beat Skip:** Jumps playhead forward or backward by the quantize interval. | **Beat Skip:** Shifts loop frame by the quantize interval. |

#### Procedure: Capturing, Halving, and Exiting a Quantized Loop

1. Go to the `LOOP/SKIP` page on Deck 1. The loop control parameters display.
2. Turn `TONE 1` to select `4 Beats`. The display confirms `[ 4 BEATS ]`.
3. Press `TONE 1` on the downbeat of a musical section. Playhead loops continuously across the 4-beat boundary.
4. Turn `TONE 1` counter-clockwise during playback. The loop duration halves sequentially to `2 Beats`, `1 Beat`, and `1/2 Beat`.
5. Press `TONE 1`. The loop releases, and continuous audio playback resumes in sync with the track timeline.

## External (`EXT`) Deck Specifications & Architecture

When the `EXT` Deck receives focus, audio originates from the analog rear-panel line jacks (`INPUT 1` or `INPUT 2`). Because incoming line audio is processed directly by the ARM Cortex-M7 ADC without non-volatile storage or pre-calculated beatgrid metadata:
* The `EXT` Deck supports **`EQ`**, **`TRIM/CUE`**, **`FILTER`**, and **`SYNC`** (tracking external analog clock pulses).
* The **`LOOP/SKIP`** page is omitted from the Ext Deck cycle. Real-time looping requires RAM-buffered PCM audio tables available only on digital file playback decks.

## Failure Modes & Troubleshooting

| Condition | Symptom | Root Cause | Corrective Action |
|---|---|---|---|
| Bass Swap operates on wrong decks | Turning Tone 1 affects Deck 1 and Ext Deck instead of Deck 2 | Control map bindings set to external input | Ensure Deck 1 and Deck 2 are both active and Ext Deck is bypassed in the soundcard routing matrix. |
| Loop triggers off-beat | Captured loop boundary does not align with transients | Quantize is disabled (`SNAP: OFF`) | Press `TONE 2` on the `LOOP/SKIP` page to enable Quantize (`SNAP: ON`). |
| High-resonance filter screech | High audio peaks or distortion during filter sweep | Filter resonance (`TONE 2`) set above 80% | Turn `TONE 2` counter-clockwise to reduce Q factor before sweeping cutoff frequency. |
