#include "launcher/touch_scroll.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_touch.h>

#include <cfloat>
#include <cmath>

namespace mscharged::launcher
{
namespace
{
// Distance before a drag scrolls, in window units (points on iOS).
constexpr float kStartDistance = 10.0f;
// Momentum: lost per second (exponential), and the speed where it stops.
constexpr float kFriction = 4.0f;
constexpr float kStopSpeed = 20.0f;

bool TouchMouse(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_EVENT_MOUSE_MOTION: return event.motion.which == SDL_TOUCH_MOUSEID;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: return event.button.which == SDL_TOUCH_MOUSEID;
    case SDL_EVENT_MOUSE_WHEEL: return event.wheel.which == SDL_TOUCH_MOUSEID;
    default: return false;
    }
}

// The window under the mouse that can scroll vertically: the hovered one or
// one of its parents, as the mouse wheel finds it. A window that refuses the
// wheel refuses the finger too: the launcher's root window holds the sidebar
// and the pages, and scrolling it moved everything.
ImGuiWindow* ScrollableUnderMouse()
{
    ImGuiContext& g = *ImGui::GetCurrentContext();
    for (ImGuiWindow* window = g.HoveredWindow; window; window = window->ParentWindow)
    {
        if (window->Flags & ImGuiWindowFlags_NoScrollWithMouse)
        {
            if (!(window->Flags & ImGuiWindowFlags_ChildWindow)) return nullptr;
            continue; // a child that refuses the wheel passes it to its parent
        }
        if (window->ScrollMax.y > 0.0f) return window;
    }
    return nullptr;
}
} // namespace

void TouchScroll::Stop()
{
    scrolling_ = pending_ = false;
    velocity_ = delta_y_ = 0.0f;
    window_ = 0;
}

bool TouchScroll::Filter(const SDL_Event& event)
{
    if (!enabled_) return false;
    const ImGuiIO& io = ImGui::GetIO();
    switch (event.type)
    {
    case SDL_EVENT_FINGER_DOWN:
        if (down_) break; // a second finger: only the first one scrolls
        Stop();           // a touch stops the momentum, like on iOS
        down_ = true;
        finger_ = event.tfinger.fingerID;
        moved_x_ = moved_y_ = 0.0f;
        last_motion_ns_ = event.tfinger.timestamp;
        break;
    case SDL_EVENT_FINGER_MOTION:
    {
        if (!down_ || event.tfinger.fingerID != finger_) break;
        // Finger coordinates are normalized to the window.
        const float dx = event.tfinger.dx * io.DisplaySize.x;
        const float dy = event.tfinger.dy * io.DisplaySize.y;
        moved_x_ += dx;
        moved_y_ += dy;
        if (scrolling_)
        {
            delta_y_ += dy;
            const float seconds = float(event.tfinger.timestamp - last_motion_ns_) * 1e-9f;
            if (seconds > 0.0f)
                velocity_ = 0.7f * (dy / seconds) + 0.3f * velocity_; // smoothed
        }
        else if (!pending_ && std::fabs(moved_y_) >= kStartDistance
                 && std::fabs(moved_y_) > 1.2f * std::fabs(moved_x_))
            pending_ = true;
        last_motion_ns_ = event.tfinger.timestamp;
        break;
    }
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
        if (!down_ || event.tfinger.fingerID != finger_) break;
        down_ = false;
        pending_ = false;
        // A finger that stopped before lifting leaves no momentum.
        if (event.tfinger.timestamp - last_motion_ns_ > 100'000'000ull) velocity_ = 0.0f;
        break;
    default: break;
    }
    // While a finger scrolls, its mouse events (press, motion, release) would
    // drag or click whatever is under it. A tap during the momentum only stops
    // it, as on iOS: SDL sends a touch's mouse press before its finger event.
    return scrolling_ && TouchMouse(event);
}

void TouchScroll::Update()
{
    if (!enabled_) return;
    const std::uint64_t now = SDL_GetTicksNS();
    const float seconds = last_update_ns_ ? float(now - last_update_ns_) * 1e-9f : 0.0f;
    last_update_ns_ = now;
    ImGuiIO& io = ImGui::GetIO();
    if (pending_ && !scrolling_)
    {
        pending_ = false;
        // NewFrame has placed the mouse at the finger: the window under it.
        if (ImGuiWindow* window = ScrollableUnderMouse())
        {
            scrolling_ = true;
            window_ = window->ID;
            delta_y_ = moved_y_;
            velocity_ = 0.0f;
            // Release the press away from everything: no click, no drag.
            io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
            io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
        }
    }
    if (!scrolling_) return;
    ImGuiWindow* window = window_ ? ImGui::FindWindowByID(window_) : nullptr;
    if (!window)
    {
        Stop();
        return;
    }
    if (!down_)
    {
        // Momentum after release.
        delta_y_ += velocity_ * seconds;
        velocity_ *= std::exp(-kFriction * seconds);
    }
    if (delta_y_ != 0.0f)
    {
        // Content follows the finger: moving it down shows what is above.
        const float target = window->Scroll.y - delta_y_;
        const bool at_edge = target <= 0.0f || target >= window->ScrollMax.y;
        ImGui::SetScrollY(window, ImClamp(target, 0.0f, window->ScrollMax.y));
        delta_y_ = 0.0f;
        if (at_edge && !down_) velocity_ = 0.0f;
    }
    if (!down_ && std::fabs(velocity_) < kStopSpeed) Stop();
}
} // namespace mscharged::launcher
