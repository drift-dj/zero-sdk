#ifndef ZDJ_XML_H
#define ZDJ_XML_H

int zdj_xml_count_names_in_node( xmlNode * node, char * name );
xmlNode * zdj_xml_child_node_named( xmlNode * node, char * child_node_name );

#endif