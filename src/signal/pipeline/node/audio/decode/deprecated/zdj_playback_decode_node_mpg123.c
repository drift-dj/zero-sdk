// #include <stdlib.h>
// #include <stdio.h>
// #include <stdbool.h>
// #include <unistd.h>
// #include <pthread.h>
// #include <semaphore.h>

// #include <libavformat/avformat.h>
// #include <libavutil/dict.h>
// #include <libavutil/error.h>
// #include <libavcodec/codec_id.h>

// #include <zerodj/library/zdj_library.h>
// #include <zerodj/signal/pipeline/zdj_pipeline.h>
// #include <zerodj/signal/pipeline/node/audio/zdj_audio_node.h>
// #include <zerodj/signal/pipeline/node/audio/decode/zdj_playback_decode_node.h>



// // mpg123_handle *m_play;
// // int16_t * decode_buf;
// size_t decode_count;
// // int encosize;

// // void _init_mp3_decod_with_loop( zdj_playback_decode_node_state_t * state, playback_loop_t * loop );
// // void _init_mp3_decod_at_sample( zdj_playback_decode_node_state_t * state, int sample );
// void _fill_mp3_decod_window( zdj_playback_decode_node_state_t * state );
// void _cull_mp3_decod_window( zdj_playback_decode_node_state_t * state );
// void _decode_mp3_node( zdj_playback_decode_node_state_t * state, decod_frame_t * node );
// bool _decode_buf_contains_sample( int buf_start_sample, int sample );
// void _get_mp3_node_address_for_offset( decod_address_t * cur_address, decod_address_t * new_address, float offset );
// double _get_loop_xfade_coeff_for_address( decod_address_t * address );
// decod_address_t * _get_loop_xfade_address_for_address( decod_address_t * address );
// int _get_frames_to_loop_jump_out_point( decod_address_t * address );

// void init_mp3_decod( zdj_playback_decode_node_state_t * state ) {
//     // printf( "init_mp3_decod( )\n" );
   
//     // state->head = 0.0f; // refactor to decod_buf_write_head
//     // state->cur_time = 0.0f;

//     // Bring up the mpg123 system
//     int ret;
//     mpg123_init();
//     state->m_play = mpg123_new( NULL, &ret );
//     if( state->m_play == NULL ) {
//         fprintf(stderr, "Unable to create mpg123 handle: %s\n", mpg123_plain_strerror(ret));
//         return;
//     }
//     mpg123_param(state->m_play, MPG123_VERBOSE, 2, 0);
//     // mpg123_param(state->m_play, MPG123_RESYNC_LIMIT, -1, 0);
//     mpg123_param(state->m_play, MPG123_RVA, MPG123_RVA_ALBUM, 0);
//     // mpg123_param(state->m_play, MPG123_PREFRAMES, 2, 0);
//     mpg123_param(state->m_play, MPG123_FLAGS, MPG123_FORCE_SEEKABLE, 0);

//     if( mpg123_open(state->m_play, state->song_dto->audio->filepath ) != MPG123_OK) {
//         fprintf(stderr, "Cannot open %s: %s\n", state->song_dto->audio->filepath, mpg123_strerror(state->m_play));
//         return;
//     }

//     long int rate;
//     int chan;
//     int enco;
//     mpg123_getformat( state->m_play, &rate, &chan, &enco );
//     state->mpg123_encosize = mpg123_encsize( enco );
    
//     mpg123_scan( state->m_play );
    
//     // state->dcod_buf = ( state == ch_1_state ) ? ch_1_ring_buf : ch_2_ring_buf;
//     // state->file_type = RES_FS_MP3;

//     // _init_mp3_decod_with_loop( state, state->loop );
//     ////////////////////////////////////////////////////////////////
//     // Set up a single node which contains the playhead on init   //
//     // All other nodes will be created from this node's data      //
//     ////////////////////////////////////////////////////////////////
//     // int cue_in_sample = state->song->beatgrid_start_sample;
//     // state->song->hotcue = cue_in_sample;
//     // printf( "_init_mp3_decod_with_loop @ %d\n", cue_in_sample );
//     // decod_frame_t * node = create_mp3_node_for_source_sample( state, cue_in_sample );
//     decod_frame_t * node = create_mp3_node_for_source_sample( state, 0 );
    
