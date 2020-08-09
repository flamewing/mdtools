/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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
SSVRAM::SSVRAM(istream& pal, istream& art) noexcept {
    // Start by decompressing Kosinski-encoded source data.
    stringstream sout(ios::in | ios::out | ios::binary);
    kosinski::decode(art, sout);
    sout.seekg(0);
    sout.clear();

    unsigned const cnt = BigEndian::Read2(sout);
    // Now convert it all to tiles.
    reserve_tiles(cnt);

    for (unsigned ii = 0; ii < cnt; ii++) {
        ShortTile& tile = new_tile();

        auto dst1   = tile.begin(NoFlip);
        auto dst2   = dst1 + ShortTile::Line_size;
        auto finish = tile.end(NoFlip);
        for (; sout.good() && dst2 != finish; ++dst1, ++dst2) {
            auto cc = static_cast<uint8_t>(sout.get());
            *dst1++ = *dst2++ = (cc >> 4) & 0xf;
            *dst1 = *dst2 = cc & 0xf;
        }
    }

    auto const start = pal.tellg();
    pal.ignore(numeric_limits<streamsize>::max());
    auto const charCount = pal.gcount();
    pal.seekg(start);
    size_t palLen = charCount / sizeof(uint16_t);
    assert(palLen >= 16);
    vector<uint16_t> palette(palLen, 0);
    for (size_t ii = 0; ii < palLen; ii++) {
        palette[ii] = BigEndian::Read2(pal);
    }
    auto const getR = [](uint16_t const clr) -> int {
        return clr & 0xf;
    };
    auto const getG = [](uint16_t const clr) -> int {
        return (clr >> 4) & 0xf;
    };
    auto const getB = [](uint16_t const clr) -> int {
        return (clr >> 8) & 0xf;
    };
    auto const deltaSquare = [](int const c1, int const c2) -> unsigned {
        return (c1 - c2) * (c1 - c2);
    };
    auto const distance
            = [&deltaSquare, &getR, &getG,
               &getB](uint16_t const c1, uint16_t const c2) -> unsigned {
        return deltaSquare(getR(c1), getR(c2)) + deltaSquare(getG(c1), getG(c2))
               + deltaSquare(getB(c1), getB(c2));
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
    vector<ShortTile> ret;
    ret.reserve(4);

    Tile::const_iterator start = tile.begin(NoFlip);
    for (unsigned ii = 0; ii < Tile::Num_lines / ShortTile::Num_lines; ii++) {
        // Want to copy 2 lines.
        Tile::const_iterator const finish = start + 2 * ShortTile::Line_size;
        ret.emplace_back(start, finish, NoFlip, 1);
    }
    return ret;
}

// Merges the given 4 short (2-line) tiles into a full-sized tile.
Tile merge_tiles(
        ShortTile const& tile0, FlipMode const flip0, ShortTile const& tile1,
        FlipMode const flip1, ShortTile const& tile2, FlipMode const flip2,
        ShortTile const& tile3, FlipMode const flip3) noexcept {
    Tile dest;
    auto it = dest.begin(NoFlip);
    for (ShortTile::const_iterator src = tile0.begin(flip0);
         it != dest.end(NoFlip) && src != tile0.end(flip0); ++it, ++src) {
        *it = *src;
    }
    for (ShortTile::const_iterator src = tile1.begin(flip1);
         it != dest.end(NoFlip) && src != tile1.end(flip1); ++it, ++src) {
        *it = *src;
    }
    for (ShortTile::const_iterator src = tile2.begin(flip2);
         it != dest.end(NoFlip) && src != tile2.end(flip2); ++it, ++src) {
        *it = *src;
    }
    for (ShortTile::const_iterator src = tile3.begin(flip3);
         it != dest.end(NoFlip) && src != tile3.end(flip3); ++it, ++src) {
        *it = *src;
    }
    return dest;
}
