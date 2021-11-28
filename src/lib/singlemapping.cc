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
#include <mdtools/singlemapping.hh>

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
#include <iomanip>
#include <iostream>

using std::cout;
using std::dec;
using std::endl;
using std::hex;
using std::ios;
using std::istream;
using std::map;
using std::nouppercase;
using std::ostream;
using std::setfill;
using std::setw;
using std::uppercase;

void single_mapping::read(istream& in, int const ver) {
    yy    = static_cast<int8_t>(Read1(in));
    sy    = Read1(in);
    sx    = ((sy & 0xcU) >> 2U) + 1;
    sy    = (sy & 0x3U) + 1;
    tile  = BigEndian::Read2(in);
    flags = ((tile & 0xf800U) >> 8U);
    tile &= 0x07ffU;
    if (ver == 2) {
        in.ignore(2);
    }
    if (ver == 1) {
        xx = static_cast<int8_t>(Read1(in));
    } else {
        xx = BigEndian::Read2(in);
    }
}

void single_mapping::write(ostream& out, int const ver) const {
    Write1(out, static_cast<uint8_t>(yy));
    Write1(out, ((unsigned(sx) - 1) << 2U) | (unsigned(sy) - 1));
    BigEndian::Write2(out, (unsigned(flags) << 8U) | tile);
    if (ver == 2) {
        BigEndian::Write2(
                out, (unsigned(flags) << 8U) | (unsigned(tile) >> 1U));
    }
    if (ver == 1) {
        Write1(out, static_cast<uint8_t>(xx));
    } else {
        BigEndian::Write2(out, xx);
    }
}

void single_mapping::print() const {
    fmt::print(
            "\t\tPosition: (x,y) = ({:4},{:4})\tSize: (x,y) = ({:4},{:4})\n",
            static_cast<int16_t>(xx), static_cast<int16_t>(yy),
            static_cast<int16_t>(sx), static_cast<int16_t>(sy));
    fmt::print(
            "\t\tFirst tile: ${:04X}\tLast tile: ${:04X}\tFlags: ", tile,
            (tile + sx * sy - 1));
    if ((flags & 0x80U) != 0) {
        fmt::print("foreground{}", (flags & 0x78U) != 0 ? "|" : "");
    }
    if ((flags & 0x60U) != 0) {
        fmt::print(
                "palette+{}{}", (flags & 0x60U) >> 5U,
                (flags & 0x18U) != 0 ? "|" : "");
    }
    if ((flags & 0x08U) != 0) {
        fmt::print("flip_x{}", (flags & 0x10U) != 0 ? "|" : "");
    }
    if ((flags & 0x10U) != 0) {
        fmt::print("flip_y");
    }
    std::fputc('\n', stdout);
}

void single_mapping::split(
        single_mapping const& src, single_dplc& dplc,
        map<size_t, size_t>& vram_map) {
    xx    = src.xx;
    yy    = src.yy;
    sx    = src.sx;
    sy    = src.sy;
    flags = src.flags;
    tile  = vram_map[src.tile];
    dplc.set_cnt(sx * sy);
    dplc.set_tile(src.tile);
}

void single_mapping::merge(
        single_mapping const& src, map<size_t, size_t>& vram_map) {
    xx    = src.xx;
    yy    = src.yy;
    sx    = src.sx;
    sy    = src.sy;
    flags = src.flags;
    tile  = vram_map[src.tile];
}

void single_mapping::change_pal(uint32_t const srcpal, uint32_t const dstpal) {
    if ((flags & 0x60U) == srcpal) {
        flags = (flags & 0x9fU) | dstpal;
    }
}
