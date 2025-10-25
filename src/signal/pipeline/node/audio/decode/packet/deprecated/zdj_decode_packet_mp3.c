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

void zdj_decode_add_packet_before_mp3_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer ) {
    // printf( "zdj_decode_add_packet_before_mp3_layer\n" );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;

    if( layer->first_packet->is_back_extent ) { printf( "not adding before back extent\n" ); return; }

    // Stand up new packet
    zdj_decode_packet_t * new_packet = calloc( 1, sizeof( zdj_decode_packet_t ) );
    new_packet->av_packet = NULL;
    new_packet->av_frame = NULL;
    new_packet->av_timebase_factor = ( node_state->song->audio->av_codec_id == AV_CODEC_ID_MP3 ) ? 320 : 1;
    
    // Set up some refs based on linking direction (fwd vs. back)
    zdj_decode_packet_t * linked_packet;
    int64_t packet_seek_timestamp = 0;
    
    linked_packet = layer->first_packet; 
    
    // printf( "back linked packet: %p\n", linked_packet );
    if ( linked_packet->type == ZDJ_DECODE_PACKET_TYPE_START_INERT ) {
        zdj_decode_make_admin_packet( 
            layer, new_packet, linked_packet,
            ZDJ_DECODE_PACKET_TYPE_START_BUMPER, ZDJ_DECODE_DIR_BACK,
            1000000
        );
        return;
    } else if ( linked_packet->has_sof ) {
        // If we're pre-pending to first decoded packet in the stream, add the start inert
        zdj_decode_make_admin_packet( 
            layer, new_packet, linked_packet,
            ZDJ_DECODE_PACKET_TYPE_START_INERT, ZDJ_DECODE_DIR_BACK,
            1000
        );
        return;
    } else if ( !linked_packet->has_sof ) {
        // If we're pre-pending to a normal packet
        new_packet->type = ZDJ_DECODE_PACKET_TYPE_NORMAL;

        // Jump back 2 packets - we need to decode/discard 1 packet before our target packet to flush the decoder.
        packet_seek_timestamp = linked_packet->packet_pcm_addr - (linked_packet->av_frame_sample_count*2);

        if( packet_seek_timestamp < 0 ) { printf( "zeroing ts\n" ); packet_seek_timestamp = 0; } // Catch first decode packet
        packet_seek_timestamp *= linked_packet->av_timebase_factor; // Modify timebase if mp3.
        av_seek_frame( node_state->fmt_ctx, 0, packet_seek_timestamp, AVSEEK_FLAG_BACKWARD );

        // Decode a garbage packet
        zdj_decode_garbage_packet( node, new_packet );
        // printf( "done decoding garbage\n" );
    } else {
        // printf( "unhandled case\n" );
    }

    int decode_count = zdj_decode_packet( node, new_packet );

    // sof is used to catch first inert packet
    if( packet_seek_timestamp == 0 ) { new_packet->has_sof = true; }

    int64_t packet_pcm_addr = new_packet->av_packet->pts / new_packet->av_timebase_factor;
    int64_t packet_pcm_offset = layer->init_map_pcm - packet_pcm_addr;
    new_packet->packet_pcm_addr = packet_pcm_addr;

    new_packet->packet_decode_addr = linked_packet->packet_decode_addr - (linked_packet->packet_pcm_addr -  new_packet->packet_pcm_addr);

    // No Discon Case
    // --------------
    if( layer->back_discon.type == ZDJ_DECODE_DISCON_INERT &&
        layer->fwd_discon.type == ZDJ_DECODE_DISCON_INERT 
    ) {
        new_packet->core_start_addr = linked_packet->core_start_addr - new_packet->av_frame_sample_count;
        new_packet->core_end_addr = new_packet->core_start_addr + new_packet->av_frame_sample_count - 1;
    
    // Loop Discon Case
    // --------------
    } else if ( layer->back_discon.type == ZDJ_DECODE_DISCON_LOOP &&
                layer->fwd_discon.type == ZDJ_DECODE_DISCON_LOOP 
    ) {
        // If loop start/end fall inside packet, make appropriate settings
        // printf( "pack bef %ld %ld %ld\n", 
        //     new_packet->packet_decode_addr, 
        //     layer->back_discon.depart_decode_addr, 
        //     new_packet->packet_decode_addr + new_packet->av_frame_sample_count 
        // );
        if( zdj_decode_packet_contains_decode_addr( new_packet, layer->back_discon.depart_decode_addr ) ) {
            // printf( "found loop start packet\n" );
            new_packet->core_start_addr = layer->back_discon.depart_decode_addr;
            new_packet->is_back_extent = true;
        } else {
            // printf( "found loop intermediate packet\n" );
            // printf( "%ld %ld %ld\n", new_packet->core_start_addr, layer->back_discon.depart_decode_addr, new_packet->core_end_addr );
            new_packet->core_start_addr = layer->init_map_decode - packet_pcm_offset;
            new_packet->is_back_extent = false;
        }

        if( zdj_decode_packet_contains_decode_addr( new_packet, layer->fwd_discon.depart_decode_addr ) ) {
            // printf( "found loop end packet\n" );
            new_packet->core_end_addr = layer->fwd_discon.depart_decode_addr;
            new_packet->is_fwd_extent = true;
        } else {
            new_packet->core_end_addr = new_packet->packet_decode_addr + new_packet->av_frame_sample_count - 1;
            new_packet->is_fwd_extent = false;
        }
        
    // Skip Discon Case
    // --------------
    } else if ( layer->fwd_discon.type == ZDJ_DECODE_DISCON_SKIP ) {
        
    }

    new_packet->core_sample_count = new_packet->core_end_addr - new_packet->core_start_addr;
    new_packet->lead_in_start_addr = new_packet->core_start_addr;
    new_packet->lead_out_end_addr = new_packet->core_end_addr;
    
    layer->earliest_core_sample = new_packet->core_start_addr;
    layer->earliest_lead_in_sample = new_packet->core_start_addr; // no lead_in during laminar.

    // Insert packet at beginning of layer's packet list
    layer->first_packet = new_packet;
    new_packet->next = linked_packet;
    linked_packet->prev = new_packet;
    new_packet->prev = NULL;

    // printf( "zdj_decode_add_packet_before_mp3_layer done\n" );
}

