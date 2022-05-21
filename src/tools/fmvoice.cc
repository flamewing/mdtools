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

#include <boost/io/ios_state.hpp>
#include <mdcomp/bigendian_io.hh>
#include <mdtools/fmvoice.hh>

#include <array>
#include <cstdint>
#include <iomanip>
#include <istream>
#include <ostream>

using std::endl;
using std::hex;
using std::istream;
using std::left;
using std::ostream;
using std::setfill;
using std::setw;
using std::string;
using std::uppercase;

using namespace std::literals::string_view_literals;

void PrintMacro(ostream& output, std::string_view macro) {
    boost::io::ios_all_saver flags(output);
    output << "\t" << setw(19) << setfill(' ') << left << macro << " ";
}

void PrintHex2(ostream& output, uint8_t const value, bool const last) {
    boost::io::ios_all_saver flags(output);
    output << "$" << hex << setw(2) << setfill('0') << uppercase
           << static_cast<unsigned int>(value);
    if (!last) {
        output << ", ";
    }
}

void PrintHex2Pre(ostream& output, uint8_t const value, bool const first) {
    boost::io::ios_all_saver flags(output);
    if (!first) {
        output << ", ";
    }
    output << "$" << hex << setw(2) << setfill('0') << uppercase
           << static_cast<unsigned int>(value);
}

void PrintName(ostream& output, std::string_view name, bool const first) {
    if (!first) {
        output << ", ";
    }
    output << name;
}

void PrintHex4(ostream& output, uint16_t const value, bool const last) {
    boost::io::ios_all_saver flags(output);
    output << "$" << hex << setw(4) << setfill('0') << uppercase
           << static_cast<unsigned int>(value);
    if (!last) {
        output << ", ";
    }
}

void fm_voice::read(istream& input, int const sonic_version) {
    {
        uint32_t const value = Read1(input);
        vcUnusedBits         = (value >> 6U) & 3U;
        vcFeedback           = (value >> 3U) & 7U;
        vcAlgorithm          = value & 7U;
    }

    constexpr static std::array const s2_indices     = {3, 1, 2, 0};
    constexpr static std::array const normal_indices = {3, 2, 1, 0};
    const auto& indices = sonic_version == 2 ? s2_indices : normal_indices;

    for (int index : indices) {
        uint32_t const value = Read1(input);
        vcDT[index]          = (value >> 4U) & 0xfU;
        vcCF[index]          = value & 0xfU;
    }
    for (int index : indices) {
        uint32_t const value = Read1(input);
        vcRS[index]          = (value >> 6U) & 0x3U;
        vcAR[index]          = value & 0x3fU;
    }
    for (int index : indices) {
        uint32_t const value = Read1(input);
        vcAM[index]          = (value >> 7U) & 1U;
        vcD1RUnk[index]      = (value >> 5U) & 3U;
        vcD1R[index]         = value & 0x1fU;
    }
    for (int index : indices) {
        vcD2R[index] = Read1(input);
    }
    for (int index : indices) {
        uint32_t const value = Read1(input);
        vcDL[index]          = (value >> 4U) & 0xfU;
        vcRR[index]          = value & 0xfU;
    }
    for (int index : indices) {
        vcTL[index] = Read1(input);
    }
}

void fm_voice::write(ostream& output, int const sonic_version) const {
    Write1(output, (vcUnusedBits << 6U) | (vcFeedback << 3U) | vcAlgorithm);
    constexpr static std::array const s2_indices     = {3, 1, 2, 0};
    constexpr static std::array const normal_indices = {3, 2, 1, 0};
    const auto& indices = sonic_version == 2 ? s2_indices : normal_indices;

    for (int index : indices) {
        Write1(output, (vcDT[index] << 4U) | vcCF[index]);
    }
    for (int index : indices) {
        Write1(output, (vcRS[index] << 6U) | vcAR[index]);
    }
    for (int index : indices) {
        Write1(output,
               (vcAM[index] << 7U) | (vcD1RUnk[index] << 5U) | vcD1R[index]);
    }
    for (int index : indices) {
        Write1(output, vcD2R[index]);
    }
    for (int index : indices) {
        Write1(output, (vcDL[index] << 4U) | vcRR[index]);
    }
    for (int index : indices) {
        Write1(output, vcTL[index]);
    }
}

