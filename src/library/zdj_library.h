// Copyright (c) 2025 Drift DJ Industries

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ZDJ_LIBRARY_H
#define ZDJ_LIBRARY_H

#include <stdint.h>
#include <stdbool.h>
#include <sqlite3.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/health/zdj_health_type.h>

#define ZDJ_LIBRARY_ENTITY_ID_LEN 37
#define ZDJ_LIBRARY_TABLE_LINK_LEN 256

#define ZDJ_LIBRARY_ARTIST_LEN 256
#define ZDJ_LIBRARY_TITLE_LEN 256
#define ZDJ_LIBRARY_ALBUM_LEN 256
#define ZDJ_LIBRARY_GENRE_LEN 256
#define ZDJ_LIBRARY_LABEL_LEN 256
#define ZDJ_LIBRARY_CUEPOINT_NAME_LEN 128
#define ZDJ_LIBRARY_FILEPATH_LEN 1024

#define ZDJ_LIBRARY_VERSION 1
#define ZDJ_LIBRARY_DIR "/media/internal/library/v1"
#define ZDJ_LIBRARY_DB_PATH "/media/internal/library/v1/zero.db"
#define ZDJ_LIBRARY_IMPORT_DB_PATH "/media/internal/library/v1/zero_import.db"
#define ZDJ_LIBRARY_PLAYBACK_WAVEFORM_DIR "/media/internal/library/v1/playback_waveform"
#define ZDJ_LIBRARY_PLAYBACK_WAVEFORM_TEMP_DIR "/media/internal/library/v1/playback_waveform/tmp"
#define ZDJ_LIBRARY_THUMB_WAVEFORM_DIR "/media/internal/library/v1/thumb_waveform"
#define ZDJ_LIBRARY_THUMB_WAVEFORM_TEMP_DIR "/media/internal/library/v1/thumb_waveform/tmp"

#define ZDJ_LIBRARY_TABLE_LIBRARY_CONFIG            "Library_Config_Entity"
#define ZDJ_LIBRARY_TABLE_LIBRARY                   "Library_Entity"
#define ZDJ_LIBRARY_TABLE_SONG                      "Song_Entity"
#define ZDJ_LIBRARY_TABLE_AUDIO_DATA                "Audio_Data_Entity"
#define ZDJ_LIBRARY_TABLE_AUDIO_PROCEDURAL_EDIT     "Audio_Procedural_Edit_Entity"
#define ZDJ_LIBRARY_TABLE_CATALOG_DATA              "Catalog_Data_Entity"
#define ZDJ_LIBRARY_TABLE_PERFORMANCE_DATA          "Performance_Data_Entity"
#define ZDJ_LIBRARY_TABLE_CURATION_DATA             "Curation_Data_Entity"
#define ZDJ_LIBRARY_TABLE_TAG                       "Tag_Entity"
#define ZDJ_LIBRARY_TABLE_CUEPOINT                  "Cuepoint_Entity"
#define ZDJ_LIBRARY_TABLE_PLAYLIST                  "Playlist_Entity"
#define ZDJ_LIBRARY_TABLE_DATA_SOURCE               "Data_Source_Entity"
#define ZDJ_LIBRARY_TABLE_SETTING					"Setting_Entity"

#define ZDJ_LIBRARY_LINKS_TABLE_SQL					"entity_id TEXT"

#define ZDJ_LIBRARY_DATA_SOURCE_ZERO				"zero"
#define ZDJ_LIBRARY_DATA_SOURCE_ID3					"id3"

// #define ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE 2048

// typedef enum {
// 	ZDJ_LIBRARY_DATA_SOURCE_NONE,
// 	ZDJ_LIBRARY_DATA_SOURCE_ZERO,
// 	ZDJ_LIBRARY_DATA_SOURCE_ID3,
// 	ZDJ_LIBRARY_DATA_SOURCE_RB_XML,
// 	ZDJ_LIBRARY_DATA_SOURCE_ENG_JSON,
// 	ZDJ_LIBRARY_DATA_SOURCE_ENG_DB,
// 	ZDJ_LIBRARY_DATA_SOURCE_SER,
// 	ZDJ_LIBRARY_DATA_SOURCE_MB
// } zdj_library_data_source_ref_t;

