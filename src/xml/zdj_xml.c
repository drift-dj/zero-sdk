#include <stdio.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <zerodj/xml/zdj_xml.h>

int zdj_xml_count_names_in_node( xmlNode * node, char * name ) {
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

xmlNode * zdj_xml_child_node_named( xmlNode * node, char * child_node_name ) {
if ( ( !xmlStrcmp( node->name, (const xmlChar *)child_node_name ) ) ) {
        return node;
    } else {
        if ( node->xmlChildrenNode ) {
            xmlNode * child_node;
            if( (child_node = zdj_xml_child_node_named( node->xmlChildrenNode, child_node_name )) ) {
                return child_node;
            }
        }
        if( node->next ) { return zdj_xml_child_node_named( node->next, child_node_name ); }
    }
    return NULL;
}