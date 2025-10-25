#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavcodec/codec_id.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>

void _accum( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node );
static void _deinit( zdj_decode_layer_t * layer );

zdj_decode_layer_t * zdj_new_decode_continuous_layer( 
    zdj_pipeline_node_t * node, 
    zdj_decode_addr_t * init_addr 
) {
    // printf( "zdj_new_decode_continuous_layer\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_layer_t * layer = calloc( 1, sizeof( zdj_decode_layer_t ) );

    layer->accum = &_accum;
    layer->fwd_discon.type = ZDJ_DECODE_DISCON_NONE;
    layer->back_discon.type = ZDJ_DECODE_DISCON_NONE;

    zdj_decode_layer_init_addr_api( layer );
    zdj_decode_layer_init_fill_api( layer );
    zdj_decode_layer_init_packet_api( layer );

    // Set up initial addresses
    zdj_decode_init_addr( &layer->init_addr );
    init_addr->copy( init_addr, &layer->init_addr );

    // Note that in continuous layer, there are no lead_in/out samples to crossfade.
    zdj_decode_init_addr( &layer->core_start );
    zdj_decode_init_addr( &layer->lead_in_start );
    zdj_decode_init_addr( &layer->lead_in_end );

    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->core_start, 0.0 );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->lead_in_start, 0.0 );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->lead_in_end, 0.0 );

    double duration = (double)state->song_pcm_duration;
    zdj_decode_init_addr( &layer->core_end );
    zdj_decode_init_addr( &layer->lead_out_start );
    zdj_decode_init_addr( &layer->lead_out_end );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->core_end, duration );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->lead_out_start, duration );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->lead_out_end, duration );
    
    layer->deinit = &_deinit;
    layer->prev = NULL;
    layer->next = NULL;

    return layer;
}

zdj_decode_layer_t * zdj_new_decode_loop_layer( 
    zdj_pipeline_node_t * node, 
    zdj_decode_addr_t * loop_start_addr, 
    int64_t loop_len
) {
    // Set up layer core_start/end + lead_in/out
}

zdj_decode_layer_t * zdj_new_decode_skip_layer( 
    zdj_pipeline_node_t * node,  
    zdj_decode_addr_t * depart_addr, 
    zdj_decode_addr_t * dest_addr 
) {
    // Set up layer core_start/end + lead-in/out
}

void _accum( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    // Run through all packets in layer.
    // For packets intersecting with node's out_buffer, accumulate samples
    // into the out_buf with lead_in/out fades.
    zdj_decode_packet_t * packet = layer->first_packet;
    while( packet ) {
        if( packet->intersects_out_buf( packet, node ) ) {
            packet->render_to_out_buf( packet, layer, node );
        }
        packet = packet->next;
    }
}

static void _deinit( zdj_decode_layer_t * layer ) {
    // printf( "zdj_decode_deinit_layer: %p\n", layer );
    zdj_decode_packet_t * packet = layer->first_packet;
    while( packet ) { 
        zdj_decode_packet_t * next_packet = packet->next;
        packet->deinit( packet );
        packet = next_packet;
    }
    free( layer );
    // printf( "zdj_decode_deinit_layer done\n" );
}

// DEPRECATING
// zdj_decode_layer_t * zdj_decode_add_loop_layer( 
//     zdj_pipeline_node_t * node, 
//     int64_t layer_start_decode_addr, 
//     int64_t layer_start_pcm_addr, 
//     int64_t loop_start_decode_addr, 
//     int64_t loop_start_pcm_addr, 
//     int64_t loop_len
// ) {
    // printf( "zdj_decode_add_loop_layer %ld/%ld %ld/%ld %ld\n", 
    //     layer_start_decode_addr,
    //     layer_start_pcm_addr,
    //     loop_start_decode_addr,
    //     loop_start_pcm_addr,
    //     loop_len
    // );
    // zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    // zdj_decode_layer_t * layer = zdj_new_decode_continuous_layer( layer_start_pcm_addr, layer_start_decode_addr );
    
    // // Set up discon props
    // layer->fwd_discon.type = ZDJ_DECODE_DISCON_LOOP;
    // layer->fwd_discon.depart_decode_addr = loop_start_decode_addr+loop_len;
    // layer->fwd_discon.depart_pcm_addr = loop_start_pcm_addr + loop_len;
    // layer->fwd_discon.dest_pcm_addr = loop_start_pcm_addr;

    // layer->back_discon.type = ZDJ_DECODE_DISCON_LOOP;
    // layer->back_discon.depart_decode_addr = loop_start_decode_addr;
    // layer->back_discon.depart_pcm_addr = loop_start_pcm_addr;
    // layer->back_discon.dest_pcm_addr = loop_start_pcm_addr + loop_len;

    // // Link layer into stack
    // if( state->last_layer && 
    //     ( layer_start_decode_addr >= state->last_layer->init_map_decode ) 
    // ) {
    //     printf( "adding loop layer to end\n" );
    //     layer->prev = state->last_layer;
    //     state->last_layer->next = layer;
    //     state->last_layer = layer;
    //     if( !state->first_layer ) { state->first_layer = layer; }
    // } else {
    //     printf( "adding loop layer to beginning\n" );
    //     layer->next = state->first_layer;
    //     state->first_layer->prev = layer;
    //     state->first_layer = layer;
    //     if( !state->last_layer ) { state->last_layer = layer; }
    // }

    // zdj_decode_fill_layer( node, layer );

    // // printf( "add loop layer done\n" );
    // return layer;
