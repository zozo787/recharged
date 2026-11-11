#include "launcher/ios_document_picker.h"

#include "platform/session_log.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_properties.h>

#include <string>

#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

// The picker's delegate, kept alive while the picker is shown (the picker
// itself only holds it weakly) and until the copy is done.
@interface RechargedImportDelegate : NSObject <UIDocumentPickerDelegate, UIAdaptivePresentationControllerDelegate>
@property(nonatomic, assign) SDL_DialogFileCallback callback;
@property(nonatomic, assign) void* userdata;
@property(nonatomic, copy) NSString* destination;
@property(nonatomic, assign) BOOL answered;
@end

static RechargedImportDelegate* g_shown = nil;

namespace
{
void Log(NSString* text)
{
    mscharged::platform::SessionLogLine(std::string("File picker: ") + text.UTF8String);
}

// Answers the caller once, on the main thread: `path` chosen, or cancelled
// (both null), or failed. Then lets the delegate go.
void Finish(RechargedImportDelegate* delegate, NSString* path, NSString* error)
{
    if (delegate.answered) return;
    delegate.answered = YES;
    if (error)
    {
        Log(error);
        SDL_SetError("%s", error.UTF8String);
        delegate.callback(delegate.userdata, nullptr, -1);
    }
    else
    {
        Log(path ? [@"copied to " stringByAppendingString:path] : @"cancelled");
        const char* list[2] = {path ? path.UTF8String : nullptr, nullptr};
        delegate.callback(delegate.userdata, list, -1);
    }
    if (g_shown == delegate) g_shown = nil;
}

// Where the picked file goes: its own name in `folder`, unless a different
// file has that name ("name (2).rvz", ...). `same` tells whether the file
// already there is taken as the one picked (same name and size).
NSString* Target(NSString* folder, NSString* name, unsigned long long size, BOOL* same)
{
    NSFileManager* files = NSFileManager.defaultManager;
    NSString* base = name.stringByDeletingPathExtension;
    NSString* extension = name.pathExtension;
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

// Copies `picked` (a file the picker opened for this app, anywhere: iCloud
// Drive, another app's folder, a USB drive) into `folder`. Off the main
// thread: an image is 1 to 2 GB. Returns the copy's path, or nil and `error`.
NSString* Import(NSURL* picked, NSString* folder, NSString** error)
{
    NSFileManager* files = NSFileManager.defaultManager;
    NSError* failure = nil;
    if (![files createDirectoryAtPath:folder withIntermediateDirectories:YES attributes:nil error:&failure])
    {
        *error = [@"Cannot make the game folder: " stringByAppendingString:failure.localizedDescription];
        return nil;
    }
    // The picker grants access to the file only between these two calls.
    const BOOL scoped = [picked startAccessingSecurityScopedResource];
    __block NSString* result = nil;
    __block NSString* problem = nil;
    // The coordinator downloads a file iCloud keeps only online first.
    NSFileCoordinator* coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];
    [coordinator coordinateReadingItemAtURL:picked
                                    options:NSFileCoordinatorReadingWithoutChanges
                                      error:&failure
                                 byAccessor:^(NSURL* readable) {
        NSError* problemError = nil;
        NSDictionary* attributes = [files attributesOfItemAtPath:readable.path error:&problemError];
        if (!attributes)
        {
            problem = [@"Cannot read the chosen file: " stringByAppendingString:problemError.localizedDescription];
            return;
        }
        Log([NSString stringWithFormat:@"copying %@ (%llu bytes)", readable.lastPathComponent, attributes.fileSize]);
        BOOL same = NO;
        NSString* target = Target(folder, picked.lastPathComponent, attributes.fileSize, &same);
        if (same)
        {
            Log(@"the same file is already in the game folder");
            result = target;
            return;
        }
        if (![files copyItemAtPath:readable.path toPath:target error:&problemError])
        {
            [files removeItemAtPath:target error:nil]; // a partial copy
            problem = [@"Cannot copy the chosen file: " stringByAppendingString:problemError.localizedDescription];
            return;
        }
        result = target;
    }];
    if (scoped) [picked stopAccessingSecurityScopedResource];
    if (!result && !problem && failure)
        problem = [@"Cannot open the chosen file: " stringByAppendingString:failure.localizedDescription];
    *error = problem;
    return result;
}
} // namespace

@implementation RechargedImportDelegate
- (void)documentPicker:(UIDocumentPickerViewController*)controller didPickDocumentsAtURLs:(NSArray<NSURL*>*)urls
{
    NSURL* picked = urls.firstObject;
    Log(picked ? [@"picked " stringByAppendingString:picked.path] : @"picked nothing");
    if (!picked)
    {
        Finish(self, nil, nil);
        return;
    }
    RechargedImportDelegate* delegate = self; // kept by the blocks until the copy is done
    NSString* folder = self.destination;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        NSString* error = nil;
        NSString* path = Import(picked, folder, &error);
        dispatch_async(dispatch_get_main_queue(), ^{
            Finish(delegate, path, error);
        });
    });
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController*)controller
{
    Finish(self, nil, nil);
}

// Closed some other way (swiped down, or by iOS): the same as Cancel, unless
// a file was picked.
- (void)presentationControllerDidDismiss:(UIPresentationController*)presentationController
{
    Log(@"closed");
    if (g_shown == self && !self.answered) Finish(self, nil, nil);
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
        Log(@"no view controller to show it over");
        SDL_SetError("Cannot show the file picker: no window to show it over");
        callback(userdata, nullptr, -1);
        return;
    }
    // Any file: iOS does not know .rvz or .wbfs, and the launcher checks what
    // the file holds. Opened in place (not "as a copy"): this app copies it.
    UIDocumentPickerViewController* picker =
        [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:@[UTTypeData] asCopy:NO];
    picker.allowsMultipleSelection = NO;
    picker.shouldShowFileExtensions = YES;
    RechargedImportDelegate* delegate = [RechargedImportDelegate new];
    delegate.callback = callback;
    delegate.userdata = userdata;
    delegate.destination = [NSString stringWithUTF8String:destination.c_str()];
    picker.delegate = delegate;
    g_shown = delegate;
    Log([NSString stringWithFormat:@"showing it over %@ (window %@)", NSStringFromClass(over.class),
                                   uiwindow ? @"found" : @"missing"]);
    [over presentViewController:picker animated:YES completion:^{
        Log(@"shown");
    }];
    picker.presentationController.delegate = delegate;
}
} // namespace mscharged::launcher
