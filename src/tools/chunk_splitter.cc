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

#include <mdcomp/bigendian_io.hh>

#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

using std::array;
using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::integer_sequence;
using std::ios;
using std::istream;
using std::make_integer_sequence;
using std::map;
using std::ostream;
using std::set;
using std::vector;

class Block {
private:
    uint16_t block{0};

public:
    void read(istream& in) noexcept {
        block = BigEndian::Read2(in);
    }
    void write(ostream& out) const noexcept {
        BigEndian::Write2(out, block);
    }
    constexpr uint32_t get_block() const noexcept {
        return block;
    }
    constexpr uint32_t get_index() const noexcept {
        return block & 0x3FFU;
    }
    constexpr void set_block(uint16_t blk) {
        block = blk;
    }
    constexpr void set_index(uint16_t idx) {
        block &= (~0x3FFU);
        block |= (idx & 0x3FFU);
    }
};

class BlockS1 : public Block {
public:
    constexpr bool operator<(BlockS1 const& other) const noexcept {
        return get_block() < other.get_block();
    }
    constexpr bool operator==(BlockS1 const& other) const noexcept {
        return !(*this < other || other < *this);
    }
    constexpr bool less(BlockS1 const& other) const noexcept {
        return (get_block() & (~0x6000U)) < (other.get_block() & (~0x6000U));
    }
    constexpr bool equal(BlockS1 const& other) const noexcept {
        return (get_block() & (~0x6000U)) == (other.get_block() & (~0x6000U));
    }
    constexpr bool get_xflip() const noexcept {
        return (get_block() & 0x0800U) != 0;
    }
    constexpr bool get_yflip() const noexcept {
        return (get_block() & 0x1000U) != 0;
    }
    constexpr uint16_t get_collision() const noexcept {
        return (get_block() >> 13U) & 3U;
    }
    constexpr void set_xflip() noexcept {
        set_block(get_block() | 0x0800U);
    }
    constexpr void set_yflip() noexcept {
        set_block(get_block() | 0x1000U);
    }
    constexpr void clear_xflip() noexcept {
        set_block(get_block() & (~0x0800U));
    }
    constexpr void clear_yflip() noexcept {
        set_block(get_block() & (~0x1000U));
    }
    constexpr void set_collision(uint16_t col) noexcept {
        uint32_t blk = get_block() & (~0x6000U);
        set_block(blk | ((col & 3U) << 13U));
    }
};

