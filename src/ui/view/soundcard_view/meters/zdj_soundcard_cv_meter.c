#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>


#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/node/analysis/meter/zdj_meter_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/soundcard_view/zdj_soundcard_view.h>
#include <zerodj/ui/view/soundcard_view/meters/zdj_soundcard_meter.h>
#include <zerodj/ui/view/soundcard_view/options/zdj_soundcard_options.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _zdj_cv_meter_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_cv_meter_handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _zdj_cv_meter_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_cv_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
) {
    printf( "zdj_new_cv_meter_view: %s\n", zdj_soundcard_node_name[ node->name ] );
    zdj_view_t * cv_meter_view = zdj_new_view( &(zdj_rect_t){0,0,12,40} );
    cv_meter_view->type = ZDJ_VIEW_MENU_ITEM;
    cv_meter_view->draw = &_zdj_cv_meter_draw;
    cv_meter_view->handle_control_event = &_zdj_cv_meter_handle_control;
    cv_meter_view->deinit_state = &_zdj_cv_meter_deinit_state;

    // Add a state instance
    zdj_soundcard_meter_state_t * state = calloc( 1, sizeof( zdj_soundcard_meter_state_t ) );
    cv_meter_view->state = state;

    // Add baseline
    zdj_view_t * baseline = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_CV_BASELINE ], NULL );
    baseline->frame->y = -1;
    state->cv_baseline = baseline;
    zdj_add_subview( cv_meter_view, baseline );

    // Add value
    zdj_view_t * value = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_CV_VALUE ], NULL );
    state->cv_value = value;
    zdj_add_subview( cv_meter_view, value );

    // Add label
    zdj_view_t * meter_label = zdj_new_asset_view( 
        &zdj_ui_assets[ zdj_meter_asset_for_label( label ) ], 
        NULL 
    );
    meter_label->frame->x = -1;
    zdj_add_subview( cv_meter_view, meter_label );

    // Add detail
    zdj_view_t * detail = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DETAIL ], NULL );
    detail->frame->x = 2;
    detail->frame->y = 43;
    detail->frame->w = 0;
    state->detail = detail;
    zdj_add_subview( cv_meter_view, detail );

    return cv_meter_view;
}

void _zdj_cv_meter_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );
    zdj_soundcard_meter_state_t * state = (zdj_soundcard_meter_state_t*)view->state;
    zdj_soundcard_node_config_context_t * config = state->config_context;

    zdj_pipeline_node_t * meter_pipe = config->node->meter_pipe;
    zdj_meter_node_state_t * meter_state = (zdj_meter_node_state_t*)meter_pipe->state;
    float meter_y = meter_state->instant_val_0 * 46;

    zdj_soundcard_signal_type_t signal_type;

    if( zdj_soundcard_node_name_is_analog_input( config->node->name ) ) {
        // Scan input node's output links for a CV node.
        for( int i=0; i<config->node->output_link_count; i++ ) {
            if( zdj_soundcard_node_name_is_cv( config->node->output_links[ i ].dest_node ) ) {
                zdj_soundcard_node_t * dest_node = zdj_soundcard_get_node_for_name( 
                    config->soundcard,
                    config->node->output_links[ i ].dest_node
                );
                signal_type = dest_node->signal_type;
            }
        }
    } else if ( zdj_soundcard_node_name_is_analog_output( config->node->name ) ) {
         // Scan output node's input links for a CV node.
        for( int i=0; i<config->node->input_link_count; i++ ) {
            if( zdj_soundcard_node_name_is_cv( config->node->input_links[ i ].source_node ) ) {
                zdj_soundcard_node_t * source_node = zdj_soundcard_get_node_for_name( 
                    config->soundcard,
                    config->node->input_links[ i ].source_node
                );
                signal_type = source_node->signal_type;
            }
        }
    } else {
        signal_type = config->node->signal_type;
    }

    

    if( signal_type == ZDJ_SOUNDCARD_SIGNAL_CV_UNIPOLAR ) {
        state->cv_baseline->frame->y = 45;
        state->cv_value->frame->y = (meter_state->instant_val_0 * 20) + 20;
    } else if( signal_type == ZDJ_SOUNDCARD_SIGNAL_CV_BIPOLAR ) {
        state->cv_baseline->frame->y = 20;
        state->cv_value->frame->y = (meter_state->instant_val_0 * 20) + 20;
    }
    if( state->is_hilite ) {
        state->detail->frame->w = 7;
    } else {
        state->detail->frame->w = 0;
    }
}

void _zdj_cv_meter_handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_control_event_t * e = (zdj_control_event_t *)_event;
    zdj_soundcard_meter_state_t * state = (zdj_soundcard_meter_state_t*)view->state;
    zdj_soundcard_node_config_context_t * context = state->config_context;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)context->options_view_state;
    
    if(e->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 ) {
        zdj_push_subview( zdj_root_view( ), zdj_new_soundcard_options( context ), true );
        e->blocked = true;
    }

    // // Ignore events which have been blocked by layers above this one.
    // if( e->blocked ) { return; }

    // if( (e->id == ZDJ_HMI_ENCO_3_TONE_1 && e->type == ZDJ_HMI_EVENT_RELEASE) ) {
    //     // Prevent views/menus below this one from getting jog wheel events
    //     e->blocked = true;

    //     printf( "toggle mute\n" );
    //     // if( !zdj_soundcard_node_name_is_output( context->node->name ) ) {
    //         context->node->mute = !context->node->mute;
    //         if( options_state ){ options_state->needs_layout_update = true; }
    //     // }
        
    // }
}

void _zdj_cv_meter_deinit_state( zdj_view_t * view ) {
    
}
