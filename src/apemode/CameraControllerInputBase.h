#pragma once

#include <Input.h>
#include <MathInc.h>

namespace apemode {

    /* Adapts the input for the camera */
    struct CameraControllerInputBase {
        XMFLOAT3 DollyDelta;
        XMFLOAT2 OrbitDelta;

        virtual void Update( float deltaSecs, apemode::Input const& _input, XMFLOAT2 _widthHeight ) {
        }
    };
} // namespace apemode