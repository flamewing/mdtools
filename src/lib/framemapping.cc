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

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include "bigendian_io.h"

#include "framemapping.h"

using namespace std;

void frame_mapping::read(istream &in, int ver) {
	size_t cnt = ver == 1 ? Read1(in) : BigEndian::Read2(in);
	for (size_t i = 0; i < cnt; i++) {
		single_mapping sd;
		sd.read(in, ver);
		maps.push_back(sd);
	}
}

void frame_mapping::write(ostream &out, int ver) const {
	if (ver == 1) {
		Write1(out, maps.size());
	} else {
		BigEndian::Write2(out, maps.size());
	}
	for (const auto & elem : maps) {
		elem.write(out, ver);
	}
}

void frame_mapping::print() const {
	for (size_t i = 0; i < maps.size(); i++) {
		cout << "\tPiece $";
		cout << uppercase   << hex << setfill('0') << setw(4) << i;
		cout << nouppercase   << ":" << endl;
		maps[i].print();
	}
}

struct SingleMapCmp {
	bool operator()(single_mapping const &lhs, single_mapping const &rhs) {
		return lhs.get_tile() < rhs.get_tile();
	}
};

struct SingleDPLCCmp {
	bool operator()(single_dplc const &lhs, single_dplc const &rhs) {
		return lhs.get_tile() + lhs.get_cnt() < rhs.get_tile();
	}
};

void frame_mapping::split(frame_mapping const &src, frame_dplc &dplc) {
	// Coalesce the mappings tiles into tile ranges, reodering adjacent DPLCs
	// that are neighbours in art to coalesce the ranges as needed.
	vector<pair<size_t, size_t>> ranges;
	for (const auto & sd : src.maps) {
		size_t ss = sd.get_tile(), sz = sd.get_sx() * sd.get_sy();
		if (ranges.empty()) {
			// Happens only once. Hopefully, the compiler will pull this out of
			// the loop, as it happens right at the start of the loop.
			ranges.push_back(make_pair(ss, sz));
		} else {
			pair<size_t, size_t> &last = ranges.back();
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
				ranges.push_back(make_pair(ss, sz));
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
	for (vector<pair<size_t, size_t>>::const_iterator it = ranges.begin();
	        it != ranges.end(); ++it) {
		size_t ss = it->first, sz = it->second;
		for (size_t i = ss; i < ss + sz; i++) {
			if (vram_map.find(i) == vram_map.end()) {
				vram_map.insert(pair<size_t, size_t>(i, vram_map.size()));
			}
		}
	}

	// Build DPLCs from VRAM map and coalesced ranges.
	set<single_dplc, SingleDPLCCmp> uniquedplcs;
	frame_dplc newdplc;
	for (vector<pair<size_t, size_t>>::const_iterator it = ranges.begin();
	        it != ranges.end(); ++it) {
		size_t ss = it->first, sz = 1;
		while (vram_map.find(ss + sz) != vram_map.end()) {
			sz++;
		}
		single_dplc nd;
		nd.set_tile(ss);
		nd.set_cnt(sz);
		auto sit = uniquedplcs.find(nd);
		if (sit == uniquedplcs.end()) {
			newdplc.insert(nd);
			uniquedplcs.insert(nd);
		}
	}
	dplc.consolidate(newdplc);

	set<size_t> loaded_tiles;
	for (const auto & sd : src.maps) {
		single_mapping nn;
		single_dplc dd;
		nn.split(sd, dd, vram_map);
		maps.push_back(nn);
	}
}

void frame_mapping::merge(frame_mapping const &src, frame_dplc const &dplc) {
	map<size_t, size_t> vram_map;
	dplc.build_vram_map(vram_map);

	for (const auto & sd : src.maps) {
		single_mapping nn;
		nn.merge(sd, vram_map);
		maps.push_back(nn);
	}
}

void frame_mapping::change_pal(int srcpal, int dstpal) {
	for (auto & elem : maps) {
		elem.change_pal(srcpal, dstpal);
	}
}

bool frame_mapping::operator<(frame_mapping const &rhs) const {
	if (maps.size() < rhs.maps.size()) {
		return true;
	} else if (maps.size() > rhs.maps.size()) {
		return false;
	}
	for (size_t ii = 0; ii < maps.size(); ii++) {
		if (maps[ii] < rhs.maps[ii]) {
			return true;
		} else if (rhs.maps[ii] < maps[ii]) {
			return false;
		}
	}
	return false;
}

