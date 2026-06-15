#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>

double zdj_playback_waveform_min_zoom_val = 0.01;
double zdj_playback_waveform_max_zoom_val = 1000;

static void _update_wait( zdj_pipeline_node_t * node );
static void _deinit_state( zdj_pipeline_node_t * node );

static void _render( zdj_pipeline_node_t * node, zdj_rect_t * frame );
static void _render_points( zdj_pipeline_node_t * node, zdj_rect_t * frame );
static void _render_waveform( zdj_pipeline_node_t * node, zdj_rect_t * frame );

static zdj_error_type_t _move_window( zdj_pipeline_node_t * node, double offset );
static zdj_error_type_t _reset_window( zdj_pipeline_node_t * node, double address );

zdj_pipeline_node_t * zdj_new_playback_waveform( 
    zdj_deck_t * deck,
    zdj_pipeline_node_t * decode_node,
    zdj_waveform_style_t style,
    zdj_library_song_t * song,
    double zoom_val,
    zdj_rect_t * tex_frame,
    bool hires
) {
    // printf( "loading waveform: %s\n", filepath );
    char filepath[ 512 ];
    snprintf( filepath, sizeof( filepath ), "%s/%s", 
        ZDJ_LIBRARY_PLAYBACK_WAVEFORM_DIR,
        song->entity_id
    );
    if( access( filepath, F_OK ) != 0 ) { return NULL; }

    zdj_pipeline_node_t * waveform = zdj_new_pipeline_node( );
    waveform->update_wait = &_update_wait;
    waveform->deinit_state = &_deinit_state;

    waveform->move_window = &_move_window;
    waveform->reset_window = &_reset_window;

    zdj_waveform_state_t * state = calloc( 1, sizeof( zdj_waveform_state_t ) );
    waveform->state = state;
    state->style = style;
    state->render = &_render;
    state->deck = deck;
    state->audio_decode_node = decode_node;
    state->has_hires = hires;
    state->zoom_val = zoom_val;
    state->needs_render = false;
    state->needs_full_render = false;
    state->render_new_pixels = 0;

    // Install the UI buffer on the decode node
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)decode_node->state;
    decode_state->install_ui_buffer( decode_node );

    // // Stand up waveform decode node
    // int decode_buf_count = 160; // number of decode buffers which fit in window.
    // state->waveform_decode_node = zdj_new_decode_node( 
    //     song, 0, ZDJ_SOUNDCARD_BUF_LEN*decode_buf_count, ZDJ_SOUNDCARD_BUF_LEN*decode_buf_count 
    // );

    state->waveform_header = calloc( 1, sizeof( zdj_waveform_header_t ) );
    state->waveform_fd = fopen( filepath, "r" );
    fread( 
        state->waveform_header, 
        sizeof( zdj_waveform_header_t ), 
        1, 
        state->waveform_fd 
    );

    // Set up initial window params
    // ----------------------------
    // Gather coordinate-space scaling factors
    // state->points_per_pixel = points_per_pixel;
    state->points_per_pixel = state->zoom_val * (double)ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE;
    state->samples_per_point = (double)state->waveform_header->samples_per_point;

    // Set pcm/point/pixel heads based on reference pcm head * scale factor

    // Capture decode_node.head addr here
    state->win_pcm_sample_head = 0;
    state->win_point_head = state->win_pcm_sample_head * state->samples_per_point;
    state->win_pixel_head = state->win_point_head * state->points_per_pixel;

    // Fill in window sizes
    zdj_playback_waveform_resize_window( 
        waveform, 
        zoom_val, 
        tex_frame->w 
    );

    // printf( "zdj_new_playback_waveform state: %p %p\n", state, state->point_buf );
    
    return waveform;
}

static void _update_wait( zdj_pipeline_node_t * node ) {
    zdj_waveform_state_t * state = (zdj_waveform_state_t*)node->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)state->audio_decode_node->state;
    // printf( "playback_waveform _update_wait: %1.1f\n", decode_state->head.origin_d );

    // Move the point window with deck's needle head.
    double win_move = decode_state->head.origin_d - state->win_pcm_sample_head;
    if( fabs(win_move) > zdj_eps ) {
        node->move_window( node, win_move );
    }

    // // TESTING ONLY
    state->needs_render = true;

    // printf( "playback_waveform _update_wait done: %d/%d/%d\n", seek_target, point_buf_index, read_count );
}

