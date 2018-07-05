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

#include <iostream>
#include <iomanip>
#include <fstream>
#include <map>
#include <set>
#include <vector>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "bigendian_io.hh"

using namespace std;

class Block {
protected:
	unsigned short block;
public:
	constexpr Block() noexcept : block(0) {
	}
	constexpr Block(Block const &other) noexcept = default;
	constexpr Block(Block &&other) noexcept = default;
	Block &operator=(Block const &other) noexcept = default;
	Block &operator=(Block &&other) noexcept = default;
	void read(istream &in) noexcept {
		block = BigEndian::Read2(in);
	}
	void write(ostream &out) noexcept {
		BigEndian::Write2(out, block);
	}
	constexpr unsigned short get_block() const noexcept {
		return block;
	}
	constexpr unsigned short get_index() const noexcept {
		return block & 0x3FF;
	}
	constexpr void set_block(unsigned short blk) {
		block = blk;
	}
	constexpr void set_index(unsigned short idx) {
		block &= (~0x3FF);
		block |= (idx & 0x3FF);
	}
};

class BlockS1 : public Block {
public:
	constexpr BlockS1() noexcept = default;
	constexpr BlockS1(BlockS1 const &other) noexcept = default;
	constexpr BlockS1(BlockS1 &&other) noexcept = default;
	BlockS1 &operator=(BlockS1 const &other) noexcept = default;
	BlockS1 &operator=(BlockS1 &&other) noexcept = default;
	constexpr bool operator<(BlockS1 const &other) const noexcept {
		return block < other.block;
	}
	constexpr bool operator==(BlockS1 const &other) const noexcept {
		return !(*this < other || other < *this);
	}
	constexpr bool less(BlockS1 const &other) const noexcept {
		return (block & (~0x6000)) < (other.block & (~0x6000));
	}
	constexpr bool equal(BlockS1 const &other) const noexcept {
		return (block & (~0x6000)) == (other.block & (~0x6000));
	}
	constexpr bool get_xflip() const noexcept {
		return (block & 0x0800) != 0;
	}
	constexpr bool get_yflip() const noexcept {
		return (block & 0x1000) != 0;
	}
	constexpr unsigned short get_collision() const noexcept {
		return (block >> 13) & 3;
	}
	constexpr void set_xflip() noexcept {
		block |= 0x0800;
	}
	constexpr void set_yflip() noexcept {
		block |= 0x1000;
	}
	constexpr void clear_xflip() noexcept {
		block &= (~0x0800);
	}
	constexpr void clear_yflip() noexcept {
		block &= (~0x1000);
	}
	constexpr void set_collision(unsigned short col) noexcept {
		block &= (~0x6000);
		block |= ((col & 3) << 13);
	}
};

class BlockS2 : public Block {
public:
	constexpr BlockS2() noexcept = default;
	constexpr BlockS2(BlockS2 const &other) noexcept = default;
	constexpr BlockS2(BlockS2 &&other) noexcept = default;
	BlockS2 &operator=(BlockS2 const &other) noexcept = default;
	BlockS2 &operator=(BlockS2 &&other) noexcept = default;
	constexpr bool operator<(BlockS2 const &other) const noexcept {
		return block < other.block;
	}
	constexpr bool operator==(BlockS2 const &other) const noexcept {
		return !(*this < other || other < *this);
	}
	constexpr bool less(BlockS2 const &other) const noexcept {
		return (block & (~0xF000)) < (other.block & (~0xF000));
	}
	constexpr bool equal(BlockS2 const &other) const noexcept {
		return (block & (~0xF000)) == (other.block & (~0xF000));
	}
	constexpr bool get_xflip() const noexcept {
		return (block & 0x0400) != 0;
	}
	constexpr bool get_yflip() const noexcept {
		return (block & 0x0800) != 0;
	}
	constexpr unsigned short get_collision1() const noexcept {
		return (block >> 12) & 3;
	}
	constexpr unsigned short get_collision2() const noexcept {
		return (block >> 14) & 3;
	}
	constexpr void set_xflip() noexcept {
		block |= 0x0400;
	}
	constexpr void set_yflip() noexcept {
		block |= 0x0800;
	}
	constexpr void clear_xflip() noexcept {
		block &= (~0x0400);
	}
	constexpr void clear_yflip() noexcept {
		block &= (~0x0800);
	}
	constexpr void set_collision1(unsigned short col) noexcept {
		block &= (~0x3000);
		block |= ((col & 3) << 12);
	}
	constexpr void set_collision2(unsigned short col) noexcept {
		block &= (~0xC000);
		block |= ((col & 3) << 14);
	}
	constexpr BlockS2 &merge(BlockS1 const &high, BlockS1 const &low) noexcept {
		assert(high.get_index() == low.get_index());
		assert(high.get_xflip() == low.get_xflip());
		assert(high.get_yflip() == low.get_yflip());
		set_index(high.get_index());
		if (high.get_xflip()) {
			set_xflip();
		} else {
			clear_xflip();
		}
		if (high.get_yflip()) {
			set_yflip();
		} else {
			clear_yflip();
		}
		set_collision1(high.get_collision());
		set_collision2(low.get_collision());
		return *this;
	}
};

