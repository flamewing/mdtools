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

#ifndef __PATTERN_NAME_TABLE_H
#define __PATTERN_NAME_TABLE_H

#include "pattern_name.hh"

#include <array>
#include <sstream>

#include "bigendian_io.hh"
#include "mdcomp/enigma.hh"

template<unsigned width, unsigned height>
class Pattern_Name_Table {
public:
	enum {
		Width  = width,
		Height = height
	};
	typedef std::array<Pattern_Name, width> Line;

protected:
	std::array<Line, height> table;

public:
	Pattern_Name_Table() noexcept = default;
	Pattern_Name_Table(std::istream &in, bool const compressed) noexcept {
		std::stringstream sin(std::ios::in | std::ios::out | std::ios::binary);
		if (compressed) {
			enigma::decode(in, sin);
		} else {
			sin << in.rdbuf();
		}

		for (typename std::array<Line, height>::iterator it = table.begin();
		     it != table.end(); ++it) {
			Line &line = *it;
			for (typename Line::iterator it2 = line.begin();
			     it2 != line.end(); ++it2) {
				unsigned short pnt = BigEndian::Read2(sin);
				*it2 = Pattern_Name(sin.good() ? pnt : 0u);
			}
		}
	}
	virtual ~Pattern_Name_Table() noexcept {}

	Line const &operator[](size_t const n) const noexcept {	return table[n];	}
	Line       &operator[](size_t const n)       noexcept {	return table[n];	}

	virtual void write(std::ostream &out) const noexcept {
		for (typename std::array<Line, height>::const_iterator it = table.begin();
		     it != table.end(); ++it) {
			Line const &line = *it;
			for (typename Line::const_iterator it2 = line.begin();
			     it2 != line.end(); ++it2) {
				it2->write(out);
			}
		}
	}
};

typedef Pattern_Name_Table< 32,28> PlaneH32V28;
typedef Pattern_Name_Table< 40,28> PlaneH40V28;
typedef Pattern_Name_Table<128,28> PlaneH128V28;

#endif // __PATTERN_NAME_TABLE_H
