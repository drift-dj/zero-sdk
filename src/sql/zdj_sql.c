#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sqlite3.h>

#include <zerodj/sql/zdj_sql.h>

static char _zdj_sql[4096];

sqlite3 * zdj_sql_open( char * path ) {
    sqlite3 * db;
    int rc = sqlite3_open( path, &db );
    if ( rc != SQLITE_OK ) {
        fprintf( stderr, "Cannot open data_model database: %s\n", sqlite3_errmsg( db ) );
        sqlite3_close( db );
        return NULL;
    }
    return db;
}

int zdj_sql_close( sqlite3 * db ) {
    int rc = sqlite3_close( db );
    if ( rc != SQLITE_OK ) {
        fprintf( stderr, "Cannot close data_model database: %s\n", sqlite3_errmsg( db ) );
        return 1;
    }
    return 0;
}

int zdj_sql_exec( char * sql, sqlite3 * db ) {
    char * err_msg;
    int rc = sqlite3_exec( db, sql, NULL, NULL, &err_msg);
    if ( rc != SQLITE_OK ) {
        printf( "SQL ERROR ===> rc: %d, SQL error: %s\n", rc, err_msg );
        printf( "sql: %s\n", sql );
    }
    return rc;
}

int zdj_sql_rows_in_table ( char * table, char * distinct, sqlite3 * db ) {
    // printf( "zdj_sql_rows_in_table: %s, %s\n", table, distinct );
    // Build SQL count statement
    if( distinct ) {
        snprintf( _zdj_sql, sizeof( _zdj_sql ), "select count(distinct %s) from %s", distinct, table );
    } else {
        snprintf( _zdj_sql, sizeof( _zdj_sql ), "select count(*) from %s", table );
    }
   
    // Execute row stepper
    int count = 0;
    int res;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&_zdj_sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            count = sqlite3_column_int ( c_stmt, 0 );
        }
        sqlite3_finalize( c_stmt );
    }
    return count;
}

sqlite3_stmt * zdj_sql_prep_row_stepper( char * sql, sqlite3 * db ) {
    sqlite3_stmt *stmt;
    int res = sqlite3_prepare_v2( db, strdup( sql ), -1, &stmt, NULL );
    int result_number = 0;
    if ( res != SQLITE_OK ){ 
        // printf( "prep row error: %s\n", sqlite3_errmsg( db ) ); 
        return NULL;
    }
    return stmt;
}

char * zdj_sql_stresc( char * in_buf, char * out_buf ) {
    int out_ind = 0;
    for( int i=0; i<strlen( in_buf ); i++ ) {
        if( in_buf[ i ] == '\'' ) {
            out_buf[ out_ind ] = '\'';
            out_ind++;
            out_buf[ out_ind ] = '\'';
        } else if( in_buf[ i ] == '\"' ) {
            out_buf[ out_ind ] = '\"';
            out_ind++;
            out_buf[ out_ind ] = '\"';
        } else {
            out_buf[ out_ind ] = in_buf[ i ];
        }
        out_ind++;
    }
    return out_buf;
}

char * zdj_sql_urldecode( char * in_buf, char * out_buf ) {
    int out_ind = 0;
    for( int i=0; i<strlen( in_buf ); i++ ) {
        if( in_buf[ i ] == '%' && in_buf[ i+1 ] == '2' && in_buf[ i+2 ] == '0' ) {
            out_buf[ out_ind ] = ' ';
            i+=2;
        } else if( in_buf[ i ] == '%' && in_buf[ i+1 ] == '2' && in_buf[ i+2 ] == '7' ) {
            out_buf[ out_ind ] = '\'';
            i+=2;
        } else if( in_buf[ i ] == '%' && in_buf[ i+1 ] == '2' && in_buf[ i+2 ] == '6' ) {
            out_buf[ out_ind ] = '&';
            i+=2;
        } else if( in_buf[ i ] == '%' && in_buf[ i+1 ] == '5' && in_buf[ i+2 ] == 'b' ) {
            out_buf[ out_ind ] = '[';
            i+=2;
        } else if( in_buf[ i ] == '%' && in_buf[ i+1 ] == '5' && in_buf[ i+2 ] == 'd' ) {
            out_buf[ out_ind ] = ']';
            i+=2;
        } else {
            out_buf[ out_ind ] = in_buf[ i ];
        }
        out_ind++;
    }

    return out_buf;
}

// sqlite3* conn;
// sqlite3_stmt* stmt = 0;

// int rc = sqlite3_open(db_name, &conn);
// //  Good idea to always check the return value of sqlite3 function calls. 
// //  Only done once in this example:
// if ( rc != SQLITE_OK ) { // Do something }

// rc = sqlite3_prepare_v2( conn, "SELECT id FROM myTable WHERE id = ? or id = ?", -1, &stmt, 0 );

// //  Optional, but will most likely increase performance.
// rc = sqlite3_exec( conn, "BEGIN TRANSACTION", 0, 0, 0 );    

// for ( int bindIndex = 0; bindIndex < number_of_times_you_wish_to_bind; bindIndex++ ) {
//     //  Binding integer values in this example.
//     //  Bind functions for other data-types are available - see end of post.

