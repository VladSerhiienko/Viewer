#include "ViewerAppShellFactory.h"
#include <viewer/vk/ViewerShellVk.h>
#include <memory>
#include <string.h>

namespace apemode {
namespace viewer {
namespace vk {

void DumpCmd( const apemode::platform::IAppShellCommand* pCmd ) {
    if ( pCmd ) {
        apemode::LogInfo( "Cmd @{}, type: {}", (const void*)pCmd, pCmd->GetType( ) );
    } else {
        apemode::LogError( "Nullptr command." );
    }
}

bool IsCmdOfType( const apemode::platform::IAppShellCommand* pCmd, const char *pszType ) {
    return pCmd && !strcmp( pCmd->GetType( ), pszType );
}

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

    apemode::platform::AppShellCommandResult Execute( const apemode::platform::IAppShellCommand* pCmd ) override {
        using namespace  apemode::platform;

        using T = IAppShellCommandArgumentValue::ValueType;

        if ( IsCmdOfType( pCmd, "Initialize" ) ) {
            auto pSurfaceArg = pCmd->FindArgumentByName( "Surface" );
            assert( pSurfaceArg && pSurfaceArg->GetValue( ) && pSurfaceArg->GetValue( )->GetType( ) == T::kValueType_PtrValue );

            auto pAppSurface = (const AppSurface*) pSurfaceArg->GetValue( )->GetPtrValue( );
            apemode::platform::AppShellCommandResult result;
            result.bSucceeded = Initialize( pAppSurface );
            return result;
        } else if ( IsCmdOfType( pCmd, "Update" ) ) {
            auto pSurfaceArg = pCmd->FindArgumentByName( "Surface" );
            auto pInputArg   = pCmd->FindArgumentByName( "Input" );

            assert( pSurfaceArg && pSurfaceArg->GetValue( ) && pSurfaceArg->GetValue( )->GetType( ) == T::kValueType_PtrValue );
            assert( pInputArg && pInputArg->GetValue( ) && pInputArg->GetValue( )->GetType( ) == T::kValueType_PtrValue );

            auto pAppSurface = (const AppSurface*) pSurfaceArg->GetValue( )->GetPtrValue( );
            auto pAppInput   = (const AppInput*) pInputArg->GetValue( )->GetPtrValue( );
            apemode::platform::AppShellCommandResult result;
            result.bSucceeded = Update( pAppSurface, pAppInput );
            return result;
        }
    
        apemode::LogWarn( "Unprocessed command:" );
        DumpCmd( pCmd );

        AppShellCommandResult result;
        result.bSucceeded = false;
        return result;
}

protected:

    bool Initialize( const apemode::platform::AppSurface* pAppSurface ) {
        apemodevk::PlatformSurface platformSurface;
        platformSurface.OverrideExtent.width  = (uint32_t) pAppSurface->OverrideWidth;
        platformSurface.OverrideExtent.height = (uint32_t) pAppSurface->OverrideHeight;

#if VK_USE_PLATFORM_WIN32_KHR
        platformSurface.hWnd      = (HWND) pAppSurface->Windows.hWindow;
        platformSurface.hInstance = (HINSTANCE) pAppSurface->Windows.hInstance;
#endif
#if VK_USE_PLATFORM_MACOS_MVK
        platformSurface.pViewMacOS = pAppSurface->macOS.pViewMacOS;
#endif
#if VK_USE_PLATFORM_IOS_MVK
        platformSurface.pViewIOS = pAppSurface->iOS.pViewIOS;
#endif

        return pShell->Initialize( &platformSurface );
    }

    bool Update( const apemode::platform::AppSurface* pAppSurface, const apemode::platform::AppInput* pAppInput ) {
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
    return std::unique_ptr< apemode::platform::IAppShell >( new ViewerAppShellBridge( args, ppArgs ) );
}
