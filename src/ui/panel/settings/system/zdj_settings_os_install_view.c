#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/reboot.h>
// #include <sys/wait.h>
#include <pthread.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/zdj_data_type.h>
#include <zerodj/controls/zdj_controls.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/hash/zdj_hash.h>
#include <zerodj/system/installer/zdj_installer.h>
#include <zerodj/system/registry/zdj_registry.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/panel/settings/zdj_settings_panel.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/dialog_view/zdj_dialog_view.h>
#include <zerodj/ui/view/file_browser_view/zdj_file_browser_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/progress_bar_view/zdj_progress_bar_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

typedef enum {
    SETTINGS_OS_INSTALL_PHASE_INIT,
    SETTINGS_OS_INSTALL_PHASE_VERIFY_UPDATES,
    SETTINGS_OS_INSTALL_PHASE_BACKUP_BOOTLOADER,
    SETTINGS_OS_INSTALL_PHASE_BACKUP_LINUX,
    SETTINGS_OS_INSTALL_PHASE_BACKUP_ROOTFS,
    SETTINGS_OS_INSTALL_PHASE_COPY_BOOTLOADER,
    SETTINGS_OS_INSTALL_PHASE_COPY_LINUX,
    SETTINGS_OS_INSTALL_PHASE_COPY_ROOTFS,
    SETTINGS_OS_INSTALL_PHASE_ERROR_REVERT,
    SETTINGS_OS_INSTALL_PHASE_SUCCESS,
    SETTINGS_OS_INSTALL_PHASE_ERROR,
    SETTINGS_OS_INSTALL_PHASE_DONE
} settings_os_install_phase_t;

typedef struct {
    volatile settings_os_install_phase_t phase;
    char err_str[128];
    zdj_view_t * phase_label;
    zdj_view_t * error_label;
    zdj_os_sysreg_t * sysreg;
    bool view_needs_refresh;
    pthread_t update_thread;
} settings_os_install_state_t;

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void * _install_thread_main( void * arg );

static void _handle_control( zdj_view_t * view, zdj_control_event_t * event );

static void _success_exit( zdj_view_t * view, void * data, bool selection );
static void _error_exit( zdj_view_t * view, void * data, bool selection );