template<int Dim, typename Blk>
class Chunk {
protected:
	Blk blocks[Dim * Dim];
public:
	constexpr Chunk() noexcept = default;
	constexpr Chunk(Chunk<Dim, Blk> const &other) noexcept = default;
	constexpr Chunk(Chunk<Dim, Blk> &&other) noexcept = default;
	constexpr Chunk &operator=(Chunk const &other) noexcept {
		if (this != &other) {
			for (size_t ii = 0; ii < Dim * Dim; ii++) {
				blocks[ii] = other.blocks[ii];
			}
		}
		return *this;
	}
	constexpr Chunk &operator=(Chunk &&other) noexcept {
		if (this != &other) {
			for (size_t ii = 0; ii < Dim * Dim; ii++) {
				blocks[ii] = other.blocks[ii];
			}
		}
		return *this;
	}
	constexpr void read(istream &in) noexcept {
		for (auto & elem : blocks) {
			elem.read(in);
		}
	}
	constexpr void write(ostream &out) noexcept {
		for (size_t ii = 0; ii < Dim * Dim; ii++) {
			blocks[ii].write(out);
		}
	}
	constexpr bool operator<(Chunk const &other) const noexcept {
		for (size_t ii = 0; ii < Dim * Dim; ii++) {
			if (blocks[ii] < other.blocks[ii]) {
				return true;
			} else if (other.blocks[ii] < blocks[ii]) {
				return false;
			}
		}
		return false;
	}
	constexpr bool operator==(Chunk const &other) const noexcept {
		return !(*this < other || other < *this);
	}
	constexpr bool less(Chunk const &other) const noexcept {
		for (size_t ii = 0; ii < Dim * Dim; ii++) {
			if (blocks[ii].less(other.blocks[ii])) {
				return true;
			} else if (other.blocks[ii].less(blocks[ii])) {
				return false;
			}
		}
		return false;
	}
	constexpr bool equal(Chunk const &other) const noexcept {
		for (size_t ii = 0; ii < Dim * Dim; ii++) {
			if (!blocks[ii].equal(other.blocks[ii])) {
				return false;
			}
		}
		return true;
	}
	constexpr Blk const &get_block(int ii) const noexcept {
		return blocks[ii];
	}
	constexpr Blk &get_block(int ii) noexcept {
		return blocks[ii];
	}
	constexpr void set_block(int ii, Blk const &blk) noexcept {
		blocks[ii] = blk;
	}
};

struct ChunkMap {
	unsigned char tl, tr, bl, br;
};

typedef Chunk<16,BlockS1> ChunkS1;
typedef Chunk< 8,BlockS2> ChunkS2;

void split_chunks(ChunkS1 const &highchunk, ChunkS1 const &lowchunk,
                  ChunkS2 &tlchunk, ChunkS2 &trchunk,
                  ChunkS2 &blchunk, ChunkS2 &brchunk) noexcept {
	for (size_t ii = 0; ii < 16; ii++) {
		for (size_t jj = 0; jj < 16; jj++) {
			ChunkS2 &curr = ii < 8 ? (jj < 8 ? tlchunk : blchunk)
			                       : (jj < 8 ? trchunk : brchunk);
			BlockS1 const &highblk = highchunk.get_block(jj * 16 + ii);
			BlockS1 const &lowblk = lowchunk.get_block(jj * 16 + ii);
			curr.get_block((jj % 8) * 8 + (ii % 8)).merge(highblk, lowblk);
		}
	}
}

constexpr bool Compare_NoCollison(ChunkS2 const &lhs, ChunkS2 const &rhs) noexcept {
	return lhs.less(rhs);
}

