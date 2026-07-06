#include <zerodj/system/m7/zdj_platform.h>

#ifdef ZDJ_EMU

// Emulator backend: there is no /dev/mem and no M7 co-processor. Hand out
// process-local buffers, one per shared region, shared by every caller
// (library modules and the zero-emu harness alike).

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <zerodj/controls/hmi/zdj_hmi_m7_state_model.h>
#include <zerodj/system/m7/zdj_m7.h>

// The A53<->M7 shared regions the device tree carves out of reserved memory
// (see the zero kernel's 'drift-a106.dtsi'). Each is backed by one buffer of
// the region's full size, allocated on first map; callers may map any prefix
// of a region and all get the same buffer. An address outside this table, or
// a length beyond the region, is a caller bug -- fail loudly rather than hand
// back memory nobody else shares.
static struct {
    unsigned long addr;
    size_t        len;
    void *        ptr;
} _regions[ ] = {
    { ZDJ_SHARED_HMI_STATE_ADDR,   0x20000, NULL },  // HMI input state model
    { ZDJ_SHARED_VIDEO_BUF_ADDR,   0x2000,  NULL },  // packed 128x64 display
    { ZDJ_SHARED_AUDIO_STATE_ADDR, 0x1000,  NULL },  // audio cycle handshake
    { ZDJ_SHARED_DAC_BUF,          0x8000,  NULL },  // DAC sample buffer
    { ZDJ_SHARED_ADC_BUF,          0x8000,  NULL },  // ADC sample buffer
    { ZDJ_SHARED_MSG_BUF_ADDR,     0x1000,  NULL },  // A53<->M7 message/req buffer
};
#define ZDJ_EMU_REGION_COUNT ( sizeof( _regions ) / sizeof( _regions[ 0 ] ) )

static pthread_mutex_t _lock = PTHREAD_MUTEX_INITIALIZER;

void * zdj_platform_map_shared( unsigned long phys_addr, size_t len ) {
    pthread_mutex_lock( &_lock );

    void * out = NULL;
    for( size_t i = 0; i < ZDJ_EMU_REGION_COUNT; i++ ) {
        if( _regions[ i ].addr != phys_addr ) { continue; }
        if( len > _regions[ i ].len ) {
            fprintf( stderr,
                "zdj_platform_map_shared: 0x%lx is a %zu-byte region, caller "
                "wants %zu -- grow the region table\n",
                phys_addr, _regions[ i ].len, len );
            abort( );
        }
        if( !_regions[ i ].ptr ) {
            _regions[ i ].ptr = calloc( 1, _regions[ i ].len );
            if( !_regions[ i ].ptr ) {
                fprintf( stderr, "zdj_platform_map_shared: calloc(%zu) failed for 0x%lx\n",
                    _regions[ i ].len, phys_addr );
                abort( );
            }
        }
        out = _regions[ i ].ptr;
        break;
    }

    if( !out ) {
        fprintf( stderr, "zdj_platform_map_shared: 0x%lx is not an emulated "
            "shared region -- add it to the table in zdj_platform.c\n", phys_addr );
        abort( );
    }

    pthread_mutex_unlock( &_lock );
    return out;
}

#else

// Device backend: map the M7 shared RAM out of /dev/mem, exactly as each
// module did inline before the seam existed.

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

void * zdj_platform_map_shared( unsigned long phys_addr, size_t len ) {
    int mem_fd = open( "/dev/mem", O_RDWR );
    void * ptr = mmap( 0, len, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, phys_addr );
    return ptr;
}

#endif
