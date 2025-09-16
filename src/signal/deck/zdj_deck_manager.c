#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>

zdj_deck_manager_t * _zdj_deck_manager;

static void * _zdj_deck_manager_thread_main( void * arg );
static bool _station_can_handle_event( zdj_deck_station_t station, zdj_control_event_t * event );

zdj_deck_manager_t * zdj_deck_manager( void ) {
    if( !_zdj_deck_manager ) { zdj_deck_manager_init( ); }
    return _zdj_deck_manager;
}

zdj_error_type_t zdj_deck_manager_init( void ) {
    _zdj_deck_manager = calloc( 1, sizeof( zdj_deck_manager_t ) );

    // Start update thread
    zdj_thread_launch_deck_manager_cycle( &_zdj_deck_manager_thread_main, _zdj_deck_manager );
}

zdj_deck_t * zdj_deck_manager_add_deck( 
    zdj_deck_type_t type,
    zdj_deck_station_t station,
    void * resource
) {
    printf( "deck_manager loading deck station %d\n", station );
    // If there's a deck in this station, start its remove process.
    // TODO
    zdj_deck_t * cur_deck = zdj_deck_manager_get_deck_for_station( station );
    if( cur_deck ) {
        zdj_deck_manager_remove_deck( cur_deck );
    }

    // Stand up the deck.
    zdj_deck_t * deck = zdj_new_deck( type, station, resource );
    
    // Link the deck into the manager.
    if( zdj_deck_manager( )->decks ) { zdj_deck_manager( )->decks->prev = deck; }
    deck->next = zdj_deck_manager( )->decks;
    zdj_deck_manager( )->decks = deck;

    return deck;
}

zdj_error_type_t zdj_deck_manager_remove_deck( zdj_deck_t * deck ) {
    // Look thru active decks for a match.
    // Advance the deck's state into the deinit flow.
    zdj_deck_t * d = zdj_deck_manager( )->decks;
    while( d ) {
        if( d == deck ) {
            if( d->begin_teardown ) {
                d->begin_teardown( d );
            } else {
                d->status = ZDJ_DECK_STATUS_STOP_TRANSPORT;
            }
        }
        d = d->next;
    }
}

zdj_deck_t * zdj_deck_manager_get_deck_for_station( zdj_deck_station_t station ) {
    zdj_deck_t * d = zdj_deck_manager( )->decks;
    while( d ) {
        if( d->station == station ) {
            return d;
        }
        d = d->next;
    }
    return NULL;
}

// Get a new batch of mapped deck control events from the Control system.
// Called from control update cycle (~900Hz).
void zdj_deck_manager_handle_events( int start_ind, int end_ind ) {
    // Step thru each deck, handling events.
    zdj_deck_t * deck = zdj_deck_manager( )->decks;
    zdj_control_event_t * event;
    while( deck ) {
        int i = start_ind;
        while ( i != end_ind ) {
            i++; i %= ZDJ_CONTROL_EVENT_BUF_LEN; // Loop i in ring buffer
            event = &zdj_deck_event_buf[ i ];
            // Step thru ring buffer, passing each event down into deck.
            if( _station_can_handle_event( deck->station, event ) && deck->handle_control_event ) {
                deck->handle_control_event( deck, event );
            }
        }
        deck = deck->next;
    }

    // Capture control-change flags on the soundcard (main/cue vol, etc.)
    int i = start_ind;
    while ( i != end_ind ) {
        i++; i %= ZDJ_CONTROL_EVENT_BUF_LEN; // Loop i in ring buffer
        // printf( "control change: %d\n", zdj_deck_event_buf[ i ].id );
        // Note the event's control_id in the deck's change flags.
        zdj_deck_manager( )->control_change_flags[ zdj_deck_event_buf[ i ].id ] = 1;
    }
}

// Clear all control flags
void zdj_deck_manager_clear_control_flags( zdj_deck_t * deck ) {
    memset( zdj_deck_manager( )->control_change_flags, 0, ZDJ_CONTROL_ID_COUNT * sizeof( uint8_t ) );
}

// Called from control update cycle (~900Hz).
// Update each active deck's control model.
// Used to control transport state, run physics sim
// for jog wheel, etc.
void zdj_deck_manager_control_update_cycle( void ) {
    zdj_deck_t * deck = zdj_deck_manager( )->decks;
    while( deck ) {
        deck->update_controls( deck );
        deck = deck->next;
    }
}

