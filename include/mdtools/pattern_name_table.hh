/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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

#ifndef __PATTERN_NAME_TABLE_H
#define __PATTERN_NAME_TABLE_H

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
    Pattern_Name_Table(std::istream& in, bool const compressed) noexcept {
        std::stringstream sin(std::ios::in | std::ios::out | std::ios::binary);
        if (compressed) {
            enigma::decode(in, sin);
        } else {
            sin << in.rdbuf();
        }

        for (Line& line : table) {
            for (auto& pattern : line) {
                uint16_t pnt = BigEndian::Read2(sin);
                pattern      = Pattern_Name(sin.good() ? pnt : 0U);
            }
        }
    }
    Pattern_Name_Table(Pattern_Name_Table const&)     = default;
    Pattern_Name_Table(Pattern_Name_Table&&) noexcept = default;
    virtual ~Pattern_Name_Table() noexcept            = default;
    Pattern_Name_Table& operator=(Pattern_Name_Table const&) = default;
    Pattern_Name_Table& operator=(Pattern_Name_Table&&) noexcept = default;

    Line const& operator[](size_t const n) const noexcept {
        return table[n];
    }
    Line& operator[](size_t const n) noexcept {
        return table[n];
    }

    virtual void write(std::ostream& out) const noexcept {
        for (Line const& line : table) {
            for (auto const& pattern : line) {
                pattern.write(out);
            }
        }
    }
};

using PlaneH32V28  = Pattern_Name_Table<32, 28>;
using PlaneH40V28  = Pattern_Name_Table<40, 28>;
using PlaneH128V28 = Pattern_Name_Table<128, 28>;

#endif    // __PATTERN_NAME_TABLE_H
