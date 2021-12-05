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
#include <iostream>

using std::ios;
using std::istream;
using std::map;
using std::ostream;

static inline single_mapping::init_tuple read_mapping(
        istream& in, int const ver) {
    int8_t const   pos_y   = BigEndian::Read<int8_t>(in);
    uint8_t const  size    = BigEndian::Read<uint8_t>(in);
    uint16_t const pattern = BigEndian::Read<uint16_t>(in);
    if (ver == 2) {
        in.ignore(2);
    }
    int16_t const pos_x = [&]() -> int16_t {
        if (ver == 1) {
            return BigEndian::Read<int8_t>(in);
        }
        return BigEndian::Read<int16_t>(in);
    }();

    return {pattern & 0x07ffU,
            ((pattern & 0xf800U) >> 8U),
            pos_x,
            pos_y,
            ((size & 0xcU) >> 2U) + 1,
            (size & 0x3U) + 1};
}

single_mapping::single_mapping(istream& in, int const ver)
        : single_mapping(read_mapping(in, ver)) {}

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

single_mapping::split_mapping single_mapping::split(
        map<size_t, size_t>& vram_map) const noexcept {
    split_mapping output{*this, {static_cast<uint16_t>(sx * sy), tile}};
    output.first.tile = vram_map[tile];
    return output;
}

single_mapping single_mapping::merge(
        map<size_t, size_t>& vram_map) const noexcept {
    single_mapping output{*this};
    output.tile = vram_map[tile];
    return output;
}

void single_mapping::change_pal(uint32_t const srcpal, uint32_t const dstpal) {
    if ((flags & 0x60U) == srcpal) {
        flags = (flags & 0x9fU) | dstpal;
    }
}
