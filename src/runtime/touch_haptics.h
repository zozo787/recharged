#pragma once

// The device's Taptic Engine for the on-screen touch controls (iOS; nothing
// happens on devices without one, such as iPads).
namespace mscharged::runtime
{
// A light tap, when a touch control is pressed. From any thread: it is
// played on the main thread.
void TouchHapticTap();

// The game's rumble (a Wii Remote's motor) while the touch controls play:
// a continuous vibration at `intensity` (0 to 1), or none at 0. From any
// thread: it is applied on the main thread.
void TouchRumble(float intensity);
} // namespace mscharged::runtime
