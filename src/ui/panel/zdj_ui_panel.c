#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/panel/assist/zdj_assist_panel.h>
#include <zerodj/ui/panel/debug/zdj_debug_panel.h>
#include <zerodj/ui/panel/recording/zdj_recording_panel.h>
#include <zerodj/ui/panel/soundcard/zdj_soundcard_panel.h>
#include <zerodj/ui/panel/widget_controls/zdj_widget_controls_panel.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static zdj_panel_state_t * _zdj_panel_state;

static void _handle_control( zdj_view_t * view, zdj_control_event_t * event );

zdj_error_type_t zdj_ui_panel_init( void ) {
    // Init ui panel state
    zdj_panel_state_t * state = calloc( 1, sizeof( zdj_panel_state_t ) );
    state->deployed = false;
    _zdj_panel_state = state;
    zdj_panel_view( )->state = state;
    zdj_panel_view( )->handle_control_event = &_handle_control;

    state->assist_panel = zdj_new_assist_panel( );
    zdj_add_subview( zdj_panel_view( ), state->assist_panel );

    // state->debug_panel = zdj_new_debug_panel( );
    // zdj_add_subview( zdj_panel_view( ), state->debug_panel );

    state->recording_panel = zdj_new_recording_panel( );
    zdj_add_subview( zdj_panel_view( ), state->recording_panel );

    state->widget_panel = zdj_new_widget_controls_panel( );
    zdj_add_subview( zdj_panel_view( ), state->widget_panel );

    state->soundcard_panel = zdj_new_soundcard_view( );
    zdj_add_subview( zdj_panel_view( ), state->soundcard_panel );

    state->current_panel = state->soundcard_panel;
    state->current_panel_name = ZDJ_UI_PANEL_SOUNDCARD;
    state->panel_scroll_index = (double)ZDJ_UI_PANEL_SOUNDCARD;

    return ZDJ_ERROR_OKAY;
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * event ) {
    // If a panel is deployed, send the event down into the panel
    zdj_panel_state_t * state = (zdj_panel_state_t*)view->state;
    zdj_view_t * in_panel;
    zdj_view_t * out_panel;

    // printf( "ui panel handle_control\n" );
    switch( event->id ) {
        case ZDJ_UI_CONTROL_SCROLL_PANEL:
            if( state->deployed && state->current_panel ) {
                state->panel_scroll_index += (float)event->i_val * 0.11;
                if( state->panel_scroll_index < 0.0 ) { state->panel_scroll_index = 0.0; }
                if( state->panel_scroll_index > (double)ZDJ_UI_PANEL_COUNT-1 ) { state->panel_scroll_index = (double)ZDJ_UI_PANEL_COUNT-1; }

                if( round( state->panel_scroll_index ) != state->current_panel_name ) {
                    // animate out current panel
                    out_panel = state->current_panel;
                    ((anim_init_t)out_panel->out_anim.init_fn)( &out_panel->out_anim, out_panel );
                    out_panel->anim = &out_panel->out_anim;
                    // zdj_pop_subview_of( zdj_panel_view( ), true );
                    // animate in new panel
                    switch ( (int)round( state->panel_scroll_index ) ) {
                        case ZDJ_UI_PANEL_ASSIST: in_panel = state->assist_panel; break;
                        // case ZDJ_UI_PANEL_DEBUG: in_panel = state->debug_panel; break;
                        case ZDJ_UI_PANEL_RECORDING: in_panel = state->recording_panel; break;
                        case ZDJ_UI_PANEL_SOUNDCARD: in_panel = state->soundcard_panel; break;
                        case ZDJ_UI_PANEL_WIDGET: in_panel = state->widget_panel; break;
                        default: break;
                    }
                    if( in_panel ) {
                        state->current_panel = in_panel;
                        state->current_panel_name = (int)round( state->panel_scroll_index );
                        ((anim_init_t)in_panel->in_anim.init_fn)( &in_panel->in_anim, in_panel );
                        in_panel->anim = &in_panel->in_anim;
                        // zdj_push_subview( zdj_panel_view( ), in_panel, true );
                        zdj_activate_control_map( in_panel->map );
                    }
                }
            }
            break;
        case ZDJ_UI_CONTROL_NEXT_PANEL:
            state->panel_scroll_index += 1;
            if( state->panel_scroll_index > (double)ZDJ_UI_PANEL_COUNT-1 ) { state->panel_scroll_index = (double)ZDJ_UI_PANEL_COUNT-1; }

            // animate out current panel
            out_panel = state->current_panel;
            ((anim_init_t)out_panel->out_anim.init_fn)( &out_panel->out_anim, out_panel );
            out_panel->anim = &out_panel->out_anim;
            // zdj_pop_subview_of( zdj_panel_view( ), true );
            // animate in new panel
            switch ( (int)round( state->panel_scroll_index ) ) {
                case ZDJ_UI_PANEL_ASSIST: in_panel = state->assist_panel; break;
                // case ZDJ_UI_PANEL_DEBUG: in_panel = state->debug_panel; break;
                case ZDJ_UI_PANEL_RECORDING: in_panel = state->recording_panel; break;
                case ZDJ_UI_PANEL_SOUNDCARD: in_panel = state->soundcard_panel; break;
                case ZDJ_UI_PANEL_WIDGET: in_panel = state->widget_panel; break;
                default: break;
            }
            if( in_panel ) {
                state->current_panel = in_panel;
                state->current_panel_name = (int)round( state->panel_scroll_index );
                ((anim_init_t)in_panel->in_anim.init_fn)( &in_panel->in_anim, in_panel );
                in_panel->anim = &in_panel->in_anim;
                // zdj_push_subview( zdj_panel_view( ), in_panel, true );
                zdj_activate_control_map( in_panel->map );
            }
            break;
        case ZDJ_UI_CONTROL_PREV_PANEL:
            state->panel_scroll_index -= 1;
            if( state->panel_scroll_index < 0.0 ) { state->panel_scroll_index = 0.0; }

            // animate out current panel
            out_panel = state->current_panel;
            ((anim_init_t)out_panel->out_anim.init_fn)( &out_panel->out_anim, out_panel );
            out_panel->anim = &out_panel->out_anim;
            // zdj_pop_subview_of( zdj_panel_view( ), true );
            // animate in new panel
            switch ( (int)round( state->panel_scroll_index ) ) {
                case ZDJ_UI_PANEL_ASSIST: in_panel = state->assist_panel; break;
                // case ZDJ_UI_PANEL_DEBUG: in_panel = state->debug_panel; break;
                case ZDJ_UI_PANEL_RECORDING: in_panel = state->recording_panel; break;
                case ZDJ_UI_PANEL_SOUNDCARD: in_panel = state->soundcard_panel; break;
                case ZDJ_UI_PANEL_WIDGET: in_panel = state->widget_panel; break;
                default: break;
            }
            if( in_panel ) {
                state->current_panel = in_panel;
                state->current_panel_name = (int)round( state->panel_scroll_index );
                ((anim_init_t)in_panel->in_anim.init_fn)( &in_panel->in_anim, in_panel );
                in_panel->anim = &in_panel->in_anim;
                // zdj_push_subview( zdj_panel_view( ), in_panel, true );
                zdj_activate_control_map( in_panel->map );
            }
            break;
        case ZDJ_UI_CONTROL_TOGGLE_ASSIST_PANEL:
        case ZDJ_UI_CONTROL_TOGGLE_RECORDING_PANEL:
        case ZDJ_UI_CONTROL_TOGGLE_SOUNDCARD_PANEL:
        case ZDJ_UI_CONTROL_TOGGLE_PANEL:
            // printf( "toggle panel control\n" );
            zdj_ui_panel_toggle( );
            event->blocked = true;
            break;
        default:
            if( state->deployed && state->current_panel_name == ZDJ_UI_PANEL_SOUNDCARD ) {
                // printf( "soundcard panel capture event\n" );
                zdj_view_t * top_subview = zdj_view_stack_top_subview_of( view );
                if( top_subview && top_subview->handle_control_event ) {
                    top_subview->handle_control_event( top_subview, event );
                    event->blocked = true;
                }
                // state->soundcard_panel->handle_control_event( state->soundcard_panel, event );
            } else if( state->deployed && state->current_panel ) {
                // printf( "panel capture event\n" );
                state->current_panel->handle_control_event( state->current_panel, event );
                event->blocked = true;
            }
            break;
    }
}

