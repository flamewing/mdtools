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

#include "dplcfile.h"

using namespace std;

void dplc_file::read(istream &in, int ver) {
	in.seekg(0, ios::beg);

	vector<size_t> off;
	signed short term = static_cast<signed short>(BigEndian::Read2(in));
	if (ver != 4) {
		while (term == 0) {
			off.push_back(term);
			term = static_cast<signed short>(BigEndian::Read2(in));
		}
	}
	off.push_back(term);
	while (in.tellg() < term) {
		signed short newterm = static_cast<signed short>(BigEndian::Read2(in));
		if (newterm > 0 && newterm < term)
			term = newterm;
		off.push_back(newterm);
	}

	for (vector<size_t>::const_iterator it = off.begin(); it != off.end(); ++it) {
		size_t pos = *it;
		in.clear();
		in.seekg(pos);
		frame_dplc sd;
		sd.read(in, ver);
		frames.push_back(sd);
	}
}

void dplc_file::write(ostream &out, int ver, bool nullfirst) const {
	map<frame_dplc, size_t> mappos;
	map<size_t, frame_dplc> posmap;
	size_t sz = 2 * frames.size();
	vector<frame_dplc>::const_iterator it;
	it = frames.begin();

	if (nullfirst && ver != 4 && it != frames.end() && it->size() == 0) {
		mappos.insert(make_pair(*it, size_t(0)));
		posmap.insert(make_pair(size_t(0), *it));
	}
	for (; it != frames.end(); ++it) {
		map<frame_dplc, size_t>::iterator it2 = mappos.find(*it);
		if (it2 != mappos.end())
			BigEndian::Write2(out, it2->second);
		else {
			mappos.insert(make_pair(*it, sz));
			posmap.insert(make_pair(sz, *it));
			BigEndian::Write2(out, sz);
			sz += it->size(ver);
		}
	}
	for (map<size_t, frame_dplc>::iterator it2 = posmap.begin();
	        it2 != posmap.end(); ++it2)
		if (it2->first == size_t(out.tellp()))
			(it2->second).write(out, ver);
		else if (it2->first) {
			cerr << "Missed write at " << out.tellp() << endl;
			(it2->second).print();
		}

}

void dplc_file::print() const {
	cout << "================================================================================" << endl;
	for (size_t i = 0; i < frames.size(); i++) {
		cout << "DPLC for frame $";
		cout << uppercase   << hex << setfill('0') << setw(4) << i;
		cout << nouppercase   << ":" << endl;
		frames[i].print();
	}
}

void dplc_file::consolidate(dplc_file const &src) {
	for (vector<frame_dplc>::const_iterator it = src.frames.begin();
	     it != src.frames.end(); ++it) {
		frame_dplc nn;
		nn.consolidate(*it);
		frames.push_back(nn);
	}
}

void dplc_file::insert(frame_dplc const &val) {
	frames.push_back(val);
}

size_t dplc_file::size(int ver) const {
	size_t sz = 2 * frames.size();
	for (vector<frame_dplc>::const_iterator it = frames.begin();
	     it != frames.end(); ++it) {
		frame_dplc const &sd = *it;
		sz += sd.size(ver);
	}
	return sz;
}
