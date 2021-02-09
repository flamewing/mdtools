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
#include <mdtools/framemapping.hh>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <set>

using std::cout;
using std::endl;
using std::hex;
using std::ios;
using std::istream;
using std::map;
using std::ostream;
using std::pair;
using std::set;
using std::setfill;
using std::setw;
using std::uppercase;
using std::vector;

void frame_mapping::read(istream& in, int const ver) {
    size_t const cnt = ver == 1 ? Read1(in) : BigEndian::Read2(in);
    for (size_t i = 0; i < cnt; i++) {
        single_mapping sd{};
        sd.read(in, ver);
        maps.push_back(sd);
    }
}

void frame_mapping::write(ostream& out, int const ver) const {
    if (ver == 1) {
        Write1(out, maps.size());
    } else {
        BigEndian::Write2(out, maps.size());
    }
    for (auto const& elem : maps) {
        elem.write(out, ver);
    }
}

void frame_mapping::print() const {
    for (size_t i = 0; i < maps.size(); i++) {
        cout << "\tPiece $";
        boost::io::ios_all_saver flags(cout);
        cout << uppercase << hex << setfill('0') << setw(4) << i;
        cout << ":" << endl;
        flags.restore();
        maps[i].print();
    }
}

struct SingleMapCmp {
    bool operator()(
            single_mapping const& lhs, single_mapping const& rhs) const {
        return lhs.get_tile() < rhs.get_tile();
    }
};

struct SingleDPLCCmp {
    bool operator()(single_dplc const& lhs, single_dplc const& rhs) const {
        return lhs.get_tile() + lhs.get_cnt() < rhs.get_tile();
    }
};

void frame_mapping::split(frame_mapping const& src, frame_dplc& dplc) {
    // Coalesce the mappings tiles into tile ranges, reodering adjacent DPLCs
    // that are neighbours in art to coalesce the ranges as needed.
    vector<pair<size_t, size_t>> ranges;
    for (auto const& sd : src.maps) {
        size_t const ss = sd.get_tile();
        size_t const sz = sd.get_sx() * sd.get_sy();
        if (ranges.empty()) {
            // Happens only once. Hopefully, the compiler will pull this out of
            // the loop, as it happens right at the start of the loop.
            ranges.emplace_back(ss, sz);
        } else {
            pair<size_t, size_t>& last = ranges.back();
            if (last.first == ss + sz) {
                // Last DPLC comes right after us on the art file.
                // Coalesce ranges and set new start.
                last.first = ss;
                last.second += sz;
            } else if (last.first + last.second == ss) {
                // Last DPLC comes right before us on the art file.
                // Coalesce ranges, keeping old start.
                last.second += sz;
            } else {
                // Disjoint DPLCs. Add new one.
                ranges.emplace_back(ss, sz);
            }
        }
    }
    // TODO: maybe make multiple passes coalescing two entries of the above
    // vector in a similar fashion until nothing more changes. This would be
    // equivalent to sorting all sprite pieces by tile order for the DPLCs, but
    // with smaller overhead for mappings; in practice, this can only be useful
    // if the art was not sorted by tile order, a 1-click operation in SonMapEd.

    // Build VRAM map for coalesced ranges.
    map<size_t, size_t> vram_map;
    for (auto const& elem : ranges) {
        size_t const ss = elem.first;
        size_t const sz = elem.second;
        for (size_t i = ss; i < ss + sz; i++) {
            if (vram_map.find(i) == vram_map.end()) {
                vram_map.emplace(i, vram_map.size());
            }
        }
    }

    // Build DPLCs from VRAM map and coalesced ranges.
    set<single_dplc, SingleDPLCCmp> uniquedplcs;
    frame_dplc                      newdplc;
    for (auto const& elem : ranges) {
        size_t const ss = elem.first;
        size_t       sz = 1;
        while (vram_map.find(ss + sz) != vram_map.end()) {
            sz++;
        }
        single_dplc nd{};
        nd.set_tile(ss);
        nd.set_cnt(sz);
        auto const sit = uniquedplcs.find(nd);
        if (sit == uniquedplcs.end()) {
            newdplc.insert(nd);
            uniquedplcs.insert(nd);
        }
    }
    dplc.consolidate(newdplc);

    set<size_t> loaded_tiles;
    for (auto const& sd : src.maps) {
        single_mapping nn{};
        single_dplc    dd{};
        nn.split(sd, dd, vram_map);
        maps.push_back(nn);
    }
}

void frame_mapping::merge(frame_mapping const& src, frame_dplc const& dplc) {
    map<size_t, size_t> vram_map;
    dplc.build_vram_map(vram_map);

    for (auto const& sd : src.maps) {
        single_mapping nn{};
        nn.merge(sd, vram_map);
        maps.push_back(nn);
    }
}

void frame_mapping::change_pal(int const srcpal, int const dstpal) {
    for (auto& elem : maps) {
        elem.change_pal(srcpal, dstpal);
    }
}

bool frame_mapping::operator<(frame_mapping const& rhs) const {
    if (maps.size() < rhs.maps.size()) {
        return true;
    }
    if (maps.size() > rhs.maps.size()) {
        return false;
    }
    for (size_t ii = 0; ii < maps.size(); ii++) {
        if (maps[ii] < rhs.maps[ii]) {
            return true;
        }
        if (rhs.maps[ii] < maps[ii]) {
            return false;
        }
    }
    return false;
}
