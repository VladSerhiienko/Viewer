#pragma once

#include <MathInc.h>

namespace apemode {

    struct BasicCamera {
        apemodem::mat4 ViewMatrix;
        apemodem::mat4 ProjMatrix;
    };
}