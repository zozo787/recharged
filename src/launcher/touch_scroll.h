#pragma once

#include <SDL3/SDL_events.h>

#include <cstdint>

// Finger drags scroll the launcher's lists, as everywhere on a touch screen
// (iOS). SDL also turns each touch into mouse events, which ImGui reads as a
// press-and-drag: once the first finger has moved far enough, mostly
// vertically, over a window that can scroll, the press is cancelled (no click)
// and the finger's motion scrolls that window, with momentum after release.
// Horizontal drags (sliders) and taps keep their usual effect.
namespace mscharged::launcher
{
class TouchScroll
{
public:
    explicit TouchScroll(bool enabled) : enabled_(enabled) {}

    // During event polling, before ImGui sees the event. True when the event
    // is a touch's mouse event that belongs to a scroll and must be dropped.
    bool Filter(const SDL_Event& event);
    // Once per frame, right after ImGui::NewFrame().
    void Update();

private:
    void Stop();

    bool enabled_;
    bool down_ = false;      // the tracked finger is on the screen
    bool pending_ = false;   // it moved far enough: start scrolling at Update
    bool scrolling_ = false; // its motion scrolls `window_`
    SDL_FingerID finger_ = 0;
    float moved_x_ = 0.0f, moved_y_ = 0.0f; // since the finger went down, in window units
    float delta_y_ = 0.0f;                  // not yet applied
    float velocity_ = 0.0f;                 // window units per second, positive downwards
    std::uint64_t last_motion_ns_ = 0;
    std::uint64_t last_update_ns_ = 0;
    unsigned window_ = 0; // ImGuiID of the scrolled window
};
} // namespace mscharged::launcher
