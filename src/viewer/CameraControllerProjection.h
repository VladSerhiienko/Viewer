#pragma once
#include <MathInc.h>

namespace apemode {

    struct CameraProjectionController {
        inline XMMATRIX ProjBiasMatrix( ) {
            /* https://matthewwellings.com/blog/the-new-vulkan-coordinate-system/ */

            const auto bias = XMFLOAT4X4( 1.f, 0.0f, 0.0f, 0.0f,
                                          0.f, -1.f, 0.0f, 0.0f, /* Y -> -Y (flipped) */
                                          0.f, 0.0f, 0.5f, 0.5f, /* Z (-1, 1) -> (-0.5, 0.5) -> (0, 1)  */
                                          0.f, 0.0f, 0.0f, 1.0f );

            return XMLoadFloat4x4( &bias );
        }

        inline XMMATRIX ProjMatrix( float _fieldOfViewYDegs, float _width, float _height, float _nearZ, float _farZ ) {
            const float fovRads      = apemodexm::DegreesToRadians< float >( _fieldOfViewYDegs );
            const float aspectWOverH = _width / _height;

            return ProjBiasMatrix( ) * apemodexm::XMMatrixPerspectiveFovLH( fovRads, aspectWOverH, _nearZ, _farZ );
        }
    };
}