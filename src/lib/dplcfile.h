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

#ifndef _DPLCFILE_H_
#define _DPLCFILE_H_

#include <vector>
#include "framedplc.h"

class istream;
class ostream;

class dplc_file
{
protected:
	std::vector<frame_dplc> frames;
public:
	void read(std::istream& in, int ver);
	void write(std::ostream& out, int ver, bool nullfirst) const;
	void print() const;
	void consolidate(dplc_file const& src);
	void insert(frame_dplc const& val);
	frame_dplc const& get_dplc(size_t i) const;
	size_t size(int ver) const;
};

#endif // _DPLCFILE_H_
