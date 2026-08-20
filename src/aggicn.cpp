#include "aggicn.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>

namespace fh2 {

namespace {

uint16_t readLE16( const uint8_t * p )
{
    return static_cast<uint16_t>( p[0] | ( p[1] << 8 ) );
}

uint32_t readLE32( const uint8_t * p )
{
    return static_cast<uint32_t>( p[0] ) | ( static_cast<uint32_t>( p[1] ) << 8 ) | ( static_cast<uint32_t>( p[2] ) << 16 )
           | ( static_cast<uint32_t>( p[3] ) << 24 );
}

int16_t readLE16s( const uint8_t * p )
{
    return static_cast<int16_t>( readLE16( p ) );
}

// AGG file name hash (port of calculateAggFilenameHash from the engine's agg_file.cpp).
uint32_t aggFilenameHash( const std::string & str )
{
    uint32_t hash = 0;
    uint32_t sum = 0;

    for ( auto it = str.rbegin(); it != str.rend(); ++it ) {
        const unsigned char c = static_cast<unsigned char>( std::toupper( static_cast<unsigned char>( *it ) ) );

        hash = ( hash << 5 ) + ( hash >> 25 );

        sum += c;
        hash += sum + c;
    }

    return hash;
}

} // namespace

bool AggContainer::open( const std::string & path )
{
    std::ifstream f( path, std::ios::binary );
    if ( !f )
        return false;

    f.seekg( 0, std::ios::end );
    const std::streamoff fileSize = f.tellg();
    f.seekg( 0, std::ios::beg );
    if ( fileSize < 4 )
        return false;

    _data.resize( static_cast<size_t>( fileSize ) );
    f.read( reinterpret_cast<char *>( _data.data() ), fileSize );
    f.close();
    if ( static_cast<std::streamoff>( f.gcount() ) != fileSize )
        return false;

    const size_t size = _data.size();
    const size_t count = readLE16( _data.data() );
    constexpr size_t fileRecordSize = 12;
    constexpr size_t maxFilenameSize = 15; // 8.3 ASCIIZ + 2 alignment bytes

    if ( count * ( fileRecordSize + maxFilenameSize ) >= size )
        return false;

    const size_t nameEntriesOffset = size - maxFilenameSize * count;
    _files.clear();
    for ( size_t i = 0; i < count; ++i ) {
        const uint8_t * namePos = _data.data() + nameEntriesOffset + i * maxFilenameSize;
        const size_t nameLen = strnlen( reinterpret_cast<const char *>( namePos ), 13 );
        if ( nameLen == 0 )
            continue;
        const std::string name( reinterpret_cast<const char *>( namePos ), nameLen );

        const uint8_t * record = _data.data() + 2 + i * fileRecordSize;
        if ( readLE32( record ) != aggFilenameHash( name ) ) {
            // Hash mismatch — the file is corrupted.
            _files.clear();
            return false;
        }
        const uint32_t fileOffset = readLE32( record + 4 );
        const uint32_t fileSz = readLE32( record + 8 );
        if ( fileOffset + fileSz > size ) {
            _files.clear();
            return false;
        }
        _files.try_emplace( name, std::make_pair( fileOffset, fileSz ) );
    }

    return !_files.empty();
}

std::vector<uint8_t> AggContainer::read( const std::string & name ) const
{
    const auto it = _files.find( name );
    if ( it == _files.end() )
        return {};

    const auto [offset, size] = it->second;
    return std::vector<uint8_t>( _data.begin() + offset, _data.begin() + offset + size );
}

IcnSprite decodeIcnSprite( const std::vector<uint8_t> & body, int index, const std::array<QRgb, 256> & palette )
{
    IcnSprite out;
    if ( body.size() < 6 )
        return out;

    const uint32_t count = readLE16( body.data() );
    const uint32_t blockSize = readLE32( body.data() + 2 );
    if ( count == 0 || index < 0 || static_cast<uint32_t>( index ) >= count )
        return out;

    constexpr size_t headerSize = 6;
    const uint8_t * header = body.data() + headerSize + static_cast<size_t>( index ) * 13;
    if ( header + 13 > body.data() + body.size() )
        return out;

    const int16_t offsetX = readLE16s( header );
    const int16_t offsetY = readLE16s( header + 2 );
    const uint16_t width = readLE16( header + 4 );
    const uint16_t height = readLE16( header + 6 );
    const uint8_t animationFrames = header[8];
    const uint32_t dataOffset = readLE32( header + 9 );

    uint32_t dataSize = 0;
    if ( static_cast<uint32_t>( index ) + 1 != count ) {
        const uint8_t * nextHeader = body.data() + headerSize + ( static_cast<size_t>( index ) + 1 ) * 13;
        dataSize = readLE32( nextHeader + 9 ) - dataOffset;
    }
    else {
        if ( dataOffset > blockSize )
            return out;
        dataSize = blockSize - dataOffset;
    }

    if ( headerSize + dataOffset + dataSize > body.size() )
        return out;
    if ( width == 0 || height == 0 )
        return out;

    // image layer: color = palette index; transform: 0 — opaque,
    // 1 — transparent, 2..15 — opaque (blitting onto a transparent background
    // does not apply darkening — like Blit in the engine).
    std::vector<uint8_t> image( static_cast<size_t>( width ) * height, 0 );
    std::vector<uint8_t> transform( static_cast<size_t>( width ) * height, 1 );

    const uint8_t * data = body.data() + headerSize + dataOffset;
    const uint8_t * dataEnd = data + dataSize;

    uint32_t posX = 0;
    uint32_t posY = 0;
    const size_t rowPitch = width;

    const auto put = [&]( uint32_t x, uint32_t y, uint8_t color, uint8_t tf ) {
        if ( y >= height || x >= width )
            return;
        image[static_cast<size_t>( y ) * rowPitch + x] = color;
        transform[static_cast<size_t>( y ) * rowPitch + x] = tf;
    };

    // Bit 5 of animationFrames — monochrome sprite (engine's decodeICNSprite).
    const bool isMonochromatic = ( animationFrames & 0x20 ) != 0;

    if ( isMonochromatic ) {
        while ( data < dataEnd ) {
            const uint8_t op = *data;
            if ( op == 0 ) {
                // End of line.
                ++data;
                posX = 0;
                ++posY;
            }
            else if ( op < 0x80 ) {
                // N black pixels (transform=0, color 0).
                const uint32_t n = op;
                for ( uint32_t i = 0; i < n; ++i )
                    put( posX++, posY, 0, 0 );
                ++data;
            }
            else if ( op == 0x80 ) {
                break;
            }
            else {
                // (n − 0x80) transparent pixels.
                posX += op - 0x80;
                ++data;
            }
        }
    }
    else {
        while ( data < dataEnd ) {
            const uint8_t op = *data;
            if ( op == 0 ) {
                // End of line; the rest of the line is transparent.
                ++data;
                posX = 0;
                ++posY;
            }
            else if ( op < 0x80 ) {
                // N literal pixels.
                const uint32_t n = op;
                ++data;
                if ( data + n > dataEnd )
                    break;
                for ( uint32_t i = 0; i < n; ++i )
                    put( posX++, posY, data[i], 0 );
                data += n;
            }
            else if ( op == 0x80 ) {
                // End of image.
                break;
            }
            else if ( op < 0xC0 ) {
                // (n − 0x80) transparent pixels.
                posX += op - 0x80;
                ++data;
            }
            else if ( op == 0xC0 ) {
                // Transform-layer run.
                ++data;
                if ( data >= dataEnd )
                    break;
                const uint8_t transformValue = *data;
                const uint32_t countValue = transformValue & 0x03;
                uint32_t pixelCount;
                if ( countValue != 0 ) {
                    pixelCount = countValue;
                }
                else {
                    ++data;
                    pixelCount = *data;
                }
                if ( transformValue & 0x40 ) {
                    const uint8_t transformType = static_cast<uint8_t>( ( ( transformValue & 0x3C ) >> 2 ) + 2 );
                    if ( transformType < 16 ) {
                        for ( uint32_t i = 0; i < pixelCount; ++i )
                            put( posX++, posY, 0, transformType );
                        ++data;
                        continue;
                    }
                }
                // Without 0x40 — transparent pixels.
                posX += pixelCount;
                ++data;
            }
            else {
                // 0xC1 — count in the next byte; 0xC2..0xFF — count = op − 0xC0.
                uint32_t pixelCount;
                if ( op == 0xC1 ) {
                    ++data;
                    if ( data >= dataEnd )
                        break;
                    pixelCount = *data;
                }
                else {
                    pixelCount = op - 0xC0;
                }
                ++data;
                if ( data >= dataEnd )
                    break;
                const uint8_t color = *data;
                for ( uint32_t i = 0; i < pixelCount; ++i )
                    put( posX++, posY, color, 0 );
                ++data;
            }
        }
    }

    // Build RGBA + idx.
    out.image = QImage( width, height, QImage::Format_RGBA8888 );
    out.image.fill( Qt::transparent );
    out.idx = QImage( width, height, QImage::Format_Indexed8 );
    for ( int i = 0; i < 256; ++i )
        out.idx.setColor( i, QColor( i, i, i ).rgb() );
    out.idx.fill( 0xFF );

    out.tf = QImage( width, height, QImage::Format_Indexed8 );
    for ( int i = 0; i < 256; ++i )
        out.tf.setColor( i, QColor( i, i, i ).rgb() );
    out.tf.fill( 1 ); // transparent by default

    // The engine does not paint transform pixels with a color: it darkens or
    // lightens whatever is already drawn on screen (transformTable, image.cpp:43).
    // Here sprites are blitted via QPainter, so such pixels become semi-transparent
    // black (2..5 — darkening) or white (6..9 — lightening) with alpha matching
    // the palette step strength: k = 1.15 − 0.17·t, alpha = 255·(1 − k).
    // They used to be drawn as OPAQUE black — hence the black slabs instead of
    // shadows on frames (BUYBUILD/SURDRBKG/WINLOSE) and dirt around letters.
    const auto transformPixel = []( uint8_t tf ) -> QRgb {
        if ( tf >= 2 && tf <= 5 ) {
            const int alpha = static_cast<int>( 255.0 * ( 0.17 * tf - 0.15 ) + 0.5 );
            return qRgba( 0, 0, 0, std::min( 255, std::max( 0, alpha ) ) );
        }
        if ( tf >= 6 && tf <= 9 ) {
            const int alpha = static_cast<int>( 255.0 * ( 0.17 * ( tf - 4 ) - 0.15 ) + 0.5 );
            return qRgba( 255, 255, 255, std::min( 255, std::max( 0, alpha ) ) );
        }
        return qRgba( 0, 0, 0, 0 );
    };

    for ( uint32_t y = 0; y < height; ++y ) {
        for ( uint32_t x = 0; x < width; ++x ) {
            const size_t o = static_cast<size_t>( y ) * rowPitch + x;
            const uint8_t tf = transform[o];
            out.tf.setPixel( static_cast<int>( x ), static_cast<int>( y ), tf );
            if ( tf == 1 )
                continue; // transparent
            if ( tf != 0 ) {
                // Shadow/highlight: the color is not taken from the image layer.
                out.image.setPixel( static_cast<int>( x ), static_cast<int>( y ), transformPixel( tf ) );
                out.idx.setPixel( static_cast<int>( x ), static_cast<int>( y ), 0 );
                continue;
            }
            const uint8_t colorId = image[o];
            out.image.setPixel( static_cast<int>( x ), static_cast<int>( y ), palette[colorId] );
            out.idx.setPixel( static_cast<int>( x ), static_cast<int>( y ), colorId );
        }
    }

    out.offsetX = offsetX;
    out.offsetY = offsetY;
    return out;
}

IcnSprite decodeTilSprite( const std::vector<uint8_t> & body, int index, const std::array<QRgb, 256> & palette )
{
    IcnSprite out;
    constexpr size_t headerSize = 6;
    if ( body.size() < headerSize || index < 0 )
        return out;

    const uint32_t count = readLE16( body.data() );
    const uint32_t width = readLE16( body.data() + 2 );
    const uint32_t height = readLE16( body.data() + 4 );
    // Check as in GetMaximumTILIndex: the file size must match exactly.
    if ( count < 1 || width < 1 || height < 1 || headerSize + static_cast<size_t>( count ) * width * height != body.size() )
        return out;
    if ( static_cast<uint32_t>( index ) >= count )
        return out;

    const uint8_t * src = body.data() + headerSize + static_cast<size_t>( index ) * width * height;

    out.image = QImage( static_cast<int>( width ), static_cast<int>( height ), QImage::Format_RGBA8888 );
    out.idx = QImage( static_cast<int>( width ), static_cast<int>( height ), QImage::Format_Indexed8 );
    for ( int i = 0; i < 256; ++i )
        out.idx.setColor( i, QColor( i, i, i ).rgb() );
    out.tf = QImage( static_cast<int>( width ), static_cast<int>( height ), QImage::Format_Indexed8 );
    for ( int i = 0; i < 256; ++i )
        out.tf.setColor( i, QColor( i, i, i ).rgb() );
    out.tf.fill( 0 ); // the whole tile is opaque

    // TIL tiles are fully opaque (the format has no transform layer).
    for ( uint32_t y = 0; y < height; ++y ) {
        for ( uint32_t x = 0; x < width; ++x ) {
            const uint8_t colorId = src[static_cast<size_t>( y ) * width + x];
            out.image.setPixel( static_cast<int>( x ), static_cast<int>( y ), palette[colorId] | 0xFF000000u );
            out.idx.setPixel( static_cast<int>( x ), static_cast<int>( y ), colorId );
        }
    }
    return out;
}

} // namespace fh2
