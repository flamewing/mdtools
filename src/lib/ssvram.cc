/*
 * Copyright (C) Flamewing 2014 <flamewing.sonic@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mdcomp/bigendian_io.hh>
#include <mdcomp/kosinski.hh>
#include <mdtools/ssvram.hh>

#include <cassert>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>

using std::ios;
using std::istream;
using std::numeric_limits;
using std::streamsize;
using std::stringstream;
using std::vector;

// Initializes the S2 Special Stage VRAM track tiles from the given stream. The
// tiles are assumed to be Kosinski-encoded after being reduced from 8 identical
// lines dow to a single line.
SSVRAM::SSVRAM(istream& palette_file, istream& art_file) noexcept {
    // Start by decompressing Kosinski-encoded source data.
    stringstream decoded_art(ios::in | ios::out | ios::binary);
    kosinski::decode(art_file, decoded_art);
    decoded_art.seekg(0);
    decoded_art.clear();

    unsigned const count = BigEndian::Read2(decoded_art);
    // Now convert it all to tiles.
    reserve_tiles(count);

    for (unsigned ii = 0; ii < count; ii++) {
        auto& tile   = new_tile();
        auto  dst1   = tile.begin(NoFlip);
        auto  dst2   = dst1 + ShortTile::Line_size;
        auto  finish = tile.end(NoFlip);
        for (; decoded_art.good() && dst2 != finish; ++dst1, ++dst2) {
            uint32_t value = static_cast<uint8_t>(decoded_art.get());
            *dst1++ = *dst2++ = (value >> 4U) & 0xfU;
            *dst1 = *dst2 = value & 0xfU;
        }
    }

    auto const start = palette_file.tellg();
    palette_file.ignore(numeric_limits<streamsize>::max());
    auto const char_count = palette_file.gcount();
    palette_file.seekg(start);
    size_t palLen = char_count / sizeof(uint16_t);
    assert(palLen >= 16);
    vector<uint16_t> palette(palLen, 0);
    for (size_t ii = 0; ii < palLen; ii++) {
        palette[ii] = BigEndian::Read2(palette_file);
    }
    auto const getRed = [](uint32_t const color) -> uint32_t {
        return color & 0xfU;
    };
    auto const getGreen = [](uint32_t const color) -> uint32_t {
        return (color >> 4U) & 0xfU;
    };
    auto const getBlue = [](uint32_t const color) -> uint32_t {
        return (color >> 8U) & 0xfU;
    };
    auto const deltaSquare
            = [](int const color1, int const color2) -> unsigned {
        return (color1 - color2) * (color1 - color2);
    };
    auto const distance = [&deltaSquare, &getRed, &getGreen, &getBlue](
                                  uint16_t const color1,
                                  uint16_t const color2) -> unsigned {
        return deltaSquare(getRed(color1), getRed(color2))
               + deltaSquare(getGreen(color1), getGreen(color2))
               + deltaSquare(getBlue(color1), getBlue(color2));
    };
    auto& DistTable = get_dist_table();
    for (size_t ii = 0; ii < 16; ii++) {
        DistTable[ii][ii] = 0;
        for (size_t jj = ii + 1; jj < 16; jj++) {
            DistTable[ii][jj] = DistTable[jj][ii]
                    = distance(palette[ii], palette[jj]);
        }
    }
}

// Splits the given full-sized tile into 4 short (2-line) tiles.
vector<ShortTile> split_tile(Tile const& tile) noexcept {
    // Check compatibility. We could do away with this if no one else ever were
    // to use this.
    static_assert(
            Tile::Line_size == ShortTile::Line_size,
            "Mismatched tile line sizes!");
    // Reserve space for return value.
    vector<ShortTile> new_tiles;
    new_tiles.reserve(4);

    auto start = tile.begin(NoFlip);
    for (unsigned ii = 0; ii < Tile::Num_lines / ShortTile::Num_lines; ii++) {
        // Want to copy 2 lines.
        auto const finish = start + 2 * ShortTile::Line_size;
        new_tiles.emplace_back(start, finish, NoFlip, 1);
    }
    return new_tiles;
}

// Merges the given 4 short (2-line) tiles into a full-sized tile.
Tile merge_tiles(
        ShortTile const& tile0, FlipMode const flip0, ShortTile const& tile1,
        FlipMode const flip1, ShortTile const& tile2, FlipMode const flip2,
        ShortTile const& tile3, FlipMode const flip3) noexcept {
    Tile dest;
    auto iter = dest.begin(NoFlip);
    for (auto source = tile0.begin(flip0);
         iter != dest.end(NoFlip) && source != tile0.end(flip0);
         ++iter, ++source) {
        *iter = *source;
    }
    for (auto source = tile1.begin(flip1);
         iter != dest.end(NoFlip) && source != tile1.end(flip1);
         ++iter, ++source) {
        *iter = *source;
    }
    for (auto source = tile2.begin(flip2);
         iter != dest.end(NoFlip) && source != tile2.end(flip2);
         ++iter, ++source) {
        *iter = *source;
    }
    for (auto source = tile3.begin(flip3);
         iter != dest.end(NoFlip) && source != tile3.end(flip3);
         ++iter, ++source) {
        *iter = *source;
    }
    return dest;
}
