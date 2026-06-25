#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.h>
#include <zerodj/signal/pipeline/node/audio/io/zdj_io_node.h>
#include <zerodj/signal/pipeline/node/analysis/meter/zdj_meter_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/db/zdj_soundcard_dto.h>
#include <zerodj/system/sql/zdj_sql.h>

zdj_soundcard_node_t * zdj_soundcard_create_node( zdj_soundcard_node_name_t name ) {
    zdj_soundcard_node_t * node = calloc( 1, sizeof( zdj_soundcard_node_t ) );
    node->name = name;

    // printf( "zdj_soundcard_create_node: %s\n", zdj_soundcard_node_name[ node->name ] );

    // Get props from the dto
    node->link_map = zdj_soundcard_dto_get_linkmap_for_node_name( &zdj_soundcard->dto, name );
    node->signal_type = zdj_soundcard_dto_get_sigtype_for_node_name( &zdj_soundcard->dto, name );
    node->stereo = zdj_soundcard_dto_get_stereo_for_node_name( &zdj_soundcard->dto, name );
    node->mute = zdj_soundcard_dto_get_mute_for_node_name( &zdj_soundcard->dto, name );
    node->direction = zdj_soundcard_dto_get_source_for_node_name( &zdj_soundcard->dto, name );
    node->val = zdj_soundcard_dto_get_val_for_node_name( &zdj_soundcard->dto, name );

    // Add signal/meter nodes
    if( zdj_soundcard_node_name_is_audio( name ) ) {
        // Add a buffer if applicable
        node->data_pipe = zdj_new_audio_buffer_node( ZDJ_SOUNDCARD_BUF_LEN, node->stereo+1 );
        node->meter_pipe = zdj_new_meter_node( ZDJ_METER_NODE_TYPE_AUDIO, node->stereo+1 );
        node->dsp_dto = zdj_soundcard_dto_get_dsp_for_node_name( &zdj_soundcard->dto, name );
        // printf( "Adding dto: %p to %s\n", node->dsp_dto, zdj_soundcard_node_name[ name ] );
    } else if( zdj_soundcard_node_name_is_cv( name ) ) {
        node->data_pipe = zdj_new_audio_buffer_node( ZDJ_SOUNDCARD_BUF_LEN, node->stereo+1 );
        node->meter_pipe = zdj_new_meter_node( ZDJ_METER_NODE_TYPE_CV, node->stereo+1 );
        node->dsp_dto = zdj_soundcard_dto_get_dsp_for_node_name( &zdj_soundcard->dto, name );
    } else if( zdj_soundcard_node_name_is_clock( name ) ) {
        node->data_pipe = zdj_new_audio_buffer_node( ZDJ_SOUNDCARD_BUF_LEN, node->stereo+1 );
        node->meter_pipe = zdj_new_meter_node( ZDJ_METER_NODE_TYPE_CLOCK, node->stereo+1 );
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

zdj_error_type_t zdj_soundcard_remove_all_nodes( zdj_soundcard_t * soundcard ) {
    if( !soundcard->nodes ) {
        return ZDJ_ERROR_OKAY;
    } else {
        zdj_soundcard_node_t * node = soundcard->nodes;
        while( node ) {
            zdj_soundcard_node_t *next_node = (zdj_soundcard_node_t*)(node->next);
            free( node );
            node = next_node;
        }
    }
    soundcard->nodes = NULL;
}

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
    // printf( "zdj_soundcard_link_source_node_to_dest_node: %s -> %s\n",
    //     zdj_soundcard_node_name[ source_node->name ],
    //     zdj_soundcard_node_name[ dest_node->name ]
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
        // If node has a stereo partner, unlink that node as well
        zdj_soundcard_node_t * partner_node = zdj_soundcard_node_get_stereo_partner_node( source_node );
        if( source_node->stereo && partner_node ) {
            zdj_soundcard_dto_set_linkmap_for_node_name( 
                &soundcard->dto, partner_node->name, source_link_map 
            );
        }
        // Refresh all source/dest_node links
        for ( int i=ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
            zdj_soundcard_pull_node_links_from_dto( 
                soundcard, 
                zdj_soundcard_get_node_for_name( soundcard, i ) 
            );
        }
        // Dirty the soundcard
        soundcard->has_edits = true;
    }
}