// Numeric and chromatic key names are interchangeable
typedef enum {
	ZDJ_LIBRARY_KEY_NUM_NONE,
	ZDJ_LIBRARY_KEY_NUM_11B, 
    ZDJ_LIBRARY_KEY_NUM_8A, 
    ZDJ_LIBRARY_KEY_NUM_4B, 
    ZDJ_LIBRARY_KEY_NUM_1A,
    ZDJ_LIBRARY_KEY_NUM_1B, 
    ZDJ_LIBRARY_KEY_NUM_10A, 
    ZDJ_LIBRARY_KEY_NUM_6B, 
    ZDJ_LIBRARY_KEY_NUM_3A,
    ZDJ_LIBRARY_KEY_NUM_8B, 
    ZDJ_LIBRARY_KEY_NUM_5A,
    ZDJ_LIBRARY_KEY_NUM_10B, 
    ZDJ_LIBRARY_KEY_NUM_7A, 
    ZDJ_LIBRARY_KEY_NUM_3B, 
    ZDJ_LIBRARY_KEY_NUM_12A,
    ZDJ_LIBRARY_KEY_NUM_12B, 
    ZDJ_LIBRARY_KEY_NUM_9A, 
    ZDJ_LIBRARY_KEY_NUM_5B, 
    ZDJ_LIBRARY_KEY_NUM_2A,
    ZDJ_LIBRARY_KEY_NUM_7B, 
    ZDJ_LIBRARY_KEY_NUM_4A, 
    ZDJ_LIBRARY_KEY_NUM_2B, 
    ZDJ_LIBRARY_KEY_NUM_11A,
    ZDJ_LIBRARY_KEY_NUM_9B, 
    ZDJ_LIBRARY_KEY_NUM_6A,
    ZDJ_LIBRARY_KEY_NUM_ANY,
	ZDJ_LIBRARY_KEY_NUM_COUNT
} zdj_library_key_numeric_t;

static char * zdj_library_key_match[ ZDJ_LIBRARY_KEY_NUM_COUNT ][ 3 ] = {
	{ "none", "none", "none" }, // ZDJ_LIBRARY_KEY_NONE,
	{ "a", "amaj", "11B" }, // ZDJ_LIBRARY_KEY_A, 
    { "am", "amin", "8A" }, // ZDJ_LIBRARY_KEY_AM, 
    { "ab", "abmaj", "4B" }, // ZDJ_LIBRARY_KEY_AB, 
    { "amb", "abmin",  "1A" }, // ZDJ_LIBRARY_KEY_ABM,
    { "b", "bmaj", "1B" }, // ZDJ_LIBRARY_KEY_B, 
    { "bm", "bmin", "10A" }, // ZDJ_LIBRARY_KEY_BM, 
    { "bb", "bbmaj", "6B" }, // ZDJ_LIBRARY_KEY_BB, 
    { "bbm", "bbmin", "3A" }, // ZDJ_LIBRARY_KEY_BBM,
    { "c", "cmaj", "8B" }, // ZDJ_LIBRARY_KEY_C, 
    { "cm", "cmin", "5A" }, // ZDJ_LIBRARY_KEY_CM,
    { "d", "dmaj", "10B" }, // ZDJ_LIBRARY_KEY_D, 
    { "dm", "dmin", "7A" }, // ZDJ_LIBRARY_KEY_DM, 
    { "db", "dbmaj", "3B" }, // ZDJ_LIBRARY_KEY_DB, 
    { "dbm", "dbmin", "12A" }, // ZDJ_LIBRARY_KEY_DBM,
    { "e", "emaj", "12B" }, // ZDJ_LIBRARY_KEY_E, 
    { "em", "emin", "9A" }, // ZDJ_LIBRARY_KEY_EM, 
    { "eb", "ebmaj", "5B" }, // ZDJ_LIBRARY_KEY_EB, 
    { "ebm", "ebmin", "2A" }, // ZDJ_LIBRARY_KEY_EBM,
    { "f", "fmaj", "7B" }, // ZDJ_LIBRARY_KEY_F, 
    { "fm", "fmin", "4A" }, // ZDJ_LIBRARY_KEY_FM, 
    { "f#", "f#maj", "2B" }, // ZDJ_LIBRARY_KEY_FS, 
    { "f#m", "f#min", "11A" }, // ZDJ_LIBRARY_KEY_FSM,
    { "g", "gmaj", "9B" }, // ZDJ_LIBRARY_KEY_G, 
    { "gm", "gmin", "6A" }, // ZDJ_LIBRARY_KEY_GM,
    { "any", "any", "any" } // ZDJ_LIBRARY_KEY_ANY,
};