// Deck manager thread runs on a slow sleep cycle and handles
// requests asynchronously after they arrive.  
static void * _zdj_deck_manager_thread_main( void * arg ) {
    zdj_deck_manager_t * manager = (zdj_deck_manager_t*)arg;

    // Set up scheduling
    int prio = sched_get_priority_max( SCHED_RR );
	struct sched_param param;
	param.sched_priority = prio;
	sched_setscheduler( syscall(SYS_gettid), SCHED_RR, &param );

    // Give realtime scheduler access to 100% of core time
	system( "echo -1 >/proc/sys/kernel/sched_rt_runtime_us" );

    // Set core affinity to Core #1;
    cpu_set_t cpuset;
	CPU_ZERO( &cpuset );
	CPU_SET( 2,&cpuset );
	int err = sched_setaffinity( syscall(SYS_gettid), sizeof(cpu_set_t), &cpuset );
    if( err != 0 ) {
        perror( "set affinity failed" );
    }


    // Sleep thread for ~.25 sec - we don't need to update very fast.
    struct timespec cycle_delay = { 0, 250000000 };

    while( 1 ) {
        // Sleep thread until next check
        nanosleep( &cycle_delay, NULL );

        // Update state for all the decks
        zdj_error_state( )->marker = ZDJ_ERROR_MARKER_DECK_UPDATE;
        zdj_deck_t * deck = zdj_deck_manager( )->decks;
        while( deck ) {
            zdj_deck_t * next_deck = deck->next;
            if( deck->update_state ) { deck->update_state( deck ); }
            // If deck is at idle state, it's ready to be removed.
            if( deck->status == ZDJ_DECK_STATUS_IDLE ) {
                if( deck->prev ) { deck->prev->next = deck->next; }
                if( deck->next ) { deck->next->prev = deck->prev; }
                if( zdj_deck_manager( )->decks == deck ) { zdj_deck_manager( )->decks = deck->next; }
            }
            deck = next_deck;
        }
        
        zdj_error_state( )->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
    }

    return NULL;
}

static bool _station_can_handle_event( zdj_deck_station_t station, zdj_control_event_t * event ) {
    switch ( event->id ) {
        case ZDJ_DECK_1_CONTROL_FADE:
        case ZDJ_DECK_1_CONTROL_TRIM:
        case ZDJ_DECK_1_CONTROL_EQ_LO:
        case ZDJ_DECK_1_CONTROL_EQ_MID:
        case ZDJ_DECK_1_CONTROL_EQ_HI:
        case ZDJ_DECK_1_CONTROL_PFL_TRIM:
        case ZDJ_DECK_1_CONTROL_PFL_MUTE:
        case ZDJ_DECK_1_CONTROL_FX_SELECT:
        case ZDJ_DECK_1_CONTROL_FX_0:
        case ZDJ_DECK_1_CONTROL_FX_1:
        case ZDJ_DECK_1_CONTROL_FX_2:
        case ZDJ_DECK_1_CONTROL_FX_3:
        case ZDJ_DECK_1_CONTROL_FX_4:
        case ZDJ_DECK_1_CONTROL_FX_5:
        case ZDJ_DECK_1_CONTROL_SCRUB:
        case ZDJ_DECK_1_CONTROL_TEMPO:
        case ZDJ_DECK_1_CONTROL_TEMPO_FINE:
        case ZDJ_DECK_1_CONTROL_PLAY_PAUSE:
        case ZDJ_DECK_1_CONTROL_PAUSE:
        case ZDJ_DECK_1_CONTROL_HOTCUE_START:
        case ZDJ_DECK_1_CONTROL_HOTCUE_END: return station == ZDJ_DECK_STATION_1;


        case ZDJ_DECK_2_CONTROL_FADE:
        case ZDJ_DECK_2_CONTROL_TRIM:
        case ZDJ_DECK_2_CONTROL_EQ_LO:
        case ZDJ_DECK_2_CONTROL_EQ_MID:
        case ZDJ_DECK_2_CONTROL_EQ_HI:
        case ZDJ_DECK_2_CONTROL_PFL_TRIM:
        case ZDJ_DECK_2_CONTROL_PFL_MUTE:
        case ZDJ_DECK_2_CONTROL_FX_SELECT:
        case ZDJ_DECK_2_CONTROL_FX_0:
        case ZDJ_DECK_2_CONTROL_FX_1:
        case ZDJ_DECK_2_CONTROL_FX_2:
        case ZDJ_DECK_2_CONTROL_FX_3:
        case ZDJ_DECK_2_CONTROL_FX_4:
        case ZDJ_DECK_2_CONTROL_FX_5:
        case ZDJ_DECK_2_CONTROL_SCRUB:
        case ZDJ_DECK_2_CONTROL_TEMPO:
        case ZDJ_DECK_2_CONTROL_TEMPO_FINE:
        case ZDJ_DECK_2_CONTROL_PLAY_PAUSE:
        case ZDJ_DECK_2_CONTROL_PAUSE:
        case ZDJ_DECK_2_CONTROL_HOTCUE_START:
        case ZDJ_DECK_2_CONTROL_HOTCUE_END: return station == ZDJ_DECK_STATION_2;


        case ZDJ_DECK_CONTROL_LR_VOL:
        case ZDJ_DECK_CONTROL_CUE_VOL:
        case ZDJ_DECK_CONTROL_XFADE: return true;

        default: break;
    }
}