
#include "MemoryManager.h"

#include <stdio.h>

#ifdef APEMODE_USE_MEMORY_TRACKING
#define USE_MEMORY_TRACKING 1
#endif

#ifdef APEMODE_USE_DLMALLOC
#define USE_DLMALLOC 1
#endif

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
    m_allocator( pszSourceFile, sourceLine, pszSourceFunc, allocationType, ( reportedSize ) )

#define apemode_internal_realloc( reportedAddress, reportedSize ) \
    m_reallocator( pszSourceFile, sourceLine, pszSourceFunc, allocationType, ( reportedSize ), ( reportedAddress ) )

#define apemode_internal_free( reportedAddress ) \
    m_deallocator( pszSourceFile, sourceLine, pszSourceFunc, deallocationType, ( reportedAddress ) )
#else

#define apemode_internal_malloc( reportedSize, flags ) apememint::allocate( reportedSize )
#define apemode_internal_realloc( reportedAddress, reportedSize, flags ) apememint::reallocate( reportedAddress, reportedSize )
#define apemode_internal_free( reportedAddress, flags ) apememint::deallocate( reportedAddress )

#endif

    void *aligned_malloc( const size_t       size,
                          const size_t       requestedAlignment,
                          const char *       pszSourceFile,
                          const unsigned int sourceLine,
                          const char *       pszSourceFunc,
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

            char cc[ 128 ] = {0};
            sprintf( cc, "%p", p );

            /* Return the address of aligned memory */
            return ptr;
        }

        assert( false );
        return nullptr;
    }

    void *aligned_realloc( void *             p,
                           const size_t       size,
                           const size_t       requestedAlignment,
                           const char *       pszSourceFile,
                           const unsigned int sourceLine,
                           const char *       pszSourceFunc,
                           const unsigned int allocationType ) {

        if ( 0 == size ) {
            return nullptr;
        }

        if ( nullptr == p ) {
            return aligned_malloc( size, requestedAlignment, pszSourceFile, sourceLine, pszSourceFunc, allocationType );
        }

        const size_t alignment = calculate_alignment( requestedAlignment );
        void *ptr = *( (void **) ( (size_t) p - sizeof( void * ) ) );

        if ( ( p = apemode_internal_realloc( ptr, calculate_size( size, alignment ) ) ) ) {
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

    void aligned_free( void *             ptr,
                       const char *       pszSourceFile,
                       const unsigned int sourceLine,
                       const char *       pszSourceFunc,
                       const unsigned int deallocationType ) {
        if ( ptr ) {
            /* Get the address of the memory, stored at the start of our total memory area. */
            void *p = *( (void **) ( (size_t) ptr - sizeof( void * ) ) );

            char cc[ 128 ] = {0};
            sprintf( cc, "%p", p );

            return apemode_internal_free( p );
        }
    }
}

void *apemode::allocate( size_t size, size_t alignment, const char *pszSourceFile, const unsigned int sourceLine, const char *pszSourceFunc ) {
    return apememext::aligned_malloc( size, alignment, pszSourceFile, sourceLine, pszSourceFunc, m_alloc_malloc );
}

void *apemode::callocate( size_t num, size_t size, size_t alignment, const char *pszSourceFile, const unsigned int sourceLine, const char *pszSourceFunc ) {
    return memset( apememext::aligned_malloc( num * size, alignment, pszSourceFile, sourceLine, pszSourceFunc, m_alloc_calloc ), 0, num * size );
}

void *apemode::reallocate( void *pMemory, size_t size, size_t alignment, const char *pszSourceFile, const unsigned int sourceLine, const char *pszSourceFunc ) {
    return apememext::aligned_realloc( pMemory, size, alignment, pszSourceFile, sourceLine, pszSourceFunc, m_alloc_realloc );
}

void apemode::deallocate( void *pMemory, const char *pszSourceFile, const unsigned int sourceLine, const char *pszSourceFunc ) {
    return apememext::aligned_free( pMemory, pszSourceFile, sourceLine, pszSourceFunc, m_alloc_free );
}

#if 0 // __GNUC__

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BT_BUF_SIZE 100

#define APEMODE_SUPPORT_CALLER_LOCATION

void WriteCallerLocation( char *pszBuffer, size_t bufferSize, void *pReturnAddress ) {
    // void * buffer[ BT_BUF_SIZE ] = {0};

    // int    nptrs = backtrace( buffer, BT_BUF_SIZE );
    // char **strings = backtrace_symbols( buffer, nptrs );

    // for ( int j = 0; strings && j < nptrs; j++ )
    //     if ( buffer[ j ] == pReturnAddress ) {
    //         printf( "%s\n", strings[ j ] );
    //         sprintf( pszBuffer, "%s", strings[ j ] );
    //         free( strings );
    //         return;
    //     }

    // if ( strings )
    //     free( strings );
    // snprintf( pszBuffer, sizeof( bufferSize ), "%p", pReturnAddress );
}

#define APEMODE_NEW_GET_CALLER_FUNCTION \
    char szCallerBuffer[ 512 ] = {0};   \
    WriteCallerLocation( szCallerBuffer, sizeof( szCallerBuffer ), __builtin_return_address( 0 ) )

#define APEMODE_NEW_CALLER_FUNCTION_NAME szCallerBuffer

#else

#define APEMODE_NEW_GET_CALLER_FUNCTION
#define APEMODE_NEW_CALLER_FUNCTION_NAME __FUNCTION__

#endif

void *operator new[]( std::size_t size, std::nothrow_t const & ) APEMODE_NO_EXCEPT {
    APEMODE_NEW_GET_CALLER_FUNCTION;
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, __FILE__, __LINE__, APEMODE_NEW_CALLER_FUNCTION_NAME, m_alloc_new_array );
}

