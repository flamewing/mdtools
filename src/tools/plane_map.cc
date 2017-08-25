/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#include <getopt.h>

#include <mdcomp/enigma.h>

#include "bigendian_io.h"
#include "ignore_unused_variable_warning.h"

using namespace std;

static void usage() {
	cerr << "Usage: plane_map [-x|--extract [{pointer}]] [--sonic2] {input_filename} {output_filename} {width} {height}" << endl;
	cerr << "\tPerforms a plane map operation on {input_filename}. {input_filename} is assumed to be a linear array which must be mapped into an area of {width} x {height} cells of 8x8 pixels." << endl;
	cerr << "\t{output_filename} is a mappings file." << endl;
	cerr << endl;
	cerr << "\t-x,--extract\tAssume input file is Enigma-compressed and decode it before doing the plane map. File is read starting from {pointer}." << endl;
	cerr << "\t--sonic2\t{output_filename} is in Sonic 2 mappings format. Default to non-Sonic 2 format." << endl << endl;
	cerr << "Usage: plane_map -u [-c|--compress] [--sonic2] {input_filename} {output_filename}" << endl;
	cerr << "\tDoes the reverse operation of the above usage (without -u). {input_filename} must contain only single-tile pieces." << endl;
	cerr << endl;
	cerr << "\t-c,--compress\t{output_filename} is Enigma compressed." << endl;
	cerr << "\t--sonic2\t{input_filename} is in Sonic 2 mappings format. Default to non-Sonic 2 format." << endl << endl;
}

static void plane_map(istream &src, ostream &dst, size_t w, size_t h,
                      streamsize pointer, bool sonic2) {
	src.seekg(0, ios::end);
	streamsize sz = streamsize(src.tellg()) - pointer;
	src.seekg(pointer);

	size_t nframes = sz / (2 * w * h);
	size_t off = 2 * nframes;

	for (size_t n = 0; n < nframes; n++, off += 2 + 8 * w * h) {
		BigEndian::Write2(dst, static_cast<uint16_t>(off));
	}

	for (size_t n = 0; n < nframes; n++, off += 2) {
		BigEndian::Write2(dst, w * h);
		for (size_t j = 0; j < h; j++) {
			signed char y_pos = (j - h / 2) << 3;
			for (size_t i = 0; i < w; i++) {
				dst.put(static_cast<char>(y_pos));
				dst.put(static_cast<char>(0x00));
				uint16_t v = BigEndian::Read2(src);
				BigEndian::Write2(dst, v);
				if (sonic2) {
					BigEndian::Write2(dst, (v & 0xf800) | ((v & 0x07ff) >> 1));
				}
				BigEndian::Write2(dst, static_cast<uint16_t>((i - w / 2) << 3));
			}
		}
	}
}

struct Position {
	int16_t x;
	int8_t y;
	bool operator<(Position const &other) const {
		return (y < other.y) || (y == other.y && x < other.x);
	}
};

using Enigma_map = map<Position, uint16_t>;

static void plane_unmap(istream &src, ostream &dst,
                        streamsize pointer, bool sonic2) {
	ignore_unused_variable_warning(pointer);
	streamsize next_loc = src.tellg();
	streamsize last_loc = BigEndian::Read2(src);
	src.seekg(0, ios::end);
	src.seekg(next_loc);

	while (next_loc < last_loc) {
		src.seekg(next_loc);

		size_t offset = BigEndian::Read2(src);
		next_loc = src.tellg();
		if (next_loc != last_loc) {
			src.ignore(2);
		}

		src.seekg(offset);

		size_t count = BigEndian::Read2(src);
		Enigma_map engfile;
		for (size_t i = 0; i < count; i++) {
			Position pos{};
			pos.y = static_cast<signed char>(src.get() & 0xff);
			src.ignore(1);
			uint16_t v = BigEndian::Read2(src);
			if (sonic2) {
				src.ignore(2);
			}
			pos.x = static_cast<int16_t>(BigEndian::Read2(src));
			engfile.emplace(pos, v);
		}

		for (auto & elem : engfile) {
			uint16_t v = elem.second;
			BigEndian::Write2(dst, v);
		}
	}
}

int main(int argc, char *argv[]) {
	int sonic2 = 0;
	static option long_options[] = {
		{"extract"  , optional_argument, nullptr, 'x'},
		{"sonic2"   , no_argument      , &sonic2, 1},
		{"compress" , no_argument      , nullptr, 'c'},
		{nullptr, 0, nullptr, 0}
	};

	bool extract = false, compress = false, unmap = false;
	streamsize pointer = 0;

	while (true) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "ux::c",
		                    long_options, &option_index);
		if (c == -1) {
			break;
		}

		switch (c) {
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
		}
	}

	if (argc - optind < 2 || (!unmap && argc - optind < 4)) {
		usage();
		return 1;
	}

	ifstream fin(argv[optind], ios::in | ios::binary);
	if (!fin.good()) {
		cerr << "Input file '" << argv[optind] << "' could not be opened." << endl << endl;
		return 2;
	}

	ofstream fout(argv[optind + 1], ios::out | ios::binary);
	if (!fout.good()) {
		cerr << "Output file '" << argv[optind + 1] << "' could not be opened." << endl << endl;
		return 3;
	}

	if (unmap) {
		stringstream fbuf(ios::in | ios::out | ios::binary | ios::trunc);
		if (compress) {
			plane_unmap(fin, fbuf, 0      , sonic2 != 0);
			enigma::encode(fbuf, fout);
		} else {
			plane_unmap(fin, fout, pointer, sonic2 != 0);
		}
	} else {
		stringstream fbuf(ios::in | ios::out | ios::binary | ios::trunc);
		size_t w = strtoul(argv[optind + 2], nullptr, 0), h = strtoul(argv[optind + 3], nullptr, 0);
		if ((w == 0u) || (h == 0u)) {
			cerr << "Invalid height or width for plane mapping." << endl << endl;
			return 4;
		}
		if (extract) {
			fin.seekg(pointer);
			enigma::decode(fin, fbuf);
			plane_map(fbuf, fout, w, h, 0      , sonic2 != 0);
		} else {
			plane_map(fin , fout, w, h, pointer, sonic2 != 0);
		}
	}
	return 0;
}

