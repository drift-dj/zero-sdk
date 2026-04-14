#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>

// static void _enable_loop_discon( zdj_pipeline_node_t * node, void * _controls );
// static void _release_loop_discon( zdj_pipeline_node_t * node, void * _controls );
// static void _move_loop_discon( zdj_pipeline_node_t * node, void * _controls );
// static bool _move_loop_discon_will_move_head( zdj_pipeline_node_t * node, void * _controls );
// static void _resize_loop_discon( zdj_pipeline_node_t * node, void * _controls );
// static void _refresh_loop_discon_layers( zdj_pipeline_node_t * node, void * _controls );
static void _add_skip_discon( zdj_pipeline_node_t * node, void * _controls );
static void _add_hyperscrub_discon( zdj_pipeline_node_t * node, void * _controls );

void zdj_decode_init_node_discon_api( zdj_pipeline_node_t * node ) { 
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    state->discon_is_active = false;
    // state->enable_loop_discon = &_enable_loop_discon;
    // state->release_loop_discon = &_release_loop_discon;
    // state->move_loop_discon = &_move_loop_discon;
    // state->move_loop_discon_will_move_head = &_move_loop_discon_will_move_head;
    // state->resize_loop_discon = &_resize_loop_discon;
    // state->refresh_loop_discon_layers = &_refresh_loop_discon_layers;
    state->add_skip_discon = &_add_skip_discon;
    state->add_hyperscrub_discon = &_add_hyperscrub_discon;
}


// // Note: Enable may be called when loop is already enabled.
// // It should reset layer state based on current state of controls->loop_state
// static void _enable_loop_discon( zdj_pipeline_node_t * node, void * _controls ) {
//     // printf( "_enable_loop_discon\n" );
//     zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
//     zdj_deck_control_state_t * controls = (zdj_deck_control_state_t*)_controls;

//     // Find Layer/Packet
//     zdj_decode_layer_t * layer_under_head = decode_state->get_layer_containing_addr( node, &decode_state->head, ZDJ_ADDR_COORD_ORIGIN );
//     if( !layer_under_head ) { printf( "ENABLED LOOP W/O LAYER\n" ); return; }
//     // Calculate loop_start addresses
//     double loop_start_d_coord;

//     double p_s = controls->loop_state.start_origin_d;
//     double p_e = controls->loop_state.end_origin_d;

//     // printf( "_enable_loop_discon head:%1.1f/%1.1f\n", decode_state->head.origin_d, decode_state->head.transport_d );

//     if( controls->discon_quantize &&
//         decode_state->song->performance &&
//         decode_state->song->performance->bpm 
//     ) {
//         // Find previous beat grid coord w/current quantize setting
//         double head_origin_bg = decode_state->head.origin_bg;
//         // Quantize head to previous BG coord
//         double head_quant = floor( head_origin_bg / controls->discon_quantize_val ) * controls->discon_quantize_val;
//         double quant_offset = zdj_signal_pcm_count_for_beatgrid_count(
//             head_quant - head_origin_bg,
//             decode_state->song->performance->bpm,
//             decode_state->song->audio->av_sample_rate
//         );
//         loop_start_d_coord = decode_state->head.origin_d + quant_offset;
//         // printf( "orig_bg:%1.3f obg_q:%1.3f offs:%1.3f head:%1.1f st_crd:%1.1f\n",
//         //     head_origin_bg, head_quant, quant_offset, decode_state->head.origin_d, loop_start_d_coord
//         // );
//     } else {
//         // If we aren't playing there's space, set the loop start to a few samples before the head.
//         // Improves stability when enabling while playhead is stationary.
//         if( !controls->platter.motor.enabled ) {
//             printf( "enabling w/offest\n" );
//             loop_start_d_coord = fmax( 0.0, (decode_state->head.origin_d - 200) );
//         } else {
//             printf( "enabling w/o offest\n" );
//             loop_start_d_coord = decode_state->head.origin_d;
//         }
        
//     }
//     controls->loop_state.start_origin_d = loop_start_d_coord;
//     controls->loop_state.end_origin_d = controls->loop_state.start_origin_d + controls->loop_state.pcm_len;
//     decode_state->layer_fade_len = controls->loop_state.fade_len;

