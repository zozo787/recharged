#include "runtime/touch_haptics.h"

#include <algorithm>

#import <CoreHaptics/CoreHaptics.h>
#import <UIKit/UIKit.h>

namespace
{
void OnMainThread(void (^work)(void))
{
    if (NSThread.isMainThread) work();
    else dispatch_async(dispatch_get_main_queue(), work);
}

// UIKit objects live on the main thread; the generator is kept prepared so
// the next tap comes without delay.
void Tap()
{
    static UIImpactFeedbackGenerator* generator =
        [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleLight];
    [generator impactOccurred];
    [generator prepare];
}

// The rumble: one looping continuous event whose intensity follows the
// game's. The engine stops by itself when idle and after interruptions (the
// app in the background, a phone call): the player is made again then.
CHHapticEngine* g_engine = nil;
id<CHHapticAdvancedPatternPlayer> g_player = nil;
bool g_playing = false;

bool Prepare()
{
    if (!CHHapticEngine.capabilitiesForHardware.supportsHaptics) return false;
    NSError* error = nil;
    if (!g_engine)
    {
        g_engine = [[CHHapticEngine alloc] initAndReturnError:&error];
        if (!g_engine) return false;
        g_engine.playsHapticsOnly = YES;
        g_engine.autoShutdownEnabled = YES;
        // Called on the engine's own queue: the state is the main thread's.
        g_engine.resetHandler = ^{
            OnMainThread(^{
                g_player = nil;
                g_playing = false;
            });
        };
        g_engine.stoppedHandler = ^(CHHapticEngineStoppedReason) {
            OnMainThread(^{
                g_player = nil;
                g_playing = false;
            });
        };
    }
    if (![g_engine startAndReturnError:&error]) return false;
    if (!g_player)
    {
        CHHapticEvent* event = [[CHHapticEvent alloc]
            initWithEventType:CHHapticEventTypeHapticContinuous
                   parameters:@[ [[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticIntensity
                                                                                value:1.0f],
                                 [[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticSharpness
                                                                                value:0.3f] ]
                 relativeTime:0
                     duration:30];
        CHHapticPattern* pattern = [[CHHapticPattern alloc] initWithEvents:@[ event ] parameters:@[] error:&error];
        if (!pattern) return false;
        g_player = [g_engine createAdvancedPlayerWithPattern:pattern error:&error];
        if (!g_player) return false;
        g_player.loopEnabled = YES;
    }
    return true;
}

void Rumble(float intensity)
{
    if (intensity <= 0)
    {
        if (g_player && g_playing) [g_player stopAtTime:CHHapticTimeImmediate error:nil];
        g_playing = false;
        return;
    }
    if (!Prepare()) return;
    CHHapticDynamicParameter* level =
        [[CHHapticDynamicParameter alloc] initWithParameterID:CHHapticDynamicParameterIDHapticIntensityControl
                                                        value:intensity
                                                 relativeTime:0];
    [g_player sendParameters:@[ level ] atTime:CHHapticTimeImmediate error:nil];
    if (!g_playing) g_playing = [g_player startAtTime:CHHapticTimeImmediate error:nil];
}
} // namespace

namespace mscharged::runtime
{
void TouchHapticTap()
{
    OnMainThread(^{ Tap(); });
}

void TouchRumble(float intensity)
{
    const float level = std::clamp(intensity, 0.0f, 1.0f);
    OnMainThread(^{ Rumble(level); });
}
} // namespace mscharged::runtime
