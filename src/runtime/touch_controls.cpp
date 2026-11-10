#include "runtime/touch_controls.h"

#include "platform/session_log.h"

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <string>

namespace mscharged::runtime {
namespace {
// Labels in the overlay font's ASCII. French: the player's language.
constexpr const char* kLabels[TouchControls::ControlCount] = {
    "A", "B", "C", "Z", "Tacle", "Objet", "+", "^", "v", "<", ">"};
// Each control's gamepad input (the default layout, bootstrap/gamepad_actions.h).
struct Input {
    int button = -1; // SDL_GamepadButton, or -1
    int axis = -1;   // SDL_GamepadAxis (a trigger), or -1
};
constexpr Input kInputs[TouchControls::ControlCount] = {
    {SDL_GAMEPAD_BUTTON_SOUTH, -1},          // A
    {SDL_GAMEPAD_BUTTON_EAST, -1},           // B
    {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, -1},  // C
    {-1, SDL_GAMEPAD_AXIS_LEFT_TRIGGER},     // Z
    {-1, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER},    // shake the Remote: hit an opponent
    {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, -1}, // shake the Nunchuk: switch items
    {SDL_GAMEPAD_BUTTON_START, -1},          // +
    {SDL_GAMEPAD_BUTTON_DPAD_UP, -1},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, -1},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, -1},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, -1},
};
// A finger presses a button a little outside its drawn circle.
constexpr float kHitSlop = 1.25f;
// The stick takes fingers that land in this left part of the screen.
constexpr float kStickZone = 0.45f;

// The name of a connected physical controller, or empty when there is none.
std::string PhysicalGamepad(SDL_JoystickID own)
{
    int count = 0;
    SDL_JoystickID* ids = SDL_GetGamepads(&count);
    if (!ids) return {};
    std::string found;
    for (int n = 0; n < count && found.empty(); ++n)
        if (ids[n] != own && !SDL_IsJoystickVirtual(ids[n])) {
            const char* name = SDL_GetGamepadNameForID(ids[n]);
            found = name && *name ? name : "unnamed controller";
        }
    SDL_free(ids);
    return found;
}

void Log(const char* format, ...) SDL_PRINTF_VARARG_FUNC(1);
void Log(const char* format, ...)
{
    char line[256];
    va_list arguments;
    va_start(arguments, format);
    std::vsnprintf(line, sizeof line, format, arguments);
    va_end(arguments);
    mscharged::platform::SessionLogLine(std::string("Touch controls: ") + line);
}
} // namespace

TouchControls::TouchControls()
{
    if (const char* hint = SDL_GetHint(SDL_HINT_TOUCH_MOUSE_EVENTS)) {
        hadHint_ = true;
        previousHint_ = hint;
    }
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
    watching_ = SDL_AddEventWatch(Watch, this);
    Log(watching_ ? "on" : "no event watch: %s", SDL_GetError());
    // Now, before the game counts its controllers (platform/hardware_owner.cpp):
    // with none connected at start it turns gamepads off for the whole run
    // and keyboard & mouse becomes player 1, so a virtual gamepad attached on
    // the first frame was never read.
    const std::string physical = PhysicalGamepad(0);
    if (physical.empty()) Attach();
    else Log("not attached at start: %s is connected", physical.c_str());
}

TouchControls::~TouchControls()
{
    if (watching_) SDL_RemoveEventWatch(Watch, this);
    Detach();
    if (hadHint_) SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, previousHint_.c_str());
    else SDL_ResetHint(SDL_HINT_TOUCH_MOUSE_EVENTS);
}

bool SDLCALL TouchControls::Watch(void* self, SDL_Event* event)
{
    auto& controls = *static_cast<TouchControls*>(self);
    switch (event->type) {
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_MOTION: {
        std::lock_guard lock(controls.mutex_);
        auto& finger = controls.fingers_[event->tfinger.fingerID];
        finger.x = event->tfinger.x;
        finger.y = event->tfinger.y;
        if (event->type == SDL_EVENT_FINGER_DOWN && controls.fingerDowns_++ == 0) {
            controls.firstX_ = finger.x;
            controls.firstY_ = finger.y;
        }
        break;
    }
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
        std::lock_guard lock(controls.mutex_);
        ++controls.resizes_;
        controls.windowW_ = event->window.data1;
        controls.windowH_ = event->window.data2;
        break;
    }
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED: {
        std::lock_guard lock(controls.mutex_);
        controls.fingers_.erase(event->tfinger.fingerID);
        break;
    }
    default: break;
    }
    return true;
}