// }
// DEPRECATING



// DEPRECATING
// zdj_decode_layer_t * zdj_decode_add_skip_layer( 
//     zdj_pipeline_node_t * node, 
//     int64_t start_decode_addr, 
//     int64_t depart_pcm_addr, 
//     int64_t dest_pcm_addr 
// ) {
    // printf( "zdj_decode_add_skip_layer: %ld %ld->%ld\n", start_decode_addr, depart_pcm_addr, dest_pcm_addr );
    // zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    // zdj_decode_layer_t * layer = zdj_new_decode_continuous_layer( depart_pcm_addr, start_decode_addr );
    // layer->fwd_discon.type = ZDJ_DECODE_DISCON_INERT;
    // layer->back_discon.type = ZDJ_DECODE_DISCON_SKIP;
    
    // // Set up discon props
    // // Link layer into stack
    // if( state->last_layer ){ 
    //     layer->prev = state->last_layer; 
    //     state->last_layer->next = layer; 
    //     state->last_layer = layer;
    // }

    // // zdj_decode_fill_layer( node, layer );
    // layer->fill( layer, node );

    // return layer;
// }
// DEPRECATING

// static bool _contains_addr( 
//     zdj_decode_layer_t * layer, 
//     zdj_decode_addr_t * addr, 
//     zdj_decode_addr_coord_t ref_coord 
// ) {
//     return ( layer->core_start.less_than( &layer->core_start, addr, ref_coord ) &&
//              layer->core_end.greater_than( &layer->core_end, addr, ref_coord ) );
// }

// static void _update_buf_coords_for_node_head( zdj_decode_layer_t * layer, zdj_decode_addr_t * head ) {
//     // Updates the indexes into node's out buffer.
//     layer->start_addr.update_buf_i_for_node_head( &layer->start_addr, head );
//     layer->end_addr.update_buf_i_for_node_head( &layer->end_addr, head );
//     layer->core_start.update_buf_i_for_node_head( &layer->end_addr, head );
//     layer->core_end.update_buf_i_for_node_head( &layer->end_addr, head );
//     layer->lead_in_start.update_buf_i_for_node_head( &layer->end_addr, head );
//     layer->lead_in_end.update_buf_i_for_node_head( &layer->end_addr, head );
//     layer->lead_out_start.update_buf_i_for_node_head( &layer->end_addr, head );
//     layer->lead_out_end.update_buf_i_for_node_head( &layer->end_addr, head );
// }

// static bool _contains_transport_d_coord( struct zdj_decode_layer_t * layer, zdj_decode_addr_coord_t coord ) {
//     return ( layer->core_start.transport_d < coord && layer->core_end.transport_d > coord );
// }

// static double _origin_d_coord_for_transport_d_coord( zdj_decode_layer_t * layer, double coord ) {
//     // Convert a transport_d coord in a given layer's space into it's origin_d coord space
//     double offset = coord - layer->start_addr.transport_d;
//     return layer->start_addr.origin_d + offset;
// }




// static void _fill( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node ) {
//     printf( "layer _fill\n" );
//     zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;

//     // Capture current node win addresses
//     zdj_decode_addr_t win_start; node_state->get_win_start_addr( node, &win_start );
//     zdj_decode_addr_t win_end; node_state->get_win_end_addr( node, &win_end );