//     //  printf( "_enable_loop_discon head:%1.1f val:%d %1.1f>%1.1f -> %1.1f>%1.1f\n\n", 
//     //     decode_state->head.origin_d, decode_state->head.has_valid_origin,
//     //     p_s, p_e,
//     //     controls->loop_state.start_origin_d, controls->loop_state.end_origin_d
//     // );
// }

// static void _refresh_loop_discon_layers( zdj_pipeline_node_t * node, void * _controls ) {
//     zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
//     zdj_deck_control_state_t * controls = (zdj_deck_control_state_t*)_controls;

//     // printf( "_refresh_loop_discon_layers head-origin:%1.1f\n", decode_state->head.origin_d );
    

//     if( !decode_state->head.has_valid_origin ) { 
//         printf( "REFRESH LOOP CALLED W/O VALID ORIGIN bugging out\n" );
//         return;
//     }

//     // Find Layer/Packet
//     zdj_decode_layer_t * layer_under_head = decode_state->get_layer_containing_addr( node, &decode_state->head, ZDJ_ADDR_COORD_ORIGIN );

//     // printf( "layer_under_head: %p\n", layer_under_head );

//     // printf( "pre layer_under_head: [%1.1f - %1.1f - %1.1f]\n", 
//     //     layer_under_head->core_start.origin_d, 
//     //     decode_state->head.origin_d,
//     //     layer_under_head->core_end.origin_d 
//     // );
//     // Calculate loop_start addresses
//     double loop_start_d_coord;


//     // Layer under head will be a loop - re-truncate layer's start and end to loop
//     // layer_under_head->truncate( 
//     //     node, layer_under_head, controls, controls->loop_state.start_origin_d, ZDJ_DECODE_DISCON_LOOP, &controls->loop_state 
//     // NOT REALLY HERE - node->move_window( node, 0 );
//     // );



//     // If the truncate clips before the playback head, 
//     // extend the end of the loop to something after the head.
//     if( decode_state->move_loop_discon_will_move_head( node, controls ) ) {
//         if( controls->discon_quantize ) {
//             // printf( "re-quantizing loop start\n" );
//             // If we're quantized, expand the layer end to the next quantize step.
//             double quant_head = decode_state->get_quantized_head_origin_bg( node, 0.250 );
//             double next_bg = quant_head + 0.250;
//             double end_addr_bg = layer_under_head->core_end.origin_bg;
//             decode_state->offset_addr_by_transport_bg_coord( 
//                 node, &layer_under_head->core_end, next_bg - end_addr_bg
//             );
//             decode_state->offset_addr_by_transport_bg_coord( 
//                 node, &layer_under_head->lead_out_start, next_bg - end_addr_bg
//             );
//             decode_state->offset_addr_by_transport_bg_coord( 
//                 node, &layer_under_head->lead_out_end, next_bg - end_addr_bg
//             );
//         } else {
//             // If we're not quantized, expand the layer end a couple buffers after the current head
//         }
//     }

//     // If the layer_under_head moves to a coord entirely after the head, 
//     // move the start of the layer back to the head
//     if( layer_under_head->core_start.greater_than( 
//             &layer_under_head->core_start, &decode_state->head, ZDJ_ADDR_COORD_TRANSPORT 
//         )
//     ) {
//         double head_offset = layer_under_head->core_start.transport_d - decode_state->head.transport_d;
//         head_offset += decode_state->estimated_packet_sample_count * 2;
//         // printf( "moving layer start by:%1.0f from:%1.0f to:%1.0f\n", 
//         //     head_offset,
//         //     layer_under_head->core_start.transport_d,
//         //     layer_under_head->core_start.transport_d - head_offset
//         // );
//         decode_state->offset_addr_by_transport_d_coord( 
//             node, &layer_under_head->core_start, head_offset * -1, true

//         );
//         decode_state->offset_addr_by_transport_d_coord( 
//             node, &layer_under_head->lead_in_start, head_offset * -1, true
//         );
//         decode_state->offset_addr_by_transport_d_coord( 
//             node, &layer_under_head->lead_in_end, head_offset * -1, true
//         );
//     }

//     // printf( "post layer_under_head: [%1.1f - %1.1f - %1.1f]\n", 
//     //     layer_under_head->core_start.origin_d, 
//     //     decode_state->head.origin_d,
//     //     layer_under_head->core_end.origin_d 
//     // );

