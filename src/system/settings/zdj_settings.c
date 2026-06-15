#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/system/sql/zdj_sql.h>
#include <zerodj/ui/zdj_ui.h>

sqlite3 * zdj_setting_db;

static void _create_db_table( void );
static void _reset_default( void );

zdj_error_type_t zdj_settings_init( void ) {
    // Open settings db
    zdj_setting_db = zdj_sql_open( ZDJ_SETTINGS_DB_PATH );
    if( !zdj_setting_db ) { 
        printf( "failed to open settings db\n" );
        return ZDJ_ERROR_FILE_MISSING; 
    }

    // Check for existence of settings table
    bool settings_table_exists = false;
    char sql[ 2048 ];
    sprintf( sql, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='Settings'" );
    int res;
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( sql, zdj_setting_db );
    if( stmt ) {
        while ( (( res = sqlite3_step( stmt ) ) == SQLITE_ROW) ) {
            int count = sqlite3_column_int(stmt, 0);
            if ( count > 0 ) {
                settings_table_exists = true;
            }
        }
        sqlite3_finalize( stmt );
    }

    // Create if not present
    if( !settings_table_exists ) { 
        _create_db_table( ); 
        _reset_default( );
    }

    // Set the UI refresh based on stored setting
    zdj_setting_t * rate_setting = zdj_setting_get( ZDJ_SETTING_REFRESH_RATE );
    if( rate_setting ) {
        zdj_setting_refresh_rate_t rate = rate_setting->i_val;
        switch ( rate ) {
            case ZDJ_SETTING_REFRESH_RATE_115: zdj_ui_set_refresh_hz( 115 ); break;
            case ZDJ_SETTING_REFRESH_RATE_60: zdj_ui_set_refresh_hz( 62 ); break;
            case ZDJ_SETTING_REFRESH_RATE_30: zdj_ui_set_refresh_hz( 30 );  break;
            case ZDJ_SETTING_REFRESH_RATE_20: zdj_ui_set_refresh_hz( 20 );  break;
            default: zdj_ui_set_refresh_hz( 30 ); break; 
        }
    } else {
        zdj_ui_set_refresh_hz( 30 );
    }

    return ZDJ_ERROR_OKAY;
}

// MAIN THREAD ONLY!
void zdj_drop_settings( void ) {
    zdj_sql_close( zdj_setting_db );
    remove( ZDJ_SETTINGS_DB_PATH );
    zdj_setting_db = zdj_sql_open( ZDJ_SETTINGS_DB_PATH );
    _create_db_table( );
    _reset_default( );
}

static void _create_db_table( void ) {
    printf( "Creating new Settings DB Table\n" );
    char sql[ 2048 ];
    strcpy( sql, "CREATE TABLE IF NOT EXISTS 'Settings' ( 'id' INT NOT NULL, 'type' INT NOT NULL, 'i_val' INT, 'b_val' INT, 'd_val' REAL, 'c_val' TEXT, PRIMARY KEY('id'))" );
    zdj_sql_exec( (char*)&sql, zdj_setting_db );
}

static void _reset_default( void ) {
    // Make screenshot index
    zdj_setting_set_int( ZDJ_SETTING_SCREENSHOT_COUNTER, 0 );
    zdj_setting_set_int( ZDJ_SETTING_RECORDING_COUNTER, 0 );
    zdj_setting_set_int( ZDJ_SETTING_DISPLAY_FLIP, 0 );
    zdj_setting_set_int( ZDJ_SETTING_REFRESH_RATE, 0 );
    zdj_setting_set_int( ZDJ_SETTING_DECK_SCRATCH_OVERRIDE, 0 );
    zdj_setting_set_bool( ZDJ_SETTING_LIB_MENU_SHOW_BPM, false );
    zdj_setting_set_bool( ZDJ_SETTING_LIB_MENU_SHOW_KEY, false );
    zdj_setting_set_bool( ZDJ_SETTING_LIB_MENU_SHOW_CAMELOT, false );
}

zdj_setting_t * zdj_setting_get( int id ) {
    zdj_setting_t * setting = NULL;
    // zdj_setting_t * setting = calloc( 1, sizeof( zdj_setting_t ) );
    // setting->id = id;

    char sql[ 256 ];
    sprintf( sql, "select * from Settings where id=%d", id );
    int res;
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( sql, zdj_setting_db );
    if( stmt ) {
        while ( (( res = sqlite3_step( stmt ) ) == SQLITE_ROW) ) {
            setting = calloc( 1, sizeof( zdj_setting_t ) );
            setting->id = id;
            setting->type = sqlite3_column_int ( stmt, 1 );
            setting->i_val = sqlite3_column_int ( stmt, 2 );
            setting->b_val = sqlite3_column_int ( stmt, 3 );
            setting->d_val = sqlite3_column_double ( stmt, 4 );
            char * c_val = (char*)sqlite3_column_text ( stmt, 5 );
            if( c_val ) { strcpy( setting->c_val, c_val ); }
        }
        sqlite3_finalize( stmt );
    }
    return setting;
}

zdj_error_type_t zdj_setting_set_int( int id, int val ) {
    zdj_setting_t * _setting = zdj_setting_get( id );
    if( !_setting ) { 
        _setting = calloc( 1, sizeof( zdj_setting_t ) ); 
        _setting->id = id;
        _setting->type = ZDJ_SETTING_TYPE_INT;
    }
    _setting->i_val = val;

    char sql[ 4096 ];
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO Settings(id,type,i_val,b_val,d_val,c_val) VALUES(%d,%d,%d,%d,%f,'%s')\n"
        // Or update existing record
        "ON CONFLICT(id) DO UPDATE SET id=%d,type=%d,i_val=%d,b_val=%d,d_val=%f,c_val='%s'",

        // Insert new record
        _setting->id,
        _setting->type,
        _setting->i_val,
        _setting->b_val,
        _setting->d_val,
        _setting->c_val,

        // Update existing record
        _setting->id,
        _setting->type,
        _setting->i_val,
        _setting->b_val,
        _setting->d_val,
        _setting->c_val
    );
    zdj_sql_exec( sql, zdj_setting_db );

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_setting_set_bool( int id, bool val ) {
    zdj_setting_t * _setting = zdj_setting_get( id );
    if( !_setting ) { 
        _setting = calloc( 1, sizeof( zdj_setting_t ) ); 
        _setting->id = id;
        _setting->type = ZDJ_SETTING_TYPE_BOOL;
    }
    _setting->b_val = val;

    char sql[ 4096 ];
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO Settings(id,type,i_val,b_val,d_val,c_val) VALUES(%d,%d,%d,%d,%f,'%s')\n"
        // Or update existing record
        "ON CONFLICT(id) DO UPDATE SET id=%d,type=%d,i_val=%d,b_val=%d,d_val=%f,c_val='%s'",

        // Insert new record
        _setting->id,
        _setting->type,
        _setting->i_val,
        _setting->b_val,
        _setting->d_val,
        _setting->c_val,

        // Update existing record
        _setting->id,
        _setting->type,
        _setting->i_val,
        _setting->b_val,
        _setting->d_val,
        _setting->c_val
    );
    zdj_sql_exec( sql, zdj_setting_db );

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_setting_set_double( int id, double val ) {
    zdj_setting_t * _setting = zdj_setting_get( id );
    if( !_setting ) { 
        _setting = calloc( 1, sizeof( zdj_setting_t ) ); 
        _setting->id = id;
        _setting->type = ZDJ_SETTING_TYPE_DOUBLE;
    }
    _setting->d_val = val;

    char sql[ 4096 ];
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO Settings(id,type,i_val,b_val,d_val,c_val) VALUES(%d,%d,%d,%d,%f,'%s')\n"
        // Or update existing record
        "ON CONFLICT(id) DO UPDATE SET id=%d,type=%d,i_val=%d,b_val=%d,d_val=%f,c_val='%s'",

        // Insert new record
        _setting->id,
        _setting->type,
        _setting->i_val,
        _setting->b_val,
        _setting->d_val,
        _setting->c_val,

        // Update existing record
        _setting->id,
        _setting->type,
        _setting->i_val,
        _setting->b_val,
        _setting->d_val,
        _setting->c_val
    );
    zdj_sql_exec( sql, zdj_setting_db );

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_setting_set_char( int id, char * val ) {
    zdj_setting_t * _setting = zdj_setting_get( id );
    if( !_setting ) { 
        _setting = calloc( 1, sizeof( zdj_setting_t ) ); 
        _setting->id = id;
        _setting->type = ZDJ_SETTING_TYPE_CHAR;
    }
    strcpy( _setting->c_val, val );

    char sql[ 4096 ];
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO Settings(id,type,i_val,b_val,d_val,c_val) VALUES(%d,%d,%d,%d,%f,'%s')\n"
        // Or update existing record
        "ON CONFLICT(id) DO UPDATE SET id=%d,type=%d,i_val=%d,b_val=%d,d_val=%f,c_val='%s'",

        // Insert new record
        _setting->id,
        _setting->type,
        _setting->i_val,
        _setting->b_val,
        _setting->d_val,
        _setting->c_val,

        // Update existing record
        _setting->id,
        _setting->type,
        _setting->i_val,
        _setting->b_val,
        _setting->d_val,
        _setting->c_val
    );
    zdj_sql_exec( sql, zdj_setting_db );

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_setting_put_char( int id, char * str ) {

}

int zdj_setting_increment_int( int id ) {
    zdj_setting_t * setting = zdj_setting_get( id );
    int val = setting->i_val;
    val++;
    zdj_setting_set_int( id, val );
    free( setting );
    return val;
}

int zdj_setting_decrement_int( int id) {
    zdj_setting_t * setting = zdj_setting_get( id );
    int val = setting->i_val;
    val--;
    zdj_setting_set_int( id, val );
    free( setting );
    return val;
}

bool zdj_setting_flip_bool( int id ) {
    bool val = false;
    zdj_setting_t * setting = zdj_setting_get( id );
    if( setting ) {
        val = !setting->b_val;
        zdj_setting_set_bool( id, val );
        free( setting );
    }
    return val;
}

bool zdj_setting_get_dev_zerod_flag( void ) {
    if( access( ZDJ_SETTINGS_DEV_ZEROD_FLAG_PATH, F_OK ) == 0 ) { 
        return true; 
    } else {
        return false;
    }
}

void zdj_setting_set_dev_zerod_flag( bool flag ) {
    if( flag ) {
        FILE * dev_flag_fd = fopen( ZDJ_SETTINGS_DEV_ZEROD_FLAG_PATH, "w" );
        fwrite( "true", sizeof( char ), 5, dev_flag_fd );
        fclose( dev_flag_fd );
    } else {
        remove( ZDJ_SETTINGS_DEV_ZEROD_FLAG_PATH );
    }
}