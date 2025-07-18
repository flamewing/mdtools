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

#include <mdcomp/kosinski.hh>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

using std::cerr;
using std::cout;
using std::dec;
using std::endl;
using std::hex;
using std::ifstream;
using std::ios;
using std::ofstream;
using std::setfill;
using std::setw;
using std::stringstream;

static void usage() {
    cerr << "Usage: chunk_census {chunk_ID} {filename_list}" << endl;
    cerr << endl;
    cerr << "\tchunk_ID     \tThe chunk to scan for." << endl;
    cerr << "\tfilename_list\tList of Kosinski-compressed files with 128x128 "
            "blocks. Currently, S2 format only."
         << endl
         << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        usage();
        return 1;
    }

    uint64_t chunkid = strtoull(argv[1], nullptr, 0);
    for (int ii = 2; ii < argc; ii++) {
        ifstream input(argv[ii], ios::in | ios::binary);
        unsigned count = 0;
        if (!input.good()) {
            cerr << "Input file '" << argv[ii] << "' could not be opened."
                 << endl;
        } else {
            stringstream decoded(ios::in | ios::out | ios::binary);
            kosinski::decode(input, decoded);
            decoded.seekg(0UL);
            size_t xpos   = 0;
            size_t ypos   = 0;
            bool   planeA = true;
            while (decoded.good()) {
                int value = decoded.get();
                if (size_t(value) == chunkid) {
                    count++;
                    cout << argv[ii] << ": chunk appears on plane "
                         << (planeA ? 'A' : 'B') << " @ (0x" << hex << setw(4)
                         << setfill('0') << xpos << ", 0x" << hex << setw(3)
                         << setfill('0') << ypos << ")" << endl;
                }
                xpos += 128;
                if (xpos == 128 * 128) {
                    xpos   = 0;
                    planeA = !planeA;
                    if (planeA) {
                        ypos += 128;
                    }
                }
            }
            cout << argv[ii] << ": " << dec << count << endl;
        }
    }

    return 0;
}
