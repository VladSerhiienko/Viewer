
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

void* GetNSViewFromNSWindow( void* pNSWindow, bool bEnableMetalLayer ) {
    NSWindow* window = (__bridge NSWindow*) ( pNSWindow );
    NSView*   view   = [window contentView];

    assert( [view isKindOfClass:[NSView class]] );
    if ( bEnableMetalLayer && ![view.layer isKindOfClass:[CAMetalLayer class]] ) {
        [view setLayer:[CAMetalLayer layer]];
        [view setWantsLayer:YES];
    }

    return view;
}