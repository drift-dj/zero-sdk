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
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

zdj_deck_manager_t * _zdj_deck_manager;

static void * _zdj_deck_manager_thread_main( void * arg );
static bool _station_can_handle_event( zdj_deck_station_t station, zdj_control_event_t * event );
static bool _is_soundcard_event( zdj_control_event_t * event );

zdj_deck_manager_t * zdj_deck_manager( void ) {
    if( !_zdj_deck_manager ) { 
        printf( "zdj_deck_manager( ) pre: %p\n", _zdj_deck_manager );
        zdj_deck_manager_init( ); 
        printf( "zdj_deck_manager( ) post: %p\n", _zdj_deck_manager );
    }
    return _zdj_deck_manager;
}

zdj_error_type_t zdj_deck_manager_init( void ) {
    // printf( "zdj_deck_manager_init\n" );
    _zdj_deck_manager = calloc( 1, sizeof( zdj_deck_manager_t ) );
    
    // Set sync initial conditions
    _zdj_deck_manager->sync.preferred = true; // Read this from user settings
    _zdj_deck_manager->sync.active = false;
    _zdj_deck_manager->sync.locked = false;
    _zdj_deck_manager->sync.set_bpm = 120.0f;

    // Start update thread
    zdj_thread_launch_deck_manager_cycle( &_zdj_deck_manager_thread_main, _zdj_deck_manager );
}

zdj_deck_t * zdj_deck_manager_add_deck( 
    zdj_deck_type_t type,
    zdj_deck_station_t station,
    void * resource,
    int win_buf_count
) {
    // printf( "deck_manager loading deck station %d type: %d %p\n", station, type, zdj_deck_manager( )->decks );
    // If there's a deck in this station, start its remove process.
    // TODO
    zdj_deck_t * cur_deck = zdj_deck_manager_get_deck_for_station( station );
    if( cur_deck ) {
        zdj_deck_manager_remove_deck( cur_deck );
    }

    // Stand up the deck.
    zdj_deck_t * deck = zdj_new_deck( type, station, resource, win_buf_count );

    // Link the deck into the manager.
    if( zdj_deck_manager( )->decks ) { 
        zdj_deck_manager( )->decks->prev = deck; 
    }
    deck->next = zdj_deck_manager( )->decks;
    zdj_deck_manager( )->decks = deck;

    return deck;
}

zdj_error_type_t zdj_deck_manager_remove_deck( zdj_deck_t * deck ) {
    // printf( "remove deck: %d %d\n", deck->station, deck->type );
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
    // printf( "zdj_deck_manager_handle_events\n" );
    zdj_control_event_t * event;
    
    int i = start_ind;
    while ( i != end_ind ) {
        i++; i %= ZDJ_CONTROL_EVENT_BUF_LEN; // Loop i in ring buffer
        event = &zdj_deck_event_buf[ i ];

        // Handle events for each deck
        zdj_deck_t * deck = zdj_deck_manager( )->decks;
        while( deck ) {
            if( _station_can_handle_event( deck->station, event ) &&
                deck->status == ZDJ_DECK_STATUS_RUNNING &&
                deck->handle_control_event 
            ) {
                deck->handle_control_event( deck, event );
            }
            deck = deck->next;
        }

        // Handle soundcard events
        if( _is_soundcard_event( event ) ) {
            zdj_soundcard_handle_deck_event( zdj_soundcard, event );
        } 
    } 

    // Capture control-change flags on the soundcard (main/cue vol, etc.)
    i = start_ind;
    while ( i != end_ind ) {
        i++; i %= ZDJ_CONTROL_EVENT_BUF_LEN; // Loop i in ring buffer
        // printf( "control change flag[ %d ] = 1\n", zdj_deck_event_buf[ i ].id );
        // Note the event's control_id in the deck's change flags.
        zdj_deck_manager( )->control_change_flags[ zdj_deck_event_buf[ i ].id ] = 1;
    }
    // printf( "zdj_deck_manager_handle_events done\n" );
}




