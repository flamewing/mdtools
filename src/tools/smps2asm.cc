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

#include <tr1/memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <map>
#include <queue>
#include <set>
#include <vector>

#include "getopt.h"
#include "bigendian_io.h"
#include "songtrack.h"
#include "fmvoice.h"

extern void PrintMacro(std::ostream &out, char const *macro);
extern void PrintHex2(std::ostream &out, unsigned char c, bool last);
extern void PrintHex4(std::ostream &out, unsigned short c, bool last);

struct S1IO {
	template <typename T>
	static inline size_t Read2(T &in) {
		return BigEndian::Read2(in);
	}

	template <typename T>
	static inline size_t Read4(T &in) {
		return BigEndian::Read4(in);
	}

	template <typename T>
	static inline void Write2(T &out, size_t c) {
		BigEndian::Write2(out, c);
	}

	template <typename T>
	static inline void Write4(T &out, size_t c) {
		BigEndian::Write4(out, c);
	}

	template <typename T>
	static inline int read_header_pointer(T &in, int base) {
		return static_cast<unsigned short>(Read2(in)) + base;
	}

	template <typename T>
	static inline int read_pointer(T &in, int base) {
		int ptr = int(in.tellg()) + 1;
		return static_cast<short>(Read2(in)) + ptr;
	}

	static inline int int2headerpointer(int val, int base) {
		return static_cast<unsigned short>(val);
	}
};

struct SNIO {
	template <typename T>
	static inline size_t Read2(T &in) {
		return LittleEndian::Read2(in);
	}

	template <typename T>
	static inline size_t Read4(T &in) {
		return LittleEndian::Read4(in);
	}

	template <typename T>
	static inline void Write2(T &out, size_t c) {
		LittleEndian::Write2(out, c);
	}

	template <typename T>
	static inline void Write4(T &out, size_t c) {
		LittleEndian::Write4(out, c);
	}

	template <typename T>
	static inline int read_header_pointer(T &in, int base) {
		return static_cast<unsigned short>(Read2(in) & 0x7fff) + base;
	}

	template <typename T>
	static inline int read_pointer(T &in, int base) {
		return static_cast<unsigned short>(Read2(in) & 0x7fff) + base;
	}

	static inline int int2headerpointer(int val, int base) {
		return static_cast<unsigned short>(val & 0x7fff) + base;
	}
};

static inline std::string make_label_code(int code) {
	char buf[20];
	std::sprintf(buf, "%02X", code);
	return std::string(buf);
}

static inline std::string need_call_label() {
	static int calllbl = 0;
	std::string ret("_Call");
	ret += make_label_code(calllbl++);
	return ret;
}

static inline std::string need_jump_label() {
	static int jumplbl = 0;
	std::string ret("_Jump");
	ret += make_label_code(jumplbl++);
	return ret;
}

static inline std::string need_loop_label() {
	static int looplbl = 0;
	std::string ret("_Loop");
	ret += make_label_code(looplbl++);
	return ret;
}

