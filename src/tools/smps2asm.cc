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

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

#include <getopt.h>

#include <mdcomp/bigendian_io.hh>
#include <mdcomp/saxman.hh>

#include <mdtools/fmvoice.hh>
#include <mdtools/ignore_unused_variable_warning.hh>
#include <mdtools/songtrack.hh>

#define SMPS2ASM_VERSION 1

using namespace std;

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
		return static_cast<uint16_t>(Read2(in)) + base;
	}

	template <typename T>
	static inline int read_pointer(T &in, int base) {
		ignore_unused_variable_warning(base);
		int ptr = int(in.tellg()) + 1;
		return static_cast<int16_t>(Read2(in)) + ptr;
	}

	static inline int int2headerpointer(int val, int base) {
		ignore_unused_variable_warning(base);
		return static_cast<uint16_t>(val);
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
		return static_cast<uint16_t>(Read2(in) & 0x7fff) + base;
	}

	template <typename T>
	static inline int read_pointer(T &in, int base) {
		return static_cast<uint16_t>(Read2(in) & 0x7fff) + base;
	}

	static inline int int2headerpointer(int val, int base) {
		return static_cast<uint16_t>(val & 0x7fff) + base;
	}
};

static inline string make_label_code(int code) {
	char buf[20];
	sprintf(buf, "%02X", code);
	return string(buf);
}

static inline string need_call_label() {
	static int calllbl = 0;
	string ret("_Call");
	ret += make_label_code(calllbl++);
	return ret;
}

static inline string need_jump_label() {
	static int jumplbl = 0;
	string ret("_Jump");
	ret += make_label_code(jumplbl++);
	return ret;
}

static inline string need_loop_label() {
	static int looplbl = 0;
	string ret("_Loop");
	ret += make_label_code(looplbl++);
	return ret;
}

