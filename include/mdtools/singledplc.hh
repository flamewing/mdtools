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

#include <compare>
#include <cstdint>
#include <iosfwd>

struct single_dplc {
    uint16_t count;
    uint16_t tile;

    [[nodiscard]] static size_t size(int const ver) noexcept {
        ignore_unused_variable_warning(ver);
        return 2;
    }

    single_dplc() = default;
    single_dplc(uint16_t count, uint16_t tile) noexcept
            : count(count), tile(tile) {}
    single_dplc(std::istream& in, int ver);
    void write(std::ostream& out, int ver) const;
    void print() const;

    [[nodiscard]] auto operator<=>(single_dplc const& rhs) const noexcept {
        if (auto cmp = count <=> rhs.count;
            cmp != std::strong_ordering::equal) {
            return cmp;
        }
        return tile <=> rhs.tile;
    }
};

#endif    // __LIB_SINGLEDPLC_H
