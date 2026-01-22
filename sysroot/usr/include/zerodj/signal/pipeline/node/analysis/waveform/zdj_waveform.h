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

#ifndef ZDJ_WAVEFORM_H
#define ZDJ_WAVEFORM_H

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>

// Live waveform buffer holds 1 sec audio plus extra for gaussian at edges
#define ZDJ_LIVE_WAVEFORM_SAMPLE_COUNT 44100+1024

// #define ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE 2048
#define ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE 256
#define ZDJ_THUMB_WAVEFORM_SAMPLE_STRIDE 30000

extern double zdj_playback_waveform_min_zoom_val;
extern double zdj_playback_waveform_max_zoom_val;

typedef enum {
    ZDJ_WAVEFORM_PLAYBACK,
    ZDJ_WAVEFORM_LIVE
} zdj_waveform_type_t;

typedef enum {
    ZDJ_WAVEFORM_SYM,
    ZDJ_WAVEFORM_TOP_HALF,
    ZDJ_WAVEFORM_BOTTOM_HALF
} zdj_waveform_style_t;

typedef struct {
    zdj_soundcard_node_t * soundcard_node;
    float * source_buf;
    float * render_buf;
    float point_count;
    float samples_per_point;
    float source_win_w;
    float source_push_head;
    float source_render_head;
    float source_push_tail;
    float source_render_tail;
    // float render_win_datum_offset;
    // zdj_sample_buf_addr_t datum_addr;
    zdj_gaussian_t * g;
    void ( *handle_soundcard_node_push )( void *, zdj_pipeline_node_t *, bool );
    void ( *render )( zdj_pipeline_node_t * );
} zdj_live_waveform_state_t;

typedef struct {
    int frame_count;
    int norm_val;
    int samples_per_point;
    char song_entity_id[ 37 ];
} zdj_waveform_header_t;

typedef enum { 
    ZDJ_WAVEFORM_MAKER_PHASE_INIT,
    ZDJ_WAVEFORM_MAKER_PHASE_PREP_WINDOW,
    ZDJ_WAVEFORM_MAKER_PHASE_WAIT_WINDOW,
    ZDJ_WAVEFORM_MAKER_PHASE_CAPTURE_WINDOW,
    ZDJ_WAVEFORM_MAKER_PHASE_BUILD_POINT
} zdj_waveform_maker_phase_t;

typedef struct {
    zdj_waveform_maker_phase_t phase;

    zdj_pipeline_node_t * decode_node;
    char song_entity_id[ 37 ];

    float sample_accum;
    int64_t sample_tally;
    int64_t point_tally;
    int64_t samples_per_point;
    float accum_norm;

    zdj_waveform_header_t * waveform_header;
    FILE * waveform_fd;

} zdj_waveform_maker_state_t;

typedef struct {
    zdj_waveform_type_t type;
    zdj_waveform_style_t style;

    uint8_t * point_buf;
    int64_t point_buf_len;

    // Keep reference values in PCM sample space
    double win_pcm_sample_head;
    double win_fwd_pcm_sample_count;
    double win_back_pcm_sample_count;
    double win_pcm_sample_count;

    // PCM sample values transformed into point-space
    double samples_per_point;
    double win_point_head;
    double win_back_point_count;
    double win_fwd_point_count;
    double win_point_count;

    // Point values transformed thru render_scale into pixel-space
    double points_per_pixel;
    double win_pixel_head;
    double win_back_pixel_count;
    double win_fwd_pixel_count;
    double win_pixel_count;

    double samples_per_pixel;

    // Playback waveform data
    zdj_waveform_header_t * waveform_header;
    FILE * waveform_fd;

    // Hi-res waveform data
    bool has_hires;
    zdj_gaussian_t * kernel;

    // Deck/control refs
    zdj_deck_t * deck;
    zdj_pipeline_node_t * audio_decode_node;
    zdj_pipeline_node_t * waveform_decode_node;

    // Renderer
    double zoom_val;
    double render_scale;
    zdj_rect_t render_frame;
    bool needs_render;
    bool needs_full_render;
    int render_new_pixels;
    void ( *render )( zdj_pipeline_node_t *, zdj_rect_t * );
} zdj_waveform_state_t;

zdj_pipeline_node_t * zdj_new_live_waveform( void );
zdj_error_type_t zdj_live_waveform_set_scale( zdj_pipeline_node_t * waveform, float scale );
zdj_error_type_t zdj_live_waveform_set_point_count( zdj_pipeline_node_t * waveform, int point_count );

zdj_pipeline_node_t * zdj_new_playback_waveform( 
    zdj_deck_t * deck,
    zdj_pipeline_node_t * decode_node,
    zdj_waveform_style_t style,
    zdj_library_song_t * song,
    double zoom_val,
    zdj_rect_t * tex_frame, // rect of texture which is created at new()
    bool hires
);
void zdj_playback_waveform_resize_window( 
    zdj_pipeline_node_t * waveform, 
    // double points_per_pixel,
    double zoom_val,
    // zdj_rect_t * frame 
    float screen_w
);

zdj_pipeline_node_t * zdj_new_thumbnail_waveform( char * filepath, int pixel_width );

zdj_pipeline_node_t * zdj_new_waveform_maker( 
    zdj_pipeline_node_t * decode_node,
    char * filepath,
    int samples_per_point,
    int hi_pass_freq
);
void zdj_close_waveform_maker( zdj_pipeline_node_t * node );

#endif