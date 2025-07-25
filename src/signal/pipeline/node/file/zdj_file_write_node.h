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

#ifndef ZDJ_FILE_WRITE_NODE_H
#define ZDJ_FILE_WRITE_NODE_H

#include <unistd.h>

#include <zerodj/system/error/zdj_error.h>

typedef struct {
    char * filepath;
    FILE * fd;
    void * header;
    size_t header_size;
    uint32_t bytes_written;
} zdj_file_write_node_state_t;

zdj_pipeline_node_t * zdj_new_file_write_node( 
    char * filepath,
    void * header,
    size_t header_size 
);
zdj_error_type_t zdj_file_write_node_write( 
    zdj_pipeline_node_t * node, 
    void * addr, 
    size_t len 
);

#endif