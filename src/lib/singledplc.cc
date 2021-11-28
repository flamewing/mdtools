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
#include <mdtools/singledplc.hh>

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

#include <iostream>

using std::ios;
using std::istream;
using std::ostream;

void single_dplc::read(istream& in, int const ver) {
    tile = BigEndian::Read2(in);
    if (ver < 4) {
        cnt = ((tile & 0xf000U) >> 12U) + 1U;
        tile &= 0x0fffU;
    } else {
        cnt  = (tile & 0x000fU) + 1U;
        tile = (tile & 0xfff0U) >> 4U;
    }
}

void single_dplc::write(ostream& out, int const ver) const {
    if (ver < 4) {
        BigEndian::Write2(out, (unsigned(cnt - 1) << 12U) | tile);
    } else {
        BigEndian::Write2(out, (unsigned(tile) << 4U) | (unsigned(cnt) - 1));
    }
}

void single_dplc::print() const {
    fmt::print(
            "\tFirst tile: ${:04X}\tLast tile: ${:04X}\tNum tiles: ${:04X}\n",
            tile, (tile + cnt - 1), cnt);
}
