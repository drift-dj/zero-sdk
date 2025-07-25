// Copyright (c) 2025 Drift DJ Industries

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ZDJ_XML_H
#define ZDJ_XML_H

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>

// Linkage functions to be implemented by front-end.
// During menu creation, the XML menu system will reach
// out to the front-end for some linkage reference based
// on field values in the individual item's XML node.
typedef zdj_menu_item_view_layout_t (*xml_layout_linkage)( char* );
typedef zdj_menu_item_view_action_t (*xml_action_linkage)( char* );
typedef void (*xml_update_linkage)( char* );

int zdj_xml_count_names_in_node( xmlNode * node, char * name );
xmlNode * zdj_xml_child_node_named( xmlNode * node, char * child_node_name );

int zdj_xml_menu_parse( char * path );
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
);

#endif