void zdj_decode_add_packet_after_mp3_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer ) {
    // printf( "zdj_decode_add_packet_after_mp3_layer\n" );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    
    // Stand up new packet
    zdj_decode_packet_t * new_packet = calloc( 1, sizeof( zdj_decode_packet_t ) );
    // new_packet->id = node_state->packet_counter++;
    new_packet->av_timebase_factor = ( node_state->song->audio->av_codec_id == AV_CODEC_ID_MP3 ) ? 320 : 1;
    
    // Set up some refs based on linking direction (fwd vs. back)
    zdj_decode_packet_t * linked_packet;
    int64_t packet_seek_timestamp = 0;
    linked_packet = layer->last_packet; 

    // printf( "fwd linked packet: %p\n", linked_packet );
    if ( linked_packet->type == ZDJ_DECODE_PACKET_TYPE_END_INERT ) {
        // Jog forward from end inert into end bumper
        zdj_decode_make_admin_packet( 
            layer, new_packet, linked_packet,
            ZDJ_DECODE_PACKET_TYPE_END_BUMPER, ZDJ_DECODE_DIR_FWD,
            1000000
        );
        return;
    } else if ( linked_packet->type == ZDJ_DECODE_PACKET_TYPE_START_BUMPER ) {
        // Job forward from start bumper into start inert
        zdj_decode_make_admin_packet( 
            layer, new_packet, linked_packet,
            ZDJ_DECODE_PACKET_TYPE_START_INERT, ZDJ_DECODE_DIR_FWD,
            1000
        );
        return;
    } else if ( linked_packet->type == ZDJ_DECODE_PACKET_TYPE_START_INERT ) {
        // Jog forward from start inert into 1st normal packet
        new_packet->type = ZDJ_DECODE_PACKET_TYPE_NORMAL;
        packet_seek_timestamp = 0;
        av_seek_frame( node_state->fmt_ctx, 0, packet_seek_timestamp, AVSEEK_FLAG_FRAME );
        // printf("fwd first normal packet packet @seek: %ld\n", packet_seek_timestamp );
    } else if ( !linked_packet->has_eof ) {
        // Jog forward from normal packet -> normal packet
        new_packet->type = ZDJ_DECODE_PACKET_TYPE_NORMAL;
        packet_seek_timestamp = linked_packet->packet_pcm_addr + linked_packet->av_frame_sample_count - 1;
        packet_seek_timestamp *= linked_packet->av_timebase_factor; // Modify timebase if mp3.
        av_seek_frame( node_state->fmt_ctx, 0, packet_seek_timestamp, AVSEEK_FLAG_FRAME );
        // printf("fwd normal packet @seek: %ld\n", packet_seek_timestamp );
    } else if ( linked_packet->has_eof ) {
        // Jog forward from normal packet (w/eof) to end inert
        zdj_decode_make_admin_packet( 
            layer, new_packet, linked_packet,
            ZDJ_DECODE_PACKET_TYPE_END_INERT, ZDJ_DECODE_DIR_FWD,
            1000
        );
        return;
    } else {
        // printf( "aft 6\n" );
    }

    // Fill the new packet's av_frame with the next chunk of samples
    int decode_count = zdj_decode_packet( node, new_packet );

    // sof is used to catch first inert packet
    if( packet_seek_timestamp == 0 ) { new_packet->has_sof = true; }

    int64_t packet_pcm_addr = new_packet->av_packet->pts / new_packet->av_timebase_factor;
    int64_t packet_pcm_offset = layer->init_map_pcm - packet_pcm_addr;
    new_packet->packet_pcm_addr = packet_pcm_addr;
    new_packet->packet_decode_addr = linked_packet->packet_decode_addr + linked_packet->av_frame_sample_count - 1;

    // No Discon Case
    // --------------
    if( layer->back_discon.type == ZDJ_DECODE_DISCON_INERT &&
        layer->fwd_discon.type == ZDJ_DECODE_DISCON_INERT 
    ) {
        // printf( "adding no discon packet\n" );
        new_packet->core_start_addr = linked_packet->core_end_addr;
        new_packet->core_end_addr = new_packet->core_start_addr + new_packet->av_frame_sample_count - 1;
    
    // Loop Discon Case
    // --------------
    } else if ( layer->back_discon.type == ZDJ_DECODE_DISCON_LOOP &&
                layer->fwd_discon.type == ZDJ_DECODE_DISCON_LOOP 
    ) {
        // printf( "%p adding loop discon packet\n", layer );
        // printf( "pack aft %ld %ld %ld\n", 
        //     new_packet->packet_decode_addr, 
        //     layer->back_discon.depart_decode_addr, 
        //     new_packet->packet_decode_addr + new_packet->av_frame_sample_count 
        // );
        if( zdj_decode_packet_contains_decode_addr( new_packet, layer->back_discon.depart_decode_addr ) ) {
            new_packet->core_start_addr = layer->back_discon.depart_decode_addr;
            new_packet->is_back_extent = true;
        } else {
            // printf( "add packet after loop\n" );
            new_packet->core_start_addr = linked_packet->packet_decode_addr + linked_packet->av_frame_sample_count - 1;
            new_packet->is_back_extent = false;
        }

        if( zdj_decode_packet_contains_decode_addr( new_packet, layer->fwd_discon.depart_decode_addr ) ) {
            new_packet->core_end_addr = layer->fwd_discon.depart_decode_addr;
            new_packet->is_fwd_extent = true;
        } else {
            new_packet->core_end_addr = new_packet->core_start_addr + new_packet->av_frame_sample_count - 1;
            new_packet->is_fwd_extent = false;
        }
    // Skip Discon Case
    // --------------
    } else if ( layer->back_discon.type == ZDJ_DECODE_DISCON_SKIP ) {
        new_packet->core_start_addr = linked_packet->core_end_addr;
        new_packet->core_end_addr = new_packet->core_start_addr + new_packet->av_frame_sample_count - 1;
    }

    new_packet->core_sample_count = new_packet->core_end_addr - new_packet->core_start_addr;

    layer->latest_core_sample = new_packet->core_end_addr;
    layer->latest_lead_out_sample = new_packet->core_end_addr; // no lead_out during laminar.

    // Add packet to end of layer's packet list
    layer->last_packet = new_packet;
    linked_packet->next = new_packet;
    new_packet->prev = linked_packet;
    new_packet->next = NULL;  

    // No lead_ins/outs during laminar state
    new_packet->lead_in_start_addr = new_packet->core_start_addr;
    new_packet->lead_in_end_addr = new_packet->core_start_addr;
    new_packet->lead_out_start_addr = new_packet->core_end_addr;
    new_packet->lead_out_end_addr = new_packet->core_end_addr;

    // printf( "zdj_decode_add_packet_after_mp3_layer done\n" );
}

