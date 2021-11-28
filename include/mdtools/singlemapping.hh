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

class single_mapping {
private:
    uint16_t flags, tile;
    int16_t  xx, yy;
    uint8_t  sx, sy;
    enum MapSizes : size_t { sizeS1 = 5, sizeS2 = 8, sizeS3 = 6 };

public:
    static size_t size(int const ver) noexcept {
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
    void split(
            single_mapping const& src, single_dplc& dplc,
            std::map<size_t, size_t>& vram_map);
    void merge(single_mapping const& src, std::map<size_t, size_t>& vram_map);
    void change_pal(uint32_t srcpal, uint32_t dstpal);

    uint16_t get_flags() const noexcept {
        return flags;
    }
    uint16_t get_tile() const noexcept {
        return tile;
    }
    int16_t get_xx() const noexcept {
        return xx;
    }
    int16_t get_yy() const noexcept {
        return yy;
    }
    uint8_t get_sx() const noexcept {
        return sx;
    }
    uint8_t get_sy() const noexcept {
        return sy;
    }

    void set_flags(uint16_t const t) noexcept {
        flags = t;
    }
    void set_tile(uint16_t const t) noexcept {
        tile = t;
    }
    void set_xx(int16_t t) noexcept {
        xx = t;
    }
    void set_yy(int16_t t) noexcept {
        yy = t;
    }
    void set_sx(int8_t const t) noexcept {
        sx = t;
    }
    void set_sy(int8_t const t) noexcept {
        sy = t;
    }
    bool operator<(single_mapping const& rhs) const noexcept {
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
    bool operator==(single_mapping const& rhs) const noexcept {
        return !(*this < rhs || rhs < *this);
    }
};

#endif    // __LIB_SINGLEMAPPING_H
