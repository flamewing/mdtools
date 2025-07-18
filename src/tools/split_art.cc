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
#include <mdcomp/comper.hh>
#include <mdcomp/kosinski.hh>
#include <mdtools/dplcfile.hh>

#include <array>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

using std::cerr;
using std::endl;
using std::hex;
using std::ifstream;
using std::ios;
using std::istream;
using std::ofstream;
using std::ostream;
using std::setfill;
using std::setw;
using std::stringstream;
using std::vector;

struct Tile {
    std::array<uint8_t, 32> tile_data;

    bool read(istream& input) {
        for (auto& elem : tile_data) {
            char value;
            input.get(value);
            elem = static_cast<uint8_t>(value);
        }
        return true;
    }

    void write(ostream& output) {
        for (auto& elem : tile_data) {
            output.put(elem);
        }
    }
};

static void usage(char* prog) {
    cerr << "Usage: " << prog
         << " --sonic=VER [-c|--comper|-m|--kosm] {input_art} {input_dplc} "
            "{output_prefix}"
         << endl;
    cerr << endl;
    cerr << "\t--sonic=VER  \tSpecifies the format of the input DPLC file."
         << endl;
    cerr << "\t             \tThe following values are accepted:" << endl;
    cerr << "\t             \tVER=1\tSonic 1 DPLC." << endl;
    cerr << "\t             \tVER=2\tSonic 2 DPLC." << endl;
    cerr << "\t             \tVER=3\tSonic 3 DPLC, as used by player objects."
         << endl;
    cerr << "\t             \tVER=4\tSonic 3 DPLC, as used by non-player "
            "objects."
         << endl;
    cerr << "\t-c,--comper  \tOutput files are Comper-compressed. Incompatible "
            "with --kosm."
         << endl;
    cerr << "\t-m,--kosm    \tOutput files are KosM-compressed. Incompatible "
            "with --comper."
         << endl
         << endl;
}

int main(int argc, char* argv[]) {
    constexpr static const std::array long_options{
            option{"kosm", no_argument, nullptr, 'm'},
            option{"comper", no_argument, nullptr, 'c'},
            option{"sonic", required_argument, nullptr, 'z'},
            option{nullptr, 0, nullptr, 0}};

    int64_t compress      = 0;
    int64_t sonic_version = 2;

    while (true) {
        int option_index = 0;
        int option_char  = getopt_long(
                 argc, argv, "cm", long_options.data(), &option_index);
        if (option_char == -1) {
            break;
        }

        switch (option_char) {
        case 'c':
            if (compress != 0) {
                cerr << "Can't use --comper an --kosm together." << endl
                     << endl;
                return 5;
            }
            compress = 1;
            break;
        case 'm':
            if (compress != 0) {
                cerr << "Can't use --comper an --kosm together." << endl
                     << endl;
                return 5;
            }
            compress = 2;
            break;
        case 'z':
            sonic_version = strtol(optarg, nullptr, 0);
            if (sonic_version < 1 || sonic_version > 4) {
                sonic_version = 2;
            }
            break;
        default:
            break;
        }
    }

    if (argc - optind < 3) {
        usage(argv[0]);
        return 1;
    }

    ifstream inart(argv[optind + 0], ios::in | ios::binary);
    ifstream indplc(argv[optind + 1], ios::in | ios::binary);

    if (!inart.good()) {
        cerr << "Input art file '" << argv[optind + 0]
             << "' could not be opened." << endl
             << endl;
        return 2;
    }

    if (!indplc.good()) {
        cerr << "Input DPLC file '" << argv[optind + 1]
             << "' could not be opened." << endl
             << endl;
        return 3;
    }

    inart.seekg(0, ios::end);
    size_t size = inart.tellg();
    inart.seekg(0);

    vector<Tile> tiles;
    tiles.resize(size / 32);

    for (auto& tile : tiles) {
        tile.read(inart);
    }
    inart.close();

    dplc_file const srcdplc(indplc, sonic_version);
    indplc.close();

    for (size_t ii = 0; ii < srcdplc.frames.size(); ii++) {
        stringstream buffer(ios::in | ios::out | ios::binary);
        auto const&  frame = srcdplc.frames[ii];
        for (auto const& dplc : frame.dplc) {
            for (size_t kk = 0; kk < dplc.count; kk++) {
                tiles[dplc.tile + kk].write(buffer);
            }
        }
        stringstream fname(ios::in | ios::out);
        fname << argv[optind + 2] << hex << setw(2) << setfill('0') << ii
              << ".bin";
        ofstream output(
                fname.str().c_str(),
                ios::in | ios::out | ios::binary | ios::trunc);
        if (!output.good()) {
            cerr << "Output file '" << fname.str() << "' could not be opened."
                 << endl
                 << endl;
            return 4;
        }
        if (compress == 1) {
            comper::encode(buffer, output);
        } else if (compress == 2) {
            kosinski::moduled_encode(buffer, output);
        } else {
            output << buffer.rdbuf();
        }
    }

    return 0;
}
