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

#ifndef __VRAM_H
#define __VRAM_H

#include <iosfwd>
#include <vector>

#include "pattern_name.hh"
#include "tile.hh"

// Template class with basic VRAM functionality.
template<typename Tile_t>
class VRAM {
public:
	typedef std::vector<Tile_t> Tiles;

protected:
	Tiles tiles;
	unsigned distTable[16][16];

public:
	// Constructor.
	VRAM() noexcept {
		tiles.reserve(400);
	}

	VRAM(std::istream &in) noexcept {
		tiles.reserve(400);

		// Just read in tiles.
		while (in.good()) {
			Tile_t tile(in);
			if (!in.good()) {
				break;
			}
			tiles.push_back(tile);
		}
	}

	decltype(distTable) const & get_dist_table() const {
		return distTable;
	}
	void copy_dist_table(VRAM<Tile_t> const& other) {
		if (this != &other) {
			std::memcpy(distTable, other.get_dist_table(), sizeof(distTable));
		}
	}
	template<typename T>
	void copy_dist_table(VRAM<T> const& other) {
		std::memcpy(distTable, other.get_dist_table(), sizeof(distTable));
	}
	// Gets the referred tile. No bounds checking!
	Tile_t &get_tile(Pattern_Name const &pnt) noexcept {
		return tiles[pnt.get_tile()];
	}

	// Gets the referred tile. No bounds checking!
	Tile_t &operator[](Pattern_Name const &pnt) noexcept {
		return tiles[pnt.get_tile()];
	}

	// Gets the referred tile. No bounds checking!
	Tile_t const &operator[](Pattern_Name const &pnt) const noexcept {
		return tiles[pnt.get_tile()];
	}

	// Appends the tile to VRAM.
	Pattern_Name push_back(Tile_t const &tile) noexcept {
		Pattern_Name pat(tiles.size());
		tiles.push_back(tile);
		return pat;
	}

	// Adds the tile to VRAM, changing its size if needed.
	void add_tile(Tile_t const &tile, Pattern_Name const &pnt) noexcept {
		unsigned index = pnt.get_tile();
		if (index >= tiles.size()) {
			tiles.resize(index + 1);
		}
		tiles[index] = tile;
	}
	// Finds the pattern name of the tile that most closely resembles the given
	// tile. This resemblance is based on the distance function defined in the
	// tile class. Returns the pattern name as a parameter, and the distance as
	// the function return.
	unsigned find_closest(Tile_t const &tile, Pattern_Name &best) const noexcept {
		// Start with "infinite" distance.
		unsigned best_dist = ~0u;
		typename Tile_t::const_iterator start  = tile.begin(NoFlip),
		                                finish = tile.end(NoFlip);
		// Want to compare using all possible flips.
		static FlipMode const modes[] = {NoFlip, XFlip, YFlip, XYFlip};

		for (typename Tiles::const_iterator it = tiles.begin();
		     it != tiles.end(); ++it) {
			for (auto & mode : modes) {
				unsigned dist = it->distance(distTable, mode, start, finish);
				if (dist < best_dist) {
					// Set new best.
					best = Pattern_Name(it - tiles.begin());
					best.set_flip(mode);
					best_dist = dist;
				}
			}
		}
		return best_dist;
	}
	void write(std::ostream &out) const noexcept {
		for (typename Tiles::const_iterator it = tiles.begin();
		     it != tiles.end(); ++it) {
			typename Tile_t::const_iterator it2 = it->begin(NoFlip);
			it->draw_tile(out, it2, Tile_t::Num_lines);
		}
	}
};

#endif // __VRAM_H
