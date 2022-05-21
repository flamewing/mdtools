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

#include "mdtools/ignore_unused_variable_warning.hh"

#include <getopt.h>
#include <mdcomp/comper.hh>
#include <mdcomp/comperx.hh>
#include <mdcomp/kosinski.hh>
#include <mdcomp/kosplus.hh>
#include <mdcomp/lzkn1.hh>
#include <mdcomp/nemesis.hh>
#include <mdcomp/rocket.hh>
#include <mdcomp/saxman.hh>
#include <mdcomp/snkrle.hh>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

using std::cerr;
using std::endl;
using std::ifstream;
using std::ios;
using std::istream;
using std::ofstream;
using std::ostream;
using std::streamsize;
using std::string;
using std::string_view;
using std::stringstream;

using namespace std::literals::string_view_literals;

static void usage() {
    cerr << "Usage: recolor-art [-o|--format "
            "{unc|comp|compx|kos|kos+|lzkn1|nem|rocket|snk}] "
            "[-m|--moduled] "
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

using ColorMap = std::array<int, 16>;

struct Tile {
    std::array<uint8_t, 64> tile_data;

    bool read(istream& input) {
        for (size_t i = 0; i < sizeof(tile_data); i += 2) {
            size_t value = input.get();
            if (!input.good()) {
                return false;
            }
            tile_data[i + 0] = value & 0x0fU;
            tile_data[i + 1] = (value & 0xf0U) >> 4U;
        }
        return true;
    }

    [[nodiscard]] bool blacklisted(uint8_t const value) const {
        const auto* found = std::find(
                std::cbegin(tile_data), std::cend(tile_data), value);
        return found != std::cend(tile_data);
    }

    void remap(ColorMap& color_map) {
        for (auto& elem : tile_data) {
            elem = color_map[elem];
        }
    }

    void write(ostream& output) {
        for (size_t i = 0; i < sizeof(tile_data); i += 2) {
            output.put(tile_data[i] | uint32_t(tile_data[i + 1] << 4U));
        }
    }
};

class uncompressed;
using basic_uncompressed   = BasicDecoder<uncompressed, PadMode::PadEven>;
using moduled_uncompressed = ModuledAdaptor<uncompressed, 4096U, 1U>;

class uncompressed : public basic_uncompressed, public moduled_uncompressed {
    friend basic_uncompressed;
    friend moduled_uncompressed;
    static bool encode(std::ostream& Dest, uint8_t const* data, size_t Size);

public:
    using basic_uncompressed::encode;
    static bool decode(std::istream& Source, std::iostream& Dest);
};

bool uncompressed::encode(
        std::ostream& Dest, uint8_t const* data, size_t Size) {
    Dest.write(reinterpret_cast<const char*>(data), Size);
    return true;
}

bool uncompressed::decode(std::istream& Source, std::iostream& Dest) {
    Dest << Source.rdbuf();
    return true;
}

void recolor(istream& input, ostream& output, ColorMap& color_map) {
    Tile tile{};
    while (true) {
        if (!tile.read(input)) {
            break;
        }
        tile.remap(color_map);
        tile.write(output);
    }
}

template <>
size_t moduled_uncompressed::PadMaskBits = 1U;

using encoder         = decltype(&basic_uncompressed::encode);
using decoder         = decltype(&uncompressed::decode);
using moduled_encoder = decltype(&uncompressed::moduled_encode);
using moduled_decoder = decltype(&uncompressed::moduled_decode);

struct art_format {
    encoder         encode;
    decoder         decode;
    moduled_encoder moduled_encode;
    moduled_decoder moduled_decode;
};

int main(int argc, char* argv[]) {
    constexpr static const std::array long_options{
            option{"format", required_argument, nullptr, 'o'},
            option{"moduled", optional_argument, nullptr, 'm'},
            option{nullptr, 0, nullptr, 0}};

    bool       moduled     = false;
    streamsize module_size = 0x1000;
    // Identity map.
    ColorMap color_map{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
                       0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF};
    unsigned num_colors = 0;

    static const std::unordered_map<string_view, art_format> format_lut{
            {{"unc"sv,
              {uncompressed::encode, uncompressed::decode,
               uncompressed::moduled_encode, uncompressed::moduled_decode}},
             {"comp"sv,
              {comper::encode, comper::decode, comper::moduled_encode,
               comper::moduled_decode}},
             {"compx"sv,
              {comperx::encode, comperx::decode, comperx::moduled_encode,
               comperx::moduled_decode}},
             {"kos"sv,
              {kosinski::encode, kosinski::decode, kosinski::moduled_encode,
               kosinski::moduled_decode}},
             {"kos+"sv,
              {kosplus::encode, kosplus::decode, kosplus::moduled_encode,
               kosplus::moduled_decode}},
             {"lzkn1"sv,
              {lzkn1::encode, lzkn1::decode, lzkn1::moduled_encode,
               lzkn1::moduled_decode}},
             {"nem"sv,
              {nemesis::encode,
               +[](std::istream& Source, std::iostream& Dest) {
                   return nemesis::decode(Source, Dest);
               },
               nemesis::moduled_encode, nemesis::moduled_decode}},
             {"rocket"sv,
              {rocket::encode, rocket::decode, rocket::moduled_encode,
               rocket::moduled_decode}},
             {"snk"sv,
              {snkrle::encode,
               +[](std::istream& Source, std::iostream& Dest) {
                   return snkrle::decode(Source, Dest);
               },
               snkrle::moduled_encode, snkrle::moduled_decode}}}};
    auto format = format_lut.cend();

    while (true) {
        int option_index = 0;
        int option_char  = getopt_long(
                 argc, argv, "o:m::0:1:2:3:4:5:6:7:8:9:A:a:B:b:C:c:D:d:E:e:F:f:",
                 long_options.data(), &option_index);
        if (option_char == -1) {
            break;
        }

        if (option_char >= 'A' && option_char <= 'F') {
            option_char += ('a' - 'A');
        }

        switch (option_char) {
        case 'o':
            if (optarg == nullptr) {
                usage();
                return 1;
            }
            format = format_lut.find(optarg);
            if (format == format_lut.cend()) {
                usage();
                return 1;
            }
            break;

        case 'm':
            moduled = true;
            if (optarg != nullptr) {
                module_size = strtoul(optarg, nullptr, 0);
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
            int source_color;
            if (option_char >= '0' && option_char <= '9') {
                source_color = option_char - '0';
            } else {
                source_color = option_char - 'a' + 10;
            }
            int dest_color = static_cast<uint8_t>(*optarg);
            if (dest_color >= '0' && dest_color <= '9') {
                color_map[source_color] = dest_color - '0';
            } else if (dest_color >= 'a' && dest_color <= 'f') {
                color_map[source_color] = dest_color - 'a' + 10;
            } else if (dest_color >= 'A' && dest_color <= 'F') {
                color_map[source_color] = dest_color - 'A' + 10;
            } else {
                usage();
                return 1;
            }
            num_colors++;
            break;
        }
        default:
            usage();
            return 1;
        }
    }

    if (argc - optind < 2 || num_colors == 0) {
        usage();
        return 2;
    }

    ifstream input(argv[optind], ios::in | ios::binary);
    if (!input.good()) {
        cerr << "Input file '" << argv[optind] << "' could not be opened."
             << endl
             << endl;
        return 3;
    }

    stringstream input_buffer(ios::in | ios::out | ios::binary);
    stringstream output_buffer(ios::in | ios::out | ios::binary);

    auto const& [key, fmt_handler] = *format;

    input.seekg(0);
    if (moduled) {
        fmt_handler.moduled_decode(input, input_buffer, module_size);
    } else {
        fmt_handler.decode(input, input_buffer);
    }

    input.close();
    input_buffer.seekg(0);
    recolor(input_buffer, output_buffer, color_map);

    ofstream output(argv[optind + 1], ios::out | ios::binary);
    if (!output.good()) {
        cerr << "Output file '" << argv[optind + 1] << "' could not be opened."
             << endl
             << endl;
        return 4;
    }

    output_buffer.seekg(0);
    if (moduled) {
        fmt_handler.moduled_encode(output_buffer, output, module_size);
    } else {
        fmt_handler.encode(output_buffer, output);
    }

    output.close();
}
