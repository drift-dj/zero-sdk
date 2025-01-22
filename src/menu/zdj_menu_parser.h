#ifndef ZDJ_MENU_PARSER_H
#define ZDJ_MENU_PARSER_H

#include <libxml/tree.h>
#include <libxml/parser.h>

#include "zdj_menu.h"

int zdj_menu_parse_xml( char * path );

#endif