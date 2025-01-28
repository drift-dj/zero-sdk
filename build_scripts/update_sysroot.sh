#!/bin/bash

cp -Rp src/* sysroot/usr/include/aarch64-linux-gnu/zerodj/
find sysroot/usr/include/aarch64-linux-gnu/zerodj -name "*.c" | xargs rm -r
find sysroot/usr/include/aarch64-linux-gnu/zerodj -name "*CMake*" | xargs rm -r