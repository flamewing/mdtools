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

#include <mdtools/fmvoice.hh>

#include <cstdint>
#include <iomanip>
#include <istream>
#include <ostream>

#include <boost/io/ios_state.hpp>

#include <mdcomp/bigendian_io.hh>

using std::endl;
using std::hex;
using std::istream;
using std::left;
using std::ostream;
using std::setfill;
using std::setw;
using std::string;
using std::uppercase;

void PrintMacro(ostream& out, char const* macro) {
    boost::io::ios_all_saver flags(out);
    out << "\t" << setw(19) << setfill(' ') << left << macro << " ";
}

void PrintHex2(ostream& out, uint8_t const c, bool const last) {
    boost::io::ios_all_saver flags(out);
    out << "$" << hex << setw(2) << setfill('0') << uppercase
        << static_cast<unsigned int>(c);
    if (!last) {
        out << ", ";
    }
}

void PrintHex2Pre(ostream& out, uint8_t const c, bool const first) {
    boost::io::ios_all_saver flags(out);
    if (!first) {
        out << ", ";
    }
    out << "$" << hex << setw(2) << setfill('0') << uppercase
        << static_cast<unsigned int>(c);
}

void PrintName(ostream& out, string const& s, bool const first) {
    if (!first) {
        out << ", ";
    }
    out << s;
}

void PrintHex4(ostream& out, uint16_t const c, bool const last) {
    boost::io::ios_all_saver flags(out);
    out << "$" << hex << setw(4) << setfill('0') << uppercase
        << static_cast<unsigned int>(c);
    if (!last) {
        out << ", ";
    }
}

void fm_voice::read(istream& in, int const sonicver) {
    uint8_t const vc = Read1(in);
    vcUnusedBits     = (vc >> 6) & 3;
    vcFeedback       = (vc >> 3) & 7;
    vcAlgorithm      = vc & 7;

    constexpr static int const s2_indices[4]     = {3, 1, 2, 0};
    constexpr static int const normal_indices[4] = {3, 2, 1, 0};
    int const(&indices)[4] = sonicver == 2 ? s2_indices : normal_indices;

    for (int index : indices) {
        uint8_t const c = Read1(in);
        vcDT[index]     = (c >> 4) & 0xf;
        vcCF[index]     = c & 0xf;
    }
    for (int index : indices) {
        uint8_t const c = Read1(in);
        vcRS[index]     = (c >> 6) & 0x3;
        vcAR[index]     = c & 0x3f;
    }
    for (int index : indices) {
        uint8_t const c = Read1(in);
        vcAM[index]     = (c >> 7) & 1;
        vcD1RUnk[index] = (c >> 5) & 3;
        vcD1R[index]    = c & 0x1f;
    }
    for (int index : indices) {
        vcD2R[index] = Read1(in);
    }
    for (int index : indices) {
        uint8_t const c = Read1(in);
        vcDL[index]     = (c >> 4) & 0xf;
        vcRR[index]     = c & 0xf;
    }
    for (int index : indices) {
        vcTL[index] = Read1(in);
    }
}

void fm_voice::write(ostream& out, int const sonicver) const {
    Write1(out, (vcUnusedBits << 6) | (vcFeedback << 3) | vcAlgorithm);
    constexpr static int const s2_indices[4]     = {3, 1, 2, 0};
    constexpr static int const normal_indices[4] = {3, 2, 1, 0};
    int const(&indices)[4] = sonicver == 2 ? s2_indices : normal_indices;

    for (int index : indices) {
        Write1(out, (vcDT[index] << 4) | vcCF[index]);
    }
    for (int index : indices) {
        Write1(out, (vcRS[index] << 6) | vcAR[index]);
    }
    for (int index : indices) {
        Write1(out, (vcAM[index] << 7) | (vcD1RUnk[index] << 5) | vcD1R[index]);
    }
    for (int index : indices) {
        Write1(out, vcD2R[index]);
    }
    for (int index : indices) {
        Write1(out, (vcDL[index] << 4) | vcRR[index]);
    }
    for (int index : indices) {
        Write1(out, vcTL[index]);
    }
}

void fm_voice::print(ostream& out, int const sonicver, int const id) const {
    out << ";\tVoice ";
    PrintHex2(out, id, true);
    out << endl << ";\t";
    PrintHex2(out, (vcUnusedBits << 6) | (vcFeedback << 3) | vcAlgorithm, true);
    constexpr static int const s2_indices[4]     = {3, 2, 1, 0};
    constexpr static int const normal_indices[4] = {3, 2, 1, 0};
    int const(&indices)[4] = sonicver == 2 ? s2_indices : normal_indices;

    out << endl << ";\t";
    for (int index : indices) {
        PrintHex2(out, (vcDT[index] << 4) | vcCF[index], false);
    }
    out << "\t";
    for (int index : indices) {
        PrintHex2(out, (vcRS[index] << 6) | vcAR[index], false);
    }
    out << "\t";
    for (int index : indices) {
        PrintHex2(
            out, (vcAM[index] << 7) | (vcD1RUnk[index] << 5) | vcD1R[index],
            index == indices[3]);
    }
    out << endl << ";\t";
    for (int index : indices) {
        PrintHex2(out, vcD2R[index], false);
    }
    out << "\t";
    for (int index : indices) {
        PrintHex2(out, (vcDL[index] << 4) | vcRR[index], false);
    }
    out << "\t";
    for (int index : indices) {
        PrintHex2(out, vcTL[index], index == indices[3]);
    }
    out << endl;

    PrintMacro(out, "smpsVcAlgorithm");
    PrintHex2(out, vcAlgorithm, true);
    out << endl;

    PrintMacro(out, "smpsVcFeedback");
    PrintHex2(out, vcFeedback, true);
    out << endl;

    PrintMacro(out, "smpsVcUnusedBits");
    if ((vcD1RUnk[0] != 0U) || (vcD1RUnk[1] != 0U) || (vcD1RUnk[2] != 0U) ||
        (vcD1RUnk[3] != 0U)) {
        PrintHex2(out, vcUnusedBits, false);
        for (int i = 0; i < 4; i++) {
            PrintHex2(out, vcD1RUnk[i], i == 3);
        }
    } else {
        PrintHex2(out, vcUnusedBits, true);
    }
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
    for (int i = 0; i < 4; i++) {
        PrintHex2(out, vcTL[i], i == 3);
    }
    out << endl << endl;
}
