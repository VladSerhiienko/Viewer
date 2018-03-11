
#pragma once
#include <new>
#include <memory>

#ifndef APEMODE_USE_MEMORY_TRACKING
 #define APEMODE_USE_MEMORY_TRACKING
#endif

#ifndef APEMODE_USE_DLMALLOC
// #define APEMODE_USE_DLMALLOC
#endif

#ifndef APEMODE_DEFAULT_ALIGNMENT
#define APEMODE_DEFAULT_ALIGNMENT ( sizeof( void* ) << 1 )
#endif

#if defined( APEMODE_USE_MEMORY_TRACKING )
#include "FluidStudios/MemoryManager/mmgr.h"

#define apemode_malloc( size )           m_allocator( __FILE__, __LINE__, __FUNCTION__, m_alloc_malloc, ( size ) )
#define apemode_calloc( count, size )    m_allocator( __FILE__, __LINE__, __FUNCTION__, m_alloc_calloc, ( ( size ) * ( count ) ) )
#define apemode_realloc( ptr, size )   m_reallocator( __FILE__, __LINE__, __FUNCTION__, m_alloc_realloc, ( size ), ( ptr ) )
#define apemode_free( ptr )            m_deallocator( __FILE__, __LINE__, __FUNCTION__, m_alloc_free, ( ptr ) )

#else

void* apemode_malloc( size_t size );
void* apemode_calloc( size_t count, size_t size );
void* apemode_realloc( void* p, size_t size );
void  apemode_free( void* p );

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
    void* allocate( size_t size );

    /**
     * The regular calloc call with aligment.
     * Do not align with less then kDefaultAlignment bytes.
     * @param num Element count.
     * @param size The byte size of the element.
     * @param alignment The byte alignment of the memory chunk.
     * @return The allocated memory chunk address.
     */
    void* callocate( size_t num, size_t size );

    /**
     * The regular realloc call with aligment.
     * Do not align with less then kDefaultAlignment bytes.
     * @param size The byte size of the memory chunk.
     * @param alignment The byte alignment of the memory chunk.
     * @return The allocated memory chunk address.
     */
    void* reallocate( void* p, size_t size );

    /**
     * The regular free call.
     * @param p The memory chunk address.
     */
    void deallocate( void* p );

#ifdef APEMODE_USE_DLMALLOC

    /**
     * The malloc call with aligment from the thread-local memory space.
     * Do not align with less then kDefaultAlignment bytes.
     * @param size The byte size of the memory chunk.
     * @param alignment The byte alignment of the memory chunk.
     * @return The allocated memory chunk address.
     */
    void* threadLocalAllocate( size_t size );

    /**
     * The realloc call with aligment from the thread-local memory space.
     * Do not align with less then kDefaultAlignment bytes.
     * @param p Address of the old memroy chunk.
     * @param size The byte size of the memory chunk.
     * @param alignment The byte alignment of the memory chunk.
     * @return The reallocated memory chunk address.
     */
    void* threadLocalReallocate( void* p, size_t size );

    /**
     * The free call from the thread-local memory space.
     * @param p The memory chunk address.
     */
    void threadLocalDeallocate( void* p );

#endif

#pragma push_macro( "new" )
#undef new

    template < typename T, typename... Args >
    inline T* Init( void* ptr, Args... args ) {
        return new ( ptr ) T( args... );
    }

#ifdef Make
#undef Make
#endif

#if defined( APEMODE_USE_MEMORY_TRACKING )

    template < typename T, typename... Args >
    inline T* Make_impl( const char* sourceFile, const unsigned int sourceLine, const char* sourceFunc, Args... args ) {
        return new ( m_allocator( sourceFile, sourceLine, sourceFunc, m_alloc_malloc, ( sizeof( T ) ) ) ) T( args... );
    }

#define Make( T, ... ) Make_impl< T >( __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__ )

#else

    template < typename T, typename... Args >
    inline T* Make_impl( Args... args ) {
        return new ( apemode::allocate( sizeof( T ) ) ) T( args... );
    }

#define Make( T, ... ) Make_impl< T >( __VA_ARGS__ )

#endif

#pragma pop_macro( "new" )

} // namespace apemode