template<typename IO>
BaseNote *BaseNote::read(std::istream &in, int sonicver, int offset,
                         std::string const &projname, LocTraits::LocType tracktype,
                         std::multimap<int, std::string>& labels,
                         int &last_voc) {
	unsigned char byte = Read1(in);
	// Initialize to invalid 32-bit address.
	if (byte >= 0 && byte < 0x80)
		return new Duration(byte);
	else if (byte < 0xe0) {
		switch (tracktype) {
			case LocTraits::eFMInit:
			case LocTraits::eFMTrack:
				return new FMPSGNote(byte);
			case LocTraits::ePSGInit:
			case LocTraits::ePSGTrack:
				if (sonicver >= 3 && (byte == 0xd2 || byte == 0xd3))
					byte = 0xe0 + (byte & 1);
				else if (sonicver <= 2 && byte == 0xc5)
					byte = 0xe0;
				return new FMPSGNote(byte);
			case LocTraits::eDACInit:
			case LocTraits::eDACTrack:
			default:
				return new DACNote(byte);
		}
	}

	if (sonicver >= 3) {
		switch (byte) {
			case 0xff: { // Meta
				unsigned char spec = Read1(in);
				switch (spec) {
					case 0x02:
					case 0x07:
						return new CoordFlag1ParamByte<false>(byte, spec);
					case 0x00:
					case 0x01:
					case 0x04:
						return new CoordFlag2ParamBytes<false>(byte, spec,
						                                       Read1(in));
					case 0x06:
						return new CoordFlag3ParamBytes<false>(byte, spec,
						                                       Read1(in), Read1(in));
					case 0x05:
						return new CoordFlag5ParamBytes<false>(byte, spec,
						                                       Read1(in), Read1(in),
						                                       Read1(in), Read1(in));
					case 0x03: {
						int ptr = IO::read_pointer(in, offset);
						unsigned char cnt = Read1(in);
						return new CoordFlagPointer2ParamBytes<false>(byte, spec,
						        cnt, ptr);
					}
				}
			}
			case 0xf7: { // Loop
				unsigned char index = Read1(in), repeats = Read1(in);
				int ptr = IO::read_pointer(in, offset);
				std::multimap<int, std::string>::iterator it = labels.find(ptr);
				if (it == labels.end())
					labels.insert(std::make_pair(ptr, projname + need_loop_label()));
				return new CoordFlagPointer2ParamBytes<false>(byte, index, repeats, ptr);
			}
			case 0xf0:  // Start modulation
			case 0xfe: { // FM3 Special mode
				unsigned char p1 = Read1(in), p2 = Read1(in),
				              p3 = Read1(in), p4 = Read1(in);
				return new CoordFlag4ParamBytes<false>(byte, p1, p2, p3, p4);
			}
			case 0xf6:  // Jump
			case 0xf8:  // Call
			case 0xfc: { // Continuous loop
				int ptr = IO::read_pointer(in, offset);
				std::multimap<int, std::string>::iterator it = labels.find(ptr);
				if (it == labels.end()) {
					std::string lbl;
					if (byte == 0xf6)
						lbl = need_jump_label();
					else if (byte == 0xf8)
						lbl = need_call_label();
					else
						lbl = need_loop_label();
					labels.insert(std::make_pair(ptr, projname + lbl));
				}
				if (byte == 0xf6)
					return new CoordFlagPointerParam<true>(byte, ptr);
				else
					return new CoordFlagPointerParam<false>(byte, ptr);
			}
			case 0xeb: { // Conditional jump
				unsigned char index = Read1(in);
				int ptr = IO::read_pointer(in, offset);
				std::multimap<int, std::string>::iterator it = labels.find(ptr);
				if (it == labels.end())
					labels.insert(std::make_pair(ptr, projname + need_jump_label()));

				return new CoordFlagPointer1ParamByte<false>(byte, index, ptr);
			}

			case 0xe5:
			case 0xee:
			case 0xf1:
				return new CoordFlag2ParamBytes<false>(byte, Read1(in), Read1(in));

			case 0xef: {
				signed char voc = Read1(in);
				if (tracktype == LocTraits::eFMTrack && voc >= 0 && voc > last_voc)
					last_voc = voc;
				if (voc < 0) {
					unsigned char id = Read1(in) - 0x81;
					if (tracktype == LocTraits::eFMTrack)
						voc &= 0x7f;
					return new CoordFlag2ParamBytes<false>(byte, voc, id);
				} else
					return new CoordFlag1ParamByte<false>(byte, voc);
			}

			case 0xe0:
			case 0xe1:
			case 0xe4:
			case 0xe6:
			case 0xe8:
			case 0xea:
			case 0xec:
			case 0xed:
			case 0xf3:
			case 0xf4:
			case 0xf5:
			case 0xfb:
			case 0xfd:
				return new CoordFlag1ParamByte<false>(byte, Read1(in));

			case 0xe2: { // Fade to previous
				unsigned char c = Read1(in);
				if (c == 0xff)
					return new CoordFlagNoParams<false>(byte);
				else
					return new CoordFlag1ParamByte<false>(byte, c);
			}

			case 0xe3:
			case 0xf2:
			case 0xf9:
				return new CoordFlagNoParams<true>(byte);

			default:
				return new CoordFlagNoParams<false>(byte);
		}
	} else {
		switch (byte) {
			case 0xf7: { // Loop
				unsigned char index = Read1(in), repeats = Read1(in);
				int ptr = IO::read_pointer(in, offset);
				std::multimap<int, std::string>::iterator it = labels.find(ptr);
				if (it == labels.end())
					labels.insert(std::make_pair(ptr, projname + need_loop_label()));
				return new CoordFlagPointer2ParamBytes<false>(byte, index, repeats, ptr);
			}
			case 0xf0: { // Start modulation
				unsigned char wait   = Read1(in), speed = Read1(in),
				              change = Read1(in), steps = Read1(in);
				return new CoordFlag4ParamBytes<false>(byte, wait, speed, change, steps);
			}
			case 0xf6:  // Jump
			case 0xf8: { // Call
				int ptr = IO::read_pointer(in, offset);
				std::multimap<int, std::string>::iterator it = labels.find(ptr);
				if (it == labels.end())
					labels.insert(std::make_pair(ptr, projname + (byte == 0xf6 ? need_jump_label()
					                             : need_call_label())));
				if (byte == 0xf6)
					return new CoordFlagPointerParam<true>(byte, ptr);
				else
					return new CoordFlagPointerParam<false>(byte, ptr);
			}
			case 0xed:  // S1: celar push flag; S2: eat byte
				if (sonicver == 1)
					return new CoordFlagNoParams<false>(byte);
				else
					return new CoordFlag1ParamByte<false>(byte, Read1(in));
			case 0xee:  // S1: Stop special FM4; S2: nop
				if (sonicver == 1)
					return new CoordFlagNoParams<true>(byte);
				else
					return new CoordFlagNoParams<false>(byte);
			case 0xef: { // Set FM voice
				int voc = Read1(in);
				if (voc > last_voc)
					last_voc = voc;
				return new CoordFlag1ParamByte<false>(byte, voc);
			}

			case 0xe0:
			case 0xe1:
			case 0xe2:
			case 0xe5:
			case 0xe6:
			case 0xe8:
			case 0xe9:
			case 0xea:
			case 0xeb:
			case 0xec:
			case 0xf3:
			case 0xf5:
				return new CoordFlag1ParamByte<false>(byte, Read1(in));

			case 0xe3:
			case 0xe4:
			case 0xf2:
				return new CoordFlagNoParams<true>(byte);

			default:
				return new CoordFlagNoParams<false>(byte);
		}
	}
}