//     // Any discontinuous jump in seek may require garbage packets from the decoder
//     bool seek_has_discontinuity = false;
//     int64_t seek_target;
//     int64_t packet_count;

//     printf( "fill 0\n" );

//     // BIG QUESTION:
//     // Can we init layer core_start/end during empty fill?
//     // Where should core_start/end be set?

//     // Add packet to empty layer
//     if( layer->is_empty( layer ) ) { 
//         packet_count = 1;
//         seek_target = node_state->head.origin_i;
//         layer->seek_and_decode( 
//             layer, node, seek_target, AVSEEK_FLAG_FRAME, ZDJ_DECODE_DIR_FWD, packet_count, node_state->requires_garbage 
//         );
//     }

//     printf( "fill 1\n" );
//     // Fill backward
//     if( layer->can_fill( layer, node, ZDJ_DECODE_DIR_BACK ) ) {
//         printf( "fill 1.1\n" );
//         packet_count = layer->fill_packet_count( layer, node, ZDJ_DECODE_DIR_BACK );
//         seek_target = layer->seek_target( layer, node, packet_count, ZDJ_DECODE_DIR_BACK );
//         layer->seek_and_decode( 
//             layer, node, seek_target, AVSEEK_FLAG_BACKWARD, ZDJ_DECODE_DIR_BACK, packet_count, node_state->requires_garbage 
//         );
//         printf( "fill 1.2\n" );
//         seek_has_discontinuity = true;
//     }

//     printf( "fill 2\n" );
//     // Fill forward
//     if( layer->can_fill( layer, node, ZDJ_DECODE_DIR_FWD ) ) {
//         packet_count = layer->fill_packet_count( layer, node, ZDJ_DECODE_DIR_FWD );
//         seek_target = layer->seek_target( layer, node, packet_count, ZDJ_DECODE_DIR_FWD );
//         layer->seek_and_decode( 
//             layer, node, seek_target, AVSEEK_FLAG_FRAME, ZDJ_DECODE_DIR_FWD, packet_count, seek_has_discontinuity 
//         );
//     }

//     printf( "fill 3\n" );
//     // Trim any packets outside the window
//     while( layer->can_remove_packet( layer, node, layer->first_packet ) ) {
//         layer->remove_packet( layer, layer->first_packet );
//     }
//     while( layer->can_remove_packet( layer, node, layer->last_packet ) ) {
//         layer->remove_packet( layer, layer->last_packet );
//     }
//     printf( "layer _fill done\n" );
// }

// static bool _can_fill( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, zdj_decode_dir_t dir ) {
//     printf( "layer can_fill\n" );
//     zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
//     // Check current first packet in layer against node's win_start and layer's lead_in start
//     zdj_decode_addr_t win_start; state->get_win_start_addr( node, &win_start );
//     zdj_decode_addr_t win_end; state->get_win_end_addr( node, &win_end );
    
//     if( dir == ZDJ_DECODE_DIR_FWD ) {
//         // First packet > layer start && first_packet > win_start && layer start < win_end
//         return ( 
//             layer->first_packet->start_addr.greater_than( 
//                 &layer->first_packet->start_addr, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT 
//             ) &&
//             layer->first_packet->start_addr.greater_than( 
//                 &layer->first_packet->start_addr, &win_start, ZDJ_ADDR_COORD_TRANSPORT 
//             ) &&
//             layer->lead_in_start.less_than( 
//                 &layer->lead_in_start, &win_end, ZDJ_ADDR_COORD_TRANSPORT 
//             )
//         );

//     } else if( dir == ZDJ_DECODE_DIR_BACK ) {
//         printf( "can_fill back: %p\n", layer->last_packet->end_addr.less_than );
//         bool lt1 = layer->last_packet->end_addr.less_than(
//             &layer->last_packet->end_addr, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT
//         );
//         printf( "can_fill back 0\n" );
//         bool lt2 = layer->last_packet->end_addr.less_than(
//             &layer->last_packet->end_addr, &win_end, ZDJ_ADDR_COORD_TRANSPORT
//         );
//         printf( "can_fill back 1\n" );
//         bool gt = layer->lead_out_end.greater_than(
//             &layer->lead_out_end, &win_start, ZDJ_ADDR_COORD_TRANSPORT
//         );
//         printf( "can_fill back 2\n" );
//         // last_packet < layer end && last_packet < win_end && layer end > win_start
//         return ( 
//             layer->last_packet->end_addr.less_than(
//                 &layer->last_packet->end_addr, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT
//             ) &&
//             layer->last_packet->end_addr.less_than(
//                 &layer->last_packet->end_addr, &win_end, ZDJ_ADDR_COORD_TRANSPORT
//             ) &&
//             layer->lead_out_end.greater_than(
//                 &layer->lead_out_end, &win_start, ZDJ_ADDR_COORD_TRANSPORT
//             )
//         );
//     }
// }

