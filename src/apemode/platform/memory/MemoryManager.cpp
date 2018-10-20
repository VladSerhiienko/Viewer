
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
#if __APPLE__
#define HAVE_MORECORE 0
#endif
#define USE_DL_PREFIX 1
#define MALLOC_ALIGNMENT ( apemode::platform::kAlignment )
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
        void *ptr = (void *) ( ( (size_t) p + ptr_size + ( alignment - 1 ) ) & ~( alignment - 1 ) );

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
    void *       ptr       = *( (void **) ( (size_t) p - sizeof( void * ) ) );

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
} // namespace apememext

void *apemode::platform::allocate( size_t size, size_t alignment, const char *pszSourceFile, const unsigned int sourceLine, const char *pszSourceFunc ) {
    return apememext::aligned_malloc( size, alignment, pszSourceFile, sourceLine, pszSourceFunc, m_alloc_malloc );
}

void *apemode::platform::callocate( size_t             num,
                                    size_t             size,
                                    size_t             alignment,
                                    const char *       pszSourceFile,
                                    const unsigned int sourceLine,
                                    const char *       pszSourceFunc ) {
    return memset( apememext::aligned_malloc( num * size, alignment, pszSourceFile, sourceLine, pszSourceFunc, m_alloc_calloc ),
                   0,
                   num * size );
}

void *apemode::platform::reallocate( void *             pMemory,
                                     size_t             size,
                                     size_t             alignment,
                                     const char *       pszSourceFile,
                                     const unsigned int sourceLine,
                                     const char *       pszSourceFunc ) {
    return apememext::aligned_realloc( pMemory, size, alignment, pszSourceFile, sourceLine, pszSourceFunc, m_alloc_realloc );
}

void apemode::platform::deallocate( void *             pMemory,
                                    const char *       pszSourceFile,
                                    const unsigned int sourceLine,
                                    const char *       pszSourceFunc ) {
    return apememext::aligned_free( pMemory, pszSourceFile, sourceLine, pszSourceFunc, m_alloc_free );
}

#ifndef APEMODE_NO_MEMORY_ALLOCATION_SCOPES
#define APEMODE_ENABLE_MEMORY_ALLOCATION_SCOPES
#endif

#ifdef APEMODE_ENABLE_MEMORY_ALLOCATION_SCOPES

thread_local apemode::platform::MemoryAllocationScope::CodeLocation gCurrLocation;

void apemode::platform::GetPrevMemoryAllocationScope( const char *& pszSourceFile,
                                                      unsigned int &sourceLine,
                                                      const char *& pszSourceFunc ) {
    pszSourceFile = gCurrLocation.pszSourceFile;
    sourceLine    = gCurrLocation.SourceLine;
    pszSourceFunc = gCurrLocation.pszSourceFunc;
}

void apemode::platform::StartMemoryAllocationScope( const char *       pszSourceFile,
                                                    const unsigned int sourceLine,
                                                    const char *       pszSourceFunc ) {
    gCurrLocation.pszSourceFile = pszSourceFile;
    gCurrLocation.SourceLine    = sourceLine;
    gCurrLocation.pszSourceFunc = pszSourceFunc;
}

void apemode::platform::EndMemoryAllocationScope( const char *       pszSourceFile,
                                                  const unsigned int sourceLine,
                                                  const char *       pszSourceFunc ) {
    gCurrLocation.pszSourceFile = pszSourceFile;
    gCurrLocation.SourceLine    = sourceLine;
    gCurrLocation.pszSourceFunc = pszSourceFunc;
}

apemode::platform::MemoryAllocationScope::MemoryAllocationScope( const char *       pszSourceFile,
                                                       const unsigned int sourceLine,
                                                       const char *       pszSourceFunc ) {
    GetPrevMemoryAllocationScope( PrevLocation.pszSourceFile, PrevLocation.SourceLine, PrevLocation.pszSourceFunc );
    StartMemoryAllocationScope( pszSourceFile, sourceLine, pszSourceFunc );
}

apemode::platform::MemoryAllocationScope::~MemoryAllocationScope( ) {
    EndMemoryAllocationScope( PrevLocation.pszSourceFile, PrevLocation.SourceLine, PrevLocation.pszSourceFunc );
}

