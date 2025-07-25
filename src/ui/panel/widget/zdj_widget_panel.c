#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/controls/hmi/zdj_hmi.h>
#include <zerodj/ui/zdj_ui.h>
// #include <zerodj/ui/anim/zdj_anim.h>
// #include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/panel/widget/zdj_widget_panel.h>
// #include <zerodj/ui/view/asset_view/zdj_asset_view.h>
// #include <zerodj/ui/view/file_browser_view/zdj_file_browser_view.h>
// #include <zerodj/ui/view/menu_view/zdj_menu_view.h>
// #include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
// #include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
// #include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
// #include <zerodj/ui/view/zdj_view_stack.h>

zdj_view_t * zdj_new_widget_panel( void ) {
    zdj_view_t * view = zdj_new_view( zdj_screen_rect( ) );
    return view;
}