// static int64_t _fill_packet_count( 
//     zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, zdj_decode_dir_t dir
// ) {
//     zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
//     int sample_count;

//     if( dir == ZDJ_DECODE_DIR_BACK ) {
//         zdj_decode_addr_t win_start; 
//         state->get_win_start_addr( node, &win_start );
//         sample_count = layer->first_packet->start_addr.transport_i - win_start.transport_i;

//     } else if( dir == ZDJ_DECODE_DIR_FWD ) {
//         zdj_decode_addr_t win_end; 
//         state->get_win_end_addr( node, &win_end );
//         sample_count = win_end.transport_i - layer->last_packet->end_addr.transport_i;
//     }
//     return ceil( (double)sample_count / state->estimated_packet_sample_count );
// }

// static int64_t _seek_target( 
//     zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, int packet_count, zdj_decode_dir_t dir 
// ) {
//     zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

//     if( dir == ZDJ_DECODE_DIR_BACK ) {
//         return layer->first_packet->start_addr.origin_i - ( packet_count * state->estimated_packet_sample_count );

//     } else if ( dir == ZDJ_DECODE_DIR_FWD ) {
//         return layer->last_packet->end_addr.origin_i;
//     }
// }

// static void _seek_and_decode( 
//     zdj_decode_layer_t * layer, 
//     zdj_pipeline_node_t * node, 
//     int64_t seek_target, 
//     int seek_flag,
//     zdj_decode_dir_t dir,
//     int packet_count, 
//     bool predecode_garbage
// ) {
//     printf( "layer seek_and_decode \n" );
//     zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;

//     // Note that we handle the garbage packet decode internally here
//     if( predecode_garbage ) { 
//         seek_target -= (node_state->estimated_packet_sample_count * 2);
//         if( seek_target < 0 ) { seek_target = 0; } 
//     }

//     av_seek_frame( node_state->fmt_ctx, 0, seek_target, seek_flag );

//     if ( predecode_garbage && seek_target > 0 ) { 
//         // Decode and discard 2 frames
//         zdj_decode_garbage_packet( 
//             node_state->song, node_state->fmt_ctx, node_state->codec_ctx 
//         );
//         zdj_decode_garbage_packet( 
//             node_state->song, node_state->fmt_ctx, node_state->codec_ctx 
//         );
//     }

//     if( dir == ZDJ_DECODE_DIR_FWD ) {
//         printf( "decode fwd: %d\n", packet_count );
//         for( int i=0; i<packet_count; i++ ) {  
//             zdj_decode_packet_t * packet = zdj_decode_packet( 
//                 node_state->song, node_state->fmt_ctx, node_state->codec_ctx, node_state->av_timebase_factor 
//             );
//             layer->append_packet( layer, packet );
//         }

//     } else if ( dir == ZDJ_DECODE_DIR_BACK ) {

//         for( int i=0; i<packet_count; i++ ) {  
//             zdj_decode_packet_t * packet = zdj_decode_packet( 
//                 node_state->song, node_state->fmt_ctx, node_state->codec_ctx, node_state->av_timebase_factor
//             );
//             if( i == 0 ) {
//                 layer->prepend_packet( layer, packet );
//             } else {
//                 layer->insert_packet_after( layer, packet, layer->first_packet );
//             }
//         }
//     } 
//     printf( "layer seek_and_decode done\n" );
// }



// static void _prepend_packet( zdj_decode_layer_t * layer, zdj_decode_packet_t * packet ) {
//     if( layer->first_packet ) {
//         packet->next = layer->first_packet;
//         layer->first_packet->prev = packet;
//         layer->first_packet = packet;
//     } else {
//         layer->first_packet = packet;
//         layer->last_packet = packet;
//     }
// }

// static void _append_packet( zdj_decode_layer_t * layer, zdj_decode_packet_t * packet ) {
//     if( layer->last_packet ) {
//         packet->prev = layer->last_packet;
//         layer->last_packet->next = packet;
//         layer->last_packet = packet;
//     } else {
//         layer->last_packet = packet;
//         layer->first_packet = packet;
//     }
// }

