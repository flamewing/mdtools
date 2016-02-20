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

#include <iostream>
#include <iomanip>
#include "bigendian_io.h"

#include "singledplc.h"

using namespace std;

void single_dplc::read(istream &in, int ver) {
	tile = BigEndian::Read2(in);
	if (ver < 4) {
		cnt  = ((tile & 0xf000u) >> 12) + 1u;
		tile &= 0x0fff;
	} else {
		cnt  = (tile & 0x000fu) + 1u;
		tile = (tile & 0xfff0u) >> 4;
	}
}

void single_dplc::write(ostream &out, int ver) const {
	if (ver < 4) {
		BigEndian::Write2(out, ((cnt - 1) << 12) | tile);
	} else {
		BigEndian::Write2(out, (tile << 4) | (cnt - 1));
	}
}

void single_dplc::print() const {
	cout << nouppercase << "\tFirst tile: $";
	cout << uppercase   << hex << setfill('0') << setw(4) << tile;
	cout << nouppercase << "\tLast tile: $";
	cout << uppercase   << hex << setfill('0') << setw(4) << (tile + cnt - 1);
	cout << nouppercase << "\tNum tiles: $";
	cout << uppercase   << hex << setfill('0') << setw(4) << cnt;
	cout << endl;
}
