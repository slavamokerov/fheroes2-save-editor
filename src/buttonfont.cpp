#include "buttonfont.h"

#include <algorithm>
#include <cstdlib>

namespace fh2 {

namespace {

// Button font colors (ui_font.cpp:40-45), KB.PAL indices.
constexpr uint8_t buttonGoodReleasedColor = 56; // letter body, released
constexpr uint8_t buttonGoodPressedColor = 62;  // letter body, pressed
constexpr uint8_t buttonContourColor = 10;      // white contour (released only)

struct Point {
    int x = 0;
    int y = 0;
};

constexpr Point buttonFontOffset{ -1, 0 };

// Button font metrics: line height 15 (ui_text.cpp:335),
// space width 8 (ui_text.cpp:1386).
constexpr int BUTTON_FONT_HEIGHT = 15;
constexpr int BUTTON_SPACE_WIDTH = 8;

// Rows of the engine's transformTable (image.cpp:43) for transform-id 2..9:
// darkening (2..5) and lightening (6..9) of an already drawn pixel.
constexpr int FIRST_TRANSFORM_ROW = 2;
constexpr int LAST_TRANSFORM_ROW = 9;

const uint8_t transformRows[LAST_TRANSFORM_ROW - FIRST_TRANSFORM_ROW + 1][256] = {
    { // transform 2
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  16,  17,  18,  19,  20,  21,
         22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  36,
         36,  36,  36,  36,  36,  43,  44,  45,  46,  47,  48,  49,  50,  51,  52,  53,
         54,  55,  56,  57,  58,  59,  60,  61,  62,  62,  62,  62,  62,  62,  62,  68,
         69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,
         84,  84,  84,  84,  84,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101,
        102, 103, 104, 105, 106, 107, 107, 107, 107, 107, 107, 107, 114, 115, 116, 117,
        118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 130, 130, 130,
        130, 130, 130, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148,
        149, 150, 151, 151, 151, 151, 151, 151, 158, 159, 160, 161, 162, 163, 164, 165,
        166, 167, 168, 169, 170, 171, 172, 173, 174, 174, 174, 174, 174, 174, 174, 180,
        181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196,
        197, 197, 197, 197, 197, 197, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211,
        212, 213, 213, 213, 213, 213, 214, 215, 216, 217, 218, 219, 220, 221, 225, 226,
        227, 228, 229, 230, 230, 230, 230,  73,  75,  77,  79,  81,  76,  78,  74,  76,
         78,  80, 244, 245, 245, 245,  73,  75,  77,  81, 250, 251, 252, 253,   0,   0,
    },
    { // transform 3
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  14,  15,  16,  17,  18,  19,
         20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,
         36,  36,  36,  36,  36,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,
         52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  62,  62,  62,  62,  66,
         67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,
         83,  84,  84,  84,  84,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,
        100, 101, 102, 103, 104, 105, 106, 107, 107, 107, 107, 107, 112, 113, 114, 115,
        116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 130,
        130, 130, 130, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146,
        147, 148, 149, 150, 151, 151, 151, 151, 156, 157, 158, 159, 160, 161, 162, 163,
        164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 174, 174, 174, 174, 178,
        179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
        195, 196, 197, 197, 197, 197, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210,
        211, 212, 213, 213, 213, 213, 214, 215, 216, 217, 218, 219, 220, 221, 224, 225,
        226, 227, 228, 229, 230, 230, 230,  76,  76,  76,  76,  76,  76,  76,  76,  76,
         76,  78, 244, 245, 245, 245,  76,  76,  76,  76, 250, 251, 252, 253,   0,   0,
    },
    { // transform 4
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  12,  13,  14,  15,  16,  17,
         18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,
         34,  35,  36,  36,  36,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,  49,
         50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  62,  62,  65,
         66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,
         82,  83,  84,  84,  84,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,
         98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 107, 107, 110, 111, 112, 113,
        114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
        130, 130, 130, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145,
        146, 147, 148, 149, 150, 151, 151, 151, 154, 155, 156, 157, 158, 159, 160, 161,
        162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 174, 174, 177,
        178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193,
        194, 195, 196, 197, 197, 197, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
        210, 211, 212, 213, 213, 213, 214, 215, 216, 217, 218, 219, 220, 221, 223, 224,
        225, 226, 227, 228, 229, 230, 230,  76,  76,  76,  76,  76,  76,  76,  76,  76,
         76,  76, 243, 244, 245, 245,  76,  76,  76,  76, 250, 251, 252, 253,   0,   0,
    },
    { // transform 5
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  11,  12,  13,  14,  15,  16,
         17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,
         33,  34,  35,  36,  36,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
         49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  62,  64,
         65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,
         81,  82,  83,  84,  84,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,
         97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 107, 109, 110, 111, 112,
        113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128,
        129, 130, 130, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144,
        145, 146, 147, 148, 149, 150, 151, 151, 153, 154, 155, 156, 157, 158, 159, 160,
        161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 174, 176,
        177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192,
        193, 194, 195, 196, 197, 197, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208,
        209, 210, 211, 212, 213, 213, 214, 215, 216, 217, 218, 219, 220, 221, 223, 224,
        225, 226, 227, 228, 229, 230, 230,  75,  75,  75,  75,  75,  75,  75,  75,  75,
         75,  75, 243, 244, 245, 245,  75,  75,  75,  75, 250, 251, 252, 253,   0,   0,
    },
    { // transform 6
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,  10,  11,  11,  11,  12,
         13,  13,  13,  14,  14,  15,  15,  15,  16,  17,  17,  17,  18,  18,  19,  19,
         20,  20,  20,  21,  21,  11,  37,  37,  37,  38,  38,  39,  39,  39,  40,  40,
         41,  41,  41,  41,  42,  42,  19,  42,  20,  20,  20,  20,  20,  20,  21,  12,
        131,  63,  63,  63,  64,  64,  64,  65,  65,  65,  65,  65, 242, 242, 242, 242,
        242, 242, 242, 242, 242,  13,  14,  15,  15,  16,  85,  17,  85,  85,  85,  85,
         19,  86,  20,  20,  20,  21,  21,  21,  21,  21,  21,  21,  10, 108, 108, 109,
        109, 109, 110, 110, 110, 110, 199,  40,  41,  41,  41,  41,  41,  42,  42,  42,
         42,  20,  20,  11,  11, 131, 131, 132, 132, 132, 133, 133, 134, 134, 134, 135,
        135,  18, 136,  19,  19,  20,  20,  20,  10,  11,  11,  11,  12,  12,  13,  13,
         13,  14,  15,  15,  15,  16,  17,  17,  17,  18,  18,  19,  19,  20,  20,  11,
        175, 175, 176, 176,  38, 177, 177, 178, 178, 178, 179, 179, 179, 179, 180, 180,
        180, 180, 180, 180,  21,  21, 108, 108,  38, 109,  38, 109,  39,  40,  40,  41,
         41,  41,  42,  42,  42,  20, 199, 179, 180, 180, 110, 110,  40,  42, 110, 110,
         86,  86,  86,  86,  18,  18,  19,  65,  65,  65,  66,  65,  66,  65, 152, 155,
         65, 242,  15,  16,  17,  19,  65,  65,  65,  65, 199, 179, 180, 180,   0,   0,
    },
    { // transform 7
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,  11,  11,  12,  12,  13,
         13,  14,  15,  15,  16,  16,  17,  17,  18,  19,  20,  20,  21,  21,  22,  22,
         23,  24,  24,  25,  25,  37,  37,  38,  38,  39,  39,  40,  41,  41,  41,  42,
         42,  43,  43,  44,  44,  45,  45,  46,  46,  23,  24,  24,  24,  24,  24, 131,
         63,  63,  64,  64,  65,  65,  66,  66, 242,  67,  67,  68,  68, 243, 243, 243,
        243, 243, 243, 243, 243,  15,  15,  85,  85,  85,  85,  86,  86,  87,  87,  88,
         88,  88,  88,  89,  24,  90,  25,  25,  25,  25,  25,  25,  37, 108, 109, 109,
        110, 110, 111, 111, 200, 200, 201, 201,  42,  43,  43,  44,  44,  44,  45,  45,
         46,  46,  46,  11, 131, 132, 132, 132, 133, 133, 134, 135, 135, 136, 242, 137,
        137, 138, 243, 243, 243, 243, 243,  24, 152, 152, 153, 153, 154, 154, 155, 156,
        156, 157, 158, 158, 159,  18,  19,  19,  20,  20,  21,  22,  22,  23,  24,  37,
        175, 176, 176, 177, 177, 178, 179, 179, 180, 180, 180, 181, 181, 181, 182, 182,
        182,  46,  47,  47,  48,  25, 108, 109, 109, 109, 198, 199, 199, 201, 201,  42,
         43,  43,  44,  45,  46,  46, 201, 181, 182, 183, 111, 111, 202,  45, 111, 111,
         87,  88,  88,  88,  88,  21,  22,  66,  66,  68,  68,  67,  68,  68, 152, 157,
         66,  69,  16,  18,  20,  21,  66,  66,  68,  67, 201, 181, 182, 183,   0,   0,
    },
    { // transform 8
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,  11,  11,  12,  13,  14,
         14,  15,  16,  17,  17,  18,  19,  20,  20,  21,  22,  23,  24,  24,  25,  26,
         26,  27,  28,  29,  29,  37,  37,  38,  39,  40,  40,  41,  42,  42,  43,  44,
         44,  45,  46,  46,  47,  47,  48,  48,  49,  50,  50,  27,  28,  28,  28,  63,
         63,  64,  65,  65,  66,  67,  67,  68,  69,  69,  69,  70,  70,  70, 244,  71,
        244, 244, 244, 244, 245,  16,  85,  85,  86,  87,  87,  88,  88,  89,  90,  90,
         91,  91,  91,  92,  93,  93,  93,  29,  29,  29,  29,  29,  37, 109, 109, 110,
        111, 111, 112, 113, 112, 112, 203, 203, 203,  44,  45,  46,  47,  47,  47,  48,
         48,  49,  50, 131, 131, 132, 133, 133, 134, 135, 136, 136, 137, 137, 139, 139,
        139, 141, 141, 141, 143, 143, 245, 245, 152, 152, 153, 154, 155, 155, 156, 157,
        158, 158, 159, 160, 161, 162, 163, 163, 164, 165, 165, 166,  26,  26,  27, 175,
         13, 176, 177, 178, 178, 179, 180, 181, 181, 182, 182, 183, 183, 183, 184, 184,
        185, 185,  50,  50,  52,  52, 109, 109, 198, 199, 200, 201, 201, 202, 202,  44,
         45,  46,  47,  48,  48,  49, 204, 205, 185, 185, 112, 112, 204,  47, 112, 113,
         88,  89,  91,  92,  93,  93,  25,  66,  68,  69,  69,  68,  69,  69, 153, 159,
         68,  71,  18, 242, 243,  24,  66,  68,  69,  68, 204, 205, 185, 185,   0,   0,
    },
    { // transform 9
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  10,  11,  12,  13,  13,  14,
         15,  16,  17,  17,  19,  19,  20,  21,  22,  23,  24,  24,  26,  26,  27,  28,
         28,  30,  30,  31,  32,  37,  38,  39,  39,  40,  41,  42,  43,  43,  44,  45,
         46,  46,  47,  48,  49,  50,  50,  51,  52,  52,  53,  54,  54,  30,  31,  63,
         64,  64,  65,  66,  67,  68,  69,  69,  70,  71,  71,  71,  72,  72,  72,  73,
         73,  73, 168, 168, 168,  85,  85,  86,  87,  88,  88,  89,  90,  91,  91,  92,
         93,  93,  94,  95,  95,  96,  96,  96,  31,  32,  32,  32, 108, 109, 198, 110,
        111, 112, 113, 113, 113, 116, 117, 118, 119, 120, 121,  47,  48,  50,  50,  51,
         51,  52,  52, 131, 132, 132, 133, 134, 135, 136, 137, 137, 138, 139, 140, 141,
        141, 143, 143, 144, 145, 146, 147,  30, 152, 153, 153, 154, 155, 156, 157, 158,
        158, 159, 160, 161, 162, 163, 164, 165, 165, 166, 167, 168, 169,  28,  29, 175,
        176, 177, 177, 178, 179, 180, 181, 182, 182, 183, 184, 185, 185, 185, 186, 186,
        187,  50,  52,  52,  54,  55, 109, 198, 199, 200, 201, 202, 202, 204, 204, 205,
        207,  47,  49,  50,  51,  52, 206, 206, 187, 188, 113, 113, 118,  49, 222, 222,
        223, 224, 225, 226,  95, 227, 228,  67,  68,  70,  71,  69,  71,  70, 153,  65,
         69,  73, 242,  22, 243, 244,  67,  68,  70,  69, 206, 206, 187, 188,   0,   0,
    },
};

void resizeGlyph( ButtonGlyph & g, int width, int height )
{
    g.width = width;
    g.height = height;
    g.image.assign( static_cast<size_t>( width ) * height, 0 );
    // reset(): the whole sprite is transparent (transform == 1).
    g.transform.assign( static_cast<size_t>( width ) * height, 1 );
}

void setGlyphPosition( ButtonGlyph & g, int x, int y )
{
    g.x = x;
    g.y = y;
}

void setPixel( ButtonGlyph & g, int x, int y, uint8_t value )
{
    if ( x < 0 || y < 0 || x >= g.width || y >= g.height )
        return;
    const size_t offset = static_cast<size_t>( y ) * g.width + x;
    g.image[offset] = value;
    g.transform[offset] = 0;
}

void fillRect( ButtonGlyph & g, int x, int y, int width, int height, uint8_t value )
{
    for ( int yy = y; yy < y + height; ++yy ) {
        for ( int xx = x; xx < x + width; ++xx )
            setPixel( g, xx, yy, value );
    }
}

// Port of fheroes2::DrawLine (image.cpp:2003) — incremental Bresenham,
// pixel-for-pixel like in the engine.
void drawLine( ButtonGlyph & g, int x1, int y1, int x2, int y2, uint8_t value )
{
    if ( g.empty() )
        return;

    const int dx = std::abs( x2 - x1 );
    const int dy = std::abs( y2 - y1 );

    if ( dx >= dy ) {
        int ns = dx / 2;
        for ( int i = 0; i <= dx; ++i ) {
            setPixel( g, x1, y1, value );
            x1 < x2 ? ++x1 : --x1;
            ns -= dy;
            if ( ns < 0 ) {
                y1 < y2 ? ++y1 : --y1;
                ns += dx;
            }
        }
    }
    else {
        int ns = dy / 2;
        for ( int i = 0; i <= dy; ++i ) {
            setPixel( g, x1, y1, value );
            y1 < y2 ? ++y1 : --y1;
            ns -= dx;
            if ( ns < 0 ) {
                x1 < x2 ? ++x1 : --x1;
                ns += dy;
            }
        }
    }
}

// Port of fheroes2::updateShadow (image.cpp:3318): marks transparent pixels
// next to opaque ones as "shadow" (transform-id).
void updateShadow( ButtonGlyph & g, const Point & shadowOffset, uint8_t transformId, bool connectCorners )
{
    if ( g.empty() || std::abs( shadowOffset.x ) >= g.width || std::abs( shadowOffset.y ) >= g.height
         || ( shadowOffset.x == 0 && shadowOffset.y == 0 ) )
        return;

    const int imageWidth = g.width;
    const int width = imageWidth - std::abs( shadowOffset.x );
    const int height = g.height - std::abs( shadowOffset.y );
    const int size = static_cast<int>( g.transform.size() );

    int inBase = 0;
    int outBase = 0;
    int cornerOffsetX = 0;
    int cornerOffsetY = 0;

    if ( shadowOffset.x > 0 ) {
        outBase += shadowOffset.x;
        cornerOffsetX = 1;
    }
    else {
        inBase -= shadowOffset.x;
        cornerOffsetX = -1;
    }

    if ( shadowOffset.y > 0 ) {
        outBase += imageWidth * shadowOffset.y;
        cornerOffsetY = imageWidth;
    }
    else {
        inBase -= imageWidth * shadowOffset.y;
        cornerOffsetY = -imageWidth;
    }

    const auto transformAt = [&g, size]( int offset ) -> uint8_t { return ( offset >= 0 && offset < size ) ? g.transform[offset] : 1; };

    for ( int row = 0; row < height; ++row ) {
        int in = inBase + row * imageWidth;
        int out = outBase + row * imageWidth;
        for ( int col = 0; col < width; ++col, ++in, ++out ) {
            if ( transformAt( out ) == 1
                 && ( transformAt( in ) == 0
                      || ( connectCorners && transformAt( in + cornerOffsetX ) == 0 && transformAt( in + cornerOffsetY ) == 0 ) ) ) {
                g.transform[out] = transformId;
            }
        }
    }
}

// Port of addContour (ui_font.cpp:79): a contour of color colorId with a shift of
// (-1, +1) — only over pixels that are still transparent.
void addContour( ButtonGlyph & g, const Point & contourOffset, uint8_t colorId )
{
    if ( g.empty() || contourOffset.x > 0 || contourOffset.y < 0 || -contourOffset.x >= g.width || contourOffset.y >= g.height )
        return;

    const ButtonGlyph input = g;
    const int imageWidth = g.width;
    const int width = imageWidth + contourOffset.x;
    const int height = g.height - contourOffset.y;
    const int offsetY = imageWidth * contourOffset.y;
    const int size = static_cast<int>( g.transform.size() );

    const auto inTransform = [&input, size]( int offset ) -> uint8_t { return ( offset >= 0 && offset < size ) ? input.transform[offset] : 1; };

    for ( int row = 0; row < height; ++row ) {
        int in = row * imageWidth - contourOffset.x;
        int out = offsetY + row * imageWidth;
        for ( int col = 0; col < width; ++col, ++in, ++out ) {
            if ( out < 0 || out >= size )
                continue;
            if ( g.transform[out] == 1
                 && ( inTransform( in ) == 0 || ( inTransform( in + contourOffset.x ) == 0 && inTransform( in + offsetY ) == 0 ) ) ) {
                g.image[out] = colorId;
                g.transform[out] = 0;
            }
        }
    }
}

void replaceColorId( ButtonGlyph & g, uint8_t from, uint8_t to )
{
    for ( size_t i = 0; i < g.image.size(); ++i ) {
        if ( g.transform[i] == 0 && g.image[i] == from )
            g.image[i] = to;
    }
}

// Button letter effects (ui_font.cpp:119-134).
void applyGoodButtonReleasedLetterEffects( ButtonGlyph & letter )
{
    updateShadow( letter, { 1, -1 }, 2, true );
    updateShadow( letter, { 2, -2 }, 4, true );
    addContour( letter, { -1, 1 }, buttonContourColor );
    updateShadow( letter, { -1, 1 }, 7, true );
}

void applyGoodButtonPressedLetterEffects( ButtonGlyph & letter )
{
    replaceColorId( letter, buttonGoodReleasedColor, buttonGoodPressedColor );

    updateShadow( letter, { 1, -1 }, 2, true );
    updateShadow( letter, { -1, 1 }, 7, true );
    updateShadow( letter, { -2, 2 }, 8, true );
}

// Port of generateGoodButtonFontBaseShape (ui_font.cpp:4948): letter shapes
// of the button font (uppercase ASCII 33..95 + 124/126/127 only).
// The code was ported from the engine mechanically (DrawLine/SetPixel/Fill).
void generateGoodButtonFontBaseShape( std::array<ButtonGlyph, 256> & g )
{
        // Button font does not exist in the original game assets but we can regenerate it from scratch.
        // All letters in buttons have some variations in colors but overall shapes are the same.
        // We want to standardize the font and to use one approach to generate letters.
        // The shape of the letter is defined only by one color (in general). The rest of information is generated from transformations and contours.
        //
        // Another essential difference from normal fonts is that button font has only uppercase letters.
        // This means that we need to generate only 26 letter of English alphabet, 10 digits and few special characters, totalling in about 50 symbols.
        // The downside of this font is that code is necessary for the generation of released and pressed states of each letter.

        // We need 2 pixels from all sides of a letter to add extra effects.
        const int32_t offset = 2;

        // Since all symbols have -1 shift by X axis to avoid any issues with alignment we need to makes all images at least 1 pixel in size.
        // These images are completely transparent.
        for ( ButtonGlyph & letter : g ) {
            resizeGlyph( letter, 1, 1 );

            setGlyphPosition( letter, buttonFontOffset.x, buttonFontOffset.y );
        }
        // Address symbols that should have even less space to neighboring symbols.
        setGlyphPosition( g[33], buttonFontOffset.x - 1, buttonFontOffset.y );
        setGlyphPosition( g[45], buttonFontOffset.x - 2, buttonFontOffset.y );
        setGlyphPosition( g[46], buttonFontOffset.x - 1, buttonFontOffset.y );
        setGlyphPosition( g[58], buttonFontOffset.x - 2, buttonFontOffset.y );
        setGlyphPosition( g[59], buttonFontOffset.x - 1, buttonFontOffset.y );
        setGlyphPosition( g[63], buttonFontOffset.x - 1, buttonFontOffset.y );
        setGlyphPosition( g[65], buttonFontOffset.x - 1, buttonFontOffset.y );
        setGlyphPosition( g[86], buttonFontOffset.x - 1, buttonFontOffset.y );
        setGlyphPosition( g[89], buttonFontOffset.x - 1, buttonFontOffset.y );

        // !
        resizeGlyph( g[33], 2 + offset * 2, 10 + offset * 2 );
        drawLine( g[33], offset + 0, offset + 0, offset + 0, offset + 5, buttonGoodReleasedColor );
        drawLine( g[33], offset + 1, offset + 0, offset + 1, offset + 5, buttonGoodReleasedColor );
        drawLine( g[33], offset + 0, offset + 8, offset + 0, offset + 9, buttonGoodReleasedColor );
        drawLine( g[33], offset + 1, offset + 8, offset + 1, offset + 9, buttonGoodReleasedColor );

        // "
        resizeGlyph( g[34], 6 + offset * 2, 10 + offset * 2 );
        drawLine( g[34], offset + 1, offset + 0, offset + 1, offset + 1, buttonGoodReleasedColor );
        drawLine( g[34], offset + 5, offset + 0, offset + 5, offset + 1, buttonGoodReleasedColor );
        setPixel( g[34], offset + 0, offset + 0, buttonGoodReleasedColor );
        setPixel( g[34], offset + 4, offset + 0, buttonGoodReleasedColor );

        // #
        resizeGlyph( g[35], 10 + offset * 2, 10 + offset * 2 );
        drawLine( g[35], offset + 1, offset + 3, offset + 9, offset + 3, buttonGoodReleasedColor );
        drawLine( g[35], offset + 0, offset + 6, offset + 8, offset + 6, buttonGoodReleasedColor );
        drawLine( g[35], offset + 4, offset + 0, offset + 2, offset + 9, buttonGoodReleasedColor );
        drawLine( g[35], offset + 7, offset + 0, offset + 5, offset + 9, buttonGoodReleasedColor );

        // %
        resizeGlyph( g[37], 9 + offset * 2, 10 + offset * 2 );
        drawLine( g[37], offset + 6, offset + 0, offset + 2, offset + 9, buttonGoodReleasedColor );
        drawLine( g[37], offset + 1, offset + 0, offset + 2, offset + 0, buttonGoodReleasedColor );
        drawLine( g[37], offset + 1, offset + 3, offset + 2, offset + 3, buttonGoodReleasedColor );
        drawLine( g[37], offset + 0, offset + 1, offset + 0, offset + 2, buttonGoodReleasedColor );
        drawLine( g[37], offset + 3, offset + 1, offset + 3, offset + 2, buttonGoodReleasedColor );
        drawLine( g[37], offset + 7, offset + 9, offset + 6, offset + 9, buttonGoodReleasedColor );
        drawLine( g[37], offset + 7, offset + 6, offset + 6, offset + 6, buttonGoodReleasedColor );
        drawLine( g[37], offset + 8, offset + 8, offset + 8, offset + 7, buttonGoodReleasedColor );
        drawLine( g[37], offset + 5, offset + 8, offset + 5, offset + 7, buttonGoodReleasedColor );

        // &
        resizeGlyph( g[38], 8 + offset * 2, 10 + offset * 2 );
        drawLine( g[38], offset + 2, offset + 0, offset + 3, offset + 0, buttonGoodReleasedColor );
        drawLine( g[38], offset + 1, offset + 1, offset + 1, offset + 2, buttonGoodReleasedColor );
        drawLine( g[38], offset + 4, offset + 1, offset + 4, offset + 2, buttonGoodReleasedColor );
        drawLine( g[38], offset + 2, offset + 3, offset + 7, offset + 9, buttonGoodReleasedColor );
        drawLine( g[38], offset + 3, offset + 3, offset + 1, offset + 5, buttonGoodReleasedColor );
        drawLine( g[38], offset + 0, offset + 6, offset + 0, offset + 8, buttonGoodReleasedColor );
        drawLine( g[38], offset + 1, offset + 9, offset + 4, offset + 9, buttonGoodReleasedColor );
        drawLine( g[38], offset + 5, offset + 8, offset + 7, offset + 5, buttonGoodReleasedColor );

        // '
        resizeGlyph( g[39], 2 + offset * 2, 10 + offset * 2 );
        drawLine( g[39], offset + 1, offset + 0, offset + 1, offset + 1, buttonGoodReleasedColor );
        setPixel( g[39], offset + 0, offset + 0, buttonGoodReleasedColor );

        // (
        resizeGlyph( g[40], 3 + offset * 2, 10 + offset * 2 );
        drawLine( g[40], offset + 0, offset + 3, offset + 0, offset + 6, buttonGoodReleasedColor );
        drawLine( g[40], offset + 1, offset + 1, offset + 1, offset + 2, buttonGoodReleasedColor );
        drawLine( g[40], offset + 1, offset + 7, offset + 1, offset + 8, buttonGoodReleasedColor );
        setPixel( g[40], offset + 2, offset + 0, buttonGoodReleasedColor );
        setPixel( g[40], offset + 2, offset + 9, buttonGoodReleasedColor );

        // )
        resizeGlyph( g[41], 3 + offset * 2, 10 + offset * 2 );
        drawLine( g[41], offset + 2, offset + 3, offset + 2, offset + 6, buttonGoodReleasedColor );
        drawLine( g[41], offset + 1, offset + 1, offset + 1, offset + 2, buttonGoodReleasedColor );
        drawLine( g[41], offset + 1, offset + 7, offset + 1, offset + 8, buttonGoodReleasedColor );
        setPixel( g[41], offset + 0, offset + 0, buttonGoodReleasedColor );
        setPixel( g[41], offset + 0, offset + 9, buttonGoodReleasedColor );

        //*
        resizeGlyph( g[42], 5 + offset * 2, 10 + offset * 2 );
        drawLine( g[42], offset + 2, offset + 0, offset + 2, offset + 5, buttonGoodReleasedColor );
        drawLine( g[42], offset + 0, offset + 1, offset + 4, offset + 4, buttonGoodReleasedColor );
        drawLine( g[42], offset + 0, offset + 4, offset + 4, offset + 1, buttonGoodReleasedColor );

        // +
        resizeGlyph( g[43], 5 + offset * 2, 10 + offset * 2 );
        drawLine( g[43], offset + 0, offset + 5, offset + 4, offset + 5, buttonGoodReleasedColor );
        drawLine( g[43], offset + 2, offset + 3, offset + 2, offset + 7, buttonGoodReleasedColor );

        // ,
        resizeGlyph( g[44], 3 + offset * 2, 11 + offset * 2 );
        drawLine( g[44], offset + 1, offset + 8, offset + 2, offset + 8, buttonGoodReleasedColor );
        drawLine( g[44], offset + 1, offset + 9, offset + 2, offset + 9, buttonGoodReleasedColor );
        drawLine( g[44], offset + 0, offset + 10, offset + 1, offset + 10, buttonGoodReleasedColor );

        // -
        resizeGlyph( g[45], 6 + offset * 2, 6 + offset * 2 );
        drawLine( g[45], offset + 0, offset + 5, offset + 5, offset + 5, buttonGoodReleasedColor );

        // .
        resizeGlyph( g[46], 2 + offset * 2, 10 + offset * 2 );
        drawLine( g[46], offset + 0, offset + 8, offset + 1, offset + 8, buttonGoodReleasedColor );
        drawLine( g[46], offset + 0, offset + 9, offset + 1, offset + 9, buttonGoodReleasedColor );

        // /
        resizeGlyph( g[47], 4 + offset * 2, 10 + offset * 2 );
        drawLine( g[47], offset + 3, offset + 0, offset + 0, offset + 9, buttonGoodReleasedColor );

        // 0
        resizeGlyph( g[48], 9 + offset * 2, 10 + offset * 2 );
        drawLine( g[48], offset + 2, offset + 0, offset + 6, offset + 0, buttonGoodReleasedColor );
        drawLine( g[48], offset + 0, offset + 2, offset + 0, offset + 7, buttonGoodReleasedColor );
        drawLine( g[48], offset + 2, offset + 9, offset + 6, offset + 9, buttonGoodReleasedColor );
        drawLine( g[48], offset + 8, offset + 2, offset + 8, offset + 7, buttonGoodReleasedColor );
        setPixel( g[48], offset + 1, offset + 1, buttonGoodReleasedColor );
        setPixel( g[48], offset + 7, offset + 1, buttonGoodReleasedColor );
        setPixel( g[48], offset + 1, offset + 8, buttonGoodReleasedColor );
        setPixel( g[48], offset + 7, offset + 8, buttonGoodReleasedColor );

        // 1
        resizeGlyph( g[49], 5 + offset * 2, 10 + offset * 2 );
        drawLine( g[49], offset + 2, offset + 0, offset + 2, offset + 9, buttonGoodReleasedColor );
        drawLine( g[49], offset + 0, offset + 9, offset + 4, offset + 9, buttonGoodReleasedColor );
        setPixel( g[49], offset + 1, offset + 1, buttonGoodReleasedColor );
        setPixel( g[49], offset + 0, offset + 2, buttonGoodReleasedColor );

        // 2
        resizeGlyph( g[50], 7 + offset * 2, 10 + offset * 2 );
        drawLine( g[50], offset + 1, offset + 0, offset + 5, offset + 0, buttonGoodReleasedColor );
        drawLine( g[50], offset + 6, offset + 1, offset + 6, offset + 3, buttonGoodReleasedColor );
        drawLine( g[50], offset + 5, offset + 4, offset + 0, offset + 9, buttonGoodReleasedColor );
        drawLine( g[50], offset + 1, offset + 9, offset + 6, offset + 9, buttonGoodReleasedColor );
        drawLine( g[50], offset + 0, offset + 1, offset + 0, offset + 2, buttonGoodReleasedColor );
        setPixel( g[50], offset + 6, offset + 8, buttonGoodReleasedColor );

        // 3
        resizeGlyph( g[51], 7 + offset * 2, 10 + offset * 2 );
        drawLine( g[51], offset + 1, offset + 0, offset + 5, offset + 0, buttonGoodReleasedColor );
        drawLine( g[51], offset + 6, offset + 1, offset + 6, offset + 3, buttonGoodReleasedColor );
        drawLine( g[51], offset + 2, offset + 4, offset + 5, offset + 4, buttonGoodReleasedColor );
        drawLine( g[51], offset + 6, offset + 5, offset + 6, offset + 8, buttonGoodReleasedColor );
        drawLine( g[51], offset + 1, offset + 9, offset + 5, offset + 9, buttonGoodReleasedColor );
        setPixel( g[51], offset + 0, offset + 1, buttonGoodReleasedColor );
        setPixel( g[51], offset + 0, offset + 8, buttonGoodReleasedColor );

        // 4
        resizeGlyph( g[52], 8 + offset * 2, 10 + offset * 2 );
        drawLine( g[52], offset + 0, offset + 4, offset + 4, offset + 0, buttonGoodReleasedColor );
        drawLine( g[52], offset + 5, offset + 0, offset + 5, offset + 4, buttonGoodReleasedColor );
        drawLine( g[52], offset + 5, offset + 6, offset + 5, offset + 8, buttonGoodReleasedColor );
        drawLine( g[52], offset + 0, offset + 5, offset + 7, offset + 5, buttonGoodReleasedColor );
        drawLine( g[52], offset + 3, offset + 9, offset + 7, offset + 9, buttonGoodReleasedColor );

        // 5
        resizeGlyph( g[53], 7 + offset * 2, 10 + offset * 2 );
        drawLine( g[53], offset + 0, offset + 0, offset + 6, offset + 0, buttonGoodReleasedColor );
        drawLine( g[53], offset + 0, offset + 1, offset + 0, offset + 3, buttonGoodReleasedColor );
        drawLine( g[53], offset + 1, offset + 4, offset + 5, offset + 4, buttonGoodReleasedColor );
        drawLine( g[53], offset + 6, offset + 5, offset + 6, offset + 8, buttonGoodReleasedColor );
        drawLine( g[53], offset + 1, offset + 9, offset + 5, offset + 9, buttonGoodReleasedColor );
        setPixel( g[53], offset + 0, offset + 8, buttonGoodReleasedColor );

        // 6
        resizeGlyph( g[54], 7 + offset * 2, 10 + offset * 2 );
        drawLine( g[54], offset + 1, offset + 0, offset + 5, offset + 0, buttonGoodReleasedColor );
        drawLine( g[54], offset + 0, offset + 1, offset + 0, offset + 8, buttonGoodReleasedColor );
        drawLine( g[54], offset + 1, offset + 4, offset + 5, offset + 4, buttonGoodReleasedColor );
        drawLine( g[54], offset + 6, offset + 5, offset + 6, offset + 8, buttonGoodReleasedColor );
        drawLine( g[54], offset + 1, offset + 9, offset + 5, offset + 9, buttonGoodReleasedColor );
        setPixel( g[54], offset + 6, offset + 1, buttonGoodReleasedColor );

        // 7
        resizeGlyph( g[55], 7 + offset * 2, 10 + offset * 2 );
        drawLine( g[55], offset + 0, offset + 0, offset + 5, offset + 0, buttonGoodReleasedColor );
        drawLine( g[55], offset + 6, offset + 0, offset + 2, offset + 9, buttonGoodReleasedColor );
        setPixel( g[55], offset + 0, offset + 1, buttonGoodReleasedColor );
        setPixel( g[55], offset + 1, offset + 9, buttonGoodReleasedColor );
        setPixel( g[55], offset + 3, offset + 9, buttonGoodReleasedColor );

        // 8
        resizeGlyph( g[56], 7 + offset * 2, 10 + offset * 2 );
        drawLine( g[56], offset + 1, offset + 0, offset + 5, offset + 0, buttonGoodReleasedColor );
        drawLine( g[56], offset + 0, offset + 1, offset + 0, offset + 3, buttonGoodReleasedColor );
        drawLine( g[56], offset + 6, offset + 1, offset + 6, offset + 3, buttonGoodReleasedColor );
        drawLine( g[56], offset + 1, offset + 4, offset + 5, offset + 4, buttonGoodReleasedColor );
        drawLine( g[56], offset + 0, offset + 5, offset + 0, offset + 8, buttonGoodReleasedColor );
        drawLine( g[56], offset + 6, offset + 5, offset + 6, offset + 8, buttonGoodReleasedColor );
        drawLine( g[56], offset + 1, offset + 9, offset + 5, offset + 9, buttonGoodReleasedColor );

        // 9
        resizeGlyph( g[57], 7 + offset * 2, 10 + offset * 2 );
        drawLine( g[57], offset + 1, offset + 0, offset + 5, offset + 0, buttonGoodReleasedColor );
        drawLine( g[57], offset + 0, offset + 1, offset + 0, offset + 4, buttonGoodReleasedColor );
        drawLine( g[57], offset + 6, offset + 1, offset + 6, offset + 8, buttonGoodReleasedColor );
        drawLine( g[57], offset + 1, offset + 5, offset + 5, offset + 5, buttonGoodReleasedColor );
        drawLine( g[57], offset + 1, offset + 9, offset + 5, offset + 9, buttonGoodReleasedColor );
        setPixel( g[57], offset + 0, offset + 8, buttonGoodReleasedColor );

        // :
        resizeGlyph( g[58], 2 + offset * 2, 10 + offset * 2 );
        drawLine( g[58], offset + 0, offset + 3, offset + 1, offset + 3, buttonGoodReleasedColor );
        drawLine( g[58], offset + 0, offset + 4, offset + 1, offset + 4, buttonGoodReleasedColor );
        drawLine( g[58], offset + 0, offset + 8, offset + 1, offset + 8, buttonGoodReleasedColor );
        drawLine( g[58], offset + 0, offset + 9, offset + 1, offset + 9, buttonGoodReleasedColor );

        // ;
        resizeGlyph( g[59], 3 + offset * 2, 11 + offset * 2 );
        drawLine( g[59], offset + 1, offset + 3, offset + 2, offset + 3, buttonGoodReleasedColor );
        drawLine( g[59], offset + 1, offset + 4, offset + 2, offset + 4, buttonGoodReleasedColor );
        drawLine( g[59], offset + 1, offset + 8, offset + 2, offset + 8, buttonGoodReleasedColor );
        drawLine( g[59], offset + 1, offset + 9, offset + 2, offset + 9, buttonGoodReleasedColor );
        drawLine( g[59], offset + 0, offset + 10, offset + 1, offset + 10, buttonGoodReleasedColor );

        // <
        resizeGlyph( g[60], 4 + offset * 2, 10 + offset * 2 );
        drawLine( g[60], offset + 3, offset + 2, offset + 0, offset + 5, buttonGoodReleasedColor );
        drawLine( g[60], offset + 3, offset + 8, offset + 1, offset + 6, buttonGoodReleasedColor );

        // =
        resizeGlyph( g[61], 6 + offset * 2, 8 + offset * 2 );
        drawLine( g[61], offset + 0, offset + 3, offset + 5, offset + 3, buttonGoodReleasedColor );
        drawLine( g[61], offset + 0, offset + 7, offset + 5, offset + 7, buttonGoodReleasedColor );

        // >
        resizeGlyph( g[62], 4 + offset * 2, 10 + offset * 2 );
        drawLine( g[62], offset + 0, offset + 2, offset + 3, offset + 5, buttonGoodReleasedColor );
        drawLine( g[62], offset + 0, offset + 8, offset + 2, offset + 6, buttonGoodReleasedColor );

        // ?
        resizeGlyph( g[63], 6 + offset * 2, 10 + offset * 2 );
        drawLine( g[63], offset + 0, offset + 1, offset + 0, offset + 2, buttonGoodReleasedColor );
        drawLine( g[63], offset + 1, offset + 0, offset + 4, offset + 0, buttonGoodReleasedColor );
        drawLine( g[63], offset + 5, offset + 1, offset + 5, offset + 2, buttonGoodReleasedColor );
        drawLine( g[63], offset + 4, offset + 3, offset + 3, offset + 5, buttonGoodReleasedColor );
        drawLine( g[63], offset + 2, offset + 8, offset + 3, offset + 8, buttonGoodReleasedColor );
        drawLine( g[63], offset + 2, offset + 9, offset + 3, offset + 9, buttonGoodReleasedColor );

        // A
        resizeGlyph( g[65], 13 + offset * 2, 10 + offset * 2 );
        drawLine( g[65], offset + 0, offset + 9, offset + 4, offset + 9, buttonGoodReleasedColor );
        drawLine( g[65], offset + 8, offset + 9, offset + 12, offset + 9, buttonGoodReleasedColor );
        drawLine( g[65], offset + 5, offset + 5, offset + 8, offset + 5, buttonGoodReleasedColor );
        drawLine( g[65], offset + 2, offset + 8, offset + 4, offset + 5, buttonGoodReleasedColor );
        drawLine( g[65], offset + 7, offset + 1, offset + 10, offset + 8, buttonGoodReleasedColor );
        setPixel( g[65], offset + 4, offset + 4, buttonGoodReleasedColor );
        setPixel( g[65], offset + 5, offset + 3, buttonGoodReleasedColor );
        setPixel( g[65], offset + 5, offset + 2, buttonGoodReleasedColor );
        setPixel( g[65], offset + 6, offset + 1, buttonGoodReleasedColor );
        setPixel( g[65], offset + 6, offset + 0, buttonGoodReleasedColor );

        // B
        resizeGlyph( g[66], 11 + offset * 2, 10 + offset * 2 );
        drawLine( g[66], offset + 0, offset + 0, offset + 8, offset + 0, buttonGoodReleasedColor );
        drawLine( g[66], offset + 0, offset + 9, offset + 8, offset + 9, buttonGoodReleasedColor );
        drawLine( g[66], offset + 3, offset + 5, offset + 9, offset + 5, buttonGoodReleasedColor );
        drawLine( g[66], offset + 2, offset + 1, offset + 2, offset + 8, buttonGoodReleasedColor );
        drawLine( g[66], offset + 10, offset + 2, offset + 10, offset + 4, buttonGoodReleasedColor );
        drawLine( g[66], offset + 10, offset + 6, offset + 10, offset + 7, buttonGoodReleasedColor );
        setPixel( g[66], offset + 9, offset + 8, buttonGoodReleasedColor );
        setPixel( g[66], offset + 9, offset + 1, buttonGoodReleasedColor );

        // C
        resizeGlyph( g[67], 10 + offset * 2, 10 + offset * 2 );
        drawLine( g[67], offset + 2, offset + 0, offset + 7, offset + 0, buttonGoodReleasedColor );
        drawLine( g[67], offset + 0, offset + 2, offset + 0, offset + 7, buttonGoodReleasedColor );
        drawLine( g[67], offset + 2, offset + 9, offset + 7, offset + 9, buttonGoodReleasedColor );
        drawLine( g[67], offset + 9, offset + 0, offset + 9, offset + 2, buttonGoodReleasedColor );
        setPixel( g[67], offset + 1, offset + 1, buttonGoodReleasedColor );
        setPixel( g[67], offset + 1, offset + 8, buttonGoodReleasedColor );
        setPixel( g[67], offset + 8, offset + 1, buttonGoodReleasedColor );
        setPixel( g[67], offset + 8, offset + 8, buttonGoodReleasedColor );
        setPixel( g[67], offset + 9, offset + 7, buttonGoodReleasedColor );

        // D
        resizeGlyph( g[68], 11 + offset * 2, 10 + offset * 2 );
        drawLine( g[68], offset + 0, offset + 0, offset + 8, offset + 0, buttonGoodReleasedColor );
        drawLine( g[68], offset + 0, offset + 9, offset + 8, offset + 9, buttonGoodReleasedColor );
        drawLine( g[68], offset + 2, offset + 1, offset + 2, offset + 8, buttonGoodReleasedColor );
        drawLine( g[68], offset + 10, offset + 2, offset + 10, offset + 7, buttonGoodReleasedColor );
        setPixel( g[68], offset + 9, offset + 1, buttonGoodReleasedColor );
        setPixel( g[68], offset + 9, offset + 8, buttonGoodReleasedColor );

        // E
        resizeGlyph( g[69], 9 + offset * 2, 10 + offset * 2 );
        drawLine( g[69], offset + 0, offset + 0, offset + 8, offset + 0, buttonGoodReleasedColor );
        drawLine( g[69], offset + 0, offset + 9, offset + 8, offset + 9, buttonGoodReleasedColor );
        drawLine( g[69], offset + 2, offset + 1, offset + 2, offset + 8, buttonGoodReleasedColor );
        drawLine( g[69], offset + 3, offset + 4, offset + 6, offset + 4, buttonGoodReleasedColor );
        setPixel( g[69], offset + 8, offset + 1, buttonGoodReleasedColor );
        setPixel( g[69], offset + 8, offset + 8, buttonGoodReleasedColor );
        setPixel( g[69], offset + 6, offset + 3, buttonGoodReleasedColor );
        setPixel( g[69], offset + 6, offset + 5, buttonGoodReleasedColor );

        // F
        resizeGlyph( g[70], 9 + offset * 2, 10 + offset * 2 );
        drawLine( g[70], offset + 0, offset + 0, offset + 8, offset + 0, buttonGoodReleasedColor );
        drawLine( g[70], offset + 0, offset + 9, offset + 3, offset + 9, buttonGoodReleasedColor );
        drawLine( g[70], offset + 2, offset + 1, offset + 2, offset + 8, buttonGoodReleasedColor );
        drawLine( g[70], offset + 3, offset + 4, offset + 6, offset + 4, buttonGoodReleasedColor );
        setPixel( g[70], offset + 8, offset + 1, buttonGoodReleasedColor );
        setPixel( g[70], offset + 6, offset + 3, buttonGoodReleasedColor );
        setPixel( g[70], offset + 6, offset + 5, buttonGoodReleasedColor );

        // G
        resizeGlyph( g[71], 11 + offset * 2, 10 + offset * 2 );
        drawLine( g[71], offset + 2, offset + 0, offset + 7, offset + 0, buttonGoodReleasedColor );
        drawLine( g[71], offset + 2, offset + 9, offset + 7, offset + 9, buttonGoodReleasedColor );
        drawLine( g[71], offset + 0, offset + 2, offset + 0, offset + 7, buttonGoodReleasedColor );
        drawLine( g[71], offset + 7, offset + 5, offset + 10, offset + 5, buttonGoodReleasedColor );
        drawLine( g[71], offset + 9, offset + 0, offset + 9, offset + 2, buttonGoodReleasedColor );
        drawLine( g[71], offset + 9, offset + 6, offset + 9, offset + 7, buttonGoodReleasedColor );
        setPixel( g[71], offset + 1, offset + 1, buttonGoodReleasedColor );
        setPixel( g[71], offset + 1, offset + 8, buttonGoodReleasedColor );
        setPixel( g[71], offset + 8, offset + 1, buttonGoodReleasedColor );
        setPixel( g[71], offset + 8, offset + 8, buttonGoodReleasedColor );

        // H
        resizeGlyph( g[72], 13 + offset * 2, 10 + offset * 2 );
        drawLine( g[72], offset + 0, offset + 0, offset + 4, offset + 0, buttonGoodReleasedColor );
        drawLine( g[72], offset + 0, offset + 9, offset + 4, offset + 9, buttonGoodReleasedColor );
        drawLine( g[72], offset + 2, offset + 1, offset + 2, offset + 8, buttonGoodReleasedColor );
        drawLine( g[72], offset + 8, offset + 0, offset + 12, offset + 0, buttonGoodReleasedColor );
        drawLine( g[72], offset + 8, offset + 9, offset + 12, offset + 9, buttonGoodReleasedColor );
        drawLine( g[72], offset + 10, offset + 1, offset + 10, offset + 8, buttonGoodReleasedColor );
        drawLine( g[72], offset + 3, offset + 5, offset + 9, offset + 5, buttonGoodReleasedColor );

        // I
        resizeGlyph( g[73], 5 + offset * 2, 10 + offset * 2 );
        drawLine( g[73], offset + 0, offset + 0, offset + 4, offset + 0, buttonGoodReleasedColor );
        drawLine( g[73], offset + 0, offset + 9, offset + 4, offset + 9, buttonGoodReleasedColor );
        drawLine( g[73], offset + 2, offset + 1, offset + 2, offset + 8, buttonGoodReleasedColor );

        // J
        resizeGlyph( g[74], 8 + offset * 2, 10 + offset * 2 );
        drawLine( g[74], offset + 3, offset + 0, offset + 7, offset + 0, buttonGoodReleasedColor );
        drawLine( g[74], offset + 1, offset + 9, offset + 4, offset + 9, buttonGoodReleasedColor );
        drawLine( g[74], offset + 5, offset + 1, offset + 5, offset + 8, buttonGoodReleasedColor );
        drawLine( g[74], offset + 0, offset + 7, offset + 0, offset + 8, buttonGoodReleasedColor );

        // K
        resizeGlyph( g[75], 12 + offset * 2, 10 + offset * 2 );
        drawLine( g[75], offset + 0, offset + 0, offset + 4, offset + 0, buttonGoodReleasedColor );
        drawLine( g[75], offset + 0, offset + 9, offset + 4, offset + 9, buttonGoodReleasedColor );
        drawLine( g[75], offset + 2, offset + 1, offset + 2, offset + 8, buttonGoodReleasedColor );
        drawLine( g[75], offset + 3, offset + 4, offset + 5, offset + 4, buttonGoodReleasedColor );
        drawLine( g[75], offset + 6, offset + 3, offset + 8, offset + 1, buttonGoodReleasedColor );
        drawLine( g[75], offset + 6, offset + 5, offset + 9, offset + 8, buttonGoodReleasedColor );
        drawLine( g[75], offset + 7, offset + 0, offset + 10, offset + 0, buttonGoodReleasedColor );
        drawLine( g[75], offset + 8, offset + 9, offset + 11, offset + 9, buttonGoodReleasedColor );

        // L
        resizeGlyph( g[76], 9 + offset * 2, 10 + offset * 2 );
        drawLine( g[76], offset + 0, offset + 0, offset + 4, offset + 0, buttonGoodReleasedColor );
        drawLine( g[76], offset + 0, offset + 9, offset + 8, offset + 9, buttonGoodReleasedColor );
        drawLine( g[76], offset + 2, offset + 1, offset + 2, offset + 8, buttonGoodReleasedColor );
        setPixel( g[76], offset + 8, offset + 8, buttonGoodReleasedColor );

        // M
        resizeGlyph( g[77], 15 + offset * 2, 10 + offset * 2 );
        drawLine( g[77], offset + 0, offset + 0, offset + 2, offset + 0, buttonGoodReleasedColor );
        drawLine( g[77], offset + 2, offset + 0, offset + 2, offset + 8, buttonGoodReleasedColor );
        drawLine( g[77], offset + 0, offset + 9, offset + 4, offset + 9, buttonGoodReleasedColor );
        drawLine( g[77], offset + 3, offset + 1, offset + 7, offset + 5, buttonGoodReleasedColor );
        drawLine( g[77], offset + 8, offset + 4, offset + 11, offset + 1, buttonGoodReleasedColor );
        drawLine( g[77], offset + 12, offset + 1, offset + 12, offset + 8, buttonGoodReleasedColor );
        drawLine( g[77], offset + 12, offset + 0, offset + 14, offset + 0, buttonGoodReleasedColor );
        drawLine( g[77], offset + 10, offset + 9, offset + 14, offset + 9, buttonGoodReleasedColor );

        // N
        resizeGlyph( g[78], 14 + offset * 2, 10 + offset * 2 );
        drawLine( g[78], offset + 0, offset + 0, offset + 1, offset + 0, buttonGoodReleasedColor );
        drawLine( g[78], offset + 2, offset + 0, offset + 2, offset + 8, buttonGoodReleasedColor );
        drawLine( g[78], offset + 0, offset + 9, offset + 4, offset + 9, buttonGoodReleasedColor );
        drawLine( g[78], offset + 3, offset + 1, offset + 10, offset + 8, buttonGoodReleasedColor );
        drawLine( g[78], offset + 9, offset + 0, offset + 13, offset + 0, buttonGoodReleasedColor );
        drawLine( g[78], offset + 11, offset + 0, offset + 11, offset + 9, buttonGoodReleasedColor );

        // O
        resizeGlyph( g[79], 10 + offset * 2, 10 + offset * 2 );
        drawLine( g[79], offset + 2, offset + 0, offset + 7, offset + 0, buttonGoodReleasedColor );
        drawLine( g[79], offset + 0, offset + 2, offset + 0, offset + 7, buttonGoodReleasedColor );
        drawLine( g[79], offset + 2, offset + 9, offset + 7, offset + 9, buttonGoodReleasedColor );
        drawLine( g[79], offset + 9, offset + 2, offset + 9, offset + 7, buttonGoodReleasedColor );
        setPixel( g[79], offset + 1, offset + 1, buttonGoodReleasedColor );
        setPixel( g[79], offset + 8, offset + 1, buttonGoodReleasedColor );
        setPixel( g[79], offset + 1, offset + 8, buttonGoodReleasedColor );
        setPixel( g[79], offset + 8, offset + 8, buttonGoodReleasedColor );

        // P
        resizeGlyph( g[80], 11 + offset * 2, 10 + offset * 2 );
        drawLine( g[80], offset + 0, offset + 0, offset + 9, offset + 0, buttonGoodReleasedColor );
        drawLine( g[80], offset + 0, offset + 9, offset + 4, offset + 9, buttonGoodReleasedColor );
        drawLine( g[80], offset + 3, offset + 5, offset + 9, offset + 5, buttonGoodReleasedColor );
        drawLine( g[80], offset + 2, offset + 1, offset + 2, offset + 8, buttonGoodReleasedColor );
        drawLine( g[80], offset + 10, offset + 1, offset + 10, offset + 4, buttonGoodReleasedColor );

        // Q
        resizeGlyph( g[81], 11 + offset * 2, 11 + offset * 2 );
        drawLine( g[81], offset + 2, offset + 0, offset + 7, offset + 0, buttonGoodReleasedColor );
        drawLine( g[81], offset + 0, offset + 2, offset + 0, offset + 7, buttonGoodReleasedColor );
        drawLine( g[81], offset + 2, offset + 9, offset + 7, offset + 9, buttonGoodReleasedColor );
        drawLine( g[81], offset + 9, offset + 2, offset + 9, offset + 7, buttonGoodReleasedColor );
        drawLine( g[81], offset + 4, offset + 7, offset + 5, offset + 7, buttonGoodReleasedColor );
        drawLine( g[81], offset + 6, offset + 7, offset + 9, offset + 10, buttonGoodReleasedColor );
        drawLine( g[81], offset + 10, offset + 10, offset + 11, offset + 10, buttonGoodReleasedColor );
        setPixel( g[81], offset + 1, offset + 1, buttonGoodReleasedColor );
        setPixel( g[81], offset + 8, offset + 1, buttonGoodReleasedColor );
        setPixel( g[81], offset + 1, offset + 8, buttonGoodReleasedColor );
        setPixel( g[81], offset + 8, offset + 8, buttonGoodReleasedColor );
        setPixel( g[81], offset + 12, offset + 9, buttonGoodReleasedColor );

        // R
        resizeGlyph( g[82], 12 + offset * 2, 10 + offset * 2 );
        drawLine( g[82], offset + 0, offset + 0, offset + 9, offset + 0, buttonGoodReleasedColor );
        drawLine( g[82], offset + 0, offset + 9, offset + 4, offset + 9, buttonGoodReleasedColor );
        drawLine( g[82], offset + 8, offset + 9, offset + 11, offset + 9, buttonGoodReleasedColor );
        drawLine( g[82], offset + 3, offset + 5, offset + 9, offset + 5, buttonGoodReleasedColor );
        drawLine( g[82], offset + 2, offset + 1, offset + 2, offset + 8, buttonGoodReleasedColor );
        drawLine( g[82], offset + 10, offset + 1, offset + 10, offset + 4, buttonGoodReleasedColor );
        drawLine( g[82], offset + 7, offset + 6, offset + 9, offset + 8, buttonGoodReleasedColor );

        // S
        resizeGlyph( g[83], 9 + offset * 2, 10 + offset * 2 );
        drawLine( g[83], offset + 1, offset + 0, offset + 7, offset + 0, buttonGoodReleasedColor );
        drawLine( g[83], offset + 0, offset + 1, offset + 0, offset + 3, buttonGoodReleasedColor );
        drawLine( g[83], offset + 1, offset + 4, offset + 7, offset + 4, buttonGoodReleasedColor );
        drawLine( g[83], offset + 8, offset + 5, offset + 8, offset + 8, buttonGoodReleasedColor );
        drawLine( g[83], offset + 1, offset + 9, offset + 7, offset + 9, buttonGoodReleasedColor );
        drawLine( g[83], offset + 0, offset + 8, offset + 1, offset + 8, buttonGoodReleasedColor );
        setPixel( g[83], offset + 8, offset + 1, buttonGoodReleasedColor );

        // T
        resizeGlyph( g[84], 11 + offset * 2, 10 + offset * 2 );
        drawLine( g[84], offset + 0, offset + 0, offset + 10, offset + 0, buttonGoodReleasedColor );
        drawLine( g[84], offset + 5, offset + 1, offset + 5, offset + 8, buttonGoodReleasedColor );
        drawLine( g[84], offset + 0, offset + 1, offset + 0, offset + 2, buttonGoodReleasedColor );
        drawLine( g[84], offset + 10, offset + 1, offset + 10, offset + 2, buttonGoodReleasedColor );
        drawLine( g[84], offset + 4, offset + 9, offset + 6, offset + 9, buttonGoodReleasedColor );

        // U
        resizeGlyph( g[85], 13 + offset * 2, 10 + offset * 2 );
        drawLine( g[85], offset + 0, offset + 0, offset + 4, offset + 0, buttonGoodReleasedColor );
        drawLine( g[85], offset + 8, offset + 0, offset + 12, offset + 0, buttonGoodReleasedColor );
        drawLine( g[85], offset + 2, offset + 1, offset + 2, offset + 7, buttonGoodReleasedColor );
        drawLine( g[85], offset + 10, offset + 1, offset + 10, offset + 7, buttonGoodReleasedColor );
        drawLine( g[85], offset + 4, offset + 9, offset + 8, offset + 9, buttonGoodReleasedColor );
        setPixel( g[85], offset + 3, offset + 8, buttonGoodReleasedColor );
        setPixel( g[85], offset + 9, offset + 8, buttonGoodReleasedColor );

        // V
        resizeGlyph( g[86], 11 + offset * 2, 10 + offset * 2 );
        drawLine( g[86], offset + 0, offset + 0, offset + 4, offset + 0, buttonGoodReleasedColor );
        drawLine( g[86], offset + 6, offset + 0, offset + 10, offset + 0, buttonGoodReleasedColor );
        drawLine( g[86], offset + 2, offset + 1, offset + 5, offset + 9, buttonGoodReleasedColor );
        drawLine( g[86], offset + 8, offset + 1, offset + 6, offset + 7, buttonGoodReleasedColor );

        // W
        resizeGlyph( g[87], 17 + offset * 2, 10 + offset * 2 );
        drawLine( g[87], offset + 0, offset + 0, offset + 4, offset + 0, buttonGoodReleasedColor );
        drawLine( g[87], offset + 7, offset + 0, offset + 9, offset + 0, buttonGoodReleasedColor );
        drawLine( g[87], offset + 12, offset + 0, offset + 16, offset + 0, buttonGoodReleasedColor );
        drawLine( g[87], offset + 2, offset + 1, offset + 5, offset + 9, buttonGoodReleasedColor );
        drawLine( g[87], offset + 8, offset + 1, offset + 6, offset + 7, buttonGoodReleasedColor );
        drawLine( g[87], offset + 9, offset + 3, offset + 10, offset + 7, buttonGoodReleasedColor );
        drawLine( g[87], offset + 14, offset + 1, offset + 11, offset + 9, buttonGoodReleasedColor );

        // X
        resizeGlyph( g[88], 12 + offset * 2, 10 + offset * 2 );
        drawLine( g[88], offset + 0, offset + 0, offset + 3, offset + 0, buttonGoodReleasedColor );
        drawLine( g[88], offset + 8, offset + 0, offset + 11, offset + 0, buttonGoodReleasedColor );
        drawLine( g[88], offset + 0, offset + 9, offset + 3, offset + 9, buttonGoodReleasedColor );
        drawLine( g[88], offset + 8, offset + 9, offset + 11, offset + 9, buttonGoodReleasedColor );
        drawLine( g[88], offset + 2, offset + 1, offset + 9, offset + 8, buttonGoodReleasedColor );
        drawLine( g[88], offset + 2, offset + 8, offset + 9, offset + 1, buttonGoodReleasedColor );

        // Y
        resizeGlyph( g[89], 11 + offset * 2, 10 + offset * 2 );
        drawLine( g[89], offset + 0, offset + 0, offset + 3, offset + 0, buttonGoodReleasedColor );
        drawLine( g[89], offset + 7, offset + 0, offset + 10, offset + 0, buttonGoodReleasedColor );
        drawLine( g[89], offset + 2, offset + 1, offset + 4, offset + 3, buttonGoodReleasedColor );
        drawLine( g[89], offset + 6, offset + 3, offset + 8, offset + 1, buttonGoodReleasedColor );
        drawLine( g[89], offset + 5, offset + 4, offset + 5, offset + 8, buttonGoodReleasedColor );
        drawLine( g[89], offset + 3, offset + 9, offset + 7, offset + 9, buttonGoodReleasedColor );

        // Z
        resizeGlyph( g[90], 9 + offset * 2, 10 + offset * 2 );
        drawLine( g[90], offset + 0, offset + 0, offset + 8, offset + 0, buttonGoodReleasedColor );
        drawLine( g[90], offset + 0, offset + 9, offset + 8, offset + 9, buttonGoodReleasedColor );
        drawLine( g[90], offset + 7, offset + 1, offset + 0, offset + 8, buttonGoodReleasedColor );
        setPixel( g[90], offset + 0, offset + 1, buttonGoodReleasedColor );
        setPixel( g[90], offset + 8, offset + 8, buttonGoodReleasedColor );

        // [
        resizeGlyph( g[91], 4 + offset * 2, 10 + offset * 2 );
        drawLine( g[91], offset + 0, offset + 0, offset + 0, offset + 9, buttonGoodReleasedColor );
        drawLine( g[91], offset + 1, offset + 0, offset + 3, offset + 0, buttonGoodReleasedColor );
        drawLine( g[91], offset + 1, offset + 9, offset + 3, offset + 9, buttonGoodReleasedColor );

        /* \ */
        resizeGlyph( g[92], 4 + offset * 2, 10 + offset * 2 );
        drawLine( g[92], offset + 0, offset + 0, offset + 3, offset + 9, buttonGoodReleasedColor );

        // ]
        resizeGlyph( g[93], 4 + offset * 2, 10 + offset * 2 );
        drawLine( g[93], offset + 3, offset + 0, offset + 3, offset + 9, buttonGoodReleasedColor );
        drawLine( g[93], offset + 0, offset + 0, offset + 2, offset + 0, buttonGoodReleasedColor );
        drawLine( g[93], offset + 0, offset + 9, offset + 2, offset + 9, buttonGoodReleasedColor );

        // ^
        resizeGlyph( g[94], 5 + offset * 2, 3 + offset * 2 );
        drawLine( g[94], offset + 0, offset + 2, offset + 2, offset + 0, buttonGoodReleasedColor );
        drawLine( g[94], offset + 3, offset + 1, offset + 4, offset + 2, buttonGoodReleasedColor );

        // _
        resizeGlyph( g[95], 8 + offset * 2, 11 + offset * 2 );
        drawLine( g[95], offset + 0, offset + 10, offset + 7, offset + 10, buttonGoodReleasedColor );

        // | - replaced with Caps Lock symbol for virtual keyboard
        // TODO: put the Caps Lock symbol to a special font to not replace any other ASCII character.
        resizeGlyph( g[124], 11 + offset * 2, 11 + offset * 2 );
        setPixel( g[124], offset + 5, offset + 0, buttonGoodReleasedColor );
        drawLine( g[124], offset + 4, offset + 1, offset + 6, offset + 1, buttonGoodReleasedColor );
        drawLine( g[124], offset + 3, offset + 2, offset + 7, offset + 2, buttonGoodReleasedColor );
        drawLine( g[124], offset + 2, offset + 3, offset + 8, offset + 3, buttonGoodReleasedColor );
        drawLine( g[124], offset + 1, offset + 4, offset + 9, offset + 4, buttonGoodReleasedColor );
        drawLine( g[124], offset + 0, offset + 5, offset + 10, offset + 5, buttonGoodReleasedColor );
        drawLine( g[124], offset + 3, offset + 6, offset + 7, offset + 6, buttonGoodReleasedColor );
        drawLine( g[124], offset + 3, offset + 7, offset + 7, offset + 7, buttonGoodReleasedColor );
        drawLine( g[124], offset + 3, offset + 9, offset + 7, offset + 9, buttonGoodReleasedColor );
        drawLine( g[124], offset + 3, offset + 10, offset + 7, offset + 10, buttonGoodReleasedColor );

        // ~ - replaced with Backspace symbol (<x]) for virtual keyboard
        // TODO: put the Backspace symbol to a special font to not replace any other ASCII character.
        resizeGlyph( g[126], 17 + offset * 2, 9 + offset * 2 );
        setPixel( g[126], offset + 0, offset + 4, buttonGoodReleasedColor );
        drawLine( g[126], offset + 1, offset + 3, offset + 1, offset + 5, buttonGoodReleasedColor );
        drawLine( g[126], offset + 2, offset + 2, offset + 2, offset + 6, buttonGoodReleasedColor );
        drawLine( g[126], offset + 3, offset + 1, offset + 3, offset + 7, buttonGoodReleasedColor );
        fillRect( g[126], offset + 4, offset + 0, 13, 9, buttonGoodReleasedColor );
        drawLine( g[126], offset + 6, offset + 1, offset + 12, offset + 7, 12 );
        drawLine( g[126], offset + 7, offset + 1, offset + 13, offset + 7, 12 );
        drawLine( g[126], offset + 12, offset + 1, offset + 6, offset + 7, 12 );
        drawLine( g[126], offset + 13, offset + 1, offset + 7, offset + 7, 12 );

        // Replaced with Change Language symbol for virtual keyboard
        // TODO: put the Change Language symbol to a special font to not replace any other ASCII character.
        resizeGlyph( g[127], 14 + offset * 2, 13 + offset * 2 );
        setGlyphPosition( g[127], buttonFontOffset.x, buttonFontOffset.y - 2 );
        drawLine( g[127], offset + 4, offset + 0, offset + 9, offset + 0, buttonGoodReleasedColor );
        drawLine( g[127], offset + 1, offset + 2, offset + 2, offset + 1, buttonGoodReleasedColor );
        drawLine( g[127], offset + 1, offset + 3, offset + 3, offset + 1, buttonGoodReleasedColor );
        drawLine( g[127], offset + 10, offset + 1, offset + 12, offset + 3, buttonGoodReleasedColor );
        drawLine( g[127], offset + 11, offset + 1, offset + 12, offset + 2, buttonGoodReleasedColor );
        drawLine( g[127], offset + 0, offset + 4, offset + 13, offset + 4, buttonGoodReleasedColor );
        drawLine( g[127], offset + 0, offset + 8, offset + 13, offset + 8, buttonGoodReleasedColor );
        drawLine( g[127], offset + 1, offset + 9, offset + 3, offset + 11, buttonGoodReleasedColor );
        drawLine( g[127], offset + 1, offset + 10, offset + 2, offset + 11, buttonGoodReleasedColor );
        drawLine( g[127], offset + 4, offset + 12, offset + 9, offset + 12, buttonGoodReleasedColor );
        drawLine( g[127], offset + 10, offset + 11, offset + 12, offset + 9, buttonGoodReleasedColor );
        drawLine( g[127], offset + 11, offset + 11, offset + 12, offset + 10, buttonGoodReleasedColor );
        drawLine( g[127], offset + 4, offset + 3, offset + 4, offset + 9, buttonGoodReleasedColor );
        drawLine( g[127], offset + 9, offset + 3, offset + 9, offset + 9, buttonGoodReleasedColor );
        drawLine( g[127], offset + 5, offset + 1, offset + 5, offset + 2, buttonGoodReleasedColor );
        drawLine( g[127], offset + 8, offset + 1, offset + 8, offset + 2, buttonGoodReleasedColor );
        drawLine( g[127], offset + 5, offset + 10, offset + 5, offset + 11, buttonGoodReleasedColor );
        drawLine( g[127], offset + 8, offset + 10, offset + 8, offset + 11, buttonGoodReleasedColor );
        drawLine( g[127], offset + 0, offset + 5, offset + 0, offset + 7, buttonGoodReleasedColor );
        drawLine( g[127], offset + 13, offset + 5, offset + 13, offset + 7, buttonGoodReleasedColor );
}

// Port of fheroes2::Copy (image.cpp): copies both color and transform without blending.
void copyGlyphPart( const ButtonGlyph & src, int inX, int inY, ButtonGlyph & out, int outX, int outY, int width, int height )
{
    for ( int y = 0; y < height; ++y ) {
        const int sy = inY + y;
        const int dy = outY + y;
        if ( sy < 0 || sy >= src.height || dy < 0 || dy >= out.height )
            continue;
        for ( int x = 0; x < width; ++x ) {
            const int sx = inX + x;
            const int dx = outX + x;
            if ( sx < 0 || sx >= src.width || dx < 0 || dx >= out.width )
                continue;
            const size_t srcOffset = static_cast<size_t>( sy ) * src.width + sx;
            const size_t dstOffset = static_cast<size_t>( dy ) * out.width + dx;
            out.image[dstOffset] = src.image[srcOffset];
            out.transform[dstOffset] = src.transform[srcOffset];
        }
    }
}

// Port of fheroes2::SetTransformPixel: changes only the transform plane.
void setTransformPixel( ButtonGlyph & g, int x, int y, uint8_t transformValue )
{
    if ( x < 0 || y < 0 || x >= g.width || y >= g.height )
        return;
    g.transform[static_cast<size_t>( y ) * g.width + x] = transformValue;
}

// Port of fheroes2::FillTransform: fills a rectangle with a transform value.
void fillTransform( ButtonGlyph & g, int x, int y, int width, int height, uint8_t transformValue )
{
    for ( int yy = y; yy < y + height; ++yy ) {
        for ( int xx = x; xx < x + width; ++xx )
            setTransformPixel( g, xx, yy, transformValue );
    }
}

// Port of fheroes2::Blit( src, dst, flip = true ): full mirrored copy.
void flipGlyphHorizontal( const ButtonGlyph & src, ButtonGlyph & dst )
{
    dst.width = src.width;
    dst.height = src.height;
    dst.x = src.x;
    dst.y = src.y;
    dst.image.assign( src.image.size(), 0 );
    dst.transform.assign( src.transform.size(), 1 );
    for ( int y = 0; y < src.height; ++y ) {
        for ( int x = 0; x < src.width; ++x ) {
            const size_t srcOffset = static_cast<size_t>( y ) * src.width + x;
            const size_t dstOffset = static_cast<size_t>( y ) * src.width + ( src.width - 1 - x );
            dst.image[dstOffset] = src.image[srcOffset];
            dst.transform[dstOffset] = src.transform[srcOffset];
        }
    }
}

// Port of fheroes2::Flip: copies a region with mirroring along the axes.
void flipRegion( const ButtonGlyph & src, int inX, int inY, ButtonGlyph & out, int outX, int outY, int width, int height, bool horizontal, bool vertical )
{
    for ( int y = 0; y < height; ++y ) {
        const int sy = inY + y;
        const int dy = outY + y;
        if ( sy < 0 || sy >= src.height || dy < 0 || dy >= out.height )
            continue;
        for ( int x = 0; x < width; ++x ) {
            const int sx = horizontal ? inX + ( width - 1 - x ) : inX + x;
            const int dx = outX + x;
            if ( sx < 0 || sx >= src.width || dx < 0 || dx >= out.width )
                continue;
            const size_t srcOffset = static_cast<size_t>( ( vertical ? src.height - 1 - sy : sy ) ) * src.width + sx;
            const size_t dstOffset = static_cast<size_t>( dy ) * out.width + dx;
            out.image[dstOffset] = src.image[srcOffset];
            out.transform[dstOffset] = src.transform[srcOffset];
        }
    }
}

// Port of generateCP1251GoodButtonFont (ui_font.cpp:5884): Cyrillic shapes
// of the button font. The code was ported from the engine mechanically.
void generateCP1251GoodButtonFont( std::array<ButtonGlyph, 256> & g )
{
    // We need 2 pixels from all sides of a letter to add extra effects.
    constexpr int offset = 2;

    // Offset symbols that either have diacritics or need less space to neighboring symbols.
    setGlyphPosition( g[141], buttonFontOffset.x, buttonFontOffset.y - 3 );
    setGlyphPosition( g[161], buttonFontOffset.x, buttonFontOffset.y - 3 );
    setGlyphPosition( g[165], buttonFontOffset.x, buttonFontOffset.y - 2 );
    setGlyphPosition( g[168], buttonFontOffset.x, buttonFontOffset.y - 3 );
    setGlyphPosition( g[175], buttonFontOffset.x, buttonFontOffset.y - 3 );
    setGlyphPosition( g[192], buttonFontOffset.x - 1, buttonFontOffset.y );
    setGlyphPosition( g[201], buttonFontOffset.x, buttonFontOffset.y - 3 );
    setGlyphPosition( g[218], buttonFontOffset.x - 1, buttonFontOffset.y );

    // K with acute, Cyrillic KJE. Needs to have upper right arm adjusted.
    resizeGlyph( g[141], g[75].width, g[75].height + 4 );
    copyGlyphPart( g[75], offset + 2, offset + 1, g[141], offset + 1, offset + 4, 7, g[75].height - offset * 2 - 2 );
    drawLine( g[141], offset + 0, offset + 3, offset + 2, offset + 3, buttonGoodReleasedColor );
    drawLine( g[141], offset + 0, offset + 12, offset + 2, offset + 12, buttonGoodReleasedColor );
    drawLine( g[141], offset + 6, offset + 3, offset + 8, offset + 3, buttonGoodReleasedColor );
    drawLine( g[141], offset + 6, offset + 12, offset + 8, offset + 12, buttonGoodReleasedColor );
    setPixel( g[141], offset + 7, offset + 11, buttonGoodReleasedColor );
    drawLine( g[141], offset + 5, offset + 1, offset + 6, offset + 0, buttonGoodReleasedColor );

    // ' (right single quotation mark)
    resizeGlyph( g[146], 3 + offset * 2, 4 + offset * 2 );
    drawLine( g[146], offset + 1, offset + 0, offset + 1, offset + 2, buttonGoodReleasedColor );
    drawLine( g[146], offset + 2, offset + 0, offset + 2, offset + 1, buttonGoodReleasedColor );
    setPixel( g[146], offset + 0, offset + 3, buttonGoodReleasedColor );

    // NBSP character.
    resizeGlyph( g[160], 8, 1 );

    // y with breve.
    resizeGlyph( g[161], 9 + offset * 2, 13 + offset * 2 );
    drawLine( g[161], offset + 0, offset + 3, offset + 2, offset + 3, buttonGoodReleasedColor );
    drawLine( g[161], offset + 6, offset + 3, offset + 8, offset + 3, buttonGoodReleasedColor );
    drawLine( g[161], offset + 3, offset + 8, offset + 1, offset + 4, buttonGoodReleasedColor );
    drawLine( g[161], offset + 5, offset + 8, offset + 7, offset + 4, buttonGoodReleasedColor );
    drawLine( g[161], offset + 4, offset + 8, offset + 3, offset + 11, buttonGoodReleasedColor );
    drawLine( g[161], offset + 0, offset + 12, offset + 2, offset + 12, buttonGoodReleasedColor );
    setPixel( g[161], offset + 0, offset + 11, buttonGoodReleasedColor );
    drawLine( g[161], offset + 3, offset + 1, offset + 5, offset + 1, buttonGoodReleasedColor );
    setPixel( g[161], offset + 2, offset + 0, buttonGoodReleasedColor );
    setPixel( g[161], offset + 6, offset + 0, buttonGoodReleasedColor );

    // J
    g[163] = g[74];

    // GHE with upturn.
    resizeGlyph( g[165], 8 + offset * 2, 12 + offset * 2 );
    drawLine( g[165], offset + 0, offset + 2, offset + 7, offset + 2, buttonGoodReleasedColor );
    drawLine( g[165], offset + 7, offset + 0, offset + 7, offset + 1, buttonGoodReleasedColor );
    drawLine( g[165], offset + 0, offset + 11, offset + 2, offset + 11, buttonGoodReleasedColor );
    drawLine( g[165], offset + 1, offset + 3, offset + 1, offset + 10, buttonGoodReleasedColor );

    // E with two dots above.
    resizeGlyph( g[168], g[69].width - 1, g[69].height + 3 );
    copyGlyphPart( g[69], offset + 1, offset + 0, g[168], offset + 0, offset + 3, g[69].width - 1 - offset * 2, g[69].height - offset * 2 );
    setPixel( g[168], offset + 3, offset + 1, buttonGoodReleasedColor );
    setPixel( g[168], offset + 6, offset + 1, buttonGoodReleasedColor );

    // Ukrainian IE (index 170) is made after the letter with index 221.

    // I with two dots above, Cyrillic YI
    resizeGlyph( g[175], g[73].width, g[73].height + 3 );
    copyGlyphPart( g[73], 0, 0, g[175], 0, 3, g[73].width, g[73].height );
    setPixel( g[175], offset + 1, offset + 1, buttonGoodReleasedColor );
    setPixel( g[175], offset + 4, offset + 1, buttonGoodReleasedColor );

    // I, Belarusian-Ukrainian I
    g[178] = g[73];

    // S
    resizeGlyph( g[189], g[83].width - 1, g[83].height );
    copyGlyphPart( g[83], 0, 0, g[189], 0, 0, 8, g[83].height );
    copyGlyphPart( g[83], 9, 0, g[189], 8, 0, g[83].width - 9, g[83].height );

    // A
    resizeGlyph( g[192], 10 + offset * 2, 10 + offset * 2 );
    drawLine( g[192], offset + 1, offset + 9, offset + 4, offset + 0, buttonGoodReleasedColor );
    drawLine( g[192], offset + 5, offset + 0, offset + 8, offset + 9, buttonGoodReleasedColor );
    drawLine( g[192], offset + 3, offset + 0, offset + 5, offset + 0, buttonGoodReleasedColor );
    drawLine( g[192], offset + 0, offset + 9, offset + 2, offset + 9, buttonGoodReleasedColor );
    drawLine( g[192], offset + 7, offset + 9, offset + 9, offset + 9, buttonGoodReleasedColor );
    drawLine( g[192], offset + 2, offset + 6, offset + 7, offset + 6, buttonGoodReleasedColor );

    // 6, Cyrillic BE
    resizeGlyph( g[193], 8 + offset * 2, 10 + offset * 2 );
    drawLine( g[193], offset + 0, offset + 0, offset + 7, offset + 0, buttonGoodReleasedColor );
    drawLine( g[193], offset + 0, offset + 9, offset + 6, offset + 9, buttonGoodReleasedColor );
    drawLine( g[193], offset + 1, offset + 1, offset + 1, offset + 8, buttonGoodReleasedColor );
    drawLine( g[193], offset + 2, offset + 4, offset + 6, offset + 4, buttonGoodReleasedColor );
    drawLine( g[193], offset + 7, offset + 5, offset + 7, offset + 8, buttonGoodReleasedColor );
    setPixel( g[193], offset + 7, offset + 1, buttonGoodReleasedColor );

    // B
    resizeGlyph( g[194], 8 + offset * 2, 10 + offset * 2 );
    drawLine( g[194], offset + 0, offset + 0, offset + 5, offset + 0, buttonGoodReleasedColor );
    drawLine( g[194], offset + 0, offset + 9, offset + 6, offset + 9, buttonGoodReleasedColor );
    drawLine( g[194], offset + 1, offset + 1, offset + 1, offset + 8, buttonGoodReleasedColor );
    drawLine( g[194], offset + 2, offset + 4, offset + 6, offset + 4, buttonGoodReleasedColor );
    drawLine( g[194], offset + 6, offset + 1, offset + 6, offset + 3, buttonGoodReleasedColor );
    drawLine( g[194], offset + 7, offset + 5, offset + 7, offset + 8, buttonGoodReleasedColor );

    // r, Cyrillic GHE
    resizeGlyph( g[195], 8 + offset * 2, 10 + offset * 2 );
    drawLine( g[195], offset + 0, offset + 0, offset + 7, offset + 0, buttonGoodReleasedColor );
    drawLine( g[195], offset + 7, offset + 1, offset + 7, offset + 2, buttonGoodReleasedColor );
    drawLine( g[195], offset + 0, offset + 9, offset + 2, offset + 9, buttonGoodReleasedColor );
    drawLine( g[195], offset + 1, offset + 1, offset + 1, offset + 8, buttonGoodReleasedColor );

    // Cyrillic DE
    resizeGlyph( g[196], 10 + offset * 2, 13 + offset * 2 );
    drawLine( g[196], offset + 2, offset + 0, offset + 8, offset + 0, buttonGoodReleasedColor );
    drawLine( g[196], offset + 8, offset + 1, offset + 8, offset + 9, buttonGoodReleasedColor );
    drawLine( g[196], offset + 3, offset + 1, offset + 3, offset + 7, buttonGoodReleasedColor );
    drawLine( g[196], offset + 0, offset + 9, offset + 9, offset + 9, buttonGoodReleasedColor );
    drawLine( g[196], offset + 0, offset + 10, offset + 0, offset + 11, buttonGoodReleasedColor );
    drawLine( g[196], offset + 9, offset + 10, offset + 9, offset + 11, buttonGoodReleasedColor );
    setPixel( g[196], offset + 2, offset + 8, buttonGoodReleasedColor );

    // E
    resizeGlyph( g[197], g[69].width - 1, g[69].height );
    copyGlyphPart( g[69], 1, 0, g[197], 0, 0, g[69].width - 1, g[69].height );
    setTransformPixel( g[197], offset + 0, 1, 1 );
    setTransformPixel( g[197], offset + 9, 1, 1 );

    // X with vertical stroke through it, Cyrillic ZHE. Needs to have upper right and left arms adjusted.
    resizeGlyph( g[198], g[88].width - 1, g[88].height );
    copyGlyphPart( g[88], offset + 1, offset + 0, g[198], offset + 0, offset + 0, 5, g[88].height - offset * 2 );
    copyGlyphPart( g[88], offset + 6, offset + 0, g[198], offset + 6, offset + 0, 5, g[88].height - offset * 2 );
    drawLine( g[198], offset + 5, offset + 1, offset + 5, offset + 8, buttonGoodReleasedColor );
    drawLine( g[198], offset + 4, offset + 0, offset + 6, offset + 0, buttonGoodReleasedColor );
    drawLine( g[198], offset + 4, offset + 9, offset + 6, offset + 9, buttonGoodReleasedColor );

    // 3, Cyrillic ZE
    resizeGlyph( g[199], 7 + offset * 2, 10 + offset * 2 );
    drawLine( g[199], offset + 2, offset + 0, offset + 5, offset + 0, buttonGoodReleasedColor );
    drawLine( g[199], offset + 0, offset + 0, offset + 0, offset + 2, buttonGoodReleasedColor );
    drawLine( g[199], offset + 6, offset + 1, offset + 6, offset + 3, buttonGoodReleasedColor );
    drawLine( g[199], offset + 2, offset + 4, offset + 5, offset + 4, buttonGoodReleasedColor );
    drawLine( g[199], offset + 6, offset + 5, offset + 6, offset + 8, buttonGoodReleasedColor );
    drawLine( g[199], offset + 1, offset + 9, offset + 5, offset + 9, buttonGoodReleasedColor );
    setPixel( g[199], offset + 1, offset + 1, buttonGoodReleasedColor );
    setPixel( g[199], offset + 0, offset + 8, buttonGoodReleasedColor );

    // Mirrored N, Cyrillic I
    resizeGlyph( g[200], 9 + offset * 2, 10 + offset * 2 );
    drawLine( g[200], offset + 1, offset + 1, offset + 1, offset + 9, buttonGoodReleasedColor );
    drawLine( g[200], offset + 0, offset + 0, offset + 2, offset + 0, buttonGoodReleasedColor );
    drawLine( g[200], offset + 2, offset + 8, offset + 6, offset + 1, buttonGoodReleasedColor );
    drawLine( g[200], offset + 7, offset + 0, offset + 7, offset + 8, buttonGoodReleasedColor );
    drawLine( g[200], offset + 6, offset + 9, offset + 8, offset + 9, buttonGoodReleasedColor );
    setPixel( g[200], offset + 0, offset + 9, buttonGoodReleasedColor );
    setPixel( g[200], offset + 8, offset + 0, buttonGoodReleasedColor );

    // Mirrored N with breve, Cyrillic Short I
    resizeGlyph( g[201], g[200].width, g[200].height + 3 );
    copyGlyphPart( g[200], 0, 0, g[201], 0, 3, g[200].width, g[200].height );
    drawLine( g[201], offset + 3, offset + 1, offset + 5, offset + 1, buttonGoodReleasedColor );
    setPixel( g[201], offset + 2, offset + 0, buttonGoodReleasedColor );
    setPixel( g[201], offset + 6, offset + 0, buttonGoodReleasedColor );

    // K.
    resizeGlyph( g[202], g[75].width - 3, g[75].height );
    copyGlyphPart( g[75], offset + 2, offset + 1, g[202], offset + 1, offset + 1, 7, g[75].height - offset * 2 - 2 );
    drawLine( g[202], offset + 0, offset + 0, offset + 2, offset + 0, buttonGoodReleasedColor );
    drawLine( g[202], offset + 0, offset + 9, offset + 2, offset + 9, buttonGoodReleasedColor );
    drawLine( g[202], offset + 6, offset + 0, offset + 8, offset + 0, buttonGoodReleasedColor );
    drawLine( g[202], offset + 6, offset + 9, offset + 8, offset + 9, buttonGoodReleasedColor );
    setPixel( g[202], offset + 7, offset + 8, buttonGoodReleasedColor );

    // /\, Cyrillic EL
    resizeGlyph( g[203], 10 + offset * 2, 10 + offset * 2 );
    drawLine( g[203], offset + 1, offset + 9, offset + 4, offset + 0, buttonGoodReleasedColor );
    drawLine( g[203], offset + 5, offset + 0, offset + 8, offset + 9, buttonGoodReleasedColor );
    drawLine( g[203], offset + 3, offset + 0, offset + 5, offset + 0, buttonGoodReleasedColor );
    drawLine( g[203], offset + 0, offset + 9, offset + 2, offset + 9, buttonGoodReleasedColor );
    drawLine( g[203], offset + 7, offset + 9, offset + 9, offset + 9, buttonGoodReleasedColor );

    // M
    resizeGlyph( g[204], 9 + offset * 2, 10 + offset * 2 );
    drawLine( g[204], offset + 1, offset + 0, offset + 1, offset + 8, buttonGoodReleasedColor );
    drawLine( g[204], offset + 0, offset + 9, offset + 2, offset + 9, buttonGoodReleasedColor );
    drawLine( g[204], offset + 2, offset + 1, offset + 4, offset + 5, buttonGoodReleasedColor );
    drawLine( g[204], offset + 5, offset + 4, offset + 6, offset + 1, buttonGoodReleasedColor );
    drawLine( g[204], offset + 7, offset + 0, offset + 7, offset + 8, buttonGoodReleasedColor );
    drawLine( g[204], offset + 6, offset + 9, offset + 8, offset + 9, buttonGoodReleasedColor );
    setPixel( g[204], offset + 0, offset + 0, buttonGoodReleasedColor );
    setPixel( g[204], offset + 8, offset + 0, buttonGoodReleasedColor );
    setPixel( g[204], offset + 4, offset + 4, buttonGoodReleasedColor );

    // H
    resizeGlyph( g[205], 9 + offset * 2, 10 + offset * 2 );
    drawLine( g[205], offset + 1, offset + 1, offset + 1, offset + 8, buttonGoodReleasedColor );
    drawLine( g[205], offset + 0, offset + 0, offset + 2, offset + 0, buttonGoodReleasedColor );
    drawLine( g[205], offset + 0, offset + 9, offset + 2, offset + 9, buttonGoodReleasedColor );
    drawLine( g[205], offset + 2, offset + 5, offset + 7, offset + 5, buttonGoodReleasedColor );
    drawLine( g[205], offset + 7, offset + 1, offset + 7, offset + 8, buttonGoodReleasedColor );
    drawLine( g[205], offset + 6, offset + 0, offset + 8, offset + 0, buttonGoodReleasedColor );
    drawLine( g[205], offset + 6, offset + 9, offset + 8, offset + 9, buttonGoodReleasedColor );

    // O
    resizeGlyph( g[206], g[79].width - 2, g[79].height );
    copyGlyphPart( g[79], 0, 0, g[206], 0, 0, 7, g[79].height );
    copyGlyphPart( g[79], 9, 0, g[206], 7, 0, g[79].width - 9, g[79].height );

    // Cyrillic PE
    resizeGlyph( g[207], 9 + offset * 2, 10 + offset * 2 );
    drawLine( g[207], offset + 0, offset + 0, offset + 8, offset + 0, buttonGoodReleasedColor );
    drawLine( g[207], offset + 1, offset + 1, offset + 1, offset + 8, buttonGoodReleasedColor );
    drawLine( g[207], offset + 7, offset + 1, offset + 7, offset + 8, buttonGoodReleasedColor );
    drawLine( g[207], offset + 0, offset + 9, offset + 2, offset + 9, buttonGoodReleasedColor );
    drawLine( g[207], offset + 6, offset + 9, offset + 8, offset + 9, buttonGoodReleasedColor );

    // P
    resizeGlyph( g[208], 8 + offset * 2, 10 + offset * 2 );
    drawLine( g[208], offset + 1, offset + 1, offset + 1, offset + 8, buttonGoodReleasedColor );
    drawLine( g[208], offset + 0, offset + 0, offset + 6, offset + 0, buttonGoodReleasedColor );
    drawLine( g[208], offset + 2, offset + 5, offset + 6, offset + 5, buttonGoodReleasedColor );
    drawLine( g[208], offset + 0, offset + 9, offset + 2, offset + 9, buttonGoodReleasedColor );
    drawLine( g[208], offset + 7, offset + 1, offset + 7, offset + 4, buttonGoodReleasedColor );

    // C
    resizeGlyph( g[209], g[67].width - 2, g[67].height );
    copyGlyphPart( g[67], 0, 0, g[209], 0, 0, 7, g[67].height );
    copyGlyphPart( g[67], 9, 0, g[209], 7, 0, g[67].width - 9, g[79].height );

    // T
    resizeGlyph( g[210], g[84].width - 2, g[84].height );
    copyGlyphPart( g[84], 0, 0, g[210], 0, 0, 5, g[84].height );
    copyGlyphPart( g[84], 6, 0, g[210], 5, 0, 3, g[84].height );
    copyGlyphPart( g[84], 10, 0, g[210], 8, 0, 5, g[84].height );

    // y, Cyrillic U
    resizeGlyph( g[211], 9 + offset * 2, 10 + offset * 2 );
    drawLine( g[211], offset + 0, offset + 0, offset + 2, offset + 0, buttonGoodReleasedColor );
    drawLine( g[211], offset + 6, offset + 0, offset + 8, offset + 0, buttonGoodReleasedColor );
    drawLine( g[211], offset + 3, offset + 5, offset + 1, offset + 1, buttonGoodReleasedColor );
    drawLine( g[211], offset + 5, offset + 5, offset + 7, offset + 1, buttonGoodReleasedColor );
    drawLine( g[211], offset + 4, offset + 5, offset + 3, offset + 8, buttonGoodReleasedColor );
    drawLine( g[211], offset + 0, offset + 9, offset + 2, offset + 9, buttonGoodReleasedColor );
    setPixel( g[211], offset + 0, offset + 8, buttonGoodReleasedColor );

    // O with vertical bar, Cyrillic EF
    resizeGlyph( g[212], 10 + offset * 2, 10 + offset * 2 );
    drawLine( g[212], offset + 1, offset + 2, offset + 7, offset + 2, buttonGoodReleasedColor );
    drawLine( g[212], offset + 0, offset + 3, offset + 0, offset + 6, buttonGoodReleasedColor );
    drawLine( g[212], offset + 1, offset + 7, offset + 7, offset + 7, buttonGoodReleasedColor );
    drawLine( g[212], offset + 8, offset + 3, offset + 8, offset + 6, buttonGoodReleasedColor );
    drawLine( g[212], offset + 4, offset + 1, offset + 4, offset + 8, buttonGoodReleasedColor );
    drawLine( g[212], offset + 3, offset + 0, offset + 5, offset + 0, buttonGoodReleasedColor );
    drawLine( g[212], offset + 3, offset + 9, offset + 5, offset + 9, buttonGoodReleasedColor );

    // X
    resizeGlyph( g[213], g[88].width - 3, g[88].height );
    copyGlyphPart( g[88], offset + 1, offset + 0, g[213], offset + 0, offset + 0, 5, g[88].height - offset * 2 );
    copyGlyphPart( g[88], offset + 7, offset + 0, g[213], offset + 5, offset + 0, 4, g[88].height - offset * 2 );

    // Cyrillic TSE
    resizeGlyph( g[214], 9 + offset * 2, 12 + offset * 2 );
    drawLine( g[214], offset + 0, offset + 9, offset + 8, offset + 9, buttonGoodReleasedColor );
    drawLine( g[214], offset + 1, offset + 1, offset + 1, offset + 8, buttonGoodReleasedColor );
    drawLine( g[214], offset + 7, offset + 1, offset + 7, offset + 8, buttonGoodReleasedColor );
    drawLine( g[214], offset + 0, offset + 0, offset + 2, offset + 0, buttonGoodReleasedColor );
    drawLine( g[214], offset + 6, offset + 0, offset + 8, offset + 0, buttonGoodReleasedColor );
    drawLine( g[214], offset + 8, offset + 10, offset + 8, offset + 11, buttonGoodReleasedColor );

    // Cyrillic CHE
    resizeGlyph( g[215], 9 + offset * 2, 10 + offset * 2 );
    drawLine( g[215], offset + 0, offset + 0, offset + 2, offset + 0, buttonGoodReleasedColor );
    drawLine( g[215], offset + 6, offset + 0, offset + 8, offset + 0, buttonGoodReleasedColor );
    drawLine( g[215], offset + 1, offset + 1, offset + 1, offset + 4, buttonGoodReleasedColor );
    drawLine( g[215], offset + 7, offset + 1, offset + 7, offset + 8, buttonGoodReleasedColor );
    drawLine( g[215], offset + 2, offset + 5, offset + 6, offset + 5, buttonGoodReleasedColor );
    drawLine( g[215], offset + 6, offset + 9, offset + 8, offset + 9, buttonGoodReleasedColor );

    // Cyrillic SHA
    resizeGlyph( g[216], 11 + offset * 2, 10 + offset * 2 );
    drawLine( g[216], offset + 1, offset + 1, offset + 1, offset + 8, buttonGoodReleasedColor );
    drawLine( g[216], offset + 5, offset + 1, offset + 5, offset + 8, buttonGoodReleasedColor );
    drawLine( g[216], offset + 9, offset + 1, offset + 9, offset + 8, buttonGoodReleasedColor );
    drawLine( g[216], offset + 0, offset + 9, offset + 10, offset + 9, buttonGoodReleasedColor );
    drawLine( g[216], offset + 0, offset + 0, offset + 2, offset + 0, buttonGoodReleasedColor );
    drawLine( g[216], offset + 4, offset + 0, offset + 6, offset + 0, buttonGoodReleasedColor );
    drawLine( g[216], offset + 8, offset + 0, offset + 10, offset + 0, buttonGoodReleasedColor );

    // Cyrillic SHCHA
    resizeGlyph( g[217], 11 + offset * 2, 12 + offset * 2 );
    copyGlyphPart( g[216], 0, 0, g[217], 0, 0, g[216].width, g[216].height );
    drawLine( g[217], offset + 10, offset + 10, offset + 10, offset + 11, buttonGoodReleasedColor );

    // b, Cyrillic hard sign
    resizeGlyph( g[218], 10 + offset * 2, 10 + offset * 2 );
    drawLine( g[218], offset + 0, offset + 0, offset + 3, offset + 0, buttonGoodReleasedColor );
    drawLine( g[218], offset + 2, offset + 9, offset + 8, offset + 9, buttonGoodReleasedColor );
    drawLine( g[218], offset + 3, offset + 1, offset + 3, offset + 8, buttonGoodReleasedColor );
    drawLine( g[218], offset + 4, offset + 4, offset + 8, offset + 4, buttonGoodReleasedColor );
    drawLine( g[218], offset + 9, offset + 5, offset + 9, offset + 8, buttonGoodReleasedColor );

    // bI, Cyrillic YERU
    resizeGlyph( g[219], 10 + offset * 2, 10 + offset * 2 );
    drawLine( g[219], offset + 0, offset + 0, offset + 2, offset + 0, buttonGoodReleasedColor );
    drawLine( g[219], offset + 0, offset + 9, offset + 4, offset + 9, buttonGoodReleasedColor );
    drawLine( g[219], offset + 1, offset + 1, offset + 1, offset + 8, buttonGoodReleasedColor );
    drawLine( g[219], offset + 2, offset + 4, offset + 4, offset + 4, buttonGoodReleasedColor );
    drawLine( g[219], offset + 5, offset + 5, offset + 5, offset + 8, buttonGoodReleasedColor );
    copyGlyphPart( g[73], offset + 1, 0, g[219], offset + 7, 0, g[73].width - 2 - offset * 2, g[73].height );

    // b, Cyrillic soft sign
    resizeGlyph( g[220], 8 + offset * 2, 10 + offset * 2 );
    drawLine( g[220], offset + 0, offset + 0, offset + 2, offset + 0, buttonGoodReleasedColor );
    drawLine( g[220], offset + 0, offset + 9, offset + 6, offset + 9, buttonGoodReleasedColor );
    drawLine( g[220], offset + 1, offset + 1, offset + 1, offset + 8, buttonGoodReleasedColor );
    drawLine( g[220], offset + 2, offset + 4, offset + 6, offset + 4, buttonGoodReleasedColor );
    drawLine( g[220], offset + 7, offset + 5, offset + 7, offset + 8, buttonGoodReleasedColor );

    // Flipped C with line inside, Cyrillic E
    resizeGlyph( g[221], 8 + offset * 2, 10 + offset * 2 );
    drawLine( g[221], offset + 2, offset + 0, offset + 5, offset + 0, buttonGoodReleasedColor );
    drawLine( g[221], offset + 7, offset + 2, offset + 7, offset + 7, buttonGoodReleasedColor );
    drawLine( g[221], offset + 2, offset + 9, offset + 5, offset + 9, buttonGoodReleasedColor );
    drawLine( g[221], offset + 0, offset + 0, offset + 0, offset + 2, buttonGoodReleasedColor );
    drawLine( g[221], offset + 2, offset + 4, offset + 6, offset + 4, buttonGoodReleasedColor );
    setPixel( g[221], offset + 0, offset + 7, buttonGoodReleasedColor );
    setPixel( g[221], offset + 1, offset + 1, buttonGoodReleasedColor );
    setPixel( g[221], offset + 1, offset + 8, buttonGoodReleasedColor );
    setPixel( g[221], offset + 6, offset + 1, buttonGoodReleasedColor );
    setPixel( g[221], offset + 6, offset + 8, buttonGoodReleasedColor );

    // Ukrainian IE. Make it by mirroring horizontally the previous letter.
    resizeGlyph( g[170], 8 + offset * 2, 10 + offset * 2 );
    {
        ButtonGlyph mirrored;
        flipGlyphHorizontal( g[221], mirrored );
        g[170] = std::move( mirrored );
        setGlyphPosition( g[170], buttonFontOffset.x, buttonFontOffset.y );
    }

    // IO, Cyrillic YU
    resizeGlyph( g[222], 11 + offset * 2, 10 + offset * 2 );
    copyGlyphPart( g[73], offset + 1, 0, g[222], offset + 0, 0, g[73].width - 2 - offset * 2, g[73].height );
    copyGlyphPart( g[79], offset, 0, g[222], offset + 4, 0, 3, g[79].height );
    copyGlyphPart( g[79], offset + 6, 0, g[222], offset + 7, 0, 4, g[79].height );
    drawLine( g[222], offset + 2, offset + 4, offset + 3, offset + 4, buttonGoodReleasedColor );

    // Mirrored R, Cyrillic YA
    resizeGlyph( g[223], g[208].width, g[208].height );
    {
        ButtonGlyph flipped;
        resizeGlyph( flipped, g[208].width, g[208].height );
        flipRegion( g[208], 0, 0, flipped, 0, 0, g[208].width, g[208].height, true, false );
        g[223] = std::move( flipped );
        setGlyphPosition( g[223], buttonFontOffset.x, buttonFontOffset.y );
    }
    drawLine( g[223], offset + 0, offset + 9, offset + 2, offset + 9, buttonGoodReleasedColor );
    drawLine( g[223], offset + 3, offset + 6, offset + 1, offset + 8, buttonGoodReleasedColor );

    // Cyrillic Capital Lje
    resizeGlyph( g[138], g[203].width + g[220].width - 8, g[203].height );
    copyGlyphPart( g[203], 0, 0, g[138], 0, 0, g[203].width, g[203].height );
    copyGlyphPart( g[220], 2, 0, g[138], g[203].width - 6, 0, g[220].width - 2, g[220].height );
    fillTransform( g[138], 7, 3, 1, 1, 1 );
    setGlyphPosition( g[138], g[203].x, g[203].y );

    // Cyrillic Capital Nje
    resizeGlyph( g[140], g[205].width + g[220].width - 7, g[205].height );
    copyGlyphPart( g[205], 0, 0, g[140], 0, 0, g[205].width, g[205].height );
    copyGlyphPart( g[220], 3, 0, g[140], g[205].width - 4, 0, g[220].width - 4, g[220].height );
    setGlyphPosition( g[140], g[205].x, g[205].y );
}

std::array<ButtonGlyph, 256> makeFont( bool pressed )
{
    std::array<ButtonGlyph, 256> font;
    generateGoodButtonFontBaseShape( font );
    generateCP1251GoodButtonFont( font );
    for ( ButtonGlyph & letter : font ) {
        if ( letter.width <= 1 && letter.height <= 1 )
            continue;
        if ( pressed )
            applyGoodButtonPressedLetterEffects( letter );
        else
            applyGoodButtonReleasedLetterEffects( letter );
    }
    return font;
}

} // namespace

const std::array<ButtonGlyph, 256> & buttonFont( bool pressed )
{
    static const std::array<ButtonGlyph, 256> released = makeFont( false );
    static const std::array<ButtonGlyph, 256> pressedFont = makeFont( true );
    return pressed ? pressedFont : released;
}

uint8_t applyTransformTable( uint8_t colorId, uint8_t transformId )
{
    if ( transformId < FIRST_TRANSFORM_ROW || transformId > LAST_TRANSFORM_ROW )
        return colorId;
    return transformRows[transformId - FIRST_TRANSFORM_ROW][colorId];
}

int buttonFontHeight()
{
    return BUTTON_FONT_HEIGHT;
}

int buttonTextWidth( const std::string & text )
{
    const std::array<ButtonGlyph, 256> & font = buttonFont( false );
    int width = 0;
    for ( const char c : text ) {
        const unsigned char code = static_cast<unsigned char>( c );
        if ( code == ' ' ) {
            width += BUTTON_SPACE_WIDTH;
            continue;
        }
        if ( code == '\n' )
            continue;
        const ButtonGlyph & g = font[code];
        if ( g.empty() )
            continue;
        width += g.x + g.width;
    }
    return width;
}

bool buttonFontSupports( const std::string & text )
{
    const std::array<ButtonGlyph, 256> & font = buttonFont( false );
    for ( const char c : text ) {
        const unsigned char code = static_cast<unsigned char>( c );
        if ( code == ' ' || code == '\n' )
            continue;
        if ( font[code].empty() )
            return false;
    }
    return true;
}

void drawButtonText( uint8_t * image, const int bufWidth, const int bufHeight, const int x, const int y, const std::string & text, const bool pressed )
{
    if ( image == nullptr || bufWidth <= 0 || bufHeight <= 0 )
        return;

    const std::array<ButtonGlyph, 256> & font = buttonFont( pressed );
    int offsetX = x;

    for ( const char c : text ) {
        const unsigned char code = static_cast<unsigned char>( c );
        if ( code == ' ' ) {
            offsetX += BUTTON_SPACE_WIDTH;
            continue;
        }
        if ( code == '\n' )
            continue;

        const ButtonGlyph & g = font[code];
        if ( g.empty() ) {
            continue;
        }

        // Blit(glyph, output, offsetX + glyph.x(), y + glyph.y()) — ui_text.cpp:267.
        const int dstX = offsetX + g.x;
        const int dstY = y + g.y;
        for ( int row = 0; row < g.height; ++row ) {
            const int outY = dstY + row;
            if ( outY < 0 || outY >= bufHeight )
                continue;
            for ( int col = 0; col < g.width; ++col ) {
                const int outX = dstX + col;
                if ( outX < 0 || outX >= bufWidth )
                    continue;
                const size_t inOffset = static_cast<size_t>( row ) * g.width + col;
                const uint8_t tf = g.transform[inOffset];
                if ( tf == 1 )
                    continue;
                uint8_t & dst = image[static_cast<size_t>( outY ) * bufWidth + outX];
                if ( tf == 0 )
                    dst = g.image[inOffset];
                else
                    dst = applyTransformTable( dst, tf );
            }
        }

        offsetX += g.width + g.x;
    }
}

} // namespace fh2
