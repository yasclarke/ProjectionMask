#include "WindowLevel.h"
#import <AppKit/AppKit.h>

void raiseWindowAboveMenuBar (void* nativeViewHandle)
{
    auto* view = (__bridge NSView*) nativeViewHandle;
    auto* window = [view window];

    if (window == nil)
        return;

    [window setLevel: NSScreenSaverWindowLevel];
    [window setCollectionBehavior: NSWindowCollectionBehaviorCanJoinAllSpaces
                                 | NSWindowCollectionBehaviorStationary
                                 | NSWindowCollectionBehaviorFullScreenAuxiliary];
    [window setHasShadow: NO];
}