#define APEMODE_GLOBAL_NEW_DELETE_FILE gCurrLocation.pszSourceFile
#define APEMODE_GLOBAL_NEW_DELETE_LINE gCurrLocation.SourceLine
#define APEMODE_GLOBAL_NEW_DELETE_FUNC gCurrLocation.pszSourceFunc

#else

#define APEMODE_GLOBAL_NEW_DELETE_FILE __FILE__
#define APEMODE_GLOBAL_NEW_DELETE_LINE __LINE__
#define APEMODE_GLOBAL_NEW_DELETE_FUNC __FUNCTION__

#endif

#ifndef APEMODE_NO_GLOBAL_NEW_DELETE_OP_OVERRIDES
//#define APEMODE_ENABLE_GLOBAL_NEW_DELETE_OP_OVERRIDES
#endif

#ifdef APEMODE_ENABLE_GLOBAL_NEW_DELETE_OP_OVERRIDES

void *operator new[]( std::size_t size, std::nothrow_t const & ) APEMODE_NO_EXCEPT {
    return apememext::aligned_malloc( size,
                                      APEMODE_DEFAULT_ALIGNMENT,
                                      APEMODE_GLOBAL_NEW_DELETE_FILE,
                                      APEMODE_GLOBAL_NEW_DELETE_LINE,
                                      APEMODE_GLOBAL_NEW_DELETE_FUNC,
                                      m_alloc_new_array );
}

void *operator new[]( std::size_t size ) APEMODE_THROW_BAD_ALLOC {
    return apememext::aligned_malloc( size,
                                      APEMODE_DEFAULT_ALIGNMENT,
                                      APEMODE_GLOBAL_NEW_DELETE_FILE,
                                      APEMODE_GLOBAL_NEW_DELETE_LINE,
                                      APEMODE_GLOBAL_NEW_DELETE_FUNC,
                                      m_alloc_new_array );
}

void *operator new( std::size_t size, std::nothrow_t const & ) APEMODE_NO_EXCEPT {
    return apememext::aligned_malloc( size,
                                      APEMODE_DEFAULT_ALIGNMENT,
                                      APEMODE_GLOBAL_NEW_DELETE_FILE,
                                      APEMODE_GLOBAL_NEW_DELETE_LINE,
                                      APEMODE_GLOBAL_NEW_DELETE_FUNC,
                                      m_alloc_new );
}

void *operator new( std::size_t size ) APEMODE_THROW_BAD_ALLOC {
    return apememext::aligned_malloc( size,
                                      APEMODE_DEFAULT_ALIGNMENT,
                                      APEMODE_GLOBAL_NEW_DELETE_FILE,
                                      APEMODE_GLOBAL_NEW_DELETE_LINE,
                                      APEMODE_GLOBAL_NEW_DELETE_FUNC,
                                      m_alloc_new );
}

void operator delete[]( void *pMemory ) APEMODE_NO_EXCEPT {
    return apememext::aligned_free( pMemory,
                                    APEMODE_GLOBAL_NEW_DELETE_FILE,
                                    APEMODE_GLOBAL_NEW_DELETE_LINE,
                                    APEMODE_GLOBAL_NEW_DELETE_FUNC,
                                    m_alloc_delete_array );
}

void operator delete( void *pMemory ) APEMODE_NO_EXCEPT {
    return apememext::aligned_free( pMemory,
                                    APEMODE_GLOBAL_NEW_DELETE_FILE,
                                    APEMODE_GLOBAL_NEW_DELETE_LINE,
                                    APEMODE_GLOBAL_NEW_DELETE_FUNC,
                                    m_alloc_delete );
}

#endif

void *operator new[]( std::size_t                       size,
                      std::nothrow_t const &            nothrow,
                      apemode::platform::EAllocationTag eAllocTag,
                      const char *                      pszSourceFile,
                      const unsigned int                sourceLine,
                      const char *                      pszSourceFunc ) APEMODE_NO_EXCEPT {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, pszSourceFile, sourceLine, pszSourceFunc, m_alloc_new_array );
}

