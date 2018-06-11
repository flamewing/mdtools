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

#ifndef __SSVRAM_H
#define __SSVRAM_H

#include <iosfwd>
#include <vector>

#include "vram.h"

// Ideally, <8,1> would be enough -- except that we will be needing to import
// from an image which may not be perfectly equal on each pair of tile lines.
typedef BaseTile<8,2> ShortTile;

// Class simulating the S2 Special Stage VRAM. Uses short (2-line) tiles as they
// are enough for representing the tiles needed by the track, including for the
// purposes of finding the tile that best matches any given 2-line tile in an
// imported image.
class SSVRAM : public VRAM<ShortTile> {
public:
	SSVRAM(std::istream &pal, std::istream &art) noexcept;
};

std::vector<ShortTile> split_tile(Tile const &tile) noexcept;
Tile merge_tiles(ShortTile const &tile0, FlipMode const flip0,
                 ShortTile const &tile1, FlipMode const flip1,
                 ShortTile const &tile2, FlipMode const flip2,
                 ShortTile const &tile3, FlipMode const flip3) noexcept;

#endif // __SSVRAM_H
