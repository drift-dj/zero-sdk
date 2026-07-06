#include <zerodj/system/m7/zdj_platform.h>

#ifdef ZDJ_EMU

// Emulator backend: there is no /dev/mem and no M7 co-processor. Hand out
// process-local buffers, one per physical address, shared by every caller
// (library modules and the zero-emu harness alike).

#include <stdio.h>
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
            // A region's size is fixed by its first mapping. Growing it would
            // realloc (and possibly move) a buffer earlier callers already
            // hold pointers into -- a silent use-after-free. On the device
            // every region is a fixed physical window anyway, so a size
            // mismatch here is a caller bug: fail loudly instead.
            if( len > _regions[ i ].len ) {
                fprintf( stderr,
                    "zdj_platform_map_shared: region 0x%lx first mapped as %zu bytes, "
                    "later caller wants %zu -- map the largest size first\n",
                    phys_addr, _regions[ i ].len, len );
                abort( );
            }
            out = _regions[ i ].ptr;
            break;
        }
    }

    if( !out ) {
        if( _region_count == ZDJ_EMU_MAX_REGIONS ) {
            fprintf( stderr, "zdj_platform_map_shared: out of region slots "
                "(ZDJ_EMU_MAX_REGIONS=%d) mapping 0x%lx\n", ZDJ_EMU_MAX_REGIONS, phys_addr );
            abort( );
        }
        out = calloc( 1, len );
        if( !out ) {
            fprintf( stderr, "zdj_platform_map_shared: calloc(%zu) failed for 0x%lx\n",
                len, phys_addr );
            abort( );
        }
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
