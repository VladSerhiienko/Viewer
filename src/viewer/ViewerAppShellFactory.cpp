#include "ViewerAppShellFactory.h"
#include <viewer/vk/ViewerShellVk.h>

namespace apemode {
namespace viewer {
namespace vk {

class ViewerAppShellBridge : public apemode::platform::IAppShell {
public:
    ViewerAppShellBridge( int args, const char** ppArgs ) {
        apemode::AppState::OnMain( args, ppArgs );
        pShell = apemode::make_unique< ViewerShell >( );
    }

    ~ViewerAppShellBridge( ) {
        pShell = nullptr;
        apemode::AppState::OnExit( );
    }

    bool Initialize( const apemode::platform::AppSurface* pAppSurface ) override {
        apemodevk::PlatformSurface platformSurface;
        platformSurface.OverrideExtent.width  = (uint32_t) pAppSurface->OverrideWidth;
        platformSurface.OverrideExtent.height = (uint32_t) pAppSurface->OverrideHeight;

#if VK_USE_PLATFORM_WIN32_KHR
        platformSurface.hWnd      = (HWND) pAppSurface->Windows.hWindow;
        platformSurface.hInstance = (HINSTANCE) pAppSurface->Windows.hInstance;
#endif

        return pShell->Initialize( &platformSurface );
    }

    bool Update( const apemode::platform::AppSurface* pAppSurface, const apemode::platform::AppInput* pAppInput ) override {
        VkExtent2D currentExtent;
        currentExtent.width  = (uint32_t) pAppSurface->OverrideWidth;
        currentExtent.height = (uint32_t) pAppSurface->OverrideHeight;
        return pShell->Update( currentExtent, pAppInput );
    }

private:
    apemode::unique_ptr< ViewerShell > pShell;
};

} // namespace vk
} // namespace viewer
} // namespace apemode

namespace std {
template <>
struct default_delete< apemode::platform::IAppShell > {
    void operator( )( apemode::platform::IAppShell* pAppShell ) {
        apemode_delete( pAppShell );
    }
};
} // namespace std

std::unique_ptr< apemode::platform::IAppShell > apemode::viewer::vk::CreateViewer( int args, const char** ppArgs ) {
    return std::unique_ptr< apemode::platform::IAppShell >( apemode_new ViewerAppShellBridge( args, ppArgs ) );
}
