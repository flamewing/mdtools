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

#include "singlemapping.h"

void single_mapping::read(std::istream &in, int ver) {
	yy = static_cast<signed char>(Read1(in));
	sx = Read1(in);
	sy = ((sx & 0xc) >> 2) + 1;
	sx = (sx & 0x3) + 1;
	tile = BigEndian::Read2(in);
	flags = ((tile & 0xf800u) >> 8u);
	tile &= 0x07ffu;
	if (ver == 2)
		in.ignore(2);
	if (ver == 1)
		xx = static_cast<signed char>(Read1(in));
	else
		xx = BigEndian::Read2(in);
}

void single_mapping::write(std::ostream &out, int ver) const {
	Write1(out, static_cast<unsigned char>(yy));
	Write1(out, ((sy - 1) << 2) | (sx - 1));
	BigEndian::Write2(out, (flags << 8) | tile);
	if (ver == 2)
		BigEndian::Write2(out, (flags << 8) | (tile >> 1));
	if (ver == 1)
		Write1(out, static_cast<unsigned char>(xx));
	else
		BigEndian::Write2(out, xx);
}

void single_mapping::print() const {
	std::cout << std::nouppercase << "\t\tPosition: (x,y) = (";
	std::cout << std::dec << std::setfill(' ') << std::setw(4) << static_cast<short>(xx);
	std::cout << ",";
	std::cout << std::dec << std::setfill(' ') << std::setw(4) << static_cast<short>(yy);
	std::cout << ")\tSize: (x,y) = (";
	std::cout << std::dec << std::setfill(' ') << std::setw(4) << static_cast<short>(sx);
	std::cout << ",";
	std::cout << std::dec << std::setfill(' ') << std::setw(4) << static_cast<short>(sy);
	std::cout << ")" << std::endl;
	std::cout << std::nouppercase << "\t\tFirst tile: $";
	std::cout << std::uppercase   << std::hex << std::setfill('0') << std::setw(4) << tile;
	std::cout << std::nouppercase << "\tLast tile: $";
	std::cout << std::uppercase   << std::hex << std::setfill('0') << std::setw(4) << (tile + sx * sy - 1);
	std::cout << std::nouppercase << "\tFlags: ";
	if ((flags & 0x80u) != 0) {
		std::cout << "foreground";
		if ((flags & 0x78u) != 0)
			std::cout << "|";
	}
	if ((flags & 0x60u) != 0) {
		std::cout << "palette+" << std::dec << ((flags & 0x60u) >> 5u);
		if ((flags & 0x18u) != 0)
			std::cout << "|";
	}
	if ((flags & 0x08u) != 0) {
		std::cout << "flip_x";
		if ((flags & 0x10u) != 0)
			std::cout << "|";
	}
	if ((flags & 0x10u) != 0)
		std::cout << "flip_y";
	std::cout << std::endl;
}

void single_mapping::split(single_mapping const &src, single_dplc &dplc, std::map<size_t, size_t>& vram_map) {
	xx = src.xx;
	yy = src.yy;
	sx = src.sx;
	sy = src.sy;
	flags = src.flags;
	tile = vram_map[src.tile];
	dplc.set_cnt(sx * sy);
	dplc.set_tile(src.tile);
}

void single_mapping::merge(single_mapping const &src, std::map<size_t, size_t>& vram_map) {
	xx = src.xx;
	yy = src.yy;
	sx = src.sx;
	sy = src.sy;
	flags = src.flags;
	tile = vram_map[src.tile];
}

void single_mapping::change_pal(int srcpal, int dstpal) {
	if ((flags & 0x60) == srcpal)
		flags = (flags & 0x9f) | dstpal;
}
