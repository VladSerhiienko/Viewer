#pragma once

#include <CameraControllerBase.h>

namespace apemode {
    
    struct ModelViewCameraController : CameraControllerBase {
        XMFLOAT3 TargetDst;
        XMFLOAT3 PositionDst;
        XMFLOAT2 OrbitCurr;
        XMFLOAT2 ZRange;

        ModelViewCameraController( ) {
            ZRange.x = 0.1f;
            ZRange.y = 1000.0f;
            Reset( );
        }

        void Reset( ) {
            Target      = {0, 0, 0};
            Position    = {0, 0, -5};
            TargetDst   = {0, 0, 0};
            PositionDst = {0, 0, -5};
            OrbitCurr   = {0, 0};
        }

        void Orbit( XMFLOAT2 _dxdy ) override {
            XMStoreFloat2( &OrbitCurr, XMLoadFloat2( &OrbitCurr ) + XMLoadFloat2( &_dxdy ) );
        }

        void Dolly( XMFLOAT3 _dzxy ) override {
            auto targetDst   = XMLoadFloat3( &TargetDst );
            auto positionDst = XMLoadFloat3( &PositionDst );

            auto toTargetNorm = targetDst - positionDst;
            auto toTargetLen  = XMVector3Length( toTargetNorm );
            toTargetNorm /= toTargetLen;
            
            auto delta =  toTargetLen * _dzxy.z;
            auto newLens = toTargetLen + delta;
            auto newLen = XMVectorGetX(newLens);

            if ( ( ZRange.x < newLen || _dzxy.z < 0.0f ) && ( newLen < ZRange.y || _dzxy.z > 0.0f ) ) {
                positionDst += toTargetNorm * delta;
                XMStoreFloat3( &PositionDst, positionDst );
            }
        }

        void ConsumeOrbit( float _amount ) {
            auto orbitCurr   = XMLoadFloat2( &OrbitCurr );
            auto targetDst   = XMLoadFloat3( &TargetDst );
            auto position    = XMLoadFloat3( &Position );
            auto positionDst = XMLoadFloat3( &PositionDst );

            auto consume = orbitCurr * _amount;
            orbitCurr -= consume;

            auto toPosNorm = targetDst - positionDst;
            auto toPosLen  = XMVector3Length( toPosNorm );
            toPosNorm /= toPosLen;

            auto ll = apemodem::LatLongFromVec( toPosNorm ) + consume * XMVectorSet( 1, -1, 0, 0 );
            ll = XMVectorSetY( ll, std::min( 0.98f, std::max( 0.02f, XMVectorGetY( ll ) ) ) );

            auto tmp  = apemodem::VecFromLatLong( ll );
            auto diff = ( tmp - toPosNorm ) * toPosLen;

            position += diff;
            positionDst += diff;

            XMStoreFloat2( &OrbitCurr, orbitCurr );
            XMStoreFloat3( &PositionDst, positionDst );
        }

        void Update( float _dt ) override {
            const float amount = std::min( _dt / 0.1f, 1.0f );

            ConsumeOrbit( amount );

            auto target      = XMLoadFloat3( &Target );
            auto targetDst   = XMLoadFloat3( &TargetDst );
            auto position    = XMLoadFloat3( &Position );
            auto positionDst = XMLoadFloat3( &PositionDst );

            target   = XMVectorLerp( target, targetDst, amount );
            position = XMVectorLerp( position, positionDst, amount );

            XMStoreFloat3( &Target, target );
            XMStoreFloat3( &Position, position );
        }

    };
}