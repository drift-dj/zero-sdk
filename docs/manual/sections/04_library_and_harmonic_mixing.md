# 4. Library Navigation & Harmonic Mixing Engine

The Library Menu provides hierarchical browsing across internal solid-state storage and mounted external USB drives. It incorporates real-time harmonic compatibility evaluation based on the Camelot wheel.

## Library Navigation Architecture

* **Open / Close Library:** Press `NAV/SHIFT` to toggle the Library Menu overlay.
* **Scroll & Browse:** Turn `JOG` to navigate lists vertically.
* **Load Track:** Press `JOG` on any track to load audio, waveform, beatgrid, and cuepoint data into the focused deck.
* **Back Navigation:** Scroll to the list top and press `JOG` on `[ BACK ]`, or press `NAV/SHIFT` to return directly to the performance screen.

![Library Harmonic Compatibility Navigation](assets/harmonic_library_animated.gif)

```text
+=============================================================+
| [LIBRARY: ALL SONGS]   (SORT: KEY)            TARGET: [D2]  |
| [ < BACK TO ROOT ]                                          |
| 01. Solar Fields - Sol          124.0 BPM  [=] 11B (PERFECT)|
|>02. Tycho - Awake (Edit)        124.0 BPM  [+] 12B (LIFT)   |
| 03. Jon Hopkins - Open Eye      124.0 BPM  [~] 11A (MODE)   |
| 04. Bicep - Glue                124.0 BPM  [Δ]  8A (MOOD)   |
| 05. Floating Points - LesAlp    128.0 BPM  [-] 10B (DROP)   |
+=============================================================+
```

### Row Annotations & Metadata Flags
Row indicators can be enabled or disabled under `Control Panels > Settings > UI Settings > Library Menu`:
* **BPM Tag:** Analyzed tempo (e.g. `124.0`).
* **Key Tag:** Musical key notation (e.g. `11B` or `A maj`).
* **Harmonic Match Indicator:** Real-time compatibility glyph evaluated against the currently playing audio deck.
* **Error Tag (`[!Err]`):** Flags corrupt headers or unsupported audio encodings.

## Procedure: Loading Tracks by Harmonic Compatibility

1. Press `DECK 2`. The header displays `TARGET: [D2]`.
2. Press `NAV/SHIFT`. The Library Menu displays.
3. Turn `JOG` to highlight `Songs`. The root song catalog displays.
4. Turn `JOG` to inspect candidate tracks. Compare the bracketed compatibility glyphs against the lead track:
   * Select `[=]` for identical key signature and zero harmonic dissonance.
   * Select `[+]` or `[++]` for upward energy transitions.
   * Select `[-]` or `[--]` for downward energy transitions.
   * Select `[~]` for relative Major/Minor mode transitions.
   * Select `[Δ]` for parallel key transitions.
5. Press `JOG` on the selected track. The track loads into Deck 2, and the display returns to DJ Playback.

## Harmonic Mixing Engine & Camelot Lookup Table

Zero evaluates musical harmony using an embedded 26x26 key lookup table (`_zdj_library_camelot_lut` in `zdj_library_api_key.c`). The engine compares the candidate song's key against the deck currently active on the master mix bus.

The Camelot system numbers the Circle of Fifths from `1` to `12`:
* **`B`** denotes Major scales (e.g. `8B` = C Major, `11B` = A Major).
* **`A`** denotes Minor scales (e.g. `5A` = C Minor, `8A` = A Minor).

```text
                     12B (E) / 12A (Dbm)
               11B (A)                 1B (B)
             11A (F#m)                 1A (Abm)
         10B (D)                             2B (F#)
         10A (Bm)                            2A (Ebm)
        9B (G)                                  3B (Db)
        9A (Em)                                 3A (Bbm)
         8B (C)                              4B (Ab)
         8A (Am)                             4A (Fm)
               7B (F)                  5B (Eb)
               7A (Dm)                 5A (Cm)
                      6B (Bb) / 6A (Gm)
```

## Real-Time Compatibility Glyphs

The song list renders distinct glyphs (`zdj_menu_item_view_song.c`) reflecting harmonic intervals:

| Glyph | Internal Enum | Musical Relationship | Harmonic Function |
|:---:|---|---|---|
| **`=`** | `ZDJ_LIBRARY_CAMELOT_EQUAL` | **Exact Key Match** (e.g. `11B` to `11B`) | Zero harmonic tension. Melodic and bass elements mix without dissonance. |
| **`+`** | `ZDJ_LIBRARY_CAMELOT_PLUS` | **Step Boost (+1)** (e.g. `11B` to `12B`) | One step clockwise on Circle of Fifths. Perceived energy elevation. |
| **`++`** | `ZDJ_LIBRARY_CAMELOT_PLUS_PLUS` | **Energy Surge (+2)** (e.g. `11B` to `1B`) | Two steps clockwise. Sharp key modulation for peak transitions. |
| **`-`** | `ZDJ_LIBRARY_CAMELOT_MINUS` | **Step Drop (-1)** (e.g. `11B` to `10B`) | One step counter-clockwise. Harmonically releases tension. |
| **`--`** | `ZDJ_LIBRARY_CAMELOT_MINUS_MINUS` | **Deep Drop (-2)** (e.g. `11B` to `9B`) | Two steps counter-clockwise. Melodic downshift. |
| **`~`** | `ZDJ_LIBRARY_CAMELOT_TILDA` | **Relative Mode Swap** (e.g. `11B` to `11A`) | Uses identical scale pitches with a shifted tonal center (Major to Minor). |
| **`Δ`** | `ZDJ_LIBRARY_CAMELOT_DELTA` | **Parallel Root Modulation** (e.g. `11B` [A Maj] to `8A` [A Min]) | Root pitch remains unchanged while scale tonality shifts between Major and Minor. |
| *(None)* | `ZDJ_LIBRARY_CAMELOT_NONE` | **Incompatible / Unmatched** | Dissonant scale intervals. No badge is rendered. |

> [!NOTE]
> Early documentation drafts erroneously cited a `"???"` wildcard indicator. In production firmware, non-matching musical keys leave the compatibility badge area blank.

## Failure Modes & Troubleshooting

| Condition | Symptom | Root Cause | Corrective Action |
|---|---|---|---|
| Track loads into wrong deck | Audio plays on active deck instead of cue deck | Target focus was set to playing deck before load | Check `TARGET: [D1/D2]` in the top-right corner of the library before pressing `JOG`. Press `DECK 1` or `DECK 2` to change destination. |
| Key tag displays `---` | No key or compatibility glyph is shown | Track analysis incomplete or audio file has insufficient harmonic content | Let background analysis complete, or enter `Station Edit Mode` to manually assign key metadata. |
| Red `[!Err]` tag on song row | Track refuses to load when selected | File missing from disk, corrupted audio header, or unreadable storage sector | Run `File System Check` in Settings, or re-import the file over USB. |
