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

#include <mdtools/sstrack.hh>

#include <iterator>
#include <sstream>
#include <string>

#include <mdcomp/bigendian_io.hh>
#include <mdcomp/bitstream.hh>

using namespace std;

SSTrackFrame::SSTrackFrame(istream& in, bool const xflip) noexcept
    : PlaneH128V28() {
    stringstream bitflags(ios::in | ios::out | ios::binary),
        sym_maps(ios::in | ios::out | ios::binary),
        dic_maps(ios::in | ios::out | ios::binary);
    // Fill in the bitflags stream.
    size_t pos = in.tellg();
    in.ignore(2);
    unsigned short const            len1 = BigEndian::Read2(in);
    ostream_iterator<unsigned char> bfdst(bitflags);
    for (unsigned short ii = 0; ii < len1; ii++) {
        *bfdst++ = Read1(in);
    }
    bitflags.seekg(0);

    // Fill in the symbolwise index stream.
    in.seekg(pos + len1 + 4);
    pos = in.tellg();
    in.ignore(2);
    unsigned short const            len2 = BigEndian::Read2(in);
    ostream_iterator<unsigned char> smdst(sym_maps);
    for (unsigned short ii = 0; ii < len2; ii++) {
        *smdst++ = Read1(in);
    }
    sym_maps.seekg(0);

    // Fill in the dictionary index stream.
    in.seekg(pos + len2 + 4);
    pos = in.tellg();
    in.ignore(2);
    unsigned short const            len3 = BigEndian::Read2(in);
    ostream_iterator<unsigned char> dmdst(dic_maps);
    for (unsigned short ii = 0; ii < len3; ii++) {
        *dmdst++ = Read1(in);
    }
    dic_maps.seekg(0);

    ibitstream<unsigned char, false>  ibitflags(bitflags);
    ibitstream<unsigned short, false> isym_maps(sym_maps);
    ibitstream<unsigned char, false>  idic_maps(dic_maps);

    auto  lineit  = table.begin();
    Line* curline = &(*lineit);
    // Only one of these get used depending on xflip.
    auto fwdit = curline->begin();
    auto revit = curline->rbegin();

    while (lineit != table.end()) {
        // Is the next entry symbolwise- or dictionary-encoded?
        if (ibitflags.pop()) {
            // Symbolwise.
            Pattern_Name pat;
            // 10-bit index or 6-bit index?
            if (isym_maps.pop()) {
                // 10-bit.
                pat = SymLUT_10bit[isym_maps.read(10)];
            } else {
                // 6-bit.
                pat = SymLUT_6bit[isym_maps.read(6)];
            }
            // Set correct palette.
            pat.set_palette(Line3);
            if (xflip) {
                // Flip the pattern name and put it from the end of line.
                pat.set_flip(pat.get_flip() ^ XFlip);
                *revit++ = pat;
            } else {
                // Just write the pattern name.
                *fwdit++ = pat;
            }
        } else {
            // Dictionary.
            // Do he have zero bits in the buffer?
            if (!idic_maps.have_waiting_bits()) {
                // Yes; check to see if the next byte is a 0xff.
                if (static_cast<unsigned char>(dic_maps.peek()) == 0xff) {
                    // It is. Discard byte, advance to next line and continue.
                    dic_maps.ignore(1);
                    ++lineit;
                    curline = &(*lineit);
                    fwdit   = curline->begin();
                    revit   = curline->rbegin();
                    continue;
                }
            }
            RLEPattern pat;
            // 7-bit index or 6-bit index?
            if (idic_maps.pop()) {
                // 7-bit.
                unsigned char const val = idic_maps.read(7);
                // Did we read a 0x7f?
                if (val == 0x7f) {
                    // Yes; this means a non-byte-aligned 0xff, or end-of-line.
                    // Advance to next line and continue.
                    ++lineit;
                    curline = &(*lineit);
                    fwdit   = curline->begin();
                    revit   = curline->rbegin();
                    continue;
                }
                pat = DicLUT_7bit[val];
            } else {
                pat = DicLUT_6bit[idic_maps.read(6)];
            }
            // Set correct palette and priority.
            pat.first.set_palette(Line3);
            pat.first.set_priority(true);
            // Write the pattern name as many times as needed.
            if (xflip) {
                for (unsigned ii = 0; ii <= pat.second; ii++) {
                    *revit++ = pat.first;
                }
            } else {
                for (unsigned ii = 0; ii <= pat.second; ii++) {
                    *fwdit++ = pat.first;
                }
            }
        }
    }
}

// Hard-coded stuff starts here.
// At some point, we will want to use the raw game data instead of hard-coding
// all of this.
#define make_block_tile(addr, flx, fly, pal, pri)                              \
    (((static_cast<unsigned>(pri)) & 1) << 15) |                               \
        (((static_cast<unsigned>(pal)) & 3) << 13) |                           \
        (((static_cast<unsigned>(fly)) & 1) << 12) |                           \
        (((static_cast<unsigned>(flx)) & 1) << 11) |                           \
        ((static_cast<unsigned>(addr)) & 0x7FF)

