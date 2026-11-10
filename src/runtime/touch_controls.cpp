#include "runtime/touch_controls.h"

#include "platform/session_log.h"

#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>

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
    "A", "B", "C", "Z", "Tacle", "Objet", "+", "-", "1", "2", "\xC3\x97", "^", "v", "<", ">"};
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
    {SDL_GAMEPAD_BUTTON_BACK, -1},           // -
    {SDL_GAMEPAD_BUTTON_WEST, -1},           // 1
    {SDL_GAMEPAD_BUTTON_NORTH, -1},          // 2
    {-1, -1},                                // the cross: no gamepad input
    {SDL_GAMEPAD_BUTTON_DPAD_UP, -1},
    {SDL_GAMEPAD_BUTTON_DPAD_DOWN, -1},
    {SDL_GAMEPAD_BUTTON_DPAD_LEFT, -1},
    {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, -1},
};
// A finger presses a button a little outside its drawn circle.
constexpr float kHitSlop = 1.25f;
// The stick takes fingers that land in the lower left corner; elsewhere, away
// from the buttons, a finger points the Wii Remote.
constexpr float kStickZoneX = 0.40f, kStickZoneY = 0.45f;
// The pointing finger's mouse (not SDL_TOUCH_MOUSEID, which the game ignores).
constexpr SDL_MouseID kPointerMouse = 0x54434850; // "TCHP"

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
    // Our own reference: the controller count at start opens and closes the
    // gamepad subsystem, which would otherwise take the virtual gamepad along.
    gamepads_ = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    watching_ = SDL_AddEventWatch(Watch, this);
    Log(watching_ ? "on" : "no event watch: %s", SDL_GetError());
    // Now, before the input owner sets up the players: the touch player's
    // channel goes to this virtual gamepad's id (platform/hardware_owner.cpp).
    Attach();
}