static void _render( zdj_pipeline_node_t * node, zdj_rect_t * frame ) {
    // printf( "playback waveform _render: %1.1f\n", frame->h );
    zdj_waveform_state_t * state = (zdj_waveform_state_t*)node->state;
    
    // if( state->needs_full_render ) {
        // Clear the thingy
        boxColor( zdj_renderer( ), 0, 0, frame->w, frame->h, ZDJ_BLACK );
        if( state->zoom_val > 1.0 || !state->has_hires ) { 
            _render_points( node, frame );
        } else {
            _render_waveform( node, frame );
        }
    // } else {
    //     // Incremental render - move current pixels by offset and add new ones.
    // }

    // Request a fresh buffer copy from the fast soundcard thread
    zdj_decode_node_state_t * audio_decode_state = (zdj_decode_node_state_t*)state->audio_decode_node->state;
    audio_decode_state->ui_buffer_req = true;

    state->needs_render = false;
    state->needs_full_render = false;

    // printf( "playback waveform _render done\n" );
}

static void _render_points( zdj_pipeline_node_t * node, zdj_rect_t * frame ) {
    zdj_waveform_state_t * state = (zdj_waveform_state_t*)node->state;

    // If point head is offscreen, just show alert bar
    // printf( "win ph:%1.1f / %d\n", state->win_point_head, state->waveform_header->frame_count );
    if( state->win_point_head < -1.0 * (state->win_point_count / 2) ||
        state->win_point_head > (state->waveform_header->frame_count + (state->win_point_count / 2))
    ) { 
        SDL_Rect s = { 
            zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].x, 
            zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].y, 
            zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].w,
            zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h
        };
        SDL_Rect d = { 
            0, 0, zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].w, zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h 
        };
        switch( state->style ) {
            case ZDJ_WAVEFORM_TOP_HALF:
                d.y = frame->h - zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h;
                break;
            case ZDJ_WAVEFORM_BOTTOM_HALF:
                // d.y = 0;
                d.y = -1;
                break;
            case ZDJ_WAVEFORM_SYM:
                d.y = (frame->h/2) - 5;
                break;
        }
        SDL_RenderCopy( zdj_renderer( ), zdj_asset_atlas( ), &s, &d );
        return;
    }

    // Fill buffer using current window settings
    int point_buf_index = 0;
    int seek_target = state->win_point_head - state->win_back_point_count;
    int read_count = state->win_point_count;
    // Clip read_count to start/end of point data.
    int clip_len;
    if( seek_target < 0 ) {
        // Clip start/fill w/0s
        clip_len = seek_target * -1;
        point_buf_index = clip_len;
        read_count -= clip_len;
        memset( state->point_buf, 0, clip_len * sizeof( uint8_t ) );
        seek_target = 0;
    } else if ( seek_target+read_count > state->waveform_header->frame_count ) {
        // Clip end/fill w/0s
        clip_len = seek_target+read_count - state->waveform_header->frame_count;
        // printf( "clip_len\n" );
        read_count -= clip_len;
        memset( &state->point_buf[ (int)state->win_point_count - clip_len - 1 ], 0, clip_len * sizeof( uint8_t ) );
    }

    // Fill the point buffer from file.
    // FIXME: We are hitting the FS every frame.  This is extremely inefficient and dumb.
    fseek( state->waveform_fd, seek_target + sizeof( zdj_waveform_header_t ), SEEK_SET );
    fread( &state->point_buf[point_buf_index], sizeof( uint8_t ), read_count, state->waveform_fd );

    double point_addr = state->win_point_head;
    double point_avg_w = state->points_per_pixel / 2.0;

    // Walk backward from head drawing points to fill the window
    int i;
    bool show_lead_in = false;
    int lead_in_x;
    bool show_lead_out = false;
    int lead_out_x = 129;
    for( i=0; i<frame->w/2; i++ ){
        double point_buf_start = state->win_point_head - state->win_back_point_count;
        float val = 0.0;
        double point_index = state->win_point_head - (state->points_per_pixel * i) - point_buf_start;

        uint8_t raw_val = 0;
        for( int n=0; n<point_avg_w; n++ ) {
            int point_avg_index = point_index-n;
            if( (point_avg_index+point_buf_start) >= 0 && 
                (point_avg_index+point_buf_start) < state->waveform_header->frame_count
            ) {
                if( state->point_buf[ point_avg_index ] > raw_val ) {
                    raw_val = state->point_buf[ point_avg_index ];
                }
            }
            point_avg_index = point_index+n;
            if( (point_avg_index+point_buf_start) >= 0 && 
                (point_avg_index+point_buf_start) < state->waveform_header->frame_count
            ) {
                if( state->point_buf[ point_avg_index ] > raw_val ) {
                    raw_val = state->point_buf[ point_avg_index ];
                }
            }
        }
        val = ( (float)raw_val / (float)state->waveform_header->norm_val ) * (frame->h/2);

        
        switch( state->style ) {
            case ZDJ_WAVEFORM_TOP_HALF:
                lineColor( zdj_renderer( ), (frame->w/2)-i, frame->h, (frame->w/2)-i, frame->h-val, ZDJ_WHITE );
                break;
            case ZDJ_WAVEFORM_BOTTOM_HALF:
                // lineColor( zdj_renderer( ), (frame->w/2)-i, 0, (frame->w/2)-i, val, ZDJ_WHITE );
                lineColor( zdj_renderer( ), (frame->w/2)-i, -1, (frame->w/2)-i, val-1, ZDJ_WHITE );
                break;
            case ZDJ_WAVEFORM_SYM:
                lineColor( zdj_renderer( ), (frame->w/2)-i, frame->h/2, (frame->w/2)-i, (frame->h/2)-val, ZDJ_WHITE );
                lineColor( zdj_renderer( ), (frame->w/2)-i, frame->h/2, (frame->w/2)-i, (frame->h/2)+val, ZDJ_WHITE );
                break;
        }

        // Find lead in/out location if in window
        // if( fabs(round( point_addr )) < 2.0 ) {
        if( fabs(round( point_addr )) < ceil( state->points_per_pixel ) ) {
            show_lead_in = true;
            lead_in_x = (frame->w/2)-i;
        }
        // if( fabs( state->waveform_header->frame_count - round( point_addr )  ) < 2.0 ) {
        if( fabs( state->waveform_header->frame_count - round( point_addr )  ) < ceil( state->points_per_pixel ) ) {
            show_lead_out = true;
            lead_out_x = (frame->w/2)-i;
        }

        point_addr -= state->points_per_pixel;
    }

    // Walk forward from head drawing points to fill the window
    point_addr = state->win_point_head;
    for( i=0; i<frame->w/2; i++ ){
        double point_buf_start = state->win_point_head - state->win_back_point_count;
        float val = 0.0;
        double point_index = state->win_point_head + (state->points_per_pixel * i) - point_buf_start;
        
        uint8_t raw_val = 0;
        for( int n=0; n<point_avg_w; n++ ) {
            int point_avg_index = point_index-n;
            if( (point_avg_index+point_buf_start) >= 0 && 
                (point_avg_index+point_buf_start) < state->waveform_header->frame_count
            ) {
                if( state->point_buf[ point_avg_index ] > raw_val ) {
                    raw_val = state->point_buf[ point_avg_index ];
                }
            }
            point_avg_index = point_index+n;
            if( (point_avg_index+point_buf_start) >= 0 && 
                (point_avg_index+point_buf_start) < state->waveform_header->frame_count
            ) {
                if( state->point_buf[ point_avg_index ] > raw_val ) {
                    raw_val = state->point_buf[ point_avg_index ];
                }
            }
        }
        val = ( (float)raw_val / (float)state->waveform_header->norm_val) * (frame->h/2);

        switch( state->style ) {
            case ZDJ_WAVEFORM_TOP_HALF:
                lineColor( zdj_renderer( ), (frame->w/2)+i, frame->h, (frame->w/2)+i, frame->h-val, ZDJ_WHITE );
                break;
            case ZDJ_WAVEFORM_BOTTOM_HALF:
                // lineColor( zdj_renderer( ), (frame->w/2)+i, 0, (frame->w/2)+i, val, ZDJ_WHITE );
                lineColor( zdj_renderer( ), (frame->w/2)+i, -1, (frame->w/2)+i, val-1, ZDJ_WHITE );
                break;
            case ZDJ_WAVEFORM_SYM:
                lineColor( zdj_renderer( ), (frame->w/2)+i, frame->h/2, (frame->w/2)+i, (frame->h/2)-val, ZDJ_WHITE );
                lineColor( zdj_renderer( ), (frame->w/2)+i, frame->h/2, (frame->w/2)+i, (frame->h/2)+val, ZDJ_WHITE );
                break;
        }

        // Find lead in/out location if in window
        // if( fabs( round( point_addr ) ) < 2.0 ) {
        if( fabs( round( point_addr ) ) < ceil(state->points_per_pixel) ) {
            show_lead_in = true;
            lead_in_x = (frame->w/2)+i;
        }
        // if( fabs( state->waveform_header->frame_count - round( point_addr )  ) < 2.0 ) {
        if( fabs( state->waveform_header->frame_count - round( point_addr )  ) < ceil(state->points_per_pixel) ) {
            show_lead_out = true;
            lead_out_x = (frame->w/2)+i;
        }

        point_addr += state->points_per_pixel;
    }

    // Draw the lead in alert strip
    if( show_lead_in ) { 
        SDL_Rect s = { 
            zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].w - lead_in_x, 
            zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].y, 
            lead_in_x, 
            zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h
        };
        SDL_Rect d = { 
            0, 0, lead_in_x, zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h 
        };
        switch( state->style ) {
            case ZDJ_WAVEFORM_TOP_HALF:
                d.y = frame->h - zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h;
                break;
            case ZDJ_WAVEFORM_BOTTOM_HALF:
                d.y = -1;
                break;
            case ZDJ_WAVEFORM_SYM:
                d.y = (frame->h/2) - 5;
                break;
        }
        SDL_RenderCopy( zdj_renderer( ), zdj_asset_atlas( ), &s, &d );
    }

    // Draw the lead out alert strip
    if( lead_out_x < frame->w ) { 
        SDL_Rect s = { 
            0, 
            zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].y, 
            frame->w - lead_out_x, 
            zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h
        };
        SDL_Rect d = { 
            lead_out_x, 0, frame->w - lead_out_x, zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h 
        };
        switch( state->style ) {
            case ZDJ_WAVEFORM_TOP_HALF:
                d.y = frame->h - zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h;
                break;
            case ZDJ_WAVEFORM_BOTTOM_HALF:
                d.y = -1;
                break;
            case ZDJ_WAVEFORM_SYM:
                d.y = (frame->h/2) - 5;
                break;
        }
        SDL_RenderCopy( zdj_renderer( ), zdj_asset_atlas( ), &s, &d );
    }    
}

