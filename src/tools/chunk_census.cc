/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) Flamewing 2013 <flamewing.sonic@gmail.com>
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
#include <fstream>
#include <sstream>
#include <cstdlib>

#include <mdcomp/kosinski.h>

static void usage() {
	std::cerr << "Usage: chunk_census {chunk_ID} {filename_list}" << std::endl;
	std::cerr << std::endl;
	std::cerr << "\tchunk_ID     \tThe chunk to scan for." << std::endl;
	std::cerr << "\tfilename_list\tList of Kosinski-compressed files with 128x128 blocks. Currently, S2 format only." << std::endl << std::endl;
}

int main(int argc, char *argv[]) {
	std::streamsize pointer = 0, slidewin = 8192, reclen = 256, modulesize = 0x1000;
	if (argc < 3) {
		usage();
		return 1;
	}

	unsigned long chunkid = strtoul(argv[1], 0, 0);
	for (size_t ii = 2; ii < argc; ii++) {
		std::ifstream fin(argv[ii], std::ios::in | std::ios::binary);
		unsigned cnt = 0;
		if (!fin.good()) {
			std::cerr << "Input file '" << argv[ii] << "' could not be opened." << std::endl;
		} else {
			std::stringstream sin(std::ios::in | std::ios::out | std::ios::binary);
			kosinski::decode(fin, sin);
			sin.seekg(0ul);
			size_t xpos = 0, ypos = 0;
			bool planeA = true;
			while (sin.good()) {
				int c = sin.get();
				if (c == chunkid) {
					cnt++;
					std::cout << argv[ii] << ": chunk appears on plane "
					          << (planeA ? "A" : "B") << " @ (0x"
					          << std::hex << std::setw(4) << std::setfill('0') << xpos
					          << ", 0x"
					          << std::hex << std::setw(3) << std::setfill('0') << ypos
					          << ")" << std::endl;
				}
				xpos += 128;
				if (xpos == 128 * 128) {
					xpos = 0;
					planeA = !planeA;
					if (planeA)
						ypos += 128;
				}
			}
			std::cout << argv[ii] << ": " << std::dec << cnt << std::endl;
		}
	}

	return 0;
}

