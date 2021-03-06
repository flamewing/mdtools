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
#include <mdcomp/kosinski.hh>
#include <mdcomp/nemesis.hh>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

using std::cerr;
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
    cerr << "Usage: recolor-art [-o|--format {unc|nem|kos}] [-m|--moduled] "
            "{-clr1 clr2}+ {input_art} {output_art}"
         << endl;
    cerr << "\tRecolors the art file, changing palette index clr1 to clr2. "
            "Both are assumed to be an hex digit."
         << endl
         << "\tYou can specify as many colors to remap as you want, but each "
            "source color can appear only once."
         << endl
         << endl;
}

enum Formats { eInvalid = -1, eUncompressed = 0, eNemesis = 1, eKosinski = 2 };

using ColorMap = std::array<int, 16>;

struct Tile {
    std::array<uint8_t, 64> tiledata;

    bool read(istream& in) {
        for (size_t i = 0; i < sizeof(tiledata); i += 2) {
            size_t col = in.get();
            if (!in.good()) {
                return false;
            }
            tiledata[i + 0] = col & 0x0fU;
            tiledata[i + 1] = (col & 0xf0U) >> 4U;
        }
        return true;
    }

    bool blacklisted(uint8_t const bll) const {
        const auto* it
                = std::find(std::cbegin(tiledata), std::cend(tiledata), bll);
        return it != std::cend(tiledata);
    }

    void remap(ColorMap& colormap) {
        for (auto& elem : tiledata) {
            elem = colormap[elem];
        }
    }

    void write(ostream& out) {
        for (size_t i = 0; i < sizeof(tiledata); i += 2) {
            out.put(tiledata[i] | uint32_t(tiledata[i + 1] << 4U));
        }
    }
};

void recolor(istream& in, ostream& out, ColorMap& colormap) {
    Tile tile{};
    while (true) {
        if (!tile.read(in)) {
            break;
        }
        tile.remap(colormap);
        tile.write(out);
    }
}

int main(int argc, char* argv[]) {
    constexpr static const std::array<option, 3> long_options{
            option{"format", required_argument, nullptr, 'o'},
            option{"moduled", optional_argument, nullptr, 'm'},
            option{nullptr, 0, nullptr, 0}};

    bool       moduled    = false;
    streamsize modulesize = 0x1000;
    // Identity map.
    ColorMap colormap{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
                      0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};
    unsigned numcolors = 0;
    Formats  fmt       = eUncompressed;

    while (true) {
        int option_index = 0;
        int c            = getopt_long(
                argc, argv, "o:m::0:1:2:3:4:5:6:7:8:9:A:a:B:b:C:c:D:d:E:e:F:f:",
                long_options.data(), &option_index);
        if (c == -1) {
            break;
        }

        if (c >= 'A' && c <= 'F') {
            c += ('a' - 'A');
        }

        auto getFormat = [](auto optarg) {
            if (optarg == nullptr) {
                return eInvalid;
            }
            if (strcmp(optarg, "unc") == 0) {
                return eUncompressed;
            }
            if (strcmp(optarg, "nem") == 0) {
                return eNemesis;
            }
            if (strcmp(optarg, "kos") == 0) {
                return eKosinski;
            }
            return eInvalid;
        };

        switch (c) {
        case 'o':
            fmt = getFormat(optarg);
            if (fmt == eInvalid) {
                usage();
                return 1;
            }
            break;

        case 'm':
            moduled = true;
            if (optarg != nullptr) {
                modulesize = strtoul(optarg, nullptr, 0);
            }
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f': {
            if ((optarg == nullptr) || strlen(optarg) != 1) {
                usage();
                return 1;
            }
            int c1;
            if (c >= '0' && c <= '9') {
                c1 = c - '0';
            } else {
                c1 = c - 'a' + 10;
            }
            int d = static_cast<uint8_t>(*optarg);
            if (d >= '0' && d <= '9') {
                colormap[c1] = d - '0';
            } else if (d >= 'a' && d <= 'f') {
                colormap[c1] = d - 'a' + 10;
            } else if (d >= 'A' && d <= 'F') {
                colormap[c1] = d - 'A' + 10;
            } else {
                usage();
                return 1;
            }
            numcolors++;
            break;
        }
        default:
            usage();
            return 1;
        }
    }

    if (argc - optind < 2 || numcolors == 0) {
        usage();
        return 2;
    }

    ifstream fin(argv[optind], ios::in | ios::binary);
    if (!fin.good()) {
        cerr << "Input file '" << argv[optind] << "' could not be opened."
             << endl
             << endl;
        return 3;
    }

    stringstream sin(ios::in | ios::out | ios::binary);
    stringstream sout(ios::in | ios::out | ios::binary);

    fin.seekg(0);
    if (fmt == eUncompressed) {
        sin << fin.rdbuf();
    } else if (fmt == eNemesis) {
        nemesis::decode(fin, sin);
    } else {    // if (fmt == eKosinski)
        if (moduled) {
            kosinski::moduled_decode(fin, sin);
        } else {
            kosinski::decode(fin, sin);
        }
    }

    fin.close();
    sin.seekg(0);
    recolor(sin, sout, colormap);

    ofstream fout(argv[optind + 1], ios::out | ios::binary);
    if (!fout.good()) {
        cerr << "Output file '" << argv[optind + 1] << "' could not be opened."
             << endl
             << endl;
        return 4;
    }

    sout.seekg(0);

    if (fmt == eUncompressed) {
        fout << sout.rdbuf();
    } else if (fmt == eNemesis) {
        nemesis::encode(sout, fout);
    } else {    // if (fmt == eKosinski)
        if (moduled) {
            kosinski::moduled_encode(sout, fout, modulesize);
        } else {
            kosinski::encode(sout, fout);
        }
    }

    fout.close();
}
