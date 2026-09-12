#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <uuid.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/log/zdj_log.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/system/usb/zdj_usb.h>

static pthread_t _zdj_usb_shell_gs0_thread;
static pthread_t _zdj_usb_shell_gs1_thread;
static void * _zdj_usb_shell_thread_main( void * arg );

// We're launching geTTY from its own thread to avoid
// creating a copy of the much heavier UI thread when
// forking. The thread is basically disposable.
void zdj_usb_shell_launch( void ) {
    pthread_create( &_zdj_usb_shell_gs0_thread, NULL, _zdj_usb_shell_thread_main, "ttyGS0" );
    pthread_create( &_zdj_usb_shell_gs1_thread, NULL, _zdj_usb_shell_thread_main, "ttyGS1" );
}

static void * _zdj_usb_shell_thread_main( void * arg ) {
    char * tty = (char*)arg;
    pid_t pid = fork( );
    if ( pid < 0 ) { return NULL; }

    //////////////////////////
    // Child Process        //
    // Fork geTTY from here //
    //////////////////////////
    if ( pid == 0 ) {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Launching geTTY: %s", tty );

        if ( setsid( ) < 0 ) {
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_ERROR, "geTTY %s failed to set sid", tty );
            exit( 1 );
        }

        // 2. Open the target TTY explicitly to make it the controlling terminal
        char dev[ 64 ];
        snprintf( dev, sizeof( dev ), "/dev/%s", tty );
        int fd = open( dev, O_RDWR );
        if (fd < 0) {
            perror("Failed to open TTY");
            exit(1);
        }
        ioctl(fd, TIOCSCTTY, 1);
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);
        if (fd > 2) {
            close(fd);
        }

        char * const argv[] = { "getty", "-L", "115200", tty, "vt100", NULL };
        char * const envp[] = { NULL };
        if( execve( "/sbin/getty", argv, envp ) == -1 ){
            // Linux didn't think it could launch geTTY
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_ERROR, "geTTY %s failed", tty );
            exit( 1 );
        } else {
            // geTTY has exited
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_ERROR, "geTTY %s exited", tty );
        }
    }
    
    int status;
    waitpid( pid, &status, 0 ); 
    return NULL;
}

bool zdj_usb_shell_is_running( void ) {
    char str[ 128 ];
    char res[ 128 ];
    strcpy( str, "pidof -s getty" );
    zdj_fs_get_popen( str, res );
    if( strlen( res ) < 1 ) { return false; } 
    else { return true; }
}