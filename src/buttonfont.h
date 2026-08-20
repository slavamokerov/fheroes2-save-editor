#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace fh2 {

// The game's button font. It does NOT exist in the original assets: the engine
// generates letter shapes in code (generateGoodButtonFontBaseShape, ui_font.cpp:4948)
// and adds effects (applyGoodButtonReleased/PressedLetterEffects,
// ui_font.cpp:119). Peculiarities: uppercase ASCII only, letter body is 10px,
// negative hotspot along X, 2px of margin around a letter for effects.
//
// A glyph is stored like in the engine — two planes:
//   image     — KB.PAL palette indices (letter body 56/62, contour 10);
//   transform — 1 transparent, 0 opaque (color from image),
//               2..9 — darken/lighten the background already drawn.
struct ButtonGlyph {
    int width = 0;
    int height = 0;
    int x = 0; // hotspot (in the button font −1..−3)
    int y = 0;
    std::vector<uint8_t> image;
    std::vector<uint8_t> transform;

    bool empty() const { return width <= 0 || height <= 0; }
};

// Ready-made glyph set (indexed by charcode, like in fheroes2).
const std::array<ButtonGlyph, 256> & buttonFont( bool pressed );

// Text width in the button font: space is 8 (ui_text.cpp:1386),
// advance = sprite.x() + sprite.width() (ui_text.cpp:1366).
int buttonTextWidth( const std::string & text );

// Whether all characters of a cp1251 string are available in the button font
// (port of isFontAvailable from fheroes2, ui_text.h).
bool buttonFontSupports( const std::string & text );

// Line height of the button font (getFontHeight, ui_text.cpp:335).
int buttonFontHeight();

// Draws a string into an INDEXED buffer of size bufWidth×bufHeight (KB.PAL indices):
// body/contour pixels are written with their color, while transform pixels darken or
// lighten the background already drawn via transformTable — like Blit in the engine.
// x, y — the pen (top of the line), like in Text::draw.
void drawButtonText( uint8_t * image, int bufWidth, int bufHeight, int x, int y, const std::string & text, bool pressed );

// A row of the engine's transformTable (image.cpp:43): the new color index for
// darkening/lightening a background pixel. transformId outside 2..9 — the color is unchanged.
uint8_t applyTransformTable( uint8_t colorId, uint8_t transformId );

} // namespace fh2
