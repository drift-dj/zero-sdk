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
