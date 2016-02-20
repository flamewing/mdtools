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

#ifndef __LIB_FRAMEMAPPING_H
#define __LIB_FRAMEMAPPING_H

#include <iosfwd>
#include <vector>
#include "singlemapping.h"
#include "framedplc.h"

class frame_mapping {
protected:
	std::vector<single_mapping> maps;
public:
	void read(std::istream &in, int ver);
	void write(std::ostream &out, int ver) const;
	void print() const;
	void split(frame_mapping const &src, frame_dplc &dplc);
	void merge(frame_mapping const &src, frame_dplc const &dplc);
	void change_pal(int srcpal, int dstpal);
	single_mapping const &get_maps(size_t i) const {
		return maps[i];
	}
	size_t size() const {
		return maps.size();
	}
	size_t size(int ver) const {
		return (ver == 1 ? 1 : 2) + single_mapping::size(ver) * maps.size();
	}
	bool operator<(frame_mapping const &rhs) const;
	bool operator==(frame_mapping const &rhs) const {
		return !(*this < rhs || rhs < *this);
	}
};

#endif // __LIB_FRAMEMAPPING_H
