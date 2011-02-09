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
#include <map>
#include <set>
#include "bigendian_io.h"

#include "framemapping.h"

void frame_mapping::read(std::istream& in, bool s2)
{
	size_t cnt = BigEndian::Read2(in);
	for (size_t i = 0; i < cnt; i++)
	{
		single_mapping sd;
		sd.read(in, s2);
		maps.push_back(sd);
	}
}

void frame_mapping::write(std::ostream& out, bool s2) const
{
	BigEndian::Write2(out,maps.size());
	for (std::vector<single_mapping>::const_iterator it = maps.begin();
	     it != maps.end(); ++it)
		it->write(out, s2);
}

void frame_mapping::print() const
{
	for (size_t i = 0; i < maps.size(); i++)
	{
		std::cout << "\tPiece $";
		std::cout << std::uppercase   << std::hex << std::setfill('0') << std::setw(4) << i;
		std::cout << std::nouppercase	<< ":" << std::endl;
		maps[i].print();
	}
}

void frame_mapping::split(frame_mapping const& src, frame_dplc& dplc)
{
	std::map<size_t,size_t> vram_map;
	for (std::vector<single_mapping>::const_iterator it = src.maps.begin();
	     it != src.maps.end(); ++it)
	{
		single_mapping const& sd = *it;
		size_t ss = sd.get_tile(), sz = sd.get_sx() * sd.get_sy();
		for (size_t i = ss; i < ss + sz; i++)
			if (vram_map.find(i) == vram_map.end())
				vram_map.insert(std::pair<size_t,size_t>(i, vram_map.size()));
	}
	
	std::set<size_t> loaded_tiles;
	for (std::vector<single_mapping>::const_iterator it = src.maps.begin();
	     it != src.maps.end(); ++it)
	{
		single_mapping const& sd = *it;
		single_mapping nn;
		single_dplc dd;
		nn.split(sd, dd, vram_map);
		maps.push_back(nn);
		size_t ss = dd.get_tile(), sz = dd.get_cnt();
		for (size_t i = ss; i < ss + sz; i++)
		{
			size_t j = i;
			while ((j < ss + sz) && (loaded_tiles.find(j) == loaded_tiles.end()))
				loaded_tiles.insert(j++);
			if (j != i)
			{
				single_dplc nd;
				nd.set_tile(i);
				nd.set_cnt(j - i);
				dplc.insert(nd);
				i = j;
			}
		}
	}
}

void frame_mapping::merge(frame_mapping const& src, frame_dplc const& dplc)
{
	std::map<size_t,size_t> vram_map;
	dplc.build_vram_map(vram_map);
	
	for (std::vector<single_mapping>::const_iterator it = src.maps.begin();
	     it != src.maps.end(); ++it)
	{
		single_mapping const& sd = *it;
		single_mapping nn;
		nn.merge(sd, vram_map);
		maps.push_back(nn);
	}
}

void frame_mapping::change_pal(int srcpal, int dstpal)
{
	for (std::vector<single_mapping>::iterator it = maps.begin();
	     it != maps.end(); ++it)
		it->change_pal(srcpal, dstpal);
}