//     state->decod_address->node = node;
//     state->decod_graph_start_frame = node;
//     state->decod_graph_end_frame = NULL;

//     update_mp3_decod( state );
// }

// void mp3_jump_to_hotcue( zdj_playback_decode_node_state_t * state ) {
//     // printf( "mp3_jump_to_hotcue\n" );
//     // int cue_in_sample = state->song->hotcue;
//     // decod_frame_t * node = create_mp3_node_for_source_sample( state, cue_in_sample );
    
//     // state->decod_address->node = node;
//     // state->decod_graph_start_frame = node;
//     // state->decod_graph_end_frame = NULL;

//     // update_mp3_decod( state );
// }

// void mp3_reset_decod_to_sample( zdj_playback_decode_node_state_t * state, int sample ) {
//     // printf( "mp3_reset_decod_to_sample\n" );

//     // Figure out if sample exists in any current node
//     // If so, set that node as the new decode_address, and run a create/cull cycle

//     decod_frame_t * node = create_mp3_node_for_source_sample( state, sample );
    
//     state->decod_address->node = node;
//     state->decod_graph_start_frame = node;
//     state->decod_graph_end_frame = NULL;

//     update_mp3_decod( state );
// }

// // void _init_mp3_decod_with_loop( zdj_playback_decode_node_state_t * state, playback_loop_t * loop ) {
// //     ////////////////////////////////////////////////////////////////
// //     // Set up a single node which contains the playhead on init   //
// //     // All other nodes will be created from this node's data      //
// //     ////////////////////////////////////////////////////////////////
// //     int cue_in_sample = state->song->beatgrid_start_sample;
// //     state->song->hotcue = cue_in_sample;
// //     // printf( "_init_mp3_decod_with_loop @ %d\n", cue_in_sample );
// //     decod_frame_t * node = create_mp3_node_for_source_sample( state, cue_in_sample );
    
// //     state->decod_address->node = node;
// //     state->decod_graph_start_frame = node;
// //     state->decod_graph_end_frame = NULL;
// // }

// // void _init_mp3_decod_at_sample( zdj_playback_decode_node_state_t * state, int sample ) {

// // }

// void update_mp3_decod( zdj_playback_decode_node_state_t * state ) {
//     // printf( "update_mp3_decod address node: %d/%d\n", 
//     // state->decod_address->node->index,
//     // state->decod_address->node->type );
//     // There exists a special case where a loop exists entirely in the lookahead window
//     // In this case we do not want to decode any additional data, as we can just render
//     // directly from existing decoded data
//     // This will be true for loop lengths less than DECOD_GRAPH_LOOKAHEAD * DECOD_NODE_FRAMES
//     // ex. (8 * 1024) = 8192 = 186 mS = a little more than a 1/16th note @ 120 bpm
//     // will happen often in single-cycle sample playback

//     // Top up the graph window if needed
//     _fill_mp3_decod_window( state );

//     // Trim any extra nodes which fall outside the cull boundaries
//     _cull_mp3_decod_window( state );

//     // Check to see if we're at the end of the song and stop playback
//     decod_frame_t * node = state->decod_graph_start_frame;
//     // if( node->source_datum_sample+DECOD_NODE_FRAMES > state->song->length ) {
//     //     if( state == ch_1_state ) {
//     //         ch_1_disable_drive( );
//     //     } else {
//     //         ch_2_disable_drive( );
//     //     }
//     // }

//     /////////////////////////////////////////////////////////////////////////
//     // Loop on entire graph and decode any nodes which don't yet have data //
//     /////////////////////////////////////////////////////////////////////////
//     // printf( "mp3 decode loop\n" );
//     // Only decode DECOD_NODE_MAX nodes in any pass
//     int decode_count = 0;
//     // node = state->decod_graph_start_frame;
//     node = state->decod_address->node;
//     // while( node && decode_count < 10 ) {
//     while( node ) {
//         // if( node->data == NULL ) {
//         if( !node->decoded ) {
//             // printf( "decoding mp3 node @%d: %p\n", node->index, node );
//             _decode_mp3_node( state, node );
//             // printf( "decoding mp3 node done\n" );
//         }
//         node = node->next;
//         // decode_count++;
//     }

