#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.h>
#include <zerodj/signal/pipeline/node/audio/io/zdj_io_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/zdj_soundcard_dto.h>
#include <zerodj/system/sql/zdj_sql.h>

zdj_soundcard_node_t * zdj_soundcard_create_node( zdj_soundcard_node_name_t name ) {
    zdj_soundcard_node_t * node = calloc( 1, sizeof( zdj_soundcard_node_t ) );
    node->name = name;

    // printf( "zdj_soundcard_create_node: %s\n", zdj_soundcard_node_name[ node->name ] );

    // Get props from the dto
    node->link_map = zdj_soundcard_dto_get_linkmap_for_node_name( &zdj_soundcard->dto, name );
    node->signal_type = zdj_soundcard_dto_get_sigtype_for_node_name( &zdj_soundcard->dto, name );
    node->gain = zdj_soundcard_dto_get_gain_for_node_name( &zdj_soundcard->dto, name );
    node->pan = zdj_soundcard_dto_get_pan_for_node_name( &zdj_soundcard->dto, name );
    node->stereo = zdj_soundcard_dto_get_stereo_for_node_name( &zdj_soundcard->dto, name );
    node->mute = zdj_soundcard_dto_get_mute_for_node_name( &zdj_soundcard->dto, name );
    node->source = zdj_soundcard_dto_get_source_for_node_name( &zdj_soundcard->dto, name );
    node->val = zdj_soundcard_dto_get_val_for_node_name( &zdj_soundcard->dto, name );
    node->invert = zdj_soundcard_dto_get_invert_for_node_name( &zdj_soundcard->dto, name );

    // Add a buffer if applicable
    if( zdj_soundcard_node_name_has_buffer( name ) ) {
        node->buffer = zdj_new_audio_buffer_node( zdj_soundcard->buffer_len, node->stereo+1 );
    }

    // Process the linkmap into links
    zdj_soundcard_pull_node_links_from_dto( zdj_soundcard, node );

    node->next = NULL;
    node->prev = NULL;
    return node;
}

zdj_error_type_t zdj_soundcard_install_node( 
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node 
) {
    // If there are no nodes, link the new one at the root.
    if( !soundcard->nodes ) {
        soundcard->nodes = node;
    } else {
        zdj_soundcard_node_t * prev_node = soundcard->nodes;
        while( prev_node ) {
            // find the tip and add the new node
            if( !prev_node->next ) {
                prev_node->next = (struct zdj_soundcard_node_t*)node;
                prev_node = NULL;
            } else {
                prev_node = (zdj_soundcard_node_t*)(prev_node->next);
            }
        }
    }
}

zdj_error_type_t zdj_soundcard_remove_node( 
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node 
) {

}
zdj_error_type_t zdj_soundcard_remove_all_nodes( zdj_soundcard_t * soundcard );

zdj_soundcard_node_t * zdj_soundcard_get_node_for_name( 
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_name_t name 
) {
    // printf( "zdj_soundcard_get_node_for_name: %s\n", zdj_soundcard_node_name[ name ] );
    // Loop thru soundcard nodes for a matching name.
    // If there are no nodes, link the new one at the root.
    if( !soundcard->nodes ) {
        return NULL;
    } else {
        zdj_soundcard_node_t * prev_node = soundcard->nodes;
        while( prev_node ) {
            // find the tip and add the new node
            if( prev_node->name == name ) {
                return prev_node;
            } else {
                prev_node = (zdj_soundcard_node_t*)(prev_node->next);
            }
        }
        return NULL;
    }
}

zdj_error_type_t zdj_soundcard_unlink_all_nodes_from_node( 
    zdj_soundcard_t * soundcard,
    zdj_soundcard_node_t * node 
) {
    zdj_soundcard_link_bitmap_t node_mask;
    zdj_soundcard_link_bitmap_t link_map;
    // Loop thru all nodes, setting node's bit to 0 in linkmap
    node_mask = 1ULL << node->name;
    for ( int i=0; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
        link_map = zdj_soundcard_dto_get_linkmap_for_node_name( &soundcard->dto, i );
        if ( link_map & node_mask ) { 
            link_map ^= node_mask; // Flip the node bit we've found.
            zdj_soundcard_dto_set_linkmap_for_node_name( &soundcard->dto, i, link_map );
            
            // Refresh node links as they're touched
            zdj_soundcard_pull_node_links_from_dto( 
                soundcard, 
                zdj_soundcard_get_node_for_name( soundcard, i )
            );

            // Dirty the soundcard if we touch anything
            soundcard->has_edits = true;
        }
    }

    
}

zdj_error_type_t zdj_soundcard_link_source_node_to_dest_node( 
    zdj_soundcard_t * soundcard,
    zdj_soundcard_node_t * source_node,
    zdj_soundcard_node_t * dest_node
) {
    // printf( "zdj_soundcard_link_source_node_to_dest_node: %s/%s\n",
    //     zdj_soundcard_node_name[ source_node->name ],
    //     zdj_soundcard_node_name[ source_node->name ]
    // );
    zdj_soundcard_link_bitmap_t source_link_map;
    zdj_soundcard_link_bitmap_t dest_node_mask;

    source_link_map = zdj_soundcard_dto_get_linkmap_for_node_name( 
        &soundcard->dto, source_node->name 
    );
    dest_node_mask = 1ULL << dest_node->name;
    if( !(source_link_map & dest_node_mask) ) {
        // printf( "flipping bit: %s\n", zdj_soundcard_node_name[ dest_node->name ] );
        // Flip the link map bit
        source_link_map ^= dest_node_mask;
        zdj_soundcard_dto_set_linkmap_for_node_name( 
            &soundcard->dto, source_node->name, source_link_map 
        );
        // Refresh node links
        zdj_soundcard_pull_node_links_from_dto( soundcard, source_node );
        // Dirty the soundcard
        soundcard->has_edits = true;
    }
}

