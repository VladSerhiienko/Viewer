#pragma once

namespace apemode {
namespace platform {

class AppInput;
struct AppSurface;

struct IAppShell {
    virtual ~IAppShell( )                                    = default;
    virtual bool Initialize( const AppSurface* pAppSurface ) = 0;
    virtual bool Update( const AppInput* pAppInput )         = 0;
};

} // namespace platform
} // namespace apemode