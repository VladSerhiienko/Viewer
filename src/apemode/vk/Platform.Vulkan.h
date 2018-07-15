#pragma once

#ifdef _WIN32 // ------------------------------------------------------------------

    #ifndef VK_USE_PLATFORM_WIN32_KHR
    #define VK_USE_PLATFORM_WIN32_KHR 1
    #endif

    // #ifndef _WINDOWS_
    //     #ifndef STRICT
    //     #define STRICT
    //     #endif
    //     #ifndef NOMINMAX
    //     #define NOMINMAX
    //     #endif
    //     #ifndef WIN32_LEAN_AND_MEAN
    //     #define WIN32_LEAN_AND_MEAN
    //     #endif
    //     #include <Windows.h>
    // #endif // _WINDOWS_

#elif __APPLE__ // ------------------------------------------------------------------

    #include "TargetConditionals.h"

    #if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
        #ifndef VK_USE_PLATFORM_IOS_MVK
        #define VK_USE_PLATFORM_IOS_MVK 1
        #endif
    #elif TARGET_OS_MAC
        #ifndef VK_USE_PLATFORM_MACOS_MVK
        #define VK_USE_PLATFORM_MACOS_MVK 1
        #endif
    #else // ERROR
        #error "Unknown Apple platform"
    #endif

#elif __linux__ // ------------------------------------------------------------------

    #ifndef VK_USE_PLATFORM_XLIB_KHR
    #define VK_USE_PLATFORM_XLIB_KHR 1
    #endif

    // #include <X11/Xlib.h>
    // #include <X11/Xutil.h>
    // #include <X11/Xresource.h>
    // #include <X11/Xlocale.h>

#else
    #error "Unsupported Vk platform"
#endif

#include <assert.h>
#include <inttypes.h>
#include <memory.h>
#include <stdarg.h>
#include <limits>
#include <iostream>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#undef min
#undef max
#include <spirv_glsl.hpp>

#ifndef _In_
#define _In_
#endif

#ifndef _Out_
#define _Out_
#endif

#ifndef APEMODEVK_DEFAULT_ALIGNMENT
#define APEMODEVK_DEFAULT_ALIGNMENT ( sizeof( void* ) << 1 )
#endif

namespace apemodevk {

    enum EAllocationTag { eAllocationTag = 0 };

    /**
     * The default alignment for memory allocations.
     */
    static const size_t kAlignment = APEMODEVK_DEFAULT_ALIGNMENT;

} // namespace apemodevk

void *operator new[]( std::size_t               size,
                      std::nothrow_t const &    eNothrowTag,
                      apemodevk::EAllocationTag eAllocTag,
                      const char *              pszSourceFile,
                      const unsigned int        sourceLine,
                      const char *              pszSourceFunc ) noexcept;

void *operator new[]( std::size_t               size,
                      apemodevk::EAllocationTag eAllocTag,
                      const char *              pszSourceFile,
                      const unsigned int        sourceLine,
                      const char *              pszSourceFunc ) throw( );

void operator delete[]( void *                    pMemory,
                        apemodevk::EAllocationTag eAllocTag,
                        const char *              pszSourceFile,
                        const unsigned int        sourceLine,
                        const char *              pszSourceFunc ) throw( );

void *operator new( std::size_t               size,
                    std::nothrow_t const &    eNothrowTag,
                    apemodevk::EAllocationTag eAllocTag,
                    const char *              pszSourceFile,
                    const unsigned int        sourceLine,
                    const char *              pszSourceFunc ) noexcept;

void *operator new( std::size_t               size,
                    apemodevk::EAllocationTag eAllocTag,
                    const char *              pszSourceFile,
                    const unsigned int        sourceLine,
                    const char *              pszSourceFunc ) throw( );

void operator delete( void *                    pMemory,
                      apemodevk::EAllocationTag eAllocTag,
                      const char *              pszSourceFile,
                      const unsigned int        sourceLine,
                      const char *              pszSourceFunc ) throw( );

namespace apemodevk {
    namespace platform {
        template < typename T >
        void CallDestructor( T *pObj ) {
            if ( pObj )
                pObj->~T( );
        }
    } // namespace platform
} // namespace apemodevk

#define apemodevk_new              new ( apemodevk::eAllocationTag, __FILE__, __LINE__, __FUNCTION__ )
#define apemodevk_create( T, ... ) new ( operator new( apemodevk::eAllocationTag, __FILE__, __LINE__, __FUNCTION__ ) ) T( __VA_ARGS__ )
#define apemodevk_delete( pObj )   apemodevk::platform::CallDestructor( pObj ), operator delete( pObj, apemodevk::eAllocationTag, __FILE__, __LINE__, __FUNCTION__ ), pObj = nullptr

