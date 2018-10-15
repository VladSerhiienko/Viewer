#pragma once

#include <apemode/platform/MathInc.h>

namespace apemode {

    struct BasicCamera {
        XMFLOAT4X4 ViewMatrix;
        XMFLOAT4X4 ProjMatrix;
    };
}