//     node = state->decod_graph_start_frame;
//     // while( node && decode_count < 10 ) {
//     while( node ) {
//         // if( node->data == NULL ) {
//         if( !node->decoded ) {
//             // printf( "decoding mp3 node @%d: %p\n", node->index, node );
//             _decode_mp3_node( state, node );
//             // printf( "decoding mp3 node done\n" );
//         }
//         node = node->next;
//         // decode_count++;
//     }
//     // printf( "mp3 decode loop done\n" );
// }

// /////////////////////////////////////////////////////////////////
// // Create new nodes as required to fill lookahead + lookbehind //
// /////////////////////////////////////////////////////////////////
// void _fill_mp3_decod_window( zdj_playback_decode_node_state_t * state ) {
//     // printf( "_fill_mp3_decod_window\n" );
//     // Lookahead - start at current address node and work forward
//     decod_frame_t * prev_node = state->decod_address->node;
//     int lookahead_val = 0;
//     int decode_node_count = 0;
//     while( lookahead_val < DECOD_GRAPH_LOOKAHEAD ){
//         // printf( "0\n" );
//         // If we reach the end of the graph before the end of lookahead, make a new node
//         decod_frame_t * node = prev_node->next;
//         if( !node ) {
//             // printf( "1\n" );
//             int node_sample_start = prev_node->source_datum_sample + DECOD_NODE_FRAMES;

//             // Create a looping discontinuity if appropriate
//             // if( state->loop && _decode_buf_contains_sample( node_sample_start, state->loop->end_sample ) ) {
//             //     // printf( "creating loop end discon\n" );
//             //     // printf( "2\n" );
//             //     // Loop end goes first
//             //     decod_frame_t * loop_end_node = create_mp3_node_for_source_sample( 
//             //         state, 
//             //         node_sample_start
//             //     );
//             //     loop_end_node->jump_out_index = state->loop->end_sample - node_sample_start;
//             //     loop_end_node->type = DECODE_NODE_TYPE_LOOP_END;
//             //     prev_node->next = loop_end_node;
//             //     loop_end_node->prev = prev_node;
//             //     lookahead_val++;

//             //     // printf( "3\n" );
//             //     // printf( "creating loop start discon\n" );
//             //     // Loop start comes after loop end
//             //     // We want to decode an entire mp3 block BEFORE the jump in point
//             //     // so set the datum_sample to jump_in - mp3 block size
//             //     // and set the loop in point to datum + mp3 block size
//             //     int mp3_block_size = 512; // this is naive
//             //     decod_frame_t * loop_start_node = create_mp3_node_for_source_sample( 
//             //         state, 
//             //         state->loop->start_sample - mp3_block_size
//             //     );
//             //     loop_start_node->jump_in_index = 0 + mp3_block_size;
//             //     loop_start_node->type = DECODE_NODE_TYPE_LOOP_START;
//             //     loop_end_node->next = loop_start_node;
//             //     loop_start_node->prev = loop_end_node;
//             //     node = loop_start_node;
//             //     // printf( "created lookahead loop sequence @%d->%d\n", loop_end_node->index, loop_start_node->index );
//             //     // printf( "4\n" );
//             // } else {

//                 // printf( "5\n" );
//                 node = create_mp3_node_for_source_sample( 
//                     state, 
//                     node_sample_start
//                 );
//                 // printf( "6\n" );
//                 if( node_sample_start > 0 ) {
//                     node->type = DECODE_NODE_TYPE_NORMAL;
//                 } else {
//                     node->type = DECODE_NODE_TYPE_LEADER;
//                 }
//                 prev_node->next = node;
//                 node->prev = prev_node;
//                 // printf( "created lookahead node @%d: %p\n", node->index, node );
//                 // printf( "7\n" );
//             // }
//         }
//         lookahead_val++;
//         prev_node = node;
//         state->decod_graph_end_frame = node;
//         // printf( "8\n" );
//     }
    
