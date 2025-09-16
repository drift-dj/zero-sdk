#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <zerodj/ui/anim/zdj_anim.h>

float _zdj_ease_linear( float a, float b );
float _zdj_ease_out_quart( float a, float b );
float _zdj_ease_in_quart( float a, float b );

void zdj_set_anim( zdj_anim_t * anim, zdj_anim_type_t type ) {
    // printf( "zdj_set_anim\n" );
    if( type == ZDJ_ANIM_NONE ) { return; }

    anim->type = type;
    anim->cb_fn = NULL;
    anim->superview = NULL;
    anim->view = NULL;
    anim->frames = 10;
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
