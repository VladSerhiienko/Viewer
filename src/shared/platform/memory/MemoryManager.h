
#pragma once

#include <new>
#include <ctime>
#include <cstdlib>
#include <cassert>
#include <memory>
#include <memory.h>

// #if _WIN32
#ifndef APEMODE_USE_MEMORY_TRACKING
#define APEMODE_USE_MEMORY_TRACKING
#endif

#ifndef APEMODE_USE_DLMALLOC
#define APEMODE_USE_DLMALLOC
#endif
// #endif

#ifndef APEMODE_DEFAULT_ALIGNMENT
#define APEMODE_DEFAULT_ALIGNMENT ( sizeof( void* ) << 1 )
#endif

namespace apemode {

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
    void* allocate( size_t size, size_t alignment, const char* pszSourceFile, const unsigned int sourceLine, const char* pszSourceFunc );

    /**
     * The regular calloc call with aligment.
     * Do not align with less then kDefaultAlignment bytes.
     * @param num Element count.
     * @param size The byte size of the element.
     * @param alignment The byte alignment of the memory chunk.
     * @return The allocated memory chunk address.
     */
    void* callocate( size_t num, size_t size, size_t alignment, const char* pszSourceFile, const unsigned int sourceLine, const char* pszSourceFunc );

    /**
     * The regular realloc call with aligment.
     * Do not align with less then kDefaultAlignment bytes.
     * @param size The byte size of the memory chunk.
     * @param alignment The byte alignment of the memory chunk.
     * @return The allocated memory chunk address.
     */
    void* reallocate( void* p, size_t size, size_t alignment, const char* pszSourceFile, const unsigned int sourceLine, const char* pszSourceFunc );

    /**
     * The regular free call.
     * @param p The memory chunk address.
     */
    void deallocate( void* p, const char* pszSourceFile, const unsigned int sourceLine, const char* pszSourceFunc );

} // namespace apemode

#ifndef APEMODE_THROW_BAD_ALLOC
#define APEMODE_THROW_BAD_ALLOC throw( std::bad_alloc )
#endif

#ifndef APEMODE_NO_EXCEPT
#define APEMODE_NO_EXCEPT noexcept
#endif

namespace apemode {
    enum EAllocationTag { eAllocationTag = 0 };
}

void* operator new[]( std::size_t s,
                      std::nothrow_t const&,
                      apemode::EAllocationTag eAllocTag,
                      const char*             pszSourceFile,
                      const unsigned int      sourceLine,
                      const char*             pszSourceFunc ) APEMODE_NO_EXCEPT;

void* operator new[]( std::size_t             s,
                      apemode::EAllocationTag eAllocTag,
                      const char*             pszSourceFile,
                      const unsigned int      sourceLine,
                      const char*             pszSourceFunc ) APEMODE_THROW_BAD_ALLOC;

void operator delete[]( void*                   p,
                        apemode::EAllocationTag eAllocTag,
                        const char*             pszSourceFile,
                        const unsigned int      sourceLine,
                        const char*             pszSourceFunc ) APEMODE_NO_EXCEPT;

void* operator new( std::size_t s,
                    std::nothrow_t const&,
                    apemode::EAllocationTag eAllocTag,
                    const char*             pszSourceFile,
                    const unsigned int      sourceLine,
                    const char*             pszSourceFunc ) APEMODE_NO_EXCEPT;

void* operator new( std::size_t             s,
                    apemode::EAllocationTag eAllocTag,
                    const char*             pszSourceFile,
                    const unsigned int      sourceLine,
                    const char*             pszSourceFunc ) APEMODE_THROW_BAD_ALLOC;

void operator delete( void*                   p,
                      apemode::EAllocationTag eAllocTag,
                      const char*             pszSourceFile,
                      const unsigned int      sourceLine,
                      const char*             pszSourceFunc ) APEMODE_NO_EXCEPT;

#if defined( APEMODE_USE_MEMORY_TRACKING )
#include "FluidStudios/MemoryManager/mmgr.h"

#define apemode_malloc( size, alignment )        apemode::allocate( size, alignment, __FILE__, __LINE__, __FUNCTION__ )
#define apemode_calloc( count, size, alignment ) apemode::callocate( count, size, alignment,  __FILE__, __LINE__, __FUNCTION__ )
#define apemode_realloc( ptr, size, alignment )  apemode::reallocate( ptr, size, alignment, __FILE__, __LINE__, __FUNCTION__ )
#define apemode_free( ptr )                      apemode::deallocate( ptr, __FILE__, __LINE__, __FUNCTION__ )

#else

void* apemode_malloc( size_t size, size_t alignment = APEMODE_DEFAULT_ALIGNMENT );
void* apemode_calloc( size_t count, size_t size, size_t alignment = APEMODE_DEFAULT_ALIGNMENT );
void* apemode_realloc( void* p, size_t size, size_t alignment = APEMODE_DEFAULT_ALIGNMENT );
void  apemode_free( void* p );

#endif

namespace apemode {
    namespace platform {
        template < typename T >
        void CallDestructor( T* pObj ) {
            if ( pObj )
                pObj->~T( );
        }
    } // namespace platform
} // namespace apemode

#define apemode_new             new ( apemode::eAllocationTag, __FILE__, __LINE__, __FUNCTION__ )
#define apemode_delete( pObj )  apemode::platform::CallDestructor( pObj ), operator delete( pObj, apemode::eAllocationTag, __FILE__, __LINE__, __FUNCTION__ ), pObj= nullptr

namespace apemode {

    template < typename T >
    struct TTrakingDeleter {
        void operator( )( T* pObj ) {
            apemode_delete( pObj );
        }
    };

    template < typename T >
    using unique_ptr = std::unique_ptr< T, TTrakingDeleter< T > >;

    /**
     * @brief make_unique for all platforms
     */
    template < typename T, typename... Args >
    unique_ptr< T > make_unique( Args&&... args ) {
        return unique_ptr< T >( apemode_new T( std::forward< Args >( args )... ) );
    }
} // namespace apemode
