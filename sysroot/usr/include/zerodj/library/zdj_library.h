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

#include <stdbool.h>
#include <sqlite3.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/health/zdj_health_type.h>

#define ZDJ_LIBRARY_ENTITY_ID_LEN 37
#define ZDJ_LIBRARY_TABLE_LINK_LEN 256

#define ZDJ_LIBRARY_DB_PATH "/etc/zero_data/zero.db"
#define ZDJ_LIBRARY_IMPORT_DB_PATH "/etc/zero_data/zero_import.db"
#define ZDJ_LIBRARY_PLAYBACK_WAVEFORM_DIR "/etc/zero_data/playback_waveform"
#define ZDJ_LIBRARY_PLAYBACK_WAVEFORM_TEMP_DIR "/etc/zero_data/playback_waveform/tmp"
#define ZDJ_LIBRARY_THUMB_WAVEFORM_DIR "/etc/zero_data/thumb_waveform"
#define ZDJ_LIBRARY_THUMB_WAVEFORM_TEMP_DIR "/etc/zero_data/thumb_waveform/tmp"

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

typedef enum {
	ZDJ_LIBRARY_KEY_1A
} zdj_library_key_t;

typedef enum {
	ZDJ_LIBRARY_BPM_0_40,
	ZDJ_LIBRARY_BPM_40_100,
	ZDJ_LIBRARY_BPM_100_130,
	ZDJ_LIBRARY_BPM_130_145,
	ZDJ_LIBRARY_BPM_145_180,
	ZDJ_LIBRARY_BPM_180
} zdj_library_bpm_range_t;

typedef enum {
	ZDJ_LIBRARY_ANALYSIS_STATE_NONE,
	ZDJ_LIBRARY_ANALYSIS_STATE_INIT,
	ZDJ_LIBRARY_ANALYSIS_STATE_PRESCAN,
	ZDJ_LIBRARY_ANALYSIS_STATE_SCAN,
	ZDJ_LIBRARY_ANALYSIS_STATE_RUNNING,
	ZDJ_LIBRARY_ANALYSIS_STATE_DONE,
	ZDJ_LIBRARY_ANALYSIS_STATE_IDLE
} zdj_library_analysis_state_t;

typedef struct {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	int entity_counter;
	char current_lib_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
} zdj_library_config_t;

typedef struct zdj_library_t {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char * name;
	char * song_links;
	char * curation_data_links;
	char * setting_links;
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
	char filepath[ 256 ];
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
	double duration;
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
	char title[ 256 ];
	char artist[ 256 ];
	char album[ 256 ];
	char label[ 256 ];
	char genre[ 256 ];
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
	struct zdj_library_cuepoint_t * cuepoints;
	char cuepoints_links_table[ ZDJ_LIBRARY_ENTITY_ID_LEN ]; // holds only selected cuepoints
	zdj_health_status_t error;
} zdj_library_performance_t;

typedef struct {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char performance_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char data_source_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	int64_t sample;
	bool is_loop;
	int64_t loop_len;
} zdj_library_cuepoint_t;

typedef struct {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char parent_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ]; // Can link to a library or song
	char data_source_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	struct zdj_library_playlist_t * playlists;
	char playlists_links_table[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	struct zdj_library_tag_t * tags;
	char tags_links_table[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	zdj_health_status_t error;
} zdj_library_curation_t;

typedef struct {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char data_source_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char name[ 256 ];
} zdj_library_tag_t;

typedef struct {
	char entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char data_source_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
	char title[ 256 ];
} zdj_library_playlist_t;

typedef struct {
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
	bool has_error;
	int error_flags;
} zdj_library_song_t;

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
	ZDJ_LIBRARY_SETTING_SHOW_BPM_MENU
} zdj_library_setting_type_t;

