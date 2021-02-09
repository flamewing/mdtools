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

#include <getopt.h>
#include <mdcomp/enigma.hh>
#include <mdcomp/kosinski.hh>
#include <mdtools/sstrack.hh>
#include <mdtools/ssvram.hh>

static void usage(char* prog) {
    cerr << "Usage: " << prog
         << " [-f|--flipped] {inpal} {inart} {intrack} {outartkos} "
            "{outplanekos} {outplaneeni}"
         << endl;
    cerr << endl;
    cerr << "\t-f,--flipped\tFlips the give frame horizontally." << endl;
}

int main(int argc, char* argv[]) {
    static option long_options[] = {
            {"flipped", no_argument, nullptr, 'f'}, {nullptr, 0, nullptr, 0}};

    bool flipped = false;

    while (true) {
        int option_index = 0;
        int c = getopt_long(argc, argv, "f", long_options, &option_index);
        if (c == -1) {
            break;
        }

        if (c == 'f') {
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

    ifstream inart(argv[optind + InArt], ios::in | ios::binary);
    if (!inart.good()) {
        cerr << "Could not read from art file '" << argv[optind + InArt] << "'."
             << endl;
        return InArt + 2;
    }

    ifstream intrack(argv[optind + InTrack], ios::in | ios::binary);
    if (!intrack.good()) {
        cerr << "Could not read from track file '" << argv[optind + InTrack]
             << "'." << endl;
        return InTrack + 2;
    }

    ofstream artout(
            argv[optind + OutArtKos], ios::out | ios::binary | ios::trunc);
    if (!artout.good()) {
        cerr << "Could not open output art file '" << argv[optind + OutArtKos]
             << "'." << endl;
        return OutArtKos + 2;
    }

    ofstream planekos(
            argv[optind + OutPlaneKos], ios::out | ios::binary | ios::trunc);
    if (!planekos.good()) {
        cerr << "Could not open output Kosinski-compressed plane map file '"
             << argv[optind + OutPlaneKos] << "'." << endl;
        return OutPlaneKos + 2;
    }

    ofstream planeeni(
            argv[optind + OutPlaneEni], ios::out | ios::binary | ios::trunc);
    if (!planeeni.good()) {
        cerr << "Could not open output Enigma-compressed pĺane map file '"
             << argv[optind + OutPlaneEni] << "'." << endl;
        return OutPlaneEni + 2;
    }

    SSVRAM       ssvram(inpal, inart);
    SSTrackFrame track(intrack, flipped);

    // Lets draw it!
    VRAM<Tile> vram;
    vram.copy_dist_table(ssvram);
    PlaneH32V28 plane;

    // First, lets create plane map and art for an equivalent scene.
    for (unsigned curline = 0; curline < SSTrackFrame::Height; curline++) {
        auto&       tline = track[curline];
        auto&       pline = plane[curline];
        const auto* tl0   = tline.cbegin();
        const auto* tl1   = tl0 + PlaneH32V28::Width;
        const auto* tl2   = tl1 + PlaneH32V28::Width;
        const auto* tl3   = tl2 + PlaneH32V28::Width;
        const auto* tlast = tline.cend();
        auto*       it    = pline.begin();
        while (tl3 != tlast) {
            Pattern_Name tp0    = *tl0++;
            Pattern_Name tp1    = *tl1++;
            Pattern_Name tp2    = *tl2++;
            Pattern_Name tp3    = *tl3++;
            ShortTile&   tile0  = ssvram[tp0];
            ShortTile&   tile1  = ssvram[tp1];
            ShortTile&   tile2  = ssvram[tp2];
            ShortTile&   tile3  = ssvram[tp3];
            Tile         merged = merge_tiles(
                    tile0, tp0.get_flip(), tile1, tp1.get_flip(), tile2,
                    tp2.get_flip(), tile3, tp3.get_flip());
            Pattern_Name bestpat;
            unsigned     dist = vram.find_closest(merged, bestpat);
            if (dist != 0) {
                bestpat = vram.push_back(merged);
            }
            *it++ = bestpat;
        }
    }

    // Now, lets save the plane map and art. First, the art.
    stringstream artbuffer(ios::in | ios::out | ios::binary);
    vram.write(artbuffer);
    artbuffer.seekg(0);
    kosinski::encode(artbuffer, artout);

    // Now plane map.
    stringstream planebuffer(ios::in | ios::out | ios::binary);
    plane.write(planebuffer);
    planebuffer.seekg(0);
    // First as Kosinski.
    kosinski::encode(planebuffer, planekos);
    planebuffer.seekg(0);
    // Now as Enigma.
    enigma::encode(planebuffer, planeeni);

    return 0;
}
