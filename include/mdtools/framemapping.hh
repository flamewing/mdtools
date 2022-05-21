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

#ifndef LIB_FRAME_MAPPING_HH
#define LIB_FRAME_MAPPING_HH

#include <mdtools/framedplc.hh>
#include <mdtools/singlemapping.hh>

#include <compare>
#include <iosfwd>
#include <vector>

struct frame_mapping {
    using split_mapping = std::pair<frame_mapping, frame_dplc>;
    std::vector<single_mapping> maps;

    [[nodiscard]] size_t size(int version) const noexcept {
        return (version == 1 ? 1 : 2) + single_mapping::size(version) * maps.size();
    }

    frame_mapping() = default;
    frame_mapping(std::istream& input, int version);
    void write(std::ostream& output, int version) const;
    void print() const;
    void change_pal(int source_palette, int dest_palette);

    [[nodiscard]] frame_mapping merge(frame_dplc const& dplc) const;
    [[nodiscard]] split_mapping split() const;

    [[nodiscard]] bool operator==(
            frame_mapping const& right) const noexcept = default;
    [[nodiscard]] std::weak_ordering operator<=>(
            frame_mapping const& right) const noexcept = default;
};

#endif    // LIB_FRAME_MAPPING_HH
