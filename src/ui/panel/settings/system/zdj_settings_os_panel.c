#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mount.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/zdj_data_type.h>
#include <zerodj/controls/zdj_controls.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/hash/zdj_hash.h>
#include <zerodj/system/installer/zdj_installer.h>
#include <zerodj/system/log/zdj_log.h>
#include <zerodj/system/registry/zdj_registry.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/panel/settings/zdj_settings_panel.h>
#include <zerodj/ui/panel/settings/system/zdj_settings_system_panel.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/dialog_view/zdj_dialog_view.h>
#include <zerodj/ui/view/file_browser_view/zdj_file_browser_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

typedef struct {
    zdj_view_t * menu;
    zdj_view_t * overlay;
    int overlay_counter;
    bool needs_layout_update;
    zdj_view_t * event_target;
    void (*exit_cb) ( void* );

    char reset_image_path[ 512 ];
    bool has_valid_image;
    zdj_os_sysreg_t * sysreg;
    zdj_view_t * usb_copy_dialog;
    zdj_fs_copy_context_t * copy_context;
} _zdj_os_panel_state_t;

static _zdj_os_panel_state_t * _os_panel_state;

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _refresh_menu( zdj_view_t * view );

static void _check_reset_image( char * image_path );

static bool _mount_reset_partitions( char * img_path );
static void _unmount_reset_partitions( void );

static void _deinit_state( zdj_view_t * view );

static void _handle_back( zdj_view_t * view );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * event );

static void _update_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _update_browser_exit( zdj_view_t * browser, zdj_file_browser_exit_context_t * context );

static void _install_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _install_dialog_exit( zdj_view_t * view, void * data, bool selection );

zdj_view_t * zdj_new_settings_os_panel( void (*cb)(void*) ) {
    // printf( "zdj_new_settings_os_panel\n" );
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_draw;
    view->handle_control_event = &_handle_control;
    view->map = ZDJ_CONTROL_MAP_SETTINGS_PANEL;

    // zdj_settings_panel_state_t * state = calloc( 1, sizeof( zdj_settings_panel_state_t ) );
    _os_panel_state = calloc( 1, sizeof( _zdj_os_panel_state_t ) );
    _os_panel_state->needs_layout_update = true;
    _os_panel_state->exit_cb = cb;
    _os_panel_state->copy_context = NULL;
    view->state = _os_panel_state;


    // Make menu
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, zdj_modal_rect( ) );
    zdj_add_subview( view, menu );
    menu->frame.x = 0;
    menu->frame.y = 0;
    menu->frame.w = ZDJ_MODAL_WIDTH;
    menu->frame.h = ZDJ_MODAL_HEIGHT;
    _os_panel_state->menu = menu;
    
    // Set up header
    zdj_view_t * menu_header = zdj_new_menu_header( 
        "Settings",
        " ",
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_BACK
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    header_state->handle_back = &_handle_back;
    zdj_menu_view_add_header( menu, menu_header );

    return view;     
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;
    
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( _os_panel_state->needs_layout_update ) { _refresh_menu( view ); }

    if( _os_panel_state->copy_context ) {
        if( _os_panel_state->usb_copy_dialog ) { 
            zdj_dialog_view_state_t * dialog_state = (zdj_dialog_view_state_t*)_os_panel_state->usb_copy_dialog->state;
            dialog_state->progress = _os_panel_state->copy_context->progress;
        }
        if( _os_panel_state->copy_context->state == ZDJ_FS_COPY_STATE_ERROR ||
            _os_panel_state->copy_context->state == ZDJ_FS_COPY_STATE_SUCCESS ) {
            if( _os_panel_state->copy_context->state == ZDJ_FS_COPY_STATE_SUCCESS ) {
                _check_reset_image( _os_panel_state->reset_image_path );
            }
            // Copy done, pop the dialog
            _os_panel_state->usb_copy_dialog = NULL;
            _os_panel_state->copy_context = NULL;
            zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
            zdj_pop_subview_of( panel_state->settings_panel, true );
        }
    }
}