//     // printf( "9\n" );
//     // Lookbehind - start at current address node and work backward
//     prev_node = state->decod_address->node;
//     lookahead_val = 0;
//     while( lookahead_val < DECOD_GRAPH_LOOKAHEAD ){
//         // printf( "10\n" );
//         // If we reach the end of the graph before the end of lookahead, make a new node
//         decod_frame_t * node = prev_node->prev;
//         int node_sample_start = prev_node->source_datum_sample - DECOD_NODE_FRAMES;
//         if( !node ) {
//             // printf( "11\n" );
//             node = create_mp3_node_for_source_sample( 
//                 state, 
//                 node_sample_start
//             );
//             // printf( "12\n" );
//             prev_node->prev = node;
//             node->next = prev_node;
//             state->decod_graph_start_frame = node;
//             // printf( "created lookback node @%d: %p\n", node->index, node );
//         }
//         // printf( "13\n" );
//         if( node_sample_start > 0 ) {
//             node->type = DECODE_NODE_TYPE_NORMAL;
//         } else {
//             node->type = DECODE_NODE_TYPE_LEADER;
//         }
//         lookahead_val++;
//         // Don't count nodes which do not render
//         // if( node->type != DECODE_NODE_TYPE_GARBAGE ) {
//         //     lookahead_val++;
//         // } else {
//         //     printf( "made garbage node\n" );
//         // }
//         prev_node = node;
//         // printf( "14\n" );
//     }
//     // printf( "_fill_mp3_decod_window done\n" );
// }

// //////////////////////////////////////////////////////
// // Free nodes which fall ouside the cull boundaries //
// //////////////////////////////////////////////////////
// void _cull_mp3_decod_window( zdj_playback_decode_node_state_t * state ) {
//     // printf( "_cull_mp3_decod_window\n" );
//     decod_frame_t * node = state->decod_graph_start_frame;
//     int lookbehind_cull_index = state->decod_address->node->index - DECOD_GRAPH_CULL_BOUNDARY;
//     while( node ) { // We want to stop iterating once we're inside the node cull bounds
//         if( node->index < lookbehind_cull_index ) {
//             // Unlink this node from the linked list
//             // printf( "unlinking node\n" );
//             decod_frame_t * n_node = (decod_frame_t*)node->next;
//             n_node->prev = NULL;
//             state->decod_graph_start_frame = n_node;
//             free_mp3_node( state, node );
//             node = n_node;
//             // printf( "unlinking node done\n" );
//         } else {
//             node = NULL;
//         }
//     }
//     int lookahead_cull_index = state->decod_address->node->index + DECOD_GRAPH_CULL_BOUNDARY;
//     while( node ) { // We want to stop iterating once we're inside the node cull bounds
//         if( node->index > lookahead_cull_index ) {
//             // Unlink this node from the linked list
//             printf( "culling node: %d\n", node->index );
//             decod_frame_t * p_node = (decod_frame_t*)node->prev;
//             p_node->prev = NULL;
//             state->decod_graph_end_frame = p_node;
//             free_mp3_node( state, node );
//             node = p_node;
//         } else {
//             node = NULL;
//         }
//     }
//     // printf( "_cull_mp3_decod_window done\n" );
// }


// decod_frame_t * create_mp3_node_for_source_sample( zdj_playback_decode_node_state_t * state, int datum_sample ) {
//     // printf( "create_mp3_node_for_source_sample malloc\n" );
//     decod_frame_t * node = malloc( sizeof( decod_frame_t ) );
//     // printf( "create_mp3_node_for_source_sample malloc done\n" );
//     node->index = (int)floor( (double)datum_sample / (double)DECOD_NODE_FRAMES );
//     node->source_datum_sample = datum_sample;
//     node->next = NULL;
//     node->prev = NULL;
//     // node->data = NULL;
//     node->data = malloc( sizeof( int16_t ) * DECOD_NODE_LEN );
//     node->decoded = false;

//     return node;
// }

// void _decode_mp3_node( zdj_playback_decode_node_state_t * state, decod_frame_t * node ) {
//     // printf( "_decode_mp3_node index: %d source_datum:%d\n", node->index, node->source_datum_sample );
//     // printf( "_decode_mp3_node malloc\n" );
//     // node->data = malloc( sizeof( int16_t ) * DECOD_NODE_LEN );
//     // printf( "_decode_mp3_node malloc done\n" );

