//
//  MainViewController.m
//  ViewerCocoa.macOS
//
//  Created by Vlad Serhiienko on 10/15/18.
//  Copyright © 2018 Vlad Serhiienko. All rights reserved.
//

#import "MainViewController.h"
#import <QuartzCore/CAMetalLayer.h>

#include "apemode/platform/AppInput.h"
#include "apemode/platform/AppSurface.h"
#include "viewer/ViewerAppShellFactory.h"

struct App {
    apemode::platform::AppInput input;
    apemode::platform::AppSurface surface;
    std::unique_ptr<apemode::platform::IAppShell> shell;
};

@implementation MainViewController{
    CVDisplayLinkRef    _displayLink;
    struct App app;
}

-(void) dealloc {
    app.shell.reset();
    CVDisplayLinkRelease(_displayLink);
}

#pragma mark Display loop callback function

/** Rendering loop callback function for use with a CVDisplayLink. */
namespace {
CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink,
                                    const CVTimeStamp* now,
                                    const CVTimeStamp* outputTime,
                                    CVOptionFlags flagsIn,
                                    CVOptionFlags* flagsOut,
                                    void* target) {
    App * app = static_cast<App*>(target);
    app->shell->Update(&app->surface, &app->input);
    return kCVReturnSuccess;
}
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // Back the view with a layer created by the makeBackingLayer method.
    self.view.wantsLayer = YES;
    
    app.surface.iOS.pViewIOS = (__bridge void*) self.view;
    app.surface.macOS.pViewMacOS = (__bridge void*) self.view;
    app.surface.OverrideWidth = (int) self.view.frame.size.width;
    app.surface.OverrideHeight = (int) self.view.frame.size.height;
    
    app.shell = apemode::viewer::vk::CreateViewer(0, nullptr);
    app.shell->Initialize(&app.surface);
    
    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    CVDisplayLinkSetOutputCallback(_displayLink, &DisplayLinkCallback, &app);
    CVDisplayLinkStart(_displayLink);
}

@end

#pragma mark -
#pragma mark MainView

@implementation MainView

/** Indicates that the view wants to draw using the backing layer instead of using drawRect:.  */
-(BOOL) wantsUpdateLayer { return YES; }

/** Returns a Metal-compatible layer. */
+(Class) layerClass { return [CAMetalLayer class]; }

/** If the wantsLayer property is set to YES, this method will be invoked to return a layer instance. */
-(CALayer*) makeBackingLayer {
    CALayer* layer = [self.class.layerClass layer];
    CGSize viewScale = [self convertSizeToBacking: CGSizeMake(1.0, 1.0)];
    layer.contentsScale = MIN(viewScale.width, viewScale.height);
    return layer;
}

@end

