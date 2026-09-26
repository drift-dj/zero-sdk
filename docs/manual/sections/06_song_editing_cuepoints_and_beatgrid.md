# 6. Track Metadata, Cuepoints & Beatgrid Calibration

While Zero automatically analyzes imported audio, parameters can be inspected and manually calibrated using Station Edit Modes (`zdj_control_map.c`):
* `STATION_1_EDIT_SONG` / `STATION_2_EDIT_SONG`: Metadata inspection and manual overrides.
* `STATION_1_EDIT_CUEPOINT` / `STATION_2_EDIT_CUEPOINT`: Real-time hotcue programming and deletion.
* `STATION_1_EDIT_BEATGRID` / `STATION_2_EDIT_BEATGRID`: Transient grid anchor and interval alignment.

## 1. Cuepoint Management (`EDIT_CUEPOINT`)

```text
+=============================================================+
| [D1: CUEPOINT EDIT]   SLOT: [2/8]             TIME: 01:24.32|
| |...WWWWWW|WWWWWWw.....|  [1]>[2]             TONE 1: COMMIT|
| (JOG: SCRUB POSITION) (NAV+TONE 1: MOVE)      TONE 3: DELETE|
+=============================================================+
```

Zero supports up to 8 stored hotcue points per track. Cuepoints are stored non-volatilely in the internal SQLite database and persist across reboots.

### Procedure: Programming and Auditioning Hotcues

1. Press `PLAY` to stop playback on the deck to edit. Playback halts.
2. Long-press `JOG` on the loaded track and select `Edit Cuepoints` (or hold `NAV/SHIFT` and press `CUE`). The cuepoint editor displays.
3. Turn `JOG` in scrub mode. The playhead moves along the waveform to align with the desired transient.
4. Press `TONE 1`. The active cue slot (e.g. `[2]`) assigns to the current playhead timestamp.
5. Press and hold `CUE`. Audio plays back from the cuepoint while held, and returns to the cuepoint upon release.
6. Hold `NAV/SHIFT` and press `TONE 3` if a cuepoint requires removal. The selected cue slot clears.

## 2. Beatgrid Calibration (`EDIT_BEATGRID`)

![Beatgrid Calibration & Downbeat Anchor](assets/beatgrid_calibrate_animated.gif)

```text
+=============================================================+
| [D1: BEATGRID CALIBRATION]                    BPM: [124.00] |
| |===||====|====|====| (ANCHOR: BAR 1.1)       TONE 1: ANCHOR|
| GRID PHASE: [+0.0 ms]  SPAN: [NORMAL]         TONE 2: STRETCH|
| (JOG: FINE NUDGE)     (TONE 2 P+T: 2X//2)     TONE 3: PHASE |
+=============================================================+
```

Precise beatgrids are required for automated Sync, quantized looping, and beat skipping.

### Procedure: Calibrating a Drifting Beatgrid

1. Select `Edit Beatgrid` from the track options menu. The display zooms 400% on the waveform transients.
2. Turn `JOG` in scrub mode to locate the primary kick drum marking Bar 1, Beat 1. The playhead rests on the initial transient.
3. Press `TONE 1`. A solid vertical anchor locks to this transient, establishing the reference origin for all subsequent quarter-note ticks.
4. Turn `JOG` forward 32 bars to verify downstream alignment against later kick transients:
   * **Ticks behind transients (Grid interval too long):** Turn `TONE 2` clockwise to contract the beat interval (increasing calculated BPM).
   * **Ticks ahead of transients (Grid interval too short):** Turn `TONE 2` counter-clockwise to expand the beat interval (decreasing calculated BPM).
5. Press and turn `TONE 2` if the track was analyzed at half or double tempo (e.g. 70 BPM instead of 140 BPM). The tempo multiplier toggles between `1x`, `2x`, and `/2`.
6. Press `TONE 1`. The calibrated grid anchors and tempo write to the SQLite database.

## 3. Metadata Overrides (`EDIT_SONG`)

Within the song editor modal:
* **Key Override:** If harmonic analysis detected an ambiguous modal scale, turn `TONE 1` to override the Camelot key tag (e.g. `8A` to `8B`).
* **Gain Calibration:** Turn `TONE 2` to configure a persistent track-level gain offset. Tracks mastered at low volumes can be boosted to match modern standards upon loading.

## Failure Modes & Troubleshooting

| Condition | Symptom | Root Cause | Corrective Action |
|---|---|---|---|
| Cuepoint does not save | Configured cuepoint vanishes after restarting console | Database write transaction was interrupted | Ensure `TONE 1 (Commit)` is pressed before exiting the cuepoint editing modal. |
| Beatgrid drifts progressively | Grid aligns at start but desynchronizes toward track end | Audio file was recorded from vinyl with tempo flutter | Lock a secondary anchor later in the track, or disable automated Sync and nudge manually using `JOG`. |
| Tempo halved on drum-and-bass | Display reads 87 BPM instead of 174 BPM | Transient detector detected half-time kick intervals | Press and turn `TONE 2` in the beatgrid editor to apply the `2x` multiplier. |
