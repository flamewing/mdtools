/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) Flamewing 2011-2013 <flamewing.sonic@gmail.com>
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

#include <iostream>
#include <iomanip>
#include "bigendian_io.h"

#include "framedplc.h"

using namespace std;

void frame_dplc::read(istream &in, int ver) {
	size_t cnt;
	if (ver == 1)
		cnt = Read1(in);
	else if (ver == 4)
		cnt = static_cast<short>(BigEndian::Read2(in)) + 1;
	else
		cnt = BigEndian::Read2(in);

	for (size_t i = 0; i < cnt; i++) {
		single_dplc sd;
		sd.read(in, ver);
		dplc.push_back(sd);
	}
}

void frame_dplc::write(ostream &out, int ver) const {
	if (ver == 1)
		Write1(out, dplc.size());
	else if (ver == 4)
		BigEndian::Write2(out, static_cast<unsigned short>(static_cast<short>(dplc.size()) - 1));
	else
		BigEndian::Write2(out, dplc.size());
	for (vector<single_dplc>::const_iterator it = dplc.begin();
	     it != dplc.end(); ++it)
		it->write(out, ver);
}

void frame_dplc::print() const {
	size_t ntiles = 0;
	for (vector<single_dplc>::const_iterator it = dplc.begin();
	     it != dplc.end(); ++it) {
		ntiles += it->get_cnt();
		it->print();
	}
	cout << nouppercase << "\tTile count: $";
	cout << uppercase   << hex << setfill('0') << setw(4) << ntiles;
	cout << endl;
}

void frame_dplc::consolidate(frame_dplc const &src) {
	if (!src.dplc.size())
		return;

	size_t start = src.dplc[0].get_tile(), size = 0;
	frame_dplc interm;
	for (vector<single_dplc>::const_iterator it = src.dplc.begin();
	     it != src.dplc.end(); ++it) {
		single_dplc const &sd = *it;
		if (sd.get_tile() != start + size) {
			single_dplc nn;
			nn.set_tile(start);
			nn.set_cnt(size);
			interm.dplc.push_back(nn);
			start = sd.get_tile();
			size = sd.get_cnt();
		} else
			size += sd.get_cnt();
	}
	if (size != 0) {
		single_dplc nn;
		nn.set_tile(start);
		nn.set_cnt(size);
		interm.dplc.push_back(nn);
	}

	dplc.clear();
	for (vector<single_dplc>::const_iterator it = interm.dplc.begin();
	     it != interm.dplc.end(); ++it) {
		size_t tile = it->get_tile(), sz = it->get_cnt();

		while (sz >= 16) {
			single_dplc nn;
			nn.set_tile(tile);
			nn.set_cnt(16);
			dplc.push_back(nn);
			sz -= 16;
			tile += 16;
		}
		if (sz) {
			single_dplc nn;
			nn.set_tile(tile);
			nn.set_cnt(sz);
			dplc.push_back(nn);
		}
	}
}

void frame_dplc::insert(single_dplc const &val) {
	dplc.push_back(val);
}

void frame_dplc::build_vram_map(map<size_t, size_t> &vram_map) const {
	for (vector<single_dplc>::const_iterator it = dplc.begin();
	        it != dplc.end(); ++it) {
		single_dplc const &sd = *it;
		size_t ss = sd.get_tile(), sz = sd.get_cnt();
		for (size_t i = ss; i < ss + sz; i++)
			vram_map.insert(pair<size_t, size_t>(vram_map.size(), i));
	}
}

bool frame_dplc::operator<(frame_dplc const &rhs) const {
	if (dplc.size() < rhs.dplc.size())
		return true;
	else if (dplc.size() > rhs.dplc.size())
		return false;
	for (size_t ii = 0; ii < dplc.size(); ii++) {
		if (dplc[ii] < rhs.dplc[ii])
			return true;
		else if (rhs.dplc[ii] < dplc[ii])
			return false;
	}
	return false;
}
