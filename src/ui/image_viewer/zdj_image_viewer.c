#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/image_viewer/zdj_image_viewer.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/file_browser_view/zdj_file_browser_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _refresh( zdj_view_t * view );

static void _handle_filename_match( char * path, void * data );


zdj_view_t * zdj_new_image_viewer( char * path, zdj_image_viewer_type_t type ) {
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_draw;
    view->handle_control_event = &_handle_control;
    view->map = ZDJ_CONTROL_MAP_IMAGE_VIEWER;

    zdj_image_viewer_state_t * state = calloc( 1, sizeof( zdj_image_viewer_state_t ) );
    state->image_index = 0;
    state->image_count = 0;
    state->view_needs_refresh = true;
    view->state = state;

    if( type == ZDJ_IMAGE_VIEWER_TYPE_FILE ) {
        strcpy( state->images[ 0 ], path );
        state->image_count = 1;
    } else if( type == ZDJ_IMAGE_VIEWER_TYPE_DIR ) {
        printf( "loading image dir: %s\n", path );

        // Iterate over files in dir
        // Put all .bmp filenames found into images array
        zdj_fs_scan_pattern_t pattern;
        strcpy( pattern.substr, ".bmp" );
        zdj_fs_scan_dir( path, false, &pattern, &_handle_filename_match, state );
        strcpy( pattern.substr, ".BMP" );
        zdj_fs_scan_dir( path, false, &pattern, &_handle_filename_match, state );
    }
    
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_image_viewer_state_t * state = (zdj_image_viewer_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( state->view_needs_refresh ) { _refresh( view ); }
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    // printf( "settings _handle_control\n" );
    // Ignore events which have been blocked by layers above this one.
    if( _event->blocked ) { return; }

    zdj_image_viewer_state_t * state = (zdj_image_viewer_state_t*)view->state;

    // grab jog adjust and deck button events to update the image index
    if( _event->id == ZDJ_UI_CONTROL_JOG_ADJUST_0 ) {
        state->image_index += _event->i_val;
    } else if( _event->id == ZDJ_UI_CONTROL_FN_1_RELEASE_0 ) {
        state->image_index -= 1;
    } else if( _event->id == ZDJ_UI_CONTROL_FN_3_RELEASE_0 ) {
        state->image_index += 1;
    } else if( _event->id == ZDJ_UI_CONTROL_NAV_RELEASE_0 ) {
        zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
        zdj_pop_subview_of( panel_state->settings_panel, true );
    }

    if( state->image_index < 0 ) {
        state->image_index = state->image_count - 1;
    } else {
        state->image_index %= state->image_count;
    }

    state->view_needs_refresh = true;

    _event->blocked = true;
}

static void _refresh( zdj_view_t * view ) {
    zdj_image_viewer_state_t * state = (zdj_image_viewer_state_t*)view->state;

    zdj_remove_all_subviews_of( view );

    if( state->image_count > 0 ) {
        SDL_Texture * tex = zdj_ui_texture_from_bmp( state->images[ state->image_index ] );
        zdj_view_t * img = zdj_new_asset_view( &(SDL_Rect){ 0,0,128,64 }, tex );
        zdj_add_subview( view, img );

        SDL_PixelFormatEnum format = 0;
        SDL_QueryTexture( tex, &format, NULL, NULL, NULL );
        printf( "pixel format: %s\n", SDL_GetPixelFormatName( format ) );
    }
    
    state->view_needs_refresh = false;
}

void _handle_filename_match( char * path, void * data ) {
    zdj_image_viewer_state_t * state = (zdj_image_viewer_state_t*)data;

    printf( "found image: %s\n", path );
    // Add filepath to images
    strcpy( state->images[ state->image_count ], path );
    state->image_count++;
}