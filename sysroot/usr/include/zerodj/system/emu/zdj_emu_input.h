// Copyright (c) 2025 Drift DJ Industries
//
// Emulator-only: the host-side input injection buffer. On the device the HMI is
// read by a GPIO quadrature scan (zdj_hmi_scan_thread.c). In an emulator build
// that scan is compiled out and replaced by zdj_emu_input_apply(), which copies
// whatever the zero-emu harness has injected here into the shared HMI model.
//
// Only built when ZDJ_EMU is defined.

#ifndef ZDJ_EMU_INPUT_H
#define ZDJ_EMU_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#include <zerodj/controls/hmi/zdj_hmi_m7_state_model.h>

// Rotary encoders (with integrated push switches handled as buttons below).
typedef enum {
    ZDJ_EMU_ENC_OUT,     // output volume encoder
    ZDJ_EMU_ENC_JOG,     // jog / nav wheel
    ZDJ_EMU_ENC_TONE_1,
    ZDJ_EMU_ENC_TONE_2,
    ZDJ_EMU_ENC_TONE_3,
} zdj_emu_enc_t;

// Pushbuttons, including the encoder push switches.
typedef enum {
    ZDJ_EMU_BTN_OUT,
    ZDJ_EMU_BTN_JOG,
    ZDJ_EMU_BTN_TONE_1,
    ZDJ_EMU_BTN_TONE_2,
    ZDJ_EMU_BTN_TONE_3,
    ZDJ_EMU_BTN_HOTCUE,
    ZDJ_EMU_BTN_PLAY,
    ZDJ_EMU_BTN_NAV,
    ZDJ_EMU_BTN_FN_1,
    ZDJ_EMU_BTN_FN_2,
    ZDJ_EMU_BTN_FN_3,
} zdj_emu_btn_t;

// Analog faders / crossfader (absolute position).
typedef enum {
    ZDJ_EMU_POT_CH_1,
    ZDJ_EMU_POT_CH_2,
    ZDJ_EMU_POT_XFADE,
} zdj_emu_pot_t;

// Fader range published to the HMI analog state machine. The device ADC range
// is hardware-defined; the SM only cares about relative motion, so any stable
// range works -- we use a 12-bit span.
#define ZDJ_EMU_POT_MAX 4095

// Initialise the injection buffer (called by zdj_control_prepare_hmi_input_scan
// under ZDJ_EMU). Safe to call more than once.
void zdj_emu_input_init( void );

// Harness -> injection buffer. Thread-safe; called from the harness/SDL thread.
void zdj_emu_input_encoder( zdj_emu_enc_t enc, int32_t delta ); // accumulates
void zdj_emu_input_button( zdj_emu_btn_t btn, bool pressed );   // held state
void zdj_emu_input_pot( zdj_emu_pot_t pot, int32_t value );     // absolute 0..MAX

// Injection buffer -> shared model. Called by the control cycle thread each
// scan; consumes (zeroes) the accumulated encoder deltas, mirroring the
// hardware scan's upval->model->reset behaviour.
void zdj_emu_input_apply( volatile zdj_hmi_m7_state_model_t * model );

#endif