vector<Pattern_Name> SSTrackFrame::SymLUT_6bit{
    make_block_tile(0x0001, 0, 0, 0, 1), make_block_tile(0x0007, 0, 0, 0, 1),
    make_block_tile(0x002C, 0, 0, 0, 1), make_block_tile(0x000B, 0, 0, 0, 1),
    make_block_tile(0x0024, 0, 0, 0, 1), make_block_tile(0x0024, 1, 0, 0, 1),
    make_block_tile(0x0039, 0, 0, 0, 1), make_block_tile(0x002B, 1, 0, 0, 1),
    make_block_tile(0x005D, 0, 0, 0, 1), make_block_tile(0x005D, 1, 0, 0, 1),
    make_block_tile(0x002B, 0, 0, 0, 1), make_block_tile(0x004A, 0, 0, 0, 1),
    make_block_tile(0x0049, 0, 0, 0, 1), make_block_tile(0x0037, 0, 0, 0, 1),
    make_block_tile(0x0049, 1, 0, 0, 1), make_block_tile(0x0045, 0, 0, 0, 1),
    make_block_tile(0x0045, 1, 0, 0, 1), make_block_tile(0x003A, 1, 0, 0, 1),
    make_block_tile(0x0048, 0, 0, 0, 1), make_block_tile(0x0050, 1, 0, 0, 1),
    make_block_tile(0x0036, 0, 0, 0, 1), make_block_tile(0x0037, 1, 0, 0, 1),
    make_block_tile(0x003A, 0, 0, 0, 1), make_block_tile(0x0050, 0, 0, 0, 1),
    make_block_tile(0x0042, 1, 0, 0, 1), make_block_tile(0x0042, 0, 0, 0, 1),
    make_block_tile(0x0015, 1, 0, 0, 1), make_block_tile(0x001D, 0, 0, 0, 1),
    make_block_tile(0x004B, 0, 0, 0, 1), make_block_tile(0x0017, 1, 0, 0, 1),
    make_block_tile(0x0048, 1, 0, 0, 1), make_block_tile(0x0036, 1, 0, 0, 1),
    make_block_tile(0x0038, 0, 0, 0, 1), make_block_tile(0x004B, 1, 0, 0, 1),
    make_block_tile(0x0015, 0, 0, 0, 1), make_block_tile(0x0021, 0, 0, 0, 1),
    make_block_tile(0x0017, 0, 0, 0, 1), make_block_tile(0x0033, 0, 0, 0, 1),
    make_block_tile(0x001A, 0, 0, 0, 1), make_block_tile(0x002A, 0, 0, 0, 1),
    make_block_tile(0x005E, 0, 0, 0, 1), make_block_tile(0x0028, 0, 0, 0, 1),
    make_block_tile(0x0030, 0, 0, 0, 1), make_block_tile(0x0021, 1, 0, 0, 1),
    make_block_tile(0x0038, 1, 0, 0, 1), make_block_tile(0x001A, 1, 0, 0, 1),
    make_block_tile(0x0025, 0, 0, 0, 1), make_block_tile(0x005E, 1, 0, 0, 1),
    make_block_tile(0x0025, 1, 0, 0, 1), make_block_tile(0x0033, 1, 0, 0, 1),
    make_block_tile(0x0003, 0, 0, 0, 1), make_block_tile(0x0014, 1, 0, 0, 1),
    make_block_tile(0x0014, 0, 0, 0, 1), make_block_tile(0x0004, 0, 0, 0, 1),
    make_block_tile(0x004E, 0, 0, 0, 1), make_block_tile(0x0003, 1, 0, 0, 1),
    make_block_tile(0x000C, 0, 0, 0, 1), make_block_tile(0x002A, 1, 0, 0, 1),
    make_block_tile(0x0002, 0, 0, 0, 1), make_block_tile(0x0051, 0, 0, 0, 1),
    make_block_tile(0x0040, 0, 0, 0, 1), make_block_tile(0x003D, 0, 0, 0, 1),
    make_block_tile(0x0019, 0, 0, 0, 1), make_block_tile(0x0052, 0, 0, 0, 1)};

