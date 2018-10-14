
#include "AppSurfaceSdl.h"
#include "InputManagerSdl.h"

#include <viewer/ViewerAppShellFactory.h>
#include <apemode/platform/Stopwatch.h>

#include <SDL_main.h>

#ifdef main
#undef main
#endif

int main( int argc, char** ppArgs ) {
    apemode_memory_allocation_scope;

    apemode::unique_ptr< apemode::platform::IAppShell > appShell( apemode::viewer::vk::CreateViewer( argc, ppArgs ) );
    if ( !appShell ) {
        return -1;
    }

    apemode::platform::sdl2::AppSurface appSurfaceSdl;
    if ( !appSurfaceSdl.Initialize( 1280, 800, "Viewer" ) ) {
        return -1;
    }

    apemode::platform::AppSurface appSurface;
    appSurface.OverrideWidth  = (int) appSurfaceSdl.GetWidth( );
    appSurface.OverrideHeight = (int) appSurfaceSdl.GetHeight( );

#if SDL_VIDEO_DRIVER_WINDOWS
    appSurface.hWindow   = appSurfaceSdl.hWnd;
    appSurface.hInstance = appSurfaceSdl.hInstance;
#elif SDL_VIDEO_DRIVER_COCOA
    assert( &appSurface.pViewIOS == &appSurface.pViewMacOS );
    appSurface.pViewIOS = appSurfaceSdl.pView;
#elif SDL_VIDEO_DRIVER_X11
    appSurface.pDisplayX11 = appSurfaceSdl.pDisplayX11;
    appSurface.pWindowX11  = &appSurfaceSdl.pWindowX11;
#endif

    apemode::platform::Stopwatch          stopwatch;
    apemode::platform::AppInput           appInputState;
    apemode::platform::sdl2::InputManager inputManagerSdl;

    if ( app && app->Initialize( &appSurface ) ) {
        apemode_memory_allocation_scope;
        inputManagerSdl.Update( appInputState, 0 );
        stopwatch.Start( );

        while ( app->Update( &appInputState ) ) {
            apemode_memory_allocation_scope;
            inputManagerSdl.Update( appInputState, stopwatch.GetElapsedSeconds( ) );
            stopwatch.Start( );
        }
    }

    return 0;
}
