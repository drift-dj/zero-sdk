#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd -P)"
ROOT_DIR=$(realpath $DIR/..)
BUILD_DIR=$ROOT_DIR/build
SYSROOT_DIR=$ROOT_DIR/sysroot

# Copy the source tree down into the sysroot include dir.
# Remove anything which isn't a header.

cp -Rp src/* $SYSROOT_DIR/usr/include/zerodj/ || exit 1
find $SYSROOT_DIR/usr/include/zerodj -name "*.c" | xargs rm -r || exit 1
find $SYSROOT_DIR/usr/include/zerodj -name "*CMake*" | xargs rm -r || exit 1
cp $BUILD_DIR/libzerodj.a $SYSROOT_DIR/usr/lib/libzerodj.a || exit 1