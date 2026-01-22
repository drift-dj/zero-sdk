#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/beatgrid_view/zdj_beatgrid_view.h>

zdj_beatgrid_mark_t zdj_beatgrid_mark_for_count( double beatgrid_count ) {
    // If count is near 0, origin
    if( fabs( beatgrid_count ) < zdj_eps ) { 
        // printf( "origin\n" );
        return ZDJ_BEATGRID_MARK_ORIGIN;

    } else if( fabs( floor( beatgrid_count ) - beatgrid_count ) < zdj_eps ) {
        // If count is near whole bar near 16-bar boundary, bar n
        if( (int)floor( beatgrid_count ) % 8 == 0 ) { 
            // printf( "bar n\n" );
            return ZDJ_BEATGRID_MARK_BAR_N; 
        } 
        
        // If count is near whole bar, whole
        else { 
            // printf( "whole\n" );
            return ZDJ_BEATGRID_MARK_WHOLE; 
        }
    
    } else {
        // Quarter
        if( fabs( beatgrid_count - floor( beatgrid_count ) - 0.25 ) < zdj_eps ||
            fabs( beatgrid_count - floor( beatgrid_count ) - 0.5 ) < zdj_eps ||
            fabs( beatgrid_count - floor( beatgrid_count ) - 0.75 ) < zdj_eps 
        ) {
            return ZDJ_BEATGRID_MARK_QUARTER;

        // 8th
        } else if( fabs( beatgrid_count - floor( beatgrid_count ) - 0.125 ) < zdj_eps ||
                   fabs( beatgrid_count - floor( beatgrid_count ) - 0.375 ) < zdj_eps ||
                   fabs( beatgrid_count - floor( beatgrid_count ) - 0.625 ) < zdj_eps ||
                   fabs( beatgrid_count - floor( beatgrid_count ) - 0.875 ) < zdj_eps 
        ) {
            return ZDJ_BEATGRID_MARK_8TH;

        // 16th
        } else if( fabs( beatgrid_count - floor( beatgrid_count ) - 0.0625 ) < zdj_eps ||
                   fabs( beatgrid_count - floor( beatgrid_count ) - 0.1875 ) < zdj_eps ||
                   fabs( beatgrid_count - floor( beatgrid_count ) - 0.3125 ) < zdj_eps ||
                   fabs( beatgrid_count - floor( beatgrid_count ) - 0.4375 ) < zdj_eps ||
                   fabs( beatgrid_count - floor( beatgrid_count ) - 0.5625 ) < zdj_eps ||
                   fabs( beatgrid_count - floor( beatgrid_count ) - 0.6875 ) < zdj_eps ||
                   fabs( beatgrid_count - floor( beatgrid_count ) - 0.8125 ) < zdj_eps ||
                   fabs( beatgrid_count - floor( beatgrid_count ) - 0.9375 ) < zdj_eps 
        ) {
            return ZDJ_BEATGRID_MARK_16TH;

        // 32nd

        }
    }
}