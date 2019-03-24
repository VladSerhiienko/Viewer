
#pragma once

#include <memory.h>
#include <assert.h>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <new>

#ifndef APEMODE_NO_MEMORY_TRACKING
#ifndef APEMODE_USE_MEMORY_TRACKING
#define APEMODE_USE_MEMORY_TRACKING
#endif
#endif

#ifndef APEMODE_NO_DLMALLOC
#ifndef APEMODE_USE_DLMALLOC
#define APEMODE_USE_DLMALLOC
#endif
#endif

#ifndef APEMODE_DEFAULT_ALIGNMENT
#define APEMODE_DEFAULT_ALIGNMENT ( sizeof( void* ) << 1 )
#endif

namespace apemode {
namespace platform {

/**
 * The default alignment for memory allocations.
 */
static const size_t kAlignment = APEMODE_DEFAULT_ALIGNMENT;

/**
 * The regular malloc call with aligment.
 * Do not align with less then kDefaultAlignment bytes.
 * @param size The byte size of the memory chunk.
 * @param alignment The byte alignment of the memory chunk.
 * @return The allocated memory chunk address.
 */
void* allocate(
    size_t size, size_t alignment, const char* pszSourceFile, const unsigned int sourceLine, const char* pszSourceFunc );

/**
 * The regular calloc call with aligment.
 * Do not align with less then kDefaultAlignment bytes.
 * @param num Element count.
 * @param size The byte size of the element.
 * @param alignment The byte alignment of the memory chunk.
 * @return The allocated memory chunk address.
 */
void* callocate( size_t             num,
                 size_t             size,
                 size_t             alignment,
                 const char*        pszSourceFile,
                 const unsigned int sourceLine,
                 const char*        pszSourceFunc );

/**
 * The regular realloc call with aligment.
 * Do not align with less then kDefaultAlignment bytes.
 * @param size The byte size of the memory chunk.
 * @param alignment The byte alignment of the memory chunk.
 * @return The allocated memory chunk address.
 */
void* reallocate( void*              p,
                  size_t             size,
                  size_t             alignment,
                  const char*        pszSourceFile,
                  const unsigned int sourceLine,
                  const char*        pszSourceFunc );

/**
 * The regular free call.
 * @param p The memory chunk address.
 */
void deallocate( void* p, const char* pszSourceFile, const unsigned int sourceLine, const char* pszSourceFunc );

} // namespace platform
} // namespace apemode

// Lots of warnings on Windows, MSVC ignores throw specifications, except declspec(nothrow).
// For C++17 don't use dynamic exception specifications.
#if defined( _WIN32 ) || __cplusplus >= 201703L
#ifndef APEMODE_THROW_BAD_ALLOC
#define APEMODE_THROW_BAD_ALLOC
#endif
#endif

#ifndef APEMODE_THROW_BAD_ALLOC
#define APEMODE_THROW_BAD_ALLOC throw( std::bad_alloc )
#endif

#ifndef APEMODE_NO_EXCEPT
#define APEMODE_NO_EXCEPT noexcept
#endif

namespace apemode {
namespace platform {
enum EAllocationTag { eAllocationTag = 0 };
}
} // namespace apemode

void* operator new[]( std::size_t s,
                      std::nothrow_t const&,
                      apemode::platform::EAllocationTag eAllocTag,
                      const char*             pszSourceFile,
                      const unsigned int      sourceLine,
                      const char*             pszSourceFunc ) APEMODE_NO_EXCEPT;

void* operator new[]( std::size_t             s,
                      apemode::platform::EAllocationTag eAllocTag,
                      const char*             pszSourceFile,
                      const unsigned int      sourceLine,
                      const char*             pszSourceFunc ) APEMODE_THROW_BAD_ALLOC;

void operator delete[]( void*                   p,
                        apemode::platform::EAllocationTag eAllocTag,
                        const char*             pszSourceFile,
                        const unsigned int      sourceLine,
                        const char*             pszSourceFunc ) APEMODE_NO_EXCEPT;

void* operator new( std::size_t s,
                    std::nothrow_t const&,
                    apemode::platform::EAllocationTag eAllocTag,
                    const char*             pszSourceFile,
                    const unsigned int      sourceLine,
                    const char*             pszSourceFunc ) APEMODE_NO_EXCEPT;

void* operator new( std::size_t             s,
                    apemode::platform::EAllocationTag eAllocTag,
                    const char*             pszSourceFile,
                    const unsigned int      sourceLine,
                    const char*             pszSourceFunc ) APEMODE_THROW_BAD_ALLOC;

void operator delete(void*                   p,
                     apemode::platform::EAllocationTag eAllocTag,
                     const char*             pszSourceFile,
                     const unsigned int      sourceLine,
                     const char*             pszSourceFunc) APEMODE_NO_EXCEPT;

#ifdef APEMODE_USE_MEMORY_TRACKING

#include "FluidStudios/MemoryManager/mmgr.h"

#define apemode_malloc( size, alignment ) apemode::platform::allocate( ( size ), ( alignment ), __FILE__, __LINE__, __FUNCTION__ )
#define apemode_calloc( count, size, alignment ) \
    apemode::platform::callocate( ( count ), ( size ), ( alignment ), __FILE__, __LINE__, __FUNCTION__ )
