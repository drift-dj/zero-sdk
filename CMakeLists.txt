cmake_minimum_required(VERSION 3.23)
project(zerodj VERSION 0.1.0 LANGUAGES C)

add_library(zerodj 
    src/controls/zdj_controls.c
    src/controls/zdj_control_map.c
    src/controls/hmi/zdj_hmi_input.c
    src/controls/hmi/zdj_hmi_input_map.c
    src/controls/hmi/zdj_hmi_scan_thread.c
    src/controls/hmi/zdj_hmi_sm_analog.c
    src/controls/hmi/zdj_hmi_sm_digital.c
    src/library/zdj_library.c
    src/library/api/library/zdj_library_api_library_config.c
    src/library/api/library/zdj_library_api_library.c
    src/library/api/library/query/zdj_library_api_query_artist.c
    src/library/api/library/query/zdj_library_api_query_bpm.c
    src/library/api/library/query/zdj_library_api_query_genre.c
    src/library/api/library/query/zdj_library_api_query_playlist.c
    src/library/api/library/query/zdj_library_api_query_year.c
    src/library/api/curation/zdj_library_api_playlist.c
    src/library/api/data_source/zdj_library_api_data_source.c
    src/library/api/setting/zdj_library_api_setting.c
    src/library/api/song/zdj_library_api_audio.c
    src/library/api/song/zdj_library_api_catalog.c
    src/library/api/song/zdj_library_api_curation.c
    src/library/api/song/zdj_library_api_performance.c
    src/library/api/song/zdj_library_api_song_graph.c
    src/library/api/song/zdj_library_api_song.c
    src/signal/deck/zdj_deck.c
    src/signal/deck/zdj_deck_loop_skip.c
    src/signal/deck/zdj_deck_manager.c
    src/signal/deck/cv/zdj_deck_cv.c
    src/signal/deck/dj/zdj_deck_dj.c
    src/signal/deck/extern/zdj_deck_extern.c
    src/signal/deck/lib/zdj_deck_lib.c
    src/signal/deck/live_loop/zdj_deck_live_loop.c
    src/signal/deck/plugin/zdj_deck_plugin.c
    src/signal/deck/test/zdj_deck_test.c
    src/signal/math/zdj_signal_math.c
    src/signal/pipeline/zdj_pipeline.c
    src/signal/pipeline/node/analysis/bpm_detect/zdj_bpm_detect_node.c
    src/signal/pipeline/node/analysis/meter/zdj_meter_node.c
    src/signal/pipeline/node/analysis/waveform/live/zdj_live_waveform.c
    src/signal/pipeline/node/analysis/waveform/playback/zdj_playback_waveform.c
    src/signal/pipeline/node/analysis/waveform/playback/zdj_playback_waveform_maker.c
    src/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.c
    src/signal/pipeline/node/audio/decode/zdj_decode_node.c
    src/signal/pipeline/node/audio/decode/zdj_decode_layer.c
    src/signal/pipeline/node/audio/decode/zdj_decode_packet.c
    src/signal/pipeline/node/audio/decode/zdj_decode_packet_mp3.c
    src/signal/pipeline/node/audio/decode/zdj_decode_packet_pcm.c
    src/signal/pipeline/node/audio/decode/zdj_decode_accum.c
    src/signal/pipeline/node/audio/io/zdj_io_analog_node.c
    src/signal/pipeline/node/audio/io/zdj_io_usb_node.c
    src/signal/pipeline/node/audio/library_decode/zdj_library_decode_node.c
    src/signal/pipeline/node/audio/record/zdj_audio_record_node.c
    src/signal/pipeline/node/audio/tsm/zdj_tsm_pitch_node.c
    src/signal/pipeline/node/file/zdj_file_read_node.c
    src/signal/pipeline/node/file/zdj_file_write_node.c
    src/signal/soundcard/zdj_soundcard.c
    src/signal/soundcard/zdj_soundcard_deck.c
    src/signal/soundcard/zdj_soundcard_dto.c
    src/signal/soundcard/zdj_soundcard_mix.c
    src/signal/soundcard/zdj_soundcard_node.c
    src/system/boot/zdj_boot.c
    src/system/debug/zdj_debug.c
    src/system/display/zdj_display.c
    src/system/error/zdj_error.c
    src/system/fs/zdj_fs.c
    src/system/m7/zdj_m7.c
    src/system/installer/zdj_installer.c
    src/system/perf/zdj_perf.c
    src/system/registry/zdj_registry_install.c
    src/system/registry/zdj_registry_launch_req.c
    src/system/thread/zdj_thread.c
    src/system/sql/zdj_sql.c
    src/system/usb/zdj_usb.c
    src/system/usb/zdj_usb_device.c
    src/system/usb/zdj_usb_msd_device.c
    src/system/usb/zdj_usb_poll.c
    src/system/xml/zdj_xml.c
    src/system/xml/zdj_xml_menu.c
    src/ui/zdj_ui.c
    src/ui/zdj_ui_util.c
    src/ui/anim/zdj_anim.c
    src/ui/anim/zdj_anim_debug_panel.c
    src/ui/anim/zdj_anim_dialog.c
    src/ui/anim/zdj_anim_dj_deck_page.c
    src/ui/anim/zdj_anim_header.c
    src/ui/anim/zdj_anim_menu.c
    src/ui/anim/zdj_anim_menu_stack.c
    src/ui/anim/zdj_anim_modal.c
    src/ui/anim/zdj_anim_view.c
    src/ui/asset/zdj_ui_asset.c
    src/ui/font/zdj_font.c
    src/ui/panel/accessibility/zdj_accessibility_panel.c
    src/ui/panel/debug/zdj_debug_panel.c
    src/ui/panel/perf/zdj_perf_panel.c
    src/ui/panel/widget/zdj_widget_panel.c
    src/ui/view/zdj_view_stack.c
    src/ui/view/asset_view/zdj_asset_view.c
    src/ui/view/cue_point_view/zdj_cue_point_view.c
    src/ui/view/debug_view/zdj_debug_view.c
    src/ui/view/device_settings_menu_view/zdj_device_settings_menu_view.c
    src/ui/view/dialog_view/zdj_dialog_view.c
    src/ui/view/file_browser_view/zdj_file_browser_view.c
    src/ui/view/file_browser_view/zdj_file_browser_devices_menu.c
    src/ui/view/file_browser_view/zdj_file_browser_file_menu.c
    src/ui/view/label_view/zdj_label_view.c
    src/ui/view/log_view/zdj_log_view.c
    src/ui/view/menu_view/zdj_menu_view.c
    src/ui/view/menu_view/zdj_menu_view_scroll_filter.c
    src/ui/view/menu_header_view/zdj_menu_header_view.c
    src/ui/view/menu_item_view/zdj_menu_item_view.c
    src/ui/view/menu_item_view/zdj_menu_item_view_asset.c
    src/ui/view/menu_item_view/zdj_menu_item_view_basic.c
    src/ui/view/menu_item_view/zdj_menu_item_view_data.c
    src/ui/view/menu_item_view/zdj_menu_item_view_dir.c
    src/ui/view/menu_item_view/zdj_menu_item_view_icon.c
    src/ui/view/menu_item_view/zdj_menu_item_view_inert.c
    src/ui/view/menu_item_view/zdj_menu_item_view_launch.c
    src/ui/view/menu_item_view/zdj_menu_item_view_slider.c
    src/ui/view/menu_item_view/zdj_menu_item_view_song.c
    src/ui/view/menu_item_view/zdj_menu_item_view_toggle.c
    src/ui/view/menu_section_view/zdj_menu_section_view.c
    src/ui/view/menu_stack/zdj_menu_stack.c
    src/ui/view/modal_view/zdj_modal_view.c
    src/ui/view/progress_bar_view/zdj_progress_bar_view.c
    src/ui/view/scope_view/zdj_scope_view.c
    src/ui/view/scroll_view/zdj_scroll_view.c
    src/ui/view/soundcard_view/zdj_soundcard_view.c
    src/ui/view/soundcard_view/meters/zdj_soundcard_meter.c
    src/ui/view/soundcard_view/meters/zdj_soundcard_audio_meter.c
    src/ui/view/soundcard_view/meters/zdj_soundcard_clock_meter.c
    src/ui/view/soundcard_view/meters/zdj_soundcard_cv_meter.c
    src/ui/view/soundcard_view/meters/zdj_soundcard_midi_meter.c
    src/ui/view/soundcard_view/meters/zdj_soundcard_usb_meter.c
    src/ui/view/soundcard_view/options/zdj_soundcard_options.c
    src/ui/view/soundcard_view/options/zdj_soundcard_options_audio_bus.c
    src/ui/view/soundcard_view/options/zdj_soundcard_options_cv.c
    src/ui/view/soundcard_view/options/zdj_soundcard_options_clock.c
    src/ui/view/soundcard_view/options/zdj_soundcard_options_port_input.c
    src/ui/view/soundcard_view/options/zdj_soundcard_options_port_output.c
    src/ui/view/soundcard_view/select_node/zdj_soundcard_select_node.c
    src/ui/view/text_input_view/zdj_text_input_view.c
    src/ui/view/text_input_view/buffer/zdj_text_input_buffer_view.c
    src/ui/view/text_input_view/keyboard/zdj_text_input_keyboard_view.c
    src/ui/view/texture_view/zdj_texture_view.c
    src/ui/view/thread_view/zdj_thread_view.c
    src/ui/view/ticker_view/zdj_ticker_view.c
    src/ui/view/usb_drive_view/zdj_usb_drive_view.c
    src/ui/view/usb_status_view/zdj_usb_status_view.c
    src/ui/view/usb_status_view/zdj_usb_status_view_device.c
    src/ui/view/usb_status_view/zdj_usb_status_view_host.c
    src/ui/view/usb_status_view/zdj_usb_status_view_offline.c
    src/ui/view/usb_status_view/zdj_usb_status_view_reboot.c
    src/ui/view/waveform_view/zdj_live_waveform_view.c
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

target_compile_options(
    zerodj
    PUBLIC
    -gdwarf-4
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
    uuid
    mpg123
    mhash
    m
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