//     // simplest case - just decode the next block into the buffer
//     // if( node->type == DECODE_NODE_TYPE_NORMAL && node->type != DECODE_NODE_TYPE_LEADER ) {
//     if( node->type != DECODE_NODE_TYPE_LEADER ) {
//         mpg123_seek( state->m_play, node->source_datum_sample, SEEK_SET );
//         if( mpg123_read( state->m_play, node->data, DECOD_BLK_LEN * state->mpg123_encosize, &decode_count ) != MPG123_OK ) {
//             // Assume the playhead is not within the mp3 data of the track anymore and just write zeros to the buffer
//             fprintf( stderr, "---> mpg123_read node %d failed: %s\n", node->index, mpg123_strerror( state->m_play ) );
//             // Mark this node as having an error during decode
//             node->type = DECODE_NODE_TYPE_ERROR;
//         }
//         node->decoded = true;
//     }
// }

// void free_mp3_node( zdj_playback_decode_node_state_t * state, decod_frame_t * node ) {
//     // printf( "free_mp3_node @%d\n", node->index );
//     free( node->data );
//     free( node );
// }

// // Get 1 sample of samples at a given offset from the state's current address
// void get_mp3_decod_sample_at_offset( 
//     zdj_playback_decode_node_state_t * state, 
//     decod_sample_float_t * sample, 
//     double offset,
//     bool interpolate
// ) {
//     // printf( "get_mp3_decod_sample_at_offset\n" );

//     decod_address_t * next_address = malloc( sizeof( decod_address_t ) );
    
//     _get_mp3_node_address_for_offset( 
//         state->decod_address, 
//         next_address, 
//         offset 
//     );


//     if( !interpolate ) {
//         // If we're not interpolating, return the value at the int_index
//         sample->channel_count = 2;
//         // If we're looping, we need to fade across the jump-in and jump-out adresses.
//         if( state->loop ) {
//             double xfade_coeff = _get_loop_xfade_coeff_for_address( next_address );
//             if( xfade_coeff > 0.0 ) {
//                 // Get an address from across the jump-out point so we can crossfade to those samples             
//                 decod_address_t * fade_in_address = _get_loop_xfade_address_for_address( next_address );
//                 if( fade_in_address ) {
//                     double l_samp_in = (double)(fade_in_address->node->data[ fade_in_address->int_index * 2 ]) * (1.0-xfade_coeff);
//                     double r_samp_in = (double)(fade_in_address->node->data[ (fade_in_address->int_index * 2) + 1 ]) * (1.0-xfade_coeff);
//                     double l_samp_out = (double)(next_address->node->data[ next_address->int_index * 2 ]) * xfade_coeff;
//                     double r_samp_out = (double)(next_address->node->data[ (next_address->int_index * 2) + 1 ]) * xfade_coeff;
//                     sample->pcm_values[ 0 ] = l_samp_out + l_samp_in;
//                     sample->pcm_values[ 1 ] = r_samp_out + r_samp_in;

//                     free( fade_in_address );
//                 } else {
//                     // Else, just get the values from the decode buffers
//                     sample->pcm_values[ 0 ] = next_address->node->data[ next_address->int_index * 2 ];
//                     sample->pcm_values[ 1 ] = next_address->node->data[ (next_address->int_index * 2) + 1 ];
//                 }
//             } else {
//                 // Else, just get the values from the decode buffers
//                 sample->pcm_values[ 0 ] = next_address->node->data[ next_address->int_index * 2 ];
//                 sample->pcm_values[ 1 ] = next_address->node->data[ (next_address->int_index * 2) + 1 ];
//             }
//         } else {
//             // Else, just get the values from the decode buffers
//             sample->pcm_values[ 0 ] = next_address->node->data[ next_address->int_index * 2 ];
//             sample->pcm_values[ 1 ] = next_address->node->data[ (next_address->int_index * 2) + 1 ];
//         }
//     } else {
//         // If we are interpolating, do some math to interpolate between in and out sample values
//         // get vals at floor and ceil of address.float_index
//         // use float index as a coeff for each
//         double interp_val = offset - floor( offset );

