/*
 * Copyright (C) Flamewing 2011-2015 <flamewing.sonic@gmail.com>
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
#include <mdtools/fmvoice.hh>

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::ios;
using std::istream;
using std::ofstream;
using std::ostream;
using std::streamsize;
using std::string;
using std::stringstream;

static void usage() {
    cerr << "Usage: voice_dumper [-x|--extract [{pointer}]] {-v|--sonicver} "
            "{version} {input_filename} {num_voices}"
         << endl;
    cerr << endl;
    cerr << "\t-x,--extract \tExtract {num_voices} from {pointer} address in "
            "{input_filename}."
         << endl
         << "\t             \tIf omitted, {pointer} is assumed to be zero."
         << endl;
    cerr << "\t-v,--sonicver\tSets Sonic version to {version}. This also sets "
            "underlying"
         << endl
         << "\t             \tSMPS type. {version} can be '1' Sonic 1, '2' for "
            "Sonic 2 or"
         << endl
         << "\t             \t'3' for Sonic 3 or Sonic & Knuckles, or '4' for "
            "Sonic 3D Blast."
         << endl
         << endl;
}

int main(int argc, char* argv[]) {
    constexpr static const std::array<option, 3> long_options
            = {option{"extract", required_argument, nullptr, 'x'},
               option{"sonicver", required_argument, nullptr, 'v'},
               option{nullptr, 0, nullptr, 0}};

    int64_t pointer  = 0;
    int64_t sonicver = -1;

    while (true) {
        int option_index = 0;
        int c            = getopt_long(
                argc, argv, "x:v:", long_options.data(), &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
        case 'x':
            pointer = strtol(optarg, nullptr, 0);
            break;

        case 'v':
            sonicver = strtol(optarg, nullptr, 0);
            break;
        default:
            break;
        }
    }

    if (argc - optind < 2 || sonicver < 1 || sonicver > 4) {
        usage();
        return 1;
    }

    ifstream fin(argv[optind + 0], ios::in | ios::binary);
    if (!fin.good()) {
        cerr << "Input file '" << argv[optind + 0] << "' could not be opened."
             << endl
             << endl;
        return 2;
    }

    uint64_t numvoices = strtoul(argv[optind + 1], nullptr, 0);
    if (numvoices == 0UL || numvoices > 128) {
        cerr << "Invalid number of voices: '" << argv[optind + 1]
             << "'. Please supply a value between 1 and 128." << endl
             << endl;
        return 3;
    }
    fin.seekg(0, ios::end);
    uint64_t len = fin.tellg();
    fin.seekg(pointer);

    for (uint64_t i = 0UL; i < numvoices; i++) {
        uint64_t pos(fin.tellg());
        if (pos + 25UL > len) {
            // End of file reached in the middle of a voice.
            cerr << "Broken voice! The end-of-file was reached in the middle "
                    "of an FM voice."
                 << endl;
            cerr << "This voice, and all subsequent ones, were not dumped."
                 << endl;
            break;
        }

        // Print the voice.
        fm_voice voc{};
        voc.read(fin, sonicver);
        voc.print(cout, sonicver, i);
    }
}
