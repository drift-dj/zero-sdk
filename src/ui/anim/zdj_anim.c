#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <zerodj/ui/anim/zdj_anim.h>

float _zdj_ease_linear( float a, float b );
float _zdj_ease_out_quart( float a, float b );
float _zdj_ease_in_quart( float a, float b );

zdj_anim_t * zdj_new_anim( zdj_anim_type_t type ) {
    if( type == ZDJ_ANIM_NONE ) { return NULL; }

    zdj_anim_t * anim = calloc( 1, sizeof( zdj_anim_t ) );
    anim->cb_fn = NULL;
    anim->superview = NULL;
    anim->view = NULL;
    anim->frames = 10;
    anim->ease = &_zdj_ease_out_quart;
    anim->start_data = NULL;
    anim->end_data = NULL;
    switch ( type ) {
        case ZDJ_ANIM_EMPTY:
            anim->update_fn = NULL;
            break;
        case ZDJ_ANIM_VIEW_SHOW:
            anim->init_fn = &zdj_anim_init_view_show;
            anim->update_fn = &zdj_anim_update_view;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_VIEW_HIDE:
            anim->init_fn = &zdj_anim_init_view_hide;
            anim->update_fn = &zdj_anim_update_view;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_MENU_SHOW:
            anim->init_fn = &zdj_anim_init_menu_show;
            anim->update_fn = &zdj_anim_update_menu;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_MENU_HIDE:
            anim->init_fn = &zdj_anim_init_menu_hide;
            anim->update_fn = &zdj_anim_update_menu;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_MENU_STACK_SHOW:
            anim->init_fn = &zdj_anim_init_menu_stack_show;
            anim->update_fn = &zdj_anim_update_menu_stack;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_MENU_STACK_HIDE:
            anim->init_fn = &zdj_anim_init_menu_stack_hide;
            anim->update_fn = &zdj_anim_update_menu_stack;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_MODAL_SHOW:
            anim->init_fn = &zdj_anim_init_modal_show;
            anim->update_fn = &zdj_anim_update_modal;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_MODAL_HIDE:
            anim->init_fn = &zdj_anim_init_modal_hide;
            anim->update_fn = &zdj_anim_update_modal;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_HEADER_ACTIVATE:
            anim->init_fn = &zdj_anim_init_header_activate;
            anim->update_fn = &zdj_anim_update_header;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_HEADER_DEACTIVATE:
            anim->init_fn = &zdj_anim_init_header_deactivate;
            anim->update_fn = &zdj_anim_update_header;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_DIALOG_SHOW:
            anim->init_fn = &zdj_anim_init_dialog_show;
            anim->update_fn = &zdj_anim_update_dialog;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_DIALOG_HIDE:
            anim->init_fn = &zdj_anim_init_dialog_hide;
            anim->update_fn = &zdj_anim_update_dialog;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_DJ_DECK_PAGE_SHOW:
            anim->init_fn = &zdj_anim_init_dj_deck_page_show;
            anim->update_fn = &zdj_anim_update_dj_deck_page;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_DJ_DECK_PAGE_HIDE:
            anim->init_fn = &zdj_anim_init_dj_deck_page_hide;
            anim->update_fn = &zdj_anim_update_dj_deck_page;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_DEBUG_PANEL_SHOW:
            anim->init_fn = &zdj_anim_init_debug_panel_show;
            anim->update_fn = &zdj_anim_update_debug_panel;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
        case ZDJ_ANIM_DEBUG_PANEL_HIDE:
            anim->init_fn = &zdj_anim_init_debug_panel_hide;
            anim->update_fn = &zdj_anim_update_debug_panel;
            anim->deinit_fn = &zdj_anim_deinit;
            break;
    }
    return anim;
}

void zdj_anim_deinit( zdj_anim_t * anim ) { 
    free( anim->start_data );
    free( anim->end_data );
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
