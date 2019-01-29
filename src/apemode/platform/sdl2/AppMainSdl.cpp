
#include "AppSurfaceSdl.h"
#include "InputManagerSdl.h"

#include <viewer/ViewerAppShellFactory.h>
#include <apemode/platform/Stopwatch.h>
#include <apemode/platform/DefaultAppShellCommand.h>
#include <apemode/platform/AppSurface.h>

#include <SDL_main.h>

#ifdef main
#undef main
#endif

int main( int argc, char** ppArgs ) {

    std::unique_ptr< apemode::platform::IAppShell > appShell( apemode::viewer::vk::CreateViewer( argc, (const char**) ppArgs ) );
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
    appSurface.Windows.hWindow   = appSurfaceSdl.hWnd;
    appSurface.Windows.hInstance = appSurfaceSdl.hInstance;
#elif SDL_VIDEO_DRIVER_COCOA
    assert( &appSurface.iOS.pViewIOS == &appSurface.macOS.pViewMacOS );
    appSurface.iOS.pViewIOS = appSurfaceSdl.pView;
#elif SDL_VIDEO_DRIVER_X11
    appSurface.X11.pDisplayX11 = appSurfaceSdl.pDisplayX11;
    appSurface.X11.pWindowX11  = &appSurfaceSdl.pWindowX11;
#endif

    apemode::platform::Stopwatch             stopwatch;
    apemode::platform::AppInput              appInputState;
    apemode::platform::sdl2::AppInputManager inputManagerSdl;

    apemode::platform::AppShellCommand initializeCommand;
    initializeCommand.Type = "Initialize";
    initializeCommand.Args[ "Surface" ].Value.SetPtrValue(&appSurface);
    
    apemode::platform::AppShellCommand updateCommand;
    updateCommand.Type = "Update";
    updateCommand.Args[ "Surface" ].Value.SetPtrValue(&appSurface);
    updateCommand.Args[ "Input" ].Value.SetPtrValue(&appInputState);

    if ( appShell && appShell->Execute( &initializeCommand ).bSucceeded ) {
        inputManagerSdl.Update( appInputState, 0 );
        stopwatch.Start( );

        while ( appShell->Execute( &updateCommand ).bSucceeded ) {
            inputManagerSdl.Update( appInputState, (float)stopwatch.GetElapsedSeconds( ) );
            stopwatch.Start( );
        }
    }

    return 0;
}