void *operator new[]( std::size_t                       size,
                      apemode::platform::EAllocationTag eAllocTag,
                      const char *                      pszSourceFile,
                      const unsigned int                sourceLine,
                      const char *                      pszSourceFunc ) APEMODE_THROW_BAD_ALLOC {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, pszSourceFile, sourceLine, pszSourceFunc, m_alloc_new_array );
}

void *operator new( std::size_t                       size,
                    std::nothrow_t const &            nothrow,
                    apemode::platform::EAllocationTag eAllocTag,
                    const char *                      pszSourceFile,
                    const unsigned int                sourceLine,
                    const char *                      pszSourceFunc ) APEMODE_NO_EXCEPT {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, __FILE__, __LINE__, __FUNCTION__, m_alloc_new );
}

void *operator new( std::size_t                       size,
                    apemode::platform::EAllocationTag eAllocTag,
                    const char *                      pszSourceFile,
                    const unsigned int                sourceLine,
                    const char *                      pszSourceFunc ) APEMODE_THROW_BAD_ALLOC {
    return apememext::aligned_malloc( size, APEMODE_DEFAULT_ALIGNMENT, __FILE__, __LINE__, __FUNCTION__, m_alloc_new );
}

void operator delete[]( void *                            pMemory,
                        apemode::platform::EAllocationTag eAllocTag,
                        const char *                      pszSourceFile,
                        const unsigned int                sourceLine,
                        const char *                      pszSourceFunc ) APEMODE_NO_EXCEPT {
    return apememext::aligned_free( pMemory, __FILE__, __LINE__, __FUNCTION__, m_alloc_delete_array );
}

void operator delete(void *                            pMemory,
                     apemode::platform::EAllocationTag eAllocTag,
                     const char *                      pszSourceFile,
                     const unsigned int                sourceLine,
                     const char *                      pszSourceFunc) APEMODE_NO_EXCEPT {
    return apememext::aligned_free( pMemory, __FILE__, __LINE__, __FUNCTION__, m_alloc_delete );
}

apemode::platform::StandardAllocator::StandardAllocator( const char * ) {
}

apemode::platform::StandardAllocator::StandardAllocator( const StandardAllocator & ) {
}

apemode::platform::StandardAllocator::StandardAllocator( const StandardAllocator &, const char * ) {
}

apemode::platform::StandardAllocator &apemode::platform::StandardAllocator::operator=( const StandardAllocator & ) {
    return *this;
}

bool apemode::platform::StandardAllocator::operator==( const StandardAllocator & ) {
    return true;
}

bool apemode::platform::StandardAllocator::operator!=( const StandardAllocator & ) {
    return false;
}

void *apemode::platform::StandardAllocator::allocate( size_t size, int /*flags*/ ) {
    return apemode_malloc( size, apemode::platform::kAlignment );
}

void *apemode::platform::StandardAllocator::allocate( size_t size,
                                                                size_t alignment,
                                                                size_t /*alignmentOffset*/,
                                                                int /*flags*/ ) {
    return apemode_malloc( size, alignment );
}

void apemode::platform::StandardAllocator::deallocate( void *p, size_t /*n*/ ) {
    return apemode_free( p );
}

const char *apemode::platform::StandardAllocator::get_name( ) const {
    return "apemode::platform::StandardAllocator";
}

void apemode::platform::StandardAllocator::set_name( const char* ) {
}

#ifdef USE_MEMORY_TRACKING

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
    return apemode::platform::allocate( size, alignment, __FILE__, __LINE__, __FUNCTION__ );
}

void *apemode_calloc( size_t count, size_t size, size_t alignment ) {
    return apemode::platform::callocate( count, size, alignment, __FILE__, __LINE__, __FUNCTION__ );
}

void *apemode_realloc( void *p, size_t size, size_t alignment ) {
    return apemode::platform::reallocate( p, size, alignment, __FILE__, __LINE__, __FUNCTION__ );
}

void apemode_free( void *p ) {
    apemode::platform::deallocate( p, __FILE__, __LINE__, __FUNCTION__ );
}

#endif
