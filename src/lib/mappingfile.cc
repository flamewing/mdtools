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

#include "mappingfile.h"

void mapping_file::read(std::istream& in, bool s2)
{
	in.seekg(0,std::ios::beg);
	
	std::vector<size_t> off;
	std::streampos const term = BigEndian::Read2(in);
	off.push_back(term);
	while (in.tellg() < term)
		off.push_back(BigEndian::Read2(in));
	
	for (std::vector<size_t>::const_iterator it = off.begin(); it != off.end(); ++it)
	{
		size_t pos = *it;
		in.clear();
		in.seekg(pos);
		frame_mapping sd;
		sd.read(in, s2);
		frames.push_back(sd);
	}
}

void mapping_file::write(std::ostream& out, bool s2) const
{
	size_t sz = 2 * frames.size();
	for (std::vector<frame_mapping>::const_iterator it = frames.begin();
	     it != frames.end(); ++it)
	{
		BigEndian::Write2(out, sz);
		sz += it->size(s2);
	}
	for (std::vector<frame_mapping>::const_iterator it = frames.begin();
	     it != frames.end(); ++it)
		it->write(out, s2);
}

void mapping_file::print() const
{
	std::cout << "================================================================================" << std::endl;
	for (size_t i = 0; i < frames.size(); i++)
		{
		std::cout << "Mappings for frame $";
		std::cout << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << i;
		std::cout << std::nouppercase << ":" << std::endl;
		frames[i].print();
		}
}

void mapping_file::split(mapping_file const& src, dplc_file& dplc)
{
	for (std::vector<frame_mapping>::const_iterator it = src.frames.begin();
	     it != src.frames.end(); ++it)
	{
		frame_mapping nn;
		frame_dplc interm, dd;
		nn.split(*it, interm);
		dd.consolidate(interm);
		frames.push_back(nn);
		dplc.insert(dd);
	}
}

void mapping_file::merge(mapping_file const& src, dplc_file const& dplc)
{
	for (size_t i = 0; i < src.frames.size(); i++)
	{
		frame_mapping nn;
		nn.merge(src.frames[i], dplc.get_dplc(i));
		frames.push_back(nn);
	}
}

void mapping_file::change_pal(int srcpal, int dstpal)
{
	for (std::vector<frame_mapping>::iterator it = frames.begin();
	     it != frames.end(); ++it)
		it->change_pal(srcpal, dstpal);
}

size_t mapping_file::size(bool s2) const
{
	size_t sz = 2 * frames.size();
	for (std::vector<frame_mapping>::const_iterator it = frames.begin();
	     it != frames.end(); ++it)
		sz += it->size(s2);
	return sz;
}