void zdj_ui_panel_toggle( void ) {
    zdj_panel_state_t * state = _zdj_panel_state;
    if( state->deployed && state->current_panel ) {
        // printf( "popping\n" );
        // retract current panel + activate previous map
        state->deployed = false;
        zdj_activate_control_map( state->exit_map ); 
        // animate out
        ((anim_init_t)state->current_panel->out_anim.init_fn)( &state->current_panel->out_anim, state->current_panel );
        state->current_panel->anim = &state->current_panel->out_anim;
    } else if( !state->deployed && state->current_panel ) {
        // printf( "pushing\n" );
        state->exit_map = zdj_control_active_map; // Store current map so we can reactivate on exit
        // deploy current panel + activate panel map
        state->deployed = true;
        zdj_activate_control_map( state->current_panel->map ); 
        // animate in
        ((anim_init_t)state->current_panel->in_anim.init_fn)( &state->current_panel->in_anim, state->current_panel );
        state->current_panel->anim = &state->current_panel->in_anim;
    }
}

void zdj_ui_panel_toggle_assist( void ) {

}

void zdj_ui_panel_toggle_debug( void ) {

}

void zdj_ui_panel_toggle_recording( void ) {
    zdj_panel_state_t * state = _zdj_panel_state;
    if( state->deployed && state->current_panel ) {
        // printf( "popping\n" );
        // retract current panel + activate previous map
        state->deployed = false;
        zdj_activate_control_map( state->exit_map ); 
        // animate out
        ((anim_init_t)state->current_panel->out_anim.init_fn)( &state->current_panel->out_anim, state->current_panel );
        state->current_panel->anim = &state->current_panel->out_anim;
    } else if( !state->deployed && state->current_panel ) {
        // printf( "pushing\n" );
        state->exit_map = zdj_control_active_map; // Store current map so we can reactivate on exit
        // deploy current panel + activate panel map
        state->deployed = true;
        state->current_panel = state->recording_panel;
        zdj_activate_control_map( state->current_panel->map ); 
        // animate in
        ((anim_init_t)state->current_panel->in_anim.init_fn)( &state->current_panel->in_anim, state->current_panel );
        state->current_panel->anim = &state->current_panel->in_anim;
    }
}

void zdj_ui_panel_toggle_soundcard( void ) {
    zdj_panel_state_t * state = _zdj_panel_state;
    if( state->deployed && state->current_panel ) {
        // printf( "popping\n" );
        // retract current panel + activate previous map
        state->deployed = false;
        zdj_activate_control_map( state->exit_map ); 
        // animate out
        ((anim_init_t)state->current_panel->out_anim.init_fn)( &state->current_panel->out_anim, state->current_panel );
        state->current_panel->anim = &state->current_panel->out_anim;
    } else if( !state->deployed && state->current_panel ) {
        // printf( "pushing\n" );
        state->exit_map = zdj_control_active_map; // Store current map so we can reactivate on exit
        // deploy current panel + activate panel map
        state->deployed = true;
        state->current_panel = state->soundcard_panel;
        zdj_activate_control_map( state->current_panel->map ); 
        // animate in
        ((anim_init_t)state->current_panel->in_anim.init_fn)( &state->current_panel->in_anim, state->current_panel );
        state->current_panel->anim = &state->current_panel->in_anim;
    }
}

void zdj_ui_panel_toggle_widget( void ) {

}

void zdj_ui_panel_scroll_next( void ) {

}

void zdj_ui_panel_scroll_prev( void ) {

}