TouchControls::~TouchControls()
{
    if (watching_) SDL_RemoveEventWatch(Watch, this);
    Detach();
    if (gamepads_) SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
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
        controls.window_ = event->tfinger.windowID;
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
        const auto finger = controls.fingers_.find(event->tfinger.fingerID);
        if (finger != controls.fingers_.end()) {
            // Never seen by Service: a tap between two frames.
            if (!finger->second.assigned && event->type == SDL_EVENT_FINGER_UP)
                controls.taps_.emplace_back(finger->second.x, finger->second.y);
            controls.fingers_.erase(finger);
        }
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
    // Every button and axis: SDL numbers a virtual gamepad's inputs in the
    // order of the mask's bits, so with all of them the joystick's button and
    // axis numbers are the SDL_GamepadButton/SDL_GamepadAxis values set below.
    // With only the used ones, everything after B and the stick was shifted
    // (C, Z, the shakes, + and the D-pad did nothing).
    desc.button_mask = (1u << SDL_GAMEPAD_BUTTON_COUNT) - 1;
    desc.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1;
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
    Log("virtual gamepad %u attached", unsigned(id_));
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
    // Away from the top edge, where iOS delays touches for its own gestures.
    // - and + as on the Remote. No HOME: the game is built without its HOME
    // Menu (MSCHARGED_DIAGNOSTIC_FRONTEND_HOME is off), so HOME does nothing.
    buttons_[Minus] = {width * 0.5f - 0.07f * s, 0.15f * s, 0.045f * s};
    buttons_[Plus] = {width * 0.5f + 0.07f * s, 0.15f * s, 0.045f * s};
    // 1 and 2 at the top right, as on a Remote held sideways.
    buttons_[One] = {width - 0.21f * s, 0.15f * s, 0.045f * s};
    buttons_[Two] = {width - 0.09f * s, 0.15f * s, 0.045f * s};
    // Small, out of the way: it only opens a confirmation.
    buttons_[Quit] = {0.06f * s, 0.07f * s, 0.032f * s};
    const float dialogW = 0.70f * s, dialogH = 0.34f * s;
    dialog_ = {width * 0.5f - dialogW * 0.5f, height * 0.5f - dialogH * 0.5f,
               width * 0.5f + dialogW * 0.5f, height * 0.5f + dialogH * 0.5f};
    const float buttonW = 0.27f * s, buttonH = 0.10f * s, gap = 0.04f * s, bottom = dialog_.y1 - 0.05f * s;
    dialogCancel_ = {width * 0.5f - gap * 0.5f - buttonW, bottom - buttonH, width * 0.5f - gap * 0.5f, bottom};
    dialogQuit_ = {width * 0.5f + gap * 0.5f, bottom - buttonH, width * 0.5f + gap * 0.5f + buttonW, bottom};
    const float padX = 0.16f * s, padY = 0.24f * s, step = 0.07f * s, padRadius = 0.037f * s;
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
        // Window size changes (folding the foldable iPhone), for the log.
        std::lock_guard lock(mutex_);
        const auto now = std::chrono::steady_clock::now();
        if (resizes_ != loggedResizes_ && now - lastResizeLog_ >= std::chrono::seconds(2)) {
            Log("game window resized %u times so far, now %dx%d pixels", resizes_, windowW_, windowH_);
            loggedResizes_ = resizes_;
            lastResizeLog_ = now;
        }
    }

    // A controller that connects plays instead (platform/desktop_wpad.cpp):
    // the controls hide, release everything and come back when it is gone.
    // Their virtual gamepad stays attached, so its id keeps the touch slot.
    const std::string physical = PhysicalGamepad(id_);
    const bool visible = physical.empty();
    if (visible != visible_ || first) {
        visible_ = visible;
        ++revision_;
        if (visible) Log("shown");
        else Log("hidden: %s plays instead", physical.c_str());
    }
    if (!visible_) {
        {
            std::lock_guard lock(mutex_);
            for (auto& [id, finger] : fingers_) finger.assigned = true, finger.control = None, finger.stick = false;
        }
        pressed_ = {};
        stickActive_ = false;
        knobX_ = stickX_ = stickBase_.x;
        knobY_ = stickY_ = stickBase_.y;
        if (joystick_) {
            for (int n = 0; n < ControlCount; ++n) {
                if (kInputs[n].button >= 0) SDL_SetJoystickVirtualButton(joystick_, kInputs[n].button, false);
                if (kInputs[n].axis >= 0) SDL_SetJoystickVirtualAxis(joystick_, kInputs[n].axis, SDL_JOYSTICK_AXIS_MIN);
            }
            SDL_SetJoystickVirtualAxis(joystick_, SDL_GAMEPAD_AXIS_LEFTX, 0);
            SDL_SetJoystickVirtualAxis(joystick_, SDL_GAMEPAD_AXIS_LEFTY, 0);
        }
        return;
    }

    std::array<bool, ControlCount> pressed{};
    bool stickActive = false;
    float stickX = stickX_, stickY = stickY_, knobX = knobX_, knobY = knobY_;
    bool pointing = false;
    float pointX = 0, pointY = 0; // window-normalized
    SDL_WindowID window = 0;
    {
        std::lock_guard lock(mutex_);
        window = window_;
        if (confirming_) {
            // Only the window's two buttons answer, to a new touch or a tap;
            // fingers already down stay idle until they lift.
            int choice = 0; // 1: Quitter, 2: Annuler
            const auto pick = [&](float nx, float ny) {
                const float x = nx * width, y = ny * height;
                if (dialogQuit_.Contains(x, y)) choice = 1;
                else if (dialogCancel_.Contains(x, y)) choice = 2;
            };
            for (const auto& [tapX, tapY] : taps_) pick(tapX, tapY);
            taps_.clear();
            for (auto& [id, finger] : fingers_)
                if (!finger.assigned) {
                    finger.assigned = true;
                    finger.control = None;
                    pick(finger.x, finger.y);
                }
            for (auto& [id, finger] : fingers_) finger.control = None, finger.stick = false, finger.pointer = false;
            if (choice) {
                confirming_ = false;
                ++revision_;
                if (choice == 1 && !quitSent_ && window) {
                    quitSent_ = true;
                    SDL_Event close{};
                    close.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
                    close.window.timestamp = SDL_GetTicksNS();
                    close.window.windowID = window;
                    SDL_PushEvent(&close);
                    Log("Quitter: asking the game window to close");
                }
            }
        }
        else {
        // Taps between frames press their button for a few frames.
        for (const auto& [tapX, tapY] : taps_)
            if (const int control = ButtonAt(tapX * width, tapY * height); control != None) held_[control] = 3;
        taps_.clear();
        bool pointerTaken = false;
        for (const auto& [id, finger] : fingers_) stickActive |= finger.stick, pointerTaken |= finger.pointer;
        for (auto& [id, finger] : fingers_) {
            const float x = finger.x * width, y = finger.y * height;
            if (!finger.assigned) {
                finger.assigned = true;
                finger.control = ButtonAt(x, y);
                // A finger landing on the left, away from the buttons, holds
                // the stick: its centre is where the finger landed.
                if (finger.control == None && !stickActive && x < width * kStickZoneX && y > height * kStickZoneY) {
                    finger.stick = true;
                    stickActive = true;
                    stickX = x;
                    stickY = y;
                }
                else if (finger.control == None && !pointerTaken) {
                    finger.pointer = true;
                    pointerTaken = true;
                }
            }
            if (finger.pointer) {
                pointing = true;
                pointX = finger.x;
                pointY = finger.y;
                continue; // it presses nothing, even over a button
            }
            if (!finger.stick) finger.control = ButtonAt(x, y); // slides between buttons
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
    }
    if (!stickActive) {
        stickX = knobX = stickBase_.x;
        stickY = knobY = stickBase_.y;
    }
    for (int n = 0; n < ControlCount; ++n)
        if (held_[n] > 0) {
            pressed[n] = true;
            --held_[n];
        }
    // The cross opens the confirmation window (the window's Quitter sends the
    // game window's close request, which the game takes as the Wii's power
    // button, platform/hardware_owner.cpp). Nothing reaches the game meanwhile.
    if (pressed[Quit] && !quitWasPressed_ && !confirming_) {
        confirming_ = true;
        ++revision_;
    }
    quitWasPressed_ = pressed[Quit];
    pressed[Quit] = false;
    if (confirming_) {
        pressed = {};
        stickActive = false;
        stickX = knobX = stickBase_.x;
        stickY = knobY = stickBase_.y;
        pointing = false;
    }
    if (pointing) {
        pointed_ = true;
        lastPointX_ = pointX;
        lastPointY_ = pointY;
    }

    if (pressed != pressed_ || stickActive != stickActive_ || knobX != knobX_ || knobY != knobY_
        || stickX != stickX_ || stickY != stickY_)
        ++revision_;
    pressed_ = pressed;
    stickActive_ = stickActive;
    stickX_ = stickX, stickY_ = stickY, knobX_ = knobX, knobY_ = knobY;

    // The pointing finger as a mouse, every frame (also after it lifts, at the
    // last place it pointed): the game keeps a mouse pointer only for 3 s
    // after its last motion, then the touch gamepad's centred one.
    if (pointed_ && window) {
        pointX = lastPointX_;
        pointY = lastPointY_;
        int w = 0, h = 0;
        if (SDL_Window* target = SDL_GetWindowFromID(window); target && SDL_GetWindowSize(target, &w, &h)) {
            SDL_Event motion{};
            motion.type = SDL_EVENT_MOUSE_MOTION;
            motion.motion.timestamp = SDL_GetTicksNS();
            motion.motion.windowID = window;
            motion.motion.which = kPointerMouse;
            motion.motion.x = pointX * float(w);
            motion.motion.y = pointY * float(h);
            SDL_PushEvent(&motion);
        }
    }

    if (!joystick_) return;
    for (int n = 0; n < ControlCount; ++n) {
        if (kInputs[n].button >= 0) SDL_SetJoystickVirtualButton(joystick_, kInputs[n].button, pressed_[n]);
        if (kInputs[n].axis >= 0)
            // A trigger rests at the axis minimum: 0 would be half pressed.
            SDL_SetJoystickVirtualAxis(joystick_, kInputs[n].axis,
                                       pressed_[n] ? SDL_JOYSTICK_AXIS_MAX : SDL_JOYSTICK_AXIS_MIN);
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
    const auto centred = [&](const char* label, float size, float x, float y, ImU32 colour) {
        const ImVec2 extent = font->CalcTextSizeA(size, FLT_MAX, 0, label);
        draw->AddText(font, size, {x - extent.x * 0.5f, y - extent.y * 0.5f}, colour, label);
    };
    for (int n = 0; n < ControlCount; ++n) {
        const auto& b = buttons_[n];
        if (n == Quit) {
            // Discreet: a faint cross, no disc.
            centred(kLabels[n], b.r * 1.4f, b.x, b.y, IM_COL32(255, 255, 255, 110));
            continue;
        }
        draw->AddCircleFilled({b.x, b.y}, b.r, pressed_[n] ? fillPressed : fill);
        draw->AddCircle({b.x, b.y}, b.r, line, 0, thickness);
        const float size = n == Hit || n == Item ? b.r * 0.55f : b.r * 0.9f;
        centred(kLabels[n], size, b.x, b.y, text);
    }
    draw->AddCircleFilled({stickX_, stickY_}, stickRadius_, IM_COL32(255, 255, 255, stickActive_ ? 35 : 20));
    draw->AddCircle({stickX_, stickY_}, stickRadius_, line, 0, thickness);
    draw->AddCircleFilled({knobX_, knobY_}, stickRadius_ * 0.45f, stickActive_ ? fillPressed : IM_COL32(255, 255, 255, 70));
    if (!confirming_) return;
    // "Quitter la partie ?" over a dimmed screen.
    const float s = std::min(width_, height_);
    draw->AddRectFilled({0, 0}, {width_, height_}, IM_COL32(0, 0, 0, 140));
    draw->AddRectFilled({dialog_.x0, dialog_.y0}, {dialog_.x1, dialog_.y1}, IM_COL32(22, 26, 34, 245), 0.03f * s);
    draw->AddRect({dialog_.x0, dialog_.y0}, {dialog_.x1, dialog_.y1}, IM_COL32(255, 255, 255, 60), 0.03f * s, 0,
                  thickness);
    // iOS cannot reopen the launcher: leaving the game closes the app.
    centred("Quitter le jeu ?", 0.055f * s, (dialog_.x0 + dialog_.x1) * 0.5f, dialog_.y0 + 0.08f * s, text);
    centred("L'application va se fermer.", 0.036f * s, (dialog_.x0 + dialog_.x1) * 0.5f, dialog_.y0 + 0.145f * s,
            IM_COL32(211, 215, 221, 200));
    const auto button = [&](const Rect& r, const char* label, ImU32 colour) {
        draw->AddRectFilled({r.x0, r.y0}, {r.x1, r.y1}, colour, 0.02f * s);
        centred(label, 0.045f * s, (r.x0 + r.x1) * 0.5f, (r.y0 + r.y1) * 0.5f, IM_COL32(255, 255, 255, 240));
    };
    button(dialogCancel_, "Annuler", IM_COL32(70, 78, 92, 255));
    button(dialogQuit_, "Quitter", IM_COL32(196, 52, 46, 255));
}

} // namespace mscharged::runtime