unsigned char *get_chunk_remaps(int levelid) noexcept {
	auto remaps = new unsigned char[256];
	for (size_t ii = 0; ii < 256; ii++) {
		if (levelid == 4) { // WWZ
			if (ii == 0x15) {
				remaps[ii] = 0x60;
			} else if (ii == 0x1E) {
				remaps[ii] = 0x61;
			} else if (ii == 0x1F) {
				remaps[ii] = 0x62;
			} else if (ii == 0x32) {
				remaps[ii] = 0x63;
			} else {
				remaps[ii] = static_cast<unsigned char>(ii & 0x7F);
			}
		} else if (levelid == 5) { // SSZ
			if (ii == 0x04 || ii == 0x06 || ii == 0x28 || ii == 0x3C
			    || ii == 0x37 || ii == 0x2F || ii == 0x16) {
				remaps[ii] = static_cast<unsigned char>((ii + 1) & 0x7F);
			} else {
				remaps[ii] = static_cast<unsigned char>(ii & 0x7F);
			}
		} else if (levelid == 6) { // MMZ
			if (ii == 0x46) {
				remaps[ii] = 0x6A;
			} else if (ii == 0x48) {
				remaps[ii] = 0x6B;
			} else if (ii == 0x4A) {
				remaps[ii] = 0x6C;
			} else if (ii == 0x10) {
				remaps[ii] = 0x6D;
			} else if (ii == 0x63) {
				remaps[ii] = 0x6E;
			} else if (ii == 0x43) {
				remaps[ii] = 0x6F;
			} else {
				remaps[ii] = static_cast<unsigned char>(ii & 0x7F);
			}
		} else {
			if (ii == 0x28) {
				remaps[ii] = 0x51;
			} else {
				remaps[ii] = static_cast<unsigned char>((ii + 1) & 0x7F);
			}
		}
	}
	return remaps;
}

