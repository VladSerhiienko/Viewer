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
#include "apemode/platform/DefaultAppShellCommand.h"
#include "viewer/ViewerAppShellFactory.h"
#include "apemode/platform/shared/AssetManager.h"

#include <vector>

struct App {
    std::unique_ptr< apemode::platform::IAppShell > shell;
    apemode::platform::AppSurface                   surface;
    apemode::platform::AppShellCommand              assetManagerCmd;
    apemode::platform::AppInput                     input;
    apemode::platform::AppShellCommand              initCmd;
    apemode::platform::AppShellCommand              updateCmd;
    apemode::platform::shared::AssetManager         assetManager;
};

namespace {
apemode::platform::AppShellCommandArgumentValue&
getShellCmdArgument(apemode::platform::AppShellCommand& cmd, const char* pszName ) {
    auto& arg = cmd.Args[pszName];
    arg.Name = pszName;
    return arg.Value;
}
}

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
    app.shell->Execute( &app.updateCmd );
    // app.shell->Update( &app.surface, &app.input );
}

- (void)initializeEmbeddedAssets:(NSString*) embeddedDir
                                :(NSString*) extension {
    NSBundle *mainBundle = [NSBundle mainBundle];
    
    NSArray *fontList = [mainBundle pathsForResourcesOfType:extension inDirectory:embeddedDir];
    for (NSUInteger i = 0; i < [fontList count]; ++i) {
        NSString *assetId = [fontList objectAtIndex:i];
        NSString *assetFileName = [assetId lastPathComponent];
        NSString *assetName = [embeddedDir stringByAppendingPathComponent:assetFileName];
        
        app.assetManager.AddAsset([assetName cStringUsingEncoding:NSASCIIStringEncoding],
                                  [assetId cStringUsingEncoding:NSASCIIStringEncoding]);
        
        NSLog(@"initializeEmbeddedAssets: + embedded file: %@ (%@)", assetName, assetId);
    }
}

- (void)initializeAssets {
    NSString *sharedFilterDir = @"shared";
    
    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    for (NSUInteger i = 0; i < [paths count]; ++i) {
        NSString *documentsDir = [paths objectAtIndex:i];
        // NSLog(@"viewDidLoad: %@", documentsDir);
        
        NSFileManager* defaultFileManager = [NSFileManager defaultManager];
        NSArray *sharedFileList = [defaultFileManager contentsOfDirectoryAtPath:documentsDir error:nil];
        for (NSUInteger j = 0; j < [sharedFileList count]; ++j) {
            NSString *relativeFilePath = [sharedFileList objectAtIndex:j];
            NSString *assetName =  [sharedFilterDir stringByAppendingPathComponent:relativeFilePath];
            NSString *assetId = [documentsDir stringByAppendingPathComponent:relativeFilePath];
            
            app.assetManager.AddAsset([assetName cStringUsingEncoding:NSASCIIStringEncoding],
                                      [assetId cStringUsingEncoding:NSASCIIStringEncoding]);
            
            NSLog(@"initializeAssets: + shared file: %@ (%@)", assetName, assetId);
        }
    }
    
    [self initializeEmbeddedAssets:@"fonts" :@"ttf"];
    [self initializeEmbeddedAssets:@"shaders/spv" :@"spv"];
    [self initializeEmbeddedAssets:@"shaders/msl" :@"metal"];
    [self initializeEmbeddedAssets:@"images/Environment" :@"dds"];
}

- (void)viewDidLoad {
    [super viewDidLoad];
    [self initializeAssets];

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
    
    app.assetManagerCmd.Type = "SetAssetManager";
    getShellCmdArgument( app.assetManagerCmd, "AssetManager" ).SetPtrValue( &app.assetManager );
    app.shell->Execute( &app.assetManagerCmd );
    
    app.initCmd.Type = "Initialize";
    getShellCmdArgument( app.initCmd, "Surface" ).SetPtrValue( &app.surface );
    
    app.updateCmd.Type = "Update";
    getShellCmdArgument( app.updateCmd, "Surface" ).SetPtrValue( &app.surface );
    getShellCmdArgument( app.updateCmd, "Input" ).SetPtrValue( &app.input );
    
    app.shell->Execute( &app.initCmd );

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
