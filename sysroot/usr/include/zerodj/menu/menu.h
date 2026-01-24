#pragma once

#define C_BOLD  "\e[1m"
#define C_UL "\e[4m"
#define C_WHITE  "\033[0;37m"
#define C_RED  "\033[0;31m"
#define C_BLUE  "\033[0;34m"
#define C_YELLOW  "\033[0;33m"
#define C_CYAN  "\033[0;36m"
#define C_GREEN  "\033[0;32m"
#define C_PURPLE  "\033[0;35m"
#define C_GRAY "\e[38;5;241m"

#define C_OFF   "\e[m"


typedef enum {
    MENU_EXIT_NONE,
    MENU_EXIT_QUIT,
    MENU_EXIT_RUN_WORKFLOW,
    MENU_EXIT_RESET_BUILD_DATA
} menu_exit_cmd_t;

extern menu_exit_cmd_t menu_exit_cmd;

void print_menu_header( char * title );
void print_quit_option( void );
void print_back_option( void );
void print_input_prompt( void );
void print_input_error( char input );

char get_menu_input( void );
char * get_menu_input_string( void );

void show_main_menu( void );
void show_workflows_menu( void );
void show_m7_menu( void );
void show_apps_menu( void );
void show_app_menu( int app_num );
void show_registry_menu( void );
void show_os_menu( void );
void show_device_menu( void );
void show_sdk_menu( void );
void show_system_menu( void );

void run_current_workflow( void );
void reset_build_data( void );
void menu_quit( void );