vector<Pattern_Name> SSTrackFrame::SymLUT_10bit{
    make_block_tile(0x0009, 0, 0, 0, 1), make_block_tile(0x005A, 0, 0, 0, 1),
    make_block_tile(0x0030, 1, 0, 0, 1), make_block_tile(0x004E, 1, 0, 0, 1),
    make_block_tile(0x0052, 1, 0, 0, 1), make_block_tile(0x0051, 1, 0, 0, 1),
    make_block_tile(0x0009, 1, 0, 0, 1), make_block_tile(0x0040, 1, 0, 0, 1),
    make_block_tile(0x002F, 0, 0, 0, 1), make_block_tile(0x005A, 1, 0, 0, 1),
    make_block_tile(0x0018, 1, 0, 0, 1), make_block_tile(0x0034, 0, 0, 0, 1),
    make_block_tile(0x0019, 1, 0, 0, 1), make_block_tile(0x002F, 1, 0, 0, 1),
    make_block_tile(0x003D, 1, 0, 0, 1), make_block_tile(0x003E, 0, 0, 0, 1),
    make_block_tile(0x0018, 0, 0, 0, 1), make_block_tile(0x000C, 1, 0, 0, 1),
    make_block_tile(0x0012, 0, 0, 0, 1), make_block_tile(0x0004, 1, 0, 0, 1),
    make_block_tile(0x0026, 0, 0, 0, 1), make_block_tile(0x0034, 1, 0, 0, 1),
    make_block_tile(0x0005, 1, 0, 0, 1), make_block_tile(0x003B, 0, 0, 0, 1),
    make_block_tile(0x003E, 1, 0, 0, 1), make_block_tile(0x003B, 1, 0, 0, 1),
    make_block_tile(0x0000, 0, 0, 0, 1), make_block_tile(0x0002, 1, 0, 0, 1),
    make_block_tile(0x0005, 0, 0, 0, 1), make_block_tile(0x000D, 0, 0, 0, 1),
    make_block_tile(0x0055, 0, 0, 0, 1), make_block_tile(0x00AF, 0, 0, 0, 1),
    make_block_tile(0x001C, 0, 0, 0, 1), make_block_tile(0x001B, 0, 0, 0, 1),
    make_block_tile(0x000D, 1, 0, 0, 1), make_block_tile(0x0016, 0, 0, 0, 1),
    make_block_tile(0x0012, 1, 0, 0, 1), make_block_tile(0x001F, 0, 0, 0, 1),
    make_block_tile(0x0032, 1, 0, 0, 1), make_block_tile(0x0013, 0, 0, 0, 1),
    make_block_tile(0x0092, 0, 0, 0, 1), make_block_tile(0x0026, 1, 0, 0, 1),
    make_block_tile(0x0010, 0, 0, 0, 1), make_block_tile(0x004D, 0, 0, 0, 1),
    make_block_tile(0x0047, 0, 0, 0, 1), make_block_tile(0x0092, 1, 0, 0, 1),
    make_block_tile(0x0000, 1, 0, 0, 1), make_block_tile(0x0062, 0, 0, 0, 1),
    make_block_tile(0x0066, 0, 0, 0, 1), make_block_tile(0x0090, 0, 0, 0, 1),
    make_block_tile(0x0008, 0, 0, 0, 1), make_block_tile(0x007C, 1, 0, 0, 1),
    make_block_tile(0x0067, 1, 0, 0, 1), make_block_tile(0x00F7, 1, 0, 0, 1),
    make_block_tile(0x000E, 0, 0, 0, 1), make_block_tile(0x0060, 0, 0, 0, 1),
    make_block_tile(0x0032, 0, 0, 0, 1), make_block_tile(0x0094, 0, 0, 0, 1),
    make_block_tile(0x001C, 1, 0, 0, 1), make_block_tile(0x0105, 1, 0, 0, 1),
    make_block_tile(0x00B0, 1, 0, 0, 1), make_block_tile(0x0059, 0, 0, 0, 1),
    make_block_tile(0x000F, 0, 0, 0, 1), make_block_tile(0x0067, 0, 0, 0, 1),
    make_block_tile(0x0068, 0, 0, 0, 1), make_block_tile(0x0094, 1, 0, 0, 1),
    make_block_tile(0x007C, 0, 0, 0, 1), make_block_tile(0x00B0, 0, 0, 0, 1),
    make_block_tile(0x00B1, 0, 0, 0, 1), make_block_tile(0x0006, 0, 0, 0, 1),
    make_block_tile(0x0041, 1, 0, 0, 1), make_block_tile(0x0087, 0, 0, 0, 1),
    make_block_tile(0x0093, 0, 0, 0, 1), make_block_tile(0x00CC, 0, 0, 0, 1),
    make_block_tile(0x001F, 1, 0, 0, 1), make_block_tile(0x0068, 1, 0, 0, 1),
    make_block_tile(0x0041, 0, 0, 0, 1), make_block_tile(0x008F, 0, 0, 0, 1),
    make_block_tile(0x0090, 1, 0, 0, 1), make_block_tile(0x00C2, 0, 0, 0, 1),
    make_block_tile(0x0013, 1, 0, 0, 1), make_block_tile(0x00C2, 1, 0, 0, 1),
    make_block_tile(0x005C, 0, 0, 0, 1), make_block_tile(0x0064, 0, 0, 0, 1),
    make_block_tile(0x00D8, 0, 0, 0, 1), make_block_tile(0x001B, 1, 0, 0, 1),
    make_block_tile(0x00CC, 1, 0, 0, 1), make_block_tile(0x0011, 1, 0, 0, 1),
    make_block_tile(0x0055, 1, 0, 0, 1), make_block_tile(0x00E2, 1, 0, 0, 1),
    make_block_tile(0x00F3, 1, 0, 0, 1), make_block_tile(0x0044, 0, 0, 0, 1),
    make_block_tile(0x00D8, 1, 0, 0, 1), make_block_tile(0x0085, 0, 0, 0, 1),
    make_block_tile(0x00A1, 0, 0, 0, 1), make_block_tile(0x00C1, 0, 0, 0, 1),
    make_block_tile(0x0119, 0, 0, 0, 1), make_block_tile(0x0089, 1, 0, 0, 1),
    make_block_tile(0x000A, 1, 0, 0, 1), make_block_tile(0x0022, 1, 0, 0, 1),
    make_block_tile(0x003F, 0, 0, 0, 1), make_block_tile(0x005B, 0, 0, 0, 1),
    make_block_tile(0x007F, 0, 0, 0, 1), make_block_tile(0x0086, 1, 0, 0, 1),
    make_block_tile(0x0008, 1, 0, 0, 1), make_block_tile(0x0080, 0, 0, 0, 1),
    make_block_tile(0x0066, 1, 0, 0, 1), make_block_tile(0x00E0, 1, 0, 0, 1),
    make_block_tile(0x00C1, 1, 0, 0, 1), make_block_tile(0x0020, 0, 0, 0, 1),
    make_block_tile(0x0022, 0, 0, 0, 1), make_block_tile(0x0054, 0, 0, 0, 1),
    make_block_tile(0x00D2, 0, 0, 0, 1), make_block_tile(0x0059, 1, 0, 0, 1),
    make_block_tile(0x00B1, 1, 0, 0, 1), make_block_tile(0x0060, 1, 0, 0, 1),
    make_block_tile(0x0119, 1, 0, 0, 1), make_block_tile(0x00A4, 1, 0, 0, 1),
    make_block_tile(0x008F, 1, 0, 0, 1), make_block_tile(0x000A, 0, 0, 0, 1),
    make_block_tile(0x0061, 0, 0, 0, 1), make_block_tile(0x0075, 0, 0, 0, 1),
    make_block_tile(0x0095, 0, 0, 0, 1), make_block_tile(0x00B6, 0, 0, 0, 1),
    make_block_tile(0x00E0, 0, 0, 0, 1), make_block_tile(0x0010, 1, 0, 0, 1),
    make_block_tile(0x0098, 1, 0, 0, 1), make_block_tile(0x005B, 1, 0, 0, 1),
    make_block_tile(0x00D2, 1, 0, 0, 1), make_block_tile(0x0016, 1, 0, 0, 1),
    make_block_tile(0x0053, 0, 0, 0, 1), make_block_tile(0x0091, 0, 0, 0, 1),
    make_block_tile(0x0096, 0, 0, 0, 1), make_block_tile(0x00A4, 0, 0, 0, 1),
    make_block_tile(0x00DD, 0, 0, 0, 1), make_block_tile(0x00E6, 0, 0, 0, 1),
    make_block_tile(0x007A, 1, 0, 0, 1), make_block_tile(0x004D, 1, 0, 0, 1),
    make_block_tile(0x00E6, 1, 0, 0, 1), make_block_tile(0x0011, 0, 0, 0, 1),
    make_block_tile(0x0057, 0, 0, 0, 1), make_block_tile(0x007A, 0, 0, 0, 1),
    make_block_tile(0x0086, 0, 0, 0, 1), make_block_tile(0x009E, 0, 0, 0, 1),
    make_block_tile(0x00DA, 0, 0, 0, 1), make_block_tile(0x0058, 0, 0, 0, 1),
    make_block_tile(0x00DC, 0, 0, 0, 1), make_block_tile(0x00E3, 0, 0, 0, 1),
    make_block_tile(0x0063, 1, 0, 0, 1), make_block_tile(0x003C, 0, 0, 0, 1),
    make_block_tile(0x0056, 0, 0, 0, 1), make_block_tile(0x0069, 0, 0, 0, 1),
    make_block_tile(0x007E, 0, 0, 0, 1), make_block_tile(0x00AE, 0, 0, 0, 1),
    make_block_tile(0x00B5, 0, 0, 0, 1), make_block_tile(0x00B8, 0, 0, 0, 1),
    make_block_tile(0x00CD, 0, 0, 0, 1), make_block_tile(0x00FB, 0, 0, 0, 1),
    make_block_tile(0x00FF, 0, 0, 0, 1), make_block_tile(0x005C, 1, 0, 0, 1),
    make_block_tile(0x00CD, 1, 0, 0, 1), make_block_tile(0x0074, 1, 0, 0, 1),
    make_block_tile(0x00EA, 1, 0, 0, 1), make_block_tile(0x00FF, 1, 0, 0, 1),
    make_block_tile(0x00B5, 1, 0, 0, 1), make_block_tile(0x0043, 0, 0, 0, 1),
    make_block_tile(0x006C, 0, 0, 0, 1), make_block_tile(0x0074, 0, 0, 0, 1),
    make_block_tile(0x0077, 0, 0, 0, 1), make_block_tile(0x0089, 0, 0, 0, 1),
    make_block_tile(0x0097, 0, 0, 0, 1), make_block_tile(0x009F, 0, 0, 0, 1),
    make_block_tile(0x00A0, 0, 0, 0, 1), make_block_tile(0x0113, 0, 0, 0, 1),
    make_block_tile(0x011B, 0, 0, 0, 1), make_block_tile(0x0078, 1, 0, 0, 1),
    make_block_tile(0x000F, 1, 0, 0, 1), make_block_tile(0x00E1, 1, 0, 0, 1),
    make_block_tile(0x00FB, 1, 0, 0, 1), make_block_tile(0x0128, 1, 0, 0, 1),
    make_block_tile(0x0063, 0, 0, 0, 1), make_block_tile(0x0084, 0, 0, 0, 1),
    make_block_tile(0x008D, 0, 0, 0, 1), make_block_tile(0x00CB, 0, 0, 0, 1),
    make_block_tile(0x00D7, 0, 0, 0, 1), make_block_tile(0x00E9, 0, 0, 0, 1),
    make_block_tile(0x0128, 0, 0, 0, 1), make_block_tile(0x0138, 0, 0, 0, 1),
    make_block_tile(0x00AE, 1, 0, 0, 1), make_block_tile(0x00EC, 1, 0, 0, 1),
    make_block_tile(0x0031, 0, 0, 0, 1), make_block_tile(0x004C, 0, 0, 0, 1),
    make_block_tile(0x00E2, 0, 0, 0, 1), make_block_tile(0x00EA, 0, 0, 0, 1),
    make_block_tile(0x0064, 1, 0, 0, 1), make_block_tile(0x0029, 0, 0, 0, 1),
    make_block_tile(0x002D, 0, 0, 0, 1), make_block_tile(0x006D, 0, 0, 0, 1),
    make_block_tile(0x0078, 0, 0, 0, 1), make_block_tile(0x0088, 0, 0, 0, 1),
    make_block_tile(0x00B4, 0, 0, 0, 1), make_block_tile(0x00BE, 0, 0, 0, 1),
    make_block_tile(0x00CF, 0, 0, 0, 1), make_block_tile(0x00E1, 0, 0, 0, 1),
    make_block_tile(0x00E4, 0, 0, 0, 1), make_block_tile(0x0054, 1, 0, 0, 1),
    make_block_tile(0x00D6, 1, 0, 0, 1), make_block_tile(0x00D7, 1, 0, 0, 1),
    make_block_tile(0x0061, 1, 0, 0, 1), make_block_tile(0x012B, 1, 0, 0, 1),
    make_block_tile(0x0047, 1, 0, 0, 1), make_block_tile(0x0035, 0, 0, 0, 1),
    make_block_tile(0x006A, 0, 0, 0, 1), make_block_tile(0x0072, 0, 0, 0, 1),
    make_block_tile(0x0073, 0, 0, 0, 1), make_block_tile(0x0098, 0, 0, 0, 1),
    make_block_tile(0x00D5, 0, 0, 0, 1), make_block_tile(0x00D6, 0, 0, 0, 1),
    make_block_tile(0x0116, 0, 0, 0, 1), make_block_tile(0x011E, 0, 0, 0, 1),
    make_block_tile(0x0126, 0, 0, 0, 1), make_block_tile(0x0127, 0, 0, 0, 1),
    make_block_tile(0x012F, 0, 0, 0, 1), make_block_tile(0x015D, 0, 0, 0, 1),
    make_block_tile(0x0069, 1, 0, 0, 1), make_block_tile(0x0088, 1, 0, 0, 1),
    make_block_tile(0x0075, 1, 0, 0, 1), make_block_tile(0x0097, 1, 0, 0, 1),
    make_block_tile(0x00B4, 1, 0, 0, 1), make_block_tile(0x00D1, 1, 0, 0, 1),
    make_block_tile(0x00D4, 1, 0, 0, 1), make_block_tile(0x00D5, 1, 0, 0, 1),
    make_block_tile(0x00CB, 1, 0, 0, 1), make_block_tile(0x00E4, 1, 0, 0, 1),
    make_block_tile(0x0091, 1, 0, 0, 1), make_block_tile(0x0062, 1, 0, 0, 1),
    make_block_tile(0x0006, 1, 0, 0, 1), make_block_tile(0x00B8, 1, 0, 0, 1),
    make_block_tile(0x0065, 0, 0, 0, 1), make_block_tile(0x006E, 0, 0, 0, 1),
    make_block_tile(0x0071, 0, 0, 0, 1), make_block_tile(0x007D, 0, 0, 0, 1),
    make_block_tile(0x00D1, 0, 0, 0, 1), make_block_tile(0x00E7, 0, 0, 0, 1),
    make_block_tile(0x00F9, 0, 0, 0, 1), make_block_tile(0x0108, 0, 0, 0, 1),
    make_block_tile(0x012E, 0, 0, 0, 1), make_block_tile(0x014B, 0, 0, 0, 1),
    make_block_tile(0x0081, 1, 0, 0, 1), make_block_tile(0x0085, 1, 0, 0, 1),
    make_block_tile(0x0077, 1, 0, 0, 1), make_block_tile(0x007E, 1, 0, 0, 1),
    make_block_tile(0x0095, 1, 0, 0, 1), make_block_tile(0x00DF, 1, 0, 0, 1),
    make_block_tile(0x0087, 1, 0, 0, 1), make_block_tile(0x006C, 1, 0, 0, 1),
    make_block_tile(0x00F5, 1, 0, 0, 1), make_block_tile(0x0108, 1, 0, 0, 1),
    make_block_tile(0x0079, 1, 0, 0, 1), make_block_tile(0x006D, 1, 0, 0, 1),
    make_block_tile(0x012A, 1, 0, 0, 1), make_block_tile(0x00AA, 1, 0, 0, 1),
    make_block_tile(0x001E, 0, 0, 0, 1), make_block_tile(0x0027, 0, 0, 0, 1),
    make_block_tile(0x0046, 0, 0, 0, 1), make_block_tile(0x005F, 0, 0, 0, 1),
    make_block_tile(0x0070, 0, 0, 0, 1), make_block_tile(0x0079, 0, 0, 0, 1),
    make_block_tile(0x009A, 0, 0, 0, 1), make_block_tile(0x00AA, 0, 0, 0, 1),
    make_block_tile(0x00C3, 0, 0, 0, 1), make_block_tile(0x00D3, 0, 0, 0, 1),
    make_block_tile(0x00D4, 0, 0, 0, 1), make_block_tile(0x00DE, 0, 0, 0, 1),
    make_block_tile(0x00DF, 0, 0, 0, 1), make_block_tile(0x00F8, 0, 0, 0, 1),
    make_block_tile(0x0100, 0, 0, 0, 1), make_block_tile(0x0101, 0, 0, 0, 1),
    make_block_tile(0x012B, 0, 0, 0, 1), make_block_tile(0x0133, 0, 0, 0, 1),
    make_block_tile(0x0136, 0, 0, 0, 1), make_block_tile(0x0143, 0, 0, 0, 1),
    make_block_tile(0x0151, 0, 0, 0, 1), make_block_tile(0x002E, 1, 0, 0, 1),
    make_block_tile(0x009E, 1, 0, 0, 1), make_block_tile(0x0099, 1, 0, 0, 1),
    make_block_tile(0x00D3, 1, 0, 0, 1), make_block_tile(0x00DD, 1, 0, 0, 1),
    make_block_tile(0x00DE, 1, 0, 0, 1), make_block_tile(0x00E9, 1, 0, 0, 1),
    make_block_tile(0x00EF, 1, 0, 0, 1), make_block_tile(0x00F0, 1, 0, 0, 1),
    make_block_tile(0x00F8, 1, 0, 0, 1), make_block_tile(0x0127, 1, 0, 0, 1),
    make_block_tile(0x00BE, 1, 0, 0, 1), make_block_tile(0x0096, 1, 0, 0, 1),
    make_block_tile(0x004F, 0, 0, 0, 1), make_block_tile(0x006F, 0, 0, 0, 1),
    make_block_tile(0x0081, 0, 0, 0, 1), make_block_tile(0x008B, 0, 0, 0, 1),
    make_block_tile(0x008E, 0, 0, 0, 1), make_block_tile(0x009C, 0, 0, 0, 1),
    make_block_tile(0x00A3, 0, 0, 0, 1), make_block_tile(0x00B3, 0, 0, 0, 1),
    make_block_tile(0x00C0, 0, 0, 0, 1), make_block_tile(0x00CE, 0, 0, 0, 1),
    make_block_tile(0x00F0, 0, 0, 0, 1), make_block_tile(0x00F1, 0, 0, 0, 1),
    make_block_tile(0x00F5, 0, 0, 0, 1), make_block_tile(0x00F7, 0, 0, 0, 1),
    make_block_tile(0x0102, 0, 0, 0, 1), make_block_tile(0x0104, 0, 0, 0, 1),
    make_block_tile(0x0105, 0, 0, 0, 1), make_block_tile(0x0109, 0, 0, 0, 1),
    make_block_tile(0x010C, 0, 0, 0, 1), make_block_tile(0x0114, 0, 0, 0, 1),
    make_block_tile(0x0118, 0, 0, 0, 1), make_block_tile(0x0120, 0, 0, 0, 1),
    make_block_tile(0x0124, 0, 0, 0, 1), make_block_tile(0x0125, 0, 0, 0, 1),
    make_block_tile(0x012A, 0, 0, 0, 1), make_block_tile(0x0130, 0, 0, 0, 1),
    make_block_tile(0x0132, 0, 0, 0, 1), make_block_tile(0x0137, 0, 0, 0, 1),
    make_block_tile(0x0159, 0, 0, 0, 1), make_block_tile(0x0165, 0, 0, 0, 1),
    make_block_tile(0x003F, 1, 0, 0, 1), make_block_tile(0x006B, 1, 0, 0, 1),
    make_block_tile(0x0080, 1, 0, 0, 1), make_block_tile(0x0053, 1, 0, 0, 1),
    make_block_tile(0x00C6, 1, 0, 0, 1), make_block_tile(0x00CF, 1, 0, 0, 1),
    make_block_tile(0x00D9, 1, 0, 0, 1), make_block_tile(0x00DC, 1, 0, 0, 1),
    make_block_tile(0x0056, 1, 0, 0, 1), make_block_tile(0x00B6, 1, 0, 0, 1),
    make_block_tile(0x00F9, 1, 0, 0, 1), make_block_tile(0x0102, 1, 0, 0, 1),
    make_block_tile(0x0104, 1, 0, 0, 1), make_block_tile(0x0115, 1, 0, 0, 1),
    make_block_tile(0x006A, 1, 0, 0, 1), make_block_tile(0x0113, 1, 0, 0, 1),
    make_block_tile(0x0072, 1, 0, 0, 1), make_block_tile(0x0035, 1, 0, 0, 1),
    make_block_tile(0x0138, 1, 0, 0, 1), make_block_tile(0x015D, 1, 0, 0, 1),
    make_block_tile(0x0143, 1, 0, 0, 1), make_block_tile(0x0023, 0, 0, 0, 1),
    make_block_tile(0x0076, 0, 0, 0, 1), make_block_tile(0x007B, 0, 0, 0, 1),
    make_block_tile(0x008A, 0, 0, 0, 1), make_block_tile(0x009D, 0, 0, 0, 1),
    make_block_tile(0x00A6, 0, 0, 0, 1), make_block_tile(0x00A8, 0, 0, 0, 1),
    make_block_tile(0x00AC, 0, 0, 0, 1), make_block_tile(0x00B2, 0, 0, 0, 1),
    make_block_tile(0x00B7, 0, 0, 0, 1), make_block_tile(0x00BB, 0, 0, 0, 1),
    make_block_tile(0x00BC, 0, 0, 0, 1), make_block_tile(0x00BD, 0, 0, 0, 1),
    make_block_tile(0x00C6, 0, 0, 0, 1), make_block_tile(0x00E5, 0, 0, 0, 1),
    make_block_tile(0x00E8, 0, 0, 0, 1), make_block_tile(0x00EE, 0, 0, 0, 1),
    make_block_tile(0x00F4, 0, 0, 0, 1), make_block_tile(0x010A, 0, 0, 0, 1),
    make_block_tile(0x010D, 0, 0, 0, 1), make_block_tile(0x0111, 0, 0, 0, 1),
    make_block_tile(0x0115, 0, 0, 0, 1), make_block_tile(0x011A, 0, 0, 0, 1),
    make_block_tile(0x011F, 0, 0, 0, 1), make_block_tile(0x0122, 0, 0, 0, 1),
    make_block_tile(0x0123, 0, 0, 0, 1), make_block_tile(0x0139, 0, 0, 0, 1),
    make_block_tile(0x013A, 0, 0, 0, 1), make_block_tile(0x013C, 0, 0, 0, 1),
    make_block_tile(0x0142, 0, 0, 0, 1), make_block_tile(0x0144, 0, 0, 0, 1),
    make_block_tile(0x0147, 0, 0, 0, 1), make_block_tile(0x0148, 0, 0, 0, 1),
    make_block_tile(0x015E, 0, 0, 0, 1), make_block_tile(0x015F, 0, 0, 0, 1),
    make_block_tile(0x0163, 0, 0, 0, 1), make_block_tile(0x0168, 0, 0, 0, 1),
    make_block_tile(0x016A, 0, 0, 0, 1), make_block_tile(0x016C, 0, 0, 0, 1),
    make_block_tile(0x0170, 0, 0, 0, 1), make_block_tile(0x00E5, 1, 0, 0, 1),
    make_block_tile(0x00CE, 1, 0, 0, 1), make_block_tile(0x00EE, 1, 0, 0, 1),
    make_block_tile(0x00F1, 1, 0, 0, 1), make_block_tile(0x0084, 1, 0, 0, 1),
    make_block_tile(0x00FD, 1, 0, 0, 1), make_block_tile(0x0100, 1, 0, 0, 1),
    make_block_tile(0x00B9, 1, 0, 0, 1), make_block_tile(0x0117, 1, 0, 0, 1),
    make_block_tile(0x0071, 1, 0, 0, 1), make_block_tile(0x0109, 1, 0, 0, 1),
    make_block_tile(0x010D, 1, 0, 0, 1), make_block_tile(0x0065, 1, 0, 0, 1),
    make_block_tile(0x0125, 1, 0, 0, 1), make_block_tile(0x0122, 1, 0, 0, 1),
    make_block_tile(0x0031, 1, 0, 0, 1), make_block_tile(0x003C, 1, 0, 0, 1),
    make_block_tile(0x010F, 1, 0, 0, 1), make_block_tile(0x00C5, 1, 0, 0, 1),
    make_block_tile(0x0133, 1, 0, 0, 1), make_block_tile(0x0137, 1, 0, 0, 1),
    make_block_tile(0x011F, 1, 0, 0, 1), make_block_tile(0x002E, 0, 0, 0, 1),
    make_block_tile(0x006B, 0, 0, 0, 1), make_block_tile(0x0082, 0, 0, 0, 1),
    make_block_tile(0x0083, 0, 0, 0, 1), make_block_tile(0x008C, 0, 0, 0, 1),
    make_block_tile(0x0099, 0, 0, 0, 1), make_block_tile(0x009B, 0, 0, 0, 1),
    make_block_tile(0x00A2, 0, 0, 0, 1), make_block_tile(0x00A5, 0, 0, 0, 1),
    make_block_tile(0x00A7, 0, 0, 0, 1), make_block_tile(0x00A9, 0, 0, 0, 1),
    make_block_tile(0x00AB, 0, 0, 0, 1), make_block_tile(0x00AD, 0, 0, 0, 1),
    make_block_tile(0x00B9, 0, 0, 0, 1), make_block_tile(0x00BA, 0, 0, 0, 1),
    make_block_tile(0x00BF, 0, 0, 0, 1), make_block_tile(0x00C4, 0, 0, 0, 1),
    make_block_tile(0x00C5, 0, 0, 0, 1), make_block_tile(0x00C7, 0, 0, 0, 1),
    make_block_tile(0x00C8, 0, 0, 0, 1), make_block_tile(0x00C9, 0, 0, 0, 1),
    make_block_tile(0x00CA, 0, 0, 0, 1), make_block_tile(0x00D0, 0, 0, 0, 1),
    make_block_tile(0x00D9, 0, 0, 0, 1), make_block_tile(0x00DB, 0, 0, 0, 1),
    make_block_tile(0x00EB, 0, 0, 0, 1), make_block_tile(0x00EC, 0, 0, 0, 1),
    make_block_tile(0x00ED, 0, 0, 0, 1), make_block_tile(0x00EF, 0, 0, 0, 1),
    make_block_tile(0x00F2, 0, 0, 0, 1), make_block_tile(0x00F3, 0, 0, 0, 1),
    make_block_tile(0x00F6, 0, 0, 0, 1), make_block_tile(0x00FA, 0, 0, 0, 1),
    make_block_tile(0x00FC, 0, 0, 0, 1), make_block_tile(0x00FD, 0, 0, 0, 1),
    make_block_tile(0x00FE, 0, 0, 0, 1), make_block_tile(0x0103, 0, 0, 0, 1),
    make_block_tile(0x0106, 0, 0, 0, 1), make_block_tile(0x0107, 0, 0, 0, 1),
    make_block_tile(0x010B, 0, 0, 0, 1), make_block_tile(0x010E, 0, 0, 0, 1),
    make_block_tile(0x010F, 0, 0, 0, 1), make_block_tile(0x0110, 0, 0, 0, 1),
    make_block_tile(0x0112, 0, 0, 0, 1), make_block_tile(0x0117, 0, 0, 0, 1),
    make_block_tile(0x011C, 0, 0, 0, 1), make_block_tile(0x011D, 0, 0, 0, 1),
    make_block_tile(0x0121, 0, 0, 0, 1), make_block_tile(0x0129, 0, 0, 0, 1),
    make_block_tile(0x012C, 0, 0, 0, 1), make_block_tile(0x012D, 0, 0, 0, 1),
    make_block_tile(0x0131, 0, 0, 0, 1), make_block_tile(0x0134, 0, 0, 0, 1),
    make_block_tile(0x0135, 0, 0, 0, 1), make_block_tile(0x013B, 0, 0, 0, 1),
    make_block_tile(0x013D, 0, 0, 0, 1), make_block_tile(0x013E, 0, 0, 0, 1),
    make_block_tile(0x013F, 0, 0, 0, 1), make_block_tile(0x0140, 0, 0, 0, 1),
    make_block_tile(0x0141, 0, 0, 0, 1), make_block_tile(0x0145, 0, 0, 0, 1),
    make_block_tile(0x0146, 0, 0, 0, 1), make_block_tile(0x0149, 0, 0, 0, 1),
    make_block_tile(0x014A, 0, 0, 0, 1), make_block_tile(0x014C, 0, 0, 0, 1),
    make_block_tile(0x014D, 0, 0, 0, 1), make_block_tile(0x014E, 0, 0, 0, 1),
    make_block_tile(0x014F, 0, 0, 0, 1), make_block_tile(0x0150, 0, 0, 0, 1),
    make_block_tile(0x0152, 0, 0, 0, 1), make_block_tile(0x0153, 0, 0, 0, 1),
    make_block_tile(0x0154, 0, 0, 0, 1), make_block_tile(0x0155, 0, 0, 0, 1),
    make_block_tile(0x0156, 0, 0, 0, 1), make_block_tile(0x0157, 0, 0, 0, 1),
    make_block_tile(0x0158, 0, 0, 0, 1), make_block_tile(0x015A, 0, 0, 0, 1),
    make_block_tile(0x015B, 0, 0, 0, 1), make_block_tile(0x015C, 0, 0, 0, 1),
    make_block_tile(0x0160, 0, 0, 0, 1), make_block_tile(0x0161, 0, 0, 0, 1),
    make_block_tile(0x0162, 0, 0, 0, 1), make_block_tile(0x0164, 0, 0, 0, 1),
    make_block_tile(0x0166, 0, 0, 0, 1), make_block_tile(0x0167, 0, 0, 0, 1),
    make_block_tile(0x0169, 0, 0, 0, 1), make_block_tile(0x016B, 0, 0, 0, 1),
    make_block_tile(0x016D, 0, 0, 0, 1), make_block_tile(0x016E, 0, 0, 0, 1),
    make_block_tile(0x016F, 0, 0, 0, 1), make_block_tile(0x0171, 0, 0, 0, 1),
    make_block_tile(0x0172, 0, 0, 0, 1), make_block_tile(0x0173, 0, 0, 0, 1),
    make_block_tile(0x006E, 1, 0, 0, 1), make_block_tile(0x007D, 1, 0, 0, 1),
    make_block_tile(0x00C3, 1, 0, 0, 1), make_block_tile(0x00DB, 1, 0, 0, 1),
    make_block_tile(0x00E7, 1, 0, 0, 1), make_block_tile(0x00E8, 1, 0, 0, 1),
    make_block_tile(0x00EB, 1, 0, 0, 1), make_block_tile(0x00ED, 1, 0, 0, 1),
    make_block_tile(0x00F2, 1, 0, 0, 1), make_block_tile(0x00F6, 1, 0, 0, 1),
    make_block_tile(0x00FA, 1, 0, 0, 1), make_block_tile(0x00FC, 1, 0, 0, 1),
    make_block_tile(0x00FE, 1, 0, 0, 1), make_block_tile(0x002D, 1, 0, 0, 1),
    make_block_tile(0x0103, 1, 0, 0, 1), make_block_tile(0x0106, 1, 0, 0, 1),
    make_block_tile(0x0107, 1, 0, 0, 1), make_block_tile(0x010B, 1, 0, 0, 1),
    make_block_tile(0x0073, 1, 0, 0, 1), make_block_tile(0x009A, 1, 0, 0, 1),
    make_block_tile(0x0129, 1, 0, 0, 1), make_block_tile(0x012C, 1, 0, 0, 1),
    make_block_tile(0x012D, 1, 0, 0, 1), make_block_tile(0x0111, 1, 0, 0, 1),
    make_block_tile(0x013C, 1, 0, 0, 1), make_block_tile(0x0120, 1, 0, 0, 1),
    make_block_tile(0x0146, 1, 0, 0, 1), make_block_tile(0x00A9, 1, 0, 0, 1),
    make_block_tile(0x009C, 1, 0, 0, 1), make_block_tile(0x0116, 1, 0, 0, 1),
    make_block_tile(0x014F, 1, 0, 0, 1), make_block_tile(0x014C, 1, 0, 0, 1),
    make_block_tile(0x006F, 1, 0, 0, 1), make_block_tile(0x0158, 1, 0, 0, 1),
    make_block_tile(0x0156, 1, 0, 0, 1), make_block_tile(0x0159, 1, 0, 0, 1),
    make_block_tile(0x015A, 1, 0, 0, 1), make_block_tile(0x0161, 1, 0, 0, 1),
    make_block_tile(0x007B, 1, 0, 0, 1), make_block_tile(0x0166, 1, 0, 0, 1),
    make_block_tile(0x011C, 1, 0, 0, 1), make_block_tile(0x0118, 1, 0, 0, 1),
    make_block_tile(0x00A0, 1, 0, 0, 1), make_block_tile(0x00A3, 1, 0, 0, 1),
    make_block_tile(0x0167, 1, 0, 0, 1), make_block_tile(0x00A1, 1, 0, 0, 1)};

