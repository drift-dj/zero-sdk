#ifndef ADD_MUSIC_IMPORT_H
#define ADD_MUSIC_IMPORT_H

typedef enum {
    // States for building the import DB from dir, file, lib, etc.
    IMPORT_MODAL_STATE_DB_INIT_STANDUP,
    IMPORT_MODAL_STATE_DB_INIT_PROCESSING,
    IMPORT_MODAL_STATE_DB_INIT_SUCCESS,
    IMPORT_MODAL_STATE_DB_INIT_ERROR,
    IMPORT_MODAL_STATE_DB_INIT_TEARDOWN,
    // States for processing the import DB - making waveforms, BPM, etc.
    IMPORT_MODAL_STATE_ANALYZE_STANDUP,
    IMPORT_MODAL_STATE_ANALYZE_PROCESSING,
    IMPORT_MODAL_STATE_ANALYZE_SUCCESS,
    IMPORT_MODAL_STATE_ANALYZE_ERROR,
    IMPORT_MODAL_STATE_ANALYZE_TEARDOWN,
    // States for presenting any option UI to user after analysis completes.
    IMPORT_MODAL_STATE_POST_STANDUP,
    IMPORT_MODAL_STATE_POST_OPTIONS,
    IMPORT_MODAL_STATE_POST_PROCESSING,
    IMPORT_MODAL_STATE_POST_SUCCESS,
    IMPORT_MODAL_STATE_POST_ERROR,
    IMPORT_MODAL_STATE_POST_TEARDOWN,
    // Complete, do UI transition back to library root.
    IMPORT_MODAL_STATE_DONE,
    IMPORT_MODAL_STATE_IDLE
} import_modal_phase_t;

typedef struct {
    char path[ 1024 ];
    zdj_view_t * modal_view;
    zdj_view_t * menu_view;
    zdj_view_t * progress_bar;
    zdj_view_t * status_item;
    zdj_library_import_type_t type;
    import_modal_phase_t phase;
    bool should_update;
    float progress;
    int song_tally;
    int error_tally;
    zdj_library_song_t ** song_dtos; // all songs in import db
    int dto_count; // count of all songs in import db
    int active_dto_count; // current number of import threads running
    int dto_index; // index into songs_dtos of last import thread launched
} import_modal_state_t;

extern import_modal_state_t * import_modal_state;

zdj_view_t * new_add_music_import( zdj_library_import_type_t type, char * path );

// DB init phase
// void add_music_import_db_init_ui( import_modal_state_t * state );
void add_music_import_db_init_standup( import_modal_state_t * state );
void add_music_import_db_init_processing( import_modal_state_t * state );
void add_music_import_db_init_success( import_modal_state_t * state );
void add_music_import_db_init_error( import_modal_state_t * state );
void add_music_import_db_init_teardown( import_modal_state_t * state );

// Analyze phase
// void add_music_import_analyze_ui( import_modal_state_t * state );
void add_music_import_analyze_standup( import_modal_state_t * state );
void add_music_import_analyze_processing( import_modal_state_t * state );
void add_music_import_analyze_success( import_modal_state_t * state );
void add_music_import_analyze_error( import_modal_state_t * state );
void add_music_import_analyze_teardown( import_modal_state_t * state );

// Post phase
void add_music_import_post_standup( import_modal_state_t * state );
void add_music_import_post_options( import_modal_state_t * state );
void add_music_import_post_processing( import_modal_state_t * state );
void add_music_import_post_success( import_modal_state_t * state );
void add_music_import_post_error( import_modal_state_t * state );
void add_music_import_post_teardown( import_modal_state_t * state );

#endif