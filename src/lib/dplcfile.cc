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
#include <mdtools/dplcfile.hh>

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
#include <numeric>

using std::ios;
using std::istream;
using std::map;
using std::ostream;
using std::vector;

dplc_file::dplc_file(istream& input, int const version) {
    input.seekg(0, ios::beg);
    vector<int16_t> offsets;

    int16_t term = BigEndian::Read<int16_t>(input);
    if (version != 4) {
        while (term == 0) {
            offsets.push_back(term);
            term = BigEndian::Read<int16_t>(input);
        }
    }
    offsets.push_back(term);
    while (input.tellg() < term) {
        auto newterm = BigEndian::Read<int16_t>(input);
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

void dplc_file::write(ostream& output, int const version, bool const null_first) const {
    map<frame_dplc, size_t> map_to_pos;
    map<size_t, frame_dplc> pos_to_map;

    size_t size = 2 * frames.size();

    if (null_first && version != 4 && !frames.empty()
        && frames.front().dplc.empty()) {
        map_to_pos.emplace(frames.front(), 0);
        pos_to_map.emplace(0, frames.front());
    }
    for (auto const& frame : frames) {
        if (auto const found = map_to_pos.find(frame); found != map_to_pos.end()) {
            BigEndian::Write2(output, found->second);
        } else {
            map_to_pos.emplace(frame, size);
            pos_to_map.emplace(size, frame);
            BigEndian::Write2(output, size);
            size += frame.size(version);
        }
    }
    for (auto const& [position, dplc] : pos_to_map) {
        if (position == size_t(output.tellp())) {
            dplc.write(output, version);
        } else if (position != 0U) {
            fmt::print(stderr, "Missed write at {}\n", output.tellp());
            dplc.print();
        }
    }
}

void dplc_file::print() const {
    fmt::print("{:=>80}\n", "");
    for (size_t i = 0; i < frames.size(); i++) {
        fmt::print("DPLC for frame ${:04X}:\n", i);
        frames[i].print();
    }
}

dplc_file dplc_file::consolidate() const {
    dplc_file output;
    for (auto const& elem : frames) {
        output.frames.push_back(elem.consolidate());
    }
    return output;
}

size_t dplc_file::size(int version) const noexcept {
    return std::accumulate(
            frames.cbegin(), frames.cend(), 2 * frames.size(),
            [version](size_t size, auto const& elem) {
                return size + elem.size(version);
            });
}
