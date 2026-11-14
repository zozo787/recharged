#pragma once

#include "bootstrap/touch_layout.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_joystick.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct ImDrawList;
struct ImFont;

namespace mscharged::runtime {

// On-screen controls for touch screens (iOS): a stick and buttons drawn over
// the game, driving an SDL virtual gamepad. The game takes that gamepad like
// any other (platform/desktop_wpad.cpp) with the default layout of
// bootstrap/gamepad_actions.h: the stick is the Nunchuk stick, A/B the Remote's
// A/B, C and Z the Nunchuk buttons, "Hit" shakes the Remote (hit an
// opponent), "Swap" shakes the Nunchuk (swap items; C uses one), +, -, 1 and
// 2 are the Remote's (no HOME: the game is built without its HOME Menu), and
// the small pad is the D-pad. A small cross at the top left opens "Quit the
// game?": Quit asks the game window to close, as closing it does on a
// computer (the game powers off like a Wii; iOS cannot start the app again, so
// the app closes); Cancel goes back to the game. Created only when a player
// chose the touch screen (controls.playerN = touch), before the input owner
// sets up the players. While a physical controller is connected it plays
// instead and the controls hide (the virtual gamepad stays attached, released).
// A finger that lands away from the buttons and the stick's corner points the
// Wii Remote: it is sent to the game as a mouse (the mouse points for player 1
// when the touch screen plays alone, platform/hardware_owner.cpp); A clicks.
// The controls sit where controls.touch_layout puts them (the launcher's
// layout editor; bootstrap/touch_layout.h), by default as described here.
// Use it on the presentation thread, between frames.
class TouchControls {
public:
    // Turns off SDL's mouse events from touches (they would also be read as
    // the keyboard-and-mouse Remote's pointer and buttons) until destroyed.
    // `layout`: controls.touch_layout (bootstrap/touch_layout.h); `opacity`:
    // controls.touch_opacity, in percent of the controls' default look;
    // `haptics`: controls.touch_haptics, a light vibration on each press
    // (runtime/touch_haptics.h).
    TouchControls(const std::string& layout, int opacity, bool haptics);
    ~TouchControls();
    TouchControls(const TouchControls&) = delete;
    TouchControls& operator=(const TouchControls&) = delete;

    // Once per frame with the drawing surface's size in pixels: applies the
    // fingers to the virtual gamepad.
    // `left` and `right`: the screen's safe area margins on the surface (a
    // notch or a Dynamic Island in landscape), which the controls keep clear of.
    void Service(float width, float height, float left = 0, float right = 0);
    bool Visible() const { return visible_; }
    // The virtual gamepad's SDL joystick id (0 when SDL refused it), which the
    // input owner gives the touch player's channel (NativePlayers::touch_id).
    std::uint32_t JoystickId() const { return id_; }
    // Changes whenever the drawing changes.
    std::uint64_t Revision() const { return revision_; }
    // Into the overlay, in surface pixels. `font` labels the buttons.
    void Draw(ImDrawList* draw, ImFont* font) const;

    enum Control : int {
        None = -1, A, B, C, Z, Hit, Item, Plus, Minus, One, Two, Quit, Up, Down, Left, Right, ControlCount
    };

private:
    struct Finger {
        float x = 0, y = 0; // window-normalized
        bool assigned = false; // given a control (or none) by Service
        int control = None;  // the button it presses, or None
        bool stick = false;  // it moves the stick
        bool pointer = false; // it points the Wii Remote
    };
    static bool SDLCALL Watch(void* self, SDL_Event* event);
    void Layout(float width, float height, float left, float right);
    int ButtonAt(float x, float y) const; // pixels; None outside every button
    void Attach();
    void Detach();

    mutable std::mutex mutex_;
    std::unordered_map<SDL_FingerID, Finger> fingers_; // guarded by mutex_
    // Fingers that went down and up between two frames (iOS delivers touches
    // that start near a screen edge late, often down and up at once), in
    // window-normalized coordinates; guarded by mutex_. Service presses the
    // button under each for a few frames, so the game sees it.
    std::vector<std::pair<float, float>> taps_;
    // SDL_HINT_TOUCH_MOUSE_EVENTS before this, restored afterwards.
    std::string previousHint_;
    bool hadHint_ = false;
    bool watching_ = false;
    bool gamepads_ = false; // holds a reference to SDL's gamepad subsystem

    TouchPlacements placements_;
    float opacity_ = 1.0f;
    bool haptics_ = true;
    float width_ = 0, height_ = 0;
    float left_ = 0, right_ = 0; // safe area margins (Service)
    std::array<TouchCircle, ControlCount> buttons_{};
    TouchRect stickZone_{}; // where a finger that lands takes the stick
    TouchCircle stickBase_{}; // the stick's resting place
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
    SDL_WindowID window_ = 0; // the window the fingers touch (guarded by mutex_)

    // Diagnostics for the session log: what the controls
    // see on the device. Counters guarded by mutex_.
    unsigned resizes_ = 0;
    int windowW_ = 0, windowH_ = 0;
    unsigned loggedResizes_ = 0;
    bool loggedService_ = false;
    // The cross's confirmation window and its buttons, in surface pixels.
    TouchRect dialog_{}, dialogQuit_{}, dialogCancel_{};
    bool confirming_ = false;    // "Quit the game?" is open
    bool quitWasPressed_ = false; // the cross was pressed on the last frame
    bool quitSent_ = false;       // the close request went out
    // Frames a tapped button stays pressed.
    std::array<int, ControlCount> held_{};
    // The pointer stays where the finger left it: the game takes the mouse only
    // for 3 s after its last motion, then the touch gamepad's centred pointer.
    bool pointed_ = false;
    float lastPointX_ = 0, lastPointY_ = 0; // window-normalized
    std::chrono::steady_clock::time_point lastResizeLog_{};
};

} // namespace mscharged::runtime
