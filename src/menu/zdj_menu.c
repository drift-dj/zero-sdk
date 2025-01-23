#include <zerodj/menu/zdj_menu.h>
#include <zerodj/menu/zdj_menu_parser.h>

char ** zdj_menu_name_map;
zdj_menu_t ** zdj_menus;

// Parse contents of an xml file into in-memory menu structures
void zdj_menu_load_data( char * path ) {
    zdj_menu_parse_xml( path );
}

// Retrieve a reference to a zdj_menu_t object using name field in xml
zdj_menu_t * zdj_menu_get_by_name( char * name ) {
    return NULL;
}

// Return a string for text-to-speech screen-reader system
char * zdj_menu_get_speech_str_for_item( zdj_menu_item_t * item ) {
    return NULL;
}