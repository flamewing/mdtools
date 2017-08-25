/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) Flamewing 2013-2015 <flamewing.sonic@gmail.com>
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

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <mdcomp/kosinski.h>

using namespace std;

static void usage() {
	cerr << "Usage: chunk_census {chunk_ID} {filename_list}" << endl;
	cerr << endl;
	cerr << "\tchunk_ID     \tThe chunk to scan for." << endl;
	cerr << "\tfilename_list\tList of Kosinski-compressed files with 128x128 blocks. Currently, S2 format only." << endl << endl;
}

int main(int argc, char *argv[]) {
	if (argc < 3) {
		usage();
		return 1;
	}

	uint64_t chunkid = strtoull(argv[1], nullptr, 0);
	for (int ii = 2; ii < argc; ii++) {
		ifstream fin(argv[ii], ios::in | ios::binary);
		unsigned cnt = 0;
		if (!fin.good()) {
			cerr << "Input file '" << argv[ii] << "' could not be opened." << endl;
		} else {
			stringstream sin(ios::in | ios::out | ios::binary);
			kosinski::decode(fin, sin);
			sin.seekg(0ul);
			size_t xpos = 0, ypos = 0;
			bool planeA = true;
			while (sin.good()) {
				int c = sin.get();
				if (size_t(c) == chunkid) {
					cnt++;
					cout << argv[ii] << ": chunk appears on plane "
					     << (planeA ? "A" : "B") << " @ (0x"
					     << hex << setw(4) << setfill('0') << xpos
					     << ", 0x"
					     << hex << setw(3) << setfill('0') << ypos
					     << ")" << endl;
				}
				xpos += 128;
				if (xpos == 128 * 128) {
					xpos = 0;
					planeA = !planeA;
					if (planeA) {
						ypos += 128;
					}
				}
			}
			cout << argv[ii] << ": " << dec << cnt << endl;
		}
	}

	return 0;
}

