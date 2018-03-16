#pragma once

#include <MathInc.h>

namespace apemode {
    struct CameraControllerBase {
        XMFLOAT3 Target;
        XMFLOAT3 Position;

        virtual void Orbit( XMFLOAT2 _dxdy ) { }
        virtual void Dolly( XMFLOAT3 _dzxy ) { }
        virtual void Update( float _dt ) { }

        virtual XMMATRIX ViewMatrix( ) {
            return XMMatrixLookAtLH( XMLoadFloat3( &Position ), XMLoadFloat3( &Target ), g_XMIdentityR1.v );
        }
    };
}