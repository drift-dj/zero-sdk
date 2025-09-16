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

#ifndef ZDJ_SIGNAL_MATH_H
#define ZDJ_SIGNAL_MATH_H

#include <zerodj/system/error/zdj_error.h>

typedef struct {
    double sigma;
    int width;
    double * lut;
} zdj_gaussian_t;

zdj_gaussian_t * zdj_new_gaussian( int kernel_width, double blur );

zdj_error_type_t zdj_gaussian_convolve( zdj_gaussian_t * kernel, float * data );

zdj_error_type_t zdj_gaussian_free( zdj_gaussian_t * g );

double zdj_calc_fader_gain( double travel );
double zdj_calc_fader_db( double travel );

double zdj_signal_lowpass( double state, double input, double coeff );

void zdj_signal_naive_resample_audio( 
    float * in_buf,
    double in_start_coord,
    double in_end_coord,
    double in_buf_ref_coord,
    int in_channel_count,
    float * out_buf,
    int64_t out_sample_count,
    int out_channel_count
);

float zdj_signal_gen_sine( 
	float freq, 
	float phase, 
	int sample_count, 
	float * buf, 
	int stride, 
	int offset, 
	double scale 
);

#endif