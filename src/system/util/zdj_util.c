#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#include <zerodj/system/util/zdj_util.h>

void zdj_util_put_hmsm_str_for_sample( int sample, int sample_rate, char * str ) {
    // TODO: Add Hours
    int mins = sample / sample_rate / 60;
    int secs = (sample / sample_rate) - (mins * 60);
    double secf = ((double)sample / (double)sample_rate);
    int msec = (int)((secf - secs) * 100.0);

    snprintf( str, -1, "%d:%02d.%02d", 
        mins,
        secs,
        msec
    );
}

void zdj_util_put_msm_str_for_sample( int sample, int sample_rate, char * str ) {
    int mins = sample / sample_rate / 60;
    int secs = (sample / sample_rate) - (mins * 60);
    double secf = ((double)sample / (double)sample_rate);
    int msec = (int)((secf - secs) * 100.0);

    snprintf( str, -1, "%d:%02d.%02d", 
        mins,
        secs,
        msec
    );
}

void zdj_util_str_to_lowercase( char *str ) {
    for ( int i = 0; str[ i ]; i++ ) {
        str[ i ] = tolower( ( unsigned char )str[ i ] );
    }
}

void zdj_util_put_urldecode( char * dest, int dest_len, char * src ) {
    memset( dest, '\0', dest_len );
    int out_ind = 0;
    for( int i=0; i<strlen( src ); i++ ) {
        if( src[ i ] == '%' && strlen( src ) > i+2 && 
            src[ i+1 ] == '2' && src[ i+2 ] == '0' 
        ) {
            // dest[ out_ind ] = '\\';
            // out_ind++;
            dest[ out_ind ] = ' ';
            i+=2;
        } else if( src[ i ] == '%' && strlen( src ) > i+2 && 
                   src[ i+1 ] == '2' && src[ i+2 ] == '7' 
        ) {
            dest[ out_ind ] = '\'';
            i+=2;
        } else if( src[ i ] == '%' && strlen( src ) > i+2 && 
                   src[ i+1 ] == '2' && src[ i+2 ] == '6' 
        ) {
            dest[ out_ind ] = '&';
            i+=2;
        } else if( src[ i ] == '%' && strlen( src ) > i+2 && 
                   src[ i+1 ] == '5' && src[ i+2 ] == 'b' 
        ) {
            dest[ out_ind ] = '[';
            i+=2;
        } else if( src[ i ] == '%' && strlen( src ) > i+2 && 
                   src[ i+1 ] == '5' && src[ i+2 ] == 'd' 
        ) {
            dest[ out_ind ] = ']';
            i+=2;
        } else if( src[ i ] == '&' && strlen( src ) > i+4 && 
                   src[ i+1 ] == 'a' && src[ i+2 ] == 'm' && src[ i+3 ] == 'p' && src[ i+4 ] == ';' 
        ) {
            dest[ out_ind ] = '&';
            i+=4;
        } else if( src[ i ] == '&' && strlen( src ) > i+5 && 
                   src[ i+1 ] == 'a' && src[ i+2 ] == 'p' && src[ i+3 ] == 'o' && src[ i+4 ] == 's' && src[ i+5 ] == ';'
        ) {
            dest[ out_ind ] = '\'';
            i+=5;
        } else if( src[ i ] == '&' && strlen( src ) > i+5 && 
                   src[ i+1 ] == 'q' && src[ i+2 ] == 'u' && src[ i+3 ] == 'o' && src[ i+4 ] == 't' && src[ i+5 ] == ';'
        ) {
            dest[ out_ind ] = '\"';
            i+=5;
        } else if( src[ i ] == '&' && strlen( src ) > i+3 && 
                   src[ i+1 ] == 'l' && src[ i+2 ] == 't' && src[ i+3 ] == ';'
        ) {
            dest[ out_ind ] = '<';
            i+=3;
        } else if( src[ i ] == '&' && strlen( src ) > i+3 && 
                   src[ i+1 ] == 'g' && src[ i+2 ] == 't' && src[ i+3 ] == ';'
        ) {
            dest[ out_ind ] = '>';
            i+=3;
        } else {
            dest[ out_ind ] = src[ i ];
        }
        out_ind++;
    }
}

void zdj_util_put_filepath_escape( char * dest, int dest_len, char * src ) {
    memset( dest, '\0', dest_len );
    int out_ind = 0;
    for( int i=0; i<strlen( src ); i++ ) {
        if( src[ i ] == ' ' ) {
            dest[ out_ind ] = '\\';
            out_ind++;
            dest[ out_ind ] = ' ';
        } else {
            dest[ out_ind ] = src[ i ];
        }
        out_ind++;
    }
}

// int out_buf_len = strlen( buf ) + 64;
//     char * out_buf = (char *) malloc( sizeof( char ) * out_buf_len );
//     memset( out_buf, '\0', sizeof( char ) * out_buf_len );

//     int out_ind = 0;
//     for( int i=0; i<strlen( buf ); i++ ) {
//         if( buf[ i ] == '\'' ) {
//             out_buf[ out_ind ] = '\'';
//             out_ind++;
//             out_buf[ out_ind ] = '\'';
//         } else if( buf[ i ] == '\"' ) {
//             out_buf[ out_ind ] = '\"';
//             out_ind++;
//             out_buf[ out_ind ] = '\"';
//         } else {
//             out_buf[ out_ind ] = buf[ i ];
//         }
//         out_ind++;
//     }