//     // Remove any other layers in node
//     zdj_decode_layer_t * prev_layer = layer_under_head->prev;
//     while( prev_layer ) {
//         zdj_decode_layer_t * new_prev_layer = prev_layer->prev;
//         decode_state->remove_layer( node, prev_layer );
//         prev_layer = new_prev_layer;
//     }
//     zdj_decode_layer_t * next_layer = layer_under_head->next;
//     while( next_layer ) {
//         zdj_decode_layer_t * new_next_layer = next_layer->next;
//         decode_state->remove_layer( node, next_layer );
//         next_layer = new_next_layer;
//     }
//     decode_state->first_layer = layer_under_head;
//     decode_state->last_layer = layer_under_head;

//     // printf( "Appending loop layer\n" );
//     decode_state->discon_is_active = true;

//     // double p_start = zdj_perf_time( );

//     // Zero move to re-fill the window
//     // printf( "_enable_loop_discon 0\n" );
//     // printf( "loop move win\n" );
//     node->move_window( node, 0 );
//     // printf( "loop move win done\n" );
//     // double p_end = zdj_perf_time( );
//     // printf( "new loop refresh layers:%1.3f\n", ( p_end - p_start ) / 1000000.0 );

//     // printf( "_refresh_loop_discon_layers done\n" );
// }

// static void _release_loop_discon( zdj_pipeline_node_t * node, void * _controls ) {
//     // printf( "_release_loop_discon\n" );
//     zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
//     // zdj_deck_control_state_t * controls = (zdj_deck_control_state_t*)_controls;

//     decode_state->discon_is_active = false;

//     zdj_decode_layer_t * layer_under_head = decode_state->get_layer_containing_addr( node, &decode_state->head, ZDJ_ADDR_COORD_TRANSPORT );
//     layer_under_head->untruncate( node, layer_under_head );

//     // Zero move to re-fill the window
//     node->move_window( node, 0 );
//     // printf( "_release_loop_discon done\n" );
// }

// static void _move_loop_discon( zdj_pipeline_node_t * node, void * _controls ) {
//     // printf( "_move_loop_discon:\n" );
//     zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
//     zdj_deck_control_state_t * controls = (zdj_deck_control_state_t*)_controls;

//     double p_s = controls->loop_state.start_origin_d;
//     double p_e = controls->loop_state.end_origin_d;

//     // Just move the start/end coords of loop_state
//     controls->loop_state.start_origin_d += controls->loop_state.move_req_len;
//     controls->loop_state.end_origin_d = controls->loop_state.start_origin_d + controls->loop_state.pcm_len;

//     // printf( "_move_loop_discon: %1.1f>%1.1f -> %1.1f>%1.1f\n", 
//     //     p_s, p_e,
//     //     controls->loop_state.start_origin_d, controls->loop_state.end_origin_d
//     // );

//     // controls->loop_state.move_req_len = 0;
// }

// static bool _move_loop_discon_will_move_head( zdj_pipeline_node_t * node, void * _controls ) {
//     // printf( "_move_loop_discon_will_move_head\n" );
//     zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
//     zdj_deck_control_state_t * controls = (zdj_deck_control_state_t*)_controls;

//     // Figure out if moving loop start/end coords will step on the decode head addr
    
//     // Loop start and end have moved. Figure out if head is now outside loop bounds.
//     // printf( "_move_loop_discon_will_move_head %1.1f - %1.1f - %1.1f\n",
//     //     controls->loop_state.start_origin_d,
//     //     decode_state->head.origin_d,
//     //     controls->loop_state.end_origin_d 
//     // );
//     if( decode_state->head.origin_d < controls->loop_state.start_origin_d ||
//         decode_state->head.origin_d > controls->loop_state.end_origin_d 
//     ) {
//         return true;
//     } else {
//         return false;
//     }
// }

// static void _resize_loop_discon( zdj_pipeline_node_t * node, void * _controls ) {
//     zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
//     zdj_deck_control_state_t * controls = (zdj_deck_control_state_t*)_controls;
   
//     double change_len = controls->loop_state.length_change_req_len;

//     // printf( "_resize_loop_discon: %1.1f - %1.1f\n", controls->loop_state.start_origin_d, controls->loop_state.end_origin_d );

