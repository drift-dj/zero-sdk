#include <stdlib.h>
#include <math.h>

#include <zerodj/system/battery/zdj_battery.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/battery_icon_view/zdj_battery_icon_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );

static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_battery_icon_view( zdj_battery_icon_type_t type ) {
    zdj_view_t * view = zdj_new_view( &(zdj_rect_t){0,0,10,5} );
    view->draw = _draw;
    // printf( "zdj_new_battery_icon_view\n" );
    
    zdj_battery_icon_state_t * state = calloc( 1, sizeof( zdj_battery_icon_state_t ) );

    state->bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BATT_SM ], NULL );
    zdj_add_subview( view, state->bg );

    state->bar = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    state->bar->frame.w = 0;
    state->bar->frame.h = 3;
    state->bar->frame.x = 3;
    state->bar->frame.y = 1;
    zdj_add_subview( view, state->bar );

    view->state = state;

    // printf( "zdj_new_battery_icon_view done: %p\n", view );
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_battery_icon_state_t * state = (zdj_battery_icon_state_t*)view->state;
    
    if( zdj_battery_state( )->valid ) {
        double val = zdj_battery_state( )->charge_pct;
        state->bar->frame.w = ( 1.0 - val ) * 6;
    }

}
