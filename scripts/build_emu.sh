#!/usr/bin/env bash
# Configure + build the native (host) emulator: libzerodj built for x86_64 plus
# the `zero-emu` desktop harness. Unlike scripts/build.sh this does NOT use the
# aarch64 cross toolchain -- it builds with the host compiler and links the
# host's own SDL2/sqlite3/etc. shared libraries.
#
# Host packages required (Arch): sdl2_ttf sdl2_image sdl2_gfx mhash dtc
set -e

DIR="$(cd "$(dirname "$0")" && pwd -P)"
ROOT_DIR=$(realpath "$DIR/..")
BUILD_DIR=$ROOT_DIR/build-emu

CONFIG_FLAGS="-G Ninja -S $ROOT_DIR -B $BUILD_DIR -DZDJ_EMU=ON -DCMAKE_BUILD_TYPE=Debug"

while getopts "c" opt; do
  case ${opt} in
    c ) rm -rf "$BUILD_DIR" ;;
    \? ) echo "Usage: build_emu.sh [-c]"; exit 1 ;;
  esac
done

if [ ! -d "$BUILD_DIR" ]; then
  echo "Configuring native emulator build..."
  cmake $CONFIG_FLAGS
fi

echo "Building zero-emu..."
cmake --build "$BUILD_DIR" --target zero-emu --parallel
