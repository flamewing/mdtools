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

#ifndef __LIB_SINGLEDPLC_H
#define __LIB_SINGLEDPLC_H

#include <mdtools/ignore_unused_variable_warning.hh>

#include <cstdint>
#include <iosfwd>

class single_dplc {
private:
    uint16_t cnt, tile;

public:
    static size_t size(int const ver) {
        ignore_unused_variable_warning(ver);
        return 2;
    }
    void read(std::istream& in, int ver);
    void write(std::ostream& out, int ver) const;
    void print() const;

    uint16_t get_cnt() const {
        return cnt;
    }
    uint16_t get_tile() const {
        return tile;
    }

    void set_cnt(uint16_t const c) {
        cnt = c;
    }
    void set_tile(uint16_t const t) {
        tile = t;
    }

    bool operator<(single_dplc const& rhs) const {
        if (cnt < rhs.cnt) {
            return true;
        }
        if (cnt > rhs.cnt) {
            return false;
        }
        return tile < rhs.tile;
    }
    bool operator==(single_dplc const& rhs) const {
        return !(*this < rhs || rhs < *this);
    }
};

#endif    // __LIB_SINGLEDPLC_H