//     controls->loop_state.length_change_req_len = 0;

//     // Move the end coord/length of the loop_state
//     controls->loop_state.end_origin_d += change_len;
//     controls->loop_state.pcm_len = controls->loop_state.end_origin_d - controls->loop_state.start_origin_d;
//     // 
//     // Constrain the loop length to the song PCM length
// }

static void _add_skip_discon( zdj_pipeline_node_t * node, void * _controls ) {
    // printf( "_add_skip_discon\n" );
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
    zdj_deck_control_state_t * controls = (zdj_deck_control_state_t*)_controls;

    // Find Layer/Packet
    zdj_decode_layer_t * layer_under_head = decode_state->get_layer_containing_addr( node, &decode_state->head, ZDJ_ADDR_COORD_ORIGIN );

    // Find skip addr
    double skip_origin_depart_bg;
    double skip_origin_depart_d;
    double skip_origin_dest_bg;
    double skip_origin_dest_d;
    double head_origin_bg;
    double head_quant;
    double quant_offset;
    switch ( controls->skip_state.skip_req_type ) {
        case ZDJ_DECK_SKIP_TYPE_QUANT:
            // TODO: quantize this to next beatgrid
            head_origin_bg = decode_state->head.origin_bg;
            // Quantize head to previous BG coord
            head_quant = floor( head_origin_bg / controls->discon_quantize_val ) * controls->discon_quantize_val;
            skip_origin_depart_bg = head_quant + controls->discon_quantize_val;
            // skip_origin_dest_bg = skip_origin_depart_bg + controls->skip_state.skip_req_len;
            skip_origin_depart_d = zdj_signal_pcm_count_for_beatgrid_count(
                skip_origin_depart_bg,
                decode_state->song->performance->bpm,
                decode_state->song->audio->av_sample_rate
            );
            skip_origin_dest_d = skip_origin_depart_d + controls->skip_state.skip_req_len;
            // printf( "dpt bg:%1.3f/d:%1.1f dst d:%1.1f\n",
            //     skip_origin_depart_bg, skip_origin_depart_d,
            //     skip_origin_dest_d 
            // );
            // printf( "hbg:%1.3f q:%1.3f qval:%1.3f obg:%1.3f hd:%1.1f od%1.1f\n",
            //     decode_state->head.origin_bg,
            //     head_quant,
            //     controls->discon_quantize_val,
            //     skip_origin_depart_bg,
            //     decode_state->head.origin_d,
            //     skip_origin_depart_d
            // );
            // printf( "skip org:%1.1f hd:%1.1f\n", skip_origin_depart_d, decode_state->head.origin_d );
            // printf( "skip orig cur:%1.1f/%1.1f to:%1.1f/%1.1f\n", 
            //     decode_state->head.origin_d, decode_state->head.transport_d,
            //     skip_origin_depart_d 
            // );
            break;
        case ZDJ_DECK_SKIP_TYPE_UNQUANT:
            // skip_origin_depart_d = decode_state->head.origin_d + controls->skip_state.skip_req_len;
            skip_origin_depart_d = decode_state->head.origin_d + decode_state->win_fwd_sample_count + 10;
            break;
        case ZDJ_DECK_SKIP_TYPE_SCRUB:
            // Stick the new skip layer halfway down the window
            skip_origin_depart_d = decode_state->head.origin_d + ((double)decode_state->win_fwd_sample_count);
            break;
    }

    // Bug out early if we're skipping outside song
    if( skip_origin_depart_d < 0.0 || skip_origin_depart_d > decode_state->song_pcm_duration ) {
        return;
    }

    // printf( "pre skip hd bg:%1.3f/q:%1.3f/->:%1.3f hd t_d:%1.1f/o_d:%1.1f sk o_d:%1.1f\n", 
    //     head_origin_bg, head_quant, skip_origin_depart_bg,
    //     decode_state->head.transport_d,
    //     decode_state->head.origin_d,
    //     skip_origin_depart_d
    // );
    
    // Truncate layer under head to start of new discon addr
    // layer_under_head->truncate( 
    //     node, layer_under_head, controls, skip_origin_depart_d, ZDJ_DECODE_DISCON_SKIP, NULL 
    // );
    layer_under_head->truncate_to_skip( layer_under_head, node, skip_origin_depart_d );

    decode_state->discon_is_active = true;

    // Set up the address for the new layer
    zdj_decode_addr_t skip_start;
    decode_state->last_layer->core_end.copy( &decode_state->last_layer->core_end, &skip_start );
    // Add d/i/bg vals to skip state


    // printf( "post skip hd bg:%1.3f d:%1.1f sk o_d:%1.1f\n\n", 
    //     decode_state->head.origin_bg, 
    //     decode_state->head.origin_d,
    //     skip_origin_depart_d
    // );
    
    // Re-use and update the skip-start addr to populate the new skip layer
    // skip_start.origin_d += controls->skip_state.skip_req_len;
    skip_start.origin_bg = skip_origin_dest_bg;
    skip_start.origin_d = skip_origin_dest_d;
    skip_start.origin_i = (int64_t)skip_start.origin_d;

    // printf( "skip 1\n" );
    // Add the new skip layer
    decode_state->append_layer( 
        node, zdj_new_decode_skip_layer( node, &skip_start ) 
    );
    // printf( "skip 2\n" );
    decode_state->last_layer->update_buf_coords_for_head( decode_state->last_layer, node );
    // printf( "skip 3 last_layer %p start: %1.0f\n", decode_state->last_layer, decode_state->last_layer->init_addr.transport_d );
    decode_state->last_layer->fill( decode_state->last_layer, node );

    // If we aren't playing forward, remove the preceeding layer and add the first back-loop layer

    // printf( "skip 4\n" );

    // Zero move to re-fill the window
    node->move_window( node, 0 );

    // Track the offset for front-end UI stuff
    controls->skip_state.current_offset += controls->skip_state.skip_req_len;

    // printf( "_add_skip_discon done\n" );
}

