#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <zerodj/signal/math/zdj_signal_math.h>

zdj_gaussian_t * zdj_new_gaussian( int kernel_width, double blur ) {
	// zdj_gaussian_t * kernel = calloc( 1, sizeof( zdj_gaussian_t ) );
	// kernel->sigma = sigma;
	// kernel->width = ceil( sigma * 4.5 ); // 3.5 is flavor - increase if edges get weird
	// kernel->lut = calloc( kernel->width, sizeof( double ) );
	// double sum = 0.0;

	// for ( int x = 0; x < kernel->width; x++ ) {
	// 	kernel->lut[ x ] = exp( -0.5 * (pow(x/sigma, 2.0) )) / (2 * M_PI * sigma * sigma);
	// 	// Accumulate the kernel values
	// 	sum += kernel->lut[ x ] * 2;
	// }

	// // Normalize
	// sum -= kernel->lut[ 0 ];
	// for ( int x = 0; x < kernel->width; x++ ){ kernel->lut[ x ] /= sum; }
	// return kernel;
}

zdj_error_type_t zdj_gaussian_free( zdj_gaussian_t * kernel ) {
	free( kernel->lut );
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