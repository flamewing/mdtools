/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * s2-ssedit
 * Copyright (C) Flamewing 2011 <flamewing.sonic@gmail.com>
 * 
 * s2-ssedit is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * s2-ssedit is distributed in the hope that it will be useful, but
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

void single_dplc::read(std::istream& in, int ver)
{
	tile = BigEndian::Read2(in);
	if (ver < 4)
	{
		cnt  = ((tile & 0xf000) >> 12) + 1;
		tile &= 0x0fff;
	}
	else
	{
		cnt  = (tile & 0x000f) + 1;
		tile = (tile & 0xfff0) >> 4;
	}
}

void single_dplc::write(std::ostream& out, int ver) const
{
	if (ver < 4)
		BigEndian::Write2(out, ((cnt - 1) << 12) | tile);
	else
		BigEndian::Write2(out, (tile << 4) | (cnt - 1));
}

void single_dplc::print() const
{
	std::cout << std::nouppercase << "\tFirst tile: $";
	std::cout << std::uppercase   << std::hex << std::setfill('0') << std::setw(4) << tile;
	std::cout << std::nouppercase << "\tLast tile: $";
	std::cout << std::uppercase   << std::hex << std::setfill('0') << std::setw(4) << (tile + cnt - 1);
	std::cout << std::nouppercase << "\tNum tiles: $";
	std::cout << std::uppercase   << std::hex << std::setfill('0') << std::setw(4) << cnt;
	std::cout << std::endl;
}