void *operator new[]( std::size_t size ) APEMODE_THROW_BAD_ALLOC {
    APEMODE_NEW_GET_CALLER_FUNCTION;
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, __FILE__, __LINE__, APEMODE_NEW_CALLER_FUNCTION_NAME, m_alloc_new_array );
}

void *operator new( std::size_t size, std::nothrow_t const & ) APEMODE_NO_EXCEPT {
    APEMODE_NEW_GET_CALLER_FUNCTION;
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, __FILE__, __LINE__, APEMODE_NEW_CALLER_FUNCTION_NAME, m_alloc_new );
}

void *operator new( std::size_t size ) APEMODE_THROW_BAD_ALLOC {
    APEMODE_NEW_GET_CALLER_FUNCTION;
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, __FILE__, __LINE__, APEMODE_NEW_CALLER_FUNCTION_NAME, m_alloc_new );
}

void operator delete[]( void *pMemory ) APEMODE_NO_EXCEPT {
    APEMODE_NEW_GET_CALLER_FUNCTION;
    return apememext::aligned_free( pMemory, __FILE__, __LINE__, APEMODE_NEW_CALLER_FUNCTION_NAME, m_alloc_delete_array );
}

void operator delete( void *pMemory ) APEMODE_NO_EXCEPT {
    APEMODE_NEW_GET_CALLER_FUNCTION;
    return apememext::aligned_free( pMemory, __FILE__, __LINE__, APEMODE_NEW_CALLER_FUNCTION_NAME, m_alloc_delete );
}

void *operator new[]( std::size_t             size,
                      std::nothrow_t const &  nothrow,
                      apemode::EAllocationTag eAllocTag,
                      const char *            pszSourceFile,
                      const unsigned int      sourceLine,
                      const char *            pszSourceFunc ) APEMODE_NO_EXCEPT {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, pszSourceFile, sourceLine, pszSourceFunc, m_alloc_new_array );
}

void *operator new[]( std::size_t             size,
                      apemode::EAllocationTag eAllocTag,
                      const char *            pszSourceFile,
                      const unsigned int      sourceLine,
                      const char *            pszSourceFunc ) APEMODE_THROW_BAD_ALLOC {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, pszSourceFile, sourceLine, pszSourceFunc, m_alloc_new_array );
}

void *operator new( std::size_t             size,
                    std::nothrow_t const &  nothrow,
                    apemode::EAllocationTag eAllocTag,
                    const char *            pszSourceFile,
                    const unsigned int      sourceLine,
                    const char *            pszSourceFunc ) APEMODE_NO_EXCEPT {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, __FILE__, __LINE__, __FUNCTION__, m_alloc_new );
}

void *operator new( std::size_t             size,
                    apemode::EAllocationTag eAllocTag,
                    const char *            pszSourceFile,
                    const unsigned int      sourceLine,
                    const char *            pszSourceFunc ) APEMODE_THROW_BAD_ALLOC {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, __FILE__, __LINE__, __FUNCTION__, m_alloc_new );
}

void operator delete[]( void *                  pMemory,
                        apemode::EAllocationTag eAllocTag,
                        const char *            pszSourceFile,
                        const unsigned int      sourceLine,
                        const char *            pszSourceFunc ) APEMODE_NO_EXCEPT {
    return apememext::aligned_free( pMemory, __FILE__, __LINE__, __FUNCTION__, m_alloc_delete_array );
}

void operator delete( void *                  pMemory,
                      apemode::EAllocationTag eAllocTag,
                      const char *            pszSourceFile,
                      const unsigned int      sourceLine,
                      const char *            pszSourceFunc ) APEMODE_NO_EXCEPT {
    return apememext::aligned_free( pMemory, __FILE__, __LINE__, __FUNCTION__, m_alloc_delete );
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