vector<RLEPattern> SSTrackFrame::DicLUT_6bit{
    RLEPattern(make_block_tile(0x0007, 0, 0, 0, 0), 0x0001),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0001),
    RLEPattern(make_block_tile(0x004A, 0, 0, 0, 0), 0x0001),
    RLEPattern(make_block_tile(0x0039, 0, 0, 0, 0), 0x0003),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0005),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x0007),
    RLEPattern(make_block_tile(0x002C, 0, 0, 0, 0), 0x0001),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0002),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x0005),
    RLEPattern(make_block_tile(0x0039, 0, 0, 0, 0), 0x0001),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x0009),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0004),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x0006),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x0003),
    RLEPattern(make_block_tile(0x004A, 0, 0, 0, 0), 0x0002),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0003),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x0004),
    RLEPattern(make_block_tile(0x0039, 0, 0, 0, 0), 0x0002),
    RLEPattern(make_block_tile(0x0039, 0, 0, 0, 0), 0x0004),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0006),
    RLEPattern(make_block_tile(0x0007, 0, 0, 0, 0), 0x0002),
    RLEPattern(make_block_tile(0x002C, 0, 0, 0, 0), 0x0002),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x0001),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0001),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x0008),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x0002),
    RLEPattern(make_block_tile(0x0007, 0, 0, 0, 0), 0x0003),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0007),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x000B),
    RLEPattern(make_block_tile(0x0039, 0, 0, 0, 0), 0x0005),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0003),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0004),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0002),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0005),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x000D),
    RLEPattern(make_block_tile(0x000B, 0, 0, 0, 0), 0x0001),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x000A),
    RLEPattern(make_block_tile(0x0039, 0, 0, 0, 0), 0x0006),
    RLEPattern(make_block_tile(0x0039, 0, 0, 0, 0), 0x0007),
    RLEPattern(make_block_tile(0x002C, 0, 0, 0, 0), 0x0003),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0009),
    RLEPattern(make_block_tile(0x004A, 0, 0, 0, 0), 0x0003),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0007),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x000F),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x000B),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0011),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x000D),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0008),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x0011),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0006),
    RLEPattern(make_block_tile(0x000B, 0, 0, 0, 0), 0x0002),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0015),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x000C),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x000A),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x000E),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0008),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x000F),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x0010),
    RLEPattern(make_block_tile(0x0007, 0, 0, 0, 0), 0x0006),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0013),
    RLEPattern(make_block_tile(0x004A, 0, 0, 0, 0), 0x0004),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0017),
    RLEPattern(make_block_tile(0x0007, 0, 0, 0, 0), 0x0004),
    RLEPattern(make_block_tile(0x000B, 0, 0, 0, 0), 0x0003)};

