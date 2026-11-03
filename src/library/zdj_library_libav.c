#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <libavcodec/avcodec.h>

#include <zerodj/library/zdj_library.h>

bool zdj_library_supports_av_codec( int av_codec_id ) {
    switch ( av_codec_id ) {
        case AV_CODEC_ID_MP3:
        case AV_CODEC_ID_FLAC:
        case AV_CODEC_ID_PCM_S16LE:
        case AV_CODEC_ID_PCM_S16BE:
        case AV_CODEC_ID_PCM_S24LE:
        case AV_CODEC_ID_PCM_S24BE:
        case AV_CODEC_ID_PCM_F32BE:
        case AV_CODEC_ID_PCM_F32LE:
            return true;
        
        case AV_CODEC_ID_AAC:
        case AV_CODEC_ID_VORBIS:
        default:
            return false;
        
    }
}

bool zdj_library_supports_sample_rate( int sample_rate ) {
    return sample_rate == 44100;
}