void fm_voice::print(
        ostream& output, int const sonic_version, int const voice_id) const {
    output << ";\tVoice ";
    PrintHex2(output, voice_id, true);
    output << endl << ";\t";
    PrintHex2(
            output, (vcUnusedBits << 6U) | (vcFeedback << 3U) | vcAlgorithm,
            true);
    constexpr static std::array const s2_indices     = {3, 2, 1, 0};
    constexpr static std::array const normal_indices = {3, 2, 1, 0};
    const auto& indices = sonic_version == 2 ? s2_indices : normal_indices;

    output << endl << ";\t";
    for (int index : indices) {
        PrintHex2(output, (vcDT[index] << 4U) | vcCF[index], false);
    }
    output << "\t";
    for (int index : indices) {
        PrintHex2(output, (vcRS[index] << 6U) | vcAR[index], false);
    }
    output << "\t";
    for (int index : indices) {
        PrintHex2(
                output,
                (vcAM[index] << 7U) | (vcD1RUnk[index] << 5U) | vcD1R[index],
                index == indices[3]);
    }
    output << endl << ";\t";
    for (int index : indices) {
        PrintHex2(output, vcD2R[index], false);
    }
    output << "\t";
    for (int index : indices) {
        PrintHex2(output, (vcDL[index] << 4U) | vcRR[index], false);
    }
    output << "\t";
    for (int index : indices) {
        PrintHex2(output, vcTL[index], index == indices[3]);
    }
    output << endl;

    PrintMacro(output, "smpsVcAlgorithm"sv);
    PrintHex2(output, vcAlgorithm, true);
    output << endl;

    PrintMacro(output, "smpsVcFeedback"sv);
    PrintHex2(output, vcFeedback, true);
    output << endl;

    PrintMacro(output, "smpsVcUnusedBits"sv);
    if ((vcD1RUnk[0] != 0U) || (vcD1RUnk[1] != 0U) || (vcD1RUnk[2] != 0U)
        || (vcD1RUnk[3] != 0U)) {
        PrintHex2(output, vcUnusedBits, false);
        for (int i = 0; i < 4; i++) {
            PrintHex2(output, vcD1RUnk[i], i == 3);
        }
    } else {
        PrintHex2(output, vcUnusedBits, true);
    }
    output << endl;

    PrintMacro(output, "smpsVcDetune"sv);
    for (int i = 0; i < 4; i++) {
        PrintHex2(output, vcDT[i], i == 3);
    }
    output << endl;
    PrintMacro(output, "smpsVcCoarseFreq"sv);
    for (int i = 0; i < 4; i++) {
        PrintHex2(output, vcCF[i], i == 3);
    }
    output << endl;

    PrintMacro(output, "smpsVcRateScale"sv);
    for (int i = 0; i < 4; i++) {
        PrintHex2(output, vcRS[i], i == 3);
    }
    output << endl;
    PrintMacro(output, "smpsVcAttackRate"sv);
    for (int i = 0; i < 4; i++) {
        PrintHex2(output, vcAR[i], i == 3);
    }
    output << endl;

    PrintMacro(output, "smpsVcAmpMod"sv);
    for (int i = 0; i < 4; i++) {
        PrintHex2(output, vcAM[i], i == 3);
    }
    output << endl;
    PrintMacro(output, "smpsVcDecayRate1"sv);
    for (int i = 0; i < 4; i++) {
        PrintHex2(output, vcD1R[i], i == 3);
    }
    output << endl;

    PrintMacro(output, "smpsVcDecayRate2"sv);
    for (int i = 0; i < 4; i++) {
        PrintHex2(output, vcD2R[i], i == 3);
    }
    output << endl;

    PrintMacro(output, "smpsVcDecayLevel"sv);
    for (int i = 0; i < 4; i++) {
        PrintHex2(output, vcDL[i], i == 3);
    }
    output << endl;
    PrintMacro(output, "smpsVcReleaseRate"sv);
    for (int i = 0; i < 4; i++) {
        PrintHex2(output, vcRR[i], i == 3);
    }
    output << endl;

    PrintMacro(output, "smpsVcTotalLevel"sv);
    for (int i = 0; i < 4; i++) {
        PrintHex2(output, vcTL[i], i == 3);
    }
    output << endl << endl;
}
