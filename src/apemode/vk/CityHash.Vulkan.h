#pragma once

#include <stdint.h>
#include <functional>

namespace apemodevk
{
    // Hash function for a byte array.
    uint64_t CityHash64( const char *buf, uint64_t len );

    template < bool IsIntegral >
    struct TCityHash;

    template <>
    struct TCityHash< false > {
        template < typename T >
        static uint64_t CityHash64( T const &Obj ) {
            return apemodevk::CityHash64( reinterpret_cast< const char * >( &Obj ), sizeof( T ) );
        }
    };

    template <>
    struct TCityHash< true > {
        template < typename T >
        static uint64_t CityHash64( T const &Obj ) {
            return static_cast< uint64_t >( Obj );
        }
    };

    // Hash function for a byte array.
    template < typename T, bool IsIntegral = std::is_integral< T >::value >
    uint64_t CityHash64( T const &Obj ) {
        return TCityHash< IsIntegral >::CityHash64( Obj );
    }

    // Hash 128 input bits down to 64 bits of output.
    // This is intended to be a reasonably good hash function.
    uint64_t CityHash128to64( uint64_t a, uint64_t b );

    // Hash 128 input bits down to 64 bits of output.
    // This is intended to be a reasonably good hash function.
    template < typename T, bool IsIntegral = std::is_integral< T >::value >
    uint64_t CityHash128to64( uint64_t a, T const &Obj ) {
        if ( a )
            return CityHash128to64( a, TCityHash< IsIntegral >::CityHash64( Obj ) );
        else
            return TCityHash< IsIntegral >::CityHash64( Obj );
    }

    // Hash 128 input bits down to 64 bits of output.
    // This is intended to be a reasonably good hash function.
    // uint64_t Hash128to64(const uint128& x);

    struct CityHashBuilder64 {
        uint64_t Value;

        CityHashBuilder64( ) : Value( 0xc949d7c7509e6557 ) {
        }

        CityHashBuilder64( uint64_t OtherHash ) : Value( OtherHash ) {
        }

        CityHashBuilder64( const CityHashBuilder64 &Other ) : Value( Other.Value ) {
        }

        CityHashBuilder64( const void *pBuffer, const size_t BufferByteSize )
            : Value( CityHash64( reinterpret_cast< char const * >( pBuffer ), BufferByteSize ) ) {
        }

        template < typename TPlainStructure >
        explicit CityHashBuilder64( TPlainStructure const &Pod ) : Value( CityHash64( Pod ) ) {
        }

        void CombineWith( CityHashBuilder64 const &Other ) {
            Value = CityHash128to64( Value, Other.Value );
        }

        template < typename TPlainStructure >
        void CombineWith( TPlainStructure const &Pod ) {
            Value = CityHash128to64( Value, Pod );
        }

        void CombineWithBuffer( const void *pBuffer, const size_t BufferByteSize ) {
            const auto pBytes     = reinterpret_cast< char const * >( pBuffer );
            const auto BufferHash = CityHash64( pBytes, BufferByteSize );
            Value                 = CityHash128to64( Value, BufferHash );
        }

        template < typename TPlainStructure >
        void CombineWithArray( const TPlainStructure *pStructs, const size_t StructCount ) {
            constexpr size_t const StructStride = sizeof( TPlainStructure );

            const auto pBytes     = reinterpret_cast< char const * >( pStructs );
            const auto ByteCount  = StructCount * StructStride;
            const auto BufferHash = CityHash64( pBytes, ByteCount );
            Value                 = CityHash128to64( Value, BufferHash );
        }

        template < typename TPlainStructure >
        void CombineWithArray( TPlainStructure const *pStructsIt, TPlainStructure const *pStructsItEnd ) {
            constexpr size_t StructStride = sizeof( TPlainStructure );

            const size_t StructCount = static_cast< size_t >( std::distance( pStructsIt, pStructsItEnd ) );
            const uint64_t BufferHash = CityHash64( reinterpret_cast< char const * >( pStructsIt ), StructCount * StructStride );
            Value = CityHash128to64( Value, BufferHash );
        }

        CityHashBuilder64 &operator<<( CityHashBuilder64 const &Other ) {
            CombineWith( Other );
            return *this;
        }

        template < typename TPlainStructure >
        CityHashBuilder64 &operator<<( TPlainStructure const &Pod ) {
            CombineWith( Pod );
            return *this;
        }

        operator uint64_t( ) const {
            return Value;
        }

        struct CmpOpLess {
            bool operator( )( uint64_t HashA, uint64_t HashB ) const {
                return HashA < HashB;
            }
        };
        struct CmpOpGreater {
            bool operator( )( uint64_t HashA, uint64_t HashB ) const {
                return HashA > HashB;
            }
        };
        struct CmpOpGreaterEqual {
            bool operator( )( uint64_t HashA, uint64_t HashB ) const {
                return HashA >= HashB;
            }
        };
        struct CmpOpEqual {
            bool operator( )( uint64_t HashA, uint64_t HashB ) const {
                return HashA == HashB;
            }
        };

        template < typename THashmapKey = size_t >
        struct HashOp : protected std::hash< THashmapKey > {
            typedef std::hash< size_t > Base;
            THashmapKey operator( )( uint64_t const &Hashed ) const {
                return Base::operator( )( Hashed );
            }
        };
    };
}
