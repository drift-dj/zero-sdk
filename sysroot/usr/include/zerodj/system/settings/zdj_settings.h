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

#ifndef ZDJ_SETTINGS_H
#define ZDJ_SETTINGS_H

#include <stdint.h>
#include <stdbool.h>

#include <sqlite3.h>

#include <zerodj/system/error/zdj_error.h>

#define ZDJ_SETTINGS_DB_PATH  "/media/internal/.system/settings.db"
#define ZDJ_SETTINGS_DEV_ZEROD_FLAG_PATH "/media/internal/.system/zerod_dev"

typedef enum {
	ZDJ_SETTING_UNKNOWN,
    ZDJ_SETTING_SCREENSHOT_COUNTER,
    ZDJ_SETTING_RECORDING_COUNTER,
    ZDJ_SETTING_DISPLAY_FLIP,
    ZDJ_SETTING_DISPLAY_BRIGHTNESS,
    ZDJ_SETTING_REFRESH_RATE
} zdj_setting_reserved_id_t;

typedef enum {
    ZDJ_SETTING_TYPE_INT,
    ZDJ_SETTING_TYPE_BOOL,
    ZDJ_SETTING_TYPE_DOUBLE,
    ZDJ_SETTING_TYPE_CHAR,
} zdj_setting_type_t;

typedef struct {
	int id;
    zdj_setting_type_t type;
	int i_val;
    bool b_val;
	double d_val;
	char c_val[ 256 ];
} zdj_setting_t;

typedef enum {
    ZDJ_SETTING_REFRESH_RATE_115,
    ZDJ_SETTING_REFRESH_RATE_60,
    ZDJ_SETTING_REFRESH_RATE_30,
    ZDJ_SETTING_REFRESH_RATE_20
} zdj_setting_refresh_rate_t;

extern sqlite3 * zdj_setting_db;

zdj_error_type_t zdj_settings_init( void );
void zdj_drop_settings( void );

zdj_setting_t * zdj_setting_get( int id );
zdj_error_type_t zdj_setting_set_int( int id, int val );
zdj_error_type_t zdj_setting_set_bool( int id, bool val );
zdj_error_type_t zdj_setting_set_double( int id, double val );
zdj_error_type_t zdj_setting_set_char( int id, char * val );
zdj_error_type_t zdj_setting_put_char( int id, char * str );

int zdj_setting_increment_int( int id );
int zdj_setting_decrement_int( int id);
bool zdj_setting_flip_bool( int id );

bool zdj_setting_get_dev_zerod_flag( void );
void zdj_setting_set_dev_zerod_flag( bool flag );

#endif