template<typename IO>
BaseNote *BaseNote::read(istream &in, int sonicver, int offset,
                         string const &projname, LocTraits::LocType tracktype,
                         multimap<int, string> &labels,
                         int &last_voc, unsigned char keydisp) {
	unsigned char byte = Read1(in);
	// Initialize to invalid 32-bit address.
	if (byte < 0x80) {
		return new Duration(byte, keydisp);
	} else if (byte < 0xe0) {
		switch (tracktype) {
			case LocTraits::eFMInit:
			case LocTraits::eFMTrack:
				return new FMPSGNote(byte, keydisp);
			case LocTraits::ePSGInit:
			case LocTraits::ePSGTrack: {
				return new FMPSGNote(byte, keydisp);
			}
			case LocTraits::eDACInit:
			case LocTraits::eDACTrack:
			case LocTraits::ePCMInit:
			case LocTraits::ePCMTrack:
			case LocTraits::ePWMInit:
			case LocTraits::ePWMTrack:
			default:
				return new DACNote(byte, keydisp);
		}
	}

	if (sonicver >= 3) {
		switch (byte) {
			case 0xff: { // Meta
				unsigned char spec = Read1(in);
				switch (spec) {
					case 0x02:
					case 0x07:
						return new CoordFlag1ParamByte<false>(byte, keydisp, spec);
					case 0x00:
					case 0x01:
					case 0x04:
						return new CoordFlag2ParamBytes<false>(byte, keydisp, spec,
						                                       Read1(in));
					case 0x06:
						return new CoordFlag3ParamBytes<false>(byte, keydisp, spec,
						                                       Read1(in), Read1(in));
					case 0x05:
						return new CoordFlag5ParamBytes<false>(byte, keydisp, spec,
						                                       Read1(in), Read1(in),
						                                       Read1(in), Read1(in));
					case 0x03: {
						int ptr = IO::read_pointer(in, offset);
						unsigned char cnt = Read1(in);
						return new CoordFlagPointer2ParamBytes<false>(byte, keydisp, spec,
						        cnt, ptr);
					}
					default:
						cerr << "Invalid meta-coordination flag '";
						PrintHex2(cerr, 0xff, true);
						PrintHex2(cerr, byte, false);
						cerr << "' found at offset " << size_t(in.tellg()) - 2
						     << "; it will be ignored." << endl;
						return new NullNote();
				}
			}
			case 0xf7: { // Loop
				unsigned char index = Read1(in), repeats = Read1(in);
				int ptr = IO::read_pointer(in, offset);
				auto it = labels.find(ptr);
				if (it == labels.end()) {
					labels.emplace(ptr, projname + need_loop_label());
				}
				return new CoordFlagPointer2ParamBytes<false>(byte, keydisp, index, repeats, ptr);
			}
			case 0xf0:  // Start modulation
			case 0xfe: { // FM3 Special mode
				unsigned char p1 = Read1(in), p2 = Read1(in),
				              p3 = Read1(in), p4 = Read1(in);
				return new CoordFlag4ParamBytes<false>(byte, keydisp, p1, p2, p3, p4);
			}
			case 0xf6:  // Jump
			case 0xf8:  // Call
			case 0xfc: { // Continuous loop
				int ptr = IO::read_pointer(in, offset);
				auto it = labels.find(ptr);
				if (it == labels.end()) {
					string lbl;
					if (byte == 0xf6) {
						lbl = need_jump_label();
					} else if (byte == 0xf8) {
						lbl = need_call_label();
					} else {
						lbl = need_loop_label();
					}
					labels.emplace(ptr, projname + lbl);
				}
				if (byte == 0xf6) {
					return new CoordFlagPointerParam<true>(byte, keydisp, ptr);
				} else {
					return new CoordFlagPointerParam<false>(byte, keydisp, ptr);
				}
			}
			case 0xeb: { // Conditional jump
				unsigned char index = Read1(in);
				int ptr = IO::read_pointer(in, offset);
				auto it = labels.find(ptr);
				if (it == labels.end()) {
					labels.emplace(ptr, projname + need_jump_label());
				}

				return new CoordFlagPointer1ParamByte<false>(byte, keydisp, index, ptr);
			}

			case 0xe5:
			case 0xee:
			case 0xf1:
				return new CoordFlag2ParamBytes<false>(byte, keydisp, Read1(in), Read1(in));

			case 0xef: {
				signed char voc = Read1(in);
				if (tracktype == LocTraits::eFMTrack && voc >= 0 && voc > last_voc) {
					last_voc = voc;
				}
				if (voc < 0) {
					unsigned char id = Read1(in) - 0x81;
					if (tracktype == LocTraits::eFMTrack) {
						voc &= 0x7f;
					}
					return new CoordFlag2ParamBytes<false>(byte, keydisp, voc, id);
				} else {
					return new CoordFlag1ParamByte<false>(byte, keydisp, voc);
				}
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
			case 0xfd:
				return new CoordFlag1ParamByte<false>(byte, keydisp, Read1(in));

			case 0xfb:
				return new CoordFlagChgKeydisp(byte, keydisp, Read1(in));

			case 0xe2: { // Fade to previous
				unsigned char c = Read1(in);
				if (c == 0xff) {
					return new CoordFlagNoParams<false>(byte, keydisp);
				} else {
					return new CoordFlag1ParamByte<false>(byte, keydisp, c);
				}
			}

			case 0xe3:
			case 0xf2:
			case 0xf9:
				return new CoordFlagNoParams<true>(byte, keydisp);

			default:
				return new CoordFlagNoParams<false>(byte, keydisp);
		}
	} else {
		switch (byte) {
			case 0xf7: { // Loop
				unsigned char index = Read1(in), repeats = Read1(in);
				int ptr = IO::read_pointer(in, offset);
				auto it = labels.find(ptr);
				if (it == labels.end()) {
					labels.emplace(ptr, projname + need_loop_label());
				}
				return new CoordFlagPointer2ParamBytes<false>(byte, keydisp, index, repeats, ptr);
			}
			case 0xf0: { // Start modulation
				unsigned char wait   = Read1(in), speed = Read1(in),
				              change = Read1(in), steps = Read1(in);
				return new CoordFlag4ParamBytes<false>(byte, keydisp, wait, speed, change, steps);
			}
			case 0xf6:  // Jump
			case 0xf8: { // Call
				int ptr = IO::read_pointer(in, offset);
				auto it = labels.find(ptr);
				if (it == labels.end()) {
					labels.emplace(ptr, projname + (byte == 0xf6
					                                ? need_jump_label()
					                                : need_call_label()));
				}
				if (byte == 0xf6) {
					return new CoordFlagPointerParam<true>(byte, keydisp, ptr);
				} else {
					return new CoordFlagPointerParam<false>(byte, keydisp, ptr);
				}
			}
			case 0xed:  // S1: clear push flag; S2: eat byte
				if (sonicver == 1) {
					return new CoordFlagNoParams<false>(byte, keydisp);
				} else {
					return new CoordFlag1ParamByte<false>(byte, keydisp, Read1(in));
				}
			case 0xee:  // S1: Stop special FM4; S2: nop
				if (sonicver == 1) {
					return new CoordFlagNoParams<true>(byte, keydisp);
				} else {
					return new CoordFlagNoParams<false>(byte, keydisp);
				}
			case 0xef: { // Set FM voice
				int voc = Read1(in);
				if (voc > last_voc) {
					last_voc = voc;
				}
				return new CoordFlag1ParamByte<false>(byte, keydisp, voc);
			}

			case 0xe0:
			case 0xe1:
			case 0xe2:
			case 0xe5:
			case 0xe6:
			case 0xe8:
			case 0xea:
			case 0xeb:
			case 0xec:
			case 0xf3:
			case 0xf5:
				return new CoordFlag1ParamByte<false>(byte, keydisp, Read1(in));

			case 0xe9:
				return new CoordFlagChgKeydisp(byte, keydisp, Read1(in));

			case 0xe3:
			case 0xe4:
			case 0xf2:
				return new CoordFlagNoParams<true>(byte, keydisp);

			default:
				return new CoordFlagNoParams<false>(byte, keydisp);
		}
	}
}

template<typename IO>
class DumpSmps {
	istream &in;
	ostream &out;
	string const projname;
	int sonicver, startloc, offset, len;
	bool sfx, s3kmode;

public:
	DumpSmps(istream &i, ostream &o, int s, int off,
	         string nm, bool tf, bool s3km)
		: in(i), out(o), projname(std::move(nm)), sonicver(s), offset(off), sfx(tf), s3kmode(s3km) {
		startloc = in.tellg();
		in.seekg(0, ios::end);
		len = in.tellg();
		in.seekg(startloc);
	}
	void dump_smps() {
		// Set up data structures for exploratory disassembly.
		priority_queue<LocTraits> todo;
		map<int, LocTraits::LocType> explored;
		map<int, shared_ptr<BaseNote> > trackdata;

		// This will hold the labels of each location.
		multimap<int, string> labels;
		set<string> tracklabels;

		// Start with music conversion header.
		out << projname << "_Header:" << endl;
		PrintMacro(out, "smpsHeaderStartSong");
		out << (sonicver > 3 ? 3 : sonicver) << ", " << SMPS2ASM_VERSION << endl;

		// Now for voice pointer; this is the first piece of data in both
		// songs and SFX.
		int vocptr = IO::Read2(in);
		bool ext_vocs;
		// Also using a hack for null pointer here.
		bool uses_uvb = (sonicver >= 3 && vocptr == 0x17d8) || (vocptr == 0);
		int last_voc = -1;

		if (vocptr == 0) {
			// Null voice bank.
			ext_vocs = true;
			out << "\tsmpsHeaderVoiceNull" << endl;
			todo.push(LocTraits(vocptr, LocTraits::eExtVoices));
		} else if (uses_uvb) {
			// Universal voice bank in S3/SK/S3D.
			ext_vocs = true;
			out << "\tsmpsHeaderVoiceUVB" << endl;
			todo.push(LocTraits(vocptr, LocTraits::eExtVoices));
		} else {
			vocptr = IO::int2headerpointer(vocptr, offset);
			ext_vocs = (vocptr >= len) || (vocptr < 0);
			PrintMacro(out, "smpsHeaderVoice");
			out << projname << "_Voices" << endl;

			// An external voice bank will be printed as a note at the end;
			// the other type of voice bank will be extracted based on the
			// number of referenced voices.
			if (ext_vocs) {
				todo.push(LocTraits(vocptr, LocTraits::eExtVoices));
			} else {
				todo.push(LocTraits(vocptr, LocTraits::eVoices));
			}
		}

		// Music and SFX differ in the next few headers.
		if (sfx) {
			// Tempo dividing timing
			int tempodiv = Read1(in);
			// Output tempo header; it will deal with main tempo conversion.
			PrintMacro(out, "smpsHeaderTempoSFX");
			PrintHex2(out, tempodiv, true);
			out << endl;

			// Number of FM+PSG channels.
			int nchan = Read1(in);
			// Output channel setup header.
			PrintMacro(out, "smpsHeaderChanSFX");
			PrintHex2(out, nchan, true);
			out << endl << endl;

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
					cerr << "Original SFX playback control byte was ";
					PrintHex2(cerr, playctrl, true);
				}
				PrintMacro(out, "smpsHeaderSFXChannel");

				// Create track label and channel assignmend constant for header.
				string lbl = projname;
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
						cerr << "Invalid SFX channel ID." << endl;
						exit(6);
						break;
				}
				out << lbl << ",\t";
				PrintHex2(out, keydisp , false);
				PrintHex2(out, initvol , true);
				out << endl;

				// Add to queue/label list.
				if ((chanid & 0x80) != 0) {
					todo.push(LocTraits(trackptr, LocTraits::ePSGInit, keydisp));
				} else {
					todo.push(LocTraits(trackptr, LocTraits::eFMInit, keydisp));
				}
				labels.emplace(trackptr, lbl);
				tracklabels.insert(lbl);
			}
		} else {
			// Number of FM+DAC, PSG channels.
			int nfm = Read1(in), npsg = Read1(in);
			// Output channel setup header.
			PrintMacro(out, "smpsHeaderChan");
			PrintHex2(out, nfm , false);
			PrintHex2(out, npsg, true);
			out << endl;

			// Tempo dividing timing, main tempo modifier.
			int tempodiv = Read1(in), tempomod = Read1(in);
			// Output tempo header; it will deal with main tempo conversion.
			PrintMacro(out, "smpsHeaderTempo");
			PrintHex2(out, tempodiv, false);
			PrintHex2(out, tempomod, true);
			out << endl << endl;

			// First come the DAC and FM channels.
			for (int i = 0; i < nfm; i++) {
				int ptr = IO::read_header_pointer(in, offset),
				    keydisp = Read1(in), initvol = Read1(in);

				string lbl = projname;
				LocTraits::LocType type;
				if (i == 0) {
					// DAC is always first.
					lbl += "_DAC";
					labels.emplace(ptr, lbl);
					tracklabels.insert(lbl);
					PrintMacro(out, "smpsHeaderDAC");
					out << lbl;
					if ((keydisp != 0) || (initvol != 0)) {
						out << ",\t";
						PrintHex2(out, keydisp, false);
						PrintHex2(out, initvol, true);
					}
					out << endl;
					type = LocTraits::eDACInit;
				} else {
					// Now come FM channels.
					char c = i + '0';
					lbl += "_FM";
					lbl += c;
					labels.emplace(ptr, lbl);
					tracklabels.insert(lbl);
					PrintMacro(out, "smpsHeaderFM");
					out << lbl << ",\t";
					PrintHex2(out, keydisp, false);
					PrintHex2(out, initvol, true);
					out << endl;
					type = LocTraits::eFMInit;
				}

				// Add to queue.
				todo.push(LocTraits(ptr, type, keydisp));
			}

			// Time for PSG channels.
			for (int i = 0; i < npsg; i++) {
				int ptr     = IO::read_header_pointer(in, offset),
				    keydisp = Read1(in), initvol  = Read1(in),
				    modctrl = Read1(in), tone = Read1(in);

				string lbl = projname;
				char c = i + '1';
				lbl += "_PSG";
				lbl += c;
				labels.emplace(ptr, lbl);
				tracklabels.insert(lbl);
				PrintMacro(out, "smpsHeaderPSG");
				out << lbl << ",\t";
				PrintHex2(out, keydisp, false);
				PrintHex2(out, initvol, false);
				PrintHex2(out, modctrl, false);
				BaseNote::print_psg_tone(out, tone, sonicver, true);
				out << endl;

				// Add to queue.
				todo.push(LocTraits(ptr, LocTraits::ePSGInit, keydisp));
			}
		}

		// Mark all contents so far as having been explored.
		for (size_t i = startloc; i < size_t(in.tellg()); i++) {
			explored.emplace(i, LocTraits::eHeader);
		}

		while (todo.size() > 1) {
			LocTraits next_loc = todo.top();

			// If we are down to voices, we are done with this loop.
			if (next_loc.type >= LocTraits::eVoices) {
				break;
			}

			// Now remap the init types for simplicity, as they did their job.
			switch (next_loc.type) {
				case LocTraits::eDACInit:
					next_loc.type = LocTraits::eDACTrack;
					break;
				case LocTraits::ePCMInit:
					next_loc.type = LocTraits::ePCMTrack;
					break;
				case LocTraits::ePWMInit:
					next_loc.type = LocTraits::ePWMTrack;
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
			if (explored.find(next_loc.loc) != explored.end()) {
				continue;
			}

			if (next_loc.loc < 0 || next_loc.loc >= len) {
				auto it = labels.lower_bound(next_loc.loc),
				                                end = labels.upper_bound(next_loc.loc);

				if (it != end) {
					BaseNote::force_linebreak(out, true);
				}

				out << "; The following track data was present at " << dec
				    << next_loc.loc << " bytes from the start of the song."
				    << endl;
				for (; it != end; ++it) {
					string lbl = it->second;
					if (tracklabels.find(lbl) != tracklabels.end()) {
						out << "; " << lbl.substr(lbl.rfind('_') + 1) << " Data" << endl;
					}
					out << it->second << ":" << endl;
				}

				explored.emplace(next_loc.loc, next_loc.type);
				continue;
			}

			// Clear stream errors and reposition to correct location.
			in.clear();
			in.seekg(next_loc.loc);

			while (true) {
				int lastloc = in.tellg();
				shared_ptr<BaseNote> note(BaseNote::read<IO>(in, sonicver,
				                         offset, projname, next_loc.type,
				                         labels, last_voc, next_loc.keydisp));
				trackdata.emplace(lastloc, note);
				next_loc.keydisp = note->get_keydisp();

				// If the data includes a jump target, add it to queue.
				if (note->has_pointer()) {
					todo.push(LocTraits(note->get_pointer(), next_loc.type, next_loc.keydisp));
				}

				// Add in freshly explored data to list.
				for (int i = lastloc; i < in.tellg(); i++) {
					explored.emplace(i, next_loc.type);
				}

				// If we reach track end, or if we reached the end of file,
				// break from loop.
				if (note->ends_track() || !in.good()) {
					break;
				}
			}
		}

		// Grab the entry for voices.
		LocTraits voices = todo.top();
		todo.pop();

		// Read voices, unless universal voice bank, null voice pointer or
		// external voice bank.
		if (uses_uvb) {
			// Don't need to do anything, really.
		} else if (last_voc < 0) {
			// There are no voices in use; so we insert a comment saying so and
			// add a dummy label for the voice pointer.
			stringstream str(ios::in | ios::out);
			str << "; Song seems to not use any FM voices" << endl;
			str << projname << "_Voices";
			labels.emplace(voices.loc, str.str());
			auto note = make_shared<NullNote>();
			trackdata.emplace(voices.loc, note);
		} else if (ext_vocs || voices.type == LocTraits::eExtVoices) {
			// The voices were not in the file. Print a comment with their
			// location and a dummy label.
			stringstream str(ios::in | ios::out);
			str << "; Voices were not within the file: they are the first "
			    << (last_voc + 1) << " voices located at " << dec << voices.loc
			    << " bytes from the start of the song." << endl;
			str << "; The following label is a dummy label and should be moved to the correct location.";
			str << endl << projname << "_Voices";
			labels.emplace(voices.loc, str.str());
			labels.emplace(voices.loc, str.str());
			auto note = make_shared<NullNote>();
			trackdata.emplace(voices.loc, note);
		} else if (last_voc >= 0) {
			in.clear();
			in.seekg(voices.loc);
			labels.emplace(voices.loc, projname + "_Voices");

			// Read each voice in turn.
			for (int i = 0; i <= last_voc; i++) {
				if (in.tellg() + streamoff(25) > len) {
					// End of file reached in the middle of a voice.
					cerr << "Broken voice! The end-of-file was reached in the middle of an FM voice used by the song." << endl;
					break;
				}

				int lastloc = in.tellg();
				auto voc = make_shared<FMVoice>(in, sonicver, i);
				trackdata.emplace(lastloc, voc);

				// Add in freshly explored data to list.
				for (int j = lastloc; j < in.tellg(); j++) {
					explored.emplace(j, LocTraits::eVoices);
				}
			}
		}

		int lastlabel = -1;
		for (auto & elem : trackdata) {
			int off = elem.first;
			shared_ptr<BaseNote> note = elem.second;
			if (off > lastlabel) {
				auto it = labels.upper_bound(lastlabel),
				                                end = labels.upper_bound(off);

				if (it != end) {
					BaseNote::force_linebreak(out, true);
				}

				for (; it != end; ++it) {
					string lbl = it->second;
					if (tracklabels.find(lbl) != tracklabels.end()) {
						out << "; " << lbl.substr(lbl.rfind('_') + 1) << " Data" << endl;
					}
					out << it->second << ":" << endl;
				}

				lastlabel = off;
			}

			auto ty = explored.find(off);
			//assert(ty != explored.end());
			if (ty != explored.end()) {
				note->print(out, sonicver, ty->second, labels, s3kmode);
			}
		}

		/*
		if (uses_uvb)
		    // Universal voice bank or null voice pointer. Print nothing, read no voices.
		    return;
		else if (last_voc < 0)
		{
		    // There are no voices in use; so we insert a comment saying so and
		    // add a dummy label for the voice pointer.
		    out << endl << endl << "; Song seems to not use any FM voices" << endl;
		    out << projname << "_Voices:" << endl;
		    return;
		}
		else if (todo.empty())
		{
		    // This should be impossible.
		    out << endl << "; FM voices used by the song were not within the file; moreover,"
		        << endl << "; due to an unknown error, this software could not locate them." << endl;
		    return;
		}

		// Grab the entry for voices.
		LocTraits voices = todo.top();
		todo.pop();

		if (voices.type == LocTraits::eExtVoices)
		{
		    // The voices were not in the file. Print a comment with their
		    // location and a dummy label.
		    out << endl << endl << "; Voices were not within the file: they are the first "
		        << (last_voc + 1) << " voices located at " << dec << voices.loc
		        << " bytes from the start of the song." << endl;
		    out << "; The following label is a dummy label and should be moved to the correct location.";
		    out << endl << projname << "_Voices:" << endl;
		    return;
		}

		// Clear errors and reposition stream.
		in.clear();
		in.seekg(voices.loc);
		out << endl << endl << projname << "_Voices:" << endl;

		// Print each voice in turn.
		for (int i = 0; i <= last_voc; i++)
		{
		    if (in.tellg() + streamoff(25) > len)
		    {
		        // End of file reached in the middle of a voice.
		        cerr << "Broken voice! The end-of-file was reached in the middle of an FM voice used by the song." << endl;
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
	cerr << "Usage: smps2asm [-b|--bank {ptrtable} [-x|--extract [{pointer}]] [-u|--saxman] [-o|--offset {offsetval}] [-s|--sfx]" << endl
	     << "                {-v|--sonicver} {version} [-3|--s3kmode] {input_filename} {output_filename} {projname}" << endl;
	cerr << endl;
	cerr << "\t-s,--sfx     \tFile is SFX, not music." << endl;
	cerr << "\t-b,--bank    \tExtracts an entire z80 bank whose pointer table is at ptrtable." << endl
	     << "\t             \tThe pointer table must reside in the bank, and this option cannot" << endl
	     << "\t             \tbe combined with --extract or with --saxman." << endl
	     << "\t             \tSince this is for extracting z80 banks, it is not compatible with" << endl
	     << "\t             \t--sonicver 1. If missing, ptrtable is assumed to be zero." << endl;
	cerr << "\t-u,--saxman  \tAssume music file is Saxman-compressed. In most cases, this" << endl
	     << "\t             \tshould be combined with --offset 0x1380 --sonicver 2." << endl;
	cerr << "\t-x,--extract \tExtract from {pointer} address in file. This should never be" << endl
	     << "\t             \tcombined with --offset unless --saxman is also used." << endl;
	cerr << "\t-o,--offset  \tAssumes starting pointer for music/sfx within its sound bank" << endl
	     << "\t             \tis {offsetval}. Ignored if used with --sonicver 1." << endl;
	cerr << "\t-v,--sonicver\tSets Sonic version to {version}. This also sets underlying" << endl
	     << "\t             \tSMPS type. {version} can be '1' Sonic 1, '2' for Sonic 2 or" << endl
	     << "\t             \t'3' for Sonic 3, '4' for Sonic & Knuckles, or '5' for Sonic" << endl
	     << "\t             \t3D Blast." << endl;
	cerr << "\t-3,--s3kmode \tThis flag is valid for Sonic 1 and Sonic 2 only; this will" << endl
	     << "\t             \tcause all sequences of durations after a rest to be printed" << endl
	     << "\t             \twith the rests shown explicitly." << endl
	     << "\t             \tWARNING: A smpsCall is assumed to set the note to something." << endl
	     << "\t             \tother than a rest; this assumption turns out to be correct" << endl
	     << "\t             \tfor all Sonic 1 and Sonic 2 songs and SFX, but it could be" << endl
	     << "\t             \twrong for another game with a compatible SMPS implementation." << endl
	     << "\t             \tUse with care." << endl << endl;
}

void dump_single_entry
(
    istream &in, ostream &out, string const &projname,
    int pointer, int offset, int sonicver, bool saxman, bool sfx, bool s3kmode
) {
	if (pointer != 0) {
		if (saxman) {
			offset = -offset;
		} else {
			offset = pointer;
			if (sonicver != 1) {
				offset &= ~0x7fff;
			}
		}
	} else if (sonicver != 1) {
		offset = -offset;
	} else {
		offset = 0;
	}

	istream *src;
	stringstream sin(ios::in | ios::out | ios::binary);

	in.seekg(0);
	if (!saxman) {
		src = &in;
		in.seekg(pointer);
	} else {
		src = &sin;
		in.seekg(pointer);
		saxman::decode(in, sin, 0u);
		sin.seekg(0);
	}

	if (sonicver == 1) {
		DumpSmps<S1IO> smps(*src, out, sonicver, offset, projname, sfx, s3kmode);
		smps.dump_smps();
	} else {
		DumpSmps<SNIO> smps(*src, out, sonicver, offset, projname, sfx, s3kmode);
		smps.dump_smps();
	}
}

int main(int argc, char *argv[]) {
	static option long_options[] = {
		{"bank"    , optional_argument, nullptr, 'b'},
		{"extract" , optional_argument, nullptr, 'x'},
		{"saxman"  , no_argument      , nullptr, 'u'},
		{"offset"  , required_argument, nullptr, 'o'},
		{"sonicver", required_argument, nullptr, 'v'},
		{"sfx"     , no_argument      , nullptr, 's'},
		{"s3kmode" , no_argument      , nullptr, '3'},
		{nullptr, 0, nullptr, 0}
	};

	bool sfx = false, saxman = false, s3kmode = false, bankmode = false;
	int pointer = 0, offset = 0, ptrtable = 0, sonicver = -1;

	while (true) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "b::x::uo:v:s3",
		                    static_cast<option*>(long_options), &option_index);
		if (c == -1) {
			break;
		}

		switch (c) {
			case 'b':
				if (optarg != nullptr) {
					ptrtable = strtoul(optarg, nullptr, 0);
				}
				bankmode = true;
				break;

			case 'x':
				if (optarg != nullptr) {
					pointer = strtoul(optarg, nullptr, 0);
				}
				break;

			case 'u':
				saxman = true;
				break;

			case 'o':
				assert(optarg != nullptr);
				offset = strtoul(optarg, nullptr, 0);
				break;

			case 's':
				sfx = true;
				break;

			case 'v':
				assert(optarg != nullptr);
				sonicver = strtoul(optarg, nullptr, 0);
				break;

			case '3':
				s3kmode = true;
				break;
		}
	}

	if (argc - optind < 3 || sonicver < 1 || sonicver > 5
	        || (!saxman && pointer != 0 && offset != 0)
	        || (bankmode && (pointer != 0 || saxman || sonicver == 1))
	        || (s3kmode && sonicver > 2)) {
		usage();
		return 1;
	}


	ifstream fin(argv[optind + 0], ios::in | ios::binary);
	if (!fin.good()) {
		cerr << "Input file '" << argv[optind + 0] << "' could not be opened." << endl << endl;
		return 2;
	}

	ofstream fout(argv[optind + 1], ios::out | ios::binary);
	if (!fout.good()) {
		cerr << "Output file '" << argv[optind + 1] << "' could not be opened." << endl << endl;
		return 3;
	}

	string projname(argv[optind + 2]);

	if (bankmode) {
		fin.seekg(ptrtable);
		vector<int> ptrtable;
		int leastptr = 0xFFFF;
		ptrtable.reserve(128);

		while (fin.good() && fin.tellg() < (leastptr & 0x7FFF)) {
			int ptr = LittleEndian::Read2(fin);
			if (ptr < 0x8000) {
				break;
			} else if (ptr < leastptr) {
				leastptr = ptr;
			}
			ptrtable.push_back(ptr);
		}

		char const *fmt;
		if (ptrtable.size() < 256) {
			fmt = "%02X";
		} else {
			fmt = "%04X";
		}
		char buf[10];

		for (unsigned ii = 0; ii < ptrtable.size(); ii++) {
			snprintf(buf, sizeof(buf), fmt, ii);
			dump_single_entry(fin, fout, projname + buf, ptrtable[ii] & 0x7FFF,
			                  0, sonicver, saxman, sfx, s3kmode);
			if (ii + 1 < ptrtable.size()) {
				fout << endl;
			}
		}
	} else {
		dump_single_entry(fin, fout, projname, pointer, offset, sonicver,
		                  saxman, sfx, s3kmode);
	}
}
