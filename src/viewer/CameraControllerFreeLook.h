#pragma once

#include <CameraControllerBase.h>

namespace apemode {
    
    struct FreeLookCameraController : CameraControllerBase {
        XMFLOAT3 TargetDst;
        XMFLOAT3 PositionDst;
        XMFLOAT2 OrbitCurr;
        XMFLOAT2 ZRange;

        FreeLookCameraController( ) {
            ZRange.x = 0.1f;
            ZRange.y = 1000.0f;
            Reset( );
        }

        void Reset( ) {
            Target      = {0, 0, 5};
            Position    = {0, 0, 0};
            TargetDst   = {0, 0, 5};
            PositionDst = {0, 0, 0};
            OrbitCurr   = {0, 0};
        }

        void Orbit( XMFLOAT2 _dxdy ) override {
            XMStoreFloat2( &OrbitCurr, XMLoadFloat2( &OrbitCurr ) + XMLoadFloat2( &_dxdy ) );
        }

        void Dolly( XMFLOAT3 _dxyz ) override {
            auto targetDst   = XMLoadFloat3( &TargetDst );
            auto positionDst = XMLoadFloat3( &PositionDst );

            auto toTargetNorm = targetDst - positionDst;
            auto toTargetLen  = XMVector3Length( toTargetNorm );
            toTargetNorm /= toTargetLen;

            auto right = XMVector3Cross( g_XMIdentityR1, toTargetNorm );
            auto up    = XMVector3Cross( toTargetNorm, right );

            auto delta = XMLoadFloat3( &_dxyz ) * toTargetLen;

            auto forwardDelta = toTargetNorm * XMVectorGetZ( delta );
            auto rightDelta   = right * XMVectorGetX( delta );
            auto upDelta      = up * XMVectorGetY( delta );

            targetDst += forwardDelta;
            targetDst += rightDelta;
            targetDst += upDelta;

            positionDst += forwardDelta;
            positionDst += rightDelta;
            positionDst += upDelta;

            XMStoreFloat3( &TargetDst, targetDst );
            XMStoreFloat3( &PositionDst, positionDst );
        }

        void ConsumeOrbit( float _amount ) {
            auto orbitCurr   = XMLoadFloat2( &OrbitCurr );
            auto targetDst   = XMLoadFloat3( &TargetDst );
            auto positionDst = XMLoadFloat3( &PositionDst );
            
            auto toPosNorm = targetDst - positionDst;
            auto toPosLen  = XMVector3Length( toPosNorm );
            toPosNorm /= toPosLen;

            auto ll = apemodem::LatLongFromVec( toPosNorm );

            auto consume = orbitCurr * _amount;
            orbitCurr -= consume;
            
            // consume.y *= ( ll.y < 0.02 && consume.y < 0 ) || ( ll.y > 0.98 && consume.y > 0 ) ? 0 : -1;
            auto lon = XMVectorGetY( ll );
            auto oy  = XMVectorGetY( consume );
            oy *= ( lon < 0.02 && oy < 0 ) || ( lon > 0.98 && oy > 0 ) ? 0 : -1;
            consume = XMVectorSetY( consume, oy );
            ll += consume;

            auto backward = apemode::VecFromLatLong( ll );
            auto diff = ( backward - toPosNorm ) * toPosLen;

            auto target = XMLoadFloat3( &Target );
            target += diff;
            targetDst += diff;

            auto diffDst = XMVector3Normalize( targetDst - positionDst );
            targetDst = positionDst + diffDst * ( ZRange.y - ZRange.x ) * 0.1f;

            XMStoreFloat2( &OrbitCurr, orbitCurr );
            XMStoreFloat3( &Target, target );
            XMStoreFloat3( &TargetDst, targetDst );
            XMStoreFloat3( &PositionDst, positionDst );
        }

        void Update( float _dt ) override {
            const float amount = std::min< float >( _dt / 0.1f, 1.0f );

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