#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

namespace apemodevk {

    namespace platform {
        template < typename T >
        struct TDelete {
            void operator( )( T *pObj ) {
                apemodevk_delete( pObj );
            }
        };

        class StandardAllocator {
        public:
            StandardAllocator( const char * = NULL );
            StandardAllocator( const StandardAllocator & );
            StandardAllocator( const StandardAllocator &, const char * );
            StandardAllocator &operator=( const StandardAllocator & );
            bool               operator==( const StandardAllocator & );
            bool               operator!=( const StandardAllocator & );
            void *             allocate( size_t n, int /*flags*/ = 0 );
            void *             allocate( size_t n, size_t alignment, size_t /*alignmentOffset*/, int /*flags*/ = 0 );
            void               deallocate( void *p, size_t /*n*/ );
            const char *       get_name( ) const;
            void               set_name( const char * );
        };

        void GetPrevMemoryAllocationScope( const char *&pszSourceFile, unsigned int &sourceLine, const char *&pszSourceFunc );
        void StartMemoryAllocationScope( const char *pszSourceFile, const unsigned int sourceLine, const char *pszSourceFunc );
        void EndMemoryAllocationScope( const char *pszSourceFile, const unsigned int sourceLine, const char *pszSourceFunc );

        struct MemoryAllocationScope {
            const char * pszSourceFile;
            unsigned int SourceLine;
            const char * pszSourceFunc;

            MemoryAllocationScope( const char *pszInSourceFile, const unsigned int InSourceLine, const char *pszInSourceFunc ) {
                GetPrevMemoryAllocationScope( pszSourceFile, SourceLine, pszSourceFunc );
                StartMemoryAllocationScope( pszInSourceFile, InSourceLine, pszInSourceFunc );
            }

            ~MemoryAllocationScope( ) {
                EndMemoryAllocationScope( pszSourceFile, SourceLine, pszSourceFunc );
            }
        };
    } // namespace platform

    template < typename T >
    using unique_ptr = eastl::unique_ptr< T, platform::TDelete< T > >;

    template < typename T >
    using vector = eastl::vector< T, platform::StandardAllocator >;

    template < typename T, typename... Args >
    unique_ptr< T > make_unique( Args &&... args ) {
        return unique_ptr< T >( apemodevk_new T( eastl::forward< Args >( args )... ) );
    }

    template < typename T, typename... Args >
    std::unique_ptr< T > std_make_unique( Args &&... args ) {
        return  std::unique_ptr< T >( new T( eastl::forward< Args >( args )... ) );
    }
}

#define apemodevk_max( a, b )( ( a ) > ( b ) ? ( a ) : ( b ) )
#define apemodevk_min( a, b )( ( a ) < ( b ) ? ( a ) : ( b ) )
#define apemodevk_memory_allocation_scope apemodevk::platform::MemoryAllocationScope memoryAllocationScope( __FILE__, __LINE__, __FUNCTION__ )

namespace apemodevk {

    constexpr uint32_t kInvalidIndex   = uint32_t( -1 );
    constexpr uint64_t kInvalidIndex64 = uint64_t( -1 );

    namespace platform {

        inline bool IsDebuggerPresent( ) {
#ifdef _WINDOWS_
            return TRUE == ::IsDebuggerPresent( );
#elif _DEBUG
            return true;
#else
            return false;
#endif
        }

        inline void DebugBreak( ) {
            if ( IsDebuggerPresent( ) ) {
#ifdef _WINDOWS_
                __debugbreak( ); /* Does not require symbols */
#endif
            }
        }

        enum LogLevel {
            Trace    = 0,
            Debug    = 1,
            Info     = 2,
            Warn     = 3,
            Err      = 4,
            Critical = 5,
        };

        void Log( LogLevel level, const char *pszMsg );

        template < bool bNewLine = false, unsigned uBufferSize = 1024u >
        static void DebugTrace( LogLevel level, char const *pszFmt, ... ) {

            const int uLength = uBufferSize - 1 - bNewLine;
            char szBuffer[ uBufferSize ] = {0};

            va_list ap;
            va_start( ap, pszFmt );

#ifdef _WINDOWS_
            auto n = _vsnprintf_s( szBuffer, uLength, pszFmt, ap );
#else
            auto n = vsnprintf( szBuffer, uLength, pszFmt, ap );
#endif
            assert( n <= uLength );

            if ( bNewLine ) {
                szBuffer[ n ]     = '\n';
                szBuffer[ n + 1 ] = '\0';
            } else {
                szBuffer[ n ] = '\0';
            }

            Log( level, szBuffer );
            va_end( ap );
        }
    } // namespace platform
}

