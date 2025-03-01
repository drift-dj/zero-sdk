cmake_minimum_required(VERSION 3.23)
project(zerodj VERSION 0.1.0 LANGUAGES C)

add_library(zerodj 
    src/display/zdj_display.c
    src/fs/zdj_fs.c
    src/m7/zdj_m7.c
    src/hmi/zdj_hmi.c
    src/hmi/input/zdj_hmi_input.c
    src/hmi/input/zdj_hmi_sm_analog.c
    src/hmi/input/zdj_hmi_sm_digital.c
    src/installer/zdj_installer.c
    src/registry/zdj_registry.c
    src/registry/zdj_registry_install.c
    src/registry/zdj_registry_launch_req.c
    src/sql/zdj_sql.c
    src/ui/zdj_ui.c
    src/ui/zdj_ui_util.c
    src/ui/anim/zdj_anim.c
    src/ui/asset/zdj_ui_asset.c
    src/ui/font/zdj_font.c
    src/ui/view/zdj_view_stack.c
    src/ui/view/asset_view/zdj_asset_view.c
    src/ui/view/file_browser_view/zdj_file_browser_view.c
    src/ui/view/file_browser_view/zdj_file_browser_devices_menu.c
    src/ui/view/file_browser_view/zdj_file_browser_file_menu.c
    src/ui/view/label_view/zdj_label_view.c
    src/ui/view/menu_view/zdj_menu_view.c
    src/ui/view/menu_header_view/zdj_menu_header_view.c
    src/ui/view/menu_item_view/zdj_menu_item_view.c
    src/ui/view/menu_item_view/zdj_menu_item_view_basic.c
    src/ui/view/menu_item_view/zdj_menu_item_view_data.c
    src/ui/view/menu_item_view/zdj_menu_item_view_dir.c
    src/ui/view/menu_item_view/zdj_menu_item_view_nohilite.c
    src/ui/view/menu_item_view/zdj_menu_item_view_slider.c
    src/ui/view/menu_item_view/zdj_menu_item_view_toggle.c
    src/ui/view/menu_section_view/zdj_menu_section_view.c
    src/ui/view/scroll_view/zdj_scroll_view.c
    src/ui/view/texture_view/zdj_texture_view.c
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
    sqlite3
    xml2
)

# add_custom_command(
#     TARGET zerodj POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E echo "Copying libzerodj.a to sysroot/usr/lib"
#     COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:zerodj> ${CMAKE_BINARY_DIR}/sysroot/usr/lib/libzerodj.a

#     COMMAND ${CMAKE_COMMAND} -E echo "Copying source directory to sysroot/usr/include/zerodj"
#     COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/sysroot/usr/include/zerodj

#     COMMAND ${CMAKE_COMMAND} -E echo "Removing .c files from sysroot/usr/include/zerodj"
#     COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/sysroot/usr/include/zerodj/*.c

#     COMMAND ${CMAKE_COMMAND} -E echo "Removing CMake files from sysroot/usr/include/zerodj"
#     COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/sysroot/usr/include/zerodj/*CMake*
# )