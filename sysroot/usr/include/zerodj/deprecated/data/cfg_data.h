#ifndef CFG_DATA_H
#define CFG_DATA_H

#include <zerodj/ui/zdj_ui.h>

void cfg_update_data_audio_fb_vol( zdj_ui_data_t * data, char *link );
void cfg_update_data_audio_fb_verbose( zdj_ui_data_t * data, char *link );
void cfg_update_data_audio_fb_theme( zdj_ui_data_t * data, char *link );
void cfg_update_data_display_brightness( zdj_ui_data_t * data, char *link );
void cfg_update_data_display_rate( zdj_ui_data_t * data, char *link );
void cfg_update_data_usb_mode( zdj_ui_data_t * data, char *link );
void cfg_update_data_usb_con_status( zdj_ui_data_t * data, char *link );
void cfg_update_data_usb_charge_status( zdj_ui_data_t * data, char *link );
void cfg_update_data_usb_data_role( zdj_ui_data_t * data, char *link );
void cfg_update_data_usb_power_role( zdj_ui_data_t * data, char *link );

#endif