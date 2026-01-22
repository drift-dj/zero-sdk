#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/beatgrid_view/zdj_beatgrid_view.h>
#include <zerodj/ui/view/cuepoint_view/zdj_cuepoint_view.h>
#include <zerodj/ui/view/flag_view/zdj_flag_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_cuepoint_view( 
    zdj_rect_t * frame, 
    zdj_cuepoint_style_t style,
    zdj_deck_t * deck,
    zdj_library_song_t * song,
    char * suppress_eid,
    double zoom_val 
) {
    // Build view
    zdj_view_t * view = zdj_new_view( frame );
    view->type = ZDJ_VIEW_CUEPOINTS;
    view->draw = &_draw;
    view->deinit_state = &_deinit_state;

    zdj_cuepoint_view_state_t * state = calloc( 1, sizeof( zdj_cuepoint_view_state_t ) );
    view->state = state;
    state->style = style;
    state->deck = deck;
    state->song = song;
    state->zoom_val = zoom_val;

    // Add existing cuepoints
    int count = zdj_library_query_count_cuepoints_for_song( song, zdj_library_db );
    if( count > 0 ){ 
        char ** cuepoint_eids = calloc( count, sizeof( char* ) );
        zdj_error_type_t res = zdj_library_query_cuepoints_for_song( 
            song, 
            cuepoint_eids, 
            count, 
            zdj_library_db
        );
        

        if( res == ZDJ_ERROR_LIBRARY_QUERY_OKAY ) {
            state->cuepoints = zdj_library_fetch_cuepoint_dto_for_entity_id( 
                cuepoint_eids[ 0 ],
                zdj_library_db
            );
            // Add items for each artist
            zdj_library_cuepoint_t * prev_cuepoint = state->cuepoints;
            for( int i=1; i<count; i++ ) {
                zdj_library_cuepoint_t * cuepoint = zdj_library_fetch_cuepoint_dto_for_entity_id( 
                    cuepoint_eids[ i ],
                    zdj_library_db
                );
                if( cuepoint ) { 
                    // if( suppress_eid && strcmp( suppress_eid, cuepoint->entity_id ) ) {
                        // Skip the suppressed cuepoint (if requested)
                        // cuepoint->prev = prev_cuepoint;
                        // cuepoint->next = NULL;
                        // if( prev_cuepoint ) { prev_cuepoint->next = cuepoint; }
                    // } else {
                        cuepoint->prev = prev_cuepoint;
                        cuepoint->next = NULL;
                        if( prev_cuepoint ) { prev_cuepoint->next = cuepoint; }
                        prev_cuepoint = cuepoint;
                    // }
                }
                free( cuepoint_eids[ i ] );
            }
        }
        free( cuepoint_eids );
    }

    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "edit_beatgrid_view draw\n" );
    zdj_cuepoint_view_state_t * view_state = (zdj_cuepoint_view_state_t*)view->state;
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)view_state->deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;

    // Remove grid markers
    zdj_remove_all_subviews_of( view );

    // Get decode head origin coord
    double pcm_head = decode_state->head.origin_d;

    // Get zoom factor in beatgrid count space
    double samples_per_pixel = view_state->zoom_val * ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE;
    // printf( "samples per pixel: %f", samples_per_pixel );

    double win_start = pcm_head - ((view->frame.w / 2.0) * samples_per_pixel);
    double win_end = pcm_head + ((view->frame.w / 2.0) * samples_per_pixel);
    
    // Loop on song's cuepoints - draw anything falling within window
    zdj_library_cuepoint_t * cuepoint = view_state->cuepoints;
    while( cuepoint ) {
        // printf( "%s: %1.0f - %ld - %1.0f\n", cuepoint->name, win_start, cuepoint->sample, win_end );
        if( (double)cuepoint->sample > (win_start-(11*samples_per_pixel)) && (double)cuepoint->sample < win_end ) {
            // Add flag
            zdj_view_t * flag_view = zdj_new_flag_view( 
                (cuepoint->is_loop) ? ZDJ_FLAG_TYPE_CUE_LOOP : ZDJ_FLAG_TYPE_CUE_NORM, 
                cuepoint->name 
            );
            zdj_add_subview( view, flag_view );
            flag_view->frame.x = round((cuepoint->sample - win_start) / samples_per_pixel);
            flag_view->frame.y = 2;
            flag_view->frame.w = 11;
            flag_view->frame.h = 10;
        }
        cuepoint = cuepoint->next;
    }
}

static void _deinit_state( zdj_view_t * view ) {
    zdj_cuepoint_view_state_t * state = (zdj_cuepoint_view_state_t*)view->state;
    zdj_library_cuepoint_t * cuepoint = state->cuepoints;
    while( cuepoint ) {
        zdj_library_cuepoint_t * next_cuepoint = cuepoint->next;
        zdj_library_free_cuepoint_dto( cuepoint );
        cuepoint = next_cuepoint;
    }
}