
#include "MemoryManager.h"

#include <stdlib.h>

#ifdef APEMODE_USE_MEMORY_TRACKING
#define USE_MEMORY_TRACKING 1
#endif

#ifdef APEMODE_USE_DLMALLOC
#define USE_DLMALLOC 1
#endif

#include <new>
#include <time.h>
#include <cstdlib>
#include <cassert>

#ifdef USE_DLMALLOC

#define MSPACES 1
#define USE_DL_PREFIX 1
#define MALLOC_ALIGNMENT ( apemode::kAlignment )
#include "dlmalloc.cc"

static thread_local mspace tlms = create_mspace( 0, 0 );

namespace apememint {
    void *allocate( size_t size ) {
#if defined( APEMODE_USE_MEMORY_TRACKING )
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

} // namespace apememint

#endif

/* @see https://gist.github.com/prasadwrites/5ada6db9ce8d0d47b93e */
namespace apememext {
    constexpr size_t ptr_size = sizeof( void * );

    /* calculate nearest power of 2 */
    uint32_t nearest_pow2( uint32_t v ) {
        v |= ( v - 1 ) >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        return v + 1;
    }

    /* Make sure it is at least default and is power of 2 */
    size_t calculate_alignment( const size_t alignment ) {
        return APEMODE_DEFAULT_ALIGNMENT >= alignment ? APEMODE_DEFAULT_ALIGNMENT
                                                      : nearest_pow2( static_cast< uint32_t >( alignment ) );
    }

    /* Accomodate at least one aligned area by adding (alignment - 1)
     * Add pointer size to store the original not aligned pointer */
    size_t calculate_size( const size_t size, const size_t alignment ) {
        return size + alignment - 1 + ptr_size;
    }

#if defined( APEMODE_USE_MEMORY_TRACKING )

#define apemode_internal_malloc( reportedSize ) \
    m_allocator( sourceFile, sourceLine, sourceFunc, allocationType, ( reportedSize ) )

#define apemode_internal_realloc( reportedAddress, reportedSize ) \
    m_reallocator( sourceFile, sourceLine, sourceFunc, allocationType, ( reportedSize ), ( reportedAddress ) )

#define apemode_internal_free( reportedAddress ) \
    m_deallocator( sourceFile, sourceLine, sourceFunc, deallocationType, ( reportedAddress ) )
#else

#define apemode_internal_malloc( reportedSize, flags ) apememint::allocate( reportedSize )
#define apemode_internal_realloc( reportedAddress, reportedSize, flags ) apememint::reallocate( reportedAddress, reportedSize )
#define apemode_internal_free( reportedAddress, flags ) apememint::deallocate( reportedAddress )

#endif

    void *aligned_malloc( const size_t       size,
                          const size_t       requestedAlignment,
                          const char *       sourceFile,
                          const unsigned int sourceLine,
                          const char *       sourceFunc,
                          const unsigned int allocationType ) {

        if ( 0 == size ) {
            return nullptr;
        }

        const size_t alignment = calculate_alignment( requestedAlignment );
        if ( void *p = apemode_internal_malloc( calculate_size( size, alignment ) ) ) {
            /* Address of the aligned memory according to the align parameter*/
            void * ptr = (void *) ( ( (size_t) p + ptr_size + ( alignment - 1 ) ) & ~( alignment - 1 ) );

            /* Store the address of the malloc() above the beginning of our total memory area. */
            *( (void **) ( (size_t) ptr - ptr_size ) ) = p;

            /* Return the address of aligned memory */
            return ptr;
        }

        assert( false );
        return nullptr;
    }

    void *aligned_realloc( void *             p,
                           const size_t       size,
                           const size_t       requestedAlignment,
                           const char *       sourceFile,
                           const unsigned int sourceLine,
                           const char *       sourceFunc,
                           const unsigned int allocationType ) {
                              
        if ( 0 == size ) {
            return nullptr;
        }

        if ( nullptr == p ) {
            return aligned_malloc( size, requestedAlignment, sourceFile, sourceLine, sourceFunc, allocationType );
        }

        const size_t alignment = calculate_alignment( requestedAlignment );
        void *ptr = *( (void **) ( (size_t) p - sizeof( void * ) ) );

        if ( p = apemode_internal_realloc( ptr, calculate_size( size, alignment ) ) ) {
            /* Address of the aligned memory according to the align parameter*/
            ptr = (void *) ( ( (size_t) p + ptr_size + ( alignment - 1 ) ) & ~( alignment - 1 ) );

            /* Store the address of the malloc() above the beginning of our total memory area. */
            *( (void **) ( (size_t) ptr - ptr_size ) ) = p;

            /* Return the address of aligned memory */
            return ptr;
        }

        assert( false );
        return nullptr;
    }

