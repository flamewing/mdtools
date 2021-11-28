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

#include <boost/io/ios_state.hpp>
#include <mdcomp/bigendian_io.hh>
#include <mdtools/dplcfile.hh>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>

using std::cerr;
using std::cout;
using std::endl;
using std::hex;
using std::ios;
using std::istream;
using std::map;
using std::ostream;
using std::setfill;
using std::setw;
using std::uppercase;
using std::vector;

void dplc_file::read(istream& in, int const ver) {
    in.seekg(0, ios::beg);

    vector<size_t> off;
    auto           term = static_cast<int16_t>(BigEndian::Read2(in));
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
    map<frame_dplc, size_t>            mappos;
    map<size_t, frame_dplc>            posmap;
    size_t                             sz = 2 * frames.size();
    vector<frame_dplc>::const_iterator it;
    it = frames.begin();

    if (nullfirst && ver != 4 && it != frames.end() && it->empty()) {
        mappos.emplace(*it, 0);
        posmap.emplace(0, *it);
    }
    for (; it != frames.end(); ++it) {
        if (auto const it2 = mappos.find(*it); it2 != mappos.end()) {
            BigEndian::Write2(out, it2->second);
        } else {
            mappos.emplace(*it, sz);
            posmap.emplace(sz, *it);
            BigEndian::Write2(out, sz);
            sz += it->size(ver);
        }
    }
    for (auto const& [pos, dplc] : posmap) {
        if (pos == size_t(out.tellp())) {
            dplc.write(out, ver);
        } else if (pos != 0U) {
            cerr << "Missed write at " << out.tellp() << endl;
            dplc.print();
        }
    }
}

void dplc_file::print() const {
    cout << "=================================================================="
            "=============="
         << endl;
    for (size_t i = 0; i < frames.size(); i++) {
        cout << "DPLC for frame $";
        boost::io::ios_all_saver flags(cout);
        cout << uppercase << hex << setfill('0') << setw(4) << i;
        cout << ":" << endl;
        flags.restore();
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
