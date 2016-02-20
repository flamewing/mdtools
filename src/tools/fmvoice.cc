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

#include "fmvoice.h"

#include <istream>
#include <iomanip>
#include <ostream>

#include "bigendian_io.h"

using namespace std;

void PrintMacro(ostream &out, char const *macro) {
	out << "\t" << setw(19) << setfill(' ') << left << macro << right << " ";
}

void PrintHex2(ostream &out, unsigned char c, bool last) {
	out << "$" << hex << setw(2) << setfill('0') << uppercase << static_cast<unsigned int>(c) << nouppercase;
	if (!last) {
		out << ", ";
	}
}

void PrintHex2Pre(ostream &out, unsigned char c, bool first) {
	if (!first) {
		out << ", ";
	}
	out << "$" << hex << setw(2) << setfill('0') << uppercase << static_cast<unsigned int>(c) << nouppercase;
}

void PrintName(ostream &out, string s, bool first) {
	if (!first) {
		out << ", ";
	}
	out << s;
}

void PrintHex4(ostream &out, unsigned short c, bool last) {
	out << "$" << hex << setw(4) << setfill('0') << uppercase << static_cast<unsigned int>(c) << nouppercase;
	if (!last) {
		out << ", ";
	}
}

void fm_voice::read(istream &in, int sonicver) {
	unsigned char c = Read1(in);
	vcUnusedBits = (c >> 6) & 3;
	vcFeedback   = (c >> 3) & 7;
	vcAlgorithm  = c & 7;
	static int s2_indices[4] = {3, 1, 2, 0}, normal_indices[4] = {3, 2, 1, 0};
	int *indices = sonicver == 2 ? s2_indices : normal_indices;
	for (int i = 0; i < 4; i++) {
		c = Read1(in);
		vcDT[indices[i]] = (c >> 4) & 0xf;
		vcCF[indices[i]] = c & 0xf;
	}
	for (int i = 0; i < 4; i++) {
		c = Read1(in);
		vcRS[indices[i]] = (c >> 6) & 0x3;
		vcAR[indices[i]] = c & 0x3f;
	}
	for (int i = 0; i < 4; i++) {
		c = Read1(in);
		vcAM [indices[i]] = (c >> 5) & 0x3;
		vcD1R[indices[i]] = c & 0x1f;
	}
	for (int i = 0; i < 4; i++) {
		vcD2R[indices[i]] = Read1(in);
	}
	for (int i = 0; i < 4; i++) {
		c = Read1(in);
		vcDL[indices[i]] = (c >> 4) & 0xf;
		vcRR[indices[i]] = c & 0xf;
	}
	for (int i = 0; i < 4; i++) {
		vcTL[indices[i]] = Read1(in);
	}
}

void fm_voice::write(ostream &out, int sonicver) const {
	Write1(out, (vcUnusedBits << 6) | (vcFeedback << 3) | vcAlgorithm);
	static int s2_indices[4] = {3, 1, 2, 0}, normal_indices[4] = {3, 2, 1, 0};
	int *indices = sonicver == 2 ? s2_indices : normal_indices;
	for (int i = 0; i < 4; i++) {
		Write1(out, (vcDT[indices[i]] << 4) | vcCF[indices[i]]);
	}
	for (int i = 0; i < 4; i++) {
		Write1(out, (vcRS[indices[i]] << 6) | vcAR[indices[i]]);
	}
	for (int i = 0; i < 4; i++) {
		Write1(out, (vcAM[indices[i]] << 5) | vcD1R[indices[i]]);
	}
	for (int i = 0; i < 4; i++) {
		Write1(out, vcD2R[indices[i]]);
	}
	for (int i = 0; i < 4; i++) {
		Write1(out, (vcDL[indices[i]] << 4) | vcRR[indices[i]]);
	}
	for (int i = 0; i < 4; i++) {
		Write1(out, vcTL[indices[i]]);
	}
}

void fm_voice::print(ostream &out, int sonicver, int id) const {
	out << ";\tVoice ";
	PrintHex2(out, id, true);
	out << endl << ";\t";
	PrintHex2(out, (vcUnusedBits << 6) | (vcFeedback << 3) | vcAlgorithm, true);
	static int s2_indices[4] = {3, 2, 1, 0}, normal_indices[4] = {3, 2, 1, 0};
	int *indices = sonicver == 2 ? s2_indices : normal_indices;
	out << endl << ";\t";
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, (vcDT[indices[i]] << 4) | vcCF[indices[i]], false);
	}
	out << "\t";
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, (vcRS[indices[i]] << 6) | vcAR[indices[i]], false);
	}
	out << "\t";
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, (vcAM[indices[i]] << 5) | vcD1R[indices[i]], i == 3);
	}
	out << endl << ";\t";
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, vcD2R[indices[i]], false);
	}
	out << "\t";
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, (vcDL[indices[i]] << 4) | vcRR[indices[i]], false);
	}
	out << "\t";
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, vcTL[indices[i]], i == 3);
	}
	out << endl;

	PrintMacro(out, "smpsVcAlgorithm");
	PrintHex2(out, vcAlgorithm, true);
	out << endl;

	PrintMacro(out, "smpsVcFeedback");
	PrintHex2(out, vcFeedback, true);
	out << endl;

	PrintMacro(out, "smpsVcUnusedBits");
	PrintHex2(out, vcUnusedBits, true);
	out << endl;

	PrintMacro(out, "smpsVcDetune");
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, vcDT[i], i == 3);
	}
	out << endl;
	PrintMacro(out, "smpsVcCoarseFreq");
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, vcCF[i], i == 3);
	}
	out << endl;

	PrintMacro(out, "smpsVcRateScale");
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, vcRS[i], i == 3);
	}
	out << endl;
	PrintMacro(out, "smpsVcAttackRate");
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, vcAR[i], i == 3);
	}
	out << endl;

	PrintMacro(out, "smpsVcAmpMod");
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, vcAM[i], i == 3);
	}
	out << endl;
	PrintMacro(out, "smpsVcDecayRate1");
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, vcD1R[i], i == 3);
	}
	out << endl;

	PrintMacro(out, "smpsVcDecayRate2");
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, vcD2R[i], i == 3);
	}
	out << endl;

	PrintMacro(out, "smpsVcDecayLevel");
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, vcDL[i], i == 3);
	}
	out << endl;
	PrintMacro(out, "smpsVcReleaseRate");
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, vcRR[i], i == 3);
	}
	out << endl;

	PrintMacro(out, "smpsVcTotalLevel");
	static int tlmasks[8] = {0x1, 0x1, 0x1, 0x1, 0x5, 0x7, 0x7, 0xf};
	int mask = tlmasks[vcAlgorithm];
	for (int i = 0; i < 4; i++) {
		PrintHex2(out, (mask & (1 << i)) != 0 ? vcTL[i] & 0x7f : vcTL[i], i == 3);
	}
	out << endl << endl;
}


