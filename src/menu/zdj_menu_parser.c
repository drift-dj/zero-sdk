#include <stdbool.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "zdj_menu_parser.h"
#include "zdj_menu.h"

int _zdj_menu_count_names_in_node( xmlNode * node, char * name ) {
    int count = 0;
    xmlNode * child = node->xmlChildrenNode;
    while ( child != NULL && count<4096 ) {
		if ( ( !xmlStrcmp( child->name, (const xmlChar *)name ) ) ) {
            count++;
		}
        child = child->next;
	}
    return count;
}

xmlNode * _zdj_menu_child_node_named( xmlNode * node, char * child_node_name ) {
if ( ( !xmlStrcmp( node->name, (const xmlChar *)child_node_name ) ) ) {
        return node;
    } else {
        if ( node->xmlChildrenNode ) {
            xmlNode * child_node;
            if( (child_node = _zdj_menu_child_node_named( node->xmlChildrenNode, child_node_name )) ) {
                return child_node;
            }
        }
        if( node->next ) { return _zdj_menu_child_node_named( node->next, child_node_name ); }
    }
    return NULL;
}

zdj_menu_t * zdj_menu_for_library_node( xmlNode * node ) {
    return NULL;
}

zdj_menu_t * zdj_menu_for_menu_data_node( xmlNode * node ) {
    return NULL;
}

zdj_menu_t * zdj_menu_for_xml( xmlNode * node ) {
    zdj_menu_t * menu = malloc( sizeof( zdj_menu_t ) );
    xmlNode * child_node;
    // Set menu header bar type
    // Set up item counts
    // Init menu's animation instance
    
    // If this is a library category menu, use lib to build menu
    if( ( child_node = _zdj_menu_child_node_named( node, "LIBRARY" ) ) ) {
        return zdj_menu_for_library_node( child_node );
    } else if( (child_node = _zdj_menu_child_node_named( node, "MENU_DATA" ) ) ) { 
        // Otherwise, parse XML groups and items into menu
        return zdj_menu_for_menu_data_node( child_node );
    }

    // Figure out y values in menu
}

int zdj_menu_parse_xml( char * path ) {
    xmlDoc * _doc = NULL;
    xmlNode * _xml_menus = NULL;
    xmlNode * _cur = NULL;

    // Do some initial error checking
    _doc = xmlParseFile( path );
    if ( _doc == NULL ) { 
        return 1; // fail if file doesn't parse
    }
    _xml_menus = xmlDocGetRootElement( _doc );
    if ( xmlStrcmp( _xml_menus->name, (const xmlChar *)"MENUS" ) ) {
        return 1; // fail if MENUS node not present
    }

    // Alloc name mapping and menu storage
    int menu_count = _zdj_menu_count_names_in_node( _xml_menus, "MENU" );
    if( menu_count < 1 ) {
        return 1; // fail if nozdj_menus are found
    }
    zdj_menu_name_map = malloc( sizeof( char * ) * menu_count );
    zdj_menus = malloc( sizeof( zdj_menu_t * ) * menu_count );

    // Build menu name mapping + individual zdj_menus
    int menu_index = 0;
    _cur = _xml_menus->xmlChildrenNode;
	while ( _cur != NULL ) {
		if ( ( !xmlStrcmp( _cur->name, (const xmlChar *)"MENU" ) ) ) {
            char * menu_name = (char*)xmlGetProp( _cur, (const xmlChar *)"menu_name" );
            // Make mapping
            zdj_menu_name_map[ menu_index ] = menu_name;
            // Make menu instance
            zdj_menu_t * menu;
            xmlNode * child_node;
            if( ( child_node = _zdj_menu_child_node_named( _cur, "LIBRARY" ) ) ) {
                menu = zdj_menu_for_library_node( child_node );
            } else if( (child_node = _zdj_menu_child_node_named( _cur, "MENU_DATA" ) ) ) { 
                // Otherwise, parse XML groups and items into menu
                menu = zdj_menu_for_menu_data_node( child_node );
            }
            // Check menu instance before adding it to the mapping
            if( menu ) {
               zdj_menus[ menu_index ] = menu;
            } else {
               zdj_menus[ menu_index ] = NULL;
                printf( "menu parse error for: %s]n", menu_name );
            }
            menu_index++;
		}
        _cur = _cur->next;
	}

    return 0;
}
