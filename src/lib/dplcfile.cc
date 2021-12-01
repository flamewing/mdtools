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

void dplc_file::read(istream& in, int const ver) {
    in.seekg(0, ios::beg);
    vector<int16_t> off;

    auto term = static_cast<int16_t>(BigEndian::Read2(in));
    if (ver != 4) {
        while (term == 0) {
            off.push_back(term);
            term = static_cast<int16_t>(BigEndian::Read2(in));
        }
    }
    off.push_back(term);
    while (in.tellg() < term) {
        auto newterm = static_cast<int16_t>(BigEndian::Read2(in));
        if (newterm > 0 && newterm < term) {
            term = newterm;
        }
        off.push_back(newterm);
    }

    for (auto const pos : off) {
        in.clear();
        in.seekg(pos);
        frame_dplc sd;
        sd.read(in, ver);
        frames.push_back(sd);
    }
}

void dplc_file::write(ostream& out, int const ver, bool const nullfirst) const {
    map<frame_dplc, size_t> mappos;
    map<size_t, frame_dplc> posmap;

    size_t sz = 2 * frames.size();

    if (nullfirst && ver != 4 && !frames.empty()
        && frames.front().dplc.empty()) {
        mappos.emplace(frames.front(), 0);
        posmap.emplace(0, frames.front());
    }
    for (auto const& frame : frames) {
        if (auto const it2 = mappos.find(frame); it2 != mappos.end()) {
            BigEndian::Write2(out, it2->second);
        } else {
            mappos.emplace(frame, sz);
            posmap.emplace(sz, frame);
            BigEndian::Write2(out, sz);
            sz += frame.size(ver);
        }
    }
    for (auto const& [pos, dplc] : posmap) {
        if (pos == size_t(out.tellp())) {
            dplc.write(out, ver);
        } else if (pos != 0U) {
            fmt::print(stderr, "Missed write at {}\n", out.tellp());
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

void dplc_file::consolidate(dplc_file const& src) {
    for (auto const& elem : src.frames) {
        frame_dplc nn;
        nn.consolidate(elem);
        frames.push_back(nn);
    }
}

void dplc_file::insert(frame_dplc const& val) {
    frames.push_back(val);
}

size_t dplc_file::size(int ver) const noexcept {
    return std::accumulate(
            frames.cbegin(), frames.cend(), 2 * frames.size(),
            [ver](size_t sz, auto const& elem) {
                return sz + elem.size(ver);
            });
}
