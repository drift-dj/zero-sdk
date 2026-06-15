#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform.h>
#include <zerodj/signal/pipeline/node/audio/library_decode/zdj_library_decode_node.h>
#include <zerodj/ui/zdj_ui.h>

static void _update_wait( zdj_pipeline_node_t * node );
static void _deinit_state( zdj_pipeline_node_t * node );

// decode_state->out_buffer[ i*decode_state->channel_count ]
static float _norm_flt( float * buf, int index );
static float _norm_s16( float * buf, int index );
static float _norm_u16( float * buf, int index );
static float _norm_s32( float * buf, int index );
static float _norm_u32( float * buf, int index );

zdj_pipeline_node_t * zdj_new_waveform_maker( 
    zdj_pipeline_node_t * decode_node,
    char * filepath,
    int samples_per_point,
    int hi_pass_freq
) {
    zdj_library_decode_node_state_t * decode_state = (zdj_library_decode_node_state_t*)decode_node->state;

    // printf( "%s zdj_new_waveform_maker\n", decode_state->song->audio->filepath );

    zdj_pipeline_node_t * maker = zdj_new_pipeline_node( );
    maker->update_wait = &_update_wait;
    maker->deinit_state = &_deinit_state;

    // Set up state
    zdj_waveform_maker_state_t * state = calloc( 1, sizeof( zdj_waveform_maker_state_t ) );
    state->phase = ZDJ_WAVEFORM_MAKER_PHASE_PREP_WINDOW;
    maker->state = state;

    state->decode_node = decode_node;
    strcpy( state->song_entity_id, decode_state->song->entity_id );

    state->sample_accum = 0.0;
    state->sample_tally = 0;
    state->point_tally = 0;
    state->samples_per_point = samples_per_point;
    
    // Set up the sample normalizer
    switch ( decode_state->song->audio->av_codec_id ) {
        
        case AV_CODEC_ID_MP3:
        case AV_CODEC_ID_AAC:
        case AV_CODEC_ID_FLAC: 
        case AV_CODEC_ID_PCM_F32LE:
        case AV_CODEC_ID_PCM_F32BE: 
            state->norm_fn = _norm_flt; 
            break;

        case AV_CODEC_ID_PCM_S16LE:
        case AV_CODEC_ID_PCM_S16BE: 
            state->norm_fn = _norm_s16; 
            break;

        case AV_CODEC_ID_PCM_U16LE:
        case AV_CODEC_ID_PCM_U16BE: 
            state->norm_fn = _norm_u16; 
            break;

        case AV_CODEC_ID_PCM_S24LE:
        case AV_CODEC_ID_PCM_S24BE:
        case AV_CODEC_ID_PCM_S32LE:
        case AV_CODEC_ID_PCM_S32BE: 
            state->norm_fn = _norm_s32; 
            break;

        case AV_CODEC_ID_PCM_U24LE:
        case AV_CODEC_ID_PCM_U24BE:
        case AV_CODEC_ID_PCM_U32LE:
        case AV_CODEC_ID_PCM_U32BE: 
            state->norm_fn = _norm_u32; 
            break;

        default: 
            state->norm_fn = _norm_flt; 
            break;
    }

    // Set up waveform output file
    state->waveform_header = calloc( 1, sizeof( zdj_waveform_header_t ) );
    strcpy( state->waveform_header->song_entity_id, decode_state->song->entity_id );
    state->waveform_fd = fopen( filepath, "w" );

    if( !state->waveform_fd ) { return NULL; }
    fwrite( state->waveform_header, sizeof( zdj_waveform_header_t ), 1, state->waveform_fd );

    return maker;
}

void zdj_close_waveform_maker( zdj_pipeline_node_t * node ) {
    printf( "zdj_close_waveform_maker: %p\n", node );
    // Update waveform header with point count and re-write
    zdj_waveform_maker_state_t * node_state = (zdj_waveform_maker_state_t*)node->state;
    node_state->waveform_header->frame_count = node_state->point_tally;
    node_state->waveform_header->norm_val = (uint8_t)(node_state->accum_norm * 255.0);
    node_state->waveform_header->samples_per_point = node_state->samples_per_point;

    printf( "writing %d points\n", node_state->waveform_header->frame_count );

    fseek( node_state->waveform_fd, 0, SEEK_SET );
    fwrite( node_state->waveform_header, sizeof( zdj_waveform_header_t ), 1, node_state->waveform_fd );
    fclose( node_state->waveform_fd );

}

// Note waveform maker behavior is undefined if discontinuities are present in decode_node.
static void _update_wait( zdj_pipeline_node_t * node ) {
    // printf( "waveform_maker _update_wait\n" );
    zdj_waveform_maker_state_t * node_state = (zdj_waveform_maker_state_t*)node->state;
    zdj_library_decode_node_state_t * decode_state = (zdj_library_decode_node_state_t*)node_state->decode_node->state;

    // Accumulate available samples from decode node into points.
    for( int i=0; i<decode_state->available_samples; i++ ) {
        float samp = fabs( 
            node_state->norm_fn( decode_state->out_buffer, i*decode_state->channel_count ) 
        );
        if( decode_state->channel_count == 2 ) {
            samp = fmax( samp, fabs(
                node_state->norm_fn( decode_state->out_buffer, i*decode_state->channel_count+1 ) 
            ) );
        }

        node_state->sample_accum += ( (samp - node_state->sample_accum) * 0.8 ) / (float)node_state->samples_per_point;
        node_state->sample_tally++;

        if( node_state->sample_tally == node_state->samples_per_point ) {
            
            uint8_t point = (uint8_t)(node_state->sample_accum * 255.0);
            fwrite( &point, sizeof( uint8_t ), 1, node_state->waveform_fd );

            node_state->accum_norm = fmax( node_state->accum_norm, node_state->sample_accum );
            
            // printf( "pnt:%ld acc:%1.7f nm:%1.3f\n", 
            //     node_state->point_tally, 
            //     node_state->sample_accum,
            //     node_state->accum_norm
            // );
            
            node_state->point_tally++;
            node_state->sample_accum = 0.0;
            node_state->sample_tally = 0;
        }
    }
    // printf( "waveform_maker _update_wait done\n" );
}

static void _deinit_state( zdj_pipeline_node_t * node ) {
    zdj_waveform_maker_state_t * state = (zdj_waveform_maker_state_t*)node->state;
    if( state->waveform_header ) { free( state->waveform_header ); }
}


// Normalize funcs to bound sampls to +- 1.0
static float _norm_flt( float * buf, int index ) {
    // printf( "_norm_flt\n" );
    return buf[ index ];
}

static float _norm_s16( float * buf, int index ) {
    // printf( "_norm_s16\n" );
    return buf[ index ] / (float)SDL_MAX_SINT16;
}

static float _norm_u16( float * buf, int index ) {
    // printf( "_norm_u16\n" );
    return buf[ index ] / (float)SDL_MAX_UINT16;
}

static float _norm_s32( float * buf, int index ) {
    // printf( "_norm_s32\n" );
    return buf[ index ] / (float)SDL_MAX_SINT32;
}

static float _norm_u32( float * buf, int index ) {
    // printf( "_norm_u32\n" );
    return buf[ index ] / (float)SDL_MAX_UINT32;
}