static void _render_waveform( zdj_pipeline_node_t * node, zdj_rect_t * frame ) {
    zdj_waveform_state_t * state = (zdj_waveform_state_t*)node->state;
    zdj_decode_node_state_t * audio_decode_state = (zdj_decode_node_state_t*)state->audio_decode_node->state;
    // zdj_decode_node_state_t * waveform_decode_state = (zdj_decode_node_state_t*)state->waveform_decode_node->state;
    // zdj_decode_node_state_t * waveform_decode_state = audio_decode_state;
    double head_coord = state->win_pcm_sample_head;

    // double head_move_val = waveform_decode_state->head.transport_d - audio_decode_state->head.transport_d;

    

    // printf( "---\n" );
    // double sample_avg_w = state->samples_per_pixel / 4.0;
    double sample_avg_w = state->samples_per_pixel / 2.0;
    double sample_avg_weight = 1.0 / sample_avg_w;
    // printf( "avg_w = %1.3f\n", sample_avg_w );

    // printf( "waveform h buf:%1.3f pcm_h:%1.3f\n", decode_state->head.buf_d, state->win_pcm_sample_head );
    //
    // Loop thru decode window, capturing points on the decoded waveform
    // Build window start/end coords based on zoom level
    // Hard part is to seemlessly transition from start/end of points-based
    // renderer to start/end of decode window-based renderer.
    // Also performance.

    // Walk backward from head drawing points to fill the window
    int i;
    int lead_in_x = -1;
    int lead_out_x = 129;
    for( i=0; i<frame->w/2; i++ ){
        // printf( "hc: %1.3f\n", head_coord );
        if( round(head_coord) >= 0 ) {
            // Build reference into decode node buffer
            int buf_sample = round( audio_decode_state->head.buf_d - (state->samples_per_pixel * i) );
            if( buf_sample > 0 && buf_sample < audio_decode_state->win_sample_count ) { 
                float buf_min = 10.0;
                float buf_max = -10.0;
                for( int n=0; n<sample_avg_w; n++ ) {
                    int buf_index = (buf_sample-n) * audio_decode_state->channel_count;
                    if( buf_index >= 0 && 
                        buf_index < audio_decode_state->win_sample_count*audio_decode_state->channel_count 
                    ) {
                        buf_max = fmax( buf_max, audio_decode_state->ui_buffer[ buf_index ] );
                        buf_min = fmin( buf_min, audio_decode_state->ui_buffer[ buf_index ] );
                    }
                    buf_index = (buf_sample+n) * audio_decode_state->channel_count;
                    if( buf_index >= 0 && 
                        buf_index < audio_decode_state->win_sample_count*audio_decode_state->channel_count 
                    ) {
                        buf_max = fmax( buf_max, audio_decode_state->ui_buffer[ buf_index ] );
                        buf_min = fmin( buf_min, audio_decode_state->ui_buffer[ buf_index ] );
                    }
                }

                lineColor( 
                    zdj_renderer( ), 
                    (frame->w/2)-i, 
                    (frame->h/2)+(buf_max * 12), 
                    (frame->w/2)-i, 
                    (frame->h/2)+(buf_min * 12), 
                    ZDJ_WHITE 
                );
            }
        } else {
            // Capture the lead in pixel index if the start of song is in frame
            lead_in_x = fmax( lead_in_x, (frame->w/2)-i );
        }
        head_coord -= state->samples_per_pixel;
    }

    // Walk forward from head drawing points to fill the window
    head_coord = state->win_pcm_sample_head;
    for( i=0; i<frame->w/2; i++ ){
        if( round(head_coord) <= audio_decode_state->song_pcm_duration ) {
            // Build reference into decode node buffer
            int buf_sample = round( audio_decode_state->head.buf_d + (state->samples_per_pixel * i) );
            // printf( "h:%1.0f s:%d\n", decode_state->head.buf_d, buf_sample );
            if( buf_sample > 0 && buf_sample < audio_decode_state->win_sample_count ) { 
                float buf_min = 10.0;
                float buf_max = -10.0;
                for( int n=0; n<sample_avg_w; n++ ) {
                    int buf_index = (buf_sample-n) * audio_decode_state->channel_count;
                    if( buf_index >= 0 && 
                        buf_index < audio_decode_state->win_sample_count * audio_decode_state->channel_count
                    ) {
                        // buf_max = fmax( buf_max, audio_decode_state->out_buffer[ buf_index ] );
                        // buf_min = fmin( buf_min, audio_decode_state->out_buffer[ buf_index ] );
                        buf_max = fmax( buf_max, audio_decode_state->ui_buffer[ buf_index ] );
                        buf_min = fmin( buf_min, audio_decode_state->ui_buffer[ buf_index ] );
                    }
                    buf_index = (buf_sample+n) * audio_decode_state->channel_count;
                    if( buf_index >= 0 && 
                        buf_index < audio_decode_state->win_sample_count * audio_decode_state->channel_count 
                    ) {
                        buf_max = fmax( buf_max, audio_decode_state->ui_buffer[ buf_index ] );
                        buf_min = fmin( buf_min, audio_decode_state->ui_buffer[ buf_index ] );
                    }
                }
                lineColor( 
                    zdj_renderer( ), 
                    (frame->w/2)+i, 
                    (frame->h/2)+(buf_max * 12), 
                    (frame->w/2)+i, 
                    (frame->h/2)+(buf_min * 12), 
                    ZDJ_WHITE 
                );
            }
        } else {
            // Capture the lead in pixel index if the start of song is in frame
            lead_out_x = fmin( lead_out_x, (frame->w/2)+i );
        }
        head_coord += state->samples_per_pixel;
    }

    // Draw the lead in alert strip
    if( lead_in_x > 0 ) { 
        SDL_Rect s = { 
            zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].w - lead_in_x, 
            zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].y, 
            lead_in_x, 
            zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h
        };
        SDL_Rect d = { 
            0, 0, lead_in_x, zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h 
        };
        switch( state->style ) {
            case ZDJ_WAVEFORM_TOP_HALF:
                d.y = frame->h - zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h;
                break;
            case ZDJ_WAVEFORM_BOTTOM_HALF:
                d.y = 0;
                break;
            case ZDJ_WAVEFORM_SYM:
                d.y = (frame->h/2) - 5;
                break;
        }
        SDL_RenderCopy( zdj_renderer( ), zdj_asset_atlas( ), &s, &d );
    }

    // Draw the lead out alert strip
    if( lead_out_x < frame->w ) { 
        SDL_Rect s = { 
            0, 
            zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].y, 
            frame->w - lead_out_x, 
            zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h
        };
        SDL_Rect d = { 
            lead_out_x, 0, frame->w - lead_out_x, zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h 
        };
        switch( state->style ) {
            case ZDJ_WAVEFORM_TOP_HALF:
                d.y = frame->h - zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ].h;
                break;
            case ZDJ_WAVEFORM_BOTTOM_HALF:
                d.y = 0;
                break;
            case ZDJ_WAVEFORM_SYM:
                d.y = (frame->h/2) - 5;
                break;
        }
        SDL_RenderCopy( zdj_renderer( ), zdj_asset_atlas( ), &s, &d );
    }

    // printf( "_render_waveform done\n" );
}