int main(int argc, char *argv[]) {
	if (argc != 5) {
		cerr << "Usage: " << argv[0] << " levelid layoutfgfile layoutbgfile chunkfile" << endl;
		cerr << "levelid     \t0 = PPZ, 1 = CCZ, 2 = TTZ, 3 = QQZ, 4 = WWZ, 5 = SSZ, 6 = MMZ" << endl;
		cerr << "layoutfgfile\tLevel's FG layout file, assumed to be in S1 format (uncompressed)" << endl;
		cerr << "layoutbgfile\tLevel's BG layout file, assumed to be in S1 format (uncompressed)" << endl;
		cerr << "chunkfile   \tLevel's 256x256 chunk file, assumed to be in SCD format (uncompressed)" << endl << endl;
		return 1;
	}
	
	int levelid = atoi(argv[1]);
	if (levelid < 0 || levelid > 6) {
		cerr << "Input level ID " << argv[1] << " must be a number: 0 = PPZ, 1 = CCZ, 2 = TTZ, 3 = QQZ, 4 = WWZ, 5 = SSZ, 6 = MMZ." << endl;
		return 2;
	}
	
	ifstream flayoutFG(argv[2], ios::in|ios::binary), flayoutBG(argv[3], ios::in|ios::binary);
	if (!flayoutFG.good()) {
		cerr << "Input layout file '" << argv[2] << "' could not be opened." << endl;
		return 3;
	}
	if (!flayoutBG.good()) {
		cerr << "Input layout file '" << argv[3] << "' could not be opened." << endl;
		return 4;
	}

	ifstream fchunks(argv[4], ios::in|ios::binary);
	if (!fchunks.good()) {
		cerr << "Input chunks file '" << argv[4] << "' could not be opened." << endl;
		return 5;
	}

	cout << "==============================================================" << endl;
	cout << argv[4] << "\t" << levelid << endl;
	unsigned char *remaps = get_chunk_remaps(levelid);
	
	vector<ChunkS1> chunkss1;
	chunkss1.reserve(256);

	do {
		ChunkS1 chunk;
		chunk.read(fchunks);
		chunkss1.push_back(chunk);
	} while (fchunks.good());
	
	fchunks.close();

	size_t fgw = flayoutFG.get() + 1, fgh = flayoutFG.get() + 1;
	size_t bgw = flayoutBG.get() + 1, bgh = flayoutBG.get() + 1;
	cout << "Layout sizes: FG: (" << fgw << ", " << fgh << "), BG: (" << bgw << ", " << bgh << ")" << endl;
	auto layouts1FG = new unsigned char[fgw * fgh], layouts1BG = new unsigned char[bgw * bgh];
	set<unsigned char> usedchunks;
	vector<unsigned char> needremap;
	needremap.resize(chunkss1.size());
	set<ChunkS1> uniquechunks;

	for (size_t ii = 0; ii < fgw * fgh; ii++) {
		char cc;
		flayoutFG.get(cc);
		unsigned char uc = static_cast<unsigned char>(cc);
		layouts1FG[ii] = uc;
		if ((uc & 0x80) != 0) {
			needremap[uc & 0x7F] = true;
			//usedchunks.insert(remaps[uc & 0x7F]);
		}
		usedchunks.insert(uc & 0x7F);
		if ((uc & 0x7F) != 0) {
			uniquechunks.insert(chunkss1[(uc & 0x7F) - 1]);
		}
	}
	flayoutFG.close();
	size_t usedfg = usedchunks.size();

	for (size_t ii = 0; ii < bgw * bgh; ii++) {
		char cc;
		flayoutBG.get(cc);
		unsigned char uc = static_cast<unsigned char>(cc);
		layouts1BG[ii] = uc;
		if ((uc & 0x80) != 0) {
			needremap[uc & 0x7F] = true;
			//usedchunks.insert(remaps[uc & 0x7F]);
		}
		usedchunks.insert(uc & 0x7F);
		if ((uc & 0x7F) != 0) {
			uniquechunks.insert(chunkss1[(uc & 0x7F) - 1]);
		}
	}
	flayoutBG.close();
	cout << "Number of chunks: total: " << chunkss1.size() << ", FG: " << usedfg
	     << ", BG: " << (usedchunks.size() - usedfg) << ", used: " << usedchunks.size()
	     << ", unique: " << uniquechunks.size() << endl;

	size_t s2w = 128, s2h = 20;
	auto layouts2 = new unsigned char[s2w * s2h];
	memset(layouts2, 0, s2w * s2h);
	
	vector<ChunkS2> chunkss2;
	chunkss2.reserve(256);
	map<ChunkS2,size_t> chunkids;
	map<unsigned char,ChunkMap> s1s2chunkidmap;

	for (auto chunk : usedchunks) {
		if (!chunk) {
			continue;
		}
		chunk--;
		ChunkS1 highchunk = chunkss1[chunk];
		ChunkS1 lowchunk = needremap[chunk] ? chunkss1[remaps[chunk]] : highchunk;
		ChunkS2 tlchunk, trchunk, blchunk, brchunk;
		split_chunks(highchunk, lowchunk, tlchunk, trchunk, blchunk, brchunk);
		map<ChunkS2,size_t>::iterator cit;
		ChunkMap map;
		if ((cit = chunkids.find(tlchunk)) != chunkids.end()) {
			map.tl = cit->second;
		} else {
			map.tl = chunkids[tlchunk] = chunkss2.size();
			chunkss2.push_back(tlchunk);
		}
		if ((cit = chunkids.find(trchunk)) != chunkids.end()) {
			map.tr = cit->second;
		} else {
			map.tr = chunkids[trchunk] = chunkss2.size();
			chunkss2.push_back(trchunk);
		}
		if ((cit = chunkids.find(blchunk)) != chunkids.end()) {
			map.bl = cit->second;
		} else {
			map.bl = chunkids[blchunk] = chunkss2.size();
			chunkss2.push_back(blchunk);
		}
		if ((cit = chunkids.find(brchunk)) != chunkids.end()) {
			map.br = cit->second;
		} else {
			map.br = chunkids[brchunk] = chunkss2.size();
			chunkss2.push_back(brchunk);
		}
		s1s2chunkidmap[chunk] = map;
	}

	cout << "Number of S2 chunks: " << chunkss2.size() << endl;

	/*
	for (size_t ii = 0; ii < fgw; ii++) {
		for (jj = 0; jj < fgh; jj++) {
			char chunk = layouts1FG[jj * fgw + ii];
			ChunkMap const &map = s1s2chunkidmap[chunk];
			layouts2[(jj + 0) * 256 + (2 * ii + 0) +   0] = map.tl;
			layouts2[(jj + 0) * 256 + (2 * ii + 1) +   0] = map.tr;
			layouts2[(jj + 1) * 256 + (2 * ii + 0) +   0] = map.bl;
			layouts2[(jj + 1) * 256 + (2 * ii + 1) +   0] = map.br;
		}
	}
	
	for (size_t ii = 0; ii < fgw; ii++) {
		for (jj = 0; jj < fgh; jj++) {
			char chunk = layouts1BG[jj * fgw + ii];
			ChunkMap const &map = s1s2chunkidmap[chunk];
			layouts2[(jj + 0) * 256 + (2 * ii + 0) + 128] = map.tl;
			layouts2[(jj + 0) * 256 + (2 * ii + 1) + 128] = map.tr;
			layouts2[(jj + 1) * 256 + (2 * ii + 0) + 128] = map.bl;
			layouts2[(jj + 1) * 256 + (2 * ii + 1) + 128] = map.br;
		}
	}
	*/

	delete [] layouts2;
	delete [] layouts1BG;
	delete [] layouts1FG;
	delete [] remaps;
	return 0;
}
