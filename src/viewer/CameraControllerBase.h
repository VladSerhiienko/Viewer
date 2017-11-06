#pragma once

#include <MathInc.h>

namespace apemode {

    struct CameraControllerBase {
        apemodem::vec3 Target;
        apemodem::vec3 Position;

        virtual void Orbit( apemodem::vec2 _dxdy ) { }
        virtual void Dolly( apemodem::vec3 _dzxy ) { }
        virtual void Update( float _dt ) { }

        virtual apemodem::XMMATRIX ViewMatrix( ) {
            return XMMatrixLookAtLH( XMLoadFloat3( &Position ), XMLoadFloat3( &Target ), g_XMIdentityR1.v );
        }

    };


}