// zero-emu: the fake-M7 display. Owns the SDL window/renderer/texture and the
// mapping of the shared packed video buffer; unpacks and presents it when the
// harness sees the library raise update_display_req (the msg-buffer handshake
// itself stays in the harness main loop, which plays the M7).

#ifndef ZERO_EMU_VIDEO_H
#define ZERO_EMU_VIDEO_H

#include <stdbool.h>

// Create the scaled window + renderer and map the shared video buffer.
// Returns false (after logging) if the window can't be created.
bool zero_emu_video_init( int scale );

// Unpack the shared video buffer's current contents and present them.
void zero_emu_video_present( void );

// Save the last presented frame as a PNG. Returns true on success.
bool zero_emu_video_dump( const char * path );

void zero_emu_video_deinit( void );

#endif
