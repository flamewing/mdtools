/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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

#include <iosfwd>
#include <mdtools/framedplc.hh>
#include <vector>

class dplc_file {
protected:
    std::vector<frame_dplc> frames;

public:
    void read(std::istream& in, int const ver);
    void write(std::ostream& out, int const ver, bool const nullfirst) const;
    void print() const;
    void consolidate(dplc_file const& src);
    void insert(frame_dplc const& val);
    frame_dplc const& get_dplc(size_t const i) const { return frames[i]; }
    bool              empty() const { return frames.empty(); }
    size_t            size() const { return frames.size(); }
    size_t            size(int const ver) const;
};

#endif // __LIB_DPLCFILE_H
