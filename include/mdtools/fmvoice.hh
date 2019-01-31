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

#ifndef __TOOLS_FMVOICE_H
#define __TOOLS_FMVOICE_H

#include <iosfwd>

void PrintMacro(std::ostream& out, char const* macro);
void PrintHex2(std::ostream& out, unsigned char const c, bool const last);
void PrintHex2Pre(std::ostream& out, unsigned char const c, bool const first);
void PrintHex4(std::ostream& out, unsigned short const c, bool const last);
void PrintName(std::ostream& out, std::string const& s, bool const first);

class fm_voice {
protected:
    unsigned char vcFeedback, vcAlgorithm, vcUnusedBits;
    unsigned char vcDT[4], vcCF[4], vcRS[4], vcAR[4], vcAM[4], vcD1R[4],
        vcD2R[4], vcDL[4], vcRR[4], vcTL[4], vcD1RUnk[4];

public:
    void read(std::istream& in, int const sonicver);
    void write(std::ostream& out, int const sonicver) const;
    void print(std::ostream& out, int const sonicver, int const id) const;
};

#endif // __TOOLS_FMVOICE_H
