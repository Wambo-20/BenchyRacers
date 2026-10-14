#include "BenchyFilePicker.h"
#if PLATFORM_MAC
#import <AppKit/AppKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import <dispatch/dispatch.h>

bool BenchyChoosePNG(FString& Path){
    FString Selected;
    auto* Result=&Selected;
    void (^Choose)(void)=^{
        @autoreleasepool {
            NSOpenPanel* Panel=[NSOpenPanel openPanel];
            [Panel setTitle:@"Dein PNG-Logo wählen"];
            [Panel setCanChooseFiles:YES];[Panel setCanChooseDirectories:NO];
            [Panel setAllowsMultipleSelection:NO];[Panel setAllowedContentTypes:@[UTTypePNG]];
            if([Panel runModal]==NSModalResponseOK){
                const char* Filename=[[[Panel URL] path] UTF8String];
                if(Filename)*Result=UTF8_TO_TCHAR(Filename);
            }
        }
    };
    if([NSThread isMainThread])Choose();else dispatch_sync(dispatch_get_main_queue(),Choose);
    Path=MoveTemp(Selected);return !Path.IsEmpty();
}
#endif
