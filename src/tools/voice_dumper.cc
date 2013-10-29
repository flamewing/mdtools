/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * Copyright (C) Flamewing 2011-2013 <flamewing.sonic@gmail.com>
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
#include <cstdlib>

#include "getopt.h"
#include "bigendian_io.h"
#include "fmvoice.h"

static void usage() {
	std::cerr << "Usage: voice_dumper [-x|--extract [{pointer}]] {-v|--sonicver} {version} {input_filename} {num_voices}" << std::endl;
	std::cerr << std::endl;
	std::cerr << "\t-x,--extract \tExtract {num_voices} from {pointer} address in {input_filename}." << std::endl
	          << "\t             \tIf ommitted, {pointer} is assumed to be zero." << std::endl;
	std::cerr << "\t-v,--sonicver\tSets Sonic version to {version}. This also sets underlying" << std::endl
	          << "\t             \tSMPS type. {version} can be '1' Sonic 1, '2' for Sonic 2 or" << std::endl
	          << "\t             \t'3' for Sonic 3 or Sonic & Knuckles, or '4' for Sonic 3D Blast." << std::endl << std::endl;
}

int main(int argc, char *argv[]) {
	static struct option long_options[] = {
		{"extract", required_argument, 0, 'x'},
		{"sonicver", required_argument, 0, 'v'},
		{0, 0, 0, 0}
	};

	int pointer = 0, sonicver = -1, numvoices;

	while (true) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "x:v:",
		                    long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 'x':
				pointer = strtoul(optarg, 0, 0);
				break;

			case 'v':
				sonicver = strtoul(optarg, 0, 0);
				break;
		}
	}

	if (argc - optind < 2 || sonicver < 1 || sonicver > 4) {
		usage();
		return 1;
	}

	std::ifstream fin(argv[optind + 0], std::ios::in | std::ios::binary);
	if (!fin.good()) {
		std::cerr << "Input file '" << argv[optind + 0] << "' could not be opened." << std::endl << std::endl;
		return 2;
	}

	numvoices = strtoul(argv[optind + 1], 0, 0);
	fin.seekg(0, std::ios::end);
	int len = fin.tellg();
	fin.seekg(pointer);

	for (int i = 0; i < numvoices; i++) {
		if (fin.tellg() + std::streamoff(25) > len) {
			// End of file reached in the middle of a voice.
			std::cerr << "Broken voice! The end-of-file was reached in the middle of an FM voice." << std::endl;
			std::cerr << "This voice, and all subsequent ones, were not dumped." << std::endl;
			break;
		}

		// Print the voice.
		fm_voice voc;
		voc.read(fin, sonicver);
		voc.print(std::cout, sonicver, i);
	}
}
