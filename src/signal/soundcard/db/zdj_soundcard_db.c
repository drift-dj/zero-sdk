#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sqlite3.h>

#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/db/zdj_soundcard_dto.h>
#include <zerodj/system/sql/zdj_sql.h>

static void _zdj_soundcard_drop_tables( sqlite3 * db );
static void _zdj_soundcard_create_linkage_table( sqlite3 * db );
static void _zdj_soundcard_create_dsp_table( sqlite3 * db );

void zdj_drop_soundcard( void ) {
    // If soundcard is running, shut down soundcard
    if( zdj_soundcard ) { zdj_soundcard_deinit( zdj_soundcard ); }

    // printf( "zdj_drop_soundcard 0\n" );
    // // Delete db
    // int res = remove( ZDJ_SOUNDCARD_DB_PATH );
    // if( res ) { printf( "remove failed: %d\n", res ); }

    // printf( "zdj_drop_soundcard 0\n" );
    // Create new db
    sqlite3 * db = zdj_sql_open( ZDJ_SOUNDCARD_DB_PATH );
    if( !db ) { 
        printf( "failed to open soundcard db\n" );
        return; 
    }

    // printf( "zdj_drop_soundcard 1\n" );
    _zdj_soundcard_drop_tables( db );

    // printf( "zdj_drop_soundcard 2\n" );
    // Create tables in new db
    _zdj_soundcard_create_linkage_table( db );
    _zdj_soundcard_create_dsp_table( db );

    // printf( "zdj_drop_soundcard 3\n" );
    zdj_sql_db_flush( db );
    zdj_sql_close( db );

    // printf( "zdj_drop_soundcard 4\n" );
    // Add defaults to new db
    zdj_soundcard_reset_db_defaults( );

    // printf( "zdj_drop_soundcard 5\n" );
}

void zdj_soundcard_reset_db_defaults( void ) {
    // Make DJ default
    zdj_soundcard_dto_t * dto = zdj_soundcard_create_dto( );
    strcpy( dto->name, "Default" );

    // Add default linkages

    // Analog input outs
    dto->ana_in_0_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE;
    dto->ana_in_0_stereo = true;
    dto->ana_in_1_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE;
    dto->ana_in_1_stereo = true;
    dto->ana_in_2_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE;
    dto->ana_in_2_stereo = true;
    dto->ana_in_3_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE;
    dto->ana_in_3_stereo = true;

    dto->ana_out_0_stereo = true;
    dto->ana_out_0_sig = ZDJ_SOUNDCARD_SIGNAL_PRO_PLUS_4_DBU;
    dto->ana_out_1_stereo = true;
    dto->ana_out_1_sig = ZDJ_SOUNDCARD_SIGNAL_PRO_PLUS_4_DBU;
    dto->ana_out_2_stereo = true;
    dto->ana_out_2_sig = ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_LOW;
    dto->ana_out_3_stereo = true;
    dto->ana_out_3_sig = ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_LOW;

    // Admin bus outs
    dto->annot_bus_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->annot_bus_stereo = false;
    dto->cue_bus_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0;
    dto->cue_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1;
    dto->cue_bus_stereo = true;
    dto->main_bus_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS;
    dto->main_bus_stereo = true;
    dto->record_bus_stereo = true;
    
    // Deck 1
    dto->deck_1_input_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE;
    dto->deck_1_edge_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE;
    dto->deck_1_prefade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_POSTFADE;
    dto->deck_1_prefade_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_CUE;
    dto->deck_1_cue_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->deck_1_postfade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_XFADE_A;
    dto->deck_1_prefade_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ;
    dto->deck_1_prefade_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_EQ_3_4P;
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_0 = 1.0; // Lo
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_1 = 1.0; // Mid
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_2 = 1.0; // Hi
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_3 = 1.0; // Xover Lo
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_4 = 1.0; // Xover Hi

    // Deck 2
    dto->deck_2_input_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE;
    dto->deck_2_edge_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE;
    dto->deck_2_prefade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_POSTFADE;
    dto->deck_2_prefade_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_CUE;
    dto->deck_2_cue_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->deck_2_postfade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_XFADE_B;
    dto->deck_2_prefade_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ;
    dto->deck_2_prefade_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_EQ_3_4P;
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_0 = 1.0; // Lo
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_1 = 1.0; // Mid
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_2 = 1.0; // Hi
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_3 = 1.0; // Xover Lo
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_4 = 1.0; // Xover Hi

    // Ext Deck
    dto->deck_ext_input_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE;
    dto->deck_ext_edge_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE;
    dto->deck_ext_prefade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_POSTFADE;
    dto->deck_ext_prefade_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_CUE;
    dto->deck_ext_cue_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->deck_ext_postfade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS;
    dto->deck_ext_prefade_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ;
    dto->deck_ext_prefade_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_EQ_3_4P;
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_0 = 1.0; // Lo
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_1 = 1.0; // Mid
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_2 = 1.0; // Hi
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_3 = 1.0; // Xover Lo
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_4 = 1.0; // Xover Hi

    // Main Clock
    dto->clock_0_sig = ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_4; // Clock output 4 PPQN
    dto->clock_0_source = ZDJ_SOUNDCARD_CLOCK_DIRECTION_OUTPUT; // Clock source Output
    dto->clock_0_sync = ZDJ_SOUNDCARD_CLOCK_SYNC_NORMAL;
    dto->clock_0_val = 120.0; // Clock BPM setting

    // Crossfader
    dto->xfade_a_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS;
    dto->xfade_a_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_XFADE;
    dto->xfade_a_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_XFADE_CURVE;
    dto->xfade_a_dsp.stages[ 0 ].knob_0 = 0.8; // Crossfader curve
    dto->xfade_b_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS;
    dto->xfade_b_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_XFADE;
    dto->xfade_b_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_XFADE_CURVE;
    dto->xfade_b_dsp.stages[ 0 ].knob_0 = 0.8; // Crossfader curve

    zdj_soundcard_store_dto( dto );

    // Make __temp__ from DJ default
    strcpy( dto->entity_id, "__temp__" );
    strcpy( dto->name, "Current" );
    zdj_soundcard_store_dto( dto );
}

static void _zdj_soundcard_drop_tables( sqlite3 * db ) {
    char sql[ 1024 ];
    strcpy( sql, "DROP TABLE IF EXISTS Linkage" );
    zdj_sql_exec( (char*)&sql, db );
    strcpy( sql, "DROP TABLE IF EXISTS DSP" );
    zdj_sql_exec( (char*)&sql, db );
}

