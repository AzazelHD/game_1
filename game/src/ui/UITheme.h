#pragma once

#include "engine/renderer/Color.h"

namespace UITheme
{
    inline constexpr Color Background = Color{0x1A, 0x1E, 0x24, 255};
    inline constexpr Color Panel = Color{0x25, 0x2B, 0x33, 235};
    inline constexpr Color Border = Color{0x5A, 0x66, 0x73, 255};
    inline constexpr Color Text = Color{0xE7, 0xE2, 0xD8, 255};
    inline constexpr Color SelectedText = Color{0xFF, 0xF6, 0xC2, 255};
    inline constexpr Color SelectedBG = Color{0x48, 0x5A, 0x73, 220};
    inline constexpr Color Cursor = Color{0xD8, 0xB6, 0x57, 255};
    inline constexpr Color PopupBG = Color{0x2A, 0x30, 0x38, 245};
    inline constexpr Color PopupBorder = Color{0x6B, 0x78, 0x87, 255};
    inline constexpr Color Danger = Color{0xB6, 0x4B, 0x4B, 255};
    inline constexpr Color Success = Color{0x4F, 0xA3, 0x6D, 255};
    inline constexpr Color Info = Color{0x52, 0x78, 0xC4, 255};
}