template<typename IO>
class DumpSmps {
	std::istream &in;
	std::ostream &out;
	std::string const &projname;
	int sonicver, startloc, offset, len;
	bool sfx, s3kmode;
public:
	DumpSmps(std::istream &i, std::ostream &o, int s, int off, std::string const &nm, bool tf, bool s3km)
		: in(i), out(o), sonicver(s), offset(off), projname(nm), sfx(tf), s3kmode(s3km) {
		startloc = in.tellg();
		in.seekg(0, std::ios::end);
		len = in.tellg();
		in.seekg(startloc);
	}
	void dump_smps() {
		// Set up data structures for exploratory disassembly.
		std::priority_queue<LocTraits> todo;
		std::map<int, LocTraits::LocType> explored;
		std::map<int, std::tr1::shared_ptr<BaseNote> > trackdata;

		// This will hold the labels of each location.
		std::multimap<int, std::string> labels;
		std::set<std::string> tracklabels;

		// Start with music conversion header.
		out << projname << "_Header:" << std::endl;
		PrintMacro(out, "smpsHeaderStartSong");
		out << (sonicver > 3 ? 3 : sonicver) << std::endl;

		// Now for voice pointer; this is the first piece of data in both
		// songs and SFX.
		int vocptr = IO::Read2(in);
		bool ext_vocs;
		// Also using a hack for null pointer here.
		bool uses_uvb = (sonicver >= 3 && vocptr == 0x17d8) || !vocptr;
		int last_voc = -1;

		if (!vocptr) {
			// Null voice bank.
			ext_vocs = true;
			out << "\tsmpsHeaderVoiceNull" << std::endl;
			todo.push(LocTraits(vocptr, LocTraits::eExtVoices));
		} else if (uses_uvb) {
			// Universal voice bank in S3/SK/S3D.
			ext_vocs = true;
			out << "\tsmpsHeaderVoiceUVB" << std::endl;
			todo.push(LocTraits(vocptr, LocTraits::eExtVoices));
		} else {
			vocptr = IO::int2headerpointer(vocptr, offset);
			ext_vocs = (vocptr >= len) || (vocptr < 0);
			PrintMacro(out, "smpsHeaderVoice");
			out << projname << "_Voices" << std::endl;

			// An external voice bank will be printed as a note at the end;
			// the other type of voice bank will be extracted based on the
			// number of referenced voices.
			if (ext_vocs)
				todo.push(LocTraits(vocptr, LocTraits::eExtVoices));
			else
				todo.push(LocTraits(vocptr, LocTraits::eVoices));
		}

		// Music and SFX differ in the next few headers.
		if (sfx) {
			// Tempo dividing timing
			int tempodiv = Read1(in);
			// Output tempo header; it will deal with main tempo conversion.
			PrintMacro(out, "smpsHeaderTempoSFX");
			PrintHex2(out, tempodiv, true);
			out << std::endl;

			// Number of FM+PSG channels.
			int nchan = Read1(in);
			// Output channel setup header.
			PrintMacro(out, "smpsHeaderChanSFX");
			PrintHex2(out, nchan, true);
			out << std::endl << std::endl;

			// Time to output headers for all tracks then queue uptheir data
			// for exploration.
			for (int i = 0; i < nchan; i++) {
				int playctrl = Read1(in), chanid  = Read1(in),
				    trackptr = IO::read_header_pointer(in, offset),
				    keydisp  = Read1(in), initvol = Read1(in);

				// Special case for non-ordinary playback control bytes.
				if (playctrl != 0x80) {
					out << ";\tOriginal SFX playback control byte was ";
					PrintHex2(out, playctrl, true);
					std::cerr << "Original SFX playback control byte was ";
					PrintHex2(std::cerr, playctrl, true);
				}
				PrintMacro(out, "smpsHeaderSFXChannel");

				// Create track label and channel assignmend constant for header.
				std::string lbl = projname;
				switch (chanid) {
					case 0x80:
					case 0xA0:
					case 0xC0: {
						char c = ((chanid & 0x60) >> 5) + '1';
						out << "cPSG" << c << ", ";
						lbl += "_PSG";
						lbl += c;
						break;
					}

					case 0xE0:
						out << "cNoise, ";
						lbl += "_Noise";
						break;

					case 0x00:
					case 0x01:
					case 0x02:
						chanid++;
						// Fall through
					case 0x04:
					case 0x05:
					case 0x06: {
						char c = (chanid & 0x07) + '0';
						out << "cFM" << c << ", ";
						lbl += "_FM";
						lbl += c;
						break;
					}

					default:
						PrintHex2(out, chanid , false);
						std::cerr << "Invalid SFX channel ID." << std::endl;
						exit(6);
						break;
				}
				out << lbl << ",\t";
				PrintHex2(out, keydisp , false);
				PrintHex2(out, initvol , true);
				out << std::endl;

				// Add to queue/label list.
				if ((chanid & 0x80) != 0)
					todo.push(LocTraits(trackptr, LocTraits::ePSGInit));
				else
					todo.push(LocTraits(trackptr, LocTraits::eFMInit));
				labels.insert(std::make_pair(trackptr, lbl));
				tracklabels.insert(lbl);
			}
		} else {
			// Number of FM+DAC, PSG channels.
			int nfm = Read1(in), npsg = Read1(in);
			// Output channel setup header.
			PrintMacro(out, "smpsHeaderChan");
			PrintHex2(out, nfm , false);
			PrintHex2(out, npsg, true);
			out << std::endl;

			// Tempo dividing timing, main tempo modifier.
			int tempodiv = Read1(in), tempomod = Read1(in);
			// Output tempo header; it will deal with main tempo conversion.
			PrintMacro(out, "smpsHeaderTempo");
			PrintHex2(out, tempodiv, false);
			PrintHex2(out, tempomod, true);
			out << std::endl << std::endl;

			// First come the DAC and FM channels.
			for (int i = 0; i < nfm; i++) {
				int ptr = IO::read_header_pointer(in, offset),
				    pitch = Read1(in), vol = Read1(in);

				std::string lbl = projname;
				LocTraits::LocType type;
				if (i == 0) {
					// DAC is always first.
					lbl += "_DAC";
					labels.insert(std::make_pair(ptr, lbl));
					tracklabels.insert(lbl);
					PrintMacro(out, "smpsHeaderDAC");
					if (pitch || vol) {
						out << lbl << ",\t";
						PrintHex2(out, pitch, false);
						PrintHex2(out, vol, true);
					}
					out << lbl << std::endl;
					type = LocTraits::eDACInit;
				} else {
					// Now come FM channels.
					char c = i + '0';
					lbl += "_FM";
					lbl += c;
					labels.insert(std::make_pair(ptr, lbl));
					tracklabels.insert(lbl);
					PrintMacro(out, "smpsHeaderFM");
					out << lbl << ",\t";
					PrintHex2(out, pitch, false);
					PrintHex2(out, vol, true);
					out << std::endl;
					type = LocTraits::eFMInit;
				}

				// Add to queue.
				todo.push(LocTraits(ptr, type));
			}

			// Time for PSG channels.
			for (int i = 0; i < npsg; i++) {
				int ptr     = IO::read_header_pointer(in, offset),
				    pitch   = Read1(in), vol  = Read1(in),
				    modctrl = Read1(in), tone = Read1(in);

				std::string lbl = projname;
				char c = i + '1';
				lbl += "_PSG";
				lbl += c;
				labels.insert(std::make_pair(ptr, lbl));
				tracklabels.insert(lbl);
				PrintMacro(out, "smpsHeaderPSG");
				out << lbl << ",\t";
				PrintHex2(out, pitch  , false);
				PrintHex2(out, vol    , false);
				PrintHex2(out, modctrl, false);
				BaseNote::print_psg_tone(out, tone, sonicver, true);
				out << std::endl;

				// Add to queue.
				todo.push(LocTraits(ptr, LocTraits::ePSGInit));
			}
		}

		// Mark all contents so far as having been explored.
		for (size_t i = startloc; i < in.tellg(); i++)
			explored.insert(std::make_pair(i, LocTraits::eHeader));

		while (todo.size() > 1) {
			LocTraits next_loc = todo.top();

			// If we are down to voices, we are done with this loop.
			if (next_loc.type >= LocTraits::eVoices)
				break;

			// Now remap the init types for simplicity, as they did their job.
			switch (next_loc.type) {
				case LocTraits::eDACInit:
					next_loc.type = LocTraits::eDACTrack;
					break;
				case LocTraits::eFMInit:
					next_loc.type = LocTraits::eFMTrack;
					break;
				case LocTraits::ePSGInit:
					next_loc.type = LocTraits::ePSGTrack;
					break;
				default:
					break;
			}

			todo.pop();

			// Don't explore again what has been done already.
			if (explored.find(next_loc.loc) != explored.end())
				continue;

			if (next_loc.loc < 0 || next_loc.loc >= len) {
				std::multimap<int, std::string>::iterator it = labels.lower_bound(next_loc.loc),
				                                          end = labels.upper_bound(next_loc.loc);

				if (it != end)
					BaseNote::force_linebreak(out, true);

				out << "; The following track data was present at " << std::dec
				    << next_loc.loc << " bytes from the start of the song."
				    << std::endl;
				for (; it != end; ++it) {

					std::string lbl = it->second;
					if (tracklabels.find(lbl) != tracklabels.end())
						out << "; " << lbl.substr(lbl.rfind('_') + 1) << " Data" << std::endl;
					out << it->second << ":" << std::endl;
				}

				explored.insert(std::make_pair(next_loc.loc, next_loc.type));
				continue;
			}

			// Clear stream errors and reposition to correct location.
			in.clear();
			in.seekg(next_loc.loc);

			while (true) {
				int lastloc = in.tellg();
				std::tr1::shared_ptr<BaseNote> note =
				    std::tr1::shared_ptr<BaseNote>(BaseNote::read<IO>(in, sonicver,
				                                   offset, projname, next_loc.type,
				                                   labels, last_voc));
				trackdata.insert(std::make_pair(lastloc, note));

				// If the data includes a jump target, add it to queue.
				if (note->has_pointer())
					todo.push(LocTraits(note->get_pointer(), next_loc.type));

				// Add in freshly explored data to list.
				for (int i = lastloc; i < in.tellg(); i++)
					explored.insert(std::make_pair(i, next_loc.type));

				// If we reach track end, or if we reached the end of file,
				// break from loop.
				if (note->ends_track() || !in.good())
					break;
			}
		}

		// Grab the entry for voices.
		LocTraits voices = todo.top();
		todo.pop();

		// Read voices, unless universal voice bank, null voice pointer or
		// external voice bank.
		if (uses_uvb) {
		} else if (last_voc < 0) {
			// There are no voices in use; so we insert a comment saying so and
			// add a dummy label for the voice pointer.
			std::stringstream str(std::ios::in | std::ios::out);
			str << "; Song seems to not use any FM voices" << std::endl;
			str << projname << "_Voices";
			labels.insert(std::make_pair(voices.loc, str.str()));
			std::tr1::shared_ptr<BaseNote> note =
			    std::tr1::shared_ptr<BaseNote>(new NullNote());
			trackdata.insert(std::make_pair(voices.loc, note));
		} else if (ext_vocs || voices.type == LocTraits::eExtVoices) {
			// The voices were not in the file. Print a comment with their
			// location and a dummy label.
			std::stringstream str(std::ios::in | std::ios::out);
			str << "; Voices were not within the file: they are the first "
			    << (last_voc + 1) << " voices located at " << std::dec << voices.loc
			    << " bytes from the start of the song." << std::endl;
			str << "; The following label is a dummy label and should be moved to the correct location.";
			str << std::endl << projname << "_Voices";
			labels.insert(std::make_pair(voices.loc, str.str()));
			labels.insert(std::make_pair(voices.loc, str.str()));
			std::tr1::shared_ptr<BaseNote> note =
			    std::tr1::shared_ptr<BaseNote>(new NullNote());
			trackdata.insert(std::make_pair(voices.loc, note));
		} else if (last_voc >= 0) {
			in.clear();
			in.seekg(voices.loc);
			labels.insert(std::make_pair(voices.loc, projname + "_Voices"));

			// Read each voice in turn.
			for (int i = 0; i <= last_voc; i++) {
				if (in.tellg() + std::streamoff(25) > len) {
					// End of file reached in the middle of a voice.
					std::cerr << "Broken voice! The end-of-file was reached in the middle of an FM voice used by the song." << std::endl;
					break;
				}

				int lastloc = in.tellg();
				std::tr1::shared_ptr<BaseNote> voc =
				    std::tr1::shared_ptr<BaseNote>(new FMVoice(in, sonicver, i));
				trackdata.insert(std::make_pair(lastloc, voc));

				// Add in freshly explored data to list.
				for (int i = lastloc; i < in.tellg(); i++)
					explored.insert(std::make_pair(i, LocTraits::eVoices));
			}
		}

		int lastlabel = -1;
		for (std::map<int, std::tr1::shared_ptr<BaseNote> >::iterator it = trackdata.begin();
		        it != trackdata.end(); ++it) {
			int off = it->first;
			std::tr1::shared_ptr<BaseNote> note = it->second;
			if (off > lastlabel) {
				std::multimap<int, std::string>::iterator it = labels.upper_bound(lastlabel),
				                                          end = labels.upper_bound(off);

				if (it != end)
					BaseNote::force_linebreak(out, true);

				for (; it != end; ++it) {
					std::string lbl = it->second;
					if (tracklabels.find(lbl) != tracklabels.end())
						out << "; " << lbl.substr(lbl.rfind('_') + 1) << " Data" << std::endl;
					out << it->second << ":" << std::endl;
				}

				lastlabel = off;
			}

			std::map<int, LocTraits::LocType>::iterator ty = explored.find(off);
			//assert(ty != explored.end());
			if (ty != explored.end())
				note->print(out, sonicver, ty->second, labels, s3kmode);
		}

		/*
		if (uses_uvb)
		    // Universal voice bank or null voice pointer. Print nothing, read no voices.
		    return;
		else if (last_voc < 0)
		{
		    // There are no voices in use; so we insert a comment saying so and
		    // add a dummy label for the voice pointer.
		    out << std::endl << std::endl << "; Song seems to not use any FM voices" << std::endl;
		    out << projname << "_Voices:" << std::endl;
		    return;
		}
		else if (todo.empty())
		{
		    // This should be impossible.
		    out << std::endl << "; FM voices used by the song were not within the file; moreover,"
		        << std::endl << "; due to an unknown error, this software could not locate them." << std::endl;
		    return;
		}

		// Grab the entry for voices.
		LocTraits voices = todo.top();
		todo.pop();

		if (voices.type == LocTraits::eExtVoices)
		{
		    // The voices were not in the file. Print a comment with their
		    // location and a dummy label.
		    out << std::endl << std::endl << "; Voices were not within the file: they are the first "
		        << (last_voc + 1) << " voices located at " << std::dec << voices.loc
		        << " bytes from the start of the song." << std::endl;
		    out << "; The following label is a dummy label and should be moved to the correct location.";
		    out << std::endl << projname << "_Voices:" << std::endl;
		    return;
		}

		// Clear errors and reposition stream.
		in.clear();
		in.seekg(voices.loc);
		out << std::endl << std::endl << projname << "_Voices:" << std::endl;

		// Print each voice in turn.
		for (int i = 0; i <= last_voc; i++)
		{
		    if (in.tellg() + std::streamoff(25) > len)
		    {
		        // End of file reached in the middle of a voice.
		        std::cerr << "Broken voice! The end-of-file was reached in the middle of an FM voice used by the song." << std::endl;
		        break;
		    }

		    // Print the voice.
		    fm_voice voc;
		    voc.read(in, sonicver);
		    voc.print(out, sonicver, i);
		}
		*/
	}
};

