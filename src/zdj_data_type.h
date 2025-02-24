#ifndef ZDJ_DATA_TYPE_H
#define ZDJ_DATA_TYPE_H

#include <stdbool.h>

typedef struct {
    char * tag;
    char * c_val;
    int i_val;
    float f_val;
    bool b_val;
    void * ptr;
} zdj_data_t;

typedef struct {
    int major;
    int minor;
    int hotfix;
    int build;
    char desc[ 64 ];
} zdj_version_t;

#endif