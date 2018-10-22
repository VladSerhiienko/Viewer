//
//  GameViewController.m
//  Viewer iOS
//
//  Created by Vlad Serhiienko on 10/19/18.
//  Copyright © 2018 Vlad Serhiienko. All rights reserved.
//

#import "GameViewController.h"

#include "apemode/platform/AppInput.h"
#include "apemode/platform/AppSurface.h"
#include "viewer/ViewerAppShellFactory.h"

#include <vector>

struct App {
    std::unique_ptr< apemode::platform::IAppShell > shell;
    apemode::platform::AppSurface                   surface;
    apemode::platform::AppInput                     input;
};

#pragma mark -
#pragma mark GameViewController

@implementation GameViewController {
    CADisplayLink* displayLink;
    App            app;
}

- (void)dealloc {
    app.shell = nullptr;
    // [displayLink release];
    // [super dealloc];
}

- (void)renderLoop {
    app.shell->Update( &app.surface, &app.input );
}

- (void)viewDidLoad {
    [super viewDidLoad];

    self.view.contentScaleFactor = UIScreen.mainScreen.nativeScale;

    app.surface.iOS.pViewIOS   = (__bridge void*) self.view;
    app.surface.OverrideWidth  = 0; //(int) self.view.frame.size.width;
    app.surface.OverrideHeight = 0; //(int) self.view.frame.size.height;

    std::vector< const char* > ppszArgs;
    // TODO: Custom IAssetManager implementation and specified args.
    // ppszArgs.push_back("--assets");
    // ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Viewer/assets/**");
    // ppszArgs.push_back("--scene");
    // ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/flare-gun.fbxp");
    // ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/forest-house.fbxp");
    // ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/rainier-ak-3d.fbxp");

    app.shell = apemode::viewer::vk::CreateViewer( (int) ppszArgs.size( ), ppszArgs.data( ) );
    app.shell->Initialize( &app.surface );
    app.shell->Update( &app.surface, &app.input );

    const uint32_t preferredFramesPerSecond = 60;

    displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector( renderLoop )];
    [displayLink setPreferredFramesPerSecond:preferredFramesPerSecond];
    [displayLink addToRunLoop:NSRunLoop.currentRunLoop forMode:NSDefaultRunLoopMode];
}

@end

#pragma mark -
#pragma mark GameView

@implementation GameView

/** Returns a Metal-compatible layer. */
+ (Class)layerClass {
    return [CAMetalLayer class];
}

@end
