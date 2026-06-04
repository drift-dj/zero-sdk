// Emulator-only HMI input injection buffer. See zdj_emu_input.h.

#include <zerodj/system/emu/zdj_emu_input.h>

#include <pthread.h>

static pthread_mutex_t _lock = PTHREAD_MUTEX_INITIALIZER;

// Accumulated encoder deltas (consumed each apply).
static int32_t _enc[ 5 ];
// Button held states (1 = pressed), indexed by zdj_emu_btn_t.
static int32_t _btn[ 11 ];
// Absolute fader positions, indexed by zdj_emu_pot_t.
static int32_t _pot[ 3 ];

void zdj_emu_input_init( void ) {
    pthread_mutex_lock( &_lock );
    for( int i = 0; i < 5; i++ )  { _enc[ i ] = 0; }
    for( int i = 0; i < 11; i++ ) { _btn[ i ] = 0; }
    // Park faders at midpoint so nothing reads as hard-zero at startup.
    _pot[ ZDJ_EMU_POT_CH_1 ]  = ZDJ_EMU_POT_MAX / 2;
    _pot[ ZDJ_EMU_POT_CH_2 ]  = ZDJ_EMU_POT_MAX / 2;
    _pot[ ZDJ_EMU_POT_XFADE ] = ZDJ_EMU_POT_MAX / 2;
    pthread_mutex_unlock( &_lock );
}

void zdj_emu_input_encoder( zdj_emu_enc_t enc, int32_t delta ) {
    pthread_mutex_lock( &_lock );
    _enc[ enc ] += delta;
    pthread_mutex_unlock( &_lock );
}

void zdj_emu_input_button( zdj_emu_btn_t btn, bool pressed ) {
    pthread_mutex_lock( &_lock );
    _btn[ btn ] = pressed ? 1 : 0;
    pthread_mutex_unlock( &_lock );
}

void zdj_emu_input_apply( volatile zdj_hmi_m7_state_model_t * model ) {
    if( !model ) { return; }
    pthread_mutex_lock( &_lock );

    // Encoders: publish accumulated delta, then reset (matches the GPIO scan).
    model->out_state    = _enc[ ZDJ_EMU_ENC_OUT ];
    model->jog_state    = _enc[ ZDJ_EMU_ENC_JOG ];
    model->tone_1_state = _enc[ ZDJ_EMU_ENC_TONE_1 ];
    model->tone_2_state = _enc[ ZDJ_EMU_ENC_TONE_2 ];
    model->tone_3_state = _enc[ ZDJ_EMU_ENC_TONE_3 ];
    for( int i = 0; i < 5; i++ ) { _enc[ i ] = 0; }

    // Buttons: held state (1 = pressed), as the scan publishes !(gpio).
    model->out_pb_state    = _btn[ ZDJ_EMU_BTN_OUT ];
    model->jog_pb_state    = _btn[ ZDJ_EMU_BTN_JOG ];
    model->tone_1_pb_state = _btn[ ZDJ_EMU_BTN_TONE_1 ];
    model->tone_2_pb_state = _btn[ ZDJ_EMU_BTN_TONE_2 ];
    model->tone_3_pb_state = _btn[ ZDJ_EMU_BTN_TONE_3 ];
    model->hotcue_pb_state = _btn[ ZDJ_EMU_BTN_HOTCUE ];
    model->play_pb_state   = _btn[ ZDJ_EMU_BTN_PLAY ];
    model->nav_pb_state    = _btn[ ZDJ_EMU_BTN_NAV ];
    model->fn_1_pb_state   = _btn[ ZDJ_EMU_BTN_FN_1 ];
    model->fn_2_pb_state   = _btn[ ZDJ_EMU_BTN_FN_2 ];
    model->fn_3_pb_state   = _btn[ ZDJ_EMU_BTN_FN_3 ];

    // Faders: absolute (the scan leaves these to the M7).
    model->fade_1_state = _pot[ ZDJ_EMU_POT_CH_1 ];
    model->fade_2_state = _pot[ ZDJ_EMU_POT_CH_2 ];
    model->xfade_state  = _pot[ ZDJ_EMU_POT_XFADE ];

    pthread_mutex_unlock( &_lock );
}
