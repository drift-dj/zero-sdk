#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/system/sql/zdj_sql.h>

static char _sql[ 1024 ];

zdj_library_setting_t * zdj_library_get_setting( char * library_entity_id, zdj_library_setting_type_t setting ) {
    zdj_library_setting_t * res = NULL;
    sprintf( _sql, "select * from %s", ZDJ_LIBRARY_TABLE_SETTING );
    int sql_res;
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( _sql, zdj_library_db );
    if( stmt ) {
        while ( (( sql_res = sqlite3_step( stmt ) ) == SQLITE_ROW) ) {
            res = calloc( 1, sizeof( zdj_library_setting_t ) );
            strcpy( res->entity_id, (char*)sqlite3_column_text ( stmt, 0 ) );
            res->type = sqlite3_column_int ( stmt, 2 );
            res->i_val = sqlite3_column_int ( stmt, 3 );
            res->b_val = sqlite3_column_int ( stmt, 4 );
            res->f_val = sqlite3_column_double ( stmt, 5 );
            char * c_val = (char*)sqlite3_column_text ( stmt, 6 );
            if( c_val ) { strcpy( res->c_val, c_val ); }
        }
        sqlite3_finalize( stmt );
    }
    return res;
}

zdj_health_status_t zdj_library_set_int_setting( char * library_entity_id, zdj_library_setting_type_t setting, int val ) {
    zdj_library_setting_t * _setting = zdj_library_get_setting( library_entity_id, setting );
    _setting->i_val = val;

    char sql[ 4096 ];
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO %s(entity_id,library_entity_id,type,i_val,b_val,f_val,c_val) VALUES('%s','%s',%d,%d,%d,%f,'%s')\n"
        // Or update existing record
        "ON CONFLICT(entity_id) DO UPDATE SET entity_id='%s',library_entity_id='%s',type=%d,i_val=%d,b_val=%d,f_val=%f,c_val='%s'",

        // Table name
        ZDJ_LIBRARY_TABLE_SETTING,

        // Insert new record
        _setting->entity_id,
        library_entity_id,
        _setting->type,
        _setting->i_val,
        _setting->b_val,
        _setting->f_val,
        _setting->c_val,

        // Update existing record
        _setting->entity_id,
        library_entity_id,
        _setting->type,
        _setting->i_val,
        _setting->b_val,
        _setting->f_val,
        _setting->c_val
    );
    zdj_sql_exec( sql, zdj_library_db );
}

zdj_health_status_t zdj_library_set_bool_setting( char * library_entity_id, zdj_library_setting_type_t setting, bool val ) {
    if( zdj_library_get_setting( library_entity_id, setting ) ) {
        // Update if setting exists
        snprintf( _sql, sizeof( _sql ), 
            "UPDATE Setting_Entity SET b_val=%d WHERE library_entity_id=\'%s\' AND type=%d;\n",
            val,
            library_entity_id,
            setting
        );
    } else {
        // Insert if setting does't exist
        char uuid[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
        zdj_library_put_uuid( uuid );
        snprintf( _sql, sizeof( _sql ), 
            "INSERT INTO Setting_Entity VALUES(\"%s\", \"%s\", %d, 0, %d, 0, \"\");\n",
            uuid,
            library_entity_id,
            setting,
            val
        );
    }
    int sql_res = zdj_sql_exec( (char *)&_sql, zdj_library_db );
    if( sql_res == SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_OKAY;
    } else {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
}

zdj_health_status_t zdj_library_set_float_setting( char * library_entity_id, zdj_library_setting_type_t setting, float val ) {
    if( zdj_library_get_setting( library_entity_id, setting ) ) {
        // Update if setting exists
        snprintf( _sql, sizeof( _sql ), 
            "UPDATE Setting_Entity SET f_val=%f WHERE library_entity_id=\'%s\' AND type=%d;\n",
            val,
            library_entity_id,
            setting
        );
    } else {
        // Insert if setting does't exist
        char uuid[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
        zdj_library_put_uuid( uuid );
        snprintf( _sql, sizeof( _sql ), 
            "INSERT INTO Setting_Entity VALUES(\"%s\", \"%s\", %d, 0, 0, %f, \"\");\n",
            uuid,
            library_entity_id,
            setting,
            val
        );
    }
    int sql_res = zdj_sql_exec( (char *)&_sql, zdj_library_db );
    if( sql_res == SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_OKAY;
    } else {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
}

zdj_health_status_t zdj_library_set_char_setting( char * library_entity_id, zdj_library_setting_type_t setting, char * val ) {
    if( zdj_library_get_setting( library_entity_id, setting ) ) {
        // Update if setting exists
        snprintf( _sql, sizeof( _sql ), 
            "UPDATE Setting_Entity SET c_val=%s WHERE library_entity_id=\'%s\' AND type=%d;\n",
            val,
            library_entity_id,
            setting
        );
    } else {
        // Insert if setting does't exist
        char uuid[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
        zdj_library_put_uuid( uuid );
        snprintf( _sql, sizeof( _sql ), 
            "INSERT INTO Setting_Entity VALUES(\"%s\", \"%s\", %d, 0, 0, 0, \"%s\");\n",
            uuid,
            library_entity_id,
            setting,
            val
        );
    }
    int sql_res = zdj_sql_exec( (char *)&_sql, zdj_library_db );
    if( sql_res == SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_OKAY;
    } else {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
}

void zdj_library_deinit_setting( zdj_library_setting_t * setting ) {
    // free( setting->c_val );
}