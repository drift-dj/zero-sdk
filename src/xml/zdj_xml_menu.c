#include <stdio.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/xml/zdj_xml.h>

static char ** _xml_menu_name_map;
static xmlNode ** _xml_menu_nodes;
static int _xml_menu_node_count;

static void _xml_menu_process_section_nodes( 
    xmlNode * node, 
    zdj_view_t * menu_view,
    xml_layout_linkage layout_linkage,
    xml_action_linkage action_linkage,
    xml_update_linkage update_linkage,
    handle_hmi_event_t handle_hmi_event 
);
static void _xml_menu_process_item_nodes( 
    xmlNode * node, 
    zdj_view_t * menu_view,
    xml_layout_linkage layout_linkage,
    xml_action_linkage action_linkage,
    xml_update_linkage update_linkage,
    handle_hmi_event_t handle_hmi_event 
);
static xmlNode * _xml_node_for_name( char * name );


int zdj_xml_menu_parse( char * path ) {
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
    int menu_count = zdj_xml_count_names_in_node( _xml_menus, "MENU" );
    if( menu_count < 1 ) {
        return 1; // fail if no menus are found
    }

    // Build menu name mapping + pre-parsed menu xmlNodes
    _xml_menu_name_map = malloc( sizeof( char * ) * menu_count );
    _xml_menu_nodes = malloc( sizeof( xmlNode * ) * menu_count );
    _xml_menu_node_count = menu_count;
    int menu_index = 0;
    _cur = _xml_menus->xmlChildrenNode;
	while ( _cur != NULL ) {
		if ( ( !xmlStrcmp( _cur->name, (const xmlChar *)"MENU" ) ) ) {
            char * menu_name = (char*)xmlGetProp( _cur, (const xmlChar *)"menu_name" );
            // Make mapping
            _xml_menu_name_map[ menu_index ] = menu_name;
            _xml_menu_nodes[ menu_index ] = _cur;
            menu_index++;
		}
        _cur = _cur->next;
	}

    return 0;
}

// Build a zdj_view_t for a pre-parsed XML <MENU> node where 
// menu_name == given string.
// Linkage is passed into each menu_item on creation, enabling
// front-end-defined behavior per-item.
// Note that we re-parse the <MENU> node every time zdj_new_menu_view
// is called, so it's best to store the view after the first call
// rather than calling new_menu_view every time a menu is needed.
zdj_view_t * zdj_xml_menu_view_for_node( 
    char * xml_menu_name, 
    zdj_rect_t * frame,
    char * title,
    bool has_back_btn,
    zdj_menu_view_header_back_btn_fn handle_back,
    xml_layout_linkage layout_linkage,
    xml_action_linkage action_linkage,
    xml_update_linkage update_linkage,
    handle_hmi_event_t handle_hmi_event
) {
    // Ensure front-end linkage is supplied
    if( !layout_linkage || !action_linkage || !update_linkage ) { return NULL; }

    // Get a reference to the pre-parsed xmlNode.
    xmlNode * node = _xml_node_for_name( xml_menu_name );
    if( !node ) { return NULL; }

    // Make a menu_view to hold sections/items
    zdj_view_t * menu_view = NULL;

    // Add menu_item_views based on xmlNode.
    xmlNode * menu_data_node = zdj_xml_child_node_named( node, "MENU_DATA" );
    if( !menu_data_node ) { return NULL; }

    // Data may be nested under sections or a single array of items
    xmlNode * section_node = zdj_xml_child_node_named( menu_data_node, "SECTION" );
    xmlNode * items_node = zdj_xml_child_node_named( menu_data_node, "ITEMS" );

    if( section_node ) {
        menu_view = zdj_new_menu_view( ZDJ_VERTICAL, frame );
        _xml_menu_process_section_nodes( 
            section_node, 
            menu_view,
            layout_linkage,
            action_linkage,
            update_linkage,
            handle_hmi_event
        );
    } else if( items_node ) {
        menu_view = zdj_new_menu_view( ZDJ_VERTICAL, frame );
        _xml_menu_process_item_nodes( 
            items_node->xmlChildrenNode, 
            menu_view,
            layout_linkage,
            action_linkage,
            update_linkage,
            handle_hmi_event
        );
    }

    if( menu_view ) {
        // Add a menu header if string is available
        char * menu_title;
        xmlNode * menu_title_node = zdj_xml_child_node_named( node, "TITLE" );
        if( menu_title_node ) {
            menu_title = (char*)xmlGetProp( menu_title_node, (const xmlChar *)"header" );
        }
        
        // Set back btn presence
        zdj_menu_view_header_back_style_t back_style = ( has_back_btn ) ? ZDJ_MENU_HEADER_BACK_STYLE_NONE : ZDJ_MENU_HEADER_BACK_STYLE_BACK;
        zdj_view_t * menu_header = zdj_new_menu_header( 
            title,
            menu_title,
            ZDJ_MENU_HEADER_STYLE_NORMAL,
            back_style
        );
        zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
        header_state->handle_back = handle_back;
        zdj_menu_view_add_header( menu_view, menu_header );
    }

    return menu_view;
}

