#pragma once

#include <CameraControllerBase.h>

namespace apemode {

    struct ModelViewCameraController : CameraControllerBase {
        XMFLOAT3 TargetDst;
        XMFLOAT3 PositionDst;
        XMFLOAT2 OrbitCurr;
        XMFLOAT2 ZRange;

        /* Set to 1 for inversing */
        XMFLOAT2 bInverseOrbitXY;

        ModelViewCameraController( ) {
            ZRange.x = 0.1f;
            ZRange.y = 10000.0f;
            Reset( );
        }

        void Reset( ) {
            Target          = {0, 0, 0};
            Position        = {0, 0, -5};
            TargetDst       = {0, 0, 0};
            PositionDst     = {0, 0, -5};
            OrbitCurr       = {0, 0};
            bInverseOrbitXY = {0, 0};
        }

        void Orbit( XMFLOAT2 _dxdy ) override {
            OrbitCurr.x += _dxdy.x;
            OrbitCurr.y += _dxdy.y;
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

            auto positionDstDelta = toTargetNorm * delta;
            if ( ( ZRange.x < newLen || _dzxy.z < 0.0f ) && ( newLen < ZRange.y || _dzxy.z > 0.0f ) ) {
                positionDst += positionDstDelta;
                XMStoreFloat3( &PositionDst, positionDst );
            }
        }

        void ConsumeOrbit( float _amount ) {
            const auto targetDst = XMLoadFloat3( &TargetDst );

            auto orbitCurr   = XMLoadFloat2( &OrbitCurr );
            auto position    = XMLoadFloat3( &Position );
            auto positionDst = XMLoadFloat3( &PositionDst );

            auto consume = orbitCurr * _amount;
            orbitCurr -= consume;

            auto toPosNorm = positionDst - targetDst;
            auto toPosLen  = XMVector3Length( toPosNorm );
            toPosNorm /= toPosLen;

            auto inv = XMVectorSet( -1.0f + bInverseOrbitXY.x * 2.0f, -1.0f +  + bInverseOrbitXY.y * 2.0f, 0, 0 );
            auto ll = LatLongFromVec( toPosNorm ) + consume * inv;
            ll = XMVectorSetY( ll, std::min( 0.98f, std::max( 0.02f, XMVectorGetY( ll ) ) ) );

            auto tmp  = VecFromLatLong( ll );
            auto diff = ( tmp - toPosNorm ) * toPosLen;

            position += diff;
            positionDst += diff;

            XMStoreFloat2( &OrbitCurr, orbitCurr );
            XMStoreFloat3( &Position, position );
            XMStoreFloat3( &PositionDst, positionDst );
        }

        void Update( float _dt ) override {
            const float amount = std::min( _dt / 0.1f, 1.0f );

            ConsumeOrbit( amount );

            const auto targetDst   = XMLoadFloat3( &TargetDst );
            const auto positionDst = XMLoadFloat3( &PositionDst );

            auto target   = XMLoadFloat3( &Target );
            auto position = XMLoadFloat3( &Position );

            target   = XMVectorLerp( target, targetDst, amount );
            position = XMVectorLerp( position, positionDst, amount );

            XMStoreFloat3( &Target, target );
            XMStoreFloat3( &Position, position );
        }

    };
}