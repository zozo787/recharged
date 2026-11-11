#include "launcher/ios_document_picker.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_properties.h>

#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

// The picker's delegate, kept alive while the picker is shown (the picker
// itself only holds it weakly).
@interface RechargedImportDelegate : NSObject <UIDocumentPickerDelegate>
@property(nonatomic, assign) SDL_DialogFileCallback callback;
@property(nonatomic, assign) void* userdata;
@property(nonatomic, copy) NSString* destination;
@end

static RechargedImportDelegate* g_shown = nil;

namespace
{
// Answers the caller once: `path` chosen, or cancelled (both null), or failed.
// Then lets the delegate go (the last use of it: the delegate methods return
// right after).
void Finish(RechargedImportDelegate* delegate, NSString* path, NSString* error)
{
    if (error)
    {
        SDL_SetError("%s", error.UTF8String);
        delegate.callback(delegate.userdata, nullptr, -1);
    }
    else
    {
        const char* list[2] = {path ? path.UTF8String : nullptr, nullptr};
        delegate.callback(delegate.userdata, list, -1);
    }
    g_shown = nil;
}

// Where the picked file goes: its own name in `folder`, unless a different
// file has that name ("name (2).rvz", ...). `same` tells whether the file
// already there is taken as the one picked (same name and size).
NSString* Target(NSString* folder, NSString* name, unsigned long long size, BOOL* same)
{
    NSFileManager* files = NSFileManager.defaultManager;
    NSString* base = name.stringByDeletingPathExtension, * extension = name.pathExtension;
    for (int n = 1;; ++n)
    {
        NSString* file = n == 1 ? name
            : extension.length ? [NSString stringWithFormat:@"%@ (%d).%@", base, n, extension]
                               : [NSString stringWithFormat:@"%@ (%d)", base, n];
        NSString* path = [folder stringByAppendingPathComponent:file];
        NSDictionary* existing = [files attributesOfItemAtPath:path error:nil];
        if (!existing)
        {
            *same = NO;
            return path;
        }
        if ([existing[NSFileType] isEqual:NSFileTypeRegular] && existing.fileSize == size)
        {
            *same = YES;
            return path;
        }
    }
}
} // namespace

@implementation RechargedImportDelegate
- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
    // Opened "as a copy": iOS has already copied the file into this app's
    // temporary folder (downloading it first from iCloud if needed), so it
    // is moved, not copied again.
    NSURL* picked = urls.firstObject;
    if (!picked)
    {
        Finish(self, nil, nil);
        return;
    }
    NSFileManager* files = NSFileManager.defaultManager;
    NSError* error = nil;
    if (![files createDirectoryAtPath:self.destination withIntermediateDirectories:YES attributes:nil error:&error])
    {
        Finish(self, nil, [@"Cannot make the game folder: " stringByAppendingString:error.localizedDescription]);
        return;
    }
    NSDictionary* attributes = [files attributesOfItemAtPath:picked.path error:&error];
    if (!attributes)
    {
        Finish(self, nil, [@"Cannot read the chosen file: " stringByAppendingString:error.localizedDescription]);
        return;
    }
    BOOL same = NO;
    NSString* target = Target(self.destination, picked.lastPathComponent, attributes.fileSize, &same);
    if (same)
    {
        [files removeItemAtURL:picked error:nil];
        Finish(self, target, nil);
        return;
    }
    if (![files moveItemAtPath:picked.path toPath:target error:&error]
        && ![files copyItemAtPath:picked.path toPath:target error:&error])
    {
        Finish(self, nil, [@"Cannot copy the chosen file: " stringByAppendingString:error.localizedDescription]);
        return;
    }
    [files removeItemAtURL:picked error:nil];
    Finish(self, target, nil);
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller
{
    Finish(self, nil, nil);
}
@end

namespace mscharged::launcher
{
void ShowImportFileDialog(SDL_DialogFileCallback callback, void* userdata, SDL_Window* window,
                          const std::string& destination)
{
    if (g_shown)
    {
        SDL_SetError("A file picker is already open");
        callback(userdata, nullptr, -1);
        return;
    }
    auto* uiwindow = (__bridge UIWindow*)SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                                                SDL_PROP_WINDOW_UIKIT_WINDOW_POINTER, nullptr);
    UIViewController* over = uiwindow.rootViewController;
    while (over.presentedViewController) over = over.presentedViewController;
    if (!over)
    {
        SDL_SetError("Cannot show the file picker: no window to show it over");
        callback(userdata, nullptr, -1);
        return;
    }
    // Any file: iOS does not know .rvz or .wbfs, and the launcher checks what
    // the file holds.
    UIDocumentPickerViewController* picker =
        [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeData] asCopy:YES];
    picker.allowsMultipleSelection = NO;
    picker.shouldShowFileExtensions = YES;
    RechargedImportDelegate* delegate = [RechargedImportDelegate new];
    delegate.callback = callback;
    delegate.userdata = userdata;
    delegate.destination = [NSString stringWithUTF8String:destination.c_str()];
    picker.delegate = delegate;
    g_shown = delegate;
    [over presentViewController:picker animated:YES completion:nil];
}
} // namespace mscharged::launcher