void zdj_decode_add_packet_to_empty_mp3_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer ) {
    printf( "zdj_decode_add_packet_to_empty_mp3_layer\n" );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    
    // Stand up new packet
    zdj_decode_packet_t * new_packet = calloc( 1, sizeof( zdj_decode_packet_t ) );
    new_packet->av_timebase_factor = 320;
    new_packet->type = ZDJ_DECODE_PACKET_TYPE_NORMAL;
    
    // Set up some refs based on linking direction (fwd vs. back)
    // zdj_decode_packet_t * linked_packet;
    // int64_t packet_seek_timestamp = layer->init_map_pcm * new_packet->av_timebase_factor;
    // av_seek_frame( node_state->fmt_ctx, 0, packet_seek_timestamp, AVSEEK_FLAG_FRAME );


    // Jump back 2 packets - we need to decode/discard 1 packet before our target packet to flush the decoder.
    int64_t packet_seek_timestamp = layer->init_map_pcm - 2000;

    if( packet_seek_timestamp < 0 ) { printf( "zeroing ts\n" ); packet_seek_timestamp = 0; } // Catch first decode packet
    packet_seek_timestamp *= new_packet->av_timebase_factor; // Modify timebase if mp3.
    av_seek_frame( node_state->fmt_ctx, 0, packet_seek_timestamp, AVSEEK_FLAG_BACKWARD );

    // Decode a garbage packet
    zdj_decode_garbage_packet( node, new_packet );



    // Fill the new packet's av_frame with the next chunk of samples
    int decode_count = zdj_decode_packet( node, new_packet );

    // sof is used to catch first inert packet
    if( packet_seek_timestamp == 0 ) { new_packet->has_sof = true; }

    // Adding packet to layer with no packets
    int64_t packet_pcm_addr = new_packet->av_packet->pts / new_packet->av_timebase_factor;
    int64_t packet_pcm_offset = layer->init_map_pcm - packet_pcm_addr;
    new_packet->packet_pcm_addr = packet_pcm_addr;
    new_packet->packet_decode_addr = layer->init_map_decode + packet_pcm_offset;

    // No Discon Case
    // --------------
    if( layer->back_discon.type == ZDJ_DECODE_DISCON_INERT &&
        layer->fwd_discon.type == ZDJ_DECODE_DISCON_INERT 
    ) {
        new_packet->packet_pcm_addr = packet_pcm_addr;
        new_packet->core_start_addr = new_packet->packet_decode_addr;
        new_packet->core_end_addr = new_packet->core_start_addr + new_packet->av_frame_sample_count - 1;
    
    // Loop Discon Case
    // --------------
    } else if ( layer->back_discon.type == ZDJ_DECODE_DISCON_LOOP &&
                layer->fwd_discon.type == ZDJ_DECODE_DISCON_LOOP 
    ) {
        // If loop start/end fall inside packet, make appropriate settings
        // printf( "pack empt %ld %ld %ld\n", 
        //     new_packet->packet_decode_addr, 
        //     layer->back_discon.depart_decode_addr, 
        //     new_packet->packet_decode_addr + new_packet->av_frame_sample_count 
        // );
        new_packet->packet_pcm_addr = packet_pcm_addr;
        new_packet->core_start_addr = layer->init_map_decode - packet_pcm_offset;
        new_packet->is_back_extent = true;

        if( zdj_decode_packet_contains_decode_addr( new_packet, layer->fwd_discon.depart_decode_addr ) ) {
            new_packet->core_end_addr = layer->fwd_discon.depart_decode_addr;
            new_packet->is_fwd_extent = true;
        } else {
            new_packet->core_end_addr = new_packet->packet_decode_addr + new_packet->av_frame_sample_count - 1;
            new_packet->is_fwd_extent = false;
        }
        
    // Skip Discon Case
    // --------------
    } else if ( layer->back_discon.type == ZDJ_DECODE_DISCON_SKIP ) {
        printf( "skip packet\n" );
        // Reset layer to fill back
        layer->back_discon.type = ZDJ_DECODE_DISCON_INERT;
        // new_packet->packet_pcm_addr = packet_pcm_addr;
        new_packet->core_start_addr = layer->init_map_decode - packet_pcm_offset;
        new_packet->is_back_extent = true;
        new_packet->core_end_addr = new_packet->packet_decode_addr + new_packet->av_frame_sample_count - 1;
    }

    new_packet->core_sample_count = new_packet->core_end_addr - new_packet->core_start_addr;

    layer->earliest_core_sample = new_packet->core_start_addr;
    layer->earliest_lead_in_sample = new_packet->core_start_addr; // no lead_in during laminar.
    layer->latest_core_sample = new_packet->core_end_addr;
    layer->latest_lead_out_sample = new_packet->core_end_addr; // deal w/ fade here.
    
    // Set new packet as start of layer's packet list
    new_packet->next = NULL;
    new_packet->prev = NULL;
    layer->first_packet = new_packet;
    layer->last_packet = new_packet;
    
    // No lead_ins/outs during laminar state
    new_packet->lead_in_start_addr = new_packet->core_start_addr;
    new_packet->lead_in_end_addr = new_packet->core_start_addr;
    new_packet->lead_out_start_addr = new_packet->core_end_addr;
    new_packet->lead_out_end_addr = new_packet->core_end_addr;
    
    // Link in accumulator function once we have decoded the first packet
    switch ( new_packet->av_frame->format ) {
        case AV_SAMPLE_FMT_U8: node_state->accum = &zdj_decode_node_accum_u8; break;
        case AV_SAMPLE_FMT_S16: node_state->accum = &zdj_decode_node_accum_s16; break;
        case AV_SAMPLE_FMT_S32: node_state->accum = &zdj_decode_node_accum_s32; break;
        case AV_SAMPLE_FMT_S64: node_state->accum = &zdj_decode_node_accum_s64; break;
        case AV_SAMPLE_FMT_FLT: node_state->accum = &zdj_decode_node_accum_flt; break;
        case AV_SAMPLE_FMT_DBL: node_state->accum = &zdj_decode_node_accum_dbl; break;
        case AV_SAMPLE_FMT_U8P: node_state->accum = &zdj_decode_node_accum_u8p; break;
        case AV_SAMPLE_FMT_S16P: node_state->accum = &zdj_decode_node_accum_s16p; break;
        case AV_SAMPLE_FMT_S32P: node_state->accum = &zdj_decode_node_accum_s32p; break;
        case AV_SAMPLE_FMT_S64P: node_state->accum = &zdj_decode_node_accum_s64p; break;
        case AV_SAMPLE_FMT_FLTP: node_state->accum = &zdj_decode_node_accum_fltp; break;
        case AV_SAMPLE_FMT_DBLP: node_state->accum = &zdj_decode_node_accum_dblp; break;
        default: node_state->accum = NULL; break;
    }

    printf( "zdj_decode_add_packet_to_empty_mp3_layer done\n" );
}

bool zdj_decode_address_intersects_packet( 
    int64_t address, 
    zdj_decode_packet_t * packet
) {
    return false;
}

bool zdj_pcm_address_intersects_packet( 
    int64_t address, 
    zdj_decode_packet_t * packet
) {
    return false;
}