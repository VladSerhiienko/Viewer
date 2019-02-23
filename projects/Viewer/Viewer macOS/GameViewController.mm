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
#include "apemode/platform/shared/AssetManager.h"

#include "viewer/ViewerAppShellFactory.h"

#include <vector>

struct App {
    std::unique_ptr< apemode::platform::IAppShell > shell;
    apemode::platform::AppSurface                   surface;
    apemode::platform::shared::AssetManager         assetManager;
    apemode::platform::AppInput                     input;
    apemode::platform::AppShellCommand              initCmd;
    apemode::platform::AppShellCommand              updateCmd;
    apemode::platform::AppShellCommand              assetManagerCmd;
};

namespace {
apemode::platform::AppShellCommandArgumentValue&
getShellCmdArgument( apemode::platform::AppShellCommand &cmd, const char * pszName ) {
    auto &arg = cmd.Args[ pszName ];
    arg.Name = pszName;
    return arg.Value;
}

void updateInput(App *app) {
    memcpy( app->input.Buttons[ 1 ], app->input.Buttons[ 0 ], sizeof( app->input.Buttons[ 0 ] ) );
}

/** Rendering loop callback function for use with a CVDisplayLink. */
CVReturn DisplayLinkCallback( CVDisplayLinkRef   displayLink,
                              const CVTimeStamp *now,
                              const CVTimeStamp *outputTime,
                              CVOptionFlags      flagsIn,
                              CVOptionFlags *    flagsOut,
                              void *             target ) {
    App * app = static_cast< App * >( target );
    apemode::platform::AppShellCommandResult result = app->shell->Execute( &app->updateCmd );
    updateInput( app );
    return result.bSucceeded ? kCVReturnSuccess : kCVReturnError;
}

//std::vector< const char* > getProcessArguments( ) {
//    std::vector< const char* > ppszArgs;
//
//    NSArray* args = [[NSProcessInfo processInfo] arguments];
//    ppszArgs.reserve( ( size_t )[ args count ] );
//
//    for ( NSString* arg in args )
//        ppszArgs.push_back( [arg UTF8String] );
//
//    return ppszArgs;
//}
}

#pragma mark -
#pragma mark GameViewController

@implementation GameViewController {
    CVDisplayLinkRef displayLink;
    App              app;
}

-(void)dealloc {
    app.shell = nullptr;
    CVDisplayLinkRelease( displayLink );
}

-(void)mouseEntered:(NSEvent *)e {
    NSLog(@"mouseEntered");
}

-(void)mouseExited:(NSEvent *)e {
    NSLog(@"mouseExited");
}

-(void)setMouseLocation:(NSEvent *)e {
    NSRect  bounds      = [[self view] bounds];
    CGSize  backingSize = [[self view] convertSizeToBacking:bounds.size];
    NSPoint cursorPoint = [e locationInWindow];

    NSPoint relativePoint = cursorPoint;
    relativePoint.x /= bounds.size.width;
    relativePoint.y /= bounds.size.height;
    relativePoint.y = 1.0f - relativePoint.y;

    NSPoint adjustedPoint = relativePoint;
    adjustedPoint.x *= backingSize.width;
    adjustedPoint.y *= backingSize.height;

    app.input.Analogs[ apemode::platform::kAnalogInput_MouseX ] = adjustedPoint.x;
    app.input.Analogs[ apemode::platform::kAnalogInput_MouseY ] = adjustedPoint.y;
}

-(void)scrollWheel:(NSEvent *)e {
    app.input.Analogs[ apemode::platform::kAnalogInput_MouseHorzScroll ] = e.deltaX / 100.0f;
    app.input.Analogs[ apemode::platform::kAnalogInput_MouseVertScroll ] = e.deltaY / 100.0f;
}

-(void)mouseMoved:(NSEvent *)e {
    [self setMouseLocation:e];
}

-(void)rightMouseDown:(NSEvent *)e {
    [self setMouseLocation:e];
    app.input.Buttons[ 0 ][ apemode::platform::kDigitalInput_Mouse1 ] = true;
}

-(void)rightMouseUp:(NSEvent *)e {
    [self setMouseLocation:e];
    app.input.Buttons[ 0 ][ apemode::platform::kDigitalInput_Mouse1 ] = false;
}

-(void)rightMouseDragged:(NSEvent *)e {
    [self setMouseLocation:e];
}

-(void)mouseDown:(NSEvent *)e {
    [self setMouseLocation:e];
    app.input.Buttons[ 0 ][ apemode::platform::kDigitalInput_Mouse0 ] = true;
}

-(void)mouseUp:(NSEvent *)e {
    [self setMouseLocation:e];
    app.input.Buttons[ 0 ][ apemode::platform::kDigitalInput_Mouse0 ] = false;
}

-(void)mouseDragged:(NSEvent *)e {
    [self setMouseLocation:e];
}

