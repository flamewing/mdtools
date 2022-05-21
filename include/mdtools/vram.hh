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

#ifndef VRAM_HH
#define VRAM_HH

#include <mdtools/pattern_name.hh>
#include <mdtools/tile.hh>

#include <array>
#include <iosfwd>
#include <vector>

// Template class with basic VRAM functionality.
template <typename Tile_t>
class VRAM {
public:
    using Tiles = std::vector<Tile_t>;

private:
    Tiles       tiles;
    DistTable_t distTable = {};

public:
    // Constructor.
    VRAM() noexcept {
        tiles.reserve(400);
    }

    explicit VRAM(std::istream& input) noexcept {
        tiles.reserve(400);

        // Just read in tiles.
        while (input.good()) {
            Tile_t tile(input);
            if (!input.good()) {
                break;
            }
            tiles.push_back(tile);
        }
    }

    void reserve_tiles(size_t count) {
        tiles.reserve(count);
    }
    Tile_t& new_tile() {
        tiles.emplace_back();
        return tiles.back();
    }

    auto& get_dist_table() const {
        return distTable;
    }
    auto& get_dist_table() {
        return distTable;
    }

    void copy_dist_table(VRAM<Tile_t> const& other) {
        if (this != &other) {
            distTable = other.get_dist_table();
        }
    }
    template <typename T>
    void copy_dist_table(VRAM<T> const& other) {
        distTable = other.get_dist_table();
    }
    // Gets the referred tile. No bounds checking!
    Tile_t& get_tile(Pattern_Name const& pattern) noexcept {
        return tiles[pattern.get_tile()];
    }

    // Gets the referred tile. No bounds checking!
    Tile_t& operator[](Pattern_Name const& pattern) noexcept {
        return tiles[pattern.get_tile()];
    }

    // Gets the referred tile. No bounds checking!
    Tile_t const& operator[](Pattern_Name const& pattern) const noexcept {
        return tiles[pattern.get_tile()];
    }

    // Appends the tile to VRAM.
    Pattern_Name push_back(Tile_t const& tile) noexcept {
        Pattern_Name pattern(tiles.size());
        tiles.push_back(tile);
        return pattern;
    }

    // Adds the tile to VRAM, changing its size if needed.
    void add_tile(Tile_t const& tile, Pattern_Name const& pattern) noexcept {
        uint32_t index = pattern.get_tile();
        if (index >= tiles.size()) {
            tiles.resize(index + 1);
        }
        tiles[index] = tile;
    }
    // Finds the pattern name of the tile that most closely resembles the given
    // tile. This resemblance is based on the distance function defined in the
    // tile class. Returns the pattern name as a parameter, and the distance as
    // the function return.
    uint32_t find_closest(
            Tile_t const& tile, Pattern_Name& best) const noexcept {
        // Start with "infinite" distance.
        uint32_t best_dist = ~0U;
        auto     start     = tile.begin(NoFlip);
        auto     finish    = tile.end(NoFlip);
        // Want to compare using all possible flips.
        static constexpr std::array<FlipMode, 4> const modes{
                NoFlip, XFlip, YFlip, XYFlip};

        for (auto it = tiles.begin(); it != tiles.end(); ++it) {
            for (const auto mode : modes) {
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
    void write(std::ostream& output) const noexcept {
        for (auto it = tiles.begin(); it != tiles.end(); ++it) {
            auto dest = it->begin(NoFlip);
            it->draw_tile(output, dest, Tile_t::Num_lines);
        }
    }
};

#endif    // VRAM_HH
