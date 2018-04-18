
#pragma once
#include <new>
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
    void* allocate( size_t size, size_t alignment, const char* sourceFile, const unsigned int sourceLine, const char* sourceFunc );

    /**
     * The regular calloc call with aligment.
     * Do not align with less then kDefaultAlignment bytes.
     * @param num Element count.
     * @param size The byte size of the element.
     * @param alignment The byte alignment of the memory chunk.
     * @return The allocated memory chunk address.
     */
    void* callocate( size_t num, size_t size, size_t alignment, const char* sourceFile, const unsigned int sourceLine, const char* sourceFunc );

    /**
     * The regular realloc call with aligment.
     * Do not align with less then kDefaultAlignment bytes.
     * @param size The byte size of the memory chunk.
     * @param alignment The byte alignment of the memory chunk.
     * @return The allocated memory chunk address.
     */
    void* reallocate( void* p, size_t size, size_t alignment, const char* sourceFile, const unsigned int sourceLine, const char* sourceFunc );

    /**
     * The regular free call.
     * @param p The memory chunk address.
     */
    void deallocate( void* p, const char* sourceFile, const unsigned int sourceLine, const char* sourceFunc );

} // namespace apemode

void* operator new[]( std::size_t s,
                      std::nothrow_t const&,
                      const char*        sourceFile,
                      const unsigned int sourceLine,
                      const char*        sourceFunc ) noexcept;

void* operator new[]( std::size_t        s,
                      const char*        sourceFile,
                      const unsigned int sourceLine,
                      const char*        sourceFunc ) throw( );

void operator delete[]( void* p, const char* sourceFile, const unsigned int sourceLine, const char* sourceFunc ) throw( );

void* operator new( std::size_t s,
                    std::nothrow_t const&,
                    const char*        sourceFile,
                    const unsigned int sourceLine,
                    const char*        sourceFunc ) noexcept;

void* operator new( std::size_t        s,
                    const char*        sourceFile,
                    const unsigned int sourceLine,
                    const char*        sourceFunc ) throw( );

void operator delete( void* p, const char* sourceFile, const unsigned int sourceLine, const char* sourceFunc ) throw( );

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

#define apemode_new new ( __FILE__, __LINE__, __FUNCTION__ )
#define apemode_delete delete ( __FILE__, __LINE__, __FUNCTION__ )
