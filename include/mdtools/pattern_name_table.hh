/*
 * Copyright (C) Flamewing 2014 <flamewing.sonic@gmail.com>
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

#ifndef PATTERN_NAME_TABLE_HH
#define PATTERN_NAME_TABLE_HH

#include <mdcomp/bigendian_io.hh>
#include <mdcomp/enigma.hh>
#include <mdtools/pattern_name.hh>

#include <array>
#include <sstream>

template <unsigned width, unsigned height>
class Pattern_Name_Table {
public:
    enum { Width = width, Height = height };
    using Line = std::array<Pattern_Name, width>;

private:
    std::array<Line, height> table;

protected:
    auto const& getTable() const noexcept {
        return table;
    }
    auto& getTable() noexcept {
        return table;
    }

public:
    Pattern_Name_Table() noexcept = default;
    Pattern_Name_Table(std::istream& input, bool const compressed) noexcept {
        std::stringstream inner_input(
                std::ios::in | std::ios::out | std::ios::binary);
        if (compressed) {
            enigma::decode(input, inner_input);
        } else {
            inner_input << input.rdbuf();
        }

        for (Line& line : table) {
            for (auto& pattern : line) {
                uint16_t new_pattern = BigEndian::Read2(inner_input);
                pattern = Pattern_Name(inner_input.good() ? new_pattern : 0U);
            }
        }
    }
    Pattern_Name_Table(Pattern_Name_Table const&) noexcept            = default;
    Pattern_Name_Table(Pattern_Name_Table&&) noexcept                 = default;
    virtual ~Pattern_Name_Table() noexcept                            = default;
    Pattern_Name_Table& operator=(Pattern_Name_Table const&) noexcept = default;
    Pattern_Name_Table& operator=(Pattern_Name_Table&&) noexcept      = default;

    Line const& operator[](size_t const index) const noexcept {
        return table[index];
    }
    Line& operator[](size_t const index) noexcept {
        return table[index];
    }

    virtual void write(std::ostream& output) const noexcept {
        for (Line const& line : table) {
            for (auto const& pattern : line) {
                pattern.write(output);
            }
        }
    }
};

using PlaneH32V28  = Pattern_Name_Table<32, 28>;
using PlaneH40V28  = Pattern_Name_Table<40, 28>;
using PlaneH128V28 = Pattern_Name_Table<128, 28>;

#endif    // PATTERN_NAME_TABLE_HH
