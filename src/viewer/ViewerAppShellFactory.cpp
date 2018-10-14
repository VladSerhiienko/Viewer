#include "ViewerAppShellFactory.h"
#include "vk/ViewerAppShellVk.h"

namespace apemode {
namespace viewer {
namespace vk {

class ViewerAppShellBridge : apemode::platform::IAppShell {
public:
    ViewerAppShellBridge( int args, const char** ppArgs ) {
        apemode::AppState::OnMain( arc, ppArgs );
    }

    ~ViewerAppShellBridge( ) {
        apemode::AppState::OnExit( );
    }

    bool Initialize( const AppSurface* pAppSurface ) override {
        // TODO
        return false;
    }

    bool Update( const AppInput* pAppInput ) override {
        // TODO
        return false;
    }

private:
    ViewerShell Shell;
};

} // namespace vk
} // namespace viewer
} // namespace apemode

apemode::unique_ptr< apemode::platform::IAppShell > apemode::viewer::vk::CreateViewer( int args, const char** ppArgs ) {
    return apemode::make_unique< apemode::platform::IAppShell >( apemode_new ViewerAppShellBridge( int args, const char** ppArgs ) );
}
