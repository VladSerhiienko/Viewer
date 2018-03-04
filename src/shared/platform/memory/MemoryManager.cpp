
#include "MemoryManager.h"

#ifdef APEMODE_USE_MEMORY_TRACKING
#define USE_MEMORY_TRACKING 1
#endif

#ifdef APEMODE_USE_DLMALLOC
#define USE_DLMALLOC 1
#endif

void *operator new( size_t size ) {
    return apemode_malloc( size );
}

void *operator new[]( size_t size ) {
    return apemode_malloc( size );
}

void *operator new[]( size_t size,
                      const char *  /*name*/,
                      int           /*flags*/,
                      unsigned      /*debugFlags*/,
                      const char *  /*file*/,
                      int           /*line*/ ) {
    return apemode_malloc( size );
}

void *operator new[]( size_t size,
                      size_t        /*alignment*/,
                      size_t        /*alignmentOffset*/,
                      const char *  /*name*/,
                      int           /*flags*/,
                      unsigned      /*debugFlags*/,
                      const char *  /*file*/,
                      int           /*line*/ ) {
    return apemode_malloc( size );
}

void *operator new( size_t size, size_t /*alignment*/ ) {
    return apemode_malloc( size );
}

void *operator new( size_t size, size_t /*alignment*/, const std::nothrow_t & ) throw( ) {
    return apemode_malloc( size );
}

void *operator new[]( size_t size, size_t /*alignment*/ ) {
    return apemode_malloc( size );
}

void *operator new[]( size_t size, size_t /*alignment*/, const std::nothrow_t & ) throw( ) {
    return apemode_malloc( size );
}

// C++14 deleter
void operator delete( void *p, std::size_t /*sz*/ ) throw( ) {
    apemode_free( p );
}

void operator delete[]( void *p, std::size_t /*sz*/ ) throw( ) {
    apemode_free( p );
}

void operator delete( void *p ) throw( ) {
    apemode_free( p );
}

void operator delete[]( void *p ) throw( ) {
    apemode_free( p );
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

#elif defined( __ANDROID__ )
        void *p = nullptr;
        ::posix_memalign( &p, alignment, size );
        return p;
#else
        (void) alignment;
        return ::malloc( size );
        //return ::_aligned_malloc( size, alignment );
#endif
    }

    void *callocate( size_t num, size_t size ) {
#if defined( USE_DLMALLOC )

        return dlcalloc( num, size );

#elif defined( __ANDROID__ )
        static_assert(true, "Not implemented.");
#else
        (void) alignment;
        return ::calloc( num, size );
        //return ::_aligned_calloc( num, size, alignment );
#endif
    }

    void *reallocate( void *p, size_t size ) {
#if defined( USE_DLMALLOC )

        return dlrealloc( p, size );

#elif defined( __ANDROID__ )
        static_assert( true, "Not implemented." );
#else
        (void) alignment;
        return ::realloc( p, size );
        //return ::_aligned_realloc( p, size, alignment );
#endif
    }

    void deallocate( void *p ) {
#if defined( USE_DLMALLOC )

        return dlfree( p );

#elif defined( __ANDROID__ )
        ::free( p );
#else
        ::free( p );
        //::_aligned_free( p );
#endif
    }

    void *threadLocalAllocate( size_t size ) {
        return mspace_malloc( tlms, size );
    }

    void *threadLocalReallocate( void *p, size_t size ) {
        return mspace_realloc( tlms, p, size );
    }

    void threadLocalDeallocate( void *p ) {
        mspace_free( tlms, p );
    }
}

#if defined( USE_MEMORY_TRACKING )
// Just include the cpp here so we don't have to add it to the all projects
#include "FluidStudios/MemoryManager/mmgr.cpp"
#endif