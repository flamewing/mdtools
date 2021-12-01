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

void frame_mapping::read(istream& in, int const ver) {
    size_t const cnt = ver == 1 ? Read1(in) : BigEndian::Read2(in);
    for (size_t i = 0; i < cnt; i++) {
        single_mapping sd{};
        sd.read(in, ver);
        maps.push_back(sd);
    }
}

void frame_mapping::write(ostream& out, int const ver) const {
    if (ver == 1) {
        Write1(out, maps.size());
    } else {
        BigEndian::Write2(out, maps.size());
    }
    for (auto const& elem : maps) {
        elem.write(out, ver);
    }
}

void frame_mapping::print() const {
    for (size_t i = 0; i < maps.size(); i++) {
        fmt::print("\tPiece ${:04X}:\n", i);
        maps[i].print();
    }
}

frame_dplc frame_mapping::split(frame_mapping const& src) {
    // First, build the set uf used tiles from main art.
    set<uint16_t> used_tiles;
    for (auto const& sd : src.maps) {
        size_t const ss = sd.tile;
        size_t const sz = size_t(sd.sx) * size_t(sd.sy);
        for (size_t ii = ss; ii < ss + sz; ii++) {
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
                [](const auto& lhs, const auto& rhs) {
                    return rhs != lhs + 1;
                });
        // If there is no such element, we get the end iterator; we must account
        // for this when computing the length of the range to avoid
        // overcounting. Prepare for next iteration.
        if (end_it != used_tiles.cend()) {
            std::advance(end_it, 1);
        }
        uint16_t const length = std::distance(start_it, end_it);
        newdplc.dplc.emplace_back(length, *start_it);
        start_it = end_it;
    }

    // Use the VRAM map to rewrite the arts for the input mappings.
    for (auto const& sd : src.maps) {
        single_mapping nn{};
        single_dplc    dd = nn.split(sd, vram_map);
        ignore_unused_variable_warning(dd);
        maps.push_back(nn);
    }

    // Consolidate the DPLCs so that each DPLC has a proper entry.
    frame_dplc dplc;
    dplc.consolidate(newdplc);
    return dplc;
}

void frame_mapping::merge(frame_mapping const& src, frame_dplc const& dplc) {
    map<size_t, size_t> vram_map;
    dplc.build_vram_map(vram_map);

    for (auto const& sd : src.maps) {
        single_mapping nn{};
        nn.merge(sd, vram_map);
        maps.push_back(nn);
    }
}

void frame_mapping::change_pal(int const srcpal, int const dstpal) {
    for (auto& elem : maps) {
        elem.change_pal(srcpal, dstpal);
    }
}

bool frame_mapping::operator<(frame_mapping const& rhs) const noexcept {
    if (maps.size() < rhs.maps.size()) {
        return true;
    }
    if (maps.size() > rhs.maps.size()) {
        return false;
    }
    for (size_t ii = 0; ii < maps.size(); ii++) {
        if (maps[ii] < rhs.maps[ii]) {
            return true;
        }
        if (rhs.maps[ii] < maps[ii]) {
            return false;
        }
    }
    return false;
}
