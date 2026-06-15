#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/panel/assist/zdj_assist_panel.h>
#include <zerodj/ui/panel/browser/zdj_browser_panel.h>
#include <zerodj/ui/panel/debug/zdj_debug_panel.h>
#include <zerodj/ui/panel/recording/zdj_recording_panel.h>
#include <zerodj/ui/panel/soundcard/zdj_soundcard_panel.h>
#include <zerodj/ui/panel/settings/zdj_settings_panel.h>
#include <zerodj/ui/panel/usb/zdj_usb_panel.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
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

    state->browser_panel = zdj_new_browser_panel( );
    zdj_add_subview( zdj_panel_view( ), state->browser_panel );

    // state->debug_panel = zdj_new_debug_panel( );
    // zdj_add_subview( zdj_panel_view( ), state->debug_panel );

    state->recording_panel = zdj_new_recording_panel( );
    zdj_add_subview( zdj_panel_view( ), state->recording_panel );

    state->settings_panel = zdj_new_settings_panel( );
    zdj_add_subview( zdj_panel_view( ), state->settings_panel );

    state->usb_panel = zdj_new_usb_panel( );
    zdj_add_subview( zdj_panel_view( ), state->usb_panel );

    // Always add soundcard last so it's the top view for event handling.
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
    void * panel_state;

    // printf( "ui panel handle_control\n" );
    switch( event->id ) {
        case ZDJ_UI_CONTROL_NEXT_PANEL:
            state->panel_scroll_index += 1;
            if( state->panel_scroll_index > (double)ZDJ_UI_PANEL_COUNT-1 ) { 
                state->panel_scroll_index = (double)ZDJ_UI_PANEL_COUNT-1; 
                return;
            }

            // animate out current panel
            out_panel = state->current_panel;
            zdj_set_anim( &out_panel->out_anim, ZDJ_ANIM_PANEL_OUT_NEXT );
            ((anim_init_t)out_panel->out_anim.init_fn)( &out_panel->out_anim, out_panel );
            out_panel->anim = &out_panel->out_anim;
            // zdj_pop_subview_of( zdj_panel_view( ), true );
            // animate in new panel

            switch ( (int)round( state->panel_scroll_index ) ) {
                // case ZDJ_UI_PANEL_ASSIST: in_panel = state->assist_panel; break;
                // case ZDJ_UI_PANEL_DEBUG: in_panel = state->debug_panel; break;
                case ZDJ_UI_PANEL_RECORDING: 
                    in_panel = state->recording_panel;
                    panel_state = (zdj_recording_panel_state_t*)in_panel->state;
                    ((zdj_recording_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 );
                    break;
                case ZDJ_UI_PANEL_SOUNDCARD: 
                    in_panel = state->soundcard_panel; 
                    panel_state = (zdj_soundcard_panel_state_t*)in_panel->state;
                    ((zdj_soundcard_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 );
                    break;
                case ZDJ_UI_PANEL_BROWSER: 
                    in_panel = state->browser_panel;
                    panel_state = (zdj_browser_panel_state_t*)in_panel->state;
                    ((zdj_browser_panel_state_t*)panel_state)->needs_layout_update = true;
                    ((zdj_browser_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 ); 
                    break;
                case ZDJ_UI_PANEL_USB: 
                    in_panel = state->usb_panel; 
                    panel_state = (zdj_usb_panel_state_t*)in_panel->state;
                    ((zdj_usb_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 );
                    break;
                case ZDJ_UI_PANEL_SETTINGS: 
                    in_panel = state->settings_panel; 
                    panel_state = (zdj_settings_panel_state_t*)in_panel->state;
                    ((zdj_settings_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 );
                    break;
                default: break;
            }
            if( in_panel ) {
                state->current_panel = in_panel;
                state->current_panel_name = (int)round( state->panel_scroll_index );
                zdj_set_anim( &in_panel->in_anim, ZDJ_ANIM_PANEL_IN_NEXT );
                ((anim_init_t)in_panel->in_anim.init_fn)( &in_panel->in_anim, in_panel );
                in_panel->anim = &in_panel->in_anim;
                // zdj_push_subview( zdj_panel_view( ), in_panel, true );
                zdj_activate_control_map( in_panel->map );
            }
            break;
        case ZDJ_UI_CONTROL_PREV_PANEL:
            state->panel_scroll_index -= 1;
            if( state->panel_scroll_index < 0.0 ) { 
                state->panel_scroll_index = 0.0; 
                return;
            }

            // animate out current panel
            out_panel = state->current_panel;
            zdj_set_anim( &out_panel->out_anim, ZDJ_ANIM_PANEL_OUT_PREV );
            ((anim_init_t)out_panel->out_anim.init_fn)( &out_panel->out_anim, out_panel );
            out_panel->anim = &out_panel->out_anim;
            // zdj_pop_subview_of( zdj_panel_view( ), true );
            // animate in new panel
            switch ( (int)round( state->panel_scroll_index ) ) {
                // case ZDJ_UI_PANEL_ASSIST: in_panel = state->assist_panel; break;
                // case ZDJ_UI_PANEL_DEBUG: in_panel = state->debug_panel; break;
                case ZDJ_UI_PANEL_RECORDING: 
                    in_panel = state->recording_panel;
                    panel_state = (zdj_recording_panel_state_t*)in_panel->state;
                    ((zdj_recording_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 );
                    break;
                case ZDJ_UI_PANEL_SOUNDCARD: 
                    in_panel = state->soundcard_panel; 
                    panel_state = (zdj_soundcard_panel_state_t*)in_panel->state;
                    ((zdj_soundcard_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 );
                    break;
                case ZDJ_UI_PANEL_BROWSER: 
                    in_panel = state->browser_panel;
                    panel_state = (zdj_browser_panel_state_t*)in_panel->state;
                    ((zdj_browser_panel_state_t*)panel_state)->needs_layout_update = true;
                    ((zdj_browser_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 );
                    break;
                case ZDJ_UI_PANEL_USB: 
                    in_panel = state->usb_panel; 
                    panel_state = (zdj_usb_panel_state_t*)in_panel->state;
                    ((zdj_usb_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 );
                    break;
                case ZDJ_UI_PANEL_SETTINGS: 
                    in_panel = state->settings_panel; 
                    panel_state = (zdj_settings_panel_state_t*)in_panel->state;
                    ((zdj_settings_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 );
                    break;
                default: break;
            }
            if( in_panel ) {
                state->current_panel = in_panel;
                state->current_panel_name = (int)round( state->panel_scroll_index );
                zdj_set_anim( &in_panel->in_anim, ZDJ_ANIM_PANEL_IN_PREV );
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
                printf( "panel capture event: %d\n", event->id );
                state->current_panel->handle_control_event( state->current_panel, event );
                event->blocked = true;
            }
            break;
    }
}

void zdj_ui_panel_toggle( void ) {
    zdj_panel_state_t * state = _zdj_panel_state;
    if( state->deployed && state->current_panel ) {
        // printf( "popping panel\n" );
        // retract current panel + activate previous map
        state->deployed = false;
        zdj_activate_control_map( state->exit_map ); 
        // animate out
        zdj_set_anim( &state->current_panel->out_anim, ZDJ_ANIM_PANEL_RETRACT );
        ((anim_init_t)state->current_panel->out_anim.init_fn)( &state->current_panel->out_anim, state->current_panel );
        state->current_panel->anim = &state->current_panel->out_anim;
    } else if( !state->deployed && state->current_panel ) {
        // printf( "pushing panel\n" );
        state->exit_map = zdj_control_active_map; // Store current map so we can reactivate on exit
        // deploy current panel + activate panel map
        state->deployed = true;
        zdj_activate_control_map( state->current_panel->map ); 
        // animate in
        zdj_set_anim( &state->current_panel->in_anim, ZDJ_ANIM_PANEL_DEPLOY );
        ((anim_init_t)state->current_panel->in_anim.init_fn)( &state->current_panel->in_anim, state->current_panel );
        state->current_panel->anim = &state->current_panel->in_anim;

        void * panel_state;
        switch ( (int)round( state->panel_scroll_index ) ) {
            // case ZDJ_UI_PANEL_ASSIST: in_panel = state->assist_panel; break;
            // case ZDJ_UI_PANEL_DEBUG: in_panel = state->debug_panel; break;
            case ZDJ_UI_PANEL_RECORDING: 
                panel_state = (zdj_recording_panel_state_t*)state->current_panel->state;
                ((zdj_recording_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 );
                break;
            case ZDJ_UI_PANEL_SOUNDCARD: 
                panel_state = (zdj_soundcard_panel_state_t*)state->current_panel->state;
                ((zdj_soundcard_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 );
                break;
            case ZDJ_UI_PANEL_BROWSER: 
                panel_state = (zdj_browser_panel_state_t*)state->current_panel->state;
                ((zdj_browser_panel_state_t*)panel_state)->needs_layout_update = true;
                ((zdj_browser_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 );
                break;
            case ZDJ_UI_PANEL_USB: 
                panel_state = (zdj_usb_panel_state_t*)state->current_panel->state;
                ((zdj_usb_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 );
                break;
            case ZDJ_UI_PANEL_SETTINGS: 
                panel_state = (zdj_settings_panel_state_t*)state->current_panel->state;
                ((zdj_settings_panel_state_t*)panel_state)->overlay_counter = zdj_ui_msec_to_frames( 400 );
                break;
            default: break;
        }
    }
}

void zdj_ui_panel_toggle_assist( void ) {

}

void zdj_ui_panel_toggle_debug( void ) {

}

void zdj_ui_panel_toggle_recording( void ) {
    zdj_panel_state_t * state = _zdj_panel_state;
    if( state->deployed && state->current_panel ) {
        // printf( "popping recording\n" );
        // retract current panel + activate previous map
        state->deployed = false;
        zdj_activate_control_map( state->exit_map ); 
        // animate out
        zdj_set_anim( &state->current_panel->out_anim, ZDJ_ANIM_PANEL_RETRACT );
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
        zdj_set_anim( &state->current_panel->out_anim, ZDJ_ANIM_PANEL_DEPLOY );
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

void zdj_ui_panel_toggle_settings( void ) {

}

zdj_view_t * zdj_ui_panel_new_overlay( char * title ) {
    // Make overlay
    zdj_view_t * view = zdj_new_view( &(zdj_rect_t){ 0,7,ZDJ_MODAL_WIDTH,ZDJ_MODAL_HEIGHT } );
    view->type = ZDJ_VIEW_OVERLAY;

    zdj_view_t * overlay_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MENU_BG ], NULL );

    zdj_view_t * label = zdj_new_label_view( title, ZDJ_FONT_12_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    label->frame.x = 64.0 - (label->frame.w / 2.0);
    label->frame.y = 22;

    zdj_view_t * label_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    label_bg->frame.x = label->frame.x - 4;
    label_bg->frame.y = label->frame.y - 4;
    label_bg->frame.w = label->frame.w + 8;
    label_bg->frame.h = label->frame.h + 8;
    
    zdj_add_subview( view, overlay_bg );
    zdj_add_subview( view, label_bg );
    zdj_add_subview( view, label );

    return view;
}

void zdj_ui_panel_scroll_next( void ) {

}

void zdj_ui_panel_scroll_prev( void ) {

}