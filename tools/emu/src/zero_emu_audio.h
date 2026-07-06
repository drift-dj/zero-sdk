// zero-emu: the fake-M7 audio bridge.
//
// libzerodj's audio fast-cycle thread fills the shared DAC buffer once each
// time shared_audio_state->cycle_ready is asserted, advancing the deck
// transport by exactly ZDJ_SOUNDCARD_BUF_LEN frames per assertion. So the
// assert rate IS the playback clock and must match the audio device's
// consumption clock, or the stream drifts and underruns.
//
// To keep the blocking handshake out of the SDL callback (any jitter there is
// an instant glitch), a producer thread runs the M7 cycles into a ring buffer
// and the callback just drains it. The producer is paced by the ring fill
// level: it only produces while the ring is below a target depth. Because the
// callback drains at the exact device rate, that backpressure pins the
// producer's average rate -- and thus the deck's playback speed -- to 1x,
// while the target depth gives ~tens of ms of slack to absorb a slow cycle.

#ifndef ZERO_EMU_AUDIO_H
#define ZERO_EMU_AUDIO_H

#include <stdbool.h>

// Bring up the bridge: map the shared audio regions, open the host audio
// device, start the producer thread, prebuffer the ring to its target depth,
// and unpause. Call after zdj_soundcard_init (the producer asserts audio
// cycles immediately) and before playback starts. If the host device fails to
// open it logs and the bridge stays inert (active() returns false).
void zero_emu_audio_start( void );

// True once start() has opened the host audio device.
bool zero_emu_audio_active( void );

// Total frames the callback has had to zero-fill (diagnostic).
int zero_emu_audio_underruns( void );

#endif
