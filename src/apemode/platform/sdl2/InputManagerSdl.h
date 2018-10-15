#pragma once

#include <apemode/platform/AppInput.h>

namespace apemode {
namespace platform {
namespace sdl2 {

/* Updates input state, adapts it the current platform or framework.
 */
class AppInputManager {
public:
    AppInputManager( );
    void Update( AppInput& appInputState, float elapsedSeconds );

private:
    uint32_t KeyMapping[ kDigitalInput_NumKeys ];
};

} // namespace sdl2
} // namespace platform
} // namespace apemode
