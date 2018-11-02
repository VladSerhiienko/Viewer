//
//  GameViewController.m
//  Viewer macOS
//
//  Created by Vlad Serhiienko on 10/19/18.
//  Copyright © 2018 Vlad Serhiienko. All rights reserved.
//

#import "GameViewController.h"
//#import "Renderer.h"

#import <QuartzCore/CAMetalLayer.h>

#include "apemode/platform/AppInput.h"
#include "apemode/platform/AppSurface.h"
#include "viewer/ViewerAppShellFactory.h"

#include <vector>

struct App {
    std::unique_ptr< apemode::platform::IAppShell > shell;
    apemode::platform::AppSurface                   surface;
    apemode::platform::AppInput                     input;
};

/** Rendering loop callback function for use with a CVDisplayLink. */
namespace {
CVReturn DisplayLinkCallback( CVDisplayLinkRef   displayLink,
                              const CVTimeStamp* now,
                              const CVTimeStamp* outputTime,
                              CVOptionFlags      flagsIn,
                              CVOptionFlags*     flagsOut,
                              void*              target ) {
    App* app = static_cast< App* >( target );
    app->shell->Update( &app->surface, &app->input );
    return kCVReturnSuccess;
}

std::vector< const char* > getProcessArguments( ) {
    std::vector< const char* > ppszArgs;
    NSArray*                   args = [[NSProcessInfo processInfo] arguments];
    ppszArgs.reserve( ( size_t )[ args count ] );

    for ( NSString* arg in args )
        ppszArgs.push_back( [arg UTF8String] );

    return ppszArgs;
}
}

#pragma mark -
#pragma mark GameViewController

@implementation GameViewController {
    CVDisplayLinkRef displayLink;
    App              app;
}

-(void) dealloc {
    app.shell = nullptr;
    CVDisplayLinkRelease(displayLink);
    // [super dealloc];
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // Back the view with a layer created by the makeBackingLayer method.
    self.view.wantsLayer = YES;

    app.surface.macOS.pViewMacOS = (__bridge void*) self.view;
    app.surface.OverrideWidth    = 0; //(int) self.view.frame.size.width;
    app.surface.OverrideHeight   = 0; //(int) self.view.frame.size.height;

    std::vector< const char* > ppszArgs = getProcessArguments( );
    ppszArgs.push_back("--assets");
    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Viewer/assets/**");
    ppszArgs.push_back("--scene");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/sci-fi-girl.fbxp"); // -
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/crystal-stone.fbxp"); // +-
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/integrated-thermostatic-valve-slim-head.fbxp"); // -
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/run-hedgehog-run.fbxp"); // -
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/3000-followers-milestone.fbxp"); // M
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/anak-the-lizardman.fbxp"); // -
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/gecko.fbxp"); // -
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/sphere-bot.fbxp"); // -
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/opel-gt-retopo.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/vanille-flirty-animation.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/awesome-mix-guardians-of-the-galaxy.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/ice-cream-truck.fbxp"); // A+
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/rainier-ak-3d.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/flare-gun.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/izzy.fbxp"); // -
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/rusty-mecha.fbxp"); // M/A
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/futuristic-safe.fbxp"); // M/A
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/stesla.fbxp"); // M/A
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/littlest-tokyo.fbxp"); // M/A
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/k07-drone.fbxp"); // M+
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/arex-rex-zero1.fbxp"); // M
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/bristleback.fbxp"); // A
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/forest-house.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/subzero.fbxp"); // -

    app.shell = apemode::viewer::vk::CreateViewer( (int) ppszArgs.size( ), ppszArgs.data( ) );
    app.shell->Initialize( &app.surface );

    CVDisplayLinkCreateWithActiveCGDisplays( &displayLink );
    CVDisplayLinkSetOutputCallback( displayLink, &DisplayLinkCallback, &app );
    CVDisplayLinkStart( displayLink );
}

@end

#pragma mark -
#pragma mark GameView

@implementation GameView

/** Indicates that the view wants to draw using the backing layer instead of using drawRect:.  */
- (BOOL)wantsUpdateLayer {
    return YES;
}

/** Returns a Metal-compatible layer. */
+ (Class)layerClass {
    return [CAMetalLayer class];
}

/** If the wantsLayer property is set to YES, this method will be invoked to return a layer instance. */
- (CALayer*)makeBackingLayer {
    CALayer* layer = [self.class.layerClass layer];
    CGSize viewScale = [self convertSizeToBacking:CGSizeMake( 1.0, 1.0 )];
    layer.contentsScale = MIN( viewScale.width, viewScale.height );
    return layer;
}

@end
