#include <zerodj/system/m7/zdj_platform.h>

#ifdef ZDJ_EMU

// Emulator backend: there is no /dev/mem and no M7 co-processor. Hand out
// process-local buffers, one per physical address, shared by every caller
// (library modules and the zero-emu harness alike).

#include <stdlib.h>
#include <pthread.h>

#define ZDJ_EMU_MAX_REGIONS 16

static struct {
    unsigned long addr;
    size_t        len;
    void *        ptr;
} _regions[ ZDJ_EMU_MAX_REGIONS ];
static int _region_count = 0;
static pthread_mutex_t _lock = PTHREAD_MUTEX_INITIALIZER;

void * zdj_platform_map_shared( unsigned long phys_addr, size_t len ) {
    pthread_mutex_lock( &_lock );

    void * out = NULL;
    for( int i = 0; i < _region_count; i++ ) {
        if( _regions[ i ].addr == phys_addr ) {
            // Grow the backing buffer if a later caller wants more of this region.
            if( len > _regions[ i ].len ) {
                _regions[ i ].ptr = realloc( _regions[ i ].ptr, len );
                _regions[ i ].len = len;
            }
            out = _regions[ i ].ptr;
            break;
        }
    }

    if( !out && _region_count < ZDJ_EMU_MAX_REGIONS ) {
        out = calloc( 1, len );
        _regions[ _region_count ].addr = phys_addr;
        _regions[ _region_count ].len  = len;
        _regions[ _region_count ].ptr  = out;
        _region_count++;
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
