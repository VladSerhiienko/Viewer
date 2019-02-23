
#include "AppSurfaceSdl.h"
#include "InputManagerSdl.h"

#include <viewer/ViewerAppShellFactory.h>
#include <apemode/platform/Stopwatch.h>
#include <apemode/platform/DefaultAppShellCommand.h>
#include <apemode/platform/AppSurface.h>
#include <apemode/platform/shared/AssetManager.h>

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
    apemode::platform::shared::AssetManager  assetManager;

    assetManager.UpdateAssets( "C:/Sources/Viewer/assets/**", nullptr, 0 );
    assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/dredd.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/dredd.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/1972-datsun-240k-gt-yellow.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/1967+Shelby+Mustang+G.T.+500.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/run-hedgehog-run.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/soul-of-cinder-dark-souls-3-boss-tilt-brush.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/hurricane-maria.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/Aston Martin Vantage.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/commander-saru.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/zbrush-for-concept-mech-design-dver.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/evoque2017dz.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/evoque2017.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/rage+rover+svr+2016.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/male-bust-sculpt.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/carefree-speedsculpt.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/C63+AMG+Coupe+W204.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/demon-sculpture-tilt-brush.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/indominus-rex-4k-free-model.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/rexy-tyranosaurus.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/2085-restomod-jetvette-free-download.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/lamborghini+urus.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/Macan+Hamann.fbxp" );
    //assetManager.AddAsset( "shared/scene.fbxp", "C:/Sources/Models/FbxPipelineDrc/run-hedgehog-run.fbxp" );

    apemode::platform::AppShellCommand initializeCommand;
    initializeCommand.Type = "Initialize";
    initializeCommand.Args[ "Surface" ].Value.SetPtrValue( &appSurface );

    apemode::platform::AppShellCommand assetManagerCmd;
    assetManagerCmd.Type = "SetAssetManager";
    assetManagerCmd.Args[ "AssetManager" ].Value.SetPtrValue( &assetManager );

    apemode::platform::AppShellCommand updateCommand;
    updateCommand.Type = "Update";
    updateCommand.Args[ "Surface" ].Value.SetPtrValue(&appSurface);
    updateCommand.Args[ "Input" ].Value.SetPtrValue(&appInputState);

    appShell->Execute( &assetManagerCmd );
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
