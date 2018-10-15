//
//  MainViewController.m
//  ViewerCocoa.macOS
//
//  Created by Vlad Serhiienko on 10/15/18.
//  Copyright © 2018 Vlad Serhiienko. All rights reserved.
//

#import "MainViewController.h"
#import <QuartzCore/CAMetalLayer.h>
#include "viewer/ViewerAppShellFactory.h"
//#include <viewer/ViewerAppShellFactory.h>

@implementation MainViewController{
    CVDisplayLinkRef    _displayLink;
    std::unique_ptr<apemode::platform::IAppShell> pAppShell;
    // struct demo demo;
}

-(void) dealloc {
    // demo_cleanup(&demo);
    CVDisplayLinkRelease(_displayLink);
    // [super dealloc];
}

#pragma mark Display loop callback function

/** Rendering loop callback function for use with a CVDisplayLink. */
static CVReturn DisplayLinkCallback(CVDisplayLinkRef displayLink,
                                    const CVTimeStamp* now,
                                    const CVTimeStamp* outputTime,
                                    CVOptionFlags flagsIn,
                                    CVOptionFlags* flagsOut,
                                    void* target) {
    // demo_draw((struct demo*)target);
    return kCVReturnSuccess;
}

- (void)viewDidLoad {
    [super viewDidLoad];

    // Back the view with a layer created by the makeBackingLayer method.
    self.view.wantsLayer = YES;
    // const char* arg = "cube";
    // demo_main(&demo, self.view, 1, &arg);

    CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
    CVDisplayLinkSetOutputCallback(_displayLink, &DisplayLinkCallback, /*&demo*/ nil);
    CVDisplayLinkStart(_displayLink);
}

//- (void)setRepresentedObject:(id)representedObject {
//    [super setRepresentedObject:representedObject];
//
//    // Update the view, if already loaded.
//}

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

