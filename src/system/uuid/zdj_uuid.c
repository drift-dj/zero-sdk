#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <uuid.h>

#include <zerodj/system/uuid/zdj_uuid.h>

void zdj_put_uuid_no_dash( char * str ) {
    uuid_t uuid;
    uuid_generate( uuid );
    char uuid_str[ ZDJ_UUID_NO_DASH_LEN ];
    char uuid_str_no_dash[ ZDJ_UUID_NO_DASH_LEN ];
    uuid_unparse_lower( uuid, uuid_str );
    int n = 0;
    for( int i=0; i<ZDJ_UUID_NO_DASH_LEN; i++ ) {
        if( uuid_str[ i ] != '-' ) {
            uuid_str_no_dash[ n ] = uuid_str[ i ];
            n++;
        }
    }
    strcpy( str, uuid_str_no_dash );
}