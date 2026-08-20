#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <QImage>
#include <QColor>

namespace fh2 {

// ICN sprite: picture + hotspot (offsets from the ICN header — a monster is
// placed into a cell using them). idx — palette color indices (for repainting).
struct IcnSprite {
    QImage image;
    QImage idx; // Format_Indexed8, 0xFF — transparent pixel
    // transform layer like in the engine: 1 — transparent, 0 — color from idx,
    // 2..15 — darken/lighten the already drawn background. Needed for the
    // "indexed" assemblies (buttons), where sprites are stacked as in fheroes2.
    QImage tf; // Format_Indexed8
    int offsetX = 0;
    int offsetY = 0;

    bool isNull() const { return image.isNull(); }
};

// AGG container: concatenation of files with a (hash, offset, size) table and
// 8.3 names at the end (format described in FH2_SAVE_FORMAT.md, § "Resources").
class AggContainer
{
public:
    bool open( const std::string & path );
    bool isGood() const { return !_files.empty(); }
    // Contents of a file from AGG or an empty vector if the file is missing.
    std::vector<uint8_t> read( const std::string & name ) const;

private:
    std::map<std::string, std::pair<uint32_t, uint32_t>, std::less<>> _files; // name → (offset, size)
    std::vector<uint8_t> _data;
};

// Decodes an ICN sprite (image layer + transform layer) into an RGBA picture and
// a palette index map. palette — normalized (8-bit) KB.PAL.
// transform==1 — transparent pixel; transform==0 and 2..15 — opaque
// (the engine applies darkening only when blitting onto an opaque background; our
// bridge blits onto a transparent one — colors stay original).
IcnSprite decodeIcnSprite( const std::vector<uint8_t> & body, int index, const std::array<QRgb, 256> & palette );

// Decodes a tile from TIL (GROUND32/CLOF32/STON): header
// count(u16) + width(u16) + height(u16), then count×width×height palette
// indices with no transparency (port of the engine's GetMaximumTILIndex/decodeTILImages).
// Returns an empty sprite if the index is out of range or the header is broken.
IcnSprite decodeTilSprite( const std::vector<uint8_t> & body, int index, const std::array<QRgb, 256> & palette );

} // namespace fh2