static void _refresh_menu( zdj_view_t * view ) {
    // zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;

    zdj_menu_view_remove_all_subviews( _os_panel_state->menu );

    char str[ 64 ];

    zdj_registry_put_system_display_name_str( str );
    zdj_view_t * os_name_label = zdj_new_label_view( str, ZDJ_FONT_12_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    // os_name_label->frame.x = 4;
    // os_name_label->frame.y = 25;
    zdj_menu_view_add_item( _os_panel_state->menu, os_name_label );

    zdj_registry_put_system_build_desc_str( str );
    zdj_view_t * build_desc_label = zdj_new_label_view( str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    build_desc_label->frame.y = 14;
    zdj_menu_view_add_item( _os_panel_state->menu, build_desc_label );

    zdj_registry_put_system_version_str( str );
    zdj_view_t * version_label = zdj_new_label_view( str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    version_label->frame.y = 22;
    zdj_menu_view_add_item( _os_panel_state->menu, version_label );
    
    if( !_os_panel_state->has_valid_image ) {
        // If we don't have a valid image to install, show the update btn
        zdj_view_t * update_btn = zdj_new_menu_item( "Update", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
        update_btn->frame.y = view->frame.h - 17;
        update_btn->handle_control_event = _update_btn;
        zdj_menu_item_view_state_t * update_state = (zdj_menu_item_view_state_t*)update_btn->state;
        update_state->data.ptr = view;
        zdj_menu_view_add_item( _os_panel_state->menu, update_btn );
    } else {
        // zdj_os_sysreg_t * sysreg = (zdj_os_sysreg_t*)state->data;
        char version_str[ 64 ];
        snprintf( version_str, sizeof( version_str ), "%s.%s.%s r%s", 
            _os_panel_state->sysreg->version_major, 
            _os_panel_state->sysreg->version_minor, 
            _os_panel_state->sysreg->version_hotfix, 
            _os_panel_state->sysreg->version_build
        );
        zdj_view_t * install_btn = zdj_new_data_menu_item( 
            "Install Version", 
            ZDJ_MENU_ITEM_LAYOUT_DATA_R,
            ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
            NULL,
            NULL 
        );
        install_btn->frame.y = view->frame.h - 17;
        install_btn->handle_control_event = &_install_btn;
        zdj_menu_item_view_state_t * install_state = (zdj_menu_item_view_state_t*)install_btn->state;
        strcpy( install_state->data.c_val, version_str );
        // Pass os panel state ref forward to install handler to carry filepath + sysreg ref
        install_state->data.ptr = _os_panel_state;
        zdj_menu_view_add_item( _os_panel_state->menu, install_btn );
    }

    _os_panel_state->needs_layout_update = false;
}


static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Ignore events which have been blocked by layers above this one.
    if( _event->blocked ) { return; }

    zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu->state;

    if( (_event->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 &&
        menu_state->scroll_index == -1) ||
        _event->id == ZDJ_UI_CONTROL_NAV_RELEASE_0
    ) {
        // Dump the top view on the stack (this view)
        zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
        zdj_pop_subview_of( panel_state->settings_panel, true );
        _event->blocked = true;
        // Return immediately since we're being dismissed
        return;
    } else { 
        // Send events down into the subview stack
        zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;
        state->menu->handle_control_event( state->menu, _event );
    }

    _event->blocked = true;
}

static void _handle_back( zdj_view_t * menu_view ) {
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_pop_subview_of( panel_state->settings_panel, true );
}

static void _deinit_state( zdj_view_t * view ) {

}

static void _update_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * update_btn_state = (zdj_menu_item_view_state_t*)view->state;

    // If there's a USB drive attached, launch the browser at the devices menu
    // else, launch at /media/internal
    char browser_path[ 128 ];
    strcpy( browser_path, "/media/internal" );
    if( zdj_usb_state->host_status.attached.has_msd ) {
        strcpy( browser_path, "devices" );
    }
    
    zdj_view_t * browser = zdj_new_file_browser_view( 
        zdj_modal_rect( ), browser_path, 
        true, 
        true, 
        ZDJ_FILE_BROWSER_TYPE_SELECT_FILE, 
        "Scan",
        false
    );
    if( !browser ) {
        printf( "Unable to open browser\n" );
        return;
    }
    
    // Add a select callback
    zdj_file_browser_view_state_t * browser_state = (zdj_file_browser_view_state_t *)browser->state;
    browser_state->handle_file_browser_exit = &_update_browser_exit;
    // Pass the btn's os_panel_view ref into the browser
    browser_state->data.ptr = update_btn_state->data.ptr;
    // Add the menu to the top of the stack
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_push_subview( panel_state->settings_panel, browser, true );
}


static void _update_browser_exit( zdj_view_t * browser, zdj_file_browser_exit_context_t * context ) {
    zdj_file_browser_view_state_t * browser_state = (zdj_file_browser_view_state_t*)browser->state;
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    
    // Pop the browser
    _os_panel_state->needs_layout_update = true;
    zdj_pop_subview_of( panel_state->settings_panel, true );

    if( context->status == ZDJ_FILE_BROWSER_EXIT_STATUS_SELECT ) {  
        // char image_path[ 512 ];

        // Immediately check a local image
        if( zdj_fs_path_is_media_partition( context->filepath ) ) {
            printf( "local image\n" );
            snprintf( _os_panel_state->reset_image_path, 512, "%s", context->filepath );
            _check_reset_image( _os_panel_state->reset_image_path );
        
        // Copy image from USB Drive if needed
        } else {
            snprintf( _os_panel_state->reset_image_path, 512, "/media/internal/installers/%s", context->filename );
            zdj_fs_copy_context_t * copy_context = zdj_fs_bg_copy_file( context->filepath, _os_panel_state->reset_image_path, true, true );
            if( !copy_context ) { return; }

            _os_panel_state->copy_context = copy_context;
            // Put up a dialog while we're copying
            _os_panel_state->usb_copy_dialog = zdj_new_dialog_view( 
                ZDJ_DIALOG_VIEW_TYPE_LOADING,
                "Copying",
                "Copying Image from Drive",
                basename( context->filepath )
            );
            zdj_dialog_view_state_t * dialog_state = (zdj_dialog_view_state_t*)_os_panel_state->usb_copy_dialog->state;
            dialog_state->handle_dialog_exit = _install_dialog_exit;
            dialog_state->selection_data = panel_state->settings_panel;

            zdj_push_subview( panel_state->settings_panel, _os_panel_state->usb_copy_dialog, true );
        }
    }
}



static void _check_reset_image( char * image_path ) {
    printf( "check reset image: %s\n", image_path );

    if( !_mount_reset_partitions( image_path ) ) {
        printf( "failed to mount reset partitions\n" );
        _unmount_reset_partitions( );
        return;
    }

    // Capture sysreg data
    FILE * sysreg_fd = fopen( "/mnt/update/sysreg", "r" );
    zdj_os_sysreg_t * sysreg = calloc( 1, sizeof( zdj_os_sysreg_t ) );
    fread( sysreg, sizeof( zdj_os_sysreg_t ), 1, sysreg_fd );
        
    // printf( "sysreg:\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%d\n%d\n%d\n", 
    //     sysreg->display_name, 
    //     sysreg->short_name, 
    //     sysreg->uuid, 
    //     sysreg->description,
    //     sysreg->version_major, 
    //     sysreg->version_minor,
    //     sysreg->version_hotfix, 
    //     sysreg->version_build,
    //     sysreg->version_build_desc, 
    //     sysreg->bootloader_img,
    //     sysreg->bootloader_sum, 
    //     sysreg->linux_img,
    //     sysreg->linux_sum, 
    //     sysreg->rootfs_img,
    //     sysreg->rootfs_sum,
    //     sysreg->installer_props.has_bootloader,
    //     sysreg->installer_props.has_linux,
    //     sysreg->installer_props.has_rootfs
    // );

    fclose( sysreg_fd );

    // Generate crc32 hash of images and match against sysreg
    char path[ 256 ];
    char hash[ ZDJ_HASH_LEN ];
    double space_required = 0.0;
    if( sysreg->installer_props.has_bootloader ) {
        sprintf( path, "/mnt/update/%s", sysreg->bootloader_img );
        // zdj_put_file_hash( path, hash );
        zdj_put_crc32_file_hash( path, hash );
        if( strcmp( hash, sysreg->bootloader_sum ) ) {
            // Show error dialog if chacksums mismatch
            printf( "mismatching bootloader hash: %s / %s\n", hash, sysreg->bootloader_sum );
            // system( unmount_command );
            return; 
        }
        space_required += zdj_fs_get_filesize( path );
    }
    if( sysreg->installer_props.has_linux ) {
        sprintf( path, "/mnt/update/%s", sysreg->linux_img );
        // zdj_put_file_hash( path, hash );
        zdj_put_crc32_file_hash( path, hash );
        if( strcmp( hash, sysreg->linux_sum ) ) {
            // Show error dialog if chacksums mismatch
            printf( "mismatching linux img hash: %s / %s\n", hash, sysreg->linux_sum );
            // system( unmount_command );
            return; 
        }
        space_required += zdj_fs_get_filesize( path );
    }
    if( sysreg->installer_props.has_rootfs ) {
        sprintf( path, "/mnt/update/%s", sysreg->rootfs_img );
        // zdj_put_file_hash( path, hash );
        zdj_put_crc32_file_hash( path, hash );
        if( strcmp( hash, sysreg->rootfs_sum ) ) {
            // Show error dialog if chacksums mismatch
            printf( "mismatching rootfs img hash: %s / %s\n", hash, sysreg->rootfs_sum );
            // system( unmount_command );
            return; 
        }
        space_required += zdj_fs_get_filesize( path );
    }

    if( (space_required * 2.5) > zdj_fs_get_free_media_space( ) ) {
        printf( "Not enough free space to install %1.1f Mb.\n", space_required );
        // system( unmount_command );
        return;
    }

    _os_panel_state->has_valid_image = true;
    _os_panel_state->sysreg = sysreg;
    _os_panel_state->needs_layout_update = true;

    _unmount_reset_partitions( );

    printf( "check reset image done\n" );
}

static void _install_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * install_btn_state = (zdj_menu_item_view_state_t*)view->state;
    
    char str[32];
    sprintf( str, "install DriftOS %s.%s.%s r%s", 
        _os_panel_state->sysreg->version_major,
        _os_panel_state->sysreg->version_minor,
        _os_panel_state->sysreg->version_hotfix,
        _os_panel_state->sysreg->version_build
    );
    // Launch startup confirm dialog
    zdj_view_t * dialog = zdj_new_dialog_view( 
        ZDJ_DIALOG_VIEW_TYPE_OKAY_CANCEL,
        "Confirm",
        "Are you sure you want to",
        str
    );
    zdj_dialog_view_state_t * dialog_state = (zdj_dialog_view_state_t*)dialog->state;
    dialog_state->handle_dialog_exit = _install_dialog_exit;
    dialog_state->selection_data = view;

    // printf( "dialog: %p, %p, %p\n", dialog, dialog_state->handle_dialog_exit, dialog_state->selection_data );

    // zdj_push_subview( zdj_root_view( ), dialog, true );
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_push_subview( panel_state->settings_panel, dialog, true );
}

static void _install_dialog_exit( zdj_view_t * view, void * data, bool selection ) {
    printf( "_install_dialog_exit: %d\n", selection );
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    if( selection == true ) {
        if( !_mount_reset_partitions( _os_panel_state->reset_image_path ) ) {
            _unmount_reset_partitions( );
            return;
        }

        // Push installer progross view behind dialog
        zdj_view_t * install_view = zdj_new_settings_os_install_view( "/mnt/update", _os_panel_state->sysreg );
        zdj_push_subview_behind( 
            panel_state->settings_panel,
            zdj_view_stack_top_subview_of( panel_state->settings_panel ),
            install_view,
            true
        );
    }
    zdj_pop_subview_of( panel_state->settings_panel, true );
    // printf( "_startup_exit done\n" );
}

static bool _mount_reset_partitions( char * img_path ) {
    printf( "_mount_reset_partitions: %s\n", img_path );
    // Mount selected image
    char mount_command[ 256 ];
    sprintf( mount_command, "/root/boot/mount_reset_partition.sh %s", img_path );
    system( mount_command );

    // Look for sysreg on mounted volume to confirm success
    if( access( "/mnt/update/sysreg", F_OK ) != 0 ) { 
        printf( "failed to find mounted update volume\n" );
        return false; 
    } else {
        return true;
    }
}

static void _unmount_reset_partitions( void ) {
    // Unmount reset image
    system( "/root/boot/unmount_reset_partition.sh" );
}