typedef struct {
	char * entity_id;
	zdj_library_setting_type_t type;
	int i_val;
	float f_val;
	bool b_val;
	char * c_val;
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


// Config
void zdj_library_new_config( void );
zdj_library_config_t * zdj_library_get_config( void );
char * zdj_library_config_get_current_library_id( void );
zdj_health_status_t zdj_library_config_set_current_library_id( char * entity_id );

// Library
int zdj_library_count_libraries( void );
void zdj_library_get_all_libraries( 
    zdj_library_t ** libraries, 
    int result_limit
);
zdj_library_t * zdj_library_get_current( void );
zdj_library_t * zdj_library_get_library_for_entity_id( char * library_entity_id );
zdj_health_status_t zdj_library_new( void );
zdj_health_status_t zdj_library_duplicate_library( char * library_entity_id );
zdj_health_status_t zdj_library_remove_library( char * library_entity_id );
zdj_health_status_t zdj_library_rename_library( char * library_entity_id, char * name );
zdj_health_status_t zdj_library_persist( zdj_library_t * library );
zdj_health_status_t zdj_library_add_song_link( char * library_entity_id, zdj_library_song_t * song, sqlite3 * db );
zdj_health_status_t zdj_library_add_playlist_link( char * library_entity_id, zdj_library_playlist_t * playlist );
void zdj_library_deinit_library( zdj_library_t * library );

// Settings
zdj_library_setting_t * zdj_library_get_setting( char * library_entity_id, zdj_library_setting_type_t setting );
zdj_health_status_t zdj_library_set_int_setting( char * library_entity_id, zdj_library_setting_type_t setting, int val );
zdj_health_status_t zdj_library_set_bool_setting( char * library_entity_id, zdj_library_setting_type_t setting, bool val );
zdj_health_status_t zdj_library_set_float_setting( char * library_entity_id, zdj_library_setting_type_t setting, float val );
zdj_health_status_t zdj_library_set_char_setting( char * library_entity_id, zdj_library_setting_type_t setting, char * val );
void zdj_library_deinit_setting( zdj_library_setting_t * setting );


// Library Data Model API
zdj_health_status_t zdj_library_store_links_table( char * links_table_name, sqlite3 * db );

// Song Graph
zdj_library_song_t * zdj_library_create_file_import_song_graph( char * filepath, sqlite3 * db );
zdj_library_song_t * zdj_library_fetch_file_import_song_graph( char * song_entity_id, sqlite3 * db );
zdj_library_song_t * zdj_library_fetch_rb_xml_import_song_graph( zdj_library_song_t * song, sqlite3 * db );
zdj_library_song_t * zdj_library_fetch_menu_song_graph( zdj_library_song_t * song, sqlite3 * db );
zdj_library_song_t * zdj_library_fetch_playback_song_graph( zdj_library_song_t * song, sqlite3 * db );
zdj_library_song_t * zdj_library_fetch_edit_song_graph( char * song_entity_id, sqlite3 * db );
zdj_library_song_t * zdj_library_fetch_migration_song_graph( char * song_entity_id, sqlite3 * db );
zdj_health_status_t zdj_library_free_song_graph( zdj_library_song_t * song );
zdj_health_status_t zdj_library_store_song_graph( zdj_library_song_t * song, sqlite3 * db );

// Song
zdj_library_song_t * zdj_library_create_song_dto( void );
zdj_health_status_t zdj_library_free_song_dto( zdj_library_song_t * song );
zdj_health_status_t zdj_library_store_song( zdj_library_song_t * song, sqlite3 * db );

zdj_library_song_t * zdj_library_fetch_song_dto_for_entity_id( char * entity_id, sqlite3 * db );
zdj_health_status_t zdj_library_fetch_song_entity_ids( char ** arr, int count, sqlite3 * db );
int zdj_library_count_songs_in_library( char * library_entity_id, sqlite3 * db );
int zdj_library_count_songs_in_db( sqlite3 * db );

zdj_library_catalog_t * zdj_library_create_catalog_dto( void );
zdj_library_catalog_t * zdj_library_fetch_current_catalog_dto_for_song( zdj_library_song_t * song, sqlite3 * db );
zdj_health_status_t zdj_library_free_catalog_dto( zdj_library_catalog_t * catalog );
zdj_health_status_t zdj_library_store_catalog( zdj_library_catalog_t * catalog, sqlite3 * db );

zdj_library_performance_t * zdj_library_create_performance_dto( void );
zdj_library_performance_t * zdj_library_fetch_current_performance_dto_for_song( zdj_library_song_t * song, sqlite3 * db );
zdj_health_status_t zdj_library_free_performance_dto( zdj_library_performance_t * performance );
zdj_health_status_t zdj_library_store_performance( zdj_library_performance_t * performance, sqlite3 * db );

zdj_library_curation_t * zdj_library_create_curation_dto( void );
zdj_library_curation_t * zdj_library_fetch_current_curation_dto_for_song( zdj_library_song_t * song, sqlite3 * db );
zdj_health_status_t zdj_library_free_curation_dto( zdj_library_curation_t * curation );
zdj_health_status_t zdj_library_store_curation( zdj_library_curation_t * curation, sqlite3 * db );

zdj_library_audio_t * zdj_library_create_audio_dto( void );
zdj_library_audio_t * zdj_library_fetch_current_audio_dto_for_song( zdj_library_song_t * song, sqlite3 * db );
zdj_health_status_t zdj_library_free_audio_dto( zdj_library_audio_t * audio );
zdj_health_status_t zdj_library_store_audio( zdj_library_audio_t * audio, sqlite3 * db );
zdj_error_type_t zdj_library_audio_parse_encoding_info( zdj_library_audio_t * audio );
bool zdj_library_audio_is_raw_pcm( zdj_library_audio_t * audio );
char * zdj_library_audio_file_crc( char * path );

// Data Source
void zdj_library_create_default_data_sources( void );
zdj_library_data_source_t * zdj_library_fetch_data_source_for_entity_id( char * entity_id, sqlite3 * db );
void zdj_library_store_data_source( zdj_library_data_source_t * data_source, sqlite3 * db );

// Curation
int zdj_library_count_playlists( char * library_entity_id );



// Query
int zdj_library_query_count_all_artists( 
	char * library_entity_id, 
	sqlite3 * db 
);
zdj_error_type_t zdj_library_query_all_artists( 
	char * library_entity_id, 
	char ** artists, 
	int count,
	sqlite3 * db 
);

int zdj_library_query_count_songs_by_artist( 
	char * library_entity_id, 
	char * artist,
	sqlite3 * db 
);
zdj_error_type_t zdj_library_query_songs_by_artist( 
	char * library_entity_id, 
	char * artist,
	zdj_library_song_t ** songs, 
	int count, 
	sqlite3 * db 
);

int zdj_library_count_songs_in_bpm_range( 
	char * library_entity_id, 
	zdj_library_bpm_range_t range, 
	sqlite3 * db 
);
zdj_error_type_t zdj_library_query_songs_in_bpm_range( 
	char * library_entity_id, 
	zdj_library_bpm_range_t range,
	zdj_library_song_t ** songs, 
	int count, 
	sqlite3 * db 
);

int zdj_library_query_count_all_genres( 
	char * library_entity_id, 
	sqlite3 * db 
);
zdj_error_type_t zdj_library_query_all_genres( 
	char * library_entity_id, 
	char ** genres, 
	int count,
	sqlite3 * db 
);
int zdj_library_query_count_songs_in_genre( 
	char * library_entity_id, 
	char * genre,
	sqlite3 * db 
);
zdj_error_type_t zdj_library_query_songs_in_genre( 
	char * library_entity_id, 
	char * genre, 
	zdj_library_song_t ** songs, 
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
	zdj_library_playlist_t ** playlists, 
	sqlite3 * db 
);
int zdj_library_query_count_songs_in_playlist( 
	char * library_entity_id, 
	zdj_library_playlist_t * playlist,
	sqlite3 * db 
);
zdj_error_type_t zdj_library_query_songs_in_playlist( 
	char * library_entity_id, 
	zdj_library_playlist_t * playlist,
	zdj_library_song_t ** songs, 
	int count, 
	sqlite3 * db 
);

#endif