#define apemode_dllapi
#define apemode_assert( ... ) // assert(__VA_ARGS__)
#define apemode_likely( ... ) __VA_ARGS__
#define apemode_unlikely( ... ) __VA_ARGS__
#define apemode_error( ... )
#define apemode_halt( ... )

namespace apemodevk {
    template < typename T >
    uint32_t GetSizeU( const T &c ) {
        return static_cast< uint32_t >( c.size( ) );
    }
}

namespace apemodevk {
    struct ScalableAllocPolicy {};

    class NoAssignPolicy {
        void operator=( const NoAssignPolicy & );
    public:
        NoAssignPolicy( ) { }
    };

    class NoCopyPolicy {
        NoCopyPolicy( const NoCopyPolicy & );
    public:
        NoCopyPolicy( ) { }
    };

    class NoCopyAssignPolicy : NoCopyPolicy, NoAssignPolicy {
    public:
        NoCopyAssignPolicy( ) { }
    };

} // namespace apemodevk

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif

#define _Aux_TArrayTraits_Has_array_traits 1

namespace apemodevk {
    namespace utils {
        template < typename TArray >
        struct TArrayTraits;

        template < typename TArray, size_t N >
        struct TArrayTraits< TArray[ N ] > {
            static const size_t ArrayLength = N;
        };

        template < typename TArray, size_t TArraySize >
        constexpr size_t GetArraySize( TArray ( & )[ TArraySize ] ) {
            return TArraySize;
        }

        template < typename TArray, uint32_t TArraySize >
        constexpr uint32_t GetArraySizeU( TArray ( & )[ TArraySize ] ) {
            return TArraySize;
        }

#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif

#ifdef ARRAYSIZEU
#undef ARRAYSIZEU
#endif

#define ARRAYSIZE apemodevk::GetArraySize
#define ARRAYSIZEU apemodevk::GetArraySizeU

#ifdef ZeroMemory
#undef ZeroMemory
#endif

        template < typename T >
        inline void ZeroMemory( T &pObj ) {
            memset( static_cast< void * >( &pObj ), 0, sizeof( T ) );
        }

        template < typename T, size_t TCount >
        inline void ZeroMemory( T ( &pObj )[ TCount ] ) {
            memset( static_cast< void * >( &pObj[ 0 ] ), 0, sizeof( T ) * TCount );
        }

        template < typename T >
        inline void ZeroMemory( T *pObj, size_t Count ) {
            memset(  static_cast< void * >( pObj ), 0, sizeof( T ) * Count );
        }

    } // namespace utils
} // namespace apemodevk

namespace apemodevk {
    namespace details {
        using FlagsType = unsigned int;
        using VoidPtr   = void *;
        using IdType    = unsigned long long;
    } // namespace Details

    template < typename TDataFlags  = details::FlagsType,
               typename TDataSrcPtr = details::VoidPtr,
               typename TDataId     = details::IdType >
    struct TDataHandle {
        TDataId     DataId;
        TDataFlags  DataFlags;
        TDataSrcPtr DataAddress;

        TDataHandle( ) : DataId( TDataId( 0 ) ), DataFlags( TDataFlags( 0 ) ), DataAddress( TDataSrcPtr( nullptr ) ) {
        }
    };

    //
    // This can be usable for some native enums that have no operators defined.
    //

    /** Returns 'a | b' bits. */
    template < typename U >
    inline static U FlagsOr( _In_ U a, _In_ U b ) {
        return static_cast< U >( a | b );
    }
    /** Returns 'a & b' bits. */
    template < typename U >
    inline static U FlagsAnd( _In_ U a, _In_ U b ) {
        return static_cast< U >( a & b );
    }
    /** Returns 'a ^ b' bits. */
    template < typename U >
    inline static U FlagsXor( _In_ U a, _In_ U b ) {
        return static_cast< U >( a ^ b );
    }
    /** Returns inverted 'a' bits. */
    template < typename U >
    inline static U FlagsInv( _In_ U a ) {
        return static_cast< U >( ~a );
    }

    /** Sets 'b' bits to 'a'. Also see 'FlagsOr'. */
    template < typename U, typename V >
    inline static void SetFlag( _Out_ U &a, _In_ V b ) {
        a |= static_cast< U >( b );
    }
    /** Removes 'b' bits from 'a'. Also see 'FlagsInv', 'FlagsAnd'. */
    template < typename U, typename V >
    inline static void RemoveFlag( _Out_ U &a, _In_ V b ) {
        a &= ~static_cast< U >( b );
    }
    /** Returns true, if exactly 'b' bits are present in 'a'. */
    template < typename U, typename V >
    inline static bool HasFlagEq( _In_ U a, _In_ V b ) {
        return V( static_cast< V >( a ) & b ) == b;
    }
    /** Returns true, if some of 'b' bits are present in 'a'. */
    template < typename U, typename V >
    inline static bool HasFlagAny( _In_ U a, _In_ V b ) {
        return V( static_cast< V >( a ) & b ) != static_cast< V >( 0 );
    }

