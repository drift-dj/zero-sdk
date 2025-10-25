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
double zdj_playback_waveform_max_zoom_val = 60;

static void _update_wait( zdj_pipeline_node_t * node );
static void _deinit_state( zdj_pipeline_node_t * node );

static void _render( zdj_pipeline_node_t * node, zdj_rect_t * frame );

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
    state->decode_node = decode_node;
    state->has_hires = hires;
    state->zoom_val = zoom_val;
    state->needs_render = false;
    state->needs_full_render = false;
    state->render_new_pixels = 0;

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

    state->win_pcm_sample_head = deck->controls.platter.needle.head;
    state->win_point_head = state->win_pcm_sample_head * state->samples_per_point;
    state->win_pixel_head = state->win_point_head * state->points_per_pixel;

    // Fill in window sizes
    zdj_playback_waveform_resize_window( 
        waveform, 
        zoom_val, 
        tex_frame->w 
    );
    
    return waveform;
}

static void _update_wait( zdj_pipeline_node_t * node ) {
    zdj_waveform_state_t * state = (zdj_waveform_state_t*)node->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)state->decode_node->state;
    // printf( "playback_waveform _update_wait\n" );

    // // Move the point window with deck's needle head.
    // if( decode_state->first_layer ) {
    //     zdj_decode_layer_t * layer = zdj_decode_get_layer_under_head( state->decode_node );
    //     zdj_decode_packet_t * packet = zdj_decode_get_packet_under_head( state->decode_node, layer );
    //     if( layer && packet ) {
    //         // Take offset from decode head to packet decode start addr.
    //         int64_t head_offset = decode_state->head_decode_addr - packet->packet_decode_addr;
    //         // Offset the packet's pcm start addr to get head pcm addr.
    //         int64_t head_pcm_addr = packet->packet_pcm_addr + head_offset;
    //         // Move the window
    //         double win_move = (double)head_pcm_addr - state->win_pcm_sample_head;
    //         node->move_window( node, round( win_move ) );
    //     }
    // } else {
    //     // Bug out early if we catch the thread with no layers
    //     return;
    // }

    // Move the point window with deck's needle head.
    if( decode_state->first_layer ) {
        double win_move = decode_state->head.origin_d - state->win_pcm_sample_head;
        node->move_window( node, round( win_move ) );
    } else {
        // Bug out early if we catch the thread with no layers
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
        read_count -= clip_len;
        memset( &state->point_buf[ (int)state->win_point_count - clip_len - 1 ], 0, clip_len * sizeof( uint8_t ) );
    }

    // printf( "window: [ %1.2f/%1.2f(%d) - %1.2f/%1.2f/%1.1f - %1.2f/%1.2f(%d) ]\n",
    //     state->win_pixel_head - state->win_back_pixel_count,
    //     state->win_point_head - state->win_back_point_count,
    //     seek_target,

    //     state->win_pixel_head,
    //     state->win_point_head,
    //     state->win_pcm_sample_head,

    //     state->win_pixel_head + state->win_fwd_pixel_count,
    //     state->win_point_head + state->win_fwd_point_count,
    //     seek_target+read_count
    // );

    // If we scroll off the end of the points, don't attempt to read.
    if( seek_target+read_count < 1 ){ return; }

    // Fill the point buffer from file.
    // FIXME: We are hitting the FS every frame.  This is extremely inefficient and dumb.
    fseek( state->waveform_fd, seek_target + sizeof( zdj_waveform_header_t ), SEEK_SET );
    fread( &state->point_buf[point_buf_index], sizeof( uint8_t ), read_count, state->waveform_fd );


    // printf( "playback_waveform _update_wait done: %d/%d/%d\n", seek_target, point_buf_index, read_count );
}

