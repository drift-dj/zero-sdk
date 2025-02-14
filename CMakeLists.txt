cmake_minimum_required(VERSION 3.23)
project(zerodj VERSION 0.1.0 LANGUAGES C)

add_library(zerodj 
    src/anim/zdj_anim.c
    src/display/zdj_display.c
    src/m7/zdj_m7.c
    src/hmi/zdj_hmi.c
    src/hmi/input/zdj_hmi_input.c
    src/hmi/input/zdj_hmi_sm_analog.c
    src/hmi/input/zdj_hmi_sm_digital.c
    src/registry/zdj_registry.c
    src/ui/zdj_ui.c
    src/ui/zdj_ui_util.c
    src/ui/asset/zdj_ui_asset.c
    src/ui/font/zdj_font.c
    src/ui/view/zdj_view_stack.c
    src/ui/view/menu_view/zdj_menu_view.c
    src/ui/view/menu_item_view/zdj_menu_item_view.c
    src/ui/view/menu_item_view/zdj_menu_item_static_simple.c
    src/ui/view/menu_section_view/zdj_menu_section_view.c
    src/ui/view/scroll_view/zdj_scroll_view.c
    src/ui/view/ticker_view/zdj_ticker_view.c
    src/xml/zdj_xml.c
)

target_compile_definitions(
    zerodj
    PUBLIC
    _POSIX_C_SOURCE=199309L
    _GNU_SOURCE
)

target_include_directories(
    zerodj
    PUBLIC
    sysroot/usr/include/
    sysroot/usr/include/aarch64-linux-gnu/
    sysroot/usr/local/include/
)

target_link_directories(
    zerodj
    PUBLIC
    lib
    sysroot/usr/lib
    sysroot/lib/aarch64-linux-gnu
    sysroot/lib/gcc/aarch64-linux-gnu/9
)

target_link_options(
    zerodj
    PUBLIC
    -fuse-ld=lld
)

target_link_libraries(
    zerodj
    PUBLIC
    stdc++
    SDL2
    SDL2_ttf
    SDL2_image
    SDL2_gfx
    xml2
)