zdj_view_t * zdj_new_settings_os_install_view( char * mount_path, zdj_os_sysreg_t * sysreg ) {
    // printf( "zdj_new_settings_os_install_view: %s / %s\n", sysreg->version_build_desc, mount_path );
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_draw;
    view->handle_control_event = _handle_control;
    view->map = ZDJ_CONTROL_MAP_SETTINGS_PANEL;

    settings_os_install_state_t * state = calloc( 1, sizeof( settings_os_install_state_t ) );
    state->sysreg = sysreg;
    state->view_needs_refresh = true;
    view->state = state;

    // Add progress bar
    zdj_view_t * progress_bar = zdj_new_progress_bar_view( &(zdj_rect_t){ 0,0,view->frame.w,6 }, ZDJ_PROGRESS_BAR_VIEW_WAIT );
    zdj_add_subview( view, progress_bar );

    // Add version labels
    zdj_view_t * installing_label = zdj_new_label_view( "Installing", ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    installing_label->frame.y = 10;
    zdj_add_subview( view, installing_label );

    zdj_view_t * desc_label = zdj_new_label_view( sysreg->version_build_desc, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    desc_label->frame.y = 18;
    zdj_add_subview( view, desc_label );

    char v_str[ 10 ];
    sprintf( v_str, "%s.%s.%s", sysreg->version_major, sysreg->version_minor, sysreg->version_hotfix );
    zdj_view_t * version_label = zdj_new_label_view( v_str, ZDJ_FONT_12_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    version_label->frame.x = view->frame.w - version_label->frame.w;
    version_label->frame.y = 11;
    zdj_add_subview( view, version_label );

    // Add phase label
    state->phase_label = zdj_new_label_view( "Initializing...", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    state->phase_label->frame.y = view->frame.h - 10;
    zdj_add_subview( view, state->phase_label );

    // Spawn update thread
    pthread_create( &state->update_thread, NULL, _install_thread_main, state );

    return view;     
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    settings_os_install_state_t * state = (settings_os_install_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( state->view_needs_refresh ) { 
        zdj_remove_subview_of( view, state->phase_label );
        if( state->error_label ) { zdj_remove_subview_of( view, state->error_label ); }
        char phase_str[ 64 ];
        switch ( state->phase ) {
            case SETTINGS_OS_INSTALL_PHASE_INIT: strcpy( phase_str, "Verifying Updates..." ); break;
            case SETTINGS_OS_INSTALL_PHASE_BACKUP_BOOTLOADER: strcpy( phase_str, "Backing Up U-Boot..." ); break;
            case SETTINGS_OS_INSTALL_PHASE_BACKUP_LINUX: strcpy( phase_str, "Backing Up Linux..." ); break;
            case SETTINGS_OS_INSTALL_PHASE_BACKUP_ROOTFS: strcpy( phase_str, "Backing Up RootFS..." ); break;
            case SETTINGS_OS_INSTALL_PHASE_COPY_BOOTLOADER: strcpy( phase_str, "Updating U-Boot..." ); break;
            case SETTINGS_OS_INSTALL_PHASE_COPY_LINUX: strcpy( phase_str, "Updating Linux..." ); break;
            case SETTINGS_OS_INSTALL_PHASE_COPY_ROOTFS: strcpy( phase_str, "Updating RootFS..." ); break;
            case SETTINGS_OS_INSTALL_PHASE_ERROR_REVERT: strcpy( phase_str, "Error! Reverting..." ); break;
            case SETTINGS_OS_INSTALL_PHASE_ERROR: strcpy( phase_str, "Error!" ); break;
            case SETTINGS_OS_INSTALL_PHASE_DONE: strcpy( phase_str, "Done" ); break;
        }
        state->phase_label = zdj_new_label_view( phase_str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        state->phase_label->frame.y = view->frame.h - 11;
        zdj_add_subview( view, state->phase_label );

        if( state->phase == SETTINGS_OS_INSTALL_PHASE_ERROR_REVERT ||
            state->phase == SETTINGS_OS_INSTALL_PHASE_ERROR 
        ) {
            state->phase_label->frame.y = view->frame.h - 20;

            state->error_label = zdj_new_label_view( state->err_str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
            state->error_label->frame.y = view->frame.h - 11;
            zdj_add_subview( view, state->error_label );
        }

        state->view_needs_refresh = false;
    }

    if( state->phase == SETTINGS_OS_INSTALL_PHASE_SUCCESS ) {
        // Show success dialog
        state->phase = SETTINGS_OS_INSTALL_PHASE_DONE;
        zdj_view_t * dialog = zdj_new_dialog_view( 
            ZDJ_DIALOG_VIEW_TYPE_OKAY,
            "Confirm",
            "Installation Complete.",
            "Reboot now."
        );
        zdj_dialog_view_state_t * dialog_state = (zdj_dialog_view_state_t*)dialog->state;
        dialog_state->handle_dialog_exit = _success_exit;
        dialog_state->selection_data = view;
        zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
        zdj_push_subview( panel_state->settings_panel, dialog, true );
    
    } else if( state->phase == SETTINGS_OS_INSTALL_PHASE_ERROR ) {
        // Show error dialog
        state->phase = SETTINGS_OS_INSTALL_PHASE_DONE;
        zdj_view_t * dialog = zdj_new_dialog_view( 
            ZDJ_DIALOG_VIEW_TYPE_OKAY,
            "Confirm",
            "Installation Failed.",
            state->err_str
        );
        zdj_dialog_view_state_t * dialog_state = (zdj_dialog_view_state_t*)dialog->state;
        dialog_state->handle_dialog_exit = _error_exit;
        dialog_state->selection_data = view;
        zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
        zdj_push_subview( panel_state->settings_panel, dialog, true );
    }
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    printf( "os install _handle_control\n" );
    // Ignore events which have been blocked by layers above this one.
    if( _event->blocked ) { return; }
    _event->blocked = true;
    printf( "os install _handle_control done\n" );
}

static void _success_exit( zdj_view_t * view, void * data, bool selection ) {
    reboot( RB_AUTOBOOT );
}

static void _error_exit( zdj_view_t * view, void * data, bool selection ) {
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_pop_n_subviews_of( panel_state->settings_panel, 3, true );
}


static void * _install_thread_main( void * arg ) {
    settings_os_install_state_t * state = (settings_os_install_state_t*)arg;


    char path[ 256 ];
    char hash[ ZDJ_HASH_LEN ];

    // Match hashes of reset image files against sysreg
    if( state->sysreg->installer_props.has_bootloader ) {
        sprintf( path, "/mnt/update/%s", state->sysreg->bootloader_img );
        // zdj_put_file_hash( path, hash );
        zdj_put_crc32_file_hash( path, hash );
        if( !zdj_hashes_match( hash, state->sysreg->bootloader_sum ) ) {
            printf( "mismatching bootloader img hash: %s / %s\n", hash, state->sysreg->bootloader_sum );
            state->phase = SETTINGS_OS_INSTALL_PHASE_ERROR;
            strcpy( state->err_str, "Bootloader Image Checksum Failed." );
            state->view_needs_refresh = true;
            return NULL;
        }
    }
    if( state->sysreg->installer_props.has_linux ) {
        sprintf( path, "/mnt/update/%s", state->sysreg->linux_img );
        // zdj_put_file_hash( path, hash );
        zdj_put_crc32_file_hash( path, hash );
        if( !zdj_hashes_match( hash, state->sysreg->linux_sum ) ) {
            printf( "mismatching reset linux img hash: %s / %s\n", hash, state->sysreg->linux_sum );
            state->phase = SETTINGS_OS_INSTALL_PHASE_ERROR;
            strcpy( state->err_str, "Linux Image Checksum Failed." );
            state->view_needs_refresh = true;
            return NULL;
        }
    }
    if( state->sysreg->installer_props.has_rootfs ) {
        sprintf( path, "/mnt/update/%s", state->sysreg->rootfs_img );
        // zdj_put_file_hash( path, hash );
        zdj_put_crc32_file_hash( path, hash );
        if( !zdj_hashes_match( hash, state->sysreg->rootfs_sum ) ) {
            printf( "mismatching rootfs img hash: %s / %s\n", hash, state->sysreg->rootfs_sum );
            state->phase = SETTINGS_OS_INSTALL_PHASE_ERROR;
            strcpy( state->err_str, "RootFS Image Checksum Failed." );
            state->view_needs_refresh = true;
            return NULL;
        }
    }

    printf( "reset image hashes okay\n" );
    
    // Safely backup current recovery images to internal drive
    if( state->sysreg->installer_props.has_bootloader &&
        ( access( "/mnt/recovery/bootloader.bin", F_OK ) == 0 )  
    ) {
        state->phase = SETTINGS_OS_INSTALL_PHASE_BACKUP_BOOTLOADER;
        state->view_needs_refresh = true;
        if( zdj_fs_copy_file_with_hash( "/mnt/recovery/bootloader.bin", "/media/internal/.backup/bootloader.bin", true ) > ZDJ_ERROR_OKAY ) {
            // Show error state
            printf( "failed to backup bootloader\n" );
            state->phase = SETTINGS_OS_INSTALL_PHASE_ERROR;
            strcpy( state->err_str, "Bootloader Backup Failed." );
            state->view_needs_refresh = true;
            return NULL;
        }
    }
    if( state->sysreg->installer_props.has_linux &&
        ( access( "/mnt/recovery/linux_img.gz", F_OK ) == 0 ) 
    ) {
        state->phase = SETTINGS_OS_INSTALL_PHASE_BACKUP_LINUX;
        state->view_needs_refresh = true;
        if( zdj_fs_copy_file_with_hash( "/mnt/recovery/linux_img.gz", "/media/internal/.backup/linux_img.gz", true ) > ZDJ_ERROR_OKAY ) {
            // Show error state
            printf( "failed to backup linux img\n" );
            state->phase = SETTINGS_OS_INSTALL_PHASE_ERROR;
            strcpy( state->err_str, "Linux Image Backup Failed." );
            state->view_needs_refresh = true;
            return NULL;
        }
    }
    if( state->sysreg->installer_props.has_rootfs &&
        ( access( "/mnt/recovery/rootfs_img.gz", F_OK ) == 0 ) 
    ) {
        state->phase = SETTINGS_OS_INSTALL_PHASE_BACKUP_ROOTFS;
        state->view_needs_refresh = true;
        if( zdj_fs_copy_file_with_hash( "/mnt/recovery/rootfs_img.gz", "/media/internal/.backup/rootfs_img.gz", true ) > ZDJ_ERROR_OKAY ) {
            // Show error state
            printf( "failed to backup rootfs img\n" );
            state->phase = SETTINGS_OS_INSTALL_PHASE_ERROR;
            strcpy( state->err_str, "RootFS Image Backup Failed." );
            state->view_needs_refresh = true;
            return NULL;
        }
    }
    if ( access( "/mnt/recovery/sysreg", F_OK ) == 0 ) {
        if( zdj_fs_copy_file_with_hash( "/mnt/recovery/sysreg", "/media/internal/.backup/sysreg", true ) > ZDJ_ERROR_OKAY ) {
            // Show error state
            printf( "failed to backup sysreg\n" );
            state->phase = SETTINGS_OS_INSTALL_PHASE_ERROR;
            strcpy( state->err_str, "Sysreg Backup Failed." );
            state->view_needs_refresh = true;
            return NULL;
        }
    }
    

    printf( "recovery backup okay\n" );

    printf( "updating sysreg\n" );
    // Copy old sysreg for updating
    FILE * sysreg_fd;
    zdj_os_sysreg_t * new_sysreg = calloc( 1, sizeof( zdj_os_sysreg_t ) );
    if ( access( "/mnt/recovery/sysreg", F_OK ) == 0 ) {
        sysreg_fd = fopen( "/mnt/recovery/sysreg", "r" );
        fread( new_sysreg, sizeof( zdj_os_sysreg_t ), 1, sysreg_fd );
        fclose( sysreg_fd );
    } else {
        sysreg_fd = fopen( "/mnt/update/sysreg", "r" );
        fread( new_sysreg, sizeof( zdj_os_sysreg_t ), 1, sysreg_fd );
        fclose( sysreg_fd );
    }

    printf( "overwriting uboot\n" );
    // Copy new images into place
    if( state->sysreg->installer_props.has_bootloader ) {
        state->phase = SETTINGS_OS_INSTALL_PHASE_COPY_BOOTLOADER;
        state->view_needs_refresh = true;
        // Bootloader overwrite happens in shell script -- track result code
        int res = system( "/root/boot/install_uboot.sh" );
        if ( WIFEXITED( res ) ) {
            // Get the actual exit status (0-255)
            int exit_status = WEXITSTATUS( res );
            if( exit_status > 0 ) {
                printf( "Script exited with status %d\n", exit_status );
                state->phase = SETTINGS_OS_INSTALL_PHASE_ERROR;
                strcpy( state->err_str, "Bootloader Update Failed." );
                state->view_needs_refresh = true;
                return NULL;
            }
        } else {
            // Handle cases where the process didn't exit normally (e.g., killed by a signal)
            printf( "Script terminated abnormally\n" );
            state->phase = SETTINGS_OS_INSTALL_PHASE_ERROR;
            strcpy( state->err_str, "Bootloader Update Failed." );
            state->view_needs_refresh = true;
            return NULL;
        }
        state->view_needs_refresh = true;
    }

    printf( "overwriting linux\n" );
    if( state->sysreg->installer_props.has_linux ) {
        state->phase = SETTINGS_OS_INSTALL_PHASE_COPY_LINUX;
        state->view_needs_refresh = true;
        zdj_error_type_t err = zdj_fs_copy_file_with_hash( "/mnt/update/linux_img.gz", "/mnt/recovery/linux_img.gz", true );
        if( err > ZDJ_ERROR_OKAY ) {
            // Show error state
            printf( "failed to copy new linux img: %d\n", err );
            state->phase = SETTINGS_OS_INSTALL_PHASE_ERROR;
            strcpy( state->err_str, "Linux Update Failed." );
            state->view_needs_refresh = true;
            return NULL;
        } else {
            strcpy( new_sysreg->linux_sum, state->sysreg->linux_sum );
            FILE * update_flag_fd = fopen( "/mnt/recovery/update_li", "w" );
            fwrite( "true", sizeof( char ), 5, update_flag_fd );
            fclose( update_flag_fd );
        }
        
    }

    printf( "overwriting rootfs\n" );
    if( state->sysreg->installer_props.has_rootfs ) {
        state->phase = SETTINGS_OS_INSTALL_PHASE_COPY_ROOTFS;
        state->view_needs_refresh = true;
        zdj_error_type_t err = zdj_fs_copy_file_with_hash( "/mnt/update/rootfs_img.gz", "/mnt/recovery/rootfs_img.gz", true );
        if( err > ZDJ_ERROR_OKAY ) {
            // Show error state
            printf( "failed to copy new rootfs img\n" );
            state->phase = SETTINGS_OS_INSTALL_PHASE_ERROR;
            strcpy( state->err_str, "RootFS Update Failed." );
            state->view_needs_refresh = true;
            return NULL;
        } else {
            strcpy( new_sysreg->rootfs_sum, state->sysreg->rootfs_sum );
            FILE * update_flag_fd = fopen( "/mnt/recovery/update_fs", "w" );
            fwrite( "true", sizeof( char ), 5, update_flag_fd );
            fclose( update_flag_fd );
        }
    }

    printf( "recovery overwrite okay\n" );

    // TODO unwind all copies in event of failure in any copy
    


    // Update Sysreg
    FILE * sysreg_out_fd = fopen( "/mnt/recovery/sysreg", "w" );
    fwrite( new_sysreg, sizeof( zdj_os_sysreg_t ), 1, sysreg_out_fd );
    fclose( sysreg_out_fd );

    FILE * etcreg_out_fd = fopen( ZDJ_REGISTRY_OS_SYSREG_PATH, "w" );
    fwrite( new_sysreg, sizeof( zdj_os_sysreg_t ), 1, etcreg_out_fd );
    fclose( etcreg_out_fd );
    free( new_sysreg );

    // Unmount partitions
    system( "/root/boot/unmount_reset_partition.sh" );

    // Show reboot required dialog
    state->phase = SETTINGS_OS_INSTALL_PHASE_SUCCESS;
    state->view_needs_refresh = true;

    return NULL;
}