static void _render( zdj_pipeline_node_t * node, zdj_rect_t * frame ) {
    // printf( "playback waveform _render: %1.1f\n", frame->h );
    zdj_waveform_state_t * state = (zdj_waveform_state_t*)node->state;
    
    if( state->needs_full_render ) {
        // Clear the thingy
        boxColor( zdj_renderer( ), 0, 0, frame->w, frame->h, ZDJ_BLACK );

        double point_addr = state->win_point_head;

        // printf( "win point head: %1.1f\n", state->win_point_head );

        // Walk backward from head drawing points to fill the window
        int i;
        int lead_in_x = -1;
        for( i=0; i<frame->w/2; i++ ){
            // printf( "i: %d, point_addr: %1.2f, ppp: %1.1f\n", i, point_addr, state->points_per_pixel );
            if( round(point_addr) >= 0 ) {
                int point_buf_start = (int)round(state->win_point_head-state->win_back_point_count);
                int point_buf_index = (int)round(point_addr) - point_buf_start;
                uint8_t raw_val = state->point_buf[ point_buf_index ];
                float val = ( (float)raw_val / (float)state->waveform_header->norm_val ) * frame->h;
                switch( state->style ) {
                    case ZDJ_WAVEFORM_TOP_HALF:
                        lineColor( zdj_renderer( ), (frame->w/2)-i, frame->h, (frame->w/2)-i, frame->h-val, ZDJ_WHITE );
                        break;
                    case ZDJ_WAVEFORM_BOTTOM_HALF:
                        lineColor( zdj_renderer( ), (frame->w/2)-i, 0, (frame->w/2)-i, val, ZDJ_WHITE );
                        break;
                    case ZDJ_WAVEFORM_SYM:
                        lineColor( zdj_renderer( ), (frame->w/2)-i, frame->h/2, (frame->w/2)-i, (frame->h/2)-val, ZDJ_WHITE );
                        lineColor( zdj_renderer( ), (frame->w/2)-i, frame->h/2, (frame->w/2)-i, (frame->h/2)+val, ZDJ_WHITE );
                        break;
                }
                
            } else {
                // Capture the lead in pixel index if the start of song is in frame
                lead_in_x = fmax( lead_in_x, (frame->w/2)-i );
            }
            point_addr -= state->points_per_pixel;
        }

        // Walk forward from head drawing points to fill the window
        point_addr = state->win_point_head;
        for( i=0; i<frame->w/2; i++ ){
            if( round(point_addr) >= 0 ) {
                // printf( "i: %d, point_addr: %1.2f, ppp: %1.1f\n", i, point_addr, state->points_per_pixel );
                int point_buf_start = (int)round(state->win_point_head-state->win_back_point_count);
                int point_buf_index = (int)round(point_addr) - point_buf_start;
                uint8_t raw_val = state->point_buf[ point_buf_index ];
                float val = ( (float)raw_val / (float)state->waveform_header->norm_val ) * frame->h;
                switch( state->style ) {
                    case ZDJ_WAVEFORM_TOP_HALF:
                        lineColor( zdj_renderer( ), (frame->w/2)+i, frame->h, (frame->w/2)+i, frame->h-val, ZDJ_WHITE );
                        break;
                    case ZDJ_WAVEFORM_BOTTOM_HALF:
                        lineColor( zdj_renderer( ), (frame->w/2)+i, 0, (frame->w/2)+i, val, ZDJ_WHITE );
                        break;
                    case ZDJ_WAVEFORM_SYM:
                        lineColor( zdj_renderer( ), (frame->w/2)+i, frame->h/2, (frame->w/2)+i, (frame->h/2)-val, ZDJ_WHITE );
                        lineColor( zdj_renderer( ), (frame->w/2)+i, frame->h/2, (frame->w/2)+i, (frame->h/2)+val, ZDJ_WHITE );
                        break;
                }
            } else {
                // Capture the lead in pixel index if the start of song is in frame
                lead_in_x = fmax( lead_in_x, (frame->w/2)+i );
            }
            point_addr += state->points_per_pixel;
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
    } else {
        // Incremental render - move current pixels by offset and add new ones.
    }

    // Draw in-leader


    // Draw out-leader

    state->needs_render = false;
    state->needs_full_render = false;

    // printf( "playback waveform _render done\n" );
}

// NOTE: window move/reset op addresses are in song PCM space, NOT waveform point space.
static zdj_error_type_t _move_window( zdj_pipeline_node_t * node, double offset ) {
    // printf( "playback waveform _move_window: %d\n", offset );
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

    state->zoom_val = zoom_val;
    state->points_per_pixel = (state->zoom_val * (double)ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE) / (double)ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE;
    state->samples_per_point = (double)state->waveform_header->samples_per_point;

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
}

static void _deinit_state( zdj_pipeline_node_t * node ) {
    zdj_waveform_state_t * state = (zdj_waveform_state_t*)node->state;
    if( state->point_buf ){ free( state->point_buf ); }
    if( state->waveform_fd ){ fclose( state->waveform_fd ); }
    if( state->waveform_header ){ free( state->waveform_header ); }
}