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

#include "dplcfile.h"

void dplc_file::read(std::istream& in)
{
	in.seekg(0,std::ios::beg);
	
	std::vector<unsigned short> off;
	std::streampos const term = BigEndian::Read2(in);
	off.push_back(term);
	while (in.tellg() < term)
		off.push_back(BigEndian::Read2(in));
	
	for (std::vector<unsigned short>::const_iterator it = off.begin();
	     it != off.end(); ++it)
	{
		unsigned short pos = *it;
		in.clear();
		in.seekg(pos);
		frame_dplc sd;
		sd.read(in);
		frames.push_back(sd);
	}
}

void dplc_file::write(std::ostream& out) const
{
	size_t sz = 2 * frames.size();
	for (std::vector<frame_dplc>::const_iterator it = frames.begin();
	     it != frames.end(); ++it)
	{
		BigEndian::Write2(out,sz);
		sz += it->size();
	}
	for (std::vector<frame_dplc>::const_iterator it = frames.begin();
	     it != frames.end(); ++it)
		it->write(out);
}

void dplc_file::print() const
{
	std::cout << "================================================================================" << std::endl;
	for (size_t i = 0; i < frames.size(); i++)
	{
		std::cout << "DPLC for frame $";
		std::cout << std::uppercase   << std::hex << std::setfill('0') << std::setw(4) << i;
		std::cout << std::nouppercase	<< ":" << std::endl;
		frames[i].print();
	}
}

void dplc_file::consolidate(dplc_file const& src)
{
	for (std::vector<frame_dplc>::const_iterator it = src.frames.begin();
	     it != src.frames.end(); ++it)
	{
		frame_dplc nn;
		nn.consolidate(*it);
		frames.push_back(nn);
	}
}

void dplc_file::insert(frame_dplc const& val)
{
	frames.push_back(val);
}

frame_dplc const& dplc_file::get_dplc(size_t i) const
{
	return frames[i];
}

size_t dplc_file::size() const
{
	size_t sz = 2 * frames.size();
	for (std::vector<frame_dplc>::const_iterator it = frames.begin();
	     it != frames.end(); ++it)
	{
		frame_dplc const& sd = *it;
		sz += sd.size();
	}
	return sz;
}
