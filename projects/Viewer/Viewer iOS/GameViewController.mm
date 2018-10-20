//
//  GameViewController.m
//  Viewer iOS
//
//  Created by Vlad Serhiienko on 10/19/18.
//  Copyright © 2018 Vlad Serhiienko. All rights reserved.
//

#import "GameViewController.h"
//#import "Renderer.h"

@implementation GameViewController
{
    MTKView *_view;

//    Renderer *_renderer;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    _view = (MTKView *)self.view;

    _view.device = MTLCreateSystemDefaultDevice();
    _view.backgroundColor = UIColor.blackColor;

    if(!_view.device)
    {
        NSLog(@"Metal is not supported on this device");
        self.view = [[UIView alloc] initWithFrame:self.view.frame];
        return;
    }

//    _renderer = [[Renderer alloc] initWithMetalKitView:_view];
//
//    [_renderer mtkView:_view drawableSizeWillChange:_view.bounds.size];
//
//    _view.delegate = _renderer;
}

@end