zdj_error_type_t zdj_soundcard_unlink_source_node_from_dest_node( 
    zdj_soundcard_t * soundcard,
    zdj_soundcard_node_t * source_node,
    zdj_soundcard_node_t * dest_node
) {
    printf( "zdj_soundcard_unlink_source_node_from_dest_node %s x-> %s\n",
        zdj_soundcard_node_name[ source_node->name ],
        zdj_soundcard_node_name[ dest_node->name ] 
    );
    zdj_soundcard_link_bitmap_t source_link_map;
    zdj_soundcard_link_bitmap_t dest_node_mask;

    source_link_map = zdj_soundcard_dto_get_linkmap_for_node_name( 
        &soundcard->dto, source_node->name 
    );
    dest_node_mask = 1ULL << dest_node->name;
    if( source_link_map & dest_node_mask ) {
        // Flip the link map bit
        source_link_map ^= dest_node_mask;
        zdj_soundcard_dto_set_linkmap_for_node_name( 
            &soundcard->dto, source_node->name, source_link_map 
        );

        // If node has a stereo partner, unlink that node as well
        if( zdj_soundcard_node_name_is_analog_output( dest_node->name ) ) {
            zdj_soundcard_node_t * partner_node = zdj_soundcard_node_get_stereo_partner_node( dest_node );
            if( dest_node->stereo && partner_node ) {
                printf( "unlinking partner: %s\n", zdj_soundcard_node_name[ partner_node->name ] );
                zdj_soundcard_link_bitmap_t partner_node_mask = (1ULL << partner_node->name);
                source_link_map ^= partner_node_mask;
                zdj_soundcard_dto_set_linkmap_for_node_name( 
                    &soundcard->dto, source_node->name, source_link_map 
                );
            }
        } else if( zdj_soundcard_node_name_is_analog_input( source_node->name ) ) {
            zdj_soundcard_node_t * partner_node = zdj_soundcard_node_get_stereo_partner_node( source_node );
            if( source_node->stereo && partner_node ) {
                printf( "unlinking partner: %s\n", zdj_soundcard_node_name[ partner_node->name ] );
                zdj_soundcard_dto_set_linkmap_for_node_name( 
                    &soundcard->dto, partner_node->name, source_link_map 
                );
            }
        } 
        
        // Refresh all source/dest_node links
        for ( int i=ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
            zdj_soundcard_pull_node_links_from_dto( 
                soundcard, 
                zdj_soundcard_get_node_for_name( soundcard, i ) 
            );
        }
        // Dirty the soundcard
        soundcard->has_edits = true;
    }
}

zdj_error_type_t zdj_soundcard_pull_node_links_from_dto( 
    zdj_soundcard_t * soundcard,
    zdj_soundcard_node_t * node
) {
    uint64_t node_mask = 0;
    // Build input links by scanning all node linkmaps for links to this node.
    int n = zdj_soundcard_count_input_nodes_to_node_name( &soundcard->dto, node->name );
    node->input_link_count = n;
    if( node->input_link_count > 0 ) {
        int cur_link = 0;
        node_mask = 1ULL << node->name;
        for ( int i=ZDJ_SOUNDCARD_NODE_NAME_NONE; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
            zdj_soundcard_link_bitmap_t link_map = zdj_soundcard_dto_get_linkmap_for_node_name( 
                &soundcard->dto, i 
            );
            if ( link_map & node_mask ) { 
                node->input_links[cur_link].source_node = i;
                node->input_links[cur_link].dest_node = node->name;
                cur_link++;
            }
        }
    }

    // Build output links by scanning each bit in this node's linkmap.
    node->link_map = zdj_soundcard_dto_get_linkmap_for_node_name( 
        &soundcard->dto, node->name 
    );
    node->output_link_count = zdj_soundcard_count_output_nodes_from_link_map( node->link_map );
    if( node->output_link_count > 0 ) {
        int cur_link = 0;
        for ( int i=ZDJ_SOUNDCARD_NODE_NAME_NONE; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
            node_mask = 1ULL << i;
            if ( node->link_map & node_mask ) { 
                node->output_links[ cur_link ].source_node = node->name;
                node->output_links[ cur_link ].dest_node = i;
                cur_link++;
            }
        }
    }
}