void TouchControls::Attach()
{
    if (joystick_) return;
    SDL_VirtualJoystickDesc desc;
    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    for (const auto& input : kInputs) {
        if (input.button >= 0) desc.button_mask |= 1u << input.button;
        if (input.axis >= 0) desc.axis_mask |= 1u << input.axis;
    }
    desc.axis_mask |= (1u << SDL_GAMEPAD_AXIS_LEFTX) | (1u << SDL_GAMEPAD_AXIS_LEFTY);
    desc.name = "Recharged touch controls";
    id_ = SDL_AttachVirtualJoystick(&desc);
    if (!id_) {
        Log("no virtual gamepad: %s", SDL_GetError());
        return;
    }
    joystick_ = SDL_OpenJoystick(id_);
    if (!joystick_) {
        Log("virtual gamepad not opened: %s", SDL_GetError());
        SDL_DetachVirtualJoystick(id_);
        id_ = 0;
        return;
    }
    Log("virtual gamepad %u attached (%s gamepad)", unsigned(id_), SDL_IsGamepad(id_) ? "an SDL" : "NOT an SDL");
}

void TouchControls::Detach()
{
    if (joystick_) SDL_CloseJoystick(joystick_);
    if (id_) SDL_DetachVirtualJoystick(id_);
    joystick_ = nullptr;
    id_ = 0;
}

void TouchControls::Layout(float width, float height)
{
    width_ = width;
    height_ = height;
    // Sizes follow the short side, so the controls keep their size in the
    // hand on every screen; the right cluster sits under the right thumb.
    const float s = std::min(width, height);
    const auto at = [&](float fromRight, float fromBottom, float radius) {
        return Circle{width - fromRight * s, height - fromBottom * s, radius * s};
    };
    buttons_[A] = at(0.20f, 0.18f, 0.10f);
    buttons_[B] = at(0.07f, 0.33f, 0.065f);
    buttons_[Hit] = at(0.37f, 0.10f, 0.065f);
    buttons_[Z] = at(0.37f, 0.29f, 0.06f);
    buttons_[C] = at(0.22f, 0.41f, 0.055f);
    buttons_[Item] = at(0.07f, 0.52f, 0.05f);
    buttons_[Plus] = {width * 0.5f, 0.07f * s, 0.04f * s};
    const float padX = 0.15f * s, padY = 0.17f * s, step = 0.065f * s, padRadius = 0.035f * s;
    buttons_[Up] = {padX, padY - step, padRadius};
    buttons_[Down] = {padX, padY + step, padRadius};
    buttons_[Left] = {padX - step, padY, padRadius};
    buttons_[Right] = {padX + step, padY, padRadius};
    stickRadius_ = 0.12f * s;
    stickBase_ = {0.22f * s, height - 0.22f * s, stickRadius_};
    ++revision_;
}

int TouchControls::ButtonAt(float x, float y) const
{
    int best = None;
    float bestDistance = FLT_MAX;
    for (int n = 0; n < ControlCount; ++n) {
        const auto& b = buttons_[n];
        const float distance = std::hypot(x - b.x, y - b.y);
        if (distance <= b.r * kHitSlop && distance / b.r < bestDistance) {
            best = n;
            bestDistance = distance / b.r;
        }
    }
    return best;
}