zdj_error_type_t zdj_soundcard_unlink_source_node_from_dest_node( 
    zdj_soundcard_t * soundcard,
    zdj_soundcard_node_t * source_node,
    zdj_soundcard_node_t * dest_node
) {
    // printf( "zdj_soundcard_unlink_source_node_from_dest_node\n" );
    zdj_soundcard_link_bitmap_t source_link_map;
    zdj_soundcard_link_bitmap_t dest_node_mask;

    source_link_map = zdj_soundcard_dto_get_linkmap_for_node_name( 
        &soundcard->dto, source_node->name 
    );
    dest_node_mask = 1ULL << dest_node->name;
    if( source_link_map & dest_node_mask ) {
        // printf( "flipping bit: %s\n", zdj_soundcard_node_name[ dest_node->name ] );
        // Flip the link map bit
        source_link_map ^= dest_node_mask;
        zdj_soundcard_dto_set_linkmap_for_node_name( 
            &soundcard->dto, source_node->name, source_link_map 
        );
        // Refresh node links
        zdj_soundcard_pull_node_links_from_dto( soundcard, source_node );
        // Dirty the soundcard
        soundcard->has_edits = true;
    }
}

zdj_error_type_t zdj_soundcard_pull_node_links_from_dto( 
    zdj_soundcard_t * soundcard,
    zdj_soundcard_node_t * node
) {
    // Process the linkmap into links
    node->link_count = 0;
    uint64_t node_mask = 0;
    if( zdj_soundcard_node_name_is_output( node->name ) ) {
        // Analog out + record nodes have input links
        int n = zdj_soundcard_count_input_nodes_to_node_name( &soundcard->dto, node->name );
        node->link_count = n;
        if( node->link_count > 0 ) {
            int cur_link = 0;
            node_mask = 1ULL << node->name;
            for ( int i=0; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
                zdj_soundcard_link_bitmap_t link_map = zdj_soundcard_dto_get_linkmap_for_node_name( 
                    &soundcard->dto, i 
                );
                if ( link_map & node_mask ) { 
                    // printf( "setting link: %s<->%s\n", 
                    //     zdj_soundcard_node_name[ i ], 
                    //     zdj_soundcard_node_name[ node->name ] 
                    // );
                    node->links[cur_link].source_node = i;
                    node->links[cur_link].dest_node = node->name;
                    cur_link++;
                }
            }
        }
    } else {
        // Normal output links for everything else
        node->link_map = zdj_soundcard_dto_get_linkmap_for_node_name( 
            &soundcard->dto, node->name 
        );
        node->link_count = zdj_soundcard_count_output_nodes_from_link_map( node->link_map );
        if( node->link_count > 0 ) {
            int cur_link = 0;
            for ( int i=0; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
                node_mask = 1ULL << i;
                if ( node->link_map & node_mask ) { 
                    // printf( "setting link: %s<->%s\n", 
                    //     zdj_soundcard_node_name[ node->name ], 
                    //     zdj_soundcard_node_name[ i ] 
                    // );
                    node->links[ cur_link ].source_node = node->name;
                    node->links[ cur_link ].dest_node = i;
                    cur_link++;
                }
            }
        }
    }
}

// Scan all linkmaps in the dto for any pointing to the given node name
int zdj_soundcard_count_input_nodes_to_node_name( 
    zdj_soundcard_dto_t * dto,
    zdj_soundcard_node_name_t name 
) {
    int i;
    int linked_node_count = 0;
    zdj_soundcard_link_bitmap_t link_map;
    uint64_t node_mask = 1ULL << name;
    for ( i=0; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
        link_map = zdj_soundcard_dto_get_linkmap_for_node_name( dto, i );
        if ( link_map & node_mask ) { linked_node_count++; }
    }
    return linked_node_count;
}

// Tally all the links from a given node
int zdj_soundcard_count_output_nodes_from_node( 
    zdj_soundcard_dto_t * dto,
    zdj_soundcard_node_t * node 
) {
    zdj_soundcard_link_bitmap_t link_map = zdj_soundcard_dto_get_linkmap_for_node_name( dto, node->name );
    return zdj_soundcard_count_output_nodes_from_link_map( link_map );
}

// Tally all the links in the given linkmap
int zdj_soundcard_count_output_nodes_from_link_map( 
    zdj_soundcard_link_bitmap_t link_map
) {
    // printf( "zdj_soundcard_count_output_nodes_from_link_map: %lu\n", link_map );
    int i;
    int linked_node_count = 0;
    zdj_soundcard_link_bitmap_t node_mask;
    for ( i=0; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
        node_mask = 1ULL << i;
        // printf( "link_map %lu node_mask: %lu\n", link_map, node_mask );
        if ( link_map & node_mask ) { 
            // printf( "found output link: %d/%s\n", i, zdj_soundcard_node_name[ i ] );
            linked_node_count++; 
        }
    }
    return linked_node_count;
}

zdj_soundcard_node_t * zdj_soundcard_node_get_stereo_partner_node( zdj_soundcard_node_t * node ) {
    switch ( node->name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0: 
            return zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1 );
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
            return zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 );
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2:
            return zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3 );
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3:
            return zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 );
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0:
            return zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1 );
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1:
            return zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0 );
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2:
            return zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3 );
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3:
            return zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2 );
        default: return NULL;
    }
}
