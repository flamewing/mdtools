/*
 * Copyright (C) Flamewing 2011-2015 <flamewing.sonic@gmail.com>
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
#include <mdtools/framemapping.hh>
#include <mdtools/ignore_unused_variable_warning.hh>

#include <compare>

#ifdef __GNUG__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#    pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif
#define FMT_HEADER_ONLY 1
#include <fmt/format.h>
#ifdef __GNUG__
#    pragma GCC diagnostic pop
#endif

#include <algorithm>
#include <iostream>
#include <set>

using std::ios;
using std::istream;
using std::map;
using std::ostream;
using std::set;

frame_mapping::frame_mapping(istream& input, int const version) {
    size_t const count = version == 1 ? BigEndian::Read<uint8_t>(input)
                                      : BigEndian::Read<uint16_t>(input);
    for (size_t i = 0; i < count; i++) {
        maps.emplace_back(input, version);
    }
}

void frame_mapping::write(ostream& output, int const version) const {
    if (version == 1) {
        Write1(output, maps.size());
    } else {
        BigEndian::Write2(output, maps.size());
    }
    for (auto const& elem : maps) {
        elem.write(output, version);
    }
}

void frame_mapping::print() const {
    for (size_t i = 0; i < maps.size(); i++) {
        fmt::print("\tPiece ${:04X}:\n", i);
        maps[i].print();
    }
}

frame_mapping::split_mapping frame_mapping::split() const {
    // First, build the set uf used tiles from main art.
    set<uint16_t> used_tiles;
    for (auto const& elem : maps) {
        size_t const tile  = elem.tile;
        size_t const count = size_t(elem.sx) * size_t(elem.sy);
        for (size_t ii = tile; ii < tile + count; ii++) {
            used_tiles.emplace(ii);
        }
    }

    // Now we will generate a VRAM map assuming these tiles are loaded to
    // VRAM in order, with 0 at the start.
    map<size_t, size_t> vram_map;
    for (auto const tile : used_tiles) {
        vram_map.emplace(tile, vram_map.size());
    }

    // Now we split the tiles into the minimal set of DPLCs of arbitrary
    // length that can reproduce them.
    frame_dplc newdplc;
    for (auto start_it = used_tiles.cbegin(); start_it != used_tiles.cend();) {
        // Find first element which is not followed by an adjacent element.
        auto end_it = std::adjacent_find(
                start_it, used_tiles.cend(),
                [](const auto& left, const auto& right) {
                    return right != left + 1;
                });
        // If there is no such element, we get the end iterator; we must account
        // for this when computing the length of the range to avoid
        // over-counting. Prepare for next iteration.
        if (end_it != used_tiles.cend()) {
            std::advance(end_it, 1);
        }
        uint16_t const length = std::distance(start_it, end_it);
        newdplc.dplc.emplace_back(length, *start_it);
        start_it = end_it;
    }

    split_mapping output{{}, newdplc.consolidate()};
    auto& [output_maps, outdplc] = output;
    // Use the VRAM map to rewrite the arts for the input mappings.
    for (auto const& elem : maps) {
        auto [nn, dd] = elem.split(vram_map);
        output_maps.maps.push_back(nn);
    }

    // Consolidate the DPLCs so that each DPLC has a proper entry.
    return output;
}

frame_mapping frame_mapping::merge(frame_dplc const& dplc) const {
    map<size_t, size_t> vram_map = dplc.build_vram_map();

    frame_mapping output;
    for (auto const& elem : maps) {
        output.maps.push_back(elem.merge(vram_map));
    }
    return output;
}

void frame_mapping::change_pal(
        int const source_palette, int const dest_palette) {
    for (auto& elem : maps) {
        elem.change_pal(source_palette, dest_palette);
    }
}