-(void)viewDidLoad {
    [super viewDidLoad];

    // Back the view with a layer created by the makeBackingLayer method.
    self.view.wantsLayer = YES;

    app.surface.macOS.pViewMacOS = (__bridge void*) self.view;
    app.surface.OverrideWidth    = 0; //(int) self.view.frame.size.width;
    app.surface.OverrideHeight   = 0; //(int) self.view.frame.size.height;
    
    app.assetManager.UpdateAssets("/Users/vlad.serhiienko/Projects/Home/Viewer/assets/**", nullptr, 0);
    //
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/run-hedgehog-run.fbxp" ); //
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/helicopter-ec135-special-forces.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/izzy-animated-female-character-free-download.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/early-medieval-nasal-helmet.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/sniper.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/1972-datsun-240k-gt-non-commercial-use-only.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/mq-9-reaper.fbxp" );
    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/run-hedgehog-run.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/stesla.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/dji.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/security-cyborg.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/horned_infernal_duke.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/littlest-tokyo.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/plasma-revolver-animation.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/colt-rebel-357.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/mountain-king.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/combat-helmet-k6-3.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/combat-jet-animation.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/bean-bot.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/speedling-realistic.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/security-cyborg.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/littlest-tokyo.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/alien-head.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/super-human.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/rainier-ak-3d.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/qutiix-stylised-circus-baby-sister-location.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/sculptjanuary19-day22-jungle.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/tentacle-creature-day-2-sculptjanuary-2016.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/horned_infernal_duke.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/mech-m-6k.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/stesla.fbxp" );
//    app.assetManager.AddAsset( "shared/scene.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/dji.fbxp" );
//    app.assetManager.AddAsset( "shared/stesla.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc_8/stesla.fbxp" );
//    app.assetManager.AddAsset( "shared/horned_infernal_duke.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/horned_infernal_duke.fbxp" );
//    app.assetManager.AddAsset( "shared/horned_infernal_duke_8.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc_8/horned_infernal_duke.fbxp" );
//    app.assetManager.AddAsset( "shared/horned_infernal_duke_16.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc_16/horned_infernal_duke.fbxp" );
//    app.assetManager.AddAsset( "shared/0004.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc_16/0004.fbxp" );
//    app.assetManager.AddAsset( "shared/0005.fbxp", "/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc_16/0005.fbxp" );
    
    _mm_lfence();
    
    std::vector< const char* > ppszArgs; // = getProcessArguments( );
//    ppszArgs.clear();
//    ppszArgs.push_back("--scene");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/horned_infernal_duke.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/horned_infernal_duke.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/holotech-bench.fbxp"); // +
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipelineDrc/mech-m-6k.fbxp"); // +
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
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/buster-drone.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/combat-jet-animation.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/combat-jet-animation.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/combat-jet-animation.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/combat-jet-animation.fbxp");
//    ppszArgs.push_back("/Users/vlad.serhiienko/Projects/Home/Models/FbxPipeline/combat-jet-animation.fbxp");

    app.shell = apemode::viewer::vk::CreateViewer( (int) ppszArgs.size( ), ppszArgs.data( ) );
    
    app.assetManagerCmd.Type = "SetAssetManager";
    getShellCmdArgument( app.assetManagerCmd, "AssetManager" ).SetPtrValue( &app.assetManager );
    app.shell->Execute( &app.assetManagerCmd );
    
    app.initCmd.Type = "Initialize";
    getShellCmdArgument( app.initCmd, "Surface" ).SetPtrValue( &app.surface );
    
    app.updateCmd.Type = "Update";
    getShellCmdArgument( app.updateCmd, "Surface" ).SetPtrValue( &app.surface );
    getShellCmdArgument( app.updateCmd, "Input" ).SetPtrValue( &app.input );
    
    app.shell->Execute( &app.initCmd );

    CVDisplayLinkCreateWithActiveCGDisplays( &displayLink );
    CVDisplayLinkSetOutputCallback( displayLink, &DisplayLinkCallback, &app );
    CVDisplayLinkStart( displayLink );

    GameView *gameView = (GameView *) [self view];
    [gameView setGameViewController:self];
}

@end

#pragma mark -
#pragma mark GameView

@implementation GameView {
    NSTrackingArea *trackingArea;
    GameViewController *gameViewController;
}

//
// Mouse tracking setup
//

-(void)setGameViewController:(GameViewController*)controller {
    gameViewController = controller;
}

-(void)mouseEntered:(NSEvent *)e {
    [gameViewController mouseEntered:e];
}

-(void)mouseMoved:(NSEvent *)e {
    [gameViewController mouseMoved:e];
}

-(void)mouseExited:(NSEvent *)e {
    [gameViewController mouseExited:e];
}

-(void)updateTrackingAreas {
    [super updateTrackingAreas];
    if(trackingArea != nil) {
        [self removeTrackingArea:trackingArea];
    }

    int opts = (NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways);
    trackingArea = [ [NSTrackingArea alloc] initWithRect:[self bounds]
                                                 options:opts
                                                   owner:self
                                                userInfo:nil];

    [self addTrackingArea:trackingArea];
}

//
// Rendering setup
//

/** Returns a Metal-compatible layer. */
+ (Class)layerClass {
    return [CAMetalLayer class];
}

/** Indicates that the view wants to draw using the backing layer instead of using drawRect:.  */
- (BOOL)wantsUpdateLayer {
    return YES;
}

/** If the wantsLayer property is set to YES, this method will be invoked to return a layer instance. */
- (CALayer*)makeBackingLayer {
    CALayer* layer = [self.class.layerClass layer];
    CGSize viewScale = [self convertSizeToBacking:CGSizeMake( 1.0, 1.0 )];
    layer.contentsScale = MIN( viewScale.width, viewScale.height );
    return layer;
}

@end
