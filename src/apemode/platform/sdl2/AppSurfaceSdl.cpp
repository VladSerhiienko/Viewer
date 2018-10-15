
#include "AppSurfaceSdl.h"

apemode::platform::sdl2::AppSurface::AppSurface( ) {
}

apemode::platform::sdl2::AppSurface::~AppSurface( ) {
    Finalize( );
}

#ifdef SDL_VIDEO_DRIVER_COCOA
void* GetNSViewFromNSWindow( void* pNSWindow, bool bEnableMetalLayer );
#endif

bool apemode::platform::sdl2::AppSurface::Initialize( uint32_t width, uint32_t height, const char* name ) {
    SDL_LogInfo( SDL_LOG_CATEGORY_VIDEO, "AppSurface: Initializing." );

    if ( nullptr == pSdlWindow && !SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) ) {
        const uint32_t sdlWindowProps = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;
        pSdlWindow = SDL_CreateWindow( name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, sdlWindowProps );

        if ( pSdlWindow ) {
            SDL_LogInfo( SDL_LOG_CATEGORY_VIDEO, "AppSurface: Created a window." );

            SDL_SysWMinfo windowInfo{};
            windowInfo.version.major = SDL_MAJOR_VERSION;
            windowInfo.version.minor = SDL_MINOR_VERSION;

            if ( SDL_TRUE == SDL_GetWindowWMInfo( pSdlWindow, &windowInfo ) ) {
#ifdef SDL_VIDEO_DRIVER_X11
                pDisplayX11 = windowInfo.info.x11.display;
                pWindowX11  = windowInfo.info.x11.window;
                SDL_LogInfo( SDL_LOG_CATEGORY_VIDEO, "AppSurface: Resolved Xlib window handles." );
#elif SDL_VIDEO_DRIVER_COCOA
                pView = GetNSViewFromNSWindow( windowInfo.info.cocoa.window, true );
                SDL_LogInfo( SDL_LOG_CATEGORY_VIDEO, "AppSurface: Resolved Cocoa window handle." );
#elif SDL_VIDEO_DRIVER_WINDOWS
                hWnd      = windowInfo.info.win.window;
                hInstance = (HINSTANCE) GetWindowLongPtrA( windowInfo.info.win.window, GWLP_HINSTANCE );
                SDL_LogInfo( SDL_LOG_CATEGORY_VIDEO, "AppSurface: Resolved Win32 handles." );
#endif
            }
        }
    }

    return nullptr != pSdlWindow;
}

void apemode::platform::sdl2::AppSurface::Finalize( ) {
    if ( pSdlWindow ) {
        SDL_LogInfo( SDL_LOG_CATEGORY_VIDEO, "AppSurface: Deleting window." );
        SDL_DestroyWindow( pSdlWindow );
        pSdlWindow = nullptr;
    }
}

uint32_t apemode::platform::sdl2::AppSurface::GetWidth( ) const {
    SDL_assert( pSdlWindow && "Not initialized." );

    int OutWidth;
    SDL_GetWindowSize( pSdlWindow, &OutWidth, nullptr );
    return static_cast< uint32_t >( OutWidth );
}

uint32_t apemode::platform::sdl2::AppSurface::GetHeight( ) const {
    SDL_assert( pSdlWindow && "Not initialized." );

    int OutHeight;
    SDL_GetWindowSize( pSdlWindow, nullptr, &OutHeight );
    return static_cast< uint32_t >( OutHeight );
}

void* apemode::platform::sdl2::AppSurface::GetWindowHandle( ) {
    return reinterpret_cast< void* >( pSdlWindow );
}