// NOTE: window move/reset op addresses are in song PCM space, NOT waveform point space.
static zdj_error_type_t _move_window( zdj_pipeline_node_t * node, double offset ) {
    zdj_waveform_state_t * state = (zdj_waveform_state_t*)node->state;

    state->win_pcm_sample_head += offset;

    // Convert pcm sample coord to point coord
    state->win_point_head = state->win_pcm_sample_head / state->samples_per_point;

    // Convert point coord to pixel coord
    state->points_per_pixel = (state->zoom_val * (double)ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE) / (double)ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE;
    state->samples_per_point = (double)state->waveform_header->samples_per_point;

    // Set pixel window dimensions
    // state->win_back_pixel_count = view->frame->w / 2;
    // state->win_fwd_pixel_count = view->frame->w / 2;
    // state->win_pixel_count = state->win_back_pixel_count + state->win_fwd_pixel_count;

    // Set point window dimensions
    state->win_back_point_count = state->win_back_pixel_count * state->points_per_pixel;
    state->win_fwd_point_count = state->win_fwd_pixel_count * state->points_per_pixel;
    state->win_point_count = state->win_back_point_count + state->win_fwd_point_count;

    double new_win_pixel_head = round( state->win_point_head / state->points_per_pixel );

    // Move pixel window if needed
    if( (int)new_win_pixel_head != (int)state->win_pixel_head ) {
        // printf( "move pixel head by: %d/%1.0f point: %1.1f, %1.1f\n", 
        //     (int)new_win_pixel_head - (int)state->win_pixel_head,
        //     state->samples_per_point,
        //     state->win_point_head,
        //     state->win_pcm_sample_head
        // );
        state->render_new_pixels = (int)new_win_pixel_head - (int)state->win_pixel_head;
        state->win_pixel_head = new_win_pixel_head;
        state->needs_render = true;
        state->needs_full_render = true;
    } 
    // printf( "waveform point hd: %1.1f\n", state->win_point_head );
}