typedef enum {
	ZDJ_LIBRARY_KEY_NONE,
	ZDJ_LIBRARY_KEY_A, 
    ZDJ_LIBRARY_KEY_AM, 
    ZDJ_LIBRARY_KEY_AB, 
    ZDJ_LIBRARY_KEY_ABM,
    ZDJ_LIBRARY_KEY_B, 
    ZDJ_LIBRARY_KEY_BM, 
    ZDJ_LIBRARY_KEY_BB, 
    ZDJ_LIBRARY_KEY_BBM,
    ZDJ_LIBRARY_KEY_C, 
    ZDJ_LIBRARY_KEY_CM,
    ZDJ_LIBRARY_KEY_D, 
    ZDJ_LIBRARY_KEY_DM, 
    ZDJ_LIBRARY_KEY_DB, 
    ZDJ_LIBRARY_KEY_DBM,
    ZDJ_LIBRARY_KEY_E, 
    ZDJ_LIBRARY_KEY_EM, 
    ZDJ_LIBRARY_KEY_EB, 
    ZDJ_LIBRARY_KEY_EBM,
    ZDJ_LIBRARY_KEY_F, 
    ZDJ_LIBRARY_KEY_FM, 
    ZDJ_LIBRARY_KEY_FS, 
    ZDJ_LIBRARY_KEY_FSM,
    ZDJ_LIBRARY_KEY_G, 
    ZDJ_LIBRARY_KEY_GM,
    ZDJ_LIBRARY_KEY_ANY,
	ZDJ_LIBRARY_KEY_COUNT
} zdj_library_key_t;

static char * zdj_library_key_name[ ZDJ_LIBRARY_KEY_COUNT ] = {
	"None", // ZDJ_LIBRARY_KEY_NONE,
	"A", // ZDJ_LIBRARY_KEY_A, 
    "Amin", // ZDJ_LIBRARY_KEY_AM, 
    "Ab", // ZDJ_LIBRARY_KEY_AB, 
    "Abmin", // ZDJ_LIBRARY_KEY_ABM,
    "B", // ZDJ_LIBRARY_KEY_B, 
    "Bmin", // ZDJ_LIBRARY_KEY_BM, 
    "Bb", // ZDJ_LIBRARY_KEY_BB, 
    "Bbmin", // ZDJ_LIBRARY_KEY_BBM,
    "C", // ZDJ_LIBRARY_KEY_C, 
    "Cmin", // ZDJ_LIBRARY_KEY_CM,
    "D", // ZDJ_LIBRARY_KEY_D, 
    "Dmin", // ZDJ_LIBRARY_KEY_DM, 
    "Db", // ZDJ_LIBRARY_KEY_DB, 
    "Dbmin", // ZDJ_LIBRARY_KEY_DBM,
    "E", // ZDJ_LIBRARY_KEY_E, 
    "Emin", // ZDJ_LIBRARY_KEY_EM, 
    "Eb", // ZDJ_LIBRARY_KEY_EB, 
    "Ebmin", // ZDJ_LIBRARY_KEY_EBM,
    "F", // ZDJ_LIBRARY_KEY_F, 
    "Fmin", // ZDJ_LIBRARY_KEY_FM, 
    "F#", // ZDJ_LIBRARY_KEY_FS, 
    "F#min", // ZDJ_LIBRARY_KEY_FSM,
    "G", // ZDJ_LIBRARY_KEY_G, 
    "Gmin", // ZDJ_LIBRARY_KEY_GM,
    "Any" // ZDJ_LIBRARY_KEY_ANY,
};

typedef enum {
	ZDJ_LIBRARY_CAMELOT_NONE,
	ZDJ_LIBRARY_CAMELOT_EQUAL,
	ZDJ_LIBRARY_CAMELOT_PLUS,
	ZDJ_LIBRARY_CAMELOT_PLUS_PLUS,
	ZDJ_LIBRARY_CAMELOT_MINUS,
	ZDJ_LIBRARY_CAMELOT_MINUS_MINUS,
	ZDJ_LIBRARY_CAMELOT_DELTA,
	ZDJ_LIBRARY_CAMELOT_TILDA,
	ZDJ_LIBRARY_CAMELOT_ANY
} zdj_library_camelot_t;

typedef enum {
	ZDJ_LIBRARY_BPM_UNKOWN,
	ZDJ_LIBRARY_BPM_0_40,
	ZDJ_LIBRARY_BPM_40_100,
	ZDJ_LIBRARY_BPM_100_130,
	ZDJ_LIBRARY_BPM_130_145,
	ZDJ_LIBRARY_BPM_145_180,
	ZDJ_LIBRARY_BPM_180,
	ZDJ_LIBRARY_BPM_COUNT
} zdj_library_bpm_range_t;

static char * zdj_library_bpm_name[ ZDJ_LIBRARY_BPM_COUNT ] = {
	"Unknown BPM", // ZDJ_LIBRARY_BPM_UNKOWN,
	"0 - 40 BPM", // ZDJ_LIBRARY_BPM_0_40,
	"40 - 100 BPM", // ZDJ_LIBRARY_BPM_40_100,
	"100 - 130 BPM", // ZDJ_LIBRARY_BPM_100_130,
	"130 - 145 BPM", // ZDJ_LIBRARY_BPM_130_145,
	"145 - 180 BPM", // ZDJ_LIBRARY_BPM_145_180,
	">180 BPM" // ZDJ_LIBRARY_BPM_180
};

