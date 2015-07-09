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

#include "ssvram.h"

#include <cassert>
#include <iostream>
#include <sstream>

#include "bigendian_io.h"
#include "mdcomp/kosinski.h"

using namespace std;

// Initializes the S2 Special Stage VRAM track tiles from the given stream. The
// tiles are assumed to be Kosinski-encoded after being reduced from 8 identical
// lines dow to a single line.
SSVRAM::SSVRAM(istream &in) noexcept : VRAM() {
	// Start by decompressing Kosinski-encoded source data.
	stringstream sout(ios::in | ios::out | ios::binary);
	kosinski::decode(in, sout);
	sout.seekg(0);
	sout.clear();

	unsigned cnt = BigEndian::Read2(sout);
	// Now convert it all to tiles.
	tiles.reserve(cnt);

	for (unsigned ii = 0; ii < cnt; ii++) {
		ShortTile tile;
		ShortTile::iterator dst1 = tile.begin(NoFlip),
		                    dst2 = dst1 + ShortTile::Line_size;
		ShortTile::const_iterator const finish = tile.end(NoFlip);
		for ( ; sout.good() && dst2 != finish; ++dst1, ++dst2) {
			unsigned char cc = static_cast<unsigned char>(sout.get());
			*dst1++ = *dst2++ = (cc >> 4) & 0xf;
			*dst1 = *dst2 = cc & 0xf;
		}
		tiles.push_back(tile);
	}
}

// Splits the given full-sized tile into 4 short (2-line) tiles.
vector<ShortTile> split_tile(Tile const &tile) noexcept {
	// Check compatibility. We could do away with this if no one else ever were
	// to use this.
	assert(static_cast<unsigned>(Tile::Line_size) == static_cast<unsigned>(ShortTile::Line_size));
	// Reserve space for return value.
	vector<ShortTile> ret;
	ret.reserve(4);

	Tile::const_iterator start = tile.begin(NoFlip);
	for (unsigned ii = 0; ii < Tile::Num_lines / ShortTile::Num_lines; ii++) {
		// Want to copy 2 lines.
		Tile::const_iterator finish = start + 2 * ShortTile::Line_size;
		ret.push_back(ShortTile(start, finish, NoFlip, 1));
	}
	return ret;
}

// Merges the given 4 short (2-line) tiles into a full-sized tile.
Tile merge_tiles(ShortTile const &tile0, FlipMode flip0,
                 ShortTile const &tile1, FlipMode flip1,
                 ShortTile const &tile2, FlipMode flip2,
                 ShortTile const &tile3, FlipMode flip3) noexcept {
	Tile dest;
	Tile::iterator it = dest.begin(NoFlip);
	for (ShortTile::const_iterator src = tile0.begin(flip0) ;
	     it != dest.end(NoFlip) && src != tile0.end(flip0); ++it, ++src) {
		*it = *src;
	}
	for (ShortTile::const_iterator src = tile1.begin(flip1) ;
	     it != dest.end(NoFlip) && src != tile1.end(flip1); ++it, ++src) {
		*it = *src;
	}
	for (ShortTile::const_iterator src = tile2.begin(flip2) ;
	     it != dest.end(NoFlip) && src != tile2.end(flip2); ++it, ++src) {
		*it = *src;
	}
	for (ShortTile::const_iterator src = tile3.begin(flip3) ;
	     it != dest.end(NoFlip) && src != tile3.end(flip3); ++it, ++src) {
		*it = *src;
	}
	return dest;
}
