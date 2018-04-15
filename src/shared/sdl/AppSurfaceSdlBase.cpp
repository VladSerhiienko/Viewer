#include "AppSurfaceSdlBase.h"
#include <AppState.h>

apemode::AppSurfaceSdlBase::AppSurfaceSdlBase( ) {
}

apemode::AppSurfaceSdlBase::~AppSurfaceSdlBase( ) {
    Finalize( );
}

bool apemode::AppSurfaceSdlBase::Initialize( uint32_t width, uint32_t height, const char* name ) {
    LogInfo( "apemode/AppSurfaceSdlBase/Initialize" );

    if ( nullptr == pSdlWindow && !SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) ) {
        const uint32_t sdlWindowProps = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
        pSdlWindow = SDL_CreateWindow( name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, sdlWindowProps );

        if ( pSdlWindow ) {
            LogInfo( "apemode/AppSurfaceSdlBase/Initialize: Created a windows." );

            SDL_SysWMinfo windowInfo{};
            windowInfo.version.major = SDL_MAJOR_VERSION;
            windowInfo.version.minor = SDL_MINOR_VERSION;

            if ( SDL_TRUE == SDL_GetWindowWMInfo( pSdlWindow, &windowInfo ) ) {
#ifdef X_PROTOCOL
                pDisplayX11 = windowInfo.info.x11.display;
                pWindowX11  = windowInfo.info.x11.window;
                LogInfo( "apemode/AppSurfaceSdlBase/Initialize: Resolved Xlib handles." );
#endif

#ifdef _WINDOWS_
                hWnd      = windowInfo.info.win.window;
                hInstance = (HINSTANCE) GetWindowLongPtrA( windowInfo.info.win.window, GWLP_HINSTANCE );
                LogInfo( "apemode/AppSurfaceSdlBase/Initialize: Resolved Win32 handles." );
#endif
            }
        }
    }

    return nullptr != pSdlWindow;
}

void apemode::AppSurfaceSdlBase::Finalize( ) {
    if ( pSdlWindow ) {
        LogInfo( "apemode/AppSurfaceSdlVk/Finalize: Deleting window." );
        SDL_DestroyWindow( pSdlWindow );
        pSdlWindow = nullptr;
    }
}

uint32_t apemode::AppSurfaceSdlBase::GetWidth( ) const {
    SDL_assert( pSdlWindow && "Not initialized." );

    int OutWidth;
    SDL_GetWindowSize( pSdlWindow, &OutWidth, nullptr );
    return static_cast< uint32_t >( OutWidth );
}

uint32_t apemode::AppSurfaceSdlBase::GetHeight( ) const {
    SDL_assert( pSdlWindow && "Not initialized." );

    int OutHeight;
    SDL_GetWindowSize( pSdlWindow, nullptr, &OutHeight );
    return static_cast< uint32_t >( OutHeight );
}

void* apemode::AppSurfaceSdlBase::GetWindowHandle( ) {
    return reinterpret_cast< void* >( pSdlWindow );
}