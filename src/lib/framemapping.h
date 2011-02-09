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

#ifndef _FRAMEMAPPING_H_
#define _FRAMEMAPPING_H_

#include <vector>
#include "singlemapping.h"
#include "framedplc.h"

class istream;
class ostream;

class frame_mapping
{
protected:
	std::vector<single_mapping> maps;
public:
	void read(std::istream& in, bool s2);
	void write(std::ostream& out, bool s2) const;
	void print() const;
	void split(frame_mapping const& src, frame_dplc& dplc);
	void merge(frame_mapping const& src, frame_dplc const& dplc);
	void change_pal(int srcpal, int dstpal);
	size_t size(bool s2) const
	{	return 2 + single_mapping::size(s2) * maps.size();	}
};

#endif // _FRAMEMAPPING_H_
