//
//  GameViewController.h
//  Viewer iOS
//
//  Created by Vlad Serhiienko on 10/19/18.
//  Copyright © 2018 Vlad Serhiienko. All rights reserved.
//

#import <UIKit/UIKit.h>

// #import <Metal/Metal.h>
// #import <MetalKit/MetalKit.h>
// #import "Renderer.h"

#pragma mark -
#pragma mark GameViewController

// Our iOS view controller
@interface GameViewController : UIViewController
@end

#pragma mark -
#pragma mark GameView

/** The Metal-compatibile view for the demo Storyboard. */
@interface GameView : UIView
@end
