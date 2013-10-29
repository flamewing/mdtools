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

#ifndef _FRAMEDPLC_H_
#define _FRAMEDPLC_H_

#include <map>
#include <vector>
#include "singledplc.h"

class istream;
class ostream;

class frame_dplc {
protected:
	std::vector<single_dplc> dplc;
public:
	void read(std::istream &in, int ver);
	void write(std::ostream &out, int ver) const;
	void print() const;
	void consolidate(frame_dplc const &src);
	void insert(single_dplc const &val);
	void build_vram_map(std::map<size_t, size_t>& vram_map) const;
	size_t size() const {
		return dplc.size();
	}
	size_t size(int ver) const {
		return (ver == 1 ? 1 : 2) + single_dplc::size(ver) * dplc.size();
	}
	bool operator<(frame_dplc const &rhs) const;
	bool operator==(frame_dplc const &rhs) const {
		return !(*this < rhs || rhs < *this);
	}
};

#endif // _FRAMEDPLC_H_
