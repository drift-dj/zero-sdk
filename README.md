# zero-sdk

`libzerodj` — the C SDK for the Drift Zero DJ hardware. Cross-compiles from a development host to `aarch64-linux-gnu` and is linked into `drift-os`.

## Prerequisites

- `clang` and `lld` (the toolchain forces `-fuse-ld=lld`)
- `cmake` ≥ 3.23
- `ninja`

No aarch64 system libraries are needed on the host — everything resolves against the in-tree `sysroot/`.

## Build

From the repo root:

```sh
./scripts/build.sh
```

This configures CMake with `cmake/toolchain.cmake` (Ninja, Release) into `./build/` and produces `build/libzerodj.a`. The first invocation configures; subsequent invocations only build.

Flags:

- `./scripts/build.sh -c` — clean rebuild (`--clean-first`).
- Override the compiler with `-DCUSTOM_CLANG_PATH=/path/to/clang` when invoking CMake directly.

## Install

After building, publish headers and the static library into the sysroot for downstream consumers:

```sh
./scripts/update_sysroot.sh
```

Copy runtime resources (`zero_atlas-32bit.bmp`, `device.db`) into a `drift-os` tree:

```sh
./scripts/install.sh <path-to-drift-os/resources/zero-externals>
```

Push resources to a connected device over zmodem:

```sh
./scripts/sz.sh <serial-tty>
```

## Emulator

`zero-emu` is a desktop harness that runs `libzerodj` natively on the host, without a device. It stands in for both `drift-os` (it brings up the library and runs the frame loop) and the M7 co-processor (it presents the video buffer in an SDL window, bridges the DAC to the host audio device, and turns keyboard input into HMI state).

The library is built for the host rather than cross-compiled, and the device's hardware access is swapped for an emulated backend under `ZDJ_EMU`. The harness itself lives in `tools/emu/src/`.

### Prerequisites

Host packages (Arch names):

- `cmake`, `ninja` and a host C compiler
- `sdl2` (or `sdl2-compat`), `sdl2_ttf`, `sdl2_image`, `sdl2_gfx`
- `sqlite`, `libxml2`, `util-linux-libs`, `mpg123`, `mhash`, `rubberband`, `dtc`, `alsa-lib`
- `ffmpeg4.4`: the decode nodes are written against the ffmpeg 4.4 API, so the build links `/usr/lib/ffmpeg4.4` rather than the system ffmpeg
- `bubblewrap`, used by the run script

### Build

```sh
./scripts/build_emu.sh
```

This configures CMake with `-DZDJ_EMU=ON` (Ninja, Debug) into `./build-emu/` and builds the `zero-emu` target. It uses the host toolchain, not `cmake/toolchain.cmake`.

- `./scripts/build_emu.sh -c` deletes `build-emu/` and reconfigures from scratch. That also deletes the emulator's saved state in `build-emu/media-internal/`.
- The configure step only runs when `build-emu/` doesn't exist. If the repo has moved since it was configured, the build fails while regenerating `build.ninja`. Reconfigure in place, which keeps `media-internal/`:

  ```sh
  cmake --fresh -G Ninja -S . -B build-emu -DZDJ_EMU=ON -DCMAKE_BUILD_TYPE=Debug
  ```
- Like the device build, the emulator reads `zerodj/*` headers from `sysroot/usr/include/zerodj/` (through the `tools/emu/include/zerodj` symlink), not from `src/`. Header edits need to reach the sysroot before the emulator sees them.

### Run

```sh
./scripts/run_emu.sh
```

The library expects device paths such as `/root/res` and `/media/internal`, so the run script starts `zero-emu` inside a bubblewrap sandbox that maps them onto the repo:

| Device path | Host path |
|---|---|
| `/root/res` | `build-emu/emu-root/res/` (the atlas and fonts, staged on each run) |
| `/media/internal` | `build-emu/media-internal/` (writable; holds the settings, soundcard and device DBs) |

Inside the sandbox, `$HOME` is read-only, the repo is writable, and `/tmp` is a private empty directory. Point any file paths you pass in (tracks, frame dumps) at somewhere under `$HOME` or the repo accordingly.

#### Fonts

The device's pixel fonts (`pixelated.ttf`, `pixelsix14.ttf`, `lo-res09-nar.ttf`) aren't redistributable and aren't in this repo. The run script copies them from `~/.local/share/fonts` when they're there. For any that are missing, it falls back to a substitute monospace font from the host, so the UI still comes up with the wrong metrics.

#### Stale databases

The saved DBs encode the soundcard node layout that is compiled into the binary. A DB written by an older build can decode wrong under a newer layout; for example, the audio mix can recurse forever and crash once a track plays. The run script deletes any `*.db` in `build-emu/media-internal/.system/` that is older than the `zero-emu` binary, and the library regenerates them from presets. If you still hit a crash in the mix path after pulling schema changes, delete those DBs by hand.

### Environment variables

| Variable | Default | Effect |
|---|---|---|
| `ZERO_EMU_SCALE` | `6` | Window scale factor for the 128×64 display |
| `ZERO_EMU_HZ` | `30` | Frame loop rate |
| `ZERO_EMU_FULL_UI` | `1` | `0` brings up a minimal UI with no panels, to check the render path alone |
| `ZERO_EMU_TRACK` | unset | Audio file to load onto deck 1 and play through the host audio device (full UI only) |
| `ZERO_EMU_DUMP` | unset | Save a PNG of one frame to this path |
| `ZERO_EMU_DUMP_FRAME` | `90` | Which frame `ZERO_EMU_DUMP` captures, so animations settle first |
| `ZERO_EMU_AUTOKEY` | `0` | `1` taps FN3 (next panel) every 40 frames, cycling through panels without input |
| `ZERO_EMU_FONTS_DIR` | `~/.local/share/fonts` | Where to look for the device fonts |
| `ZERO_EMU_FONT` | first match on the host | Substitute font for any missing device font |

For example:

```sh
ZERO_EMU_TRACK=~/Music/track.mp3 ./scripts/run_emu.sh
```

### Keys

Each encoder keypress sends one detent, and key repeat keeps stepping.

| Key | Control |
|---|---|
| Left / Right | Jog wheel |
| Enter | Jog press (select) |
| Page Down / Page Up | Output encoder |
| `o` | Output encoder press |
| Esc | NAV (back) |
| Space | PLAY |
| `h` | HOTCUE |
| `1` / `2` / `3` | FN1 / FN2 / FN3 |
| `q`/`a`, `w`/`s`, `e`/`d` | Tone 1 / 2 / 3 encoder (up / down) |
| Tab | Deploy or retract the current panel |
| `p` | Deck 1 play/pause (with `ZERO_EMU_TRACK`) |
| Shift+Esc | Quit |

The emulator also prints this keymap when it starts.
