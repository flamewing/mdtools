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

#ifndef __LIB_SINGLEMAPPING_H
#define __LIB_SINGLEMAPPING_H

#include <iosfwd>
#include <map>
#include "singledplc.hh"

class single_mapping {
protected:
	unsigned short flags, tile;
	short xx, yy;
	unsigned char sx, sy;
public:
	void read(std::istream &in, int const ver);
	void write(std::ostream &out, int const ver) const;
	void print() const;
	void split(single_mapping const &src, single_dplc &dplc, std::map<size_t, size_t> &vram_map);
	void merge(single_mapping const &src, std::map<size_t, size_t> &vram_map);
	void change_pal(int const srcpal, int const dstpal);
	static size_t size(int const ver) {
		switch (ver) {
			case 1:
				return 5;
			case 2:
				return 8;
			default:
				return 6;
		}
	}
	unsigned short get_flags() const {
		return flags;
	}
	unsigned short get_tile() const {
		return tile;
	}
	short get_xx() const {
		return xx;
	}
	short get_yy() const {
		return yy;
	}
	unsigned char get_sx() const {
		return sx;
	}
	unsigned char get_sy() const {
		return sy;
	}
	void set_flags(unsigned short const t) {
		flags = t;
	}
	void set_tile(unsigned short const t) {
		tile = t;
	}
	void set_xx(short const t) {
		xx = t;
	}
	void set_yy(short const t) {
		yy = t;
	}
	void set_sx(unsigned char const t) {
		sx = t;
	}
	void set_sy(unsigned char const t) {
		sy = t;
	}
	bool operator<(single_mapping const &rhs) const {
		if (tile < rhs.tile) {
			return true;
		} else if (tile > rhs.tile) {
			return false;
		}
		if (sx < rhs.sx) {
			return true;
		} else if (sx > rhs.sx) {
			return false;
		}
		if (sy < rhs.sy) {
			return true;
		} else if (sy > rhs.sy) {
			return false;
		}
		if (flags < rhs.flags) {
			return true;
		} else if (flags > rhs.flags) {
			return false;
		}
		if (xx < rhs.xx) {
			return true;
		} else if (xx > rhs.xx) {
			return false;
		}
		if (yy < rhs.yy) {
			return true;
		} else /*if (yy > rhs.yy)*/ {
			return false;
		}
	}
	bool operator==(single_mapping const &rhs) const {
		return !(*this < rhs || rhs < *this);
	}
};

#endif // __LIB_SINGLEMAPPING_H
