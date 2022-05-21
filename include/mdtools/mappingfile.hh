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

#ifndef LIB_MAPPING_FILE_HH
#define LIB_MAPPING_FILE_HH

#include <mdtools/dplcfile.hh>
#include <mdtools/framemapping.hh>

#include <iosfwd>
#include <vector>

struct mapping_file {
    std::vector<frame_mapping> frames;

    [[nodiscard]] size_t size(int version) const noexcept;

    mapping_file() = default;
    mapping_file(std::istream& input, int version);
    void write(std::ostream& output, int version, bool null_first) const;
    void print() const;
    void merge(mapping_file const& source, dplc_file const& dplc);
    void change_pal(int source_palette, int dest_palette);
    void optimize(
            mapping_file const& source, dplc_file const& indplc,
            dplc_file& outdplc);

    [[nodiscard]] dplc_file split(mapping_file const& source);
};

#endif    // LIB_MAPPING_FILE_HH
