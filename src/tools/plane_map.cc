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
#include <mdcomp/enigma.hh>
#include <mdtools/ignore_unused_variable_warning.hh>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using std::cerr;
using std::endl;
using std::ifstream;
using std::ios;
using std::istream;
using std::map;
using std::ofstream;
using std::ostream;
using std::streamsize;
using std::string;
using std::stringstream;

static void usage() {
    cerr << "Usage: plane_map [-x|--extract [{pointer}]] [--sonic2] "
            "{input_filename} {output_filename} {width} {height}"
         << endl;
    cerr << "\tPerforms a plane map operation on {input_filename}. "
            "{input_filename} is assumed to be a linear array which must be "
            "mapped into an area of {width} x {height} cells of 8x8 pixels."
         << endl;
    cerr << "\t{output_filename} is a mappings file." << endl;
    cerr << endl;
    cerr << "\t-x,--extract\tAssume input file is Enigma-compressed and decode "
            "it before doing the plane map. File is read starting from "
            "{pointer}."
         << endl;
    cerr << "\t--sonic2\t{output_filename} is in Sonic 2 mappings format. "
            "Default to non-Sonic 2 format."
         << endl
         << endl;
    cerr << "Usage: plane_map -u [-c|--compress] [--sonic2] {input_filename} "
            "{output_filename}"
         << endl;
    cerr << "\tDoes the reverse operation of the above usage (without -u). "
            "{input_filename} must contain only single-tile pieces."
         << endl;
    cerr << endl;
    cerr << "\t-c,--compress\t{output_filename} is Enigma compressed." << endl;
    cerr << "\t--sonic2\t{input_filename} is in Sonic 2 mappings format. "
            "Default to non-Sonic 2 format."
         << endl
         << endl;
}

static void plane_map(
        istream& source, ostream& dest, size_t width, size_t height,
        streamsize pointer, bool sonic2) {
    source.seekg(0, ios::end);
    streamsize size = streamsize(source.tellg()) - pointer;
    source.seekg(pointer);

    size_t nframes = size / (2 * width * height);
    size_t offset  = 2 * nframes;

    for (size_t frame = 0; frame < nframes;
         frame++, offset += 2 + 8 * width * height) {
        BigEndian::Write2(dest, static_cast<uint16_t>(offset));
    }

    for (size_t frame = 0; frame < nframes; frame++, offset += 2) {
        BigEndian::Write2(dest, width * height);
        for (size_t line = 0; line < height; line++) {
            auto y_pos = static_cast<int8_t>((line - height / 2) << 3U);
            for (size_t column = 0; column < width; column++) {
                dest.put(static_cast<char>(y_pos));
                dest.put(static_cast<char>(0x00));
                uint16_t value = BigEndian::Read2(source);
                BigEndian::Write2(dest, value);
                if (sonic2) {
                    BigEndian::Write2(
                            dest,
                            (value & 0xf800U) | ((value & 0x07ffU) >> 1U));
                }
                BigEndian::Write2(
                        dest,
                        static_cast<uint16_t>((column - width / 2) << 3U));
            }
        }
    }
}

struct Position {
    int16_t x;
    int8_t  y;
    bool    operator<(Position const& other) const {
           return (y < other.y) || (y == other.y && x < other.x);
    }
};

using Enigma_map = map<Position, uint16_t>;

static void plane_unmap(
        istream& source, ostream& dest, streamsize pointer, bool sonic2) {
    ignore_unused_variable_warning(pointer);
    streamsize next_loc = source.tellg();
    streamsize last_loc = BigEndian::Read2(source);
    source.seekg(0, ios::end);
    source.seekg(next_loc);

    while (next_loc < last_loc) {
        source.seekg(next_loc);

        size_t offset = BigEndian::Read2(source);
        next_loc      = source.tellg();
        if (next_loc != last_loc) {
            source.ignore(2);
        }

        source.seekg(offset);

        size_t     count = BigEndian::Read2(source);
        Enigma_map enigma_file;
        for (size_t i = 0; i < count; i++) {
            Position position{};
            position.y = static_cast<int8_t>(source.get());
            source.ignore(1);
            uint16_t value = BigEndian::Read2(source);
            if (sonic2) {
                source.ignore(2);
            }
            position.x = static_cast<int16_t>(BigEndian::Read2(source));
            enigma_file.emplace(position, value);
        }

        for (auto& elem : enigma_file) {
            uint16_t value = elem.second;
            BigEndian::Write2(dest, value);
        }
    }
}

int main(int argc, char* argv[]) {
    int sonic2 = 0;

    static const std::array long_options{
            option{"extract", optional_argument, nullptr, 'x'},
            option{"sonic2", no_argument, &sonic2, 1},
            option{"compress", no_argument, nullptr, 'c'},
            option{nullptr, 0, nullptr, 0}};

    bool extract  = false;
    bool compress = false;
    bool unmap    = false;

    streamsize pointer = 0;

    while (true) {
        int option_index = 0;
        int option_char  = getopt_long(
                 argc, argv, "ux::c", long_options.data(), &option_index);
        if (option_char == -1) {
            break;
        }

        switch (option_char) {
        case 'x':
            extract = true;
            if (optarg != nullptr) {
                pointer = strtoul(optarg, nullptr, 0);
            }
            break;

        case 'c':
            compress = true;
            break;

        case 'u':
            unmap = true;
            break;
        default:
            break;
        }
    }

    if (argc - optind < 2 || (!unmap && argc - optind < 4)) {
        usage();
        return 1;
    }

    ifstream input(argv[optind], ios::in | ios::binary);
    if (!input.good()) {
        cerr << "Input file '" << argv[optind] << "' could not be opened."
             << endl
             << endl;
        return 2;
    }

    ofstream output(argv[optind + 1], ios::out | ios::binary);
    if (!output.good()) {
        cerr << "Output file '" << argv[optind + 1] << "' could not be opened."
             << endl
             << endl;
        return 3;
    }

    if (unmap) {
        stringstream fbuf(ios::in | ios::out | ios::binary | ios::trunc);
        if (compress) {
            plane_unmap(input, fbuf, 0, sonic2 != 0);
            enigma::encode(fbuf, output);
        } else {
            plane_unmap(input, output, pointer, sonic2 != 0);
        }
    } else {
        stringstream fbuf(ios::in | ios::out | ios::binary | ios::trunc);

        size_t width  = strtoul(argv[optind + 2], nullptr, 0);
        size_t height = strtoul(argv[optind + 3], nullptr, 0);
        if ((width == 0U || width > 128U) || (height == 0U || height > 128U)) {
            cerr << "Invalid height or width for plane mapping." << endl
                 << endl;
            return 4;
        }
        if (extract) {
            input.seekg(pointer);
            enigma::decode(input, fbuf);
            plane_map(fbuf, output, width, height, 0, sonic2 != 0);
        } else {
            plane_map(input, output, width, height, pointer, sonic2 != 0);
        }
    }
    return 0;
}