    void aligned_free( void *             p,
                       const char *       sourceFile,
                       const unsigned int sourceLine,
                       const char *       sourceFunc,
                       const unsigned int deallocationType ) {
        if ( p ) {
            /* Get the address of the memory, stored at the start of our total memory area. */
            void *ptr = *( (void **) ( (size_t) p - sizeof( void * ) ) );
            return apemode_internal_free( ptr );
        }
    }
}

void *apemode::allocate( size_t size, size_t alignment, const char *sourceFile, const unsigned int sourceLine, const char *sourceFunc ) {
    return apememext::aligned_malloc( size, alignment, sourceFile, sourceLine, sourceFunc, m_alloc_malloc );
}

void *apemode::callocate( size_t num, size_t size, size_t alignment, const char *sourceFile, const unsigned int sourceLine, const char *sourceFunc ) {
    return memset( apememext::aligned_malloc( num * size, alignment, sourceFile, sourceLine, sourceFunc, m_alloc_calloc ), 0, num * size );
}

void *apemode::reallocate( void *p, size_t size, size_t alignment, const char *sourceFile, const unsigned int sourceLine, const char *sourceFunc ) {
    return apememext::aligned_realloc( p, size, alignment, sourceFile, sourceLine, sourceFunc, m_alloc_realloc );
}

void apemode::deallocate( void *p, const char *sourceFile, const unsigned int sourceLine, const char *sourceFunc ) {
    return apememext::aligned_free( p, sourceFile, sourceLine, sourceFunc, m_alloc_free );
}

void *operator new[]( std::size_t size, std::nothrow_t const & ) noexcept {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, __FILE__, __LINE__, __FUNCTION__, m_alloc_new_array );
}

void *operator new[]( std::size_t size ) _THROW_BAD_ALLOC {
// void *operator new[]( std::size_t size ) throw( ) {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, __FILE__, __LINE__, __FUNCTION__, m_alloc_new_array );
}

void *operator new( std::size_t size, std::nothrow_t const & ) noexcept {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, __FILE__, __LINE__, __FUNCTION__, m_alloc_new );
}

void *operator new( std::size_t size ) _THROW_BAD_ALLOC {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, __FILE__, __LINE__, __FUNCTION__, m_alloc_new );
}

void operator delete[]( void *p ) _THROW_BAD_ALLOC {
    return apememext::aligned_free( p, __FILE__, __LINE__, __FUNCTION__, m_alloc_delete_array );
}

void operator delete( void *p ) throw( ) {
    return apememext::aligned_free( p, __FILE__, __LINE__, __FUNCTION__, m_alloc_delete );
}

void *operator new[]( std::size_t           size,
                      std::nothrow_t const &nothrow,
                      const char *          sourceFile,
                      const unsigned int    sourceLine,
                      const char *          sourceFunc ) noexcept {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, sourceFile, sourceLine, sourceFunc, m_alloc_new_array );
}

void *operator new[]( std::size_t        size,
                      const char *       sourceFile,
                      const unsigned int sourceLine,
                      const char *       sourceFunc ) _THROW_BAD_ALLOC {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, sourceFile, sourceLine, sourceFunc, m_alloc_new_array );
}

void *operator new( std::size_t           size,
                    std::nothrow_t const &nothrow,
                    const char *          sourceFile,
                    const unsigned int    sourceLine,
                    const char *          sourceFunc ) noexcept {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, __FILE__, __LINE__, __FUNCTION__, m_alloc_new );
}

void *operator new( std::size_t size, const char *sourceFile, const unsigned int sourceLine, const char *sourceFunc ) throw( ) {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, __FILE__, __LINE__, __FUNCTION__, m_alloc_new );
}

void operator delete[]( void *p, const char *sourceFile, const unsigned int sourceLine, const char *sourceFunc ) throw( ) {
    return apememext::aligned_free( p, __FILE__, __LINE__, __FUNCTION__, m_alloc_delete_array );
}

void operator delete( void *p, const char *sourceFile, const unsigned int sourceLine, const char *sourceFunc ) throw( ) {
    return apememext::aligned_free( p, __FILE__, __LINE__, __FUNCTION__, m_alloc_delete );
}

#if defined( USE_MEMORY_TRACKING )

#undef apemode_malloc
#undef apemode_calloc
#undef apemode_realloc
#undef apemode_free

#define apemode_malloc  apememint::allocate
#define apemode_realloc apememint::reallocate
#define apemode_calloc  apememint::callocate
#define apemode_free    apememint::deallocate

// Just include the cpp here so we don't have to add it to all projects
#include "FluidStudios/MemoryManager/mmgr.cpp"

#else

void *apemode_malloc( size_t size, size_t alignment ) {
    return apemode::allocate( size, alignment, __FILE__, __LINE__, __FUNCTION__ );
}

void *apemode_calloc( size_t count, size_t size, size_t alignment ) {
    return apemode::callocate( count, size, alignment, __FILE__, __LINE__, __FUNCTION__ );
}

void *apemode_realloc( void *p, size_t size, size_t alignment ) {
    return apemode::reallocate( p, size, alignment, __FILE__, __LINE__, __FUNCTION__ );
}

void apemode_free( void *p ) {
    apemode::deallocate( p, __FILE__, __LINE__, __FUNCTION__ );
}

#endif