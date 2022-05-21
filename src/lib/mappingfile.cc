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

#include <mdcomp/bigendian_io.hh>
#include <mdtools/mappingfile.hh>

#ifdef __GNUG__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#    pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif
#define FMT_HEADER_ONLY 1
#include <fmt/format.h>
#ifdef __GNUG__
#    pragma GCC diagnostic pop
#endif

#include <cstdint>
#include <iostream>
#include <map>
#include <numeric>

using std::ios;
using std::istream;
using std::map;
using std::ostream;
using std::vector;

mapping_file::mapping_file(istream& input, int const version) {
    input.seekg(0, ios::beg);
    vector<int16_t> offsets;

    int16_t term = BigEndian::Read<int16_t>(input);
    while (term == 0) {
        offsets.push_back(term);
        term = BigEndian::Read<int16_t>(input);
    }
    offsets.push_back(term);
    while (input.tellg() < term) {
        int16_t const newterm = BigEndian::Read<int16_t>(input);
        if (newterm > 0 && newterm < term) {
            term = newterm;
        }
        offsets.push_back(newterm);
    }

    for (auto const position : offsets) {
        input.clear();
        input.seekg(position);
        frames.emplace_back(input, version);
    }
}

void mapping_file::write(
        ostream& output, int const version, bool const null_first) const {
    map<frame_mapping, size_t> map_to_pos;
    map<size_t, frame_mapping> pos_to_map;
    size_t                     size = 2 * frames.size();

    if (null_first && !frames.empty() && frames.front().maps.empty()) {
        map_to_pos.emplace(frames.front(), 0);
        pos_to_map.emplace(0, frames.front());
    }
    for (auto const& frame : frames) {
        if (auto const found = map_to_pos.find(frame);
            found != map_to_pos.end()) {
            BigEndian::Write2(output, found->second);
        } else {
            map_to_pos.emplace(frame, size);
            pos_to_map.emplace(size, frame);
            BigEndian::Write2(output, size);
            size += frame.size(version);
        }
    }
    for (auto const& [pos, maps] : pos_to_map) {
        if (pos == size_t(output.tellp())) {
            maps.write(output, version);
        } else if (pos != 0U) {
            fmt::print(stderr, "Missed write at {}\n", output.tellp());
            maps.print();
        }
    }
}

void mapping_file::print() const {
    fmt::print("{:=>80}\n", "");
    for (size_t i = 0; i < frames.size(); i++) {
        fmt::print("Mappings for frame ${:04X}:\n", i);
        frames[i].print();
    }
}

dplc_file mapping_file::split(mapping_file const& source) {
    dplc_file dplc;
    for (auto const& elem : source.frames) {
        auto [nn, interm] = elem.split();
        frames.push_back(nn);
        dplc.frames.push_back(interm.consolidate());
    }
    return dplc;
}

void mapping_file::merge(mapping_file const& source, dplc_file const& dplc) {
    for (size_t i = 0; i < source.frames.size(); i++) {
        frame_mapping new_map = source.frames[i].merge(dplc.frames[i]);
        frames.push_back(new_map);
    }
}

void mapping_file::optimize(
        mapping_file const& source, dplc_file const& indplc,
        dplc_file& outdplc) {
    for (size_t i = 0; i < source.frames.size(); i++) {
        auto const& inter_map  = source.frames[i];
        auto const& inter_dplc = indplc.frames[i];
        if (!inter_dplc.dplc.empty() && !inter_map.maps.empty()) {
            auto [end_map, dd] = inter_map.merge(inter_dplc).split();
            frames.push_back(end_map);
            outdplc.frames.push_back(dd.consolidate());
        } else if (!inter_dplc.dplc.empty()) {
            frames.emplace_back();
            outdplc.frames.push_back(inter_dplc.consolidate());
        } else {
            frames.push_back(inter_map);
            outdplc.frames.emplace_back();
        }
    }
}

void mapping_file::change_pal(
        int const source_palette, int const dest_palette) {
    for (auto& elem : frames) {
        elem.change_pal(source_palette, dest_palette);
    }
}

size_t mapping_file::size(int const version) const noexcept {
    return std::accumulate(
            frames.cbegin(), frames.cend(), 2 * frames.size(),
            [version](size_t size, auto const& elem) {
                return size + elem.size(version);
            });
}
