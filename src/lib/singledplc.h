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

#ifndef _SINGLEDPLC_H_
#define _SINGLEDPLC_H_

class istream;
class ostream;

class single_dplc
{
protected:
	unsigned short cnt, tile;
public:
	void read(std::istream& in, int ver);
	void write(std::ostream& out, int ver) const;
	void print() const;
	static size_t size(int ver)
	{	return 2;	}
	unsigned short get_cnt() const
	{	return cnt;	}
	unsigned short get_tile() const
	{	return tile;	}
	void set_cnt(unsigned short c)
	{	cnt = c;	}
	void set_tile(unsigned short t)
	{	tile = t;	}
	bool operator<(single_dplc const& rhs) const
	{
		if (cnt < rhs.cnt)
			return true;
		else if (cnt > rhs.cnt)
			return false;
		if (tile < rhs.tile)
			return true;
		return false;
	}
	bool operator==(single_dplc const& rhs) const
	{   return !(*this < rhs || rhs < *this);	}  
};

#endif // _SINGLEDPLC_H_
