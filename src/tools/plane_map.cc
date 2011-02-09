// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with main.c; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <map>

#include "getopt.h"
#include "bigendian_io.h"
#include <mdcomp/enigma.h>

static void usage()
{
	std::cerr << "Usage: plane_map [-x|--extract [{pointer}]] [--sonic2] {input_filename} {output_filename} {width} {height}" << std::endl;
	std::cerr << "\tPerforms a plane map operation on {input_filename}. {input_filename} is assumed to be a linear array which must be mapped into an area of {width} x {height} cells of 8x8 pixels." << std::endl;
	std::cerr << "\t{output_filename} is a mappings file." << std::endl;
	std::cerr << std::endl;
	std::cerr << "\t-x,--extract\tAssume input file is Enigma-compressed and decode it before doing the plane map. File is read starting from {pointer}." << std::endl;
	std::cerr << "\t--sonic2\t{output_filename} is in Sonic 2 mappings format. Default to non-Sonic 2 format." << std::endl << std::endl;
	std::cerr << "Usage: plane_map -u [-c|--compress] [--sonic2] {input_filename} {output_filename}" << std::endl;
	std::cerr << "\tDoes the reverse operation of the above usage (without -u). {input_filename} must contain only single-tile pieces." << std::endl;
	std::cerr << std::endl;
	std::cerr << "\t-c,--compress\t{output_filename} is Enigma compressed." << std::endl;
	std::cerr << "\t--sonic2\t{input_filename} is in Sonic 2 mappings format. Default to non-Sonic 2 format." << std::endl << std::endl;
}

static void plane_map(std::istream& src, std::ostream& dst, size_t w, size_t h,
                      std::streamsize pointer, bool sonic2)
{
	src.seekg(0, std::ios::end);
	std::streamsize sz = std::streamsize(src.tellg()) - pointer;
	src.seekg(pointer);

	size_t nframes = sz / (2 * w * h);
	size_t off = 2 * nframes;

	for (size_t n = 0; n < nframes; n++, off += 2 + 8 * w * h)
		BigEndian::Write2(dst,static_cast<unsigned short>(off));

	for (size_t n = 0; n < nframes; n++, off += 2)
	{
		BigEndian::Write2(dst, w * h);
		for (size_t j = 0; j < h; j++)
		{
			signed char y_pos = (j - h/2) << 3;
			for (size_t i = 0; i < w; i++)
			{
				dst.put(static_cast<char>(y_pos));
				dst.put(static_cast<char>(0x00));
				unsigned short v = BigEndian::Read2(src);
				BigEndian::Write2(dst,v);
				if (sonic2)
					BigEndian::Write2(dst,(v & 0xf800) | ((v & 0x07ff) >> 1));
				BigEndian::Write2(dst,static_cast<unsigned short>((i - w/2) << 3));
			}
		}
	}
}

struct Position
	{
	signed short x;
	signed char y;
	bool operator<(Position const& other) const
		{
		return (y < other.y) || (y == other.y && x < other.x);
		}
	};

typedef std::pair<Position const,unsigned short> Enigma_entry;
typedef std::map<Position,unsigned short> Enigma_map;

static void plane_unmap(std::istream& src, std::ostream& dst,
                      std::streamsize pointer, bool sonic2)
{
	std::streamsize next_loc = src.tellg();
	std::streamsize last_loc = BigEndian::Read2(src);
	src.seekg(0,std::ios::end);
	std::streamsize end_loc = 0, file_len = src.tellg();
	src.seekg(next_loc);
	
	while (next_loc < last_loc)
	{
		src.seekg(next_loc);
	
		size_t offset = BigEndian::Read2(src);
		next_loc = src.tellg();
		if (next_loc == last_loc)
			end_loc = file_len;
		else
			end_loc = BigEndian::Read2(src);
			
		src.seekg(offset);
	
		size_t count = BigEndian::Read2(src);
		Enigma_map engfile;
		for (size_t i = 0; i < count; i++)
		{
			Position pos;
			pos.y = static_cast<signed char>(src.get() & 0xff);
			src.ignore(1);
			unsigned short v = BigEndian::Read2(src);
			if (sonic2)
				src.ignore(2);
			pos.x = static_cast<signed short>(BigEndian::Read2(src));
			engfile.insert(Enigma_entry(pos, v));
		}
	
		for (Enigma_map::iterator it = engfile.begin(); it != engfile.end(); ++it)
		{
			unsigned short v = it->second;
			BigEndian::Write2(dst,v);
		}
	}
}

int main(int argc, char *argv[])
{
	int sonic2 = 0;
	static struct option long_options[] = {
		{"extract"  , optional_argument, 0, 'x'},
		{"sonic2"   , no_argument      , &sonic2, 1},
		{"compress" , no_argument      , 0, 'c'},
		{0, 0, 0, 0}
	};

	bool extract = false, compress = false, unmap = false;
	std::streamsize pointer = 0;

	while (true)
	{
		int option_index = 0;
		int c = getopt_long(argc, argv, "ux::c",
                            long_options, &option_index);
		if (c == -1)
			break;
		
		switch (c)
		{
			case 'x':
				extract = true;
				if (optarg)
					pointer = strtoul(optarg, 0, 0);
				break;

			case 'c':
				compress = true;
				break;

			case 'u':
				unmap = true;
				break;
		}
	}

	if (argc - optind < 2 || (!unmap && argc - optind < 4))
	{
		usage();
		return 1;
	}

	std::ifstream fin(argv[optind], std::ios::in|std::ios::binary);
	if (!fin.good())
	{
		std::cerr << "Input file '" << argv[optind] << "' could not be opened." << std::endl << std::endl;
		return 2;
	}

	std::ofstream fout(argv[optind+1], std::ios::out|std::ios::binary);
	if (!fout.good())
	{
		std::cerr << "Output file '" << argv[optind+1] << "' could not be opened." << std::endl << std::endl;
		return 3;
	}

	if (unmap)
	{
		std::stringstream fbuf(std::ios::in|std::ios::out|std::ios::binary|std::ios::trunc);
		if (compress)
		{
			plane_unmap(fin, fbuf, 0      , sonic2);
			enigma::encode(fbuf, fout);
		}
		else
			plane_unmap(fin, fout, pointer, sonic2);
	}
	else
	{
		std::stringstream fbuf(std::ios::in|std::ios::out|std::ios::binary|std::ios::trunc);
		size_t w = strtoul(argv[optind+2], 0, 0), h = strtoul(argv[optind+3], 0, 0);
		if (!w || !h)
		{
			std::cerr << "Invalid height or width for plane mapping." << std::endl << std::endl;
			return 4;
		}
		if (extract)
		{
			enigma::decode(fin, fbuf, pointer);
			plane_map(fbuf, fout, w, h, 0      , sonic2);
		}
		else
			plane_map(fin , fout, w, h, pointer, sonic2);
	}
	return 0;
}

 
