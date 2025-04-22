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

#include <zerodj/health/zdj_health_type.h>

#define ZDJ_LIBRARY_DB_PATH "/etc/zero_data/zero.db"
#define ZDJ_LIBRARY_IMPORT_DB_PATH "/etc/zero_data/zero_import.db"

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

typedef enum {
	ZDJ_LIBRARY_DATA_SOURCE_ZERO,
	ZDJ_LIBRARY_DATA_SOURCE_ID3,
	ZDJ_LIBRARY_DATA_SOURCE_RB_XML,
	ZDJ_LIBRARY_DATA_SOURCE_ENG_JSON,
	ZDJ_LIBRARY_DATA_SOURCE_ENG_DB,
	ZDJ_LIBRARY_DATA_SOURCE_SER,
	ZDJ_LIBRARY_DATA_SOURCE_MB
} zdj_library_data_source_ref_t;

typedef enum {
	ZDJ_LIBRARY_KEY_1A
} zdj_library_key_t;

struct zdj_library_t;

typedef struct {
	int entity_id;
	int entity_counter;
	int current_lib_entity_id;
} zdj_library_config_t;

typedef struct zdj_library_t {
	int entity_id;
	char * name;
	char * song_links;
	char * curation_data_links;
	char * setting_links;
} zdj_library_t;

typedef struct {
	int entity_id;
	struct zdj_library_catalog_data_t * catalog_data;
	struct zdj_library_performance_data_t * performance_data;
	struct zdj_library_curation_data_t * curation_data; // optional - only used in edit workflows
	struct zdj_library_audio_data_t * audio_data;
	struct zdj_library_analysis_state_t * analysis_state;  // optional - only used during lib import workflows
} zdj_library_song_t;

typedef struct {
	int entity_id;
	char * filepath;
	bool has_procedural_edit;
	char * procedural_edit_filepath;
} zdj_library_audio_data_t;

typedef enum {
	ZDJ_LIBRARY_AUDIO_PROCEDURAL_EDIT_OP_SPLICE
} zdj_library_audio_procedural_edit_op_t;

typedef struct {
	int entity_id;
	zdj_library_audio_procedural_edit_op_t op;
	int edit_in_sample;
	int edit_out_sample;
} zdj_library_audio_procedural_edit_step_t;

typedef struct {
	int entity_id;
	int data_source_ref;
	char * title;
	char * artist;
	char * album;
	char * label;
	int year;
} zdj_library_catalog_data_t;

typedef struct {
	int entity_id;
	int data_source_ref;
	int sample_length;
	zdj_library_key_t key;
	float bpm;
	bool has_beat_grid;
	int beat_grid_start_sample;
	struct zdj_library_cuepoint_t * cuepoints;
} zdj_library_performance_data_t;

typedef struct {
	int entity_id;
	int data_source_ref;
	struct zdj_library_playlist_t * playlists;
	struct zdj_library_tag_t * tags;
} zdj_library_curation_data_t;

typedef struct {
	int entity_id;
	int data_source_ref;
	char * name;
} zdj_library_tag_t;

typedef struct {
	int entity_id;
	int data_source_ref;
	int sample;
	bool is_loop;
} zdj_library_cuepoint_t;

typedef struct {
	int entity_id;
	int data_source_ref;
	char * title;
} zdj_library_playlist_t;

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
	int entity_id;
	zdj_library_setting_type_t type;
	int i_val;
	float f_val;
	bool b_val;
	char * c_val;
} zdj_library_setting_t;

extern sqlite3 * zdj_library_db;
extern sqlite3 * zdj_library_import_db;
extern zdj_library_config_t * library_config;

// Library Init
zdj_health_status_t zdj_library_init( void );
zdj_health_status_t zdj_library_health( void );

// Library DB API
zdj_health_status_t zdj_library_db_init( void );
zdj_health_status_t zdj_library_db_flush( void );

// Global entity counter
int zdj_library_increment_entity_count( void );

// Config
zdj_library_config_t * zdj_library_get_config( void );
int zdj_library_config_get_current_library_id( void );
zdj_health_status_t zdj_library_config_set_current_library_id( int entity_id );

// Library
int zdj_library_count_libraries( void );
void zdj_library_get_all_libraries( 
    zdj_library_config_t * config, 
    zdj_library_t ** libraries, 
    int result_limit
);
zdj_library_t * zdj_library_get_library_for_entity_id( int entity_id );
zdj_health_status_t zdj_library_new( void );
zdj_health_status_t zdj_library_duplicate( int library_entity_id );
zdj_health_status_t zdj_library_remove( int library_entity_id );
zdj_health_status_t zdj_library_rename( int library_entity_id, char * name );
void zdj_library_deinit_library( zdj_library_t * library );

// Import
zdj_library_import_type_t zdj_library_get_import_type_for_path( char * path );
zdj_health_status_t zdj_library_new_import_db( void );
zdj_health_status_t zdj_library_open_import_db( void );
zdj_health_status_t zdj_library_close_import_db( void );

// Settings
zdj_library_setting_t * zdj_library_get_setting( int library_entity_id, zdj_library_setting_type_t setting );
zdj_health_status_t zdj_library_set_int_setting( int library_entity_id, zdj_library_setting_type_t setting, int val );
zdj_health_status_t zdj_library_set_bool_setting( int library_entity_id, zdj_library_setting_type_t setting, bool val );
zdj_health_status_t zdj_library_set_float_setting( int library_entity_id, zdj_library_setting_type_t setting, float val );
zdj_health_status_t zdj_library_set_char_setting( int library_entity_id, zdj_library_setting_type_t setting, char * val );
void zdj_library_deinit_setting( zdj_library_setting_t * setting );


// Library Data Model API
// Song
int zdj_library_count_songs( int library_entity_id );
zdj_library_song_t * zdj_library_import_song_for_filepath( char * path, sqlite3 * db );
// zdj_library_song_t * zdj_library_import_song_for_rekordbox_node( xmlNode * node, sqlite3 * db );

int zdj_library_count_playlists( int library_entity_id );


#endif