// static void _insert_packet_after( 
//     zdj_decode_layer_t * layer, zdj_decode_packet_t * new_packet, zdj_decode_packet_t * target_packet 
// ) {
//     if( target_packet == layer->last_packet ) {
//         layer->append_packet( layer, new_packet );
//     } else {
//         new_packet->next = target_packet->next;
//         new_packet->next->prev = new_packet;
//         new_packet->prev = target_packet;
//         new_packet->prev->next = new_packet;
//     }
// }

// static void _insert_packet_before( 
//     zdj_decode_layer_t * layer, zdj_decode_packet_t * new_packet, zdj_decode_packet_t * target_packet
// ) {
//     if( target_packet == layer->first_packet ) {
//         layer->prepend_packet( layer, new_packet );
//     } else {
//         new_packet->next = target_packet;
//         new_packet->next->prev = new_packet;
//         new_packet->prev = target_packet->prev;
//         new_packet->prev->next = new_packet;
//     }
// }

// static void _remove_packet( zdj_decode_layer_t * layer, zdj_decode_packet_t * packet ) {
//     if( packet == layer->first_packet ) {
//         layer->first_packet = packet->next;
//         if( layer->first_packet ) { layer->first_packet->prev = NULL; }
//     } else if ( packet == layer->last_packet ) {
//         layer->last_packet = packet->prev;
//         if( layer->last_packet ) { layer->last_packet->next = NULL; }
//     } else {
//         packet->prev->next = packet->next;
//         packet->next->prev = packet->prev;
//     }
//     packet->deinit( packet );
// }

// static bool _is_empty( zdj_decode_layer_t * layer ) {
//     return layer->first_packet == NULL && layer->last_packet == NULL;
// }



// static bool _can_remove_packet( 
//     zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, zdj_decode_packet_t * packet
// ) {
//     zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
//     zdj_decode_addr_t win_start; node_state->get_win_start_addr( node, &win_start );
//     zdj_decode_addr_t win_end; node_state->get_win_end_addr( node, &win_end );
//     return ( 
//         layer->lead_in_start.greater_than( &layer->lead_in_start, &win_end, ZDJ_ADDR_COORD_TRANSPORT ) ||
//         layer->lead_in_start.less_than( &layer->lead_out_end, &win_start, ZDJ_ADDR_COORD_TRANSPORT ) );
// }




// // Build addreses from origin coords mapped to layer's transport coords
// static void _addr_for_origin_i_coord( zdj_decode_layer_t * layer, zdj_decode_addr_t * addr, int64_t coord ) {
//     addr->origin_i = coord;
//     addr->origin_d = (double)addr->origin_i;
//     // addr->origin_bg = state->get_beatgrid_coord_for_d_coord( node, addr->origin_d );
//     addr->has_valid_origin = true;

//     // Map against offset from layer's core_start addr
//     double offset = addr->origin_d - layer->core_start.origin_d;

//     addr->transport_d = layer->core_start.transport_d + offset;
//     addr->transport_i = round( addr->transport_d );
//     // addr->transport_bg = state->get_beatgrid_coord_for_d_coord( node, addr->transport_d );
    
//     // addr->buf_i = state->head.buf_i - addr->transport_i;
//     // addr->buf_d = (double)addr->buf_i;
// }

// static void _addr_for_origin_d_coord( zdj_decode_layer_t * layer, zdj_decode_addr_t * addr, double coord ) {
//     addr->origin_d = coord;
//     addr->origin_i = round( addr->origin_d );
//     // addr->origin_bg = state->get_beatgrid_coord_for_d_coord( node, addr->origin_d );
//     addr->has_valid_origin = true;

//     // Map against offset from layer's core_start addr
//     double offset = addr->origin_d - layer->core_start.origin_d;

//     addr->transport_d = layer->core_start.transport_d + offset;
//     addr->transport_i = round( addr->transport_d );
//     // addr->transport_bg = state->get_beatgrid_coord_for_d_coord( node, addr->transport_d );
    
//     // addr->buf_i = state->head.buf_i - addr->transport_i;
//     // addr->buf_d = (double)addr->buf_i;
// }

// static void _addr_for_origin_bg_coord( zdj_decode_layer_t * layer, zdj_decode_addr_t * addr, double coord ) {

// }

