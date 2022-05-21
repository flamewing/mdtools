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
#include <mdtools/framedplc.hh>

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

#include <cstdint>
#include <iostream>

using std::ios;
using std::istream;
using std::map;
using std::ostream;

frame_dplc::frame_dplc(istream& input, int const version) {
    size_t count = [&]() -> size_t {
        if (version == 1) {
            return BigEndian::Read<uint8_t>(input);
        }
        if (version == 4) {
            return BigEndian::Read<int16_t>(input) + 1;
        }
        return BigEndian::Read<uint16_t>(input);
    }();

    for (size_t i = 0; i < count; i++) {
        dplc.emplace_back(input, version);
    }
}

void frame_dplc::write(ostream& output, int const version) const {
    if (version == 1) {
        Write1(output, dplc.size());
    } else if (version == 4) {
        BigEndian::Write2(
                output,
                static_cast<uint16_t>(static_cast<int16_t>(dplc.size()) - 1));
    } else {
        BigEndian::Write2(output, dplc.size());
    }
    for (auto const& elem : dplc) {
        elem.write(output, version);
    }
}

void frame_dplc::print() const {
    size_t ntiles = 0;
    for (auto const& elem : dplc) {
        ntiles += elem.count;
        elem.print();
    }
    fmt::print("\tTile count: ${:04X}\n", ntiles);
}

[[nodiscard]] frame_dplc frame_dplc::consolidate() const {
    frame_dplc output;
    if (dplc.empty()) {
        return output;
    }

    uint16_t   start = dplc[0].tile;
    uint16_t   size  = 0;
    frame_dplc interm;
    for (auto const& elem : dplc) {
        if (elem.tile != start + size) {
            interm.dplc.emplace_back(size, start);
            start = elem.tile;
            size  = elem.count;
        } else {
            size += elem.count;
        }
    }
    if (size != 0) {
        interm.dplc.emplace_back(size, start);
    }

    for (auto const& elem : interm.dplc) {
        uint16_t tile  = elem.tile;
        uint16_t count = elem.count;

        while (count >= 16) {
            output.dplc.emplace_back(16, tile);
            count -= 16;
            tile += 16;
        }
        if (count != 0U) {
            output.dplc.emplace_back(count, tile);
        }
    }
    return output;
}

std::map<size_t, size_t> frame_dplc::build_vram_map() const {
    std::map<size_t, size_t> vram_map;
    for (auto const& elem : dplc) {
        size_t tile  = elem.tile;
        size_t count = elem.count;
        for (size_t ii = tile; ii < tile + count; ii++) {
            vram_map.emplace(vram_map.size(), ii);
        }
    }
    return vram_map;
}