void TouchControls::Service(float width, float height)
{
    if (width <= 0 || height <= 0) return;
    if (width != width_ || height != height_) Layout(width, height);
    const bool first = !loggedService_;
    if (first) {
        loggedService_ = true;
        Log("drawing on a %.0fx%.0f surface", width, height);
    }
    {
        // What reached the controls since the last frame, for the log.
        std::lock_guard lock(mutex_);
        if (fingerDowns_ && !loggedFingers_)
            Log("first touch at %.2f, %.2f of the window", firstX_, firstY_);
        loggedFingers_ = fingerDowns_;
        const auto now = std::chrono::steady_clock::now();
        if (resizes_ != loggedResizes_ && now - lastResizeLog_ >= std::chrono::seconds(2)) {
            Log("game window resized %u times so far, now %dx%d pixels", resizes_, windowW_, windowH_);
            loggedResizes_ = resizes_;
            lastResizeLog_ = now;
        }
    }

    // A physical controller takes over: no drawing and no second gamepad.
    const std::string physical = PhysicalGamepad(id_);
    const bool visible = physical.empty();
    if (visible != visible_ || first) {
        visible_ = visible;
        ++revision_;
        if (visible) Log("shown");
        else Log("hidden while %s is connected", physical.c_str());
    }
    if (!visible_) {
        Detach();
        std::lock_guard lock(mutex_);
        for (auto& [id, finger] : fingers_) finger.assigned = true, finger.control = None, finger.stick = false;
        pressed_ = {};
        stickActive_ = false;
        return;
    }
    Attach();

    std::array<bool, ControlCount> pressed{};
    bool stickActive = false;
    float stickX = stickX_, stickY = stickY_, knobX = knobX_, knobY = knobY_;
    {
        std::lock_guard lock(mutex_);
        for (const auto& [id, finger] : fingers_) stickActive |= finger.stick;
        for (auto& [id, finger] : fingers_) {
            const float x = finger.x * width, y = finger.y * height;
            if (!finger.assigned) {
                finger.assigned = true;
                finger.control = ButtonAt(x, y);
                // A finger landing on the left, away from the buttons, holds
                // the stick: its centre is where the finger landed.
                if (finger.control == None && !stickActive && x < width * kStickZone && y > height * 0.3f) {
                    finger.stick = true;
                    stickActive = true;
                    stickX = x;
                    stickY = y;
                }
            }
            else if (!finger.stick) finger.control = ButtonAt(x, y); // slides between buttons
            if (finger.stick) {
                float dx = x - stickX, dy = y - stickY;
                const float length = std::hypot(dx, dy);
                if (length > stickRadius_) dx *= stickRadius_ / length, dy *= stickRadius_ / length;
                knobX = stickX + dx;
                knobY = stickY + dy;
            }
            if (finger.control != None) pressed[finger.control] = true;
        }
    }
    if (!stickActive) {
        stickX = knobX = stickBase_.x;
        stickY = knobY = stickBase_.y;
    }

    for (int n = 0; n < ControlCount; ++n)
        if (pressed[n] && !loggedPress_[n]) {
            loggedPress_[n] = true;
            Log("first press of %s (virtual gamepad %s)", kLabels[n], joystick_ ? "attached" : "MISSING");
        }
    if (stickActive && !stickActive_) {
        static bool logged = false;
        if (!logged) Log("first use of the stick");
        logged = true;
    }
    if (pressed != pressed_ || stickActive != stickActive_ || knobX != knobX_ || knobY != knobY_
        || stickX != stickX_ || stickY != stickY_)
        ++revision_;
    pressed_ = pressed;
    stickActive_ = stickActive;
    stickX_ = stickX, stickY_ = stickY, knobX_ = knobX, knobY_ = knobY;

    if (!joystick_) return;
    for (int n = 0; n < ControlCount; ++n) {
        if (kInputs[n].button >= 0) SDL_SetJoystickVirtualButton(joystick_, kInputs[n].button, pressed_[n]);
        if (kInputs[n].axis >= 0)
            SDL_SetJoystickVirtualAxis(joystick_, kInputs[n].axis, pressed_[n] ? SDL_JOYSTICK_AXIS_MAX : 0);
    }
    const auto axis = [&](float offset) {
        const float value = std::clamp(offset / stickRadius_, -1.0f, 1.0f);
        return Sint16(std::lround(value * SDL_JOYSTICK_AXIS_MAX));
    };
    SDL_SetJoystickVirtualAxis(joystick_, SDL_GAMEPAD_AXIS_LEFTX, axis(knobX_ - stickX_));
    SDL_SetJoystickVirtualAxis(joystick_, SDL_GAMEPAD_AXIS_LEFTY, axis(knobY_ - stickY_));
}

void TouchControls::Draw(ImDrawList* draw, ImFont* font) const
{
    if (!visible_ || width_ <= 0) return;
    const ImU32 fill = IM_COL32(255, 255, 255, 40), fillPressed = IM_COL32(255, 255, 255, 120);
    const ImU32 line = IM_COL32(255, 255, 255, 140), text = IM_COL32(255, 255, 255, 220);
    const float thickness = std::max(1.5f, std::min(width_, height_) * 0.004f);
    for (int n = 0; n < ControlCount; ++n) {
        const auto& b = buttons_[n];
        draw->AddCircleFilled({b.x, b.y}, b.r, pressed_[n] ? fillPressed : fill);
        draw->AddCircle({b.x, b.y}, b.r, line, 0, thickness);
        const float size = n == Hit || n == Item ? b.r * 0.55f : b.r * 0.9f;
        const ImVec2 extent = font->CalcTextSizeA(size, FLT_MAX, 0, kLabels[n]);
        draw->AddText(font, size, {b.x - extent.x * 0.5f, b.y - extent.y * 0.5f}, text, kLabels[n]);
    }
    draw->AddCircleFilled({stickX_, stickY_}, stickRadius_, IM_COL32(255, 255, 255, stickActive_ ? 35 : 20));
    draw->AddCircle({stickX_, stickY_}, stickRadius_, line, 0, thickness);
    draw->AddCircleFilled({knobX_, knobY_}, stickRadius_ * 0.45f, stickActive_ ? fillPressed : IM_COL32(255, 255, 255, 70));
}

} // namespace mscharged::runtime