/////////////////////////////////////////////////////////////
// Sync Linkage
// This is crazy complicated
// User can enable sync as a global preference in settings.
// But user can still load un-syncable songs
// If an un-syncable song is present in any DJ deck, UI
// must enter non-synced mode (BPM/tempo shown for each deck).
// If only syncable songs are loaded into the DJ decks,
// UI must enter synced mode (BPM shown at bottom of screen).
/////////////////////////////////////////////////////////////

// Setting prefer sync to true should sync up both DJ decks
// if they both contains syncable songs.
void zdj_deck_manager_set_prefer_sync( bool prefer ) {
    printf( "zdj_deck_manager_prefer_sync\n" );
    // Adopt source_deck's bpm as root bpm.
    zdj_deck_manager( )->sync.preferred = prefer;
    if( prefer && zdj_deck_manager_can_activate_sync( ) ) {
        // FIXME: Set sync bpm to whichever deck is selected in the UI
        zdj_deck_manager_set_sync( 120.0 );
    }
    if( !prefer ) { zdj_deck_manager_deactivate_sync( ); }
}

// Determine if all running DJ decks contain syncable songs.
bool zdj_deck_manager_can_activate_sync( void ) {
    // If both decks have songs w/BPM data, allow sync enable
    bool res = true;
    zdj_deck_t * deck = zdj_deck_manager( )->decks;
    while( deck ) {
        if( deck->type == ZDJ_DECK_TYPE_DJ &&
            deck->status == ZDJ_DECK_STATUS_RUNNING &&
            !deck->can_sync
        ) {
            res = false;
        }
        deck = deck->next;
    }
    printf( "Can activate sync: %d\n", res );
    return res;
}

// 
void zdj_deck_manager_set_sync( double bpm ) {
    printf( "zdj_deck_manager_set_sync: %1.1f\n", bpm );
    if( !zdj_deck_manager_can_activate_sync( ) ){ return; }
    zdj_deck_manager( )->sync.active = true;
    // Adopt source_deck's bpm as root bpm.
    zdj_deck_manager( )->sync.set_bpm = bpm;
    // Loop thru all decks - if syncable, update set_bpm to root bpm
    zdj_deck_t * deck = zdj_deck_manager( )->decks;
    while( deck ) {
        if( deck->can_sync ) { deck->set_sync_bpm( deck, bpm ); }
        deck = deck->next;
    }
}

void zdj_deck_manager_deactivate_sync( void ) {
    printf( "zdj_deck_manager_deactivate_sync\n" );
    zdj_deck_manager( )->sync.active = false;
}

void zdj_deck_manager_update_sync_bpm( double offset ) {
    // printf( "zdj_deck_manager_update_sync_bpm: %f\n", offset );
    if( zdj_deck_manager( )->sync.active ) {
        if( offset > 0.0 || zdj_deck_manager( )->sync.set_bpm + offset > 2.0 ) {
            zdj_deck_manager( )->sync.set_bpm += offset;
            // Set locked to true in case we're updating from eg. an external deck
            // which has no source bpm so won't lock tempo on load.
            zdj_deck_manager( )->sync.locked = true;
            // Update all decks' bpm
            zdj_deck_t * deck = zdj_deck_manager( )->decks;
            while( deck ) {
                if( deck->can_sync ) { deck->set_sync_bpm( deck, zdj_deck_manager( )->sync.set_bpm ); }
                deck = deck->next;
            }
        }
    }
}

// Clear all control flags
void zdj_deck_manager_clear_control_flags( zdj_deck_t * deck ) {
    memset( zdj_deck_manager( )->control_change_flags, 0, ZDJ_CONTROL_ID_COUNT * sizeof( uint8_t ) );
}