static void _zdj_soundcard_create_linkage_table( sqlite3 * db ) {
    // Create tables
    char sql[ 8192 ];
    // Audio
    strcpy( sql, "CREATE TABLE 'Linkage' ( 'entity_id' TEXT NOT NULL UNIQUE, 'name' TEXT NOT NULL, 'ana_out_0_sig' INT NOT NULL DEFAULT 0, 'ana_out_0_stereo' INT NOT NULL DEFAULT 1, 'ana_out_1_sig' INT NOT NULL DEFAULT 0, 'ana_out_1_stereo' INT NOT NULL DEFAULT 1, 'ana_out_2_sig' INT NOT NULL DEFAULT 0, 'ana_out_2_stereo' INT NOT NULL DEFAULT 1, 'ana_out_3_sig' INT NOT NULL DEFAULT 0, 'ana_out_3_stereo' INT NOT NULL DEFAULT 1, 'ana_in_0_link_map' INT NOT NULL DEFAULT 0, 'ana_in_0_dsp_eid' TEXT NOT NULL, 'ana_in_0_sig' INT NOT NULL DEFAULT 0, 'ana_in_0_stereo' INT NOT NULL DEFAULT 1, 'ana_in_0_mute' INT NOT NULL DEFAULT 0, 'ana_in_1_link_map' INT NOT NULL DEFAULT 0, 'ana_in_1_dsp_eid' TEXT NOT NULL, 'ana_in_1_sig' INT NOT NULL DEFAULT 0, 'ana_in_1_stereo' INT NOT NULL DEFAULT 1, 'ana_in_1_mute' INT NOT NULL DEFAULT 0, 'ana_in_2_link_map' INT NOT NULL DEFAULT 0, 'ana_in_2_dsp_eid' TEXT NOT NULL, 'ana_in_2_sig' INT NOT NULL DEFAULT 0, 'ana_in_2_stereo' INT NOT NULL DEFAULT 1, 'ana_in_2_mute' INT NOT NULL DEFAULT 0, 'ana_in_3_link_map' INT NOT NULL DEFAULT 0, 'ana_in_3_dsp_eid' TEXT NOT NULL, 'ana_in_3_sig' INT NOT NULL DEFAULT 0, 'ana_in_3_stereo' INT NOT NULL DEFAULT 1, 'ana_in_3_mute' INT NOT NULL DEFAULT 0, 'main_bus_link_map' INT NOT NULL DEFAULT 0, 'main_bus_dsp_eid' TEXT NOT NULL, 'main_bus_stereo' INT NOT NULL DEFAULT 1, 'main_bus_mute' INT NOT NULL DEFAULT 0, 'cue_bus_link_map' INT NOT NULL DEFAULT 0, 'cue_bus_dsp_eid' TEXT NOT NULL, 'cue_bus_stereo' INT NOT NULL DEFAULT 1, 'cue_bus_mute' INT NOT NULL DEFAULT 0, 'annot_bus_link_map' INT NOT NULL DEFAULT 0, 'annot_bus_dsp_eid' TEXT NOT NULL, 'annot_bus_stereo' INT NOT NULL DEFAULT 1, 'annot_bus_mute' INT NOT NULL DEFAULT 0, 'record_bus_dsp_eid' TEXT NOT NULL, 'record_bus_stereo' INT NOT NULL DEFAULT 1, 'deck_1_input_link_map' INT NOT NULL DEFAULT 0, 'deck_1_input_dsp_eid' TEXT NOT NULL, 'deck_1_edge_link_map' INT NOT NULL DEFAULT 0, 'deck_1_prefade_link_map' INT NOT NULL DEFAULT 0, 'deck_1_prefade_dsp_eid' TEXT NOT NULL, 'deck_1_postfade_link_map' INT NOT NULL DEFAULT 0, 'deck_1_postfade_dsp_eid' TEXT NOT NULL, 'deck_1_cue_link_map' INT NOT NULL DEFAULT 0, 'deck_1_cue_dsp_eid' TEXT NOT NULL, 'deck_2_input_link_map' INT NOT NULL DEFAULT 0, 'deck_2_input_dsp_eid' TEXT NOT NULL, 'deck_2_edge_link_map' INT NOT NULL DEFAULT 0, 'deck_2_prefade_link_map' INT NOT NULL DEFAULT 0, 'deck_2_prefade_dsp_eid' TEXT NOT NULL, 'deck_2_postfade_link_map' INT NOT NULL DEFAULT 0, 'deck_2_postfade_dsp_eid' TEXT NOT NULL, 'deck_2_cue_link_map' INT NOT NULL DEFAULT 0, 'deck_2_cue_dsp_eid' TEXT NOT NULL, 'deck_ext_input_link_map' INT NOT NULL DEFAULT 0, 'deck_ext_input_dsp_eid' TEXT NOT NULL, 'deck_ext_edge_link_map' INT NOT NULL DEFAULT 0, 'deck_ext_prefade_link_map' INT NOT NULL DEFAULT 0, 'deck_ext_prefade_dsp_eid' TEXT NOT NULL, 'deck_ext_postfade_link_map' INT NOT NULL DEFAULT 0, 'deck_ext_postfade_dsp_eid' TEXT NOT NULL, 'deck_ext_cue_link_map' INT NOT NULL DEFAULT 0, 'deck_ext_cue_dsp_eid' TEXT NOT NULL, 'aux_bus_0_link_map' INT NOT NULL DEFAULT 0, 'aux_bus_0_dsp_eid' TEXT NOT NULL, 'aux_bus_0_stereo' INT NOT NULL DEFAULT 1, 'aux_bus_0_mute' INT NOT NULL DEFAULT 0, 'aux_bus_1_link_map' INT NOT NULL DEFAULT 0, 'aux_bus_1_dsp_eid' TEXT NOT NULL, 'aux_bus_1_stereo' INT NOT NULL DEFAULT 1, 'aux_bus_1_mute' INT NOT NULL DEFAULT 0, 'aux_bus_2_link_map' INT NOT NULL DEFAULT 0, 'aux_bus_2_dsp_eid' TEXT NOT NULL, 'aux_bus_2_stereo' INT NOT NULL DEFAULT 1, 'aux_bus_2_mute' INT NOT NULL DEFAULT 0, 'aux_bus_3_link_map' INT NOT NULL DEFAULT 0, 'aux_bus_3_dsp_eid' TEXT NOT NULL, 'aux_bus_3_stereo' INT NOT NULL DEFAULT 1, 'aux_bus_3_mute' INT NOT NULL DEFAULT 0, 'clock_0_sig' INT NOT NULL DEFAULT 0, 'clock_0_link_map' INT NOT NULL DEFAULT 0, 'clock_0_source' INT NOT NULL DEFAULT 0, 'clock_0_val' REAL NOT NULL DEFAULT 0, 'clock_0_sync' INT NOT NULL DEFAULT 0, 'clock_1_sig' INT NOT NULL DEFAULT 0, 'clock_1_link_map' INT NOT NULL DEFAULT 0, 'clock_1_source' INT NOT NULL DEFAULT 0, 'clock_1_val' REAL NOT NULL DEFAULT 0, 'clock_1_sync' INT NOT NULL DEFAULT 0, 'clock_2_sig' INT NOT NULL DEFAULT 0, 'clock_2_link_map' INT NOT NULL DEFAULT 0, 'clock_2_source' INT NOT NULL DEFAULT 0, 'clock_2_val' REAL NOT NULL DEFAULT 0, 'clock_2_sync' INT NOT NULL DEFAULT 0, 'clock_3_sig' INT NOT NULL DEFAULT 0, 'clock_3_link_map' INT NOT NULL DEFAULT 0, 'clock_3_source' INT NOT NULL DEFAULT 0, 'clock_3_val' REAL NOT NULL DEFAULT 0, 'clock_3_sync' INT NOT NULL DEFAULT 0, 'cv_0_sig' INT NOT NULL DEFAULT 0, 'cv_0_link_map' INT NOT NULL DEFAULT 0, 'cv_0_dsp_eid' TEXT NOT NULL, 'cv_0_source' INT NOT NULL DEFAULT 0, 'cv_0_mute' INT NOT NULL DEFAULT 0, 'cv_1_sig' INT NOT NULL DEFAULT 0, 'cv_1_link_map' INT NOT NULL DEFAULT 0, 'cv_1_dsp_eid' TEXT NOT NULL, 'cv_1_source' INT NOT NULL DEFAULT 0, 'cv_1_mute' INT NOT NULL DEFAULT 0, 'cv_2_sig' INT NOT NULL DEFAULT 0, 'cv_2_link_map' INT NOT NULL DEFAULT 0, 'cv_2_dsp_eid' TEXT NOT NULL, 'cv_2_source' INT NOT NULL DEFAULT 0, 'cv_2_mute' INT NOT NULL DEFAULT 0, 'cv_3_sig' INT NOT NULL DEFAULT 0, 'cv_3_link_map' INT NOT NULL DEFAULT 0, 'cv_3_dsp_eid' TEXT NOT NULL, 'cv_3_source' INT NOT NULL DEFAULT 0, 'cv_3_mute' INT NOT NULL DEFAULT 0, 'xfade_a_link_map' INT NOT NULL DEFAULT 0, 'xfade_a_dsp_eid' TEXT NOT NULL, 'xfade_b_link_map' INT NOT NULL DEFAULT 0, 'xfade_b_dsp_eid' TEXT NOT NULL, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, db );
}


static void _zdj_soundcard_create_dsp_table( sqlite3 * db ) {
    // Create tables
    char sql[ 8192 ];
    // Audio
    strcpy( sql, "CREATE TABLE 'DSP' ( 'entity_id' TEXT NOT NULL UNIQUE, 'gain' REAL DEFAULT 1.0, 'pan' REAL DEFAULT 0.0, 'stg_0_type' INT DEFAULT 0, 'stg_0_id' INT DEFAULT 0, 'stg_0_knob_0' REAL DEFAULT 0.0, 'stg_0_knob_1' REAL DEFAULT 0.0, 'stg_0_knob_2' REAL DEFAULT 0.0, 'stg_0_knob_3' REAL DEFAULT 0.0, 'stg_0_knob_4' REAL DEFAULT 0.0, 'stg_0_knob_5' REAL DEFAULT 0.0, 'stg_0_knob_6' REAL DEFAULT 0.0, 'stg_0_knob_7' REAL DEFAULT 0.0, 'stg_1_type' INT DEFAULT 0, 'stg_1_id' INT DEFAULT 0, 'stg_1_knob_0' REAL DEFAULT 0.0, 'stg_1_knob_1' REAL DEFAULT 0.0, 'stg_1_knob_2' REAL DEFAULT 0.0, 'stg_1_knob_3' REAL DEFAULT 0.0, 'stg_1_knob_4' REAL DEFAULT 0.0, 'stg_1_knob_5' REAL DEFAULT 0.0, 'stg_1_knob_6' REAL DEFAULT 0.0, 'stg_1_knob_7' REAL DEFAULT 0.0, 'stg_2_type' INT DEFAULT 0, 'stg_2_id' INT DEFAULT 0, 'stg_2_knob_0' REAL DEFAULT 0.0, 'stg_2_knob_1' REAL DEFAULT 0.0, 'stg_2_knob_2' REAL DEFAULT 0.0, 'stg_2_knob_3' REAL DEFAULT 0.0, 'stg_2_knob_4' REAL DEFAULT 0.0, 'stg_2_knob_5' REAL DEFAULT 0.0, 'stg_2_knob_6' REAL DEFAULT 0.0, 'stg_2_knob_7' REAL DEFAULT 0.0, 'stg_3_type' INT DEFAULT 0, 'stg_3_id' INT DEFAULT 0, 'stg_3_knob_0' REAL DEFAULT 0.0, 'stg_3_knob_1' REAL DEFAULT 0.0, 'stg_3_knob_2' REAL DEFAULT 0.0, 'stg_3_knob_3' REAL DEFAULT 0.0, 'stg_3_knob_4' REAL DEFAULT 0.0, 'stg_3_knob_5' REAL DEFAULT 0.0, 'stg_3_knob_6' REAL DEFAULT 0.0, 'stg_3_knob_7' REAL DEFAULT 0.0, 'stg_4_type' INT DEFAULT 0, 'stg_4_id' INT DEFAULT 0, 'stg_4_knob_0' REAL DEFAULT 0.0, 'stg_4_knob_1' REAL DEFAULT 0.0, 'stg_4_knob_2' REAL DEFAULT 0.0, 'stg_4_knob_3' REAL DEFAULT 0.0, 'stg_4_knob_4' REAL DEFAULT 0.0, 'stg_4_knob_5' REAL DEFAULT 0.0, 'stg_4_knob_6' REAL DEFAULT 0.0, 'stg_4_knob_7' REAL DEFAULT 0.0, 'stg_5_type' INT DEFAULT 0, 'stg_5_id' INT DEFAULT 0, 'stg_5_knob_0' REAL DEFAULT 0.0, 'stg_5_knob_1' REAL DEFAULT 0.0, 'stg_5_knob_2' REAL DEFAULT 0.0, 'stg_5_knob_3' REAL DEFAULT 0.0, 'stg_5_knob_4' REAL DEFAULT 0.0, 'stg_5_knob_5' REAL DEFAULT 0.0, 'stg_5_knob_6' REAL DEFAULT 0.0, 'stg_5_knob_7' REAL DEFAULT 0.0, 'stg_6_type' INT DEFAULT 0, 'stg_6_id' INT DEFAULT 0, 'stg_6_knob_0' REAL DEFAULT 0.0, 'stg_6_knob_1' REAL DEFAULT 0.0, 'stg_6_knob_2' REAL DEFAULT 0.0, 'stg_6_knob_3' REAL DEFAULT 0.0, 'stg_6_knob_4' REAL DEFAULT 0.0, 'stg_6_knob_5' REAL DEFAULT 0.0, 'stg_6_knob_6' REAL DEFAULT 0.0, 'stg_6_knob_7' REAL DEFAULT 0.0, 'stg_7_type' INT DEFAULT 0, 'stg_7_id' INT DEFAULT 0, 'stg_7_knob_0' REAL DEFAULT 0.0, 'stg_7_knob_1' REAL DEFAULT 0.0, 'stg_7_knob_2' REAL DEFAULT 0.0, 'stg_7_knob_3' REAL DEFAULT 0.0, 'stg_7_knob_4' REAL DEFAULT 0.0, 'stg_7_knob_5' REAL DEFAULT 0.0, 'stg_7_knob_6' REAL DEFAULT 0.0, 'stg_7_knob_7' REAL DEFAULT 0.0, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, db );
}

zdj_error_type_t zdj_soundcard_fetch_dto( char * entity_id, zdj_soundcard_dto_t * dto ) {
    // printf( "zdj_soundcard_fetch_dto: %p %s\n", entity_id, entity_id );
    if( !entity_id ) { return ZDJ_ERROR_OKAY; }
    
    sqlite3 * db = zdj_sql_open( ZDJ_SOUNDCARD_DB_PATH );

    if( !db ) { 
        printf( "failed to open zero db\n" );
        return ZDJ_ERROR_LIBRARY_DB_ERROR; 
    }

    // Grab all the values from db
    int sql_res;
    char _sql[ 1024 ];
    sprintf( _sql, "select * from %s where entity_id=\'%s\'", ZDJ_SOUNDCARD_LINKAGE_TABLE, entity_id );
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( _sql, db );
    if( stmt ) {
        while ( ( sql_res = sqlite3_step( stmt ) ) == SQLITE_ROW ) {
            strcpy( dto->entity_id, (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_ENTITY_ID ) );
            char * str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_NAME );
            if( str ) { strcpy( dto->name, str ); }

            dto->ana_out_0_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_0_SIG );
            dto->ana_out_0_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_0_STEREO );
            dto->ana_out_1_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_1_SIG );
            dto->ana_out_1_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_1_STEREO );
            dto->ana_out_2_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_2_SIG );
            dto->ana_out_2_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_2_STEREO );
            dto->ana_out_3_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_3_SIG );
            dto->ana_out_3_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_3_STEREO );
            dto->ana_in_0_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_0_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_0_DSP );
            if( str ) { strcpy( dto->ana_in_0_dsp_eid, str ); }
            dto->ana_in_0_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_0_SIG );
            dto->ana_in_0_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_0_STEREO );
            dto->ana_in_0_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_0_MUTE );
            dto->ana_in_1_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_1_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_1_DSP );
            if( str ) { strcpy( dto->ana_in_1_dsp_eid, str ); }
            dto->ana_in_1_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_1_SIG );
            dto->ana_in_1_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_1_STEREO );
            dto->ana_in_1_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_1_MUTE );
            dto->ana_in_2_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_2_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_2_DSP );
            if( str ) { strcpy( dto->ana_in_2_dsp_eid, str ); }
            dto->ana_in_2_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_2_SIG );
            dto->ana_in_2_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_2_STEREO );
            dto->ana_in_2_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_2_MUTE );
            dto->ana_in_3_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_3_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_3_DSP );
            if( str ) { strcpy( dto->ana_in_3_dsp_eid, str ); }
            dto->ana_in_3_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_3_SIG );
            dto->ana_in_3_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_3_STEREO );
            dto->ana_in_3_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_3_MUTE );
            dto->main_bus_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_MAIN_BUS_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_MAIN_BUS_DSP );
            if( str ) { strcpy( dto->main_bus_dsp_eid, str ); }
            dto->main_bus_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_MAIN_BUS_STEREO );
            dto->main_bus_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_MAIN_BUS_MUTE );
            dto->cue_bus_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CUE_BUS_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_CUE_BUS_DSP );
            if( str ) { strcpy( dto->cue_bus_dsp_eid, str ); }
            dto->cue_bus_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CUE_BUS_STEREO );
            dto->cue_bus_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CUE_BUS_MUTE );
            dto->annot_bus_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_ANNOT_BUS_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_ANNOT_BUS_DSP );
            if( str ) { strcpy( dto->annot_bus_dsp_eid, str ); }
            dto->annot_bus_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANNOT_BUS_STEREO );
            dto->annot_bus_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANNOT_BUS_MUTE );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_RECORD_BUS_DSP );
            if( str ) { strcpy( dto->record_bus_dsp_eid, str ); }
            dto->record_bus_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_RECORD_BUS_STEREO );

            dto->deck_1_input_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_1_INPUT_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_DECK_1_INPUT_DSP );
            if( str ) { strcpy( dto->deck_1_input_dsp_eid, str ); }
            dto->deck_1_edge_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_1_EDGE_LINK );
            dto->deck_1_prefade_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_1_PREFADE_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_DECK_1_PREFADE_DSP );
            if( str ) { strcpy( dto->deck_1_prefade_dsp_eid, str ); }
            dto->deck_1_postfade_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_1_POSTFADE_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_DECK_1_POSTFADE_DSP );
            if( str ) { strcpy( dto->deck_1_postfade_dsp_eid, str ); }
            dto->deck_1_cue_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_1_CUE_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_DECK_1_CUE_DSP );
            if( str ) { strcpy( dto->deck_1_cue_dsp_eid, str ); }

            dto->deck_2_input_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_2_INPUT_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_DECK_2_INPUT_DSP );
            if( str ) { strcpy( dto->deck_2_input_dsp_eid, str ); }
            dto->deck_2_edge_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_2_EDGE_LINK );
            dto->deck_2_prefade_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_2_PREFADE_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_DECK_2_PREFADE_DSP );
            if( str ) { strcpy( dto->deck_2_prefade_dsp_eid, str ); }
            dto->deck_2_postfade_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_2_POSTFADE_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_DECK_2_POSTFADE_DSP );
            if( str ) { strcpy( dto->deck_2_postfade_dsp_eid, str ); }
            dto->deck_2_cue_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_2_CUE_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_DECK_2_CUE_DSP );
            if( str ) { strcpy( dto->deck_2_cue_dsp_eid, str ); }

            dto->deck_ext_input_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_EXT_INPUT_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_DECK_EXT_CUE_DSP );
            if( str ) { strcpy( dto->deck_ext_cue_dsp_eid, str ); }
            dto->deck_ext_edge_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_EXT_EDGE_LINK );
            dto->deck_ext_prefade_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_EXT_PREFADE_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_DECK_EXT_PREFADE_DSP );
            if( str ) { strcpy( dto->deck_ext_prefade_dsp_eid, str ); }
            dto->deck_ext_postfade_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_EXT_POSTFADE_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_DECK_EXT_POSTFADE_DSP );
            if( str ) { strcpy( dto->deck_ext_postfade_dsp_eid, str ); }
            dto->deck_ext_cue_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_EXT_CUE_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_DECK_EXT_CUE_DSP );
            if( str ) { strcpy( dto->deck_ext_cue_dsp_eid, str ); }

            dto->aux_bus_0_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_0_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_0_DSP );
            if( str ) { strcpy( dto->aux_bus_0_dsp_eid, str ); }
            dto->aux_bus_0_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_0_STEREO );
            dto->aux_bus_0_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_0_MUTE );
            dto->aux_bus_1_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_1_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_1_DSP );
            if( str ) { strcpy( dto->aux_bus_1_dsp_eid, str ); }
            dto->aux_bus_1_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_1_STEREO );
            dto->aux_bus_1_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_1_MUTE );
            dto->aux_bus_2_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_2_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_2_DSP );
            if( str ) { strcpy( dto->aux_bus_2_dsp_eid, str ); }
            dto->aux_bus_2_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_2_STEREO );
            dto->aux_bus_2_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_2_MUTE );
            dto->aux_bus_3_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_3_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_3_DSP );
            if( str ) { strcpy( dto->aux_bus_3_dsp_eid, str ); }
            dto->aux_bus_3_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_3_STEREO );
            dto->aux_bus_3_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_3_MUTE );

            dto->clock_0_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_0_SIG );
            dto->clock_0_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_0_LINK );
            dto->clock_0_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_0_DIRECTION );
            dto->clock_0_val = sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_0_VAL );
            dto->clock_0_sync = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_0_SYNC );
            dto->clock_1_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_1_SIG );
            dto->clock_1_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_1_LINK );
            dto->clock_1_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_1_DIRECTION );
            dto->clock_1_val = sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_1_VAL );
            dto->clock_1_sync = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_1_SYNC );
            dto->clock_2_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_2_SIG );
            dto->clock_2_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_2_LINK );
            dto->clock_2_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_2_DIRECTION );
            dto->clock_2_val = sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_2_VAL );
            dto->clock_2_sync = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_2_SYNC );
            dto->clock_3_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_3_SIG );
            dto->clock_3_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_3_LINK );
            dto->clock_3_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_3_DIRECTION );
            dto->clock_3_val = sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_3_VAL );
            dto->clock_3_sync = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_3_SYNC );

            dto->cv_0_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_0_SIG );
            dto->cv_0_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CV_0_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_CV_0_DSP );
            if( str ) { strcpy( dto->cv_0_dsp_eid, str ); }
            dto->cv_0_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_0_DIRECTION );
            dto->cv_0_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_0_MUTE );
            dto->cv_1_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_1_SIG );
            dto->cv_1_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CV_1_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_CV_1_DSP );
            if( str ) { strcpy( dto->cv_1_dsp_eid, str ); }
            dto->cv_1_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_1_DIRECTION );
            dto->cv_1_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_1_MUTE );
            dto->cv_2_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_2_SIG );
            dto->cv_2_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CV_2_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_CV_2_DSP );
            if( str ) { strcpy( dto->cv_2_dsp_eid, str ); }
            dto->cv_2_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_2_DIRECTION );
            dto->cv_2_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_2_MUTE );
            dto->cv_3_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_3_SIG );
            dto->cv_3_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CV_3_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_CV_3_DSP );
            if( str ) { strcpy( dto->cv_3_dsp_eid, str ); }
            dto->cv_3_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_3_DIRECTION );
            dto->cv_2_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_3_MUTE );

            dto->xfade_a_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_XFADE_A_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_XFADE_A_DSP );
            if( str ) { strcpy( dto->xfade_a_dsp_eid, str ); }
            dto->xfade_b_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_XFADE_B_LINK );
            str = (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_XFADE_B_DSP );
            if( str ) { strcpy( dto->xfade_b_dsp_eid, str ); }

            // printf( "dto->ana_in_0_link_map:%lu\ndto->ana_in_1_link_map:%lu\ndto->ana_in_2_link_map:%lu\ndto->ana_in_3_link_map:%lu\ndto->main_bus_link_map:%lu\ndto->cue_bus_link_map:%lu\ndto->annot_bus_link_map:%lu\ndto->deck_1_input_link_map:%lu\ndto->deck_1_edge_link_map:%lu\ndto->deck_1_prefade_link_map:%lu\ndto->deck_1_postfade_link_map:%lu\ndto->deck_1_cue_link_map:%lu\ndto->deck_2_input_link_map:%lu\ndto->deck_2_edge_link_map:%lu\ndto->deck_2_prefade_link_map:%lu\ndto->deck_2_postfade_link_map:%lu\ndto->deck_2_cue_link_map:%lu\ndto->deck_ext_input_link_map:%lu\ndto->deck_ext_edge_link_map:%lu\ndto->deck_ext_prefade_link_map:%lu\ndto->deck_ext_postfade_link_map:%lu\ndto->deck_ext_cue_link_map:%lu\ndto->aux_bus_0_link_map:%lu\ndto->aux_bus_1_link_map:%lu\ndto->aux_bus_2_link_map:%lu\ndto->aux_bus_3_link_map:%lu\ndto->clock_0_link_map:%lu\ndto->clock_1_link_map:%lu\ndto->clock_2_link_map:%lu\ndto->clock_3_link_map:%lu\ndto->cv_0_link_map:%lu\ndto->cv_1_link_map:%lu\ndto->cv_2_link_map:%lu\ndto->cv_3_link_map:%lu\n",
            //     dto->ana_in_0_link_map,
            //     dto->ana_in_1_link_map,
            //     dto->ana_in_2_link_map,
            //     dto->ana_in_3_link_map,
            //     dto->main_bus_link_map,
            //     dto->cue_bus_link_map,
            //     dto->annot_bus_link_map,
            //     dto->deck_1_input_link_map,
            //     dto->deck_1_edge_link_map,
            //     dto->deck_1_prefade_link_map,
            //     dto->deck_1_postfade_link_map,
            //     dto->deck_1_cue_link_map,
            //     dto->deck_2_input_link_map,
            //     dto->deck_2_edge_link_map,
            //     dto->deck_2_prefade_link_map,
            //     dto->deck_2_postfade_link_map,
            //     dto->deck_2_cue_link_map,
            //     dto->deck_ext_input_link_map,
            //     dto->deck_ext_edge_link_map,
            //     dto->deck_ext_prefade_link_map,
            //     dto->deck_ext_postfade_link_map,
            //     dto->deck_ext_cue_link_map,
            //     dto->aux_bus_0_link_map,
            //     dto->aux_bus_1_link_map,
            //     dto->aux_bus_2_link_map,
            //     dto->aux_bus_3_link_map,
            //     dto->clock_0_link_map,
            //     dto->clock_1_link_map,
            //     dto->clock_2_link_map,
            //     dto->clock_3_link_map,
            //     dto->cv_0_link_map,
            //     dto->cv_1_link_map,
            //     dto->cv_2_link_map,
            //     dto->cv_3_link_map
            // );
        }
        sqlite3_finalize( stmt );
    }

    // Populate DSP for all nodes
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->ana_in_0_dsp, dto->ana_in_0_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->ana_in_1_dsp, dto->ana_in_1_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->ana_in_2_dsp, dto->ana_in_2_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->ana_in_3_dsp, dto->ana_in_3_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->main_bus_dsp, dto->main_bus_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->cue_bus_dsp, dto->cue_bus_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->annot_bus_dsp, dto->annot_bus_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->record_bus_dsp, dto->record_bus_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->deck_1_input_dsp, dto->deck_1_input_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->deck_1_prefade_dsp, dto->deck_1_prefade_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->deck_1_postfade_dsp, dto->deck_1_postfade_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->deck_1_cue_dsp, dto->deck_1_cue_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->deck_2_input_dsp, dto->deck_2_input_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->deck_2_prefade_dsp, dto->deck_2_prefade_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->deck_2_postfade_dsp, dto->deck_2_postfade_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->deck_2_cue_dsp, dto->deck_2_cue_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->deck_ext_input_dsp, dto->deck_ext_input_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->deck_ext_prefade_dsp, dto->deck_ext_prefade_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->deck_ext_postfade_dsp, dto->deck_ext_postfade_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->deck_ext_cue_dsp, dto->deck_ext_cue_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->aux_bus_0_dsp, dto->aux_bus_0_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->aux_bus_1_dsp, dto->aux_bus_1_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->aux_bus_2_dsp, dto->aux_bus_2_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->aux_bus_3_dsp, dto->aux_bus_3_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->cv_0_dsp, dto->cv_0_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->cv_1_dsp, dto->cv_1_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->cv_2_dsp, dto->cv_2_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->cv_3_dsp, dto->cv_3_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->xfade_a_dsp, dto->xfade_a_dsp_eid, db );
    zdj_soundcard_fetch_dsp_for_entity_id( &dto->xfade_b_dsp, dto->xfade_b_dsp_eid, db );

    int res = zdj_sql_close( db );
    if( res ) {
        return ZDJ_ERROR_LIBRARY_DB_ERROR;
    } else {
        return ZDJ_ERROR_OKAY;
    }

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_soundcard_store_dto( zdj_soundcard_dto_t * dto ) {    
    
    sqlite3 * db = zdj_sql_open( ZDJ_SOUNDCARD_DB_PATH );

    if( !db ) { 
        printf( "failed to open zero db\n" );
        return ZDJ_ERROR_LIBRARY_DB_ERROR; 
    }

    int count = 0;
    int res;
    char sql[ 8192 ];
    // Set up for prepared stmt w/binds to use built-in string escaping.
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO %s VALUES(\'%s\',\'%s\',%d,%d,%d,%d,%d,%d,%d,%d,%lu,\'%s\',%d,%d,%d,%lu,\'%s\',%d,%d,%d,%lu,\'%s\',%d,%d,%d,%lu,\'%s\',%d,%d,%d,%lu,\'%s\',%d,%d,%lu,\'%s\',%d,%d,%lu,\'%s\',%d,%d,\'%s\',%d,%lu,\'%s\',%lu,%lu,\'%s\',%lu,\'%s\',%lu,\'%s\',%lu,\'%s\',%lu,%lu,\'%s\',%lu,\'%s\',%lu,\'%s\',%lu,\'%s\',%lu,%lu,\'%s\',%lu,\'%s\',%lu,\'%s\',%lu,\'%s\',%d,%d,%lu,\'%s\',%d,%d,%lu,\'%s\',%d,%d,%lu,\'%s\',%d,%d,%d,%lu,%d,%f,%d,%d,%lu,%d,%f,%d,%d,%lu,%d,%f,%d,%d,%lu,%d,%f,%d,%d,%lu,\'%s\',%d,%d,%d,%lu,\'%s\',%d,%d,%d,%lu,\'%s\',%d,%d,%d,%lu,\'%s\',%d,%d,%lu,\'%s\',%lu,\'%s\')\n"

        // Or update existing record
        "ON CONFLICT(entity_id) DO UPDATE SET entity_id=\'%s\',name=\'%s\',ana_out_0_sig=%d,ana_out_0_stereo=%d,ana_out_1_sig=%d,ana_out_1_stereo=%d,ana_out_2_sig=%d,ana_out_2_stereo=%d,ana_out_3_sig=%d,ana_out_3_stereo=%d,ana_in_0_link_map=%lu,ana_in_0_dsp_eid=\'%s\',ana_in_0_sig=%d,ana_in_0_stereo=%d,ana_in_0_mute=%d,ana_in_1_link_map=%lu,ana_in_1_dsp_eid=\'%s\',ana_in_1_sig=%d,ana_in_1_stereo=%d,ana_in_1_mute=%d,ana_in_2_link_map=%lu,ana_in_2_dsp_eid=\'%s\',ana_in_2_sig=%d,ana_in_2_stereo=%d,ana_in_2_mute=%d,ana_in_3_link_map=%lu,ana_in_3_dsp_eid=\'%s\',ana_in_3_sig=%d,ana_in_3_stereo=%d,ana_in_3_mute=%d,main_bus_link_map=%lu,main_bus_dsp_eid=\'%s\',main_bus_stereo=%d,main_bus_mute=%d,cue_bus_link_map=%lu,cue_bus_dsp_eid=\'%s\',cue_bus_stereo=%d,cue_bus_mute=%d,annot_bus_link_map=%lu,annot_bus_dsp_eid=\'%s\',annot_bus_stereo=%d,annot_bus_mute=%d,record_bus_dsp_eid=\'%s\',record_bus_stereo=%d,deck_1_input_link_map=%lu,deck_1_input_dsp_eid=\'%s\',deck_1_edge_link_map=%lu,deck_1_prefade_link_map=%lu,deck_1_prefade_dsp_eid=\'%s\',deck_1_postfade_link_map=%lu,deck_1_postfade_dsp_eid=\'%s\',deck_1_cue_link_map=%lu,deck_1_cue_dsp_eid=\'%s\',deck_2_input_link_map=%lu,deck_2_input_dsp_eid=\'%s\',deck_2_edge_link_map=%lu,deck_2_prefade_link_map=%lu,deck_2_prefade_dsp_eid=\'%s\',deck_2_postfade_link_map=%lu,deck_2_postfade_dsp_eid=\'%s\',deck_2_cue_link_map=%lu,deck_2_cue_dsp_eid=\'%s\',deck_ext_input_link_map=%lu,deck_ext_input_dsp_eid=\'%s\',deck_ext_edge_link_map=%lu,deck_ext_prefade_link_map=%lu,deck_ext_prefade_dsp_eid=\'%s\',deck_ext_postfade_link_map=%lu,deck_ext_postfade_dsp_eid=\'%s\',deck_ext_cue_link_map=%lu,deck_ext_cue_dsp_eid=\'%s\',aux_bus_0_link_map=%lu,aux_bus_0_dsp_eid=\'%s\',aux_bus_0_stereo=%d,aux_bus_0_mute=%d,aux_bus_1_link_map=%lu,aux_bus_1_dsp_eid=\'%s\',aux_bus_1_stereo=%d,aux_bus_1_mute=%d,aux_bus_2_link_map=%lu,aux_bus_2_dsp_eid=\'%s\',aux_bus_2_stereo=%d,aux_bus_2_mute=%d,aux_bus_3_link_map=%lu,aux_bus_3_dsp_eid=\'%s\',aux_bus_3_stereo=%d,aux_bus_3_mute=%d,clock_0_sig=%d,clock_0_link_map=%lu,clock_0_source=%d,clock_0_val=%f,clock_0_sync=%d,clock_1_sig=%d,clock_1_link_map=%lu,clock_1_source=%d,clock_1_val=%f,clock_1_sync=%d,clock_2_sig=%d,clock_2_link_map=%lu,clock_2_source=%d,clock_2_val=%f,clock_2_sync=%d,clock_3_sig=%d,clock_3_link_map=%lu,clock_3_source=%d,clock_3_val=%f,clock_3_sync=%d,cv_0_sig=%d,cv_0_link_map=%lu,cv_0_dsp_eid=\'%s\',cv_0_source=%d,cv_0_mute=%d,cv_1_sig=%d,cv_1_link_map=%lu,cv_1_dsp_eid=\'%s\',cv_1_source=%d,cv_1_mute=%d,cv_2_sig=%d,cv_2_link_map=%lu,cv_2_dsp_eid=\'%s\',cv_2_source=%d,cv_2_mute=%d,cv_3_sig=%d,cv_3_link_map=%lu,cv_3_dsp_eid=\'%s\',cv_3_source=%d,cv_3_mute=%d,xfade_a_link_map=%lu,xfade_a_dsp_eid=\'%s\',xfade_b_link_map=%lu,xfade_b_dsp_eid=\'%s\'",

        // Table Name
        ZDJ_SOUNDCARD_LINKAGE_TABLE,
        dto->entity_id,
        dto->name,

        dto->ana_out_0_sig,
        dto->ana_out_0_stereo,
        dto->ana_out_1_sig,
        dto->ana_out_1_stereo,
        dto->ana_out_2_sig,
        dto->ana_out_2_stereo,
        dto->ana_out_3_sig,
        dto->ana_out_3_stereo,

        dto->ana_in_0_link_map,
        dto->ana_in_0_dsp_eid,
        dto->ana_in_0_sig,
        dto->ana_in_0_stereo,
        dto->ana_in_0_mute,
        dto->ana_in_1_link_map,
        dto->ana_in_1_dsp_eid,
        dto->ana_in_1_sig,
        dto->ana_in_1_stereo,
        dto->ana_in_1_mute,
        dto->ana_in_2_link_map,
        dto->ana_in_2_dsp_eid,
        dto->ana_in_2_sig,
        dto->ana_in_2_stereo,
        dto->ana_in_2_mute,
        dto->ana_in_3_link_map,
        dto->ana_in_3_dsp_eid,
        dto->ana_in_3_sig,
        dto->ana_in_3_stereo,
        dto->ana_in_3_mute,

        dto->main_bus_link_map,
        dto->main_bus_dsp_eid,
        dto->main_bus_stereo,
        dto->main_bus_mute,

        dto->cue_bus_link_map,
        dto->cue_bus_dsp_eid,
        dto->cue_bus_stereo,
        dto->cue_bus_mute,

        dto->annot_bus_link_map,
        dto->annot_bus_dsp_eid,
        dto->annot_bus_stereo,
        dto->annot_bus_mute,

        dto->record_bus_dsp_eid,
        dto->record_bus_stereo,

        dto->deck_1_input_link_map,
        dto->deck_1_input_dsp_eid,
        dto->deck_1_edge_link_map,
        dto->deck_1_prefade_link_map,
        dto->deck_1_prefade_dsp_eid,
        dto->deck_1_postfade_link_map,
        dto->deck_1_postfade_dsp_eid,
        dto->deck_1_cue_link_map,
        dto->deck_1_cue_dsp_eid,

        dto->deck_2_input_link_map,
        dto->deck_2_input_dsp_eid,
        dto->deck_2_edge_link_map,
        dto->deck_2_prefade_link_map,
        dto->deck_2_prefade_dsp_eid,
        dto->deck_2_postfade_link_map,
        dto->deck_2_postfade_dsp_eid,
        dto->deck_2_cue_link_map,
        dto->deck_2_cue_dsp_eid,

        dto->deck_ext_input_link_map,
        dto->deck_ext_input_dsp_eid,
        dto->deck_ext_edge_link_map,
        dto->deck_ext_prefade_link_map,
        dto->deck_ext_prefade_dsp_eid,
        dto->deck_ext_postfade_link_map,
        dto->deck_ext_postfade_dsp_eid,
        dto->deck_ext_cue_link_map,
        dto->deck_ext_cue_dsp_eid,

        dto->aux_bus_0_link_map,
        dto->aux_bus_0_dsp_eid,
        dto->aux_bus_0_stereo,
        dto->aux_bus_0_mute,
        dto->aux_bus_1_link_map,
        dto->aux_bus_1_dsp_eid,
        dto->aux_bus_1_stereo,
        dto->aux_bus_1_mute,
        dto->aux_bus_2_link_map,
        dto->aux_bus_2_dsp_eid,
        dto->aux_bus_2_stereo,
        dto->aux_bus_2_mute,
        dto->aux_bus_3_link_map,
        dto->aux_bus_3_dsp_eid,
        dto->aux_bus_3_stereo,
        dto->aux_bus_3_mute,

        dto->clock_0_sig,
        dto->clock_0_link_map,
        dto->clock_0_source,
        dto->clock_0_val,
        dto->clock_0_sync,
        dto->clock_1_sig,
        dto->clock_1_link_map,
        dto->clock_1_source,
        dto->clock_1_val,
        dto->clock_1_sync,
        dto->clock_2_sig,
        dto->clock_2_link_map,
        dto->clock_2_source,
        dto->clock_2_val,
        dto->clock_2_sync,
        dto->clock_3_sig,
        dto->clock_3_link_map,
        dto->clock_3_source,
        dto->clock_3_val,
        dto->clock_3_sync,

        dto->cv_0_sig,
        dto->cv_0_link_map,
        dto->cv_0_dsp_eid,
        dto->cv_0_source,
        dto->cv_0_mute,
        dto->cv_1_sig,
        dto->cv_1_link_map,
        dto->cv_1_dsp_eid,
        dto->cv_1_source,
        dto->cv_1_mute,
        dto->cv_2_sig,
        dto->cv_2_link_map,
        dto->cv_2_dsp_eid,
        dto->cv_2_source,
        dto->cv_2_mute,
        dto->cv_3_sig,
        dto->cv_3_link_map,
        dto->cv_3_dsp_eid,
        dto->cv_3_source,
        dto->cv_3_mute,

        dto->xfade_a_link_map,
        dto->xfade_a_dsp_eid,
        dto->xfade_b_link_map,
        dto->xfade_b_dsp_eid,



        dto->entity_id,
        dto->name,

        dto->ana_out_0_sig,
        dto->ana_out_0_stereo,
        dto->ana_out_1_sig,
        dto->ana_out_1_stereo,
        dto->ana_out_2_sig,
        dto->ana_out_2_stereo,
        dto->ana_out_3_sig,
        dto->ana_out_3_stereo,

        dto->ana_in_0_link_map,
        dto->ana_in_0_dsp_eid,
        dto->ana_in_0_sig,
        dto->ana_in_0_stereo,
        dto->ana_in_0_mute,
        dto->ana_in_1_link_map,
        dto->ana_in_1_dsp_eid,
        dto->ana_in_1_sig,
        dto->ana_in_1_stereo,
        dto->ana_in_1_mute,
        dto->ana_in_2_link_map,
        dto->ana_in_2_dsp_eid,
        dto->ana_in_2_sig,
        dto->ana_in_2_stereo,
        dto->ana_in_2_mute,
        dto->ana_in_3_link_map,
        dto->ana_in_3_dsp_eid,
        dto->ana_in_3_sig,
        dto->ana_in_3_stereo,
        dto->ana_in_3_mute,

        dto->main_bus_link_map,
        dto->main_bus_dsp_eid,
        dto->main_bus_stereo,
        dto->main_bus_mute,

        dto->cue_bus_link_map,
        dto->cue_bus_dsp_eid,
        dto->cue_bus_stereo,
        dto->cue_bus_mute,

        dto->annot_bus_link_map,
        dto->annot_bus_dsp_eid,
        dto->annot_bus_stereo,
        dto->annot_bus_mute,

        dto->record_bus_dsp_eid,
        dto->record_bus_stereo,

        dto->deck_1_input_link_map,
        dto->deck_1_input_dsp_eid,
        dto->deck_1_edge_link_map,
        dto->deck_1_prefade_link_map,
        dto->deck_1_prefade_dsp_eid,
        dto->deck_1_postfade_link_map,
        dto->deck_1_postfade_dsp_eid,
        dto->deck_1_cue_link_map,
        dto->deck_1_cue_dsp_eid,

        dto->deck_2_input_link_map,
        dto->deck_2_input_dsp_eid,
        dto->deck_2_edge_link_map,
        dto->deck_2_prefade_link_map,
        dto->deck_2_prefade_dsp_eid,
        dto->deck_2_postfade_link_map,
        dto->deck_2_postfade_dsp_eid,
        dto->deck_2_cue_link_map,
        dto->deck_2_cue_dsp_eid,

        dto->deck_ext_input_link_map,
        dto->deck_ext_input_dsp_eid,
        dto->deck_ext_edge_link_map,
        dto->deck_ext_prefade_link_map,
        dto->deck_ext_prefade_dsp_eid,
        dto->deck_ext_postfade_link_map,
        dto->deck_ext_postfade_dsp_eid,
        dto->deck_ext_cue_link_map,
        dto->deck_ext_cue_dsp_eid,

        dto->aux_bus_0_link_map,
        dto->aux_bus_0_dsp_eid,
        dto->aux_bus_0_stereo,
        dto->aux_bus_0_mute,
        dto->aux_bus_1_link_map,
        dto->aux_bus_1_dsp_eid,
        dto->aux_bus_1_stereo,
        dto->aux_bus_1_mute,
        dto->aux_bus_2_link_map,
        dto->aux_bus_2_dsp_eid,
        dto->aux_bus_2_stereo,
        dto->aux_bus_2_mute,
        dto->aux_bus_3_link_map,
        dto->aux_bus_3_dsp_eid,
        dto->aux_bus_3_stereo,
        dto->aux_bus_3_mute,

        dto->clock_0_sig,
        dto->clock_0_link_map,
        dto->clock_0_source,
        dto->clock_0_val,
        dto->clock_0_sync,
        dto->clock_1_sig,
        dto->clock_1_link_map,
        dto->clock_1_source,
        dto->clock_1_val,
        dto->clock_1_sync,
        dto->clock_2_sig,
        dto->clock_2_link_map,
        dto->clock_2_source,
        dto->clock_2_val,
        dto->clock_2_sync,
        dto->clock_3_sig,
        dto->clock_3_link_map,
        dto->clock_3_source,
        dto->clock_3_val,
        dto->clock_3_sync,

        dto->cv_0_sig,
        dto->cv_0_link_map,
        dto->cv_0_dsp_eid,
        dto->cv_0_source,
        dto->cv_0_mute,
        dto->cv_1_sig,
        dto->cv_1_link_map,
        dto->cv_1_dsp_eid,
        dto->cv_1_source,
        dto->cv_1_mute,
        dto->cv_2_sig,
        dto->cv_2_link_map,
        dto->cv_2_dsp_eid,
        dto->cv_2_source,
        dto->cv_2_mute,
        dto->cv_3_sig,
        dto->cv_3_link_map,
        dto->cv_3_dsp_eid,
        dto->cv_3_source,
        dto->cv_3_mute,

        dto->xfade_a_link_map,
        dto->xfade_a_dsp_eid,
        dto->xfade_b_link_map,
        dto->xfade_b_dsp_eid
    );

    zdj_sql_exec( sql, db );

    // printf( "sql: %s\n", sql );


    // Store DSP for all nodes
    zdj_soundcard_store_dsp_for_entity_id( &dto->ana_in_0_dsp, dto->ana_in_0_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->ana_in_1_dsp, dto->ana_in_1_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->ana_in_2_dsp, dto->ana_in_2_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->ana_in_3_dsp, dto->ana_in_3_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->main_bus_dsp, dto->main_bus_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->cue_bus_dsp, dto->cue_bus_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->annot_bus_dsp, dto->annot_bus_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->record_bus_dsp, dto->record_bus_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->deck_1_input_dsp, dto->deck_1_input_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->deck_1_prefade_dsp, dto->deck_1_prefade_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->deck_1_postfade_dsp, dto->deck_1_postfade_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->deck_1_cue_dsp, dto->deck_1_cue_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->deck_2_input_dsp, dto->deck_2_input_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->deck_2_prefade_dsp, dto->deck_2_prefade_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->deck_2_postfade_dsp, dto->deck_2_postfade_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->deck_2_cue_dsp, dto->deck_2_cue_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->deck_ext_input_dsp, dto->deck_ext_input_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->deck_ext_prefade_dsp, dto->deck_ext_prefade_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->deck_ext_postfade_dsp, dto->deck_ext_postfade_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->deck_ext_cue_dsp, dto->deck_ext_cue_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->aux_bus_0_dsp, dto->aux_bus_0_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->aux_bus_1_dsp, dto->aux_bus_1_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->aux_bus_2_dsp, dto->aux_bus_2_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->aux_bus_3_dsp, dto->aux_bus_3_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->cv_0_dsp, dto->cv_0_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->cv_1_dsp, dto->cv_1_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->cv_2_dsp, dto->cv_2_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->cv_3_dsp, dto->cv_3_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->xfade_a_dsp, dto->xfade_a_dsp_eid, db );
    zdj_soundcard_store_dsp_for_entity_id( &dto->xfade_b_dsp, dto->xfade_b_dsp_eid, db );

    zdj_sql_db_flush( db );
    res = zdj_sql_close( db );

    if( res ) {
        return ZDJ_ERROR_LIBRARY_DB_ERROR;
    } else {
        return ZDJ_ERROR_OKAY;
    }

    return ZDJ_ERROR_OKAY;
}


