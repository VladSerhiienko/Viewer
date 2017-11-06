#pragma once

#include <math.h>
#include <Inc/DirectXMath.h>
#include <algorithm>

namespace apemodem {
    using namespace DirectX;
    using vec2 = DirectX::XMFLOAT2;
    using vec3 = DirectX::XMFLOAT3;
    using vec4 = DirectX::XMFLOAT4;
    using mat4 = DirectX::XMFLOAT4X4;
    
    static const float kPi               = 3.1415926535897932f;
    static const float kSmallNumber      = 1.e-8f;
    static const float kKindaSmallNumber = 1.e-4f;
    static const float kBigNumber        = 3.4e+38f;
    static const float kEulersNumber     = 2.71828182845904523536f;
    static const float kMaxFloat         = 3.402823466e+38f;
    static const float kInversePi        = 0.31830988618f;
    static const float kPiDiv2           = 1.57079632679f;
    static const float kSmallDelta       = 0.00001f;
    static const float k90               = kPiDiv2;
    static const float k180              = kPi;

    inline vec2 LatLongFromVec( const vec3& _vec ) {
        const float phi   = atan2f( _vec.x, _vec.z );
        const float theta = acosf( _vec.y );

        return {( kPi + phi ) * kInversePi * 0.5f, theta * kInversePi};
    }

    inline XMVECTOR LatLongFromVec( XMVECTOR _vec ) {
        XMFLOAT3 v;
        XMStoreFloat3( &v, _vec );
        XMFLOAT2 latLon = LatLongFromVec( v );
        return XMLoadFloat2( &latLon );
    }

    inline vec3 VecFromLatLong( vec2 _uv ) {
        const float phi   = _uv.x * 2.0f * kPi;
        const float theta = _uv.y * kPi;

        const float st = sin( theta );
        const float sp = sin( phi );
        const float ct = cos( theta );
        const float cp = cos( phi );

        return {-st * sp, ct, -st * cp};
    }

    inline XMVECTOR VecFromLatLong( XMVECTOR _uv ) {
        XMFLOAT2 uv;
        XMStoreFloat2( &uv, _uv );
        XMFLOAT3 vec = VecFromLatLong( uv );
        return XMLoadFloat3( &vec );
    }

    template < class T >
    inline auto RadiansToDegrees( T const& RadVal ) -> decltype( RadVal * ( 180.f / kPi ) ) {
        return RadVal * ( 180.f / kPi );
    }

    template < class T >
    inline auto DegreesToRadians( T const& DegVal ) -> decltype( DegVal * ( kPi / 180.f ) ) {
        return DegVal * ( kPi / 180.f );
    }

    inline bool IsNearlyEqual( float A, float B, float ErrorTolerance = kSmallNumber ) {
        return fabsf( A - B ) <= ErrorTolerance;
    }

    inline bool IsNearlyEqual( vec2 const A, vec2 const B, const float ErrorTolerance = kSmallNumber ) {
        return fabsf( A.x - B.x ) <= ErrorTolerance && fabsf( A.y - B.y ) <= ErrorTolerance;
    }

    inline bool IsNearlyEqual( vec3 const A, vec3 const B, const float ErrorTolerance = kSmallNumber ) {
        return fabsf( A.x - B.x ) <= ErrorTolerance && fabsf( A.y - B.y ) <= ErrorTolerance && fabsf( A.z - B.z ) <= ErrorTolerance;
    }

    inline bool IsNearlyZero( float Value, float ErrorTolerance = kSmallNumber ) {
        return fabsf( Value ) <= ErrorTolerance;
    }

    inline bool IsNearlyZero( vec2 const Value, float ErrorTolerance = kSmallNumber ) {
        return fabsf( Value.x ) <= ErrorTolerance && fabsf( Value.y ) <= ErrorTolerance;
    }

    inline bool IsNearlyZero( vec3 const Value, float ErrorTolerance = kSmallNumber ) {
        return fabsf( Value.x ) <= ErrorTolerance && fabsf( Value.y ) <= ErrorTolerance && fabsf( Value.z ) <= ErrorTolerance;
    }
}

namespace apemode {
    using namespace apemodem;
}