static void usage() {
	std::cerr << "Usage: smps2asm [-x|--extract [{pointer}]] [-u|--saxman] [-o|--offset {offsetval}] [-s|--sfx]" << std::endl
	          << "                {-v|--sonicver} {version} [-3|--s3kmode] {input_filename} {output_filename} {projname}" << std::endl;
	std::cerr << std::endl;
	std::cerr << "\t-s,--sfx     \tFile is SFX, not music." << std::endl;
	std::cerr << "\t-u,--saxman  \tNot implemented yet; once it is, it will work as follows:" << std::endl
	          << "\t             \tAssume Music file is Saxman-compressed. In most cases, this" << std::endl
	          << "\t             \tshould be combined with --offset 0x1380 --sonicver 2." << std::endl;
	std::cerr << "\t-x,--extract \tExtract from {pointer} address in file. This should never be" << std::endl
	          << "\t             \tcombined with --offset unless --saxman is also used." << std::endl;
	std::cerr << "\t-o,--offset  \tAssumes starting pointer for music/sfx within its sound bank" << std::endl
	          << "\t             \tis {offsetval}. Ignored if used with --sonicver 1." << std::endl;
	std::cerr << "\t-v,--sonicver\tSets Sonic version to {version}. This also sets underlying" << std::endl
	          << "\t             \tSMPS type. {version} can be '1' Sonic 1, '2' for Sonic 2 or" << std::endl
	          << "\t             \t'3' for Sonic 3, '4' for Sonic & Knuckles, or '5' for Sonic" << std::endl
	          << "\t             \t3D Blast." << std::endl;
	std::cerr << "\t-3,--s3kmode \tThis flag is valid for Sonic 1 and Sonic 2 only; this will" << std::endl
	          << "\t             \tcause all sequences of durations after a rest to be printed" << std::endl
	          << "\t             \twith the rests shown explicitly." << std::endl
	          << "\t             \tWARNING: A smpsCall is assumed to set the note to something." << std::endl
	          << "\t             \tother than a rest; this assumption turns out to be correct" << std::endl
	          << "\t             \tfor all Sonic 1 and Sonic 2 songs and SFX, but it could be" << std::endl
	          << "\t             \twrong for another game with a compatible SMPS implementation." << std::endl
	          << "\t             \tUse with care." << std::endl << std::endl;
}

