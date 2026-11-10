#include "bootstrap/touch_layout.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <string_view>

namespace mscharged
{
namespace
{
constexpr int kMaxOffset = 3000; // thousandths of the short side
// D-pad buttons: their distance from its centre and their radius, as parts
// of the D-pad's radius (0.07 and 0.037 short sides at the default size).
constexpr float kDpadStep = 0.07f / 0.107f, kDpadButton = 0.037f / 0.107f;

float Anchor(char where, float size)
{
    return where == 'c' || where == 'm' ? size * 0.5f : where == 'r' || where == 'b' ? size : 0.0f;
}

bool ReadInt(std::string_view text, int& value)
{
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

std::string_view Trim(std::string_view text)
{
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) text.remove_prefix(1);
    while (!text.empty() && (text.back() == ' ' || text.back() == '\t')) text.remove_suffix(1);
    return text;
}

// "rb,-200,-180,100" into a placement; false when it is not one.
bool ReadPlacement(std::string_view text, TouchPlacement& placement)
{
    std::array<std::string_view, 4> parts;
    for (std::size_t n = 0; n < parts.size(); ++n)
    {
        const auto comma = text.find(',');
        if ((comma == std::string_view::npos) != (n + 1 == parts.size())) return false;
        parts[n] = Trim(text.substr(0, comma));
        if (comma != std::string_view::npos) text.remove_prefix(comma + 1);
    }
    if (parts[0].size() != 2) return false;
    const char horizontal = parts[0][0], vertical = parts[0][1];
    if ((horizontal != 'l' && horizontal != 'c' && horizontal != 'r')
        || (vertical != 't' && vertical != 'm' && vertical != 'b'))
        return false;
    TouchPlacement read{horizontal, vertical};
    if (!ReadInt(parts[1], read.dx) || !ReadInt(parts[2], read.dy) || !ReadInt(parts[3], read.scale)) return false;
    read.dx = std::clamp(read.dx, -kMaxOffset, kMaxOffset);
    read.dy = std::clamp(read.dy, -kMaxOffset, kMaxOffset);
    read.scale = std::clamp(read.scale, kMinTouchScale, kMaxTouchScale);
    placement = read;
    return true;
}
} // namespace

const TouchPlacements& DefaultTouchPlacements()
{
    // Under the right thumb A and the buttons around it; - and + at the top
    // in the middle, 1 and 2 at the top right, as on a Wii Remote held
    // sideways; the cross small in the top left corner; the D-pad at the top
    // left and the stick at the bottom left, under the left thumb. Away from
    // the top edge, where iOS delays touches for its own gestures.
    static const TouchPlacements defaults{{
        {'r', 'b', -200, -180, 100}, // A
        {'r', 'b', -70, -330, 100},  // B
        {'r', 'b', -220, -410, 100}, // C
        {'r', 'b', -370, -290, 100}, // Z
        {'r', 'b', -370, -100, 100}, // Tacle
        {'r', 'b', -70, -520, 100},  // Changer
        {'c', 't', 70, 150, 100},    // +
        {'c', 't', -70, 150, 100},   // -
        {'r', 't', -210, 150, 100},  // 1
        {'r', 't', -90, 150, 100},   // 2
        {'l', 't', 60, 70, 100},     // the cross
        {'l', 't', 160, 240, 100},   // D-pad
        {'l', 'b', 220, -220, 100},  // stick
    }};
    return defaults;
}

TouchPlacements ParseTouchLayout(const std::string& text)
{
    TouchPlacements placements = DefaultTouchPlacements();
    std::string_view rest = text;
    while (!rest.empty())
    {
        const auto bar = rest.find('|');
        const auto entry = Trim(rest.substr(0, bar));
        rest = bar == std::string_view::npos ? std::string_view{} : rest.substr(bar + 1);
        const auto colon = entry.find(':');
        if (colon == std::string_view::npos) continue;
        const auto key = Trim(entry.substr(0, colon));
        for (std::size_t item = 0; item < TouchItemCount; ++item)
            if (key == kTouchItems[item].key)
            {
                ReadPlacement(entry.substr(colon + 1), placements[item]);
                break;
            }
    }
    return placements;
}

std::string FormatTouchLayout(const TouchPlacements& placements)
{
    if (placements == DefaultTouchPlacements()) return {};
    std::string text;
    for (std::size_t item = 0; item < TouchItemCount; ++item)
    {
        const auto& p = placements[item];
        if (!text.empty()) text += " | ";
        text += std::string(kTouchItems[item].key) + ":" + p.horizontal + p.vertical + "," + std::to_string(p.dx)
            + "," + std::to_string(p.dy) + "," + std::to_string(p.scale);
    }
    return text;
}

TouchCircle PlaceTouchItem(TouchItem item, const TouchPlacement& placement, float width, float height)
{
    const float s = std::min(width, height);
    const float r = kTouchItems[item].radius * float(placement.scale) / 100.0f * s;
    float x = Anchor(placement.horizontal, width) + float(placement.dx) / 1000.0f * s;
    float y = Anchor(placement.vertical, height) + float(placement.dy) / 1000.0f * s;
    // On a screen of another shape a control could end up outside: keep it in.
    if (2 * r < width) x = std::clamp(x, r, width - r);
    if (2 * r < height) y = std::clamp(y, r, height - r);
    return {x, y, r};
}

TouchPlacement TouchPlacementAt(float x, float y, int scale, float width, float height)
{
    const float s = std::min(width, height);
    TouchPlacement placement;
    placement.horizontal = std::fabs(x - width * 0.5f) <= width * 0.15f ? 'c' : x < width * 0.5f ? 'l' : 'r';
    placement.vertical = std::fabs(y - height * 0.5f) <= height * 0.10f ? 'm' : y < height * 0.5f ? 't' : 'b';
    const auto offset = [&](float value) {
        return std::clamp(int(std::lround(value / s * 1000.0f)), -kMaxOffset, kMaxOffset);
    };
    placement.dx = offset(x - Anchor(placement.horizontal, width));
    placement.dy = offset(y - Anchor(placement.vertical, height));
    placement.scale = std::clamp(scale, kMinTouchScale, kMaxTouchScale);
    return placement;
}

std::array<TouchCircle, 4> TouchDpadButtons(const TouchCircle& dpad)
{
    const float step = dpad.r * kDpadStep, r = dpad.r * kDpadButton;
    return {{{dpad.x, dpad.y - step, r}, {dpad.x, dpad.y + step, r}, {dpad.x - step, dpad.y, r},
             {dpad.x + step, dpad.y, r}}};
}

TouchRect TouchStickZone(const TouchCircle& stick, float width, float height)
{
    // The default zone around the default stick, on this screen.
    const TouchCircle home = PlaceTouchItem(TouchStick, DefaultTouchPlacements()[TouchStick], width, height);
    const TouchRect zone{0.0f, height * 0.45f, width * 0.40f, height};
    const float k = home.r > 0 ? stick.r / home.r : 1.0f;
    float left = (zone.x0 - home.x) * k, right = (zone.x1 - home.x) * k;
    float top = (zone.y0 - home.y) * k, bottom = (zone.y1 - home.y) * k;
    if (stick.x > width * 0.5f) std::swap(left, right), left = -left, right = -right;
    if (stick.y < height * 0.5f) std::swap(top, bottom), top = -top, bottom = -bottom;
    return {stick.x + left, stick.y + top, stick.x + right, stick.y + bottom};
}
} // namespace mscharged
