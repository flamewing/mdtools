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

void frame_dplc::read(istream& in, int const ver) {
    size_t cnt = [&]() -> size_t {
        if (ver == 1) {
            return Read1(in);
        }
        if (ver == 4) {
            return static_cast<int16_t>(BigEndian::Read2(in)) + 1;
        }
        return BigEndian::Read2(in);
    }();

    for (size_t i = 0; i < cnt; i++) {
        single_dplc sd{};
        sd.read(in, ver);
        dplc.push_back(sd);
    }
}

void frame_dplc::write(ostream& out, int const ver) const {
    if (ver == 1) {
        Write1(out, dplc.size());
    } else if (ver == 4) {
        BigEndian::Write2(
                out,
                static_cast<uint16_t>(static_cast<int16_t>(dplc.size()) - 1));
    } else {
        BigEndian::Write2(out, dplc.size());
    }
    for (auto const& elem : dplc) {
        elem.write(out, ver);
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
    for (auto const& sd : dplc) {
        if (sd.tile != start + size) {
            interm.dplc.emplace_back(size, start);
            start = sd.tile;
            size  = sd.count;
        } else {
            size += sd.count;
        }
    }
    if (size != 0) {
        interm.dplc.emplace_back(size, start);
    }

    for (auto const& elem : interm.dplc) {
        uint16_t tile = elem.tile;
        uint16_t sz   = elem.count;

        while (sz >= 16) {
            output.dplc.emplace_back(16, tile);
            sz -= 16;
            tile += 16;
        }
        if (sz != 0U) {
            output.dplc.emplace_back(sz, tile);
        }
    }
    return output;
}

std::map<size_t, size_t> frame_dplc::build_vram_map() const {
    std::map<size_t, size_t> vram_map;
    for (auto const& sd : dplc) {
        size_t ss = sd.tile;
        size_t sz = sd.count;
        for (size_t i = ss; i < ss + sz; i++) {
            vram_map.emplace(vram_map.size(), i);
        }
    }
    return vram_map;
}