static void _add_hyperscrub_discon( zdj_pipeline_node_t * node, void * _controls ) {
    // printf( "_add_hyperscrub_discon\n" );
    // zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
    // zdj_deck_control_state_t * controls = (zdj_deck_control_state_t*)_controls;

    // // Find Layer/Packet
    // zdj_decode_layer_t * layer_under_head = decode_state->get_layer_containing_addr( node, &decode_state->head, ZDJ_ADDR_COORD_ORIGIN );


    // /////////////////////////////////////////
    // // TODO: Fix the skip origin algo below
    // /////////////////////////////////////////

    // // Find origin addr
    // double skip_origin_depart_d = decode_state->head.origin_d + ((double)decode_state->win_fwd_sample_count);
    
    // // Truncate layer under head to start of new discon addr
    // layer_under_head->truncate( 
    //     node, layer_under_head, controls, skip_origin_depart_d, ZDJ_DECODE_DISCON_SKIP, NULL 
    // );

    // //////////////////////////////////////////
    // //////////////////////////////////////////
    // //////////////////////////////////////////

    // decode_state->discon_is_active = true;

    // // Set up the address for the new layer
    // zdj_decode_addr_t skip_start;
    // decode_state->last_layer->core_end.copy( &decode_state->last_layer->core_end, &skip_start );

    // printf( "skip len: %1.3f\n", controls->skip_state.skip_req_len );
    // skip_start.origin_d += controls->skip_state.skip_req_len;
    
    // // printf( "skip 1\n" );
    // // Add the new skip layer
    // decode_state->append_layer( 
    //     node, 
    //     zdj_new_decode_hyperscrub_layer( 
    //         node, 
    //         &skip_start,
    //         (controls->hyperscrub_state.req_offset > 0) ? ZDJ_DECODE_DIR_FWD : ZDJ_DECODE_DIR_BACK
    //     ) 
    // );
    // // printf( "skip 2\n" );
    // decode_state->last_layer->update_buf_coords_for_head( decode_state->last_layer, node );
    // printf( "skip 3 last_layer start: %1.0f\n", decode_state->last_layer->init_addr.transport_d );
    // decode_state->last_layer->fill( decode_state->last_layer, node );

    // // If we aren't playing forward, remove the preceeding layer and add the first back-loop layer

    // // printf( "skip 4\n" );

    // // Zero move to re-fill the window
    // node->move_window( node, 0 );

    // // printf( "_add_skip_discon done\n" );
}