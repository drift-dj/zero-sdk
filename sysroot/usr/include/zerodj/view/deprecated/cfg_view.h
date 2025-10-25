#ifndef VIEW_H
#define VIEW_H

#include <zerodj/system/installer/zdj_installer.h>
#include <zerodj/system/registry/zdj_registry.h>

extern zdj_view_t * ui_state->menu_stack;

void cfg_add_launch_view( void );
void cfg_add_apps_view( void );
void cfg_add_app_view( zdj_install_t * install );
void cfg_add_log_view( char * log_path );
void cfg_add_audio_bit_view( void );
void cfg_add_audio_io_view( void );
void cfg_add_installer_view( zdj_installer_t * installer, char * path );
void cfg_add_installer_confirm_view( zdj_installer_t * installer );
void cfg_add_installer_result_view( zdj_installer_t * installer );
void cfg_add_remove_confirm_view( zdj_install_t * install );
void cfg_add_remove_result_view( zdj_install_t * install );
zdj_view_t * cfg_new_menu( char * xml_menu_name );
void cfg_menu_item_handle_control_event( zdj_view_t * view, zdj_control_event_t * _event );
void cfg_menu_item_handle_back_btn( zdj_view_t * view );

#endif