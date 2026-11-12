#include "runtime/touch_haptics.h"

#import <UIKit/UIKit.h>

namespace
{
// UIKit objects live on the main thread; the generator is kept prepared so
// the next tap comes without delay.
void Tap()
{
    static UIImpactFeedbackGenerator* generator =
        [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];
    [generator impactOccurred];
    [generator prepare];
}
} // namespace

namespace mscharged::runtime
{
void TouchHapticTap()
{
    if (NSThread.isMainThread) Tap();
    else dispatch_async(dispatch_get_main_queue(), ^{ Tap(); });
}
} // namespace mscharged::runtime
