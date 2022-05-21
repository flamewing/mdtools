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

#ifndef TOOLS_FMVOICE_HH
#define TOOLS_FMVOICE_HH

#include <array>
#include <cstdint>
#include <iosfwd>
#include <string_view>

void PrintMacro(std::ostream& out, std::string_view macro);
void PrintHex2(std::ostream& out, uint8_t c, bool last);
void PrintHex2Pre(std::ostream& out, uint8_t c, bool first);
void PrintHex4(std::ostream& out, uint16_t c, bool last);
void PrintName(std::ostream& out, std::string_view s, bool first);

class fm_voice {
private:
    uint32_t vcFeedback;
    uint32_t vcAlgorithm;
    uint32_t vcUnusedBits;

    std::array<uint32_t, 4> vcDT;
    std::array<uint32_t, 4> vcCF;
    std::array<uint32_t, 4> vcRS;
    std::array<uint32_t, 4> vcAR;
    std::array<uint32_t, 4> vcAM;
    std::array<uint32_t, 4> vcD1R;
    std::array<uint32_t, 4> vcD2R;
    std::array<uint32_t, 4> vcDL;
    std::array<uint32_t, 4> vcRR;
    std::array<uint32_t, 4> vcTL;
    std::array<uint32_t, 4> vcD1RUnk;

public:
    void read(std::istream& in, int sonicver);
    void write(std::ostream& out, int sonicver) const;
    void print(std::ostream& out, int sonicver, int id) const;
};

#endif    // TOOLS_FMVOICE_HH
