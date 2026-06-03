// Copyright (c) 2025 Drift DJ Industries

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ZDJ_PLATFORM_H
#define ZDJ_PLATFORM_H

#include <stddef.h>

// Map a shared (A53 <-> M7) memory region.
//
// On the real device this opens /dev/mem and mmaps the given physical address,
// which is exactly what every hardware-facing module used to do inline. It is
// the single boundary between zero-sdk and the M7 co-processor's shared RAM.
//
// In an emulator build (ZDJ_EMU defined) there is no /dev/mem and no M7. This
// instead hands back an ordinary process-local buffer. All callers asking for
// the same phys_addr get the *same* buffer, so the library and the host-side
// "fake M7" (the zero-emu harness) share one view of each region -- the harness
// maps the same addresses to read the video buffer, write HMI input, etc.
void * zdj_platform_map_shared( unsigned long phys_addr, size_t len );

#endif