int main(int argc, char *argv[]) {
	static struct option long_options[] = {
		{"extract", required_argument, 0, 'x'},
		{"saxman",  no_argument      , 0, 'u'},
		{"offset",  required_argument, 0, 'o'},
		{"sonicver", required_argument, 0, 'v'},
		{"sfx",     no_argument      , 0, 's'},
		{"s3kmode", no_argument      , 0, '3'},
		{0, 0, 0, 0}
	};

	bool sfx = false, saxman = false, s3kmode = false;
	int pointer = 0, offset = 0, sonicver = -1;

	while (true) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "x:uo:v:s3",
		                    long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 'x':
				pointer = strtoul(optarg, 0, 0);
				break;

			case 'u':
				saxman = true;
				break;

			case 'o':
				offset = strtoul(optarg, 0, 0);
				break;

			case 's':
				sfx = true;
				break;

			case 'v':
				sonicver = strtoul(optarg, 0, 0);
				break;

			case '3':
				s3kmode = true;
				break;
		}
	}

	if (argc - optind < 3 || sonicver < 1 || sonicver > 5
	        || (!saxman && pointer != 0 && offset != 0)
	        || (s3kmode && sonicver > 2)) {
		usage();
		return 1;
	}

	if (pointer) {
		if (saxman)
			offset = -offset;
		else {
			offset = pointer;
			if (sonicver != 1)
				offset &= ~0x7fff;
		}
	} else if (sonicver != 1)
		offset = -offset;
	else
		offset = 0;

	std::ifstream fin(argv[optind + 0], std::ios::in | std::ios::binary);
	if (!fin.good()) {
		std::cerr << "Input file '" << argv[optind + 0] << "' could not be opened." << std::endl << std::endl;
		return 2;
	}

	std::istream *src;
	std::stringstream sin(std::ios::in | std::ios::out | std::ios::binary);

	fin.seekg(0);
	if (!saxman) {
		src = &fin;
		//sin << fin.rdbuf();
		fin.seekg(pointer);
	} else {
		//saxman::decode(fin, sin, 0, false);
		src = &sin;
		std::cerr << "Sorry, Saxman decompression not supported yet." << std::endl;
		return 4;
	}

	std::ofstream fout(argv[optind + 1], std::ios::out | std::ios::binary);
	if (!fout.good()) {
		std::cerr << "Output file '" << argv[optind + 1] << "' could not be opened." << std::endl << std::endl;
		return 3;
	}

	std::string projname(argv[optind + 2]);
	if (sonicver == 1) {
		DumpSmps<S1IO> smps(*src, fout, sonicver, offset, projname, sfx, s3kmode);
		smps.dump_smps();
	} else {
		DumpSmps<SNIO> smps(*src, fout, sonicver, offset, projname, sfx, s3kmode);
		smps.dump_smps();
	}
}