class BlockS2 : public Block {
public:
    constexpr bool operator<(BlockS2 const& other) const noexcept {
        return get_block() < other.get_block();
    }
    constexpr bool operator==(BlockS2 const& other) const noexcept {
        return !(*this < other || other < *this);
    }
    constexpr bool less(BlockS2 const& other) const noexcept {
        return (get_block() & (~0xF000U)) < (other.get_block() & (~0xF000U));
    }
    constexpr bool equal(BlockS2 const& other) const noexcept {
        return (get_block() & (~0xF000U)) == (other.get_block() & (~0xF000U));
    }
    constexpr bool get_xflip() const noexcept {
        return (get_block() & 0x0400U) != 0;
    }
    constexpr bool get_yflip() const noexcept {
        return (get_block() & 0x0800U) != 0;
    }
    constexpr uint16_t get_collision1() const noexcept {
        return (get_block() >> 12U) & 3U;
    }
    constexpr uint16_t get_collision2() const noexcept {
        return (get_block() >> 14U) & 3U;
    }
    constexpr void set_xflip() noexcept {
        set_block(get_block() | 0x0400U);
    }
    constexpr void set_yflip() noexcept {
        set_block(get_block() | 0x0800U);
    }
    constexpr void clear_xflip() noexcept {
        set_block(get_block() & (~0x0400U));
    }
    constexpr void clear_yflip() noexcept {
        set_block(get_block() & (~0x0800U));
    }
    constexpr void set_collision1(uint16_t col) noexcept {
        uint16_t blk = get_block() & (~0x3000U);
        set_block(blk | ((col & 3U) << 12U));
    }
    constexpr void set_collision2(uint16_t col) noexcept {
        uint16_t blk = get_block() & (~0xC000U);
        set_block(blk | ((col & 3U) << 14U));
    }
    constexpr BlockS2& merge(BlockS1 const& high, BlockS1 const& low) noexcept {
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

template <unsigned Dim, typename Blk>
class Chunk {
private:
    constexpr static size_t const numBlocks = size_t(Dim) * size_t(Dim);

    std::array<Blk, numBlocks> blocks;

public:
    constexpr void read(istream& in) noexcept {
        for (auto& elem : blocks) {
            elem.read(in);
        }
    }
    constexpr void write(ostream& out) noexcept {
        for (auto& elem : blocks) {
            elem.write(out);
        }
    }
    constexpr bool operator<(Chunk const& other) const noexcept {
        for (size_t ii = 0; ii < numBlocks; ii++) {
            if (blocks[ii] < other.blocks[ii]) {
                return true;
            }
            if (other.blocks[ii] < blocks[ii]) {
                return false;
            }
        }
        return false;
    }
    constexpr bool operator==(Chunk const& other) const noexcept {
        return !(*this < other || other < *this);
    }
    constexpr bool less(Chunk const& other) const noexcept {
        for (size_t ii = 0; ii < numBlocks; ii++) {
            if (blocks[ii].less(other.blocks[ii])) {
                return true;
            }
            if (other.blocks[ii].less(blocks[ii])) {
                return false;
            }
        }
        return false;
    }
    constexpr bool equal(Chunk const& other) const noexcept {
        for (size_t ii = 0; ii < numBlocks; ii++) {
            if (!blocks[ii].equal(other.blocks[ii])) {
                return false;
            }
        }
        return true;
    }
    constexpr Blk const& get_block(int ii) const noexcept {
        return blocks[ii];
    }
    constexpr Blk& get_block(int ii) noexcept {
        return blocks[ii];
    }
    constexpr void set_block(int ii, Blk const& blk) noexcept {
        blocks[ii] = blk;
    }
};

struct ChunkMap {
    uint8_t tl, tr, bl, br;
};

using ChunkS1 = Chunk<16, BlockS1>;
using ChunkS2 = Chunk<8, BlockS2>;

void split_chunks(
        ChunkS1 const& highchunk, ChunkS1 const& lowchunk, ChunkS2& tlchunk,
        ChunkS2& trchunk, ChunkS2& blchunk, ChunkS2& brchunk) noexcept {
    for (int ii = 0; ii < 16; ii++) {
        for (int jj = 0; jj < 16; jj++) {
            ChunkS2&       curr    = ii < 8 ? (jj < 8 ? tlchunk : blchunk)
                                            : (jj < 8 ? trchunk : brchunk);
            BlockS1 const& highblk = highchunk.get_block(jj * 16 + ii);
            BlockS1 const& lowblk  = lowchunk.get_block(jj * 16 + ii);
            curr.get_block((jj % 8) * 8 + (ii % 8)).merge(highblk, lowblk);
        }
    }
}

namespace {
    // Some functions to generate lookup tables.
    template <typename Callable, int... ids>
    constexpr auto get_base_remaps(
            Callable base_remap, integer_sequence<int, ids...>) noexcept {
        return array<uint8_t, sizeof...(ids)>{base_remap(ids)...};
    }

    constexpr auto identity_remap(uint32_t cc) {
        return static_cast<uint8_t>(cc & 0x7FU);
    }

    constexpr auto increment_remap(uint32_t cc) {
        return static_cast<uint8_t>((cc + 1U) & 0x7FU);
    }

    constexpr auto init_remaps(int levelid) {
        if (levelid >= 4 && levelid <= 6) {
            return get_base_remaps(
                    identity_remap, make_integer_sequence<int, 256>{});
        }
        return get_base_remaps(
                increment_remap, make_integer_sequence<int, 256>{});
    }
}    // namespace

auto get_chunk_remaps(int levelid) noexcept {
    auto remaps{init_remaps(levelid)};
    if (levelid == 4) {
        // WWZ
        remaps[0x15] = 0x60;
        remaps[0x1E] = 0x61;
        remaps[0x1F] = 0x62;
        remaps[0x32] = 0x63;
    } else if (levelid == 5) {
        // SSZ
        remaps[0x04] = 0x05;
        remaps[0x06] = 0x07;
        remaps[0x16] = 0x17;
        remaps[0x28] = 0x29;
        remaps[0x2F] = 0x30;
        remaps[0x37] = 0x38;
        remaps[0x3C] = 0x3D;
    } else if (levelid == 6) {
        // MMZ
        remaps[0x10] = 0x6D;
        remaps[0x43] = 0x6F;
        remaps[0x46] = 0x6A;
        remaps[0x48] = 0x6B;
        remaps[0x4A] = 0x6C;
        remaps[0x63] = 0x6E;
    } else {
        remaps[0x28] = 0x51;
    }
    return remaps;
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        cerr << "Usage: " << argv[0]
             << " levelid layoutfgfile layoutbgfile chunkfile" << endl;
        cerr << "levelid     \t0 = PPZ, 1 = CCZ, 2 = TTZ, 3 = QQZ, 4 = WWZ, 5 "
                "= SSZ, 6 = MMZ"
             << endl;
        cerr << "layoutfgfile\tLevel's FG layout file, assumed to be in S1 "
                "format (uncompressed)"
             << endl;
        cerr << "layoutbgfile\tLevel's BG layout file, assumed to be in S1 "
                "format (uncompressed)"
             << endl;
        cerr << "chunkfile   \tLevel's 256x256 chunk file, assumed to be in "
                "SCD format (uncompressed)"
             << endl
             << endl;
        return 1;
    }

    int64_t levelid = strtol(argv[1], nullptr, 10);
    if (levelid < 0 || levelid > 6) {
        cerr << "Input level ID " << argv[1]
             << " must be a number: 0 = PPZ, 1 = CCZ, 2 = TTZ, 3 = QQZ, 4 = "
                "WWZ, 5 = SSZ, 6 = MMZ."
             << endl;
        return 2;
    }

    ifstream flayoutFG(argv[2], ios::in | ios::binary);
    if (!flayoutFG.good()) {
        cerr << "Input layout file '" << argv[2] << "' could not be opened."
             << endl;
        return 3;
    }

    ifstream flayoutBG(argv[3], ios::in | ios::binary);
    if (!flayoutBG.good()) {
        cerr << "Input layout file '" << argv[3] << "' could not be opened."
             << endl;
        return 4;
    }

    ifstream fchunks(argv[4], ios::in | ios::binary);
    if (!fchunks.good()) {
        cerr << "Input chunks file '" << argv[4] << "' could not be opened."
             << endl;
        return 5;
    }

    cout << "=============================================================="
         << endl;
    cout << argv[4] << "\t" << levelid << endl;
    auto remaps = get_chunk_remaps(levelid);

    vector<ChunkS1> chunkss1;
    chunkss1.reserve(256);

    do {
        ChunkS1 chunk;
        chunk.read(fchunks);
        chunkss1.push_back(chunk);
    } while (fchunks.good());

    fchunks.close();

    size_t fgw = flayoutFG.get() + 1;
    size_t fgh = flayoutFG.get() + 1;
    size_t bgw = flayoutBG.get() + 1;
    size_t bgh = flayoutBG.get() + 1;
    cout << "Layout sizes: FG: (" << fgw << ", " << fgh << "), BG: (" << bgw
         << ", " << bgh << ")" << endl;
    set<uint8_t>    usedchunks;
    vector<uint8_t> needremap;
    needremap.resize(chunkss1.size());
    set<ChunkS1> uniquechunks;

    auto process_layout = [&](auto& flayout, auto count) {
        vector<uint8_t> vlayout(count, 0);
        for (size_t ii = 0; ii < count; ii++) {
            char cc;
            flayout.get(cc);
            auto uc     = static_cast<uint8_t>(cc);
            vlayout[ii] = uc;
            if ((uc & 0x80U) != 0) {
                needremap[uc & 0x7FU] = 1;
                // usedchunks.insert(remaps[uc & 0x7F]);
            }
            usedchunks.insert(uc & 0x7FU);
            if ((uc & 0x7FU) != 0) {
                uniquechunks.insert(chunkss1[(uc & 0x7FU) - 1]);
            }
        }
        flayout.close();
        return vlayout;
    };

    auto   layouts1FG = process_layout(flayoutFG, fgw * fgh);
    size_t usedfg     = usedchunks.size();
    auto   layouts1BG = process_layout(flayoutBG, bgw * bgh);

    cout << "Number of chunks: total: " << chunkss1.size() << ", FG: " << usedfg
         << ", BG: " << (usedchunks.size() - usedfg)
         << ", used: " << usedchunks.size()
         << ", unique: " << uniquechunks.size() << endl;

    // size_t s2w = 128, s2h = 20;
    // vector<uint8_t> layouts2(s2w * s2h, 0);
    vector<ChunkS2> chunkss2;
    chunkss2.reserve(256);
    map<ChunkS2, size_t>   chunkids;
    map<uint8_t, ChunkMap> s1s2chunkidmap;

    auto checked_insert = [&](auto& id, auto& dst) {
        auto it = chunkids.find(id);
        if (it != chunkids.end()) {
            dst = it->second;
        } else {
            dst = chunkids[id] = chunkss2.size();
            chunkss2.push_back(id);
        }
    };

    for (auto chunk : usedchunks) {
        if (chunk == 0U) {
            continue;
        }
        chunk--;
        ChunkS1 highchunk = chunkss1[chunk];
        ChunkS1 lowchunk
                = needremap[chunk] != 0U ? chunkss1[remaps[chunk]] : highchunk;
        ChunkS2 tlchunk;
        ChunkS2 trchunk;
        ChunkS2 blchunk;
        ChunkS2 brchunk;
        split_chunks(highchunk, lowchunk, tlchunk, trchunk, blchunk, brchunk);
        ChunkMap map{};
        checked_insert(tlchunk, map.tl);
        checked_insert(trchunk, map.tr);
        checked_insert(blchunk, map.bl);
        checked_insert(brchunk, map.br);
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

    return 0;
}
