
#import <Cocoa/Cocoa.h>

void* GetNSViewFromNSWindow( void* pNSWindow ) {
    NSWindow* nsWindow = (__bridge NSWindow*) ( pNSWindow );
    return [nsWindow contentView];
}