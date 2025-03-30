
#include <zerodj/audio/zdj_audio.h

typedef enum {
    ZDJ_AUDIO_ANALYSIS_STATE_IDLE
} zdh_audio_analysis_state_t;

typedef struct {
    zdh_audio_analysis_state_t state;
    float progress;
    bool has_error;
} zdh_audio_analysis_status_t;

typedef struct {
    zdj_audio_file_type_t type;
    zdj_audio_source_t * source;
    zdj_audio_decode_t * decode;
    zdj_library_song_t * data_model;
    zdh_audio_analysis_status_t * status;
    void * thread;
    void * thread_main;
    int thread_priority;
    int core_afinity;=
} zdj_audio_analysis_context_t;

zdj_audio_analysis_context_t * zdj_audio_analysis_new_thread( zdj_audio_source_t * source );
void * zdj_audio_analysis_start_thread( zdj_audio_analysis_context_t * context );
void * zdj_audio_analysis_end_thread( zdj_audio_analysis_context_t * context );