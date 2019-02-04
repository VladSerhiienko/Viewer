//
//  GameViewController.h
//  Viewer macOS
//
//  Created by Vlad Serhiienko on 10/19/18.
//  Copyright © 2018 Vlad Serhiienko. All rights reserved.
//

#import <Cocoa/Cocoa.h>

//#import <Metal/Metal.h>
//#import <MetalKit/MetalKit.h>
//#import "Renderer.h"

#pragma mark -
#pragma mark GameViewController

// Our macOS view controller.
@interface GameViewController : NSViewController
@end

#pragma mark -
#pragma mark GameView

/** The Metal-compatibile view for the demo Storyboard. */
@interface GameView : NSView
-(void)setGameViewController:(GameViewController*)controller;
@end
