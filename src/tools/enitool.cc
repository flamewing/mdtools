/*
 * Copyright (C) Flamewing 2015 <flamewing.sonic@gmail.com>
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

#include <getopt.h>
#include <mdcomp/bigendian_io.hh>
#include <mdcomp/enigma.hh>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::ios;
using std::ofstream;
using std::set;
using std::stringstream;

static void usage(char* prog) {
    cerr << "Usage: " << prog << "-s|--size {filename}" << endl << endl;
    cerr << "\tComputes uncompressed file size, in words, of enigma-compressed "
            "file."
         << endl
         << endl;
    cerr << "Usage: " << prog
         << "[-b num|--blacklist=num]* [-p num|--palette num] delta {filename}"
         << endl
         << endl;
    cerr << "\tDecompresses enigma mappings, adds the (unsigned) value "
            "specified by delta to every word"
         << endl;
    cerr << "\tof the decompressed file, then recompressed the result." << endl;
    cerr << "\t-p,--palette  \tIf specified, add the value (mod 4) to each "
            "tile's palette. "
         << endl;
    cerr << "\t-b,--blacklist\tCan be used 0 or more times; it is a list of "
            "values that ought "
         << endl;
    cerr << "\t              \tbe left untouched in the file." << endl << endl;
}

int main(int argc, char* argv[]) {
    constexpr static const std::array<option, 4> long_options{
            option{"size", no_argument, nullptr, 's'},
            option{"palette", required_argument, nullptr, 'p'},
            option{"blacklist", required_argument, nullptr, 'b'},
            option{nullptr, 0, nullptr, 0}};

    set<uint16_t> blacklist;
    bool          sizeOnly = false;
    uint16_t      paldelta = 0;

    while (true) {
        int option_index = 0;
        int c            = getopt_long(
                argc, argv, "sb:p:", long_options.data(), &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
        case 'p':
            if (optarg != nullptr) {
                paldelta = static_cast<uint16_t>(
                        (strtoul(optarg, nullptr, 0) & 3U) << 13U);
            }
            break;
        case 'b':
            if (optarg != nullptr) {
                blacklist.insert(static_cast<uint16_t>(
                        strtoul(optarg, nullptr, 0) & 0x7FFU));
            }
            break;
        case 's':
            sizeOnly = true;
            break;
        default:
            break;
        }
    }

    int numArgs = argc - optind;
    if (numArgs == 0 || (!sizeOnly && numArgs != 2)) {
        usage(argv[0]);
        return 1;
    }

    int32_t delta = 0;
    if (!sizeOnly) {
        delta = strtol(argv[optind++], nullptr, 0);
        if ((delta == 0) && (paldelta == 0U)) {
            cerr << "Adding zero to tile... aborting." << endl << endl;
            return 2;
        }
    }

    ifstream fin(argv[optind], ios::in | ios::binary);
    if (!fin.good()) {
        cerr << "Input file '" << argv[optind] << "' could not be opened."
             << endl
             << endl;
        return 3;
    }

    stringstream inbuffer(ios::in | ios::out | ios::binary);
    enigma::decode(fin, inbuffer);
    fin.close();

    if (sizeOnly) {
        inbuffer.seekg(0, ios::end);
        cout << inbuffer.tellg() / 2 << endl;
    } else {
        ofstream fout(argv[optind], ios::out | ios::binary);
        if (!fout.good()) {
            cerr << "Output file '" << argv[optind] << "' could not be opened."
                 << endl
                 << endl;
            return 4;
        }

        inbuffer.seekg(0);
        stringstream outbuffer(ios::in | ios::out | ios::binary);
        size_t       cnt = 0;
        while (true) {
            uint16_t val = BigEndian::Read2(inbuffer);
            if (!inbuffer.good()) {
                break;
            }
            uint32_t tile  = val & 0x7FFU;
            uint32_t pal   = val & 0x6000U;
            uint32_t flags = val & 0x9800U;
            if (blacklist.find(tile) == blacklist.cend()) {
                val = ((tile + delta) & 0x7FFU) | ((pal + paldelta) & 0x6000U)
                      | flags;
            }
            BigEndian::Write2(outbuffer, val);
            cnt++;
        }
        cout << cnt << endl;

        outbuffer.seekg(0);
        enigma::encode(outbuffer, fout);
    }

    return 0;
}
