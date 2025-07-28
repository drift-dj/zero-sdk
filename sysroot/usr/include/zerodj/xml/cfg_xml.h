#ifndef CFG_XML_H
#define CFG_XML_H

int cfg_menu_parse_xml( char * path );
zdj_view_t *  cfg_new_menu_view_for_xml_name( 
    char * xml_menu_name,
    zdj_rect_t * frame 
);

#endif