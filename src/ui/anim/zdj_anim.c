#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <zerodj/ui/anim/zdj_anim.h>

float _zdj_ease_linear( float a, float b );
float _zdj_ease_out_quart( float a, float b );
float _zdj_ease_in_quart( float a, float b );

int zdj_anim_show_predelay( void ) {
    // float len_msec = 200.0;
    // float frame_msec = (float)zdj_ui_get_frame_nanos( ) / 100000.0;
    // return (int)( len_msec / frame_msec ) * -1;
    return zdj_ui_msec_to_frames( 90 ) * -1;
}

int zdj_anim_hide_predelay( void ) {
    // float len_msec = 150.0;
    // float frame_msec = (float)zdj_ui_get_frame_nanos( ) / 100000.0;
    // return (int)( len_msec / frame_msec ) * -1;
    return zdj_ui_msec_to_frames( 50 ) * -1;
}

float zdj_anim_show_hide_frames( void ) {
    // float len_msec = 300.0;
    // float frame_msec = (float)zdj_ui_get_frame_nanos( ) / 100000.0;
    // return (int)( len_msec / frame_msec );
    return zdj_ui_msec_to_frames( 100 );
}

void zdj_set_anim( zdj_anim_t * anim, zdj_anim_type_t type ) {
    // printf( "zdj_set_anim\n" );
    if( type == ZDJ_ANIM_NONE ) { return; }

    anim->type = type;
    anim->cb_fn = NULL;
    anim->superview = NULL;
    anim->view = NULL;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->ease = &_zdj_ease_out_quart;

    switch ( type ) {
        case ZDJ_ANIM_EMPTY:
            anim->update_fn = NULL;
            break;
        case ZDJ_ANIM_VIEW_SHOW:
            anim->init_fn = &zdj_anim_init_view_show;
            anim->update_fn = &zdj_anim_update_view;
            break;
        case ZDJ_ANIM_VIEW_HIDE:
            anim->init_fn = &zdj_anim_init_view_hide;
            anim->update_fn = &zdj_anim_update_view;
            break;
        case ZDJ_ANIM_MENU_SHOW:
            anim->init_fn = &zdj_anim_init_menu_show;
            anim->update_fn = &zdj_anim_update_menu;
            break;
        case ZDJ_ANIM_MENU_HIDE:
            anim->init_fn = &zdj_anim_init_menu_hide;
            anim->update_fn = &zdj_anim_update_menu;
            break;
        case ZDJ_ANIM_MENU_STACK_SHOW:
            anim->init_fn = &zdj_anim_init_menu_stack_show;
            anim->update_fn = &zdj_anim_update_menu_stack;
            break;
        case ZDJ_ANIM_MENU_STACK_HIDE:
            anim->init_fn = &zdj_anim_init_menu_stack_hide;
            anim->update_fn = &zdj_anim_update_menu_stack;
            break;
        case ZDJ_ANIM_MODAL_SHOW:
            anim->init_fn = &zdj_anim_init_modal_show;
            anim->update_fn = &zdj_anim_update_modal;
            break;
        case ZDJ_ANIM_MODAL_HIDE:
            anim->init_fn = &zdj_anim_init_modal_hide;
            anim->update_fn = &zdj_anim_update_modal;
            break;
        case ZDJ_ANIM_HEADER_ACTIVATE:
            anim->init_fn = &zdj_anim_init_header_activate;
            anim->update_fn = &zdj_anim_update_header;
            break;
        case ZDJ_ANIM_HEADER_DEACTIVATE:
            anim->init_fn = &zdj_anim_init_header_deactivate;
            anim->update_fn = &zdj_anim_update_header;
            break;
        case ZDJ_ANIM_HEADER_BACK_ACTIVATE:
            anim->init_fn = &zdj_anim_init_header_back_activate;
            anim->update_fn = &zdj_anim_update_header_back;
            break;
        case ZDJ_ANIM_HEADER_BACK_DEACTIVATE:
            anim->init_fn = &zdj_anim_init_header_back_deactivate;
            anim->update_fn = &zdj_anim_update_header_back;
            break;
        case ZDJ_ANIM_HEADER_CLOSE_ACTIVATE:
            anim->init_fn = &zdj_anim_init_header_close_activate;
            anim->update_fn = &zdj_anim_update_header_close;
            break;
        case ZDJ_ANIM_HEADER_CLOSE_DEACTIVATE:
            anim->init_fn = &zdj_anim_init_header_close_deactivate;
            anim->update_fn = &zdj_anim_update_header_close;
            break;
        case ZDJ_ANIM_DIALOG_SHOW:
            anim->init_fn = &zdj_anim_init_dialog_show;
            anim->update_fn = &zdj_anim_update_dialog;
            break;
        case ZDJ_ANIM_DIALOG_HIDE:
            anim->init_fn = &zdj_anim_init_dialog_hide;
            anim->update_fn = &zdj_anim_update_dialog;
            break;
        case ZDJ_ANIM_DJ_DECK_PAGE_SHOW:
            anim->init_fn = &zdj_anim_init_dj_deck_page_show;
            anim->update_fn = &zdj_anim_update_dj_deck_page;
            break;
        case ZDJ_ANIM_DJ_DECK_PAGE_HIDE:
            anim->init_fn = &zdj_anim_init_dj_deck_page_hide;
            anim->update_fn = &zdj_anim_update_dj_deck_page;
            break;
        case ZDJ_ANIM_DEBUG_PANEL_SHOW:
            anim->init_fn = &zdj_anim_init_debug_panel_show;
            anim->update_fn = &zdj_anim_update_debug_panel;
            break;
        case ZDJ_ANIM_DEBUG_PANEL_HIDE:
            anim->init_fn = &zdj_anim_init_debug_panel_hide;
            anim->update_fn = &zdj_anim_update_debug_panel;
            break;
        case ZDJ_ANIM_VOLUME_PANEL_SHOW:
            anim->init_fn = &zdj_anim_init_volume_panel_show;
            anim->update_fn = &zdj_anim_update_volume_panel;
            break;
        case ZDJ_ANIM_VOLUME_PANEL_HIDE:
            anim->init_fn = &zdj_anim_init_volume_panel_hide;
            anim->update_fn = &zdj_anim_update_volume_panel;
            break;
        case ZDJ_ANIM_RECORD_PANEL_SHOW:
            anim->init_fn = &zdj_anim_init_record_panel_show;
            anim->update_fn = &zdj_anim_update_record_panel;
            break;
        case ZDJ_ANIM_RECORD_PANEL_HIDE:
            anim->init_fn = &zdj_anim_init_record_panel_hide;
            anim->update_fn = &zdj_anim_update_record_panel;
            break;
        case ZDJ_ANIM_RECORD_WIDGET_SHOW:
            anim->init_fn = &zdj_anim_init_record_widget_show;
            anim->update_fn = &zdj_anim_update_record_widget;
            break;
        case ZDJ_ANIM_RECORD_WIDGET_HIDE:
            anim->init_fn = &zdj_anim_init_record_widget_hide;
            anim->update_fn = &zdj_anim_update_record_widget;
            break;
        case ZDJ_ANIM_DEBUG_WIDGET_SHOW:
            anim->init_fn = &zdj_anim_init_debug_widget_show;
            anim->update_fn = &zdj_anim_update_debug_widget;
            break;
        case ZDJ_ANIM_DEBUG_WIDGET_HIDE:
            anim->init_fn = &zdj_anim_init_debug_widget_hide;
            anim->update_fn = &zdj_anim_update_debug_widget;
            break;
        case ZDJ_ANIM_PERF_WIDGET_SHOW:
            anim->init_fn = &zdj_anim_init_perf_widget_show;
            anim->update_fn = &zdj_anim_update_perf_widget;
            break;
        case ZDJ_ANIM_PERF_WIDGET_HIDE:
            anim->init_fn = &zdj_anim_init_perf_widget_hide;
            anim->update_fn = &zdj_anim_update_perf_widget;
            break;
        case ZDJ_ANIM_NOTIFY_WIDGET_SHOW:
            anim->init_fn = &zdj_anim_init_notify_widget_show;
            anim->update_fn = &zdj_anim_update_notify_widget;
            break;
        case ZDJ_ANIM_NOTIFY_WIDGET_HIDE:
            anim->init_fn = &zdj_anim_init_notify_widget_hide;
            anim->update_fn = &zdj_anim_update_notify_widget;
            break;
        case ZDJ_ANIM_PANEL_IN_NEXT:
            anim->init_fn = &zdj_anim_init_panel_in_next;
            anim->update_fn = &zdj_anim_update_panel;
            break;
        case ZDJ_ANIM_PANEL_OUT_NEXT:
            anim->init_fn = &zdj_anim_init_panel_out_next;
            anim->update_fn = &zdj_anim_update_panel;
            break;
        case ZDJ_ANIM_PANEL_IN_PREV:
            anim->init_fn = &zdj_anim_init_panel_in_prev;
            anim->update_fn = &zdj_anim_update_panel;
            break;
        case ZDJ_ANIM_PANEL_OUT_PREV:
            anim->init_fn = &zdj_anim_init_panel_out_prev;
            anim->update_fn = &zdj_anim_update_panel;
            break;
        case ZDJ_ANIM_PANEL_DEPLOY:
            anim->init_fn = &zdj_anim_init_panel_deploy;
            anim->update_fn = &zdj_anim_update_panel;
            break;
        case ZDJ_ANIM_PANEL_RETRACT:
            anim->init_fn = &zdj_anim_init_panel_retract;
            anim->update_fn = &zdj_anim_update_panel;
            break;
    }
}

float _zdj_ease_linear( float a, float b ) {
    return a / b;
}

float _zdj_ease_out_quart( float a, float b ) {
    float x = a / b;
    return x * x * x * x;
}

float _zdj_ease_in_quart( float a, float b ) {
    float x = a / b;
    return 1.0 - pow( 1.0 - x, 4 );
}
