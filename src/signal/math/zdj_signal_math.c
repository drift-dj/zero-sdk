#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/math/zdj_signal_math.h>

static double _gaussian_pdf( double x, double mu, double sigma ) {
    return ( 1.0 / (sigma * sqrt(2 * M_PI))) * exp(-0.5 * pow((x - mu) / sigma, 2) );
}
// #define TABLE_SIZE 1000 // Number of points in the lookup table
// #define MU 0.0          // Mean
// #define SIGMA 1.0       // Standard deviation
// #define X_MIN -4.0      // Minimum x-value for the table
// #define X_MAX 4.0       // Maximum x-value for the table

// double gaussian_lut[TABLE_SIZE];
// double x_step;

zdj_gaussian_t * zdj_new_gaussian( int kernel_width, double blur ) {
	zdj_gaussian_t * kernel = calloc( 1, sizeof( zdj_gaussian_t ) );
	kernel->lut = calloc( kernel_width, sizeof( double ) );
	kernel->width = kernel_width;
	
	double mu = 0.0;
	double sigma = blur;
	double x_min = -4.0;
	double x_max = 4.0;
	double x_step = ( x_max - x_min ) / ( kernel_width - 1 );
    for ( int i = 0; i < kernel_width; i++ ) {
        double x = x_min + i * x_step;
        kernel->lut[ i ] = _gaussian_pdf( x, mu, sigma );
    }

	// zdj_gaussian_t * kernel = calloc( 1, sizeof( zdj_gaussian_t ) );
	// double sigma = blur;
	// kernel->sigma = sigma;
	// kernel->width = ceil( sigma * 2.5 ); // 3.5 is flavor - increase if edges get weird
	// kernel->lut = calloc( kernel->width, sizeof( double ) );
	// double sum = 0.0;

	// for ( int x = 0; x < kernel->width; x++ ) {
	// 	kernel->lut[ x ] = exp( -0.5 * (pow(x/sigma, 2.0) )) / (2 * M_PI * sigma * sigma);
	// 	// Accumulate the kernel values
	// 	sum += kernel->lut[ x ] * 2;
	// }

	// // Normalize
	// sum -= kernel->lut[ 0 ];
	// for ( int x = 0; x < kernel->width; x++ ){ 
	// 	kernel->lut[ x ] /= sum;
	// 	// printf( "x: %1.3f\n", kernel->lut[ x ] ); 
	// }

	return kernel;
}

zdj_error_type_t zdj_gaussian_free( zdj_gaussian_t * kernel ) {
	free( kernel->lut );
	free( kernel );
}

double zdj_calc_fader_gain( double travel ) {
	// Max travel = +10dB = 3.162x amplitude
	// Min travel = -∞ = 0x amplitude
	// ^5 places unity @ travel=~0.8
	// ^3 places unity @ travel=~0.6
	return (travel*travel*travel*travel*travel) * 3.162;
}
double zdj_calc_fader_db( double travel ) {
	// This is super hacky at the moment.
	// Just a linear interpolation where travel=1.0 is +10dB,
	// and travel=fader_gain unity = 0dB
	return (travel - 0.7f) / 0.2f;
}

double zdj_signal_lowpass( double state, double input, double coeff ) {
	return state + ((input-state) * coeff);
}

void zdj_signal_resample_audio( 
    float * in_buf,
    double in_start_coord,
    double in_end_coord,
    int in_channel_count,
    float * out_buf,
    double out_start_coord,
    double out_end_coord,
    int out_channel_count
) {

}

float zdj_signal_gen_sine( 
	float freq, 
	float phase, 
	int sample_count, 
	float * buf, 
	int stride, 
	int offset, 
	double scale 
) {
    float p = phase;
    float inc = freq / 44100.0f;
    double v;
    for( int i=0; i<sample_count; i++ ) {
        p += inc;
        v = (SDL_sinf( p ) * scale);
        buf[ (i*stride)+offset ] = (float)v;
        // printf( "%1.2f/%1.0f\n", p, v );
    }
    return p;
}