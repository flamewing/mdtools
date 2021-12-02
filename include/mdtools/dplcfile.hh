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

#ifndef __LIB_DPLCFILE_H
#define __LIB_DPLCFILE_H

#include <mdtools/framedplc.hh>

#include <iosfwd>
#include <vector>

struct dplc_file {
    std::vector<frame_dplc> frames;

    [[nodiscard]] size_t size(int ver) const noexcept;

    dplc_file() = default;
    dplc_file(std::istream& in, int ver);
    void write(std::ostream& out, int ver, bool nullfirst) const;
    void print() const;

    [[nodiscard]] dplc_file consolidate() const;
};

#endif    // __LIB_DPLCFILE_H