vector<RLEPattern> SSTrackFrame::DicLUT_7bit{
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x001B),
    RLEPattern(make_block_tile(0x004A, 0, 0, 0, 0), 0x0006),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x001D),
    RLEPattern(make_block_tile(0x004A, 0, 0, 0, 0), 0x0005),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0009),
    RLEPattern(make_block_tile(0x0007, 0, 0, 0, 0), 0x0005),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x001E),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0019),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0011),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x000C),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x007F),
    RLEPattern(make_block_tile(0x002C, 0, 0, 0, 0), 0x0004),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x000E),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x001C),
    RLEPattern(make_block_tile(0x004A, 0, 0, 0, 0), 0x000A),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x001A),
    RLEPattern(make_block_tile(0x004A, 0, 0, 0, 0), 0x0007),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0018),
    RLEPattern(make_block_tile(0x000B, 0, 0, 0, 0), 0x0004),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0012),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0010),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x000F),
    RLEPattern(make_block_tile(0x000B, 0, 0, 0, 0), 0x0005),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x000D),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0013),
    RLEPattern(make_block_tile(0x004A, 0, 0, 0, 0), 0x0009),
    RLEPattern(make_block_tile(0x004A, 0, 0, 0, 0), 0x000B),
    RLEPattern(make_block_tile(0x004A, 0, 0, 0, 0), 0x000C),
    RLEPattern(make_block_tile(0x002C, 0, 0, 0, 0), 0x0005),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0014),
    RLEPattern(make_block_tile(0x000B, 0, 0, 0, 0), 0x0007),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0016),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x000C),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x000E),
    RLEPattern(make_block_tile(0x004A, 0, 0, 0, 0), 0x0008),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x005F),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x000A),
    RLEPattern(make_block_tile(0x000B, 0, 0, 0, 0), 0x0006),
    RLEPattern(make_block_tile(0x000B, 0, 0, 0, 0), 0x0008),
    RLEPattern(make_block_tile(0x000B, 0, 0, 0, 0), 0x000A),
    RLEPattern(make_block_tile(0x0039, 0, 0, 0, 0), 0x0008),
    RLEPattern(make_block_tile(0x000B, 0, 0, 0, 0), 0x0009),
    RLEPattern(make_block_tile(0x002C, 0, 0, 0, 0), 0x0006),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0010),
    RLEPattern(make_block_tile(0x000B, 0, 0, 0, 0), 0x000C),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x000B),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0012),
    RLEPattern(make_block_tile(0x0007, 0, 0, 0, 0), 0x0007),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x001F),
    RLEPattern(make_block_tile(0x0028, 0, 0, 0, 0), 0x0012),
    RLEPattern(make_block_tile(0x000B, 0, 0, 0, 0), 0x000B),
    RLEPattern(make_block_tile(0x002C, 0, 0, 0, 0), 0x0007),
    RLEPattern(make_block_tile(0x002C, 0, 0, 0, 0), 0x000B),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0023),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0015),
    RLEPattern(make_block_tile(0x002C, 0, 0, 0, 0), 0x0008),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x002E),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x003F),
    RLEPattern(make_block_tile(0x0001, 0, 0, 0, 0), 0x0014),
    RLEPattern(make_block_tile(0x000B, 0, 0, 0, 0), 0x000D),
    RLEPattern(make_block_tile(0x002C, 0, 0, 0, 0), 0x0009),
    RLEPattern(make_block_tile(0x002C, 0, 0, 0, 0), 0x000A),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0025),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0055),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x0071),
    RLEPattern(make_block_tile(0x001D, 0, 0, 0, 0), 0x007C),
    RLEPattern(make_block_tile(0x004A, 0, 0, 0, 0), 0x000D),
    RLEPattern(make_block_tile(0x002C, 0, 0, 0, 0), 0x000C),
    RLEPattern(make_block_tile(0x002C, 0, 0, 0, 0), 0x000F),
    RLEPattern(make_block_tile(0x002C, 0, 0, 0, 0), 0x0010)};
#undef make_block_tile
