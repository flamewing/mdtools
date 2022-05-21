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

#ifndef LIB_SINGLEMAPPING_HH
#define LIB_SINGLEMAPPING_HH

#include <mdtools/singledplc.hh>

#include <compare>
#include <iosfwd>
#include <map>
#include <tuple>

struct single_mapping {
    using split_mapping = std::pair<single_mapping, single_dplc>;
    using init_tuple    = std::tuple<
            uint16_t, uint16_t, int16_t, int16_t, uint8_t, uint8_t>;
    uint16_t tile;
    uint16_t flags;
    int16_t  xx;
    int16_t  yy;
    uint8_t  sx;
    uint8_t  sy;
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

    single_mapping() = default;
    explicit single_mapping(init_tuple const& values)
            : tile{std::get<0>(values)}, flags{std::get<1>(values)},
              xx{std::get<2>(values)}, yy{std::get<3>(values)},
              sx{std::get<4>(values)}, sy{std::get<5>(values)} {}
    single_mapping(std::istream& in, int ver);
    void write(std::ostream& out, int ver) const;
    void print() const;
    void change_pal(uint32_t srcpal, uint32_t dstpal);

    [[nodiscard]] single_mapping merge(
            std::map<size_t, size_t>& vram_map) const noexcept;
    [[nodiscard]] split_mapping split(
            std::map<size_t, size_t>& vram_map) const noexcept;

    [[nodiscard]] bool operator==(
            single_mapping const& rhs) const noexcept = default;
    [[nodiscard]] std::strong_ordering operator<=>(
            single_mapping const& rhs) const noexcept = default;
};

#endif    // LIB_SINGLEMAPPING_HH