// Called from control update cycle (~900Hz).
// Update each active deck's control model.
// Used to control transport state, XXXX run physics sim XXXX
// for jog wheel, etc.
void zdj_deck_manager_control_update_cycle( void ) {
    // printf( "zdj_deck_manager_control_update_cycle\n" );
    zdj_deck_t * deck = zdj_deck_manager( )->decks;
    while( deck ) {
        if( deck->update_transport_inputs ){ deck->update_transport_inputs( deck ); }
        deck = deck->next;
    }
    // printf( "zdj_deck_manager_control_update_cycle done\n" );
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
	CPU_SET( 0,&cpuset );
	int err = sched_setaffinity( syscall(SYS_gettid), sizeof(cpu_set_t), &cpuset );
    if( err != 0 ) {
        perror( "set affinity failed" );
    }


    // Sleep thread for ~.25 sec - we don't need to update very fast.
    struct timespec cycle_delay = { 0, 250000000 };

    while( 1 ) {
        // Sleep thread until next check
        nanosleep( &cycle_delay, NULL );

        // printf( "deck_manager thread start\n" );

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
        // printf( "deck_manager thread done\n" );
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
        case ZDJ_DECK_1_CONTROL_PFL_TOGGLE_MUTE:
        case ZDJ_DECK_1_CONTROL_LOOP_TOGGLE:
        case ZDJ_DECK_1_CONTROL_LOOP_ON:
        case ZDJ_DECK_1_CONTROL_LOOP_OFF:
        case ZDJ_DECK_1_CONTROL_LOOP_START:
        case ZDJ_DECK_1_CONTROL_LOOP_END:
        case ZDJ_DECK_1_CONTROL_LOOP_LENGTH:
        case ZDJ_DECK_1_CONTROL_SKIP:
        case ZDJ_DECK_1_CONTROL_SKIP_LENGTH:
        case ZDJ_DECK_1_CONTROL_SKIP_SET_ORIGIN:
        case ZDJ_DECK_1_CONTROL_SKIP_RESET_TO_ORIGIN:
        case ZDJ_DECK_1_CONTROL_FX_SELECT:
        case ZDJ_DECK_1_CONTROL_FX_0:
        case ZDJ_DECK_1_CONTROL_FX_1:
        case ZDJ_DECK_1_CONTROL_FX_2:
        case ZDJ_DECK_1_CONTROL_FX_3:
        case ZDJ_DECK_1_CONTROL_FX_4:
        case ZDJ_DECK_1_CONTROL_FX_5:
        case ZDJ_DECK_1_CONTROL_SYNC_MULT:
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
        case ZDJ_DECK_2_CONTROL_PFL_TOGGLE_MUTE:
        case ZDJ_DECK_2_CONTROL_LOOP_TOGGLE:
        case ZDJ_DECK_2_CONTROL_LOOP_ON:
        case ZDJ_DECK_2_CONTROL_LOOP_OFF:
        case ZDJ_DECK_2_CONTROL_LOOP_START:
        case ZDJ_DECK_2_CONTROL_LOOP_END:
        case ZDJ_DECK_2_CONTROL_LOOP_LENGTH:
        case ZDJ_DECK_2_CONTROL_SKIP:
        case ZDJ_DECK_2_CONTROL_SKIP_LENGTH:
        case ZDJ_DECK_2_CONTROL_SKIP_SET_ORIGIN:
        case ZDJ_DECK_2_CONTROL_SKIP_RESET_TO_ORIGIN:
        case ZDJ_DECK_2_CONTROL_FX_SELECT:
        case ZDJ_DECK_2_CONTROL_FX_0:
        case ZDJ_DECK_2_CONTROL_FX_1:
        case ZDJ_DECK_2_CONTROL_FX_2:
        case ZDJ_DECK_2_CONTROL_FX_3:
        case ZDJ_DECK_2_CONTROL_FX_4:
        case ZDJ_DECK_2_CONTROL_FX_5:
        case ZDJ_DECK_2_CONTROL_SYNC_MULT:
        case ZDJ_DECK_2_CONTROL_SCRUB:
        case ZDJ_DECK_2_CONTROL_TEMPO:
        case ZDJ_DECK_2_CONTROL_TEMPO_FINE:
        case ZDJ_DECK_2_CONTROL_PLAY_PAUSE:
        case ZDJ_DECK_2_CONTROL_PAUSE:
        case ZDJ_DECK_2_CONTROL_HOTCUE_START:
        case ZDJ_DECK_2_CONTROL_HOTCUE_END: return station == ZDJ_DECK_STATION_2;


        case ZDJ_DECK_EXT_CONTROL_TRIM:
        case ZDJ_DECK_EXT_CONTROL_EQ_LO:
        case ZDJ_DECK_EXT_CONTROL_EQ_MID:
        case ZDJ_DECK_EXT_CONTROL_EQ_HI:
        case ZDJ_DECK_EXT_CONTROL_PFL_TRIM:
        case ZDJ_DECK_EXT_CONTROL_PFL_TOGGLE_MUTE:
        case ZDJ_DECK_EXT_CONTROL_LOOP_TOGGLE:
        case ZDJ_DECK_EXT_CONTROL_LOOP_ON:
        case ZDJ_DECK_EXT_CONTROL_LOOP_OFF:
        case ZDJ_DECK_EXT_CONTROL_LOOP_START:
        case ZDJ_DECK_EXT_CONTROL_LOOP_END:
        case ZDJ_DECK_EXT_CONTROL_LOOP_LENGTH:
        case ZDJ_DECK_EXT_CONTROL_SKIP:
        case ZDJ_DECK_EXT_CONTROL_SKIP_LENGTH:
        case ZDJ_DECK_EXT_CONTROL_SKIP_SET_ORIGIN:
        case ZDJ_DECK_EXT_CONTROL_SKIP_RESET_TO_ORIGIN:
        case ZDJ_DECK_EXT_CONTROL_FX_SELECT:
        case ZDJ_DECK_EXT_CONTROL_FX_0:
        case ZDJ_DECK_EXT_CONTROL_FX_1:
        case ZDJ_DECK_EXT_CONTROL_FX_2:
        case ZDJ_DECK_EXT_CONTROL_FX_3:
        case ZDJ_DECK_EXT_CONTROL_FX_4:
        case ZDJ_DECK_EXT_CONTROL_FX_5:
        case ZDJ_DECK_EXT_CONTROL_SCRUB: return station == ZDJ_DECK_STATION_EXT;

        case ZDJ_DECK_XPORT_CONTROL_SCRUB:
        case ZDJ_DECK_XPORT_CONTROL_SYNC_MULT:
        case ZDJ_DECK_XPORT_CONTROL_TEMPO:
        case ZDJ_DECK_XPORT_CONTROL_TEMPO_FINE:
        case ZDJ_DECK_XPORT_CONTROL_PLAY_PAUSE:
        case ZDJ_DECK_XPORT_CONTROL_PAUSE:
        case ZDJ_DECK_XPORT_CONTROL_HOTCUE_START:
        case ZDJ_DECK_XPORT_CONTROL_HOTCUE_STOP:
        case ZDJ_DECK_XPORT_CONTROL_HOTCUE_END: return station == ZDJ_DECK_STATION_XPORT;

        // Everyone hears these
        case ZDJ_DECK_CONTROL_SYNC_TOGGLE:
        case ZDJ_DECK_CONTROL_SYNC_ENABLE:
        case ZDJ_DECK_CONTROL_SYNC_DISABLE:return true;

        default: return false;
    }
}

static bool _is_soundcard_event( zdj_control_event_t * event ) {
    switch ( event->id ) {
        case ZDJ_DECK_CONTROL_LR_VOL:
        case ZDJ_DECK_CONTROL_CUE_VOL:
        case ZDJ_DECK_CONTROL_TOGGLE_RECORD: 
        case ZDJ_DECK_CONTROL_XFADE: return true;

        default: return false;
    }
}