static zdj_error_type_t _reset_window( zdj_pipeline_node_t * node, double address ) {

}

void zdj_playback_waveform_resize_window( 
    zdj_pipeline_node_t * waveform, 
    double zoom_val,
    float screen_w
) {
    // printf( "zdj_playback_waveform_resize_window: %1.1f\n", zoom_val );
    zdj_waveform_state_t * state = (zdj_waveform_state_t*)waveform->state;

    if( !state ) { return; }

    state->zoom_val = zoom_val;
    state->points_per_pixel = (state->zoom_val * (double)ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE) / (double)ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE;
    state->samples_per_point = (double)state->waveform_header->samples_per_point;

    state->samples_per_pixel = state->points_per_pixel * state->samples_per_point;

    // Set pixel window dimensions
    state->win_back_pixel_count = round(screen_w / 2.0);
    state->win_fwd_pixel_count = round(screen_w / 2.0);
    state->win_pixel_count = state->win_back_pixel_count + state->win_fwd_pixel_count;

    // Set point window dimensions
    state->win_back_point_count = state->win_back_pixel_count * state->points_per_pixel;
    state->win_fwd_point_count = state->win_fwd_pixel_count * state->points_per_pixel;
    state->win_point_count = state->win_back_point_count + state->win_fwd_point_count;

    // Re-alloc point buffer
    if( state->point_buf ){ free( state->point_buf ); }
    state->point_buf = calloc( state->win_point_count, sizeof( uint8_t ) );

    // Re-render the entire waveform window
    state->needs_render = true;
    state->needs_full_render = true;
    // printf( "zdj_playback_waveform_resize_window done\n" );
}

static void _deinit_state( zdj_pipeline_node_t * node ) {
    // printf( "playback waveform _deinit_state\n" );
    zdj_waveform_state_t * state = (zdj_waveform_state_t*)node->state;
    if( state->point_buf ){ free( state->point_buf ); }
    if( state->waveform_fd ){ fclose( state->waveform_fd ); }
    if( state->waveform_header ){ free( state->waveform_header ); }
}