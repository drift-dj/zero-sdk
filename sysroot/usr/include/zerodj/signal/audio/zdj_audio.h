
typedef enum {
    ZDJ_AUDIO_ANALYSIS_TYPE_FILE_MP3,
    ZDJ_AUDIO_ANALYSIS_TYPE_FILE_WAV,
    ZDJ_AUDIO_ANALYSIS_TYPE_FILE_AIF,
    ZDJ_AUDIO_ANALYSIS_TYPE_FILE_OGG,
    ZDJ_AUDIO_ANALYSIS_TYPE_FILE_FLAC
} zdj_audio_file_type_t;

typedef struct {
    char * path;
    FILE * fp;
} zdj_audio_source_t;

typedef enum {
    ZDJ_AUDIO_DECODE_MPG123,
    ZDJ_AUDIO_DECODE_FFMPG
} zdj_audio_decode_type_t;

typedef struct {
    zdj_audio_decode_type_t type;
    void * handle;
    void * data;
} zdj_audio_decode_t;