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

#ifndef __LIB_FRAMEDPLC_H
#define __LIB_FRAMEDPLC_H

#include <mdtools/singledplc.hh>

#include <compare>
#include <iosfwd>
#include <map>
#include <vector>

struct frame_dplc {
    std::vector<single_dplc> dplc;

    [[nodiscard]] size_t size(int ver) const noexcept {
        return (ver == 1 ? 1 : 2) + single_dplc::size(ver) * dplc.size();
    }

    frame_dplc() = default;
    frame_dplc(std::istream& in, int ver);
    void write(std::ostream& out, int ver) const;
    void print() const;

    [[nodiscard]] std::map<size_t, size_t> build_vram_map() const;

    [[nodiscard]] frame_dplc consolidate() const;

    [[nodiscard]] bool operator==(
            frame_dplc const& rhs) const noexcept = default;
    [[nodiscard]] auto operator<=>(
            frame_dplc const& rhs) const noexcept = default;
};

static_assert(std::three_way_comparable<frame_dplc>);

#endif    // __LIB_FRAMEDPLC_H