    typedef TDataHandle<> GenericUserDataHandle;
} // namespace apemodevk

namespace apemodevk {
    template < size_t _TEnumSize_ >
    struct TIntTypeForEnumOfSize;

    template <>
    struct TIntTypeForEnumOfSize< 1 > {
        static_assert( sizeof( char ) == 1, "Size mismatch." );
        using SignedIntType  = char;
        using UnignedIntType = unsigned char;
    };

    template <>
    struct TIntTypeForEnumOfSize< 2 > {
        static_assert( sizeof( short ) == 2, "Size mismatch." );
        using SignedIntType  = short;
        using UnignedIntType = unsigned short;
    };

    template <>
    struct TIntTypeForEnumOfSize< 4 > {
        static_assert( sizeof( int ) == 4, "Size mismatch." );
        using SignedIntType  = int;
        using UnignedIntType = unsigned int;
    };

    template <>
    struct TIntTypeForEnumOfSize< 8 > {
        static_assert( sizeof( long long ) == 8, "Size mismatch." );
        using SignedIntType  = long long;
        using UnignedIntType = unsigned long long;
    };

    // used as an approximation of std::underlying_type<T>
    template < class _TEnum_ >
    struct TIntTypeForEnum {
        using SignedIntType  = typename TIntTypeForEnumOfSize< sizeof( _TEnum_ ) >::SignedIntType;
        using UnignedIntType = typename TIntTypeForEnumOfSize< sizeof( _TEnum_ ) >::UnignedIntType;
    };
} // namespace apemodevk

#define _Game_engine_Define_enum_flag_operators( _TEnum_ )                                     \
                                                                                               \
    extern "C++" {                                                                             \
                                                                                               \
    inline _TEnum_ operator|( _TEnum_ a, _TEnum_ b ) {                                         \
        return _TEnum_( ( (apemodevk::TIntTypeForEnum< _TEnum_ >::SignedIntType) a ) |         \
                        ( (apemodevk::TIntTypeForEnum< _TEnum_ >::SignedIntType) b ) );        \
    }                                                                                          \
                                                                                               \
    inline _TEnum_ &operator|=( _TEnum_ &a, _TEnum_ b ) {                                      \
        return (_TEnum_ &) ( ( (apemodevk::TIntTypeForEnum< _TEnum_ >::SignedIntType &) a ) |= \
                             ( (apemodevk::TIntTypeForEnum< _TEnum_ >::SignedIntType) b ) );   \
    }                                                                                          \
                                                                                               \
    inline _TEnum_ operator&( _TEnum_ a, _TEnum_ b ) {                                         \
        return _TEnum_( ( (apemodevk::TIntTypeForEnum< _TEnum_ >::SignedIntType) a ) &         \
                        ( (apemodevk::TIntTypeForEnum< _TEnum_ >::SignedIntType) b ) );        \
    }                                                                                          \
                                                                                               \
    inline _TEnum_ &operator&=( _TEnum_ &a, _TEnum_ b ) {                                      \
        return (_TEnum_ &) ( ( (apemodevk::TIntTypeForEnum< _TEnum_ >::SignedIntType &) a ) &= \
                             ( (apemodevk::TIntTypeForEnum< _TEnum_ >::SignedIntType) b ) );   \
    }                                                                                          \
                                                                                               \
    inline _TEnum_ operator~( _TEnum_ a ) {                                                    \
        return _TEnum_( ~( (apemodevk::TIntTypeForEnum< _TEnum_ >::SignedIntType) a ) );       \
    }                                                                                          \
                                                                                               \
    inline _TEnum_ operator^( _TEnum_ a, _TEnum_ b ) {                                         \
        return _TEnum_( ( (apemodevk::TIntTypeForEnum< _TEnum_ >::SignedIntType) a ) ^         \
                        ( (apemodevk::TIntTypeForEnum< _TEnum_ >::SignedIntType) b ) );        \
    }                                                                                          \
                                                                                               \
    inline _TEnum_ &operator^=( _TEnum_ &a, _TEnum_ b ) {                                      \
        return (_TEnum_ &) ( ( (apemodevk::TIntTypeForEnum< _TEnum_ >::SignedIntType &) a ) ^= \
                             ( (apemodevk::TIntTypeForEnum< _TEnum_ >::SignedIntType) b ) );   \
    }                                                                                          \
    }
