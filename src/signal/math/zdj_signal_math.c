#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/math/zdj_signal_math.h>

// Epsilon val for float equality comparisons.
double zdj_eps = 0.00001;

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

// double zdj_calc_fader_gain( double travel ) {
// 	// Max travel = +10dB = 3.162x amplitude
// 	// Min travel = -∞ = 0x amplitude
// 	// ^5 places unity @ travel=~0.8
// 	// ^3 places unity @ travel=~0.6
// 	return (travel*travel*travel*travel*travel) * 3.162;
// }
// double zdj_calc_fader_db( double travel ) {
// 	// This is super hacky at the moment.
// 	// Just a linear interpolation where travel=1.0 is +10dB,
// 	// and travel=fader_gain unity = 0dB
// 	return (travel - 0.7f) / 0.2f;
// }

double zdj_signal_lowpass( double state, double input, double coeff ) {
	return state + ((input-state) * coeff);
}

// Naïvely re-sample audio.
// Only interpolate between nearest samples.
// No decimation is performed for re-sample rates > 1.0.
// A new interpolation/decimation system is needed for better audio quality.
void zdj_signal_naive_resample_audio( 
    float * in_buf,
    double in_start_coord,
    double in_end_coord,
	double in_buf_ref_coord,
    int in_channel_count,
    float * out_buf,
    int64_t out_sample_count,
    int out_channel_count
) {
	double rate = (in_end_coord - in_start_coord) / (double)out_sample_count;

	
	// printf( "zdj_signal_naive_resample_audio rate: %1.3f/%ld %1.0f/%1.0f\n", rate, out_sample_count, in_start_coord, in_end_coord );

	double cur_sample = in_start_coord - in_buf_ref_coord;
	int left_neighbor_sample, left_neighbor_index;
	int right_neighbor_sample, right_neighbor_index;
	double interp_val;
	double samp;

	double hyperscrub_fade = 1.0;
	// if( rate > 20.0 ) {
	// 	hyperscrub_fade = (140.0 - rate - 20.0) / 140.0;
	// } else if ( rate < -20.0 ) { 
	// 	hyperscrub_fade = (140 - fabs(rate) - 20.0) / 140.0;
	// }
	if( fabs( rate ) > 17 ) { hyperscrub_fade = (90.0 - fabs(rate) - 17.0) / 90.0; }
	if( hyperscrub_fade < 0.0 ) { hyperscrub_fade = 0.0; }

	// printf( "%1.1f\n", rate );

	for( int i=0; i<out_sample_count; i++ ) {
		// printf( "resamp: rat: %1.3f, out i %d, src cur_sam: %1.3f\n", rate, i, cur_sample );
		// Gather neighboring samples for interpolation
		left_neighbor_sample = floor( cur_sample );
		right_neighbor_sample = ceil( cur_sample );
		interp_val = cur_sample - (double)left_neighbor_sample;
		
		// printf( "1\n" );

		// Interpolate nearest neighbor
		left_neighbor_index = left_neighbor_sample * in_channel_count;
		right_neighbor_index = right_neighbor_sample * in_channel_count;

		// printf( "1.1: %1.1f %d %d %1.3f\n", rate, left_neighbor_index, right_neighbor_index, interp_val );
		
		samp = in_buf[ left_neighbor_index ] * (1.0 - interp_val);
		samp += in_buf[ right_neighbor_index ] * interp_val;
		// printf( "1.2 %d\n", i*out_channel_count );
		out_buf[ i*out_channel_count ] = samp * hyperscrub_fade;

		// printf( "1.3\n" );
		if( out_channel_count == 2 ) {
			// printf( "2\n" );
			if( in_channel_count == 1 ) {
				left_neighbor_index = left_neighbor_sample;
				right_neighbor_index = right_neighbor_sample;
			} else if( in_channel_count == 2 ) {
				left_neighbor_index = left_neighbor_sample * in_channel_count + 1;
				right_neighbor_index = right_neighbor_sample * in_channel_count + 1;
			}
			samp = in_buf[ left_neighbor_index ] * (1.0 - interp_val);
			samp += in_buf[ right_neighbor_index ] * interp_val;
			out_buf[ i*out_channel_count+1 ] = samp * hyperscrub_fade;
			// printf( "3\n" );
		}

		// Increment the source sample coord by playback rate
		cur_sample += rate;
	}
	// printf( "zdj_signal_naive_resample_audio done\n" );
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

// float zdj_signal_accum_floats( float val_1, float val_2 ) {
// 	val_1 += val_2;
// 	if( val_1 > 1.0f ) { val_1 = 1.0f; }
// 	if( val_1 < -1.0f ) { val_1 = -1.0f; }
// 	return val_1;
// }
float zdj_signal_accum_floats( float val_1, float val_2 ) {
	val_1 += val_2;
	if( val_1 > 3.0f ) { val_1 = 3.0f; }
	if( val_1 < -3.0f ) { val_1 = -3.0f; }
	return val_1;
}

// Beatgrid count is specified in bars.  So a quarter note would be 0.250 in beatgrid-space.
double zdj_signal_pcm_count_for_beatgrid_count( double beatgrid_count, double bpm, int sample_rate ) {
	double bars_per_minute = bpm / 4.0;
	double samples_per_minute = sample_rate * 60.0;

	double samples_per_bar = samples_per_minute / bars_per_minute;
	return samples_per_bar * beatgrid_count;
}

double zdj_signal_beatgrid_count_for_pcm_count( double pcm_count, int sample_rate, double bpm ) {
	double bars_per_minute = bpm / 4.0;
	double samples_per_minute = sample_rate * 60.0;

	double samples_per_bar = samples_per_minute / bars_per_minute;
	return pcm_count / samples_per_bar;
}

float zdj_signal_db_for_gain( float gain ) {
	return log10( gain ) * 20;
}

float zdj_signal_lo_xover_hz_for_unit_val( float unit_val ) {
	return ( unit_val * 1000 ) + 80;
}

float zdj_signal_hi_xover_hz_for_unit_val( float unit_val ) {
	return ( unit_val * 10000 ) + 1500;
}