void zdj_soundcard_fetch_dsp_for_entity_id( zdj_soundcard_dsp_dto_t * dto, char * dsp_eid, sqlite3 * db ) {
    // Grab all the values from db
    int sql_res;
    char sql[ 1024 ];
    sprintf( sql, "select * from %s where entity_id=\'%s\'", ZDJ_SOUNDCARD_DSP_TABLE, dsp_eid );
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( sql, db );
    if( stmt ) {
        while ( ( sql_res = sqlite3_step( stmt ) ) == SQLITE_ROW ) {
            strcpy( dto->entity_id, (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_DSP_COL_ENTITY_ID ) );
            dto->gain = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_GAIN );
            dto->pan = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_PAN );

            dto->stages[ 0 ].type = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_0_TYPE );
            dto->stages[ 0 ].id = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_0_ID );
            dto->stages[ 0 ].knob_0 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_0 );
            dto->stages[ 0 ].knob_1 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_1 );
            dto->stages[ 0 ].knob_2 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_2 );
            dto->stages[ 0 ].knob_3 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_3 );
            dto->stages[ 0 ].knob_4 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_4 );
            dto->stages[ 0 ].knob_5 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_5 );
            dto->stages[ 0 ].knob_6 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_6 );
            dto->stages[ 0 ].knob_7 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_7 );

            dto->stages[ 1 ].type = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_1_TYPE );
            dto->stages[ 1 ].id = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_1_ID );
            dto->stages[ 1 ].knob_0 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_0 );
            dto->stages[ 1 ].knob_1 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_1 );
            dto->stages[ 1 ].knob_2 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_2 );
            dto->stages[ 1 ].knob_3 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_3 );
            dto->stages[ 1 ].knob_4 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_4 );
            dto->stages[ 1 ].knob_5 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_5 );
            dto->stages[ 1 ].knob_6 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_6 );
            dto->stages[ 1 ].knob_7 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_7 );

            dto->stages[ 2 ].type = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_2_TYPE );
            dto->stages[ 2 ].id = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_2_ID );
            dto->stages[ 2 ].knob_0 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_0 );
            dto->stages[ 2 ].knob_1 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_1 );
            dto->stages[ 2 ].knob_2 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_2 );
            dto->stages[ 2 ].knob_3 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_3 );
            dto->stages[ 2 ].knob_4 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_4 );
            dto->stages[ 2 ].knob_5 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_5 );
            dto->stages[ 2 ].knob_6 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_6 );
            dto->stages[ 2 ].knob_7 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_7 );

            dto->stages[ 3 ].type = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_3_TYPE );
            dto->stages[ 3 ].id = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_3_ID );
            dto->stages[ 3 ].knob_0 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_0 );
            dto->stages[ 3 ].knob_1 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_1 );
            dto->stages[ 3 ].knob_2 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_2 );
            dto->stages[ 3 ].knob_3 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_3 );
            dto->stages[ 3 ].knob_4 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_4 );
            dto->stages[ 3 ].knob_5 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_5 );
            dto->stages[ 3 ].knob_6 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_6 );
            dto->stages[ 3 ].knob_7 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_7 );

            dto->stages[ 4 ].type = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_4_TYPE );
            dto->stages[ 4 ].id = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_4_ID );
            dto->stages[ 4 ].knob_0 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_0 );
            dto->stages[ 4 ].knob_1 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_1 );
            dto->stages[ 4 ].knob_2 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_2 );
            dto->stages[ 4 ].knob_3 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_3 );
            dto->stages[ 4 ].knob_4 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_4 );
            dto->stages[ 4 ].knob_5 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_5 );
            dto->stages[ 4 ].knob_6 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_6 );
            dto->stages[ 4 ].knob_7 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_7 );

            dto->stages[ 5 ].type = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_5_TYPE );
            dto->stages[ 5 ].id = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_5_ID );
            dto->stages[ 5 ].knob_0 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_0 );
            dto->stages[ 5 ].knob_1 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_1 );
            dto->stages[ 5 ].knob_2 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_2 );
            dto->stages[ 5 ].knob_3 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_3 );
            dto->stages[ 5 ].knob_4 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_4 );
            dto->stages[ 5 ].knob_5 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_5 );
            dto->stages[ 5 ].knob_6 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_6 );
            dto->stages[ 5 ].knob_7 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_7 );

            dto->stages[ 6 ].type = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_6_TYPE );
            dto->stages[ 6 ].id = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_6_ID );
            dto->stages[ 6 ].knob_0 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_0 );
            dto->stages[ 6 ].knob_1 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_1 );
            dto->stages[ 6 ].knob_2 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_2 );
            dto->stages[ 6 ].knob_3 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_3 );
            dto->stages[ 6 ].knob_4 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_4 );
            dto->stages[ 6 ].knob_5 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_5 );
            dto->stages[ 6 ].knob_6 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_6 );
            dto->stages[ 6 ].knob_7 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_7 );

            dto->stages[ 7 ].type = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_7_TYPE );
            dto->stages[ 7 ].id = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_7_ID );
            dto->stages[ 7 ].knob_0 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_0 );
            dto->stages[ 7 ].knob_1 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_1 );
            dto->stages[ 7 ].knob_2 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_2 );
            dto->stages[ 7 ].knob_3 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_3 );
            dto->stages[ 7 ].knob_4 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_4 );
            dto->stages[ 7 ].knob_5 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_5 );
            dto->stages[ 7 ].knob_6 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_6 );
            dto->stages[ 7 ].knob_7 = (float)sqlite3_column_double ( stmt, ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_7 );
        }
        sqlite3_finalize( stmt );
    }

    // Attach dsp handlers, state storage, etc. for everything
    zdj_soundcard_put_dsp_state_for_dto( dto );
}