//         int16_t start_val_l = state->decod_address->node->data[ state->decod_address->int_index * 2 ];
//         int16_t start_val_r = state->decod_address->node->data[ (state->decod_address->int_index * 2) + 1 ];
//         int16_t end_val_l = next_address->node->data[ next_address->int_index * 2 ];
//         int16_t end_val_r = next_address->node->data[ (next_address->int_index * 2) + 1 ];
        
//         sample->channel_count = 2;
//         sample->pcm_values[ 0 ] = ((double)start_val_l * interp_val) + ((double)end_val_l * (1-interp_val));
//         sample->pcm_values[ 1 ] = ((double)start_val_r * interp_val) + ((double)end_val_r * (1-interp_val));
//     }
//     free( next_address );
//     // printf( "get_mp3_decod_sample_at_offset done\n" );
// }

// // Move the state's address by offset amount
// void offset_mp3_decod_address( zdj_playback_decode_node_state_t * state, double offset ) {
//     decod_address_t * new_address = malloc( sizeof( decod_address_t ) );
//     // printf( "offset_mp3_decod_address node: %p\n", state->decod_address->node );
//     // printf( "offset_mp3_decod_address: %1.0f\n", offset );
//     _get_mp3_node_address_for_offset( 
//         state->decod_address, 
//         new_address, 
//         offset 
//     );
//     // printf( "new_address->node: %p %d %1.0f\n", new_address->node, new_address->int_index, new_address->float_index );
//     state->decod_address->node = new_address->node;
//     state->decod_address->int_index = new_address->int_index;
//     state->decod_address->float_index = new_address->float_index;
    
//     free( new_address );

//     update_mp3_decod( state );
// }

// int get_mp3_decod_current_address( zdj_playback_decode_node_state_t * state ) {
//     return state->decod_address->int_index + state->decod_address->node->source_datum_sample;
// }

// // Find a node for a given sample-time offset value
// // Must work with small (<1.0), large (>1.0), postive and negative values for offset
// void _get_mp3_node_address_for_offset( 
//     decod_address_t * cur_address, 
//     decod_address_t * new_address, 
//     float offset
// ) {
//     // printf( "_get_mp3_node_address_for_offset: %1.1f\n", offset );
//     // printf( "_get_mp3_node_address_for_offset @%d %p->type:%d\n", cur_address->node->index, cur_address->node, cur_address->node->type );
//     if( cur_address->node->type == DECODE_NODE_TYPE_LOOP_END ) { 
//         // printf( "discon: %d, %d\n", cur_address->int_index, cur_address->node->jump_out_index );
//         if( offset >= 0 ) { // Looking forward
//             // Check discontinuity node for a jump case
//             if( cur_address->int_index >= cur_address->node->jump_out_index ) {
//                 // jump address accross the loop end/start sequence
//                 new_address->node = cur_address->node->next;
//                 new_address->float_index = (float)new_address->node->jump_in_index;
//                 new_address->int_index = new_address->node->jump_in_index;
//                 // printf( "Jumping from loop end to loop start\n" );
//             } else {
//                 new_address->node = cur_address->node;
//                 new_address->float_index = cur_address->float_index + offset;
//                 new_address->int_index = floor( new_address->float_index );
//             }
//         } else { // Looking in reverse

//         }
//     } else if( offset >= 0 && (cur_address->float_index+offset > ((float)DECOD_NODE_FRAMES-1.0)) ) {
//     // if( offset >= 0 && (cur_address->float_index+offset > ((float)DECOD_NODE_FRAMES-1.0)) ) {
//         // printf( "boundary forward\n" );
//         // Looking forward across continuous node boundary
//         new_address->node = cur_address->node->next;
//         new_address->float_index = (cur_address->float_index+offset) - ((float)DECOD_NODE_FRAMES-1);
//         new_address->int_index = floor( new_address->float_index );
//     } else if( offset < 0 && cur_address->float_index+offset < 0 ) {
//         // Looking in reverse across continuous node boundary
//         new_address->node = cur_address->node->prev;
//         new_address->float_index = ((float)DECOD_NODE_FRAMES-1) + (cur_address->float_index+offset) ;
//         new_address->int_index = floor( new_address->float_index );
//         // printf( "backing cross-boundary off:%1.2f cur: %1.2f/%d next:%1.2f/%d\n", offset, cur_address->float_index, cur_address->int_index, new_address->float_index, new_address->int_index );
//     } else {
//         // printf( "normal forward\n" );
//         // Increment
//         new_address->node = cur_address->node;
//         new_address->float_index = cur_address->float_index + offset;
//         new_address->int_index = floor( new_address->float_index );
//     }   
// }