// Scan all linkmaps in the dto for any pointing to the given node name
int zdj_soundcard_count_input_nodes_to_node_name( 
    zdj_soundcard_dto_t * dto,
    zdj_soundcard_node_name_t name 
) {
    // printf( "zdj_soundcard_count_input_nodes_to_node_name %p\n", dto );
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


void zdj_soundcard_set_stereo_for_node( zdj_soundcard_node_t * node, bool stereo ) {
    // printf( "zdj_soundcard_set_stereo_for_node\n" );
    zdj_soundcard_node_t * stereo_partner_node = zdj_soundcard_node_get_stereo_partner_node( node );
    node->stereo = stereo;
    if( stereo_partner_node ) { 
        // Link stereo
        stereo_partner_node->stereo = stereo; 

        // If stereo, join up the signals
        if( stereo ) { 
            stereo_partner_node->signal_type = node->signal_type;
            stereo_partner_node->mute = node->mute;
            
            // Output/Input ports need special node link handling.
            // Require left channel only to have linkage when stereo.
            if( node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1 ||
                node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3
            ) {
                printf( "unlinking node for stereo merge: %s\n", zdj_soundcard_node_name[ node->name ] );
                for( int i=0; i<node->input_link_count; i++ ) {
                    zdj_soundcard_unlink_source_node_from_dest_node( 
                        zdj_soundcard, node, 
                        zdj_soundcard_get_node_for_name( 
                            zdj_soundcard, node->input_links[ i ].source_node 
                        )
                    );
                }
            } else if( node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1 ||
                node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3
            ) {
                // printf( "unlinking node for stereo merge: %s\n", zdj_soundcard_node_name[ node->name ] );
                for( int i=0; i<node->output_link_count; i++ ) {
                    zdj_soundcard_unlink_source_node_from_dest_node( 
                        zdj_soundcard, node, 
                        zdj_soundcard_get_node_for_name( 
                            zdj_soundcard, node->output_links[ i ].dest_node 
                        )
                    );
                }
            }
        } else {
            // Output/Input ports need special node link handling.
            // When splitting a stereo channel into monos, 
            // copy this node's links to the stereo partner.
            // printf( "linking node for stereo split: %s\n", zdj_soundcard_node_name[ node->name ] );
            if( zdj_soundcard_node_name_is_analog_output( node->name ) ) {
                for( int i=0; i<node->input_link_count; i++ ) {
                    zdj_soundcard_link_source_node_to_dest_node( 
                        zdj_soundcard,
                        zdj_soundcard_get_node_for_name( 
                            zdj_soundcard, node->input_links[ i ].source_node 
                        ),
                        stereo_partner_node
                    );
                }
            } else if( zdj_soundcard_node_name_is_analog_input( node->name ) ) {
                for( int i=0; i<node->output_link_count; i++ ) {
                    zdj_soundcard_link_source_node_to_dest_node( 
                        zdj_soundcard,
                        stereo_partner_node,
                        zdj_soundcard_get_node_for_name( 
                            zdj_soundcard, node->output_links[ i ].dest_node 
                        )
                    );
                }
            }
        } 
    }
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
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0: 
            return zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1 );
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1:
            return zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0 );
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0: 
            return zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1 );
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1:
            return zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0 );
        default: return NULL;
    }
}

zdj_error_type_t zdj_soundcard_get_port_title_with_stereo( 
    zdj_soundcard_t * soundcard,
    zdj_soundcard_node_name_t name, 
    char * str 
) {
    if( name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0 &&
        zdj_soundcard_get_node_for_name( soundcard, name )->stereo 
    ) {
        strcpy( str, "Analog In 1/2" );
    } else if( name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2 &&
        zdj_soundcard_get_node_for_name( soundcard, name )->stereo 
    ) {
        strcpy( str, "Analog In 3/4" );
    } else if( name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 &&
        zdj_soundcard_get_node_for_name( soundcard, name )->stereo 
    ) {
        strcpy( str, "Analog Out 1/2" );
    } else if( name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 &&
        zdj_soundcard_get_node_for_name( soundcard, name )->stereo 
    ) {
        strcpy( str, "Analog Out 3/4" );
    } else if( name == ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0 &&
        zdj_soundcard_get_node_for_name( soundcard, name )->stereo 
    ) {
        strcpy( str, "USB In 1/2" );
    } else if( name == ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0 &&
        zdj_soundcard_get_node_for_name( soundcard, name )->stereo 
    ) {
        strcpy( str, "USB Out 1/2" );
    }
}


void zdj_soundcard_set_mute_for_node( zdj_soundcard_node_t * node, bool mute ) {
    node->mute = mute;
    // zdj_soundcard_dto_set_mute_for_node_name( node )
    if( node->stereo ) {
        zdj_soundcard_node_t * stereo_partner_node = zdj_soundcard_node_get_stereo_partner_node( node );
        if( stereo_partner_node ) { 
            stereo_partner_node->mute = mute;
            // zdj_soundcard_dto_set_mute_for_node_name( partner )
        }
    }
}


bool zdj_soundcard_can_add_aux_bus( zdj_soundcard_t * soundcard ) {
    if( !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0 )->output_link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1 )->output_link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2 )->output_link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3 )->output_link_count 
    ) {
        return true;
    } else {
        return false;
    }
}

bool zdj_soundcard_can_add_clock_bus( zdj_soundcard_t * soundcard ) {
    if( !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_0 )->output_link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_1 )->output_link_count 
        // ||
        // !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_2 )->output_link_count ||
        // !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_3 )->output_link_count 
    ) {
        return true;
    } else {
        return false;
    }
}