void _xml_menu_process_section_nodes( 
    xmlNode * node, 
    zdj_view_t * menu_view,
    xml_layout_linkage layout_linkage,
    xml_action_linkage action_linkage,
    xml_update_linkage update_linkage,
    handle_hmi_event_t handle_hmi_event 
) {
    while( node ) {
        if( !xmlStrcmp( node->name, (const xmlChar *)"SECTION" ) ) {
            // First add a section to the menu
            char * section_title = (char*)xmlGetProp( node, (const xmlChar *)"title" );

            zdj_view_t * section = zdj_new_menu_section( section_title );
            zdj_menu_section_view_state_t * state = (zdj_menu_section_view_state_t*)section->state;
            
            zdj_menu_view_add_section( menu_view, section );

            // printf( "adding section: %s, %p\n", section_title, section );

            // Then add all items to the menu
            xmlNode * child = node->xmlChildrenNode;
            xmlNode * items = zdj_xml_child_node_named( node, "ITEMS" );
            if( items ) {
                _xml_menu_process_item_nodes( 
                    items->xmlChildrenNode, 
                    menu_view,
                    layout_linkage,
                    action_linkage,
                    update_linkage,
                    handle_hmi_event
                );
            }
        }
        node = node->next;
    }
}

void _xml_menu_process_item_nodes( 
    xmlNode * node, 
    zdj_view_t * menu_view,
    xml_layout_linkage layout_linkage,
    xml_action_linkage action_linkage,
    xml_update_linkage update_linkage,
    handle_hmi_event_t handle_hmi_event
) {
    while( node ) {
        if( !xmlStrcmp( node->name, (const xmlChar *)"ITEM" ) ) {
            char * item_title = (char*)xmlGetProp( node, (const xmlChar *)"title" );

            // Static layout/hmi -- no front-end linkage
            zdj_view_t * item = zdj_new_menu_item( item_title );
            zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)item->state;
            
            // Setup layout type
            zdj_menu_item_view_layout_t layout;
            if( layout_linkage ) {
                char * layout_str = (char*)xmlGetProp( node, (const xmlChar *)"layout" );
                layout = layout_linkage( layout_str );
                if( layout ) {
                    state->update_layout = zdj_menu_item_update_for_layout( layout );
                    // Add linkage for data layout type
                    if( zdj_menu_item_layout_is_dynamic( layout ) ) {
                        // Set the update_data func
                        state->update_data = update_linkage( state->link );
                    }
                }
            }
            
            // Capture link string
            state->link = strdup((char*)xmlGetProp( node, (const xmlChar *)"link" ));

            // Find hmi_event action type
            char * action_str = (char*)xmlGetProp( node, (const xmlChar *)"action" );
            state->action = action_linkage( action_str );

            // Handle hmi event
            item->handle_hmi_event = handle_hmi_event;
            
            // Insert into menu
            zdj_menu_view_add_item( menu_view, item );
        }
        node = node->next;
    }
}

// Search the node mapping for a string match with name.
// Return the associated xmlNode created during parse_xml.
xmlNode * _xml_node_for_name( char * name ) {
    for( int i=0; i<_xml_menu_node_count; i++ ) {
        if( !strcmp( _xml_menu_name_map[ i ], name ) ) {
            return _xml_menu_nodes[ i ];
        }
    }
    return NULL;
}