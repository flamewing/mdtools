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

#ifndef __LIB_FRAMEMAPPING_H
#define __LIB_FRAMEMAPPING_H

#include <mdtools/framedplc.hh>
#include <mdtools/singlemapping.hh>

#include <compare>
#include <iosfwd>
#include <vector>

struct frame_mapping {
    using split_mapping = std::pair<frame_mapping, frame_dplc>;
    std::vector<single_mapping> maps;

    [[nodiscard]] size_t size(int ver) const noexcept {
        return (ver == 1 ? 1 : 2) + single_mapping::size(ver) * maps.size();
    }
    void read(std::istream& in, int ver);
    void write(std::ostream& out, int ver) const;
    void print() const;
    void change_pal(int srcpal, int dstpal);

    [[nodiscard]] frame_mapping merge(frame_dplc const& dplc) const;
    [[nodiscard]] split_mapping split() const;

    [[nodiscard]] auto operator<=>(frame_mapping const& rhs) const noexcept = default;
};

#endif    // __LIB_FRAMEMAPPING_H