#define apemode_realloc( ptr, size, alignment ) \
    apemode::platform::reallocate( ( ptr ), ( size ), ( alignment ), __FILE__, __LINE__, __FUNCTION__ )
#define apemode_free( ptr ) apemode::platform::deallocate( ( ptr ), __FILE__, __LINE__, __FUNCTION__ )

#else // APEMODE_USE_MEMORY_TRACKING

void* apemode_malloc( size_t size, size_t alignment = APEMODE_DEFAULT_ALIGNMENT );
void* apemode_calloc( size_t count, size_t size, size_t alignment = APEMODE_DEFAULT_ALIGNMENT );
void* apemode_realloc( void* p, size_t size, size_t alignment = APEMODE_DEFAULT_ALIGNMENT );
void  apemode_free( void* p );

#endif // !APEMODE_USE_MEMORY_TRACKING

namespace apemode {
namespace platform {
template < typename T >
void CallDestructor( T* pObj ) {
    if ( pObj )
        pObj->~T( );
}
} // namespace platform
} // namespace apemode

#define apemode_new new ( apemode::platform::eAllocationTag, __FILE__, __LINE__, __FUNCTION__ )
#define apemode_delete( pObj )                     \
    apemode::platform::CallDestructor( ( pObj ) ), \
    operator delete( ( pObj ), apemode::platform::eAllocationTag, __FILE__, __LINE__, __FUNCTION__ ), pObj = nullptr

#include <EASTL/linked_ptr.h>
#include <EASTL/string.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#include <EASTL/vector_multimap.h>
#include <EASTL/vector_set.h>

namespace apemode {

namespace platform {
template < typename T >
struct TDelete {
    void operator( )( T* pObj ) {
        apemode_delete( pObj );
    }
};

class StandardAllocator {
public:
    StandardAllocator( const char* = NULL );
    StandardAllocator( const StandardAllocator& );
    StandardAllocator( const StandardAllocator&, const char* );
    StandardAllocator& operator=( const StandardAllocator& );
    bool               operator==( const StandardAllocator& );
    bool               operator!=( const StandardAllocator& );
    void*              allocate( size_t n, int /*flags*/ = 0 );
    void*              allocate( size_t n, size_t alignment, size_t /*alignmentOffset*/, int /*flags*/ = 0 );
    void               deallocate( void* p, size_t /*n*/ );
    const char*        get_name( ) const;
    void               set_name( const char* );
};
} // namespace platform

template < typename T >
using unique_ptr = eastl::unique_ptr< T, platform::TDelete< T > >;

template < typename T >
using shared_ptr = eastl::linked_ptr< T, platform::TDelete< T > >;

template < typename T >
using vector = eastl::vector< T, platform::StandardAllocator >;

template < typename K, typename T, typename Cmp = eastl::less< K > >
using vector_map = eastl::vector_map< K, T, Cmp, platform::StandardAllocator >;

template < typename K, typename Cmp = eastl::less< K > >
using vector_set = eastl::vector_set< K, Cmp, platform::StandardAllocator >;

template < typename K, typename T, typename Cmp = eastl::less< K > >
using vector_multimap = eastl::vector_multimap< K, T, Cmp, platform::StandardAllocator >;

template < typename T >
using basic_string = eastl::basic_string< T, platform::StandardAllocator >;

using string8 = basic_string< char8_t >;

/**
 * @brief make_unique for all platforms
 */
template < typename T, typename... Args >
unique_ptr< T > make_unique( Args&&... args ) {
    return unique_ptr< T >( apemode_new T( std::forward< Args >( args )... ) );
}
} // namespace apemode

#ifdef APEMODE_USE_MEMORY_TRACKING
#ifndef apemode_named_memory_allocation_scope

namespace apemode {
namespace platform {

void GetPrevMemoryAllocationScope( const char*& pszSourceFile, unsigned int& sourceLine, const char*& pszSourceFunc );
void StartMemoryAllocationScope( const char* pszSourceFile, const unsigned int sourceLine, const char* pszSourceFunc );
void EndMemoryAllocationScope( const char* pszSourceFile, const unsigned int sourceLine, const char* pszSourceFunc );

struct MemoryAllocationScope {
    struct CodeLocation {
        const char*  pszSourceFile;
        unsigned int SourceLine;
        const char*  pszSourceFunc;
    };

    CodeLocation PrevLocation;

    MemoryAllocationScope( const char* pszSourceFile, const unsigned int sourceLine, const char* pszSourceFunc );
    ~MemoryAllocationScope( );
};

} // namespace apemode
} // namespace apemode

#define apemode_named_memory_allocation_scope( name ) \
    apemode::platform::MemoryAllocationScope _##name##MemoryAllocationScope( __FILE__, __LINE__, __FUNCTION__ )
#endif // apemode_named_memory_allocation_scope
#endif // APEMODE_USE_MEMORY_TRACKING

#ifdef apemode_named_memory_allocation_scope
#define apemode_memory_allocation_scope apemode_named_memory_allocation_scope( default )
#else
#define apemode_memory_allocation_scope
#endif