//     //  Bind-parameter indexing is 1-based.
//     rc = sqlite3_bind_int( stmt, 1, int_you_wish_to_bind ); // Bind first parameter.
//     rc = sqlite3_bind_int( stmt, 2, int_you_wish_to_bind ); // Bind second parameter.

//     //  Reading interger results in this example.
//     //  Read functions for other data-types are available - see end of post.
//     while ( sqlite3_step( stmt ) == SQLITE_ROW ) { // While query has result-rows.
//         //  In your example the column count will be 1.
//         for ( int colIndex = 0; colIndex < sqlite3_column_count( stmt ); colIndex++ ) { 
//             int result = sqlite3_column_int( stmt, colIndex );
//             //  Do something with the result.
//         }
//     }
//     //  Step, Clear and Reset the statement after each bind.
//     rc = sqlite3_step( stmt );
//     rc = sqlite3_clear_bindings( stmt );
//     rc = sqlite3_reset( stmt );
// }
// char *zErrMsg = 0;  //  Can perhaps display the error message if rc != SQLITE_OK.
// rc = sqlite3_exec( conn, "END TRANSACTION", 0, 0, &zErrMsg );   //  End the transaction.

// rc = sqlite3_finalize( stmt );  //  Finalize the prepared statement.









// #include <string.h>
// #include <stdio.h>
// #include "sqlite3.h"

// sqlite3* db;

// int first_row;

// int select_callback(void *p_data, int num_fields, char **p_fields, char **p_col_names) {

//   int i;

//   int* nof_records = (int*) p_data;
//   (*nof_records)++;

//   if (first_row) {
//     first_row = 0;

//     for (i=0; i < num_fields; i++) {
//       printf("%20s", p_col_names[i]);
//     }

//     printf("\n");
//     for (i=0; i< num_fields*20; i++) {
//       printf("=");
//     }
//     printf("\n");
//   }

//   for(i=0; i < num_fields; i++) {
//     if (p_fields[i]) {
//       printf("%20s", p_fields[i]);
//     }
//     else {
//       printf("%20s", " ");
//     }
//   }

//   printf("\n");
//   return 0;
// }

// void select_stmt(const char* stmt) {
//   char *errmsg;
//   int   ret;
//   int   nrecs = 0;

//   first_row = 1;

//   ret = sqlite3_exec(db, stmt, select_callback, &nrecs, &errmsg);

//   if(ret!=SQLITE_OK) {
//     printf("Error in select statement %s [%s].\n", stmt, errmsg);
//   }
//   else {
//     printf("\n   %d records returned.\n", nrecs);
//   }
// }

// void sql_stmt(const char* stmt) {
//   char *errmsg;
//   int   ret;

//   ret = sqlite3_exec(db, stmt, 0, 0, &errmsg);

//   if(ret != SQLITE_OK) {
//     printf("Error in statement: %s [%s].\n", stmt, errmsg);
//   }
// }

// int main() {
//   sqlite3_open("./bind_insert.db", &db);

//   if(db == 0) {
//     printf("\nCould not open database.");
//     return 1;
//   }

//   sql_stmt("create table foo (bar, baz)");

//   sqlite3_stmt *stmt;

//   if ( sqlite3_prepare(
//          db, 
//          "insert into foo values (?,?)",  // stmt
//         -1, // If than zero, then stmt is read up to the first nul terminator
//         &stmt,
//          0  // Pointer to unused portion of stmt
//        )
//        != SQLITE_OK) {
//     printf("\nCould not prepare statement.");
//     return 1;
//   }

//   printf("\nThe statement has %d wildcards\n", sqlite3_bind_parameter_count(stmt));

//   if (sqlite3_bind_double(
//         stmt,
//         1,  // Index of wildcard
//         4.2
//         )
//       != SQLITE_OK) {
//     printf("\nCould not bind double.\n");
//     return 1;
//   }

//   if (sqlite3_bind_int(
//         stmt,
//         2,  // Index of wildcard
//         42
//         )
//       != SQLITE_OK) {
//     printf("\nCould not bind int.\n");
//     return 1;
//   }

//   if (sqlite3_step(stmt) != SQLITE_DONE) {
//     printf("\nCould not step (execute) stmt.\n");
//     return 1;
//   }

//   sqlite3_reset(stmt);

//   if (sqlite3_bind_null(
//         stmt,
//         1  // Index of wildcard
//         )
//       != SQLITE_OK) {
//     printf("\nCould not bind double.\n");
//     return 1;
//   }

//   if (sqlite3_bind_text (
//         stmt,
//         2,  // Index of wildcard
//         "hello",
//         5,  // length of text
//         SQLITE_STATIC
//         )
//       != SQLITE_OK) {
//     printf("\nCould not bind int.\n");
//     return 1;
//   }

//   if (sqlite3_step(stmt) != SQLITE_DONE) {
//     printf("\nCould not step (execute) stmt.\n");
//     return 1;
//   }

//   printf("\n");
//   select_stmt("select * from foo");

//   sqlite3_close(db);
//   return 0;
// }