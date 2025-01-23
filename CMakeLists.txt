cmake_minimum_required(VERSION 3.23)
project(zerodj VERSION 0.1.0 LANGUAGES C)

# add_subdirectory(src/anim)
# add_subdirectory(src/boot)
# add_subdirectory(src/display)
# add_subdirectory(src/health)
# add_subdirectory(src/hmi)
# add_subdirectory(src/m7)
# add_subdirectory(src/menu)
# add_subdirectory(src/registry)

# include_directories(src/anim)
# include_directories(src/boot)
# include_directories(src/display)
# include_directories(src/health)
# include_directories(src/hmi)
# include_directories(src/m7)
# include_directories(src/menu)
# include_directories(src/registry)

add_library(zerodj 
    src/anim/zdj_anim.c
    src/display/zdj_display.c
    src/m7/zdj_m7.c
    src/hmi/zdj_hmi.c
    src/menu/zdj_menu.c
    src/menu/zdj_menu_parser.c
    src/registry/zdj_registry.c
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
)