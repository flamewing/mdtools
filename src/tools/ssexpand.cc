/*
 * Copyright (C) Flamewing 2014 <flamewing.sonic@gmail.com>
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
#include <mdcomp/enigma.hh>
#include <mdcomp/kosinski.hh>
#include <mdtools/sstrack.hh>
#include <mdtools/ssvram.hh>

#include <array>
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
using std::stringstream;

static void usage(char* prog) {
    cerr << "Usage: " << prog
         << " [-f|--flipped] {inpal} {inart} {input_track} {output_art_kos} "
            "{output_plane_kos} {output_plane_eni}"
         << endl;
    cerr << endl;
    cerr << "\t-f,--flipped\tFlips the give frame horizontally." << endl;
}

int main(int argc, char* argv[]) {
    constexpr static const std::array long_options{
            option{"flipped", no_argument, nullptr, 'f'},
            option{nullptr, 0, nullptr, 0}};

    bool flipped = false;

    while (true) {
        int option_index = 0;
        int option_char  = getopt_long(
                 argc, argv, "f", long_options.data(), &option_index);
        if (option_char == -1) {
            break;
        }

        if (option_char == 'f') {
            flipped = true;
        }
    }

    enum ArgumentIDs {
        InPal = 0,
        InArt,
        InTrack,
        OutArtKos,
        OutPlaneKos,
        OutPlaneEni,
        TotalArgCount
    };
    if (argc - optind < TotalArgCount) {
        usage(argv[0]);
        return 1;
    }

    ifstream inpal(argv[optind + InPal], ios::in | ios::binary);
    if (!inpal.good()) {
        cerr << "Could not read from palette file '" << argv[optind + InPal]
             << "'." << endl;
        return InPal + 2;
    }

    ifstream input_art(argv[optind + InArt], ios::in | ios::binary);
    if (!input_art.good()) {
        cerr << "Could not read from art file '" << argv[optind + InArt] << "'."
             << endl;
        return InArt + 2;
    }

    ifstream input_track(argv[optind + InTrack], ios::in | ios::binary);
    if (!input_track.good()) {
        cerr << "Could not read from track file '" << argv[optind + InTrack]
             << "'." << endl;
        return InTrack + 2;
    }

    ofstream art_output(
            argv[optind + OutArtKos], ios::out | ios::binary | ios::trunc);
    if (!art_output.good()) {
        cerr << "Could not open output art file '" << argv[optind + OutArtKos]
             << "'." << endl;
        return OutArtKos + 2;
    }

    ofstream plane_kos(
            argv[optind + OutPlaneKos], ios::out | ios::binary | ios::trunc);
    if (!plane_kos.good()) {
        cerr << "Could not open output Kosinski-compressed plane map file '"
             << argv[optind + OutPlaneKos] << "'." << endl;
        return OutPlaneKos + 2;
    }

    ofstream plane_eni(
            argv[optind + OutPlaneEni], ios::out | ios::binary | ios::trunc);
    if (!plane_eni.good()) {
        cerr << "Could not open output Enigma-compressed plane map file '"
             << argv[optind + OutPlaneEni] << "'." << endl;
        return OutPlaneEni + 2;
    }

    SSVRAM       ssvram(inpal, input_art);
    SSTrackFrame track(input_track, flipped);

    // Lets draw it!
    VRAM<Tile> vram;
    vram.copy_dist_table(ssvram);
    PlaneH32V28 plane;

    // First, lets create plane map and art for an equivalent scene.
    for (unsigned cur_line = 0; cur_line < SSTrackFrame::Height; cur_line++) {
        auto&       tline = track[cur_line];
        auto&       pline = plane[cur_line];
        const auto* line0 = tline.cbegin();
        const auto* line1 = line0 + PlaneH32V28::Width;
        const auto* line2 = line1 + PlaneH32V28::Width;
        const auto* line3 = line2 + PlaneH32V28::Width;
        const auto* tlast = tline.cend();
        auto*       dest  = pline.begin();
        while (line3 != tlast) {
            Pattern_Name pat0   = *line0++;
            Pattern_Name pat1   = *line1++;
            Pattern_Name pat2   = *line2++;
            Pattern_Name pat3   = *line3++;
            ShortTile&   tile0  = ssvram[pat0];
            ShortTile&   tile1  = ssvram[pat1];
            ShortTile&   tile2  = ssvram[pat2];
            ShortTile&   tile3  = ssvram[pat3];
            Tile         merged = merge_tiles(
                            tile0, pat0.get_flip(), tile1, pat1.get_flip(), tile2,
                            pat2.get_flip(), tile3, pat3.get_flip());
            Pattern_Name best_pattern;
            unsigned     dist = vram.find_closest(merged, best_pattern);
            if (dist != 0) {
                best_pattern = vram.push_back(merged);
            }
            *dest++ = best_pattern;
        }
    }

    // Now, lets save the plane map and art. First, the art.
    stringstream art_buffer(ios::in | ios::out | ios::binary);
    vram.write(art_buffer);
    art_buffer.seekg(0);
    kosinski::encode(art_buffer, art_output);

    // Now plane map.
    stringstream plane_buffer(ios::in | ios::out | ios::binary);
    plane.write(plane_buffer);
    plane_buffer.seekg(0);
    // First as Kosinski.
    kosinski::encode(plane_buffer, plane_kos);
    plane_buffer.seekg(0);
    // Now as Enigma.
    enigma::encode(plane_buffer, plane_eni);

    return 0;
}
