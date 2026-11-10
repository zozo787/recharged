#pragma once

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_joystick.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

struct ImDrawList;
struct ImFont;

namespace mscharged::runtime {

// On-screen controls for touch screens (iOS): a stick and buttons drawn over
// the game, driving an SDL virtual gamepad. The game takes that gamepad like
// any other (platform/desktop_wpad.cpp) with the default layout of
// bootstrap/gamepad_actions.h: the stick is the Nunchuk stick, A/B the Remote's
// A/B, C and Z the Nunchuk buttons, "Tacle" shakes the Remote (hit an
// opponent), "Objet" shakes the Nunchuk (switch items), + pauses and the small
// pad is the D-pad. Hidden, and the virtual gamepad removed, while a physical
// controller is connected. Use it on the presentation thread, between frames.
class TouchControls {
public:
    // Turns off SDL's mouse events from touches (they would also be read as
    // the keyboard-and-mouse Remote's pointer and buttons) until destroyed.
    TouchControls();
    ~TouchControls();
    TouchControls(const TouchControls&) = delete;
    TouchControls& operator=(const TouchControls&) = delete;

    // Once per frame with the drawing surface's size in pixels: applies the
    // fingers to the virtual gamepad.
    void Service(float width, float height);
    bool Visible() const { return visible_; }
    // Changes whenever the drawing changes.
    std::uint64_t Revision() const { return revision_; }
    // Into the overlay, in surface pixels. `font` labels the buttons.
    void Draw(ImDrawList* draw, ImFont* font) const;

    enum Control : int {
        None = -1, A, B, C, Z, Hit, Item, Plus, Up, Down, Left, Right, ControlCount
    };

private:
    struct Finger {
        float x = 0, y = 0; // window-normalized
        bool assigned = false; // given a control (or none) by Service
        int control = None;  // the button it presses, or None
        bool stick = false;  // it moves the stick
    };
    struct Circle {
        float x = 0, y = 0, r = 0;
    };

    static bool SDLCALL Watch(void* self, SDL_Event* event);
    void Layout(float width, float height);
    int ButtonAt(float x, float y) const; // pixels; None outside every button
    void Attach();
    void Detach();

    mutable std::mutex mutex_;
    std::unordered_map<SDL_FingerID, Finger> fingers_; // guarded by mutex_
    // SDL_HINT_TOUCH_MOUSE_EVENTS before this, restored afterwards.
    std::string previousHint_;
    bool hadHint_ = false;
    bool watching_ = false;
    bool gamepads_ = false; // holds a reference to SDL's gamepad subsystem

    float width_ = 0, height_ = 0;
    std::array<Circle, ControlCount> buttons_{};
    Circle stickBase_{}; // the stick's resting place
    float stickRadius_ = 0;
    // Current state.
    std::array<bool, ControlCount> pressed_{};
    bool stickActive_ = false;
    float stickX_ = 0, stickY_ = 0;       // where the stick's centre is, pixels
    float knobX_ = 0, knobY_ = 0;         // where the knob is, pixels
    bool visible_ = true;
    std::uint64_t revision_ = 1;

    SDL_JoystickID id_ = 0;
    SDL_Joystick* joystick_ = nullptr;

    // Diagnostics for the session log (docs/IOS_PORT.md): what the controls
    // see on the device. Counters guarded by mutex_.
    unsigned fingerDowns_ = 0, resizes_ = 0;
    float firstX_ = 0, firstY_ = 0;
    int windowW_ = 0, windowH_ = 0;
    unsigned loggedFingers_ = 0, loggedResizes_ = 0;
    bool loggedService_ = false;
    std::array<bool, ControlCount> loggedPress_{};
    std::chrono::steady_clock::time_point lastResizeLog_{};
};

} // namespace mscharged::runtime
