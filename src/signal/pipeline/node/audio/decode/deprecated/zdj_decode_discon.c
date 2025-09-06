#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
#include <libavutil/error.h>
#include <libavcodec/codec_id.h>

#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>


zdj_decode_discon_t * zdj_new_decode_discon( zdj_decode_discon_type_t type ) {
    zdj_decode_discon_t * discon = calloc( 1, sizeof( zdj_decode_discon_t ) );
    discon->type = type;
    return discon;
}

void zdj_decode_prepend_discon( zdj_pipeline_node_t * node, zdj_decode_discon_t * discon ) {
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
    if( decode_state->first_discon ) {
        decode_state->first_discon->prev = discon;
        discon->next = decode_state->first_discon;
        decode_state->first_discon = discon;
    } else {
        decode_state->first_discon = discon;
        decode_state->last_discon = discon;
    }
}

void zdj_decode_append_discon( zdj_pipeline_node_t * node, zdj_decode_discon_t * discon ) {
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
    if( decode_state->last_discon ) {
        decode_state->last_discon->next = discon;
        discon->prev = decode_state->last_discon;
        decode_state->last_discon = discon;
    } else {
        decode_state->first_discon = discon;
        decode_state->last_discon = discon;
    }
}

void zdj_decode_clear_discon_stack( zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
    if( decode_state->first_discon ) {
        zdj_decode_discon_t * discon = decode_state->first_discon;
        while( discon ) {
            zdj_decode_discon_t * next_discon = discon->next;
            free( discon );
            discon = next_discon;
        }
        decode_state->first_discon = NULL;
        decode_state->last_discon = NULL;
    }
}

void zdj_decode_update_discon_stack( zdj_pipeline_node_t * node, int64_t win_start, int64_t win_end ) {
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
    if( decode_state->first_discon ) {
        zdj_decode_discon_t * discon = decode_state->first_discon;
        
        // Figure out what we're filling.
        bool fill_fwd = false;
        bool fill_back = false;
        // If the last discon is a loop, fill fwd
        if( decode_state->last_discon->type == ZDJ_DECODE_DISCON_LOOP ) {
            fill_fwd = true;
        }
        // If the first discon is a loop and it has a start addr earlier than decode head, fill back
        if( discon->type == ZDJ_DECODE_DISCON_LOOP &&
            discon->back_jump.depart_decode_addr < decode_state->head_decode_addr
        ) {
            fill_back = true;
        }
        // SKIP TO LOOP CASE
        // If first discon is a skip and it has a depart addr earlier than decode head, remove it.
        // This will cause the next cycle to fill backwards with loop, if present.
        if( discon->type == ZDJ_DECODE_DISCON_SKIP &&
            discon->fwd_jump.depart_decode_addr < decode_state->head_decode_addr 
        ) {
            // Remove first discon
            if( discon->next ) {
                decode_state->first_discon = discon->next;
                discon->next->prev = NULL;
            } else {
                decode_state->first_discon = NULL;
                decode_state->last_discon = NULL;
            }
            free( discon );
        }

        // Perform fills
        if( fill_back ) {
            // Fill back must only happen when first discon is a loop
            while( decode_state->first_discon->back_jump.depart_decode_addr > win_start ) {
                zdj_decode_discon_t * new_discon = zdj_new_decode_discon( ZDJ_DECODE_DISCON_LOOP );

                // Gather loop length from first discon
                int64_t loop_len = decode_state->first_discon->back_jump.dest_pcm_addr - decode_state->first_discon->back_jump.depart_pcm_addr;
                
                new_discon->back_jump.dest_pcm_addr = decode_state->first_discon->back_jump.dest_pcm_addr;
                new_discon->back_jump.depart_pcm_addr = decode_state->first_discon->back_jump.depart_pcm_addr;
                new_discon->back_jump.depart_decode_addr = decode_state->first_discon->back_jump.depart_decode_addr - loop_len;

                new_discon->fwd_jump.dest_pcm_addr = decode_state->first_discon->fwd_jump.dest_pcm_addr;
                new_discon->fwd_jump.depart_pcm_addr = decode_state->first_discon->fwd_jump.depart_pcm_addr;
                new_discon->fwd_jump.depart_decode_addr = decode_state->first_discon->back_jump.depart_decode_addr - 1;

                zdj_decode_prepend_discon( node, new_discon );
            }
        }

        if( fill_fwd ) {
            while( decode_state->last_discon->fwd_jump.depart_decode_addr < win_end ) {
                zdj_decode_discon_t * new_discon = zdj_new_decode_discon( ZDJ_DECODE_DISCON_LOOP );
                
                // Gather loop length from first discon
                int64_t loop_len = decode_state->last_discon->fwd_jump.depart_pcm_addr - decode_state->last_discon->fwd_jump.dest_pcm_addr;

                new_discon->back_jump.dest_pcm_addr = decode_state->last_discon->back_jump.dest_pcm_addr;
                new_discon->back_jump.depart_pcm_addr = decode_state->last_discon->back_jump.depart_pcm_addr;
                new_discon->back_jump.depart_decode_addr = decode_state->last_discon->fwd_jump.depart_decode_addr + 1;

                new_discon->fwd_jump.dest_pcm_addr = decode_state->last_discon->fwd_jump.dest_pcm_addr;
                new_discon->fwd_jump.depart_pcm_addr = decode_state->last_discon->fwd_jump.depart_pcm_addr;
                new_discon->fwd_jump.depart_decode_addr = decode_state->last_discon->fwd_jump.depart_decode_addr + loop_len;

                zdj_decode_append_discon( node, new_discon );
            }
        }

        // Trim any discons which fall before window
        discon = decode_state->first_discon;
        while( discon ) {
            zdj_decode_discon_t * next_discon = discon->next;
            if ( discon->fwd_jump.depart_decode_addr < win_start ) {
                decode_state->first_discon = discon->next;
                if( discon->next ) { 
                    discon->next->prev = NULL; 
                } else {
                    decode_state->last_discon = NULL;
                    decode_state->first_discon = NULL;
                }
                free( discon );
            } else {
                discon = NULL;
                continue;
            }
            discon = next_discon;
        }

        // Trim any discons which fall after window
        discon = decode_state->last_discon;
        while( discon ) {
            zdj_decode_discon_t * prev_discon = discon->prev;
            if ( discon->back_jump.depart_decode_addr > win_end ) {
                decode_state->last_discon = discon->prev;
                if( discon->prev ) { 
                    discon->prev->next = NULL; 
                } else {
                    decode_state->last_discon = NULL;
                    decode_state->first_discon = NULL;
                }
                free( discon );
            } else {
                discon = NULL;
                continue;
            }
            discon = prev_discon;
        }
    }
}