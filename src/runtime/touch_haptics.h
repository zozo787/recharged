#pragma once

// A light tap of the device's Taptic Engine when a touch control is pressed
// (iOS; nothing happens on devices without one, such as iPads).
namespace mscharged::runtime
{
// From any thread: the tap is played on the main thread.
void TouchHapticTap();
} // namespace mscharged::runtime
