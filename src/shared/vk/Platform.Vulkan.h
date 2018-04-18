#pragma once


#ifdef _WIN32

#ifndef STRICT
#define STRICT
#endif

#ifndef NOMINMAX
// Macros min(a,b) and max(a,b)
#define NOMINMAX
#endif

#ifdef _WINDOWS_
#error "Windows.h was included somewhere before, please use this header instead"
#endif // _WINDOWS_

// WIN32_LEAN_AND_MEAN excludes rarely-used services from windows headers.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define NOGDICAPMASKS			// CC_*, LC_*, PC_*, CP_*, TC_*, RC_
//#define NOVIRTUALKEYCODES		// VK_*
//#define NOWINMESSAGES			// WM_*, EM_*, LB_*, CB_*
//#define NOWINSTYLES			// WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
//#define NOSYSMETRICS			// SM_*
// #define NOMENUS				// MF_*
//#define NOICONS				// IDI_*
//#define NOKEYSTATES			// MK_*
//#define NOSYSCOMMANDS			// SC_*
//#define NORASTEROPS			// Binary and Tertiary raster ops
//#define NOSHOWWINDOW			// SW_*
#define OEMRESOURCE				// OEM Resource values
#define NOATOM					// Atom Manager routines
//#define NOCLIPBOARD			// Clipboard routines
//#define NOCOLOR				// Screen colors
//#define NOCTLMGR				// Control and Dialog routines
#define NODRAWTEXT				// DrawText() and DT_*
//#define NOGDI					// All GDI #defines and routines
#define NOKERNEL				// All KERNEL #defines and routines
//#define NOUSER				// All USER #defines and routines
//#define NONLS					// All NLS #defines and routines
//#define NOMB					// MB_* and MessageBox()
#define NOMEMMGR				// GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE				// typedef METAFILEPICT
#ifndef NOMINMAX
#define NOMINMAX				// Macros min(a,b) and max(a,b)
#endif
//#define NOMSG					// typedef MSG and associated routines
#define NOOPENFILE				// OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL				// SB_* and scrolling routines
#define NOSERVICE				// All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND					// Sound driver routines
//#define NOTEXTMETRIC			// typedef TEXTMETRIC and associated routines
//#define NOWH					// SetWindowsHook and WH_*
//#define NOWINOFFSETS			// GWL_*, GCL_*, associated routines
#define NOCOMM					// COMM driver routines
#define NOKANJI					// Kanji support stuff.
#define NOHELP					// Help engine interface.
#define NOPROFILER				// Profiler interface.
#define NODEFERWINDOWPOS		// DeferWindowPos routines
#define NOMCX					// Modem Configuration Extensions
#define NOCRYPT
#define NOTAPE
#define NOIMAGE
#define NOPROXYSTUB
#define NORPC

 #include <Windows.h>

// Undo any Windows defines.
#undef uint8
#undef uint16
#undef uint32
#undef int32
#undef float
#undef MAX_uint8
#undef MAX_uint16
#undef MAX_uint32
#undef MAX_int32
#undef CDECL
#undef PF_MAX
#undef PlaySound
#undef DrawText
#undef CaptureStackBackTrace
#undef MemoryBarrier
#undef DeleteFile
#undef MoveFile
#undef CopyFile
#undef CreateDirectory

// SIMD intrinsics
#include <intrin.h>

#include <stdint.h>
#include <tchar.h>
#include <intsafe.h>
#include <strsafe.h>
#include <process.h>
#include <sal.h>
#include <synchapi.h>
#include <varargs.h>
#include <winnt.h>
#include <wrl.h>
#define VK_USE_PLATFORM_WIN32_KHR 1

#else

// We'll get these headers included early on to avoid compiler errors with that.
#define _In_
#define _Out_
#define VK_USE_PLATFORM_XLIB_KHR 1

#endif

#include <assert.h>
#include <future>
#include <intsafe.h>
#include <inttypes.h>
#include <iostream>
#include <map>
#include <memory>
#include <memory.h>
#include <mutex>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unordered_map>
#include <iterator>
#include <algorithm>
#include <utility>
#include <stdarg.h>
#include <vector>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <spirv_glsl.hpp>

namespace apemode {
    template < typename T, typename... Args >
    std::unique_ptr< T > make_unique( Args&&... args ) {
        return std::unique_ptr< T >( new T( std::forward< Args >( args )... ) );
    }
} // namespace apemode

namespace apemodevk {

    constexpr uint32_t kInvalidIndex   = UINT_ERROR;
    constexpr uint64_t kInvalidIndex64 = UINT64_ERROR;

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
        memset( &pObj, 0, sizeof( T ) );
    }

    template < typename T, size_t TCount >
    inline void ZeroMemory( T ( &pObj )[ TCount ] ) {
        memset( &pObj[ 0 ], 0, sizeof( T ) * TCount );
    }

    template < typename T >
    inline void ZeroMemory( T *pObj, size_t Count ) {
        memset( pObj, 0, sizeof( T ) * Count );
    }

    template < typename T, typename... TArgs >
    inline T &PushBackAndGet( std::vector< T > &_collection, TArgs... args ) {
        _collection.emplace_back( std::forward< TArgs >( args )... );
        return _collection.back( );
    }
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
    template < typename T >
    static void TSafeDeleteObj( T *&pObj ) {
        if ( apemode_likely( pObj != nullptr ) )
            delete pObj, pObj = nullptr;
    }

    template < typename T >
    static void TSafeDeleteObj( T const *&pObj ) {
        if ( apemode_likely( pObj != nullptr ) )
            delete const_cast< T * >( pObj ), pObj = nullptr;
    }

    template < class TDecoratedObj >
    struct TSafeDeleteObjOp {
        using TObj = typename std::decay< TDecoratedObj >::type;
        void operator( )( TObj *pObj ) {
            apemodevk::TSafeDeleteObj< TObj >( pObj );
        }
        void operator( )( TObj const *pObj ) {
            apemodevk::TSafeDeleteObj< TObj >( pObj );
        }
    };
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
