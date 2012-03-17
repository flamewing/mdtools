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
#include <cstring>
#include <cstdlib>

#include "getopt.h"
#include <mdcomp/kosinski.h>
#include <mdcomp/nemesis.h>

static void usage()
{
	std::cerr << "Usage: recolor-art [-f|--format {unc|nem|kos}] [-m|--moduled] {-s|--source-color clr1} {-d|--dest-color clr2} {-b|--blacklist clr3} {input_art} {output_art}" << std::endl;
	std::cerr << "\tRecolors the art file, changing palette index clr1 to clr2" << std::endl << std::endl;
}

enum Formats
{
	eUncompressed = 0,
	eNemesis      = 1,
	eKosinski     = 2
};

struct Tile
{
	unsigned char tiledata[64];
	bool read(std::istream& in)
	{
		for (size_t i = 0; i < sizeof(tiledata); i += 2)
		{
			size_t col = in.get();
			if (!in.good())
				return false;
			tiledata[i + 0] = col & 0x0f;
			tiledata[i + 1] = (col & 0xf0) >> 4;
		}
		return true;
	}
	bool blacklisted(unsigned char const bll)
	{
		for (size_t i = 0; i < sizeof(tiledata); i++)
			if (tiledata[i] == bll)
				return true;
		return false;
	}
	void remap(unsigned char const c1l, unsigned char const c2l)
	{
		for (size_t i = 0; i < sizeof(tiledata); i++)
			if (tiledata[i] == c1l)
				tiledata[i] = c2l;
	}
	void write(std::ostream& out)
	{
		for (size_t i = 0; i < sizeof(tiledata); i += 2)
			out.put(tiledata[i] | (tiledata[i + 1] << 4));
	}
};

void recolor(std::istream& in, std::ostream& out, int const srccolor, int const dstcolor, int const blacklist)
{
	unsigned char const c1l = srccolor  & 0xf,
	                    c2l = dstcolor  & 0xf,
	                    bll = blacklist & 0xf;

	Tile tile;
	while (true)
	{
		if (!tile.read(in))
			break;
		if (blacklist < 0 || !tile.blacklisted(bll))
			tile.remap(c1l, c2l);
		tile.write(out);
	}
}

int main(int argc, char *argv[])
{
	static struct option long_options[] = {
		{"format"      , required_argument, 0, 'f'},
		{"moduled"     , no_argument      , 0, 'm'},
		{"source-color", required_argument, 0, 's'},
		{"dest-color"  , required_argument, 0, 'd'},
		{"blacklist"   , required_argument, 0, 'b'},
		{0, 0, 0, 0}
	};

	bool moduled = false;
	int srccolor = -1, dstcolor = -1, blacklist = -1;
	Formats fmt = eUncompressed;

	while (true)
	{
		int option_index = 0;
		int c = getopt_long(argc, argv, "f:ms:d:b:",
                            long_options, &option_index);
		if (c == -1)
			break;
		
		switch (c)
		{
			case 'f':
				if (!std::strcmp(optarg, "unc"))
					fmt = eUncompressed;
				else if (!std::strcmp(optarg, "nem"))
					fmt = eNemesis;
				else if (!std::strcmp(optarg, "kos"))
					fmt = eKosinski;
				else
				{
					usage();
					return 1;
				}
				break;
				
			case 'm':
				moduled = true;
				break;

			case 's':
				srccolor = std::strtoul(optarg, 0, 0);
				break;

			case 'd':
				dstcolor = std::strtoul(optarg, 0, 0);
				break;

			case 'b':
				blacklist = std::strtoul(optarg, 0, 0);
				break;
		}
	}

	if (argc - optind < 2 || srccolor < 0 || dstcolor < 0)
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

	std::stringstream sin (std::ios::in|std::ios::out|std::ios::binary),
	                  sout(std::ios::in|std::ios::out|std::ios::binary);

	fin.seekg(0);
	if (fmt == eUncompressed)
		sin << fin.rdbuf();
	else if (fmt == eNemesis)
		nemesis::decode(fin, sin, 0);
	else // if (fmt == eKosinski)
		kosinski::decode(fin, sin, 0, moduled);

	fin.close();
	sin.seekg(0);
	recolor(sin, sout, srccolor, dstcolor, blacklist);

	std::fstream fout(argv[optind+1], std::ios::in|std::ios::out|std::ios::binary|std::ios::trunc);
	if (!fout.good())
	{
		std::cerr << "Output file '" << argv[optind+1] << "' could not be opened." << std::endl << std::endl;
		return 3;
	}

	sout.seekg(0);

	if (fmt == eUncompressed)
		fout << sout.rdbuf();
	else if (fmt == eNemesis)
		nemesis::encode(sout, fout);
	else // if (fmt == eKosinski)
		kosinski::encode(sout, fout, 8192, 256, moduled);

	fout.close();
}
 
