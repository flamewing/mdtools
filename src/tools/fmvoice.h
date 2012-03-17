/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * s2-ssedit
 * Copyright (C) Flamewing 2011 <flamewing.sonic@gmail.com>
 * 
 * s2-ssedit is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * s2-ssedit is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _FMVOICE_H_
#define _FMVOICE_H_

#include <iosfwd>

void PrintMacro(std::ostream& out, char const *macro);
void PrintHex2(std::ostream& out, unsigned char c, bool last);
void PrintHex2Pre(std::ostream& out, unsigned char c, bool last);
void PrintHex4(std::ostream& out, unsigned short c, bool last);
void PrintName(std::ostream& out, std::string s, bool last);

class fm_voice
{
protected:
	unsigned char vcFeedback, vcAlgorithm, vcUnusedBits;
	unsigned char vcDT[4],  vcCF[4], vcRS[4], vcAR[4], vcAM[4], vcD1R[4],
	              vcD2R[4], vcDL[4], vcRR[4], vcTL[4];
public:
	void read(std::istream& in, int sonicver);
	void write(std::ostream& out, int sonicver) const;
	void print(std::ostream& out, int sonicver, int id) const;
};

#endif // _FMVOICE_H_