bool zdj_soundcard_can_add_cv_bus( zdj_soundcard_t * soundcard ) {
    if( !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_0 )->output_link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_1 )->output_link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_2 )->output_link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_3 )->output_link_count 
    ) {
        return true;
    } else {
        return false;
    }
}

bool zdj_soundcard_can_add_midi_bus( zdj_soundcard_t * soundcard ) {
    // if( !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0 )->output_link_count ||
    //     !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1 )->output_link_count ||
    //     !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2 )->output_link_count ||
    //     !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3 )->output_link_count 
    // ) {
    //     return true;
    // } else {
        return false;
    // }
}

zdj_soundcard_node_t * zdj_soundcard_get_available_aux_bus_node( zdj_soundcard_t * soundcard ) {
    if( !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0 )->output_link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1 )->output_link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2 )->output_link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3 )->output_link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3 );
    }
}

zdj_soundcard_node_t * zdj_soundcard_get_available_clock_bus_node( zdj_soundcard_t * soundcard ) {
    if( !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_0 )->output_link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_0 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_1 )->output_link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_1 );
    } 
    // else if( 
    //     !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_2 )->output_link_count 
    // ) {
    //     return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_2 );
    // } else if( 
    //     !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_3 )->output_link_count 
    // ) {
    //     return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_3 );
    // }
}

zdj_soundcard_node_t * zdj_soundcard_get_available_cv_bus_node( zdj_soundcard_t * soundcard ) {
    if( !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_0 )->output_link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_0 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_1 )->output_link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_1 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_2 )->output_link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_2 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_3 )->output_link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_3 );
    }
}

zdj_soundcard_node_t * zdj_soundcard_get_available_midi_bus_node( zdj_soundcard_t * soundcard ) {
    return NULL;
}

// Step to next sig type for node.
zdj_error_type_t zdj_soundcard_cycle_pad_for_io_node(
    zdj_soundcard_t * soundcard,
    zdj_soundcard_node_t * node
) {
    if( zdj_soundcard_node_name_is_io( node->name ) ) {
        zdj_soundcard_signal_type_t sig_type = zdj_soundcard_dto_get_sigtype_for_node_name( 
            &soundcard->dto, node->name 
        );
        switch ( sig_type ){
            case ZDJ_SOUNDCARD_SIGNAL_MINUS_10_DBV:
                node->signal_type = ZDJ_SOUNDCARD_SIGNAL_CON_0_DBV;
                break;
            case ZDJ_SOUNDCARD_SIGNAL_CON_0_DBV:
                node->signal_type = ZDJ_SOUNDCARD_SIGNAL_PRO_PLUS_4_DBU;
                break;
            case ZDJ_SOUNDCARD_SIGNAL_PRO_PLUS_4_DBU:
                node->signal_type = ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_LOW;
                break;
            case ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_LOW:
                node->signal_type = ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_HI;
                break;
            case ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_HI:
                node->signal_type = ZDJ_SOUNDCARD_SIGNAL_AUDIO_RAIL_TO_RAIL;
                break;
            case ZDJ_SOUNDCARD_SIGNAL_AUDIO_RAIL_TO_RAIL:
                node->signal_type = ZDJ_SOUNDCARD_SIGNAL_MINUS_10_DBV;
                break;
        } 
        zdj_soundcard_dto_set_sigtype_for_node_name( 
            &soundcard->dto, node->name, node->signal_type 
        );
    }
}

bool zdj_soundcard_node_name_show_in_mixer( zdj_soundcard_node_name_t name ) {
    return true;
}

bool zdj_soundcard_node_name_is_audio( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3:
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0:
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1:
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2:
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_INPUT:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_EDGE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_POSTFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_CUE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_INPUT:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_EDGE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_POSTFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_CUE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_EDGE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_POSTFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_CUE: 
        case ZDJ_SOUNDCARD_NODE_NAME_XFADE_A: 
        case ZDJ_SOUNDCARD_NODE_NAME_XFADE_B: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_io( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_physical_port( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_input( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_analog_input( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_output( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_analog_output( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_internal_bus( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_dj_deck( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_ext_deck( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_aux_bus( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0:
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1:
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2:
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_clock( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_2:
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_cv( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0:
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1:
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2:
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_muted( zdj_soundcard_node_name_t name ) {
    return false;
}

bool zdj_soundcard_node_name_is_midi( zdj_soundcard_node_name_t name ) {
    return false;
}

bool zdj_soundcard_node_name_is_usb( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_usb_input( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_usb_output( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_right_channel( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_should_show_fader( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3: return false;
        default: return true;
    }
}