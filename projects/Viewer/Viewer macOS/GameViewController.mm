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
#include "apemode/platform/DefaultAppShellCommand.h"
#include "viewer/ViewerAppShellFactory.h"

#include <vector>

struct App {
    std::unique_ptr< apemode::platform::IAppShell > shell;
    apemode::platform::AppSurface                   surface;
    apemode::platform::AppInput                     input;
    apemode::platform::AppShellCommand              initCmd;
    apemode::platform::AppShellCommand              updateCmd;
};

/** Rendering loop callback function for use with a CVDisplayLink. */
namespace {
apemode::platform::AppShellCommandArgumentValue&
getShellCmdArgument(apemode::platform::AppShellCommand& cmd, const char* pszName ) {
    auto& arg = cmd.Args[pszName];
    arg.Name = pszName;
    return arg.Value;
}

CVReturn DisplayLinkCallback( CVDisplayLinkRef   displayLink,
                              const CVTimeStamp* now,
                              const CVTimeStamp* outputTime,
                              CVOptionFlags      flagsIn,
                              CVOptionFlags*     flagsOut,
                              void*              target ) {
    App* app = static_cast< App* >( target );
    app->shell->Execute(&app->updateCmd);
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
    ppszArgs.clear();
    ppszArgs.push_back("--assets");
    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Viewer/assets/**");
    ppszArgs.push_back("--scene");
    
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/mech-m-6k.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/starburst.fbxp"); // M+
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/mountain-king.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/dji.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/colt-rebel-357.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/lovecraftian.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/ice-cream-truck.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/opel-gt-retopo.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/vanille-flirty-animation.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/awesome-mix-guardians-of-the-galaxy.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/rainier-ak-3d.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/flare-gun.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/crystal-stone.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/forest-house.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/anak-the-lizardman.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/k07-drone.fbxp"); // ?M
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/arex-rex-zero1.fbxp"); // M
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/3000-followers-milestone.fbxp"); // M
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/stesla.fbxp"); // M
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/gecko.fbxp"); // A
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/sphere-bot.fbxp"); // A
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/subzero.fbxp"); // A
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/rusty-mecha.fbxp"); // M/A
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/futuristic-safe.fbxp"); // M/A
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/run-hedgehog-run.fbxp"); // M/A
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/lord-inquisitor-servo-skull.fbxp"); // M/A
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/subzero-b-posicao-vitoria.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/luminaris-starship.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/mech-drone.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/mech-drone.fbxp");

//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/kgirls01.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/alien.fbxp"); // M/A
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/littlest-tokyo.fbxp"); // M/A

//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/integrated-thermostatic-valve-slim-head.fbxp"); // Crash

//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/sci-fi-girl.fbxp"); // M/BlendShapes?
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/bristleback-dota-fan-art.fbxp"); // BlendShapes?
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/izzy-animated-female-character-free-download.fbxp"); // M/BlendShapes?
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/combat-jet-animation.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/ethiziel-whip-of-the-icestorm.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/wolf-with-animations.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/lord-inquisitor-servo-skull.fbxp");
    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/buster-drone.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/combat-jet-animation.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/combat-jet-animation.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/combat-jet-animation.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/combat-jet-animation.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/combat-jet-animation.fbxp");

    app.shell = apemode::viewer::vk::CreateViewer( (int) ppszArgs.size( ), ppszArgs.data( ) );
    
    app.initCmd.Type = "Initialize";
    getShellCmdArgument(app.initCmd, "Surface").SetPtrValue(&app.surface);
    
    app.updateCmd.Type = "Update";
    getShellCmdArgument(app.updateCmd, "Surface").SetPtrValue(&app.surface);
    getShellCmdArgument(app.updateCmd, "Input").SetPtrValue(&app.input);
    
    app.shell->Execute(&app.initCmd);

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
