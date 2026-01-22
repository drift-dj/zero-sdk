#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavcodec/codec_id.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>


static bool _contains_core_addr( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * addr, 
    zdj_decode_addr_coord_t ref_coord 
);
static bool _contains_addr( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * addr, 
    zdj_decode_addr_coord_t ref_coord 
);
static void _update_buf_coords_for_node_head( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node );
static double _origin_d_coord_for_transport_d_coord( 
    zdj_decode_layer_t * layer, double coord 
);

void zdj_decode_layer_init_addr_api( zdj_decode_layer_t * layer ) {

    layer->contains_addr = &_contains_addr;
    layer->contains_core_addr = &_contains_core_addr;
    layer->update_buf_coords_for_head = &_update_buf_coords_for_node_head;
    layer->origin_d_coord_for_transport_d_coord = &_origin_d_coord_for_transport_d_coord;
}

static bool _contains_core_addr( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * addr, 
    zdj_decode_addr_coord_t ref_coord 
) {
    return ( (layer->core_start.less_than( &layer->core_start, addr, ref_coord ) &&
              layer->core_end.greater_than( &layer->core_end, addr, ref_coord )) ||
              layer->core_start.equal( &layer->core_start, addr, ref_coord ) ||
              layer->core_end.equal( &layer->core_end, addr, ref_coord ) );
}

static bool _contains_addr( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * addr, 
    zdj_decode_addr_coord_t ref_coord 
) {
    return ( (layer->lead_in_start.less_than( &layer->lead_in_start, addr, ref_coord ) &&
              layer->lead_out_end.greater_than( &layer->lead_out_end, addr, ref_coord )) ||
              layer->lead_in_start.equal( &layer->lead_in_start, addr, ref_coord ) ||
              layer->lead_out_end.equal( &layer->lead_out_end, addr, ref_coord ) );
}

static void _update_buf_coords_for_node_head( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node ) {
    // Updates the indexes into node's out buffer.
    layer->core_start.update_buf_i_for_node_head( &layer->core_start, node );
    layer->core_end.update_buf_i_for_node_head( &layer->core_end, node );
    layer->lead_in_start.update_buf_i_for_node_head( &layer->lead_in_start, node );
    layer->lead_in_end.update_buf_i_for_node_head( &layer->lead_in_end, node );
    layer->lead_out_start.update_buf_i_for_node_head( &layer->lead_out_start, node );
    layer->lead_out_end.update_buf_i_for_node_head( &layer->lead_out_end, node );
    zdj_decode_packet_t * packet = layer->first_packet;
    while( packet ) {
        packet->start_addr.update_buf_i_for_node_head( &packet->start_addr, node );
        packet->end_addr.update_buf_i_for_node_head( &packet->end_addr, node );
        packet = packet->next;
    }
}

static double _origin_d_coord_for_transport_d_coord( zdj_decode_layer_t * layer, double coord ) {
    // Convert a transport_d coord in a given layer's space into it's origin_d coord space
    double offset = coord - layer->init_addr.transport_d;
    return layer->init_addr.origin_d + offset;
}