/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

#include <getopt.h>

#include "ssvram.h"
#include "sstrack.h"
#include "mdcomp/enigma.h"
#include "mdcomp/kosinski.h"

static void usage(char *prog) {
	cerr << "Usage: " << prog << " [-f|--flipped] {inart} {intrack} {outartkos} {outplanekos} {outplaneeni}" << endl;
	cerr << endl;
	cerr << "\t-f,--flipped\tFlips the give frame horizontally." << endl;
}

int main(int argc, char *argv[]) {
	static option long_options[] = {
		{"flipped", no_argument      , nullptr, 'f'},
		{nullptr, 0, nullptr, 0}
	};

	bool flipped = false;

	while (true) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "f",
		                    long_options, &option_index);
		if (c == -1) {
			break;
		}

		switch (c) {
			case 'f':
				flipped = true;
				break;
		}
	}

	if (argc - optind < 5) {
		usage(argv[0]);
		return 1;
	}

	ifstream inart(argv[optind+0], ios::in|ios::binary);
	if (!inart.good()) {
		cerr << "Could not read from art file '" << argv[optind+0] << "'." << endl;
		return 2;
	}

	ifstream intrack(argv[optind+1], ios::in|ios::binary);
	if (!intrack.good()) {
		cerr << "Could not read from track file '" << argv[optind+1] << "'." << endl;
		return 3;
	}

	ofstream artout(argv[optind+2], ios::out|ios::binary|ios::trunc);
	if (!artout.good()) {
		cerr << "Could not open output art file '" << argv[optind+2] << "'." << endl;
		return 4;
	}

	ofstream planekos(argv[optind+3], ios::out|ios::binary|ios::trunc);
	if (!planekos.good()) {
		cerr << "Could not open output Kosinski-compressed plane map file '" << argv[optind+3] << "'." << endl;
		return 5;
	}

	ofstream planeeni(argv[optind+4], ios::out|ios::binary|ios::trunc);
	if (!planeeni.good()) {
		cerr << "Could not open output Enigma-compressed pĺane map file '" << argv[optind+4] << "'." << endl;
		return 6;
	}

	SSVRAM ssvram(inart);
	SSTrackFrame track(intrack, flipped);

	// Lets draw it!
	VRAM<Tile>  vram;
	PlaneH32V28 plane;

	// First, lets create plane map and art for an equivalent scene.
	for (unsigned curline = 0; curline < SSTrackFrame::Height; curline++) {
		SSTrackFrame::Line &tline = track[curline];
		PlaneH32V28::Line &pline = plane[curline];
		auto tl0 = tline.cbegin(),
		     tl1 = tl0 + PlaneH32V28::Width,
		     tl2 = tl1 + PlaneH32V28::Width,
		     tl3 = tl2 + PlaneH32V28::Width,
		     tlast = tline.cend();
		auto it = pline.begin();
		while (tl3 != tlast) {
			Pattern_Name tp0 = *tl0++, tp1 = *tl1++, tp2 = *tl2++, tp3 = *tl3++;
			ShortTile &tile0 = ssvram[tp0];
			ShortTile &tile1 = ssvram[tp1];
			ShortTile &tile2 = ssvram[tp2];
			ShortTile &tile3 = ssvram[tp3];
			Tile merged = merge_tiles(tile0, tp0.get_flip(),
			                          tile1, tp1.get_flip(),
			                          tile2, tp2.get_flip(),
			                          tile3, tp3.get_flip());
			Pattern_Name bestpat;
			unsigned dist = vram.find_closest(merged, bestpat);
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
