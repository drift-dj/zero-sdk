#!/bin/bash

cp build/libzerodj.a sysroot/usr/lib/libzerodj.a
cp -Rp src/* sysroot/usr/include/zerodj/
find sysroot/usr/include/zerodj -name "*.c" | xargs rm -r
find sysroot/usr/include/zerodj -name "*CMake*" | xargs rm -r