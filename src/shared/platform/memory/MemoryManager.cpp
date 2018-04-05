
#include "MemoryManager.h"

#ifdef APEMODE_USE_MEMORY_TRACKING
#define USE_MEMORY_TRACKING 1
#endif

#ifdef APEMODE_USE_DLMALLOC
#define USE_DLMALLOC 1
#endif

void *operator new[]( std::size_t s, std::nothrow_t const & ) noexcept {
    return apemode_calloc( 1, s );
    // return apemode_malloc( s );
}

void *operator new[]( std::size_t s ) throw( std::bad_alloc ) {
    return apemode_calloc( 1, s );
    // return apemode_malloc( s );
}

void operator delete[]( void *p ) throw( ) {
    return apemode_free( p );
}

void *operator new( std::size_t s, std::nothrow_t const & ) noexcept {
    return apemode_calloc( 1, s );
    // return apemode_malloc( s );
}

void *operator new( std::size_t s ) throw( std::bad_alloc ) {
    return apemode_calloc( 1, s );
    // return apemode_malloc( s );
}

void operator delete( void *p ) throw( ) {
    return apemode_free( p );
}

#undef apemode_malloc
#undef apemode_calloc
#undef apemode_realloc
#undef apemode_free

void *apemode_malloc( size_t size ) {
    return apemode::allocate( size );
}

void *apemode_calloc( size_t count, size_t size ) {
    return apemode::callocate( count, size );
}

void *apemode_realloc( void *p, size_t size ) {
    return apemode::reallocate( p, size );
}

void apemode_free( void *p ) {
    apemode::deallocate( p );
}

#include <new>
#include <time.h>
#include <cstdlib>

#ifdef USE_DLMALLOC

#define MSPACES 1
#define USE_DL_PREFIX 1
#define MALLOC_ALIGNMENT ( apemode::kAlignment )
#include "dlmalloc.cc"

static thread_local mspace tlms = create_mspace( 0, 0 );

#endif

namespace apemode {
    void *allocate( size_t size ) {
#if defined( USE_DLMALLOC )
        return dlmalloc( size );
#else
        return malloc( size );
#endif
    }

    void *callocate( size_t num, size_t size ) {
#if defined( USE_DLMALLOC )
        return dlcalloc( num, size );
#else
        return calloc( num, size );
#endif
    }

    void *reallocate( void *p, size_t size ) {
#if defined( USE_DLMALLOC )

        return dlrealloc( p, size );

#elif defined( __ANDROID__ )
        static_assert( true, "Not implemented." );
#else
        return realloc( p, size );
#endif
    }

    void deallocate( void *p ) {
#if defined( USE_DLMALLOC )
        return dlfree( p );
#else
        return free( p );
#endif
    }

#ifdef APEMODE_USE_DLMALLOC

    void *threadLocalAllocate( size_t size ) {
        return mspace_malloc( tlms, size );
    }

    void *threadLocalReallocate( void *p, size_t size ) {
        return mspace_realloc( tlms, p, size );
    }

    void threadLocalDeallocate( void *p ) {
        mspace_free( tlms, p );
    }

#endif
}

#if defined( USE_MEMORY_TRACKING )
// Just include the cpp here so we don't have to add it to the all projects
#include "FluidStudios/MemoryManager/mmgr.cpp"
#endif