void zdj_soundcard_store_dsp_for_entity_id( zdj_soundcard_dsp_dto_t * dto, char * dsp_eid, sqlite3 * db ) {
    int count = 0;
    int res;
    char sql[ 8192 ];
    // Set up for prepared stmt w/binds to use built-in string escaping.
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO %s VALUES(\'%s\',%f,%f,%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%d,%d,%f,%f,%f,%f,%f,%f,%f,%f)\n"

        // Or update existing record
        "ON CONFLICT(entity_id) DO UPDATE SET entity_id=\'%s\',gain=%f,pan=%f,stg_0_type=%d,stg_0_id=%d,stg_0_knob_0=%f,stg_0_knob_1=%f,stg_0_knob_2=%f,stg_0_knob_3=%f,stg_0_knob_4=%f,stg_0_knob_5=%f,stg_0_knob_6=%f,stg_0_knob_7=%f,stg_1_type=%d,stg_1_id=%d,stg_1_knob_0=%f,stg_1_knob_1=%f,stg_1_knob_2=%f,stg_1_knob_3=%f,stg_1_knob_4=%f,stg_1_knob_5=%f,stg_1_knob_6=%f,stg_1_knob_7=%f,stg_2_type=%d,stg_2_id=%d,stg_2_knob_0=%f,stg_2_knob_1=%f,stg_2_knob_2=%f,stg_2_knob_3=%f,stg_2_knob_4=%f,stg_2_knob_5=%f,stg_2_knob_6=%f,stg_2_knob_7=%f,stg_3_type=%d,stg_3_id=%d,stg_3_knob_0=%f,stg_3_knob_1=%f,stg_3_knob_2=%f,stg_3_knob_3=%f,stg_3_knob_4=%f,stg_3_knob_5=%f,stg_3_knob_6=%f,stg_3_knob_7=%f,stg_4_type=%d,stg_4_id=%d,stg_4_knob_0=%f,stg_4_knob_1=%f,stg_4_knob_2=%f,stg_4_knob_3=%f,stg_4_knob_4=%f,stg_4_knob_5=%f,stg_4_knob_6=%f,stg_4_knob_7=%f,stg_5_type=%d,stg_5_id=%d,stg_5_knob_0=%f,stg_5_knob_1=%f,stg_5_knob_2=%f,stg_5_knob_3=%f,stg_5_knob_4=%f,stg_5_knob_5=%f,stg_5_knob_6=%f,stg_5_knob_7=%f,stg_6_type=%d,stg_6_id=%d,stg_6_knob_0=%f,stg_6_knob_1=%f,stg_6_knob_2=%f,stg_6_knob_3=%f,stg_6_knob_4=%f,stg_6_knob_5=%f,stg_6_knob_6=%f,stg_6_knob_7=%f,stg_7_type=%d,stg_7_id=%d,stg_7_knob_0=%f,stg_7_knob_1=%f,stg_7_knob_2=%f,stg_7_knob_3=%f,stg_7_knob_4=%f,stg_7_knob_5=%f,stg_7_knob_6=%f,stg_7_knob_7=%f",

        // Table Name
        ZDJ_SOUNDCARD_DSP_TABLE,
        dto->entity_id,
        dto->gain,
        dto->pan,
        dto->stages[ 0 ].type,
        dto->stages[ 0 ].id,
        dto->stages[ 0 ].knob_0,
        dto->stages[ 0 ].knob_1,
        dto->stages[ 0 ].knob_2,
        dto->stages[ 0 ].knob_3,
        dto->stages[ 0 ].knob_4,
        dto->stages[ 0 ].knob_5,
        dto->stages[ 0 ].knob_6,
        dto->stages[ 0 ].knob_7,
        dto->stages[ 1 ].type,
        dto->stages[ 1 ].id,
        dto->stages[ 1 ].knob_0,
        dto->stages[ 1 ].knob_1,
        dto->stages[ 1 ].knob_2,
        dto->stages[ 1 ].knob_3,
        dto->stages[ 1 ].knob_4,
        dto->stages[ 1 ].knob_5,
        dto->stages[ 1 ].knob_6,
        dto->stages[ 1 ].knob_7,
        dto->stages[ 2 ].type,
        dto->stages[ 2 ].id,
        dto->stages[ 2 ].knob_0,
        dto->stages[ 2 ].knob_1,
        dto->stages[ 2 ].knob_2,
        dto->stages[ 2 ].knob_3,
        dto->stages[ 2 ].knob_4,
        dto->stages[ 2 ].knob_5,
        dto->stages[ 2 ].knob_6,
        dto->stages[ 2 ].knob_7,
        dto->stages[ 3 ].type,
        dto->stages[ 3 ].id,
        dto->stages[ 3 ].knob_0,
        dto->stages[ 3 ].knob_1,
        dto->stages[ 3 ].knob_2,
        dto->stages[ 3 ].knob_3,
        dto->stages[ 3 ].knob_4,
        dto->stages[ 3 ].knob_5,
        dto->stages[ 3 ].knob_6,
        dto->stages[ 3 ].knob_7,
        dto->stages[ 4 ].type,
        dto->stages[ 4 ].id,
        dto->stages[ 4 ].knob_0,
        dto->stages[ 4 ].knob_1,
        dto->stages[ 4 ].knob_2,
        dto->stages[ 4 ].knob_3,
        dto->stages[ 4 ].knob_4,
        dto->stages[ 4 ].knob_5,
        dto->stages[ 4 ].knob_6,
        dto->stages[ 4 ].knob_7,
        dto->stages[ 5 ].type,
        dto->stages[ 5 ].id,
        dto->stages[ 5 ].knob_0,
        dto->stages[ 5 ].knob_1,
        dto->stages[ 5 ].knob_2,
        dto->stages[ 5 ].knob_3,
        dto->stages[ 5 ].knob_4,
        dto->stages[ 5 ].knob_5,
        dto->stages[ 5 ].knob_6,
        dto->stages[ 5 ].knob_7,
        dto->stages[ 6 ].type,
        dto->stages[ 6 ].id,
        dto->stages[ 6 ].knob_0,
        dto->stages[ 6 ].knob_1,
        dto->stages[ 6 ].knob_2,
        dto->stages[ 6 ].knob_3,
        dto->stages[ 6 ].knob_4,
        dto->stages[ 6 ].knob_5,
        dto->stages[ 6 ].knob_6,
        dto->stages[ 6 ].knob_7,
        dto->stages[ 7 ].type,
        dto->stages[ 7 ].id,
        dto->stages[ 7 ].knob_0,
        dto->stages[ 7 ].knob_1,
        dto->stages[ 7 ].knob_2,
        dto->stages[ 7 ].knob_3,
        dto->stages[ 7 ].knob_4,
        dto->stages[ 7 ].knob_5,
        dto->stages[ 7 ].knob_6,
        dto->stages[ 7 ].knob_7,


        dto->entity_id,
        dto->gain,
        dto->pan,
        dto->stages[ 0 ].type,
        dto->stages[ 0 ].id,
        dto->stages[ 0 ].knob_0,
        dto->stages[ 0 ].knob_1,
        dto->stages[ 0 ].knob_2,
        dto->stages[ 0 ].knob_3,
        dto->stages[ 0 ].knob_4,
        dto->stages[ 0 ].knob_5,
        dto->stages[ 0 ].knob_6,
        dto->stages[ 0 ].knob_7,
        dto->stages[ 1 ].type,
        dto->stages[ 1 ].id,
        dto->stages[ 1 ].knob_0,
        dto->stages[ 1 ].knob_1,
        dto->stages[ 1 ].knob_2,
        dto->stages[ 1 ].knob_3,
        dto->stages[ 1 ].knob_4,
        dto->stages[ 1 ].knob_5,
        dto->stages[ 1 ].knob_6,
        dto->stages[ 1 ].knob_7,
        dto->stages[ 2 ].type,
        dto->stages[ 2 ].id,
        dto->stages[ 2 ].knob_0,
        dto->stages[ 2 ].knob_1,
        dto->stages[ 2 ].knob_2,
        dto->stages[ 2 ].knob_3,
        dto->stages[ 2 ].knob_4,
        dto->stages[ 2 ].knob_5,
        dto->stages[ 2 ].knob_6,
        dto->stages[ 2 ].knob_7,
        dto->stages[ 3 ].type,
        dto->stages[ 3 ].id,
        dto->stages[ 3 ].knob_0,
        dto->stages[ 3 ].knob_1,
        dto->stages[ 3 ].knob_2,
        dto->stages[ 3 ].knob_3,
        dto->stages[ 3 ].knob_4,
        dto->stages[ 3 ].knob_5,
        dto->stages[ 3 ].knob_6,
        dto->stages[ 3 ].knob_7,
        dto->stages[ 4 ].type,
        dto->stages[ 4 ].id,
        dto->stages[ 4 ].knob_0,
        dto->stages[ 4 ].knob_1,
        dto->stages[ 4 ].knob_2,
        dto->stages[ 4 ].knob_3,
        dto->stages[ 4 ].knob_4,
        dto->stages[ 4 ].knob_5,
        dto->stages[ 4 ].knob_6,
        dto->stages[ 4 ].knob_7,
        dto->stages[ 5 ].type,
        dto->stages[ 5 ].id,
        dto->stages[ 5 ].knob_0,
        dto->stages[ 5 ].knob_1,
        dto->stages[ 5 ].knob_2,
        dto->stages[ 5 ].knob_3,
        dto->stages[ 5 ].knob_4,
        dto->stages[ 5 ].knob_5,
        dto->stages[ 5 ].knob_6,
        dto->stages[ 5 ].knob_7,
        dto->stages[ 6 ].type,
        dto->stages[ 6 ].id,
        dto->stages[ 6 ].knob_0,
        dto->stages[ 6 ].knob_1,
        dto->stages[ 6 ].knob_2,
        dto->stages[ 6 ].knob_3,
        dto->stages[ 6 ].knob_4,
        dto->stages[ 6 ].knob_5,
        dto->stages[ 6 ].knob_6,
        dto->stages[ 6 ].knob_7,
        dto->stages[ 7 ].type,
        dto->stages[ 7 ].id,
        dto->stages[ 7 ].knob_0,
        dto->stages[ 7 ].knob_1,
        dto->stages[ 7 ].knob_2,
        dto->stages[ 7 ].knob_3,
        dto->stages[ 7 ].knob_4,
        dto->stages[ 7 ].knob_5,
        dto->stages[ 7 ].knob_6,
        dto->stages[ 7 ].knob_7
    );

    zdj_sql_exec( sql, db );
}