typedef enum {
	ZDJ_LIBRARY_ANALYSIS_STATE_NONE,
	ZDJ_LIBRARY_ANALYSIS_STATE_INIT,
	ZDJ_LIBRARY_ANALYSIS_STATE_PRESCAN,
	ZDJ_LIBRARY_ANALYSIS_STATE_SCAN,
	ZDJ_LIBRARY_ANALYSIS_STATE_RUNNING,
	ZDJ_LIBRARY_ANALYSIS_STATE_DONE,
	ZDJ_LIBRARY_ANALYSIS_STATE_SKIP,
	ZDJ_LIBRARY_ANALYSIS_STATE_IDLE
} zdj_library_analysis_state_t;

typedef struct {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	int entity_counter;
	char current_lib_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
} zdj_library_config_t;

typedef struct zdj_library_t {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char name[ 256 ];
	char song_links_table[ ZDJ_LIBRARY_TABLE_LINK_LEN ];
	char playlist_links_table[ ZDJ_LIBRARY_TABLE_LINK_LEN ];
	int playlist_count;
	char ** playlist_table_names;
	char ** playlist_titles;
	char setting_links_table[ ZDJ_LIBRARY_TABLE_LINK_LEN ];
} zdj_library_t;

typedef struct {
	char * table_name;
	int count;
	void ** catalogs;
} zdj_library_links_t;

typedef struct {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char name[ 128 ];
} zdj_library_data_source_t;

typedef struct {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char filepath[ ZDJ_LIBRARY_FILEPATH_LEN ];
	char file_checksum[ 64 ];
	char song_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char data_source_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	bool has_procedural_edit;
	char procedural_edit_filepath[ 256 ];
	bool has_libav_error;
	int libav_error;
	int av_codec_id;
	int av_stream_index;
	int av_sample_rate;
	int av_sample_format;
	int av_channel_count;
	double duration_sec;
	int64_t duration_pcm;
	double timebase;
	int decoded_sample_count;
	sqlite3_stmt * store_stmt;
	sqlite3_stmt * fetch_stmt;
	zdj_health_status_t error;
} zdj_library_audio_t;

typedef enum {
	ZDJ_LIBRARY_AUDIO_PROCEDURAL_EDIT_OP_SPLICE
} zdj_library_audio_procedural_edit_op_t;

typedef struct {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	zdj_library_audio_procedural_edit_op_t op;
	int edit_in_sample;
	int edit_out_sample;
} zdj_library_audio_procedural_edit_step_t;

typedef struct {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char song_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char data_source_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char title[ ZDJ_LIBRARY_TITLE_LEN ];
	char artist[ ZDJ_LIBRARY_ARTIST_LEN ];
	char album[ ZDJ_LIBRARY_ALBUM_LEN ];
	char label[ ZDJ_LIBRARY_LABEL_LEN ];
	char genre[ ZDJ_LIBRARY_GENRE_LEN ];
	int year;
	sqlite3_stmt * store_stmt;
	sqlite3_stmt * fetch_stmt;
	zdj_health_status_t error;
} zdj_library_catalog_t;

typedef struct {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char song_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char data_source_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	int sample_length;
	zdj_library_key_t key;
	float bpm;
	bool has_beat_grid;
	int beat_grid_start_sample;
	int cuepoint_count;
	int current_cuepoint;
	struct zdj_library_cuepoint_t ** cuepoints;
	char cuepoints_links_table[ ZDJ_LIBRARY_ENTITY_ID_LEN ]; // deprecated
	zdj_health_status_t error;
} zdj_library_performance_t;

typedef struct zdj_library_cuepoint_t {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char performance_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char data_source_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char name[ ZDJ_LIBRARY_CUEPOINT_NAME_LEN ];
	int num;
	int64_t sample;
	bool is_loop;
	int64_t loop_len;
	struct zdj_library_cuepoint_t * next;
    struct zdj_library_cuepoint_t * prev;
} zdj_library_cuepoint_t;

