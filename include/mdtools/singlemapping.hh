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

#ifndef __LIB_SINGLEMAPPING_H
#define __LIB_SINGLEMAPPING_H

#include <mdtools/singledplc.hh>

#include <iosfwd>
#include <map>

struct single_mapping {
    using split_mapping = std::pair<single_mapping, single_dplc>;
    uint16_t flags, tile;
    int16_t  xx, yy;
    uint8_t  sx, sy;
    enum MapSizes : size_t { sizeS1 = 5, sizeS2 = 8, sizeS3 = 6 };

    [[nodiscard]] static size_t size(int const ver) noexcept {
        switch (ver) {
        case 1:
            return sizeS1;
        case 2:
            return sizeS2;
        default:
            return sizeS3;
        }
    }
    void read(std::istream& in, int ver);
    void write(std::ostream& out, int ver) const;
    void print() const;
    void change_pal(uint32_t srcpal, uint32_t dstpal);

    [[nodiscard]] single_mapping merge(
            std::map<size_t, size_t>& vram_map) const noexcept;
    [[nodiscard]] split_mapping split(
            std::map<size_t, size_t>& vram_map) const noexcept;

    [[nodiscard]] bool operator<(single_mapping const& rhs) const noexcept {
        if (tile < rhs.tile) {
            return true;
        }
        if (tile > rhs.tile) {
            return false;
        }
        if (sx < rhs.sx) {
            return true;
        }
        if (sx > rhs.sx) {
            return false;
        }
        if (sy < rhs.sy) {
            return true;
        }
        if (sy > rhs.sy) {
            return false;
        }
        if (flags < rhs.flags) {
            return true;
        }
        if (flags > rhs.flags) {
            return false;
        }
        if (xx < rhs.xx) {
            return true;
        }
        if (xx > rhs.xx) {
            return false;
        }
        if (yy < rhs.yy) {
            return true;
        }
        /*if (yy > rhs.yy)*/
        return false;
    }
    [[nodiscard]] bool operator==(single_mapping const& rhs) const noexcept {
        return !(*this < rhs || rhs < *this);
    }
};

#endif    // __LIB_SINGLEMAPPING_H