// bool _decode_buf_contains_sample( int buf_start_sample, int sample ) {
//     bool result = (buf_start_sample < sample) && (buf_start_sample + DECOD_NODE_FRAMES > sample);
//     // if( result ) {
//         // printf( "buffer @ %d contains %d\n", buf_start_sample, sample );
//     // }
//     return result;
// }

// // Take an address, assume there's a loop jump-out point nearby.
// // Figure out how many frames away the jump out point is.
// // Calculate a coefficient for a crossfade between this address and
// // an address across the loop jump
// #define LOOP_XFADE_DURATION 256

// double _get_loop_xfade_coeff_for_address( decod_address_t * address ) {
//     int jump_out_distance = _get_frames_to_loop_jump_out_point( address );
//     if( jump_out_distance > -1 ) {
//         // printf( "coeff: %1.1f\n", (double)jump_out_distance / (double)LOOP_XFADE_DURATION );
//         return (double)jump_out_distance / (double)LOOP_XFADE_DURATION;
//     } else {
//         return 0.0;
//     }
// }

// // Assume there is a loop jump-out near the given address
// // Figure out how many frames away the jump-out point is.
// // Return an address that many frames before the jump-in point.
// decod_address_t * _get_loop_xfade_address_for_address( decod_address_t * address ) {
//     int jump_out_distance = _get_frames_to_loop_jump_out_point( address );
//     if( jump_out_distance > -1 ) {
//         // Find next jump-in point after address
//         decod_frame_t * next_node = address->node->next;
//         if( next_node && next_node->type == DECODE_NODE_TYPE_LOOP_START ) { 
//             // Make an address [jump_out_distance] frames earlier than the jump-in point
//             decod_address_t * new_address = malloc( sizeof( decod_address_t ) );
//             new_address->node = next_node;
//             new_address->float_index = (float)next_node->jump_in_index-jump_out_distance;
//             new_address->int_index = next_node->jump_in_index-jump_out_distance;
//             return new_address;
//         }
//     } else {
//         return NULL;
//     }
// }

// // Take an address, scan forward in the decode buffer spec to find the next jump-out point
// // Return the number of PCM frames between the given address and the address at the jump-out point
// // NOTE that return will ALWAYS be less thann LOOP_XFADE_DURATION OR will be -1
// int _get_frames_to_loop_jump_out_point( decod_address_t * address ) {
//     // If jump-out is in the given address's node
//     if( address->node->type == DECODE_NODE_TYPE_LOOP_END ) {
//         // If jump-out is fewer than LOOP_XFADE_DURATION frames away
//         if( address->node->jump_out_index - address->int_index < LOOP_XFADE_DURATION ) {
//             // return difference between address index and node's jump-out index 
//             return address->node->jump_out_index - address->int_index;
//         } else {
//             return -1;
//         }
//     } else if( DECOD_NODE_FRAMES - address->int_index > LOOP_XFADE_DURATION ) {
//     // Else If given address is more than LOOP_XFADE_DURATION frames from the end of it's node's buffer
//         // Assume the jump-out is too far away and return -1
//         return -1;
//     } else {
//         // Else look into the next node's buffer for jump-out index
//         decod_frame_t * next_node = address->node->next;
//         if( next_node && next_node->type == DECODE_NODE_TYPE_LOOP_END ) {
//             // If it exists return the offset
//             int offset = DECOD_NODE_FRAMES - address->int_index + next_node->jump_out_index;
//             if( offset < LOOP_XFADE_DURATION ) {
//                 return offset;
//             } else {
//                 return -1;
//             }
//         } else {
//             return -1;
//         }
//     }
//     return -1;
// }