typedef struct {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char parent_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ]; // Can link to a library or song
	char data_source_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	int playlist_count;
	char ** playlist_entity_ids;
	char playlists_links_table[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	int tag_count;
	char ** tag_entity_ids;
	char tags_links_table[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	zdj_health_status_t error;
} zdj_library_curation_t;

typedef struct {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char data_source_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char name[ 256 ];
} zdj_library_tag_t;

typedef struct zdj_library_playlist_t {
	char table_name[ ZDJ_LIBRARY_TABLE_LINK_LEN ]; // Playlists are stored as an entire table
	char data_source_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char title[ 256 ];
	int row_count;
	void * row_head;
	void * row_tail;
	struct zdj_library_playlist_t * next;
	struct zdj_library_playlist_t * prev;
} zdj_library_playlist_t;

typedef enum {
	ZDJ_LIBRARY_SONG_ERROR_FLAG_BAD_FORMAT,
	ZDJ_LIBRARY_SONG_ERROR_FLAG_BAD_ENCODING,
	ZDJ_LIBRARY_SONG_ERROR_FLAG_COPY_FAILED,
	ZDJ_LIBRARY_SONG_ERROR_FLAG_FILE_MISSING,
	ZDJ_LIBRARY_SONG_ERROR_FLAG_FILE_NOACCESS,
	ZDJ_LIBRARY_SONG_ERROR_FLAG_DECODE_FAILED,
} zdj_library_song_error_t;

typedef struct {
	bool can_merge;
	bool should_merge;
	int merge_option;
	char merge_source[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
} zdj_library_song_import_context_t;

typedef struct zdj_library_song_t {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char catalog_links_table[ ZDJ_LIBRARY_TABLE_LINK_LEN ];
	char current_catalog_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	zdj_library_catalog_t * catalog;
	zdj_library_links_t * catalog_links;
	char performance_links_table[ ZDJ_LIBRARY_TABLE_LINK_LEN ];
	char current_performance_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	zdj_library_performance_t * performance;
	zdj_library_links_t * performance_links;
	char curation_links_table[ ZDJ_LIBRARY_TABLE_LINK_LEN ];
	char current_curation_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	zdj_library_curation_t * curation; // optional - only used in edit workflows
	zdj_library_links_t * curation_links;
	char audio_links_table[ ZDJ_LIBRARY_TABLE_LINK_LEN ];
	char current_audio_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	zdj_library_audio_t * audio;
	zdj_library_links_t * audio_links;
	zdj_library_analysis_state_t analysis_state;  // optional - only used during lib import workflows
	float analysis_progress;
	zdj_library_song_import_context_t import_context;
	bool has_error;
	int error_flags;
	bool has_db_error; // runtime-only flag for DB-related problems
	struct zdj_library_song_t * next;
	struct zdj_library_song_t * prev;
} zdj_library_song_t;

typedef struct zdj_library_menu_row_t {
	char song_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char artist[ ZDJ_LIBRARY_ARTIST_LEN ];
	char title[ ZDJ_LIBRARY_TITLE_LEN ];
	char genre[ ZDJ_LIBRARY_GENRE_LEN ];
	int year;
	char filepath[ ZDJ_LIBRARY_FILEPATH_LEN ];
	float bpm;
	int key;
	bool has_error;
	int error_flags;
	int num;
	bool is_section;
	struct zdj_library_menu_row_t * next;
	struct zdj_library_menu_row_t * prev;
} zdj_library_menu_row_t;

typedef enum {
    ZDJ_LIBRARY_IMPORT_TYPE_UNKNOWN,
	ZDJ_LIBRARY_IMPORT_TYPE_AUDIO_FILE,
    ZDJ_LIBRARY_IMPORT_TYPE_AUDIO_DIR,
	ZDJ_LIBRARY_IMPORT_TYPE_LIBRARY_FILE,
    ZDJ_LIBRARY_IMPORT_TYPE_LIBRARY_DIR
} zdj_library_import_type_t;

typedef enum {
    ZDJ_LIBRARY_IMPORT_FILE_TYPE_UNKNOWN,
	ZDJ_LIBRARY_IMPORT_FILE_TYPE_AIF,
	ZDJ_LIBRARY_IMPORT_FILE_TYPE_WAV,
	ZDJ_LIBRARY_IMPORT_FILE_TYPE_MP3,
	ZDJ_LIBRARY_IMPORT_FILE_TYPE_FLAC,
	ZDJ_LIBRARY_IMPORT_FILE_TYPE_OGG,
	ZDJ_LIBRARY_IMPORT_FILE_TYPE_REKORDBOX_XML,
	ZDJ_LIBRARY_IMPORT_FILE_TYPE_ENGINEDJ_DB,
	ZDJ_LIBRARY_IMPORT_FILE_TYPE_ENGINEDJ_JSON
} zdj_library_import_file_type_t;

typedef enum {
	ZDJ_LIBRARY_SETTING_SHOW_PLAYLISTS_MENU,
	ZDJ_LIBRARY_SETTING_SHOW_ARTISTS_MENU,
	ZDJ_LIBRARY_SETTING_SHOW_ALBUMS_MENU,
	ZDJ_LIBRARY_SETTING_SHOW_LABELS_MENU,
	ZDJ_LIBRARY_SETTING_SHOW_GENRES_MENU,
	ZDJ_LIBRARY_SETTING_SHOW_YEARS_MENU,
	ZDJ_LIBRARY_SETTING_SHOW_BPM_MENU,
	ZDJ_LIBRARY_SETTING_SCREENSHOT_COUNTER
} zdj_library_setting_type_t;

typedef struct {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	zdj_library_setting_type_t type;
	int i_val;
	float f_val;
	bool b_val;
	char c_val[ 256 ];
} zdj_library_setting_t;

extern sqlite3 * zdj_library_db;
extern sqlite3 * zdj_library_import_db;
extern zdj_library_config_t * library_config;

// char * zdj_library_get_uuid( void );
void zdj_library_put_uuid( char * str );

// Library Init
// zdj_health_status_t zdj_library_init( void );
zdj_health_status_t zdj_library_health( void );

// Library DB API
// zdj_health_status_t zdj_library_db_init( void );
zdj_health_status_t zdj_import_db_flush( void );
zdj_health_status_t zdj_library_db_flush( void );

// Import DB
zdj_library_import_type_t zdj_library_get_import_type_for_path( char * path );
zdj_health_status_t zdj_library_new_import_db( void );
zdj_health_status_t zdj_library_open_import_db( void );
zdj_health_status_t zdj_library_close_import_db( void );

// Main Library DB
void zdj_library_reset_db( void );
zdj_health_status_t zdj_library_open_db( void );
zdj_health_status_t zdj_library_close_db( void );
void zdj_library_create_db_tables( sqlite3 * db );


// Config
void zdj_library_new_config( void );
zdj_library_config_t * zdj_library_get_config( void );
char * zdj_library_config_get_current_library_id( void );
void zdj_library_config_put_current_library_id( char * dest );
zdj_health_status_t zdj_library_config_set_current_library_id( char * entity_id );
zdj_error_type_t zdj_library_generate_stress_test_library( int song_count );

// Library
int zdj_library_count_libraries( void );
void zdj_library_get_all_libraries( 
    zdj_library_t ** libraries, 
    int result_limit
);
zdj_library_t * zdj_library_get_current( void );
zdj_library_t * zdj_library_fetch_library_dto_for_entity_id( char * library_entity_id, sqlite3 * db );
zdj_health_status_t zdj_library_new( void );
zdj_health_status_t zdj_library_duplicate_library( char * library_entity_id );
zdj_health_status_t zdj_library_remove_library( char * library_entity_id );
zdj_health_status_t zdj_library_rename_library( char * library_entity_id, char * name );
zdj_health_status_t zdj_library_persist( zdj_library_t * library );
zdj_health_status_t zdj_library_add_song_link( char * library_entity_id, zdj_library_song_t * song, sqlite3 * db );
zdj_error_type_t zdj_library_populate_playlists_for_library(
    zdj_library_t * library, sqlite3 * db 
);
void zdj_library_deinit_library( zdj_library_t * library );

// Settings
zdj_library_setting_t * zdj_library_get_setting( 
	char * library_entity_id, zdj_library_setting_type_t setting 
);
zdj_health_status_t zdj_library_set_int_setting( 
	char * library_entity_id, zdj_library_setting_type_t setting, int val 
);
zdj_health_status_t zdj_library_set_bool_setting( 
	char * library_entity_id, zdj_library_setting_type_t setting, bool val 
);
zdj_health_status_t zdj_library_set_float_setting( 
	char * library_entity_id, zdj_library_setting_type_t setting, float val 
);
zdj_health_status_t zdj_library_set_char_setting( 
	char * library_entity_id, zdj_library_setting_type_t setting, char * val 
);
void zdj_library_deinit_setting( zdj_library_setting_t * setting );


// Library Data Model API
zdj_health_status_t zdj_library_store_links_table( char * links_table_name, sqlite3 * db );

// Song Graph
zdj_library_song_t * zdj_library_create_file_import_song_graph( char * filepath, sqlite3 * db );
zdj_library_song_t * zdj_library_fetch_file_import_song_graph( char * song_entity_id, sqlite3 * db );
zdj_library_song_t * zdj_library_create_rb_xml_import_song_graph( char * track_id, sqlite3 * db );
zdj_library_song_t * zdj_library_fetch_rb_xml_import_song_graph( zdj_library_song_t * song, sqlite3 * db );
zdj_library_song_t * zdj_library_fetch_menu_song_graph( zdj_library_song_t * song, sqlite3 * db );
zdj_library_song_t * zdj_library_fetch_playback_song_graph( zdj_library_song_t * song, sqlite3 * db );
zdj_library_song_t * zdj_library_fetch_edit_song_graph( char * song_entity_id, sqlite3 * db );
zdj_library_song_t * zdj_library_fetch_migration_song_graph( char * song_entity_id, sqlite3 * db );
zdj_health_status_t zdj_library_free_song_graph( zdj_library_song_t * song );
zdj_health_status_t zdj_library_store_song_graph( zdj_library_song_t * song, sqlite3 * db );
zdj_health_status_t zdj_library_delete_song_graph( zdj_library_song_t * song, sqlite3 * db );

// Song
zdj_library_song_t * zdj_library_create_song_dto( void );
zdj_health_status_t zdj_library_free_song_dto( zdj_library_song_t * song );
zdj_health_status_t zdj_library_store_song( zdj_library_song_t * song, sqlite3 * db );
zdj_health_status_t zdj_library_delete_song( zdj_library_song_t * song, sqlite3 * db );

zdj_library_song_t * zdj_library_fetch_song_dto_for_entity_id( char * entity_id, sqlite3 * db );
zdj_health_status_t zdj_library_fetch_song_entity_ids( char ** arr, int count, sqlite3 * db );
int zdj_library_count_songs_in_library( char * library_entity_id, sqlite3 * db );
int zdj_library_count_songs_in_db( sqlite3 * db );
bool zdj_library_song_can_play( zdj_library_song_t * song );
bool zdj_library_song_has_error_flag( zdj_library_song_t * song, zdj_library_song_error_t flag );

zdj_library_catalog_t * zdj_library_create_catalog_dto( void );
zdj_library_catalog_t * zdj_library_fetch_current_catalog_dto_for_song( zdj_library_song_t * song, sqlite3 * db );
zdj_health_status_t zdj_library_free_catalog_dto( zdj_library_catalog_t * catalog );
zdj_health_status_t zdj_library_store_catalog( zdj_library_catalog_t * catalog, sqlite3 * db );
zdj_health_status_t zdj_library_delete_catalog( zdj_library_catalog_t * catalog, sqlite3 * db );

zdj_library_performance_t * zdj_library_create_performance_dto( void );
zdj_library_performance_t * zdj_library_fetch_current_performance_dto_for_song( zdj_library_song_t * song, sqlite3 * db );
zdj_health_status_t zdj_library_free_performance_dto( zdj_library_performance_t * performance );
zdj_health_status_t zdj_library_store_performance( zdj_library_performance_t * performance, sqlite3 * db );
zdj_health_status_t zdj_library_delete_performance( zdj_library_performance_t * performance, sqlite3 * db );

void zdj_library_merge_song_with_entity_id( zdj_library_song_t * song, char * eid, sqlite3 * db );			
void zdj_library_replace_entity_id_with_song( zdj_library_song_t * song, char * eid, sqlite3 * db );

// Audio
zdj_library_audio_t * zdj_library_create_audio_dto( void );
zdj_library_audio_t * zdj_library_fetch_current_audio_dto_for_song( zdj_library_song_t * song, sqlite3 * db );
zdj_health_status_t zdj_library_free_audio_dto( zdj_library_audio_t * audio );
zdj_health_status_t zdj_library_store_audio( zdj_library_audio_t * audio, sqlite3 * db );
zdj_health_status_t zdj_library_delete_audio( zdj_library_audio_t * audio, sqlite3 * db );
zdj_error_type_t zdj_library_audio_parse_encoding_info( zdj_library_audio_t * audio );
bool zdj_library_audio_is_raw_pcm( zdj_library_audio_t * audio );
char * zdj_library_audio_file_crc( char * path );

// Data Source
void zdj_library_create_default_data_sources( void );
zdj_library_data_source_t * zdj_library_fetch_data_source_for_entity_id( char * entity_id, sqlite3 * db );
void zdj_library_store_data_source( zdj_library_data_source_t * data_source, sqlite3 * db );

// Curation
zdj_library_curation_t * zdj_library_create_curation_dto( void );
zdj_library_curation_t * zdj_library_fetch_current_curation_dto_for_song( zdj_library_song_t * song, sqlite3 * db );
zdj_error_state_t zdj_library_fetch_curation_links_for_dto( zdj_library_curation_t * curation, sqlite3 * db );
zdj_health_status_t zdj_library_free_curation_dto( zdj_library_curation_t * curation );
zdj_health_status_t zdj_library_store_curation( zdj_library_curation_t * curation, sqlite3 * db );
zdj_health_status_t zdj_library_delete_curation( zdj_library_curation_t * curation, sqlite3 * db );

zdj_library_playlist_t * zdj_library_create_playlist_dto( void );
bool zdj_library_playlist_title_exists_in_db( char * title, char * table_name, sqlite3 * db );
void zdj_library_merge_playlist( zdj_library_playlist_t * playlist, char * merge_source, sqlite3 * db );
void zdj_library_replace_playlist( zdj_library_playlist_t * playlist, char * target_table_name, sqlite3 * db );
zdj_error_type_t zdj_library_store_playlist( 
    char * library_entity_id, 
    zdj_library_playlist_t * playlist, 
    sqlite3 * db 
);
zdj_error_type_t zdj_library_delete_playlist( char * library_entity_id, zdj_library_playlist_t * playlist, sqlite3 * db );
zdj_error_type_t zdj_library_delete_playlist_named( 
	char * playlist_table_name, sqlite3 * db 
);

zdj_library_playlist_t * zdj_library_make_playlist_dto_for_table_name( char * library_entity_id, char * playlist_table_name, sqlite3 * db );
void zdj_library_playlist_append_row( 
    zdj_library_playlist_t * playlist, 
    zdj_library_menu_row_t * row
);
void zdj_library_put_playlist_name_for_playlist( zdj_library_playlist_t * playlist, sqlite3 * db );
void zdj_library_playlist_remove_row( 
    zdj_library_playlist_t * playlist, 
    zdj_library_menu_row_t * row
);
void zdj_library_playlist_move_row( 
    zdj_library_playlist_t * playlist, 
    zdj_library_menu_row_t * row, 
    int index
);
void zdj_library_free_playlist_dto( zdj_library_playlist_t * playlist, bool free_rows );

zdj_error_state_t zdj_library_populate_playlist_eids_for_song(
    zdj_library_song_t * song,
    sqlite3 * db 
);

// Performance
zdj_library_cuepoint_t * zdj_library_create_cuepoint_dto( void );
zdj_library_cuepoint_t * zdj_library_fetch_cuepoint_dto_for_entity_id( char * entity_id, sqlite3 * db );
void zdj_library_free_cuepoint_dto( zdj_library_cuepoint_t * cuepoint );
void zdj_library_remove_cuepoints_for_performance_eid( char * perf_eid, sqlite3 * db );
void zdj_library_remove_cuepoint( zdj_library_cuepoint_t * cuepoint );
zdj_error_type_t zdj_library_store_cuepoint( zdj_library_cuepoint_t * cuepoint, sqlite3 * db );
int zdj_library_get_key_for_str( char * str );

// Recordings
void zdj_library_remove_all_recordings( void );


// Query
void zdj_library_refresh_menu_query_table( sqlite3 * db );

int zdj_library_query_count_all_songs( sqlite3 * db );
bool zdj_library_query_song_eid_exists( char * song_eid, sqlite3 * db );
bool zdj_library_query_import_song_match( zdj_library_song_t * song, char * match_eid, sqlite3 * db );

zdj_error_type_t zdj_library_query_artist_menu( 
	zdj_library_menu_row_t ** rows, 
	sqlite3 * db 
);

zdj_error_type_t zdj_library_query_bpm_menu( 
	zdj_library_menu_row_t ** rows, 
	int count,
	sqlite3 * db 
);

int zdj_library_query_count_all_genres( sqlite3 * db );
zdj_error_type_t zdj_library_query_genres_menu( 
	char ** rows,
	int count,
	sqlite3 * db 
);
int zdj_library_query_count_songs_in_genre( sqlite3 * db, char * genre );
zdj_error_type_t zdj_library_query_genre_menu( 
	zdj_library_menu_row_t ** rows, 
	char * genre,
	int count,
	sqlite3 * db 
);

int zdj_library_query_count_songs_in_key( sqlite3 * db, int key );
zdj_error_type_t zdj_library_query_key_menu( 
	zdj_library_menu_row_t ** rows, 
	int key,
	int count,
	sqlite3 * db 
);

int zdj_library_query_count_all_years( 
	char * library_entity_id, 
	sqlite3 * db 
);
zdj_error_type_t zdj_library_query_all_years( 
	char * library_entity_id, 
	int * years, 
	int count,
	sqlite3 * db 
);
int zdj_library_query_count_songs_in_year( 
	char * library_entity_id, 
	int year,
	sqlite3 * db 
);
zdj_error_type_t zdj_library_query_songs_in_year( 
	char * library_entity_id, 
	int year,
	zdj_library_song_t ** songs, 
	int count, 
	sqlite3 * db 
);

int zdj_library_query_count_all_playlists( 
	char * library_entity_id, 
	sqlite3 * db 
);
zdj_error_type_t zdj_library_query_all_playlists( 
	char * library_entity_id, 
	char ** playlist_eids, 
	int count,
	sqlite3 * db 
);

int zdj_library_query_count_cuepoints_for_song( 
	zdj_library_song_t * song, 
	sqlite3 * db 
);
zdj_error_type_t zdj_library_query_cuepoints_for_song( 
    zdj_library_song_t * song, 
    char ** cuepoints, 
    int count, 
    sqlite3 * db 
);

zdj_library_key_t zdj_library_get_key_for_id3( char * key_str );
zdj_library_camelot_t zdj_library_get_camelot( 
	zdj_library_key_t current_key, 
	zdj_library_key_t new_key 
);

#endif