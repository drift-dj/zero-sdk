#ifndef ZDJ_SQL_H
#define ZDJ_SQL_H

sqlite3 * zdj_sql_open( char * path );
int zdj_sql_exec( char * sql, sqlite3 * db );
int zdj_sql_rows_in_table ( char * table, char * distinct, sqlite3 * db );
sqlite3_stmt * zdj_sql_prep_row_stepper( char * sql, sqlite3 * db );

#endif