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

#ifndef __LIB_MAPPINGFILE_H
#define __LIB_MAPPINGFILE_H

#include <mdtools/dplcfile.hh>
#include <mdtools/framemapping.hh>

#include <iosfwd>
#include <vector>

class mapping_file {
private:
    std::vector<frame_mapping> frames;

public:
    size_t size(int ver) const noexcept;
    size_t size() const noexcept {
        return frames.size();
    }

    void read(std::istream& in, int ver);
    void write(std::ostream& out, int ver, bool nullfirst) const;
    void print() const;
    void split(mapping_file const& src, dplc_file& dplc);
    void merge(mapping_file const& src, dplc_file const& dplc);
    void change_pal(int srcpal, int dstpal);
    bool empty() const noexcept {
        return frames.empty();
    }
    void optimize(
            mapping_file const& src, dplc_file const& indplc,
            dplc_file& outdplc);
    frame_mapping const& get_maps(size_t const i) const noexcept {
        return frames[i];
    }
};

#endif    // __LIB_MAPPINGFILE_H
