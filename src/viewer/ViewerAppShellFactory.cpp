#include "ViewerAppShellFactory.h"
#include <viewer/vk/ViewerShellVk.h>

namespace apemode {
namespace viewer {
namespace vk {

class ViewerAppShellBridge : public apemode::platform::IAppShell {
public:
    ViewerAppShellBridge( int args, const char** ppArgs ) {
        apemode::AppState::OnMain( args, ppArgs );
    }

    ~ViewerAppShellBridge( ) {
        apemode::AppState::OnExit( );
    }

    bool Initialize( const apemode::platform::AppSurface* pAppSurface ) override {
        // TODO
        return false;
    }

    bool Update( const apemode::platform::AppInput* pAppInput ) override {
        // TODO
        return false;
    }

private:
    ViewerShell Shell;
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
