#ifndef ZDJ_FILE_BROWSER_VIEW_H
#define ZDJ_FILE_BROWSER_VIEW_H

typedef enum {
    ZDJ_FILE_BROWSER_SELECT_TYPE_DIR,
    ZDJ_FILE_BROWSER_SELECT_TYPE_FILE
} zdj_file_browser_select_type_t;

typedef enum {
    ZDJ_FILE_BROWSER_EXIT_STATUS_CANCEL,
    ZDJ_FILE_BROWSER_EXIT_STATUS_SELECT
} zdj_file_browser_exit_status_t;

typedef struct {
    zdj_file_browser_exit_status_t status;
    char * path;
} zdj_file_browser_exit_context_t;

typedef struct {
    zdj_view_t * header_view;
    zdj_view_t * menu_stack;
    void ( *handle_file_browser_exit )( zdj_view_t *, zdj_file_browser_exit_context_t * );
    bool ( *file_validator )( char * );
    bool read_only;
    bool allow_nav;
    zdj_file_browser_select_type_t select_type;
} zdj_file_browser_view_state_t;

zdj_view_t * zdj_new_file_browser_view( 
    zdj_rect_t * frame, 
    char * path,
    bool read_only,
    bool allow_nav,
    zdj_file_browser_select_type_t select_type 
);

void zdj_file_browser_item_hmi_delegate( zdj_view_t * view, void * _event );

zdj_view_t * zdj_new_file_browser_menu_for_path( 
    zdj_view_t * browser,
    zdj_rect_t * frame, 
    char * path, 
    bool read_only,
    bool allow_nav,
    zdj_file_browser_select_type_t select_type 
);
zdj_view_t * zdj_new_device_browser_menu( zdj_view_t * browser, zdj_rect_t * frame );

#endif