#pragma once

#include <array>
#include <cstddef>
#include <string>

// The on-screen touch controls' layout (iOS): where each control sits and how
// big it is, on a screen of any size. The game places, draws and reads its
// controls with it (runtime/touch_controls.cpp); the launcher's layout editor
// (Controls, beside the touch screen player) moves and sizes them and saves
// the result as controls.touch_layout.
namespace mscharged
{
enum TouchItem : std::size_t
{
    // A to Quit in the order of runtime::TouchControls::Control.
    TouchA, TouchB, TouchC, TouchZ, TouchHit, TouchSwap, TouchPlus, TouchMinus, TouchOne, TouchTwo, TouchQuit,
    TouchDpad,  // the four D-pad buttons, as one piece
    TouchStick, // where the stick rests
    TouchItemCount
};

struct TouchItemInfo
{
    const char* key;   // in controls.touch_layout
    const char* label; // on the control (the game's font: ASCII and the cross)
    float radius;      // default radius, in short sides of the screen
};
// The D-pad's radius reaches the outer edge of its buttons.
inline constexpr std::array<TouchItemInfo, TouchItemCount> kTouchItems{{
    {"a", "A", 0.10f},
    {"b", "B", 0.065f},
    {"c", "C", 0.055f},
    {"z", "Z", 0.06f},
    {"hit", "Tacle", 0.065f},
    {"swap", "Changer", 0.05f},
    {"plus", "+", 0.045f},
    {"minus", "-", 0.045f},
    {"one", "1", 0.045f},
    {"two", "2", 0.045f},
    {"quit", "\xC3\x97", 0.032f},
    {"dpad", "", 0.107f},
    {"stick", "", 0.12f},
}};

// A control's place: its centre as an offset from a point of the screen
// (horizontal: l left edge, c centre, r right edge; vertical: t top edge,
// m middle, b bottom edge), in thousandths of the screen's short side, and
// its size in percent of the default. Offsets in short sides keep a control
// at the same distance from the thumb on every screen (a folded or unfolded
// iPhone, an iPad).
struct TouchPlacement
{
    char horizontal = 'l';
    char vertical = 't';
    int dx = 0, dy = 0;
    int scale = 100;
    bool operator==(const TouchPlacement& other) const
    {
        return horizontal == other.horizontal && vertical == other.vertical && dx == other.dx && dy == other.dy
            && scale == other.scale;
    }
    bool operator!=(const TouchPlacement& other) const { return !(*this == other); }
};
using TouchPlacements = std::array<TouchPlacement, TouchItemCount>;

inline constexpr int kMinTouchScale = 50, kMaxTouchScale = 200, kTouchScaleStep = 10;

struct TouchCircle
{
    float x = 0, y = 0, r = 0;
};
struct TouchRect
{
    float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    bool Contains(float x, float y) const { return x >= x0 && x < x1 && y >= y0 && y < y1; }
};

const TouchPlacements& DefaultTouchPlacements();
// controls.touch_layout: "key:anchor,dx,dy,scale" per control, separated by
// " | " (a:rb,-200,-180,100); empty for the default layout. Parsing starts
// from the default and skips what it cannot read, so a damaged value never
// stops the game.
TouchPlacements ParseTouchLayout(const std::string& text);
std::string FormatTouchLayout(const TouchPlacements& placements);

// Where a control is on a width x height screen (any unit: the game's pixels,
// the launcher's points).
TouchCircle PlaceTouchItem(TouchItem item, const TouchPlacement& placement, float width, float height);
// The placement that puts a control's centre at (x, y), at `scale`: the
// anchor is the nearest edge, or the centre line for a control near it.
TouchPlacement TouchPlacementAt(float x, float y, int scale, float width, float height);
// The D-pad's up, down, left and right buttons.
std::array<TouchCircle, 4> TouchDpadButtons(const TouchCircle& dpad);
// Where a finger that lands takes the stick: the default stick's corner of
// the screen (left 40 %, lower 55 %), moved and sized with the stick, and
// turned to the side of the screen the stick is on.
TouchRect TouchStickZone(const TouchCircle& stick, float width, float height);
} // namespace mscharged
