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

#include <getopt.h>
#include <mdcomp/bigendian_io.hh>
#include <mdcomp/saxman.hh>
#include <mdtools/fmvoice.hh>
#include <mdtools/ignore_unused_variable_warning.hh>
#include <mdtools/songtrack.hh>

#ifdef __GNUG__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#    pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif
#define FMT_HEADER_ONLY 1
#include <fmt/format.h>
#ifdef __GNUG__
#    pragma GCC diagnostic pop
#endif

#include <array>
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

#define SMPS2ASM_VERSION 1

using std::cerr;
using std::dec;
using std::endl;
using std::ifstream;
using std::ios;
using std::istream;
using std::make_shared;
using std::map;
using std::multimap;
using std::ofstream;
using std::ostream;
using std::priority_queue;
using std::set;
using std::shared_ptr;
using std::streamoff;
using std::string;
using std::stringstream;
using std::vector;

struct S1IO {
    template <typename T>
    static inline size_t Read2(T& input) {
        return BigEndian::Read2(input);
    }

    template <typename T>
    static inline size_t Read4(T& input) {
        return BigEndian::Read4(input);
    }

    template <typename T>
    static inline void Write2(T& output, size_t value) {
        BigEndian::Write2(output, value);
    }

    template <typename T>
    static inline void Write4(T& output, size_t value) {
        BigEndian::Write4(output, value);
    }

    template <typename T>
    static inline int read_header_pointer(T& input, int base) {
        return static_cast<uint16_t>(Read2(input)) + base;
    }

    template <typename T>
    static inline int read_pointer(T& input, int base) {
        ignore_unused_variable_warning(base);
        int pointer = int(input.tellg()) + 1;
        return static_cast<int16_t>(Read2(input)) + pointer;
    }

    static inline int int2header_pointer(int value, int base) {
        ignore_unused_variable_warning(base);
        return static_cast<uint16_t>(value);
    }
};

struct SNIO {
    template <typename T>
    static inline size_t Read2(T& input) {
        return LittleEndian::Read2(input);
    }

    template <typename T>
    static inline size_t Read4(T& input) {
        return LittleEndian::Read4(input);
    }

    template <typename T>
    static inline void Write2(T& output, size_t value) {
        LittleEndian::Write2(output, value);
    }

    template <typename T>
    static inline void Write4(T& output, size_t value) {
        LittleEndian::Write4(output, value);
    }

    template <typename T>
    static inline int read_header_pointer(T& input, int base) {
        return static_cast<uint16_t>(Read2(input) % 0x8000) + base;
    }

    template <typename T>
    static inline int read_pointer(T& input, int base) {
        return static_cast<uint16_t>(Read2(input) % 0x8000) + base;
    }

    static inline int int2header_pointer(int value, int base) {
        return static_cast<uint16_t>(value % 0x8000) + base;
    }
};

static inline string make_label_code(int code) {
    return fmt::format("{:02X}", code);
}

static inline string need_call_label() {
    static int call_label = 0;
    string     label("_Call");
    label += make_label_code(call_label++);
    return label;
}

static inline string need_jump_label() {
    static int jump_label = 0;
    string     label("_Jump");
    label += make_label_code(jump_label++);
    return label;
}

static inline string need_loop_label() {
    static int loop_label = 0;
    string     label("_Loop");
    label += make_label_code(loop_label++);
    return label;
}

template <typename IO>
BaseNote* BaseNote::read(
        istream& input, int sonic_version, int offset,
        string const& project_name, LocTraits::LocType track_type,
        multimap<int, string>& labels, int& last_voc,
        uint8_t key_displacement) {
    uint8_t byte = Read1(input);
    // Initialize to invalid 32-bit address.
    if (byte < 0x80) {
        return new Duration(byte, key_displacement);
    }
    if (byte < 0xe0) {
        switch (track_type) {
        case LocTraits::eFMInit:
        case LocTraits::eFMTrack:
        case LocTraits::ePSGInit:
        case LocTraits::ePSGTrack:
            return new FMPSGNote(byte, key_displacement);
        case LocTraits::eDACInit:
        case LocTraits::eDACTrack:
        case LocTraits::ePCMInit:
        case LocTraits::ePCMTrack:
        case LocTraits::ePWMInit:
        case LocTraits::ePWMTrack:
            return new DACNote(byte, key_displacement);
        case LocTraits::eHeader:
        case LocTraits::eVoices:
        case LocTraits::eExtVoices:
            __builtin_unreachable();
        }
    }

    if (sonic_version >= 3) {
        switch (byte) {
        case 0xff: {    // Meta
            uint8_t spec = Read1(input);
            switch (spec) {
            case 0x02:
            case 0x07:
                return new CoordFlag1ParamByte<false>(
                        byte, key_displacement, spec);
            case 0x00:
            case 0x01:
            case 0x04:
                return new CoordFlag2ParamBytes<false>(
                        byte, key_displacement, spec, Read1(input));
            case 0x06:
                return new CoordFlag3ParamBytes<false>(
                        byte, key_displacement, spec, Read1(input),
                        Read1(input));
            case 0x05:
                return new CoordFlag5ParamBytes<false>(
                        byte, key_displacement, spec, Read1(input),
                        Read1(input), Read1(input), Read1(input));
            case 0x03: {
                int     pointer = IO::read_pointer(input, offset);
                uint8_t count   = Read1(input);
                return new CoordFlagPointer2ParamBytes<false>(
                        byte, key_displacement, spec, count, pointer);
            }
            default:
                cerr << "Invalid meta-coordination flag '";
                PrintHex2(cerr, 0xff, true);
                PrintHex2(cerr, byte, false);
                cerr << "' found at offset " << size_t(input.tellg()) - 2
                     << "; it will be ignored." << endl;
                return new NullNote();
            }
        }
        case 0xf7: {    // Loop
            uint8_t index   = Read1(input);
            uint8_t repeats = Read1(input);
            int     pointer = IO::read_pointer(input, offset);
            auto    found   = labels.find(pointer);
            if (found == labels.end()) {
                labels.emplace(pointer, project_name + need_loop_label());
            }
            return new CoordFlagPointer2ParamBytes<false>(
                    byte, key_displacement, index, repeats, pointer);
        }
        case 0xf0:      // Start modulation
        case 0xfe: {    // FM3 Special mode
            uint8_t param1 = Read1(input);
            uint8_t param2 = Read1(input);
            uint8_t param3 = Read1(input);
            uint8_t param4 = Read1(input);
            return new CoordFlag4ParamBytes<false>(
                    byte, key_displacement, param1, param2, param3, param4);
        }
        case 0xf6:      // Jump
        case 0xf8:      // Call
        case 0xfc: {    // Continuous loop
            int  pointer = IO::read_pointer(input, offset);
            auto found   = labels.find(pointer);
            if (found == labels.end()) {
                string label;
                if (byte == 0xf6) {
                    label = need_jump_label();
                } else if (byte == 0xf8) {
                    label = need_call_label();
                } else {
                    label = need_loop_label();
                }
                labels.emplace(pointer, project_name + label);
            }
            if (byte == 0xf6) {
                return new CoordFlagPointerParam<true>(
                        byte, key_displacement, pointer);
            }
            return new CoordFlagPointerParam<false>(
                    byte, key_displacement, pointer);
        }
        case 0xeb: {    // Conditional jump
            uint8_t index   = Read1(input);
            int     pointer = IO::read_pointer(input, offset);
            auto    found   = labels.find(pointer);
            if (found == labels.end()) {
                labels.emplace(pointer, project_name + need_jump_label());
            }

            return new CoordFlagPointer1ParamByte<false>(
                    byte, key_displacement, index, pointer);
        }

        case 0xe5:
        case 0xee:
        case 0xf1:
            return new CoordFlag2ParamBytes<false>(
                    byte, key_displacement, Read1(input), Read1(input));

        case 0xef: {
            int8_t voice = Read1(input);
            if (track_type == LocTraits::eFMTrack && voice >= 0
                && voice > last_voc) {
                last_voc = static_cast<uint8_t>(voice);
            }
            if (voice < 0) {
                uint8_t value = Read1(input) - 0x81;
                if (track_type == LocTraits::eFMTrack) {
                    voice %= 0x80;
                }
                return new CoordFlag2ParamBytes<false>(
                        byte, key_displacement, voice, value);
            }
            return new CoordFlag1ParamByte<false>(
                    byte, key_displacement, voice);
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
            return new CoordFlag1ParamByte<false>(
                    byte, key_displacement, Read1(input));

        case 0xfb:
            return new CoordFlagChangeKeyDisplacement(
                    byte, key_displacement, Read1(input));

        case 0xe2: {    // Fade to previous
            uint8_t value = Read1(input);
            if (value == 0xff) {
                return new CoordFlagNoParams<false>(byte, key_displacement);
            }
            return new CoordFlag1ParamByte<false>(
                    byte, key_displacement, value);
        }

        case 0xe3:
        case 0xf2:
        case 0xf9:
            return new CoordFlagNoParams<true>(byte, key_displacement);

        default:
            return new CoordFlagNoParams<false>(byte, key_displacement);
        }
    } else {
        switch (byte) {
        case 0xf7: {    // Loop
            uint8_t index   = Read1(input);
            uint8_t repeats = Read1(input);
            int     pointer = IO::read_pointer(input, offset);
            auto    found   = labels.find(pointer);
            if (found == labels.end()) {
                labels.emplace(pointer, project_name + need_loop_label());
            }
            return new CoordFlagPointer2ParamBytes<false>(
                    byte, key_displacement, index, repeats, pointer);
        }
        case 0xf0: {    // Start modulation
            uint8_t wait   = Read1(input);
            uint8_t speed  = Read1(input);
            uint8_t change = Read1(input);
            uint8_t steps  = Read1(input);
            return new CoordFlag4ParamBytes<false>(
                    byte, key_displacement, wait, speed, change, steps);
        }
        case 0xf6:      // Jump
        case 0xf8: {    // Call
            int  pointer = IO::read_pointer(input, offset);
            auto found   = labels.find(pointer);
            if (found == labels.end()) {
                labels.emplace(
                        pointer, project_name
                                         + (byte == 0xf6 ? need_jump_label()
                                                         : need_call_label()));
            }
            if (byte == 0xf6) {
                return new CoordFlagPointerParam<true>(
                        byte, key_displacement, pointer);
            }
            return new CoordFlagPointerParam<false>(
                    byte, key_displacement, pointer);
        }
        case 0xed:    // S1: clear push flag; S2: eat byte
            if (sonic_version == 1) {
                return new CoordFlagNoParams<false>(byte, key_displacement);
            } else {
                return new CoordFlag1ParamByte<false>(
                        byte, key_displacement, Read1(input));
            }
        case 0xee:    // S1: Stop special FM4; S2: nop
            if (sonic_version == 1) {
                return new CoordFlagNoParams<true>(byte, key_displacement);
            } else {
                return new CoordFlagNoParams<false>(byte, key_displacement);
            }
        case 0xef: {    // Set FM voice
            int voice = Read1(input);
            if (voice > last_voc) {
                last_voc = voice;
            }
            return new CoordFlag1ParamByte<false>(
                    byte, key_displacement, voice);
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
            return new CoordFlag1ParamByte<false>(
                    byte, key_displacement, Read1(input));

        case 0xe9:
            return new CoordFlagChangeKeyDisplacement(
                    byte, key_displacement, Read1(input));

        case 0xe3:
        case 0xe4:
        case 0xf2:
            return new CoordFlagNoParams<true>(byte, key_displacement);

        default:
            return new CoordFlagNoParams<false>(byte, key_displacement);
        }
    }
}

template <typename IO>
class DumpSmps {
    istream&     input;
    ostream&     output;
    string const project_name;
    int          sonic_version, start_position, offset, length;
    bool         is_sfx, s3kmode;

public:
    DumpSmps(
            istream& input_, ostream& output_, int sonic_version_, int offset_,
            string name, bool is_sfx_, bool s3km)
            : input(input_), output(output_), project_name(std::move(name)),
              sonic_version(sonic_version_), offset(offset_), is_sfx(is_sfx_),
              s3kmode(s3km) {
        start_position = input.tellg();
        input.seekg(0, ios::end);
        length = input.tellg();
        input.seekg(start_position);
    }
    void dump_smps() {
        // Set up data structures for exploratory disassembly.
        priority_queue<LocTraits>      todo;
        map<int, LocTraits::LocType>   explored;
        map<int, shared_ptr<BaseNote>> track_data;

        // This will hold the labels of each location.
        multimap<int, string> labels;
        set<string>           track_labels;

        // Start with music conversion header.
        output << project_name << "_Header:" << endl;
        PrintMacro(output, "smpsHeaderStartSong");
        output << (sonic_version > 3 ? 3 : sonic_version) << ", "
               << SMPS2ASM_VERSION << endl;

        // Now for voice pointer; this is the first piece of data in both
        // songs and SFX.
        int  vocptr = IO::Read2(input);
        bool ext_vocs;
        // Also using a hack for null pointer here.
        bool uses_uvb
                = (sonic_version >= 3 && vocptr == 0x17d8) || (vocptr == 0);
        int last_voice = -1;

        if (vocptr == 0) {
            // Null voice bank.
            ext_vocs = true;
            output << "\tsmpsHeaderVoiceNull" << endl;
            todo.push(LocTraits(vocptr, LocTraits::eExtVoices));
        } else if (uses_uvb) {
            // Universal voice bank in S3/SK/S3D.
            ext_vocs = true;
            output << "\tsmpsHeaderVoiceUVB" << endl;
            todo.push(LocTraits(vocptr, LocTraits::eExtVoices));
        } else {
            vocptr   = IO::int2header_pointer(vocptr, offset);
            ext_vocs = (vocptr >= length) || (vocptr < 0);
            PrintMacro(output, "smpsHeaderVoice");
            output << project_name << "_Voices" << endl;

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
        if (is_sfx) {
            // Tempo dividing timing
            int tempo_div = Read1(input);
            // Output tempo header; it will deal with main tempo conversion.
            PrintMacro(output, "smpsHeaderTempoSFX");
            PrintHex2(output, tempo_div, true);
            output << endl;

            // Number of FM+PSG channels.
            int nchan = Read1(input);
            // Output channel setup header.
            PrintMacro(output, "smpsHeaderChanSFX");
            PrintHex2(output, nchan, true);
            output << endl << endl;

            // Time to output headers for all tracks then queue up their data
            // for exploration.
            for (int i = 0; i < nchan; i++) {
                int      playback_control = Read1(input);
                uint32_t channel          = Read1(input);
                int      track_pointer = IO::read_header_pointer(input, offset);
                int      key_displacement = Read1(input);
                int      initial_volume   = Read1(input);

                // Special case for non-ordinary playback control bytes.
                if (playback_control != 0x80) {
                    output << ";\tOriginal SFX playback control byte was ";
                    PrintHex2(output, playback_control, true);
                    cerr << "Original SFX playback control byte was ";
                    PrintHex2(cerr, playback_control, true);
                }
                PrintMacro(output, "smpsHeaderSFXChannel");

                // Create track label and channel assignment constant for
                // header.
                string label = project_name;
                switch (channel) {
                case 0x80:
                case 0xA0:
                case 0xC0: {
                    auto value = static_cast<char>(
                            ((channel & 0x60U) >> 5U) + '1');
                    output << "cPSG" << value << ", ";
                    label += "_PSG";
                    label += value;
                    break;
                }

                case 0xE0:
                    output << "cNoise, ";
                    label += "_Noise";
                    break;

                case 0x00:
                case 0x01:
                case 0x02:
                    channel++;
                    [[fallthrough]];
                case 0x04:
                case 0x05:
                case 0x06: {
                    auto value = static_cast<char>((channel & 0x07U) + '0');
                    output << "cFM" << value << ", ";
                    label += "_FM";
                    label += value;
                    break;
                }

                default:
                    PrintHex2(output, channel, false);
                    cerr << "Invalid SFX channel ID." << endl;
                    exit(6);
                    break;
                }
                output << label << ",\t";
                PrintHex2(output, key_displacement, false);
                PrintHex2(output, initial_volume, true);
                output << endl;

                // Add to queue/label list.
                if ((channel & 0x80U) != 0) {
                    todo.push(LocTraits(
                            track_pointer, LocTraits::ePSGInit,
                            key_displacement));
                } else {
                    todo.push(LocTraits(
                            track_pointer, LocTraits::eFMInit,
                            key_displacement));
                }
                labels.emplace(track_pointer, label);
                track_labels.insert(label);
            }
        } else {
            // Number of FM+DAC, PSG channels.
            int num_fm  = Read1(input);
            int num_psg = Read1(input);
            // Output channel setup header.
            PrintMacro(output, "smpsHeaderChan");
            PrintHex2(output, num_fm, false);
            PrintHex2(output, num_psg, true);
            output << endl;

            // Tempo dividing timing, main tempo modifier.
            int tempo_div = Read1(input);
            int tempo_mod = Read1(input);
            // Output tempo header; it will deal with main tempo conversion.
            PrintMacro(output, "smpsHeaderTempo");
            PrintHex2(output, tempo_div, false);
            PrintHex2(output, tempo_mod, true);
            output << endl << endl;

            // First come the DAC and FM channels.
            for (int i = 0; i < num_fm; i++) {
                int pointer          = IO::read_header_pointer(input, offset);
                int key_displacement = Read1(input);
                int initial_volume   = Read1(input);

                string             label = project_name;
                LocTraits::LocType type;
                if (i == 0) {
                    // DAC is always first.
                    label += "_DAC";
                    labels.emplace(pointer, label);
                    track_labels.insert(label);
                    PrintMacro(output, "smpsHeaderDAC");
                    output << label;
                    if ((key_displacement != 0) || (initial_volume != 0)) {
                        output << ",\t";
                        PrintHex2(output, key_displacement, false);
                        PrintHex2(output, initial_volume, true);
                    }
                    output << endl;
                    type = LocTraits::eDACInit;
                } else {
                    // Now come FM channels.
                    auto value = static_cast<char>(i + '0');
                    label += "_FM";
                    label += value;
                    labels.emplace(pointer, label);
                    track_labels.insert(label);
                    PrintMacro(output, "smpsHeaderFM");
                    output << label << ",\t";
                    PrintHex2(output, key_displacement, false);
                    PrintHex2(output, initial_volume, true);
                    output << endl;
                    type = LocTraits::eFMInit;
                }

                // Add to queue.
                todo.push(LocTraits(pointer, type, key_displacement));
            }

            // Time for PSG channels.
            for (int i = 0; i < num_psg; i++) {
                int    pointer = IO::read_header_pointer(input, offset);
                int    key_displacement   = Read1(input);
                int    initial_volume     = Read1(input);
                int    modulation_control = Read1(input);
                int    tone               = Read1(input);
                string label              = project_name;
                auto   value              = static_cast<char>(i + '1');
                label += "_PSG";
                label += value;
                labels.emplace(pointer, label);
                track_labels.insert(label);
                PrintMacro(output, "smpsHeaderPSG");
                output << label << ",\t";
                PrintHex2(output, key_displacement, false);
                PrintHex2(output, initial_volume, false);
                PrintHex2(output, modulation_control, false);
                BaseNote::print_psg_tone(output, tone, sonic_version, true);
                output << endl;

                // Add to queue.
                todo.push(LocTraits(
                        pointer, LocTraits::ePSGInit, key_displacement));
            }
        }

        // Mark all contents so far as having been explored.
        for (size_t i = start_position; i < size_t(input.tellg()); i++) {
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
            case LocTraits::eDACTrack:
            case LocTraits::eHeader:
            case LocTraits::eFMTrack:
            case LocTraits::ePCMTrack:
            case LocTraits::ePSGTrack:
            case LocTraits::ePWMTrack:
                break;
            case LocTraits::eVoices:
            case LocTraits::eExtVoices:
                __builtin_unreachable();
            }

            todo.pop();

            // Don't explore again what has been done already.
            if (explored.find(next_loc.location) != explored.end()) {
                continue;
            }

            if (next_loc.location < 0 || next_loc.location >= length) {
                auto lower = labels.lower_bound(next_loc.location);
                auto upper = labels.upper_bound(next_loc.location);

                if (lower != upper) {
                    BaseNote::force_line_break(output, true);
                }

                output << "; The following track data was present at " << dec
                       << next_loc.location
                       << " bytes from the start of the song." << endl;
                for (; lower != upper; ++lower) {
                    string label = lower->second;
                    if (track_labels.find(label) != track_labels.end()) {
                        output << "; " << label.substr(label.rfind('_') + 1)
                               << " Data" << endl;
                    }
                    output << lower->second << ":" << endl;
                }

                explored.emplace(next_loc.location, next_loc.type);
                continue;
            }

            // Clear stream errors and reposition to correct location.
            input.clear();
            input.seekg(next_loc.location);

            while (true) {
                int                  last_loc = input.tellg();
                shared_ptr<BaseNote> note(BaseNote::read<IO>(
                        input, sonic_version, offset, project_name,
                        next_loc.type, labels, last_voice,
                        next_loc.key_displacement));
                track_data.emplace(last_loc, note);
                next_loc.key_displacement = note->get_key_displacement();

                // If the data includes a jump target, add it to queue.
                if (note->has_pointer()) {
                    todo.push(LocTraits(
                            note->get_pointer(), next_loc.type,
                            next_loc.key_displacement));
                }

                // Add in freshly explored data to list.
                for (int i = last_loc; i < input.tellg(); i++) {
                    explored.emplace(i, next_loc.type);
                }

                // If we reach track end, or if we reached the end of file,
                // break from loop.
                if (note->ends_track() || !input.good()) {
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
        } else if (last_voice < 0) {
            // There are no voices in use; so we insert a comment saying so and
            // add a dummy label for the voice pointer.
            stringstream message(ios::in | ios::out);
            message << "; Song seems to not use any FM voices" << endl;
            message << project_name << "_Voices";
            labels.emplace(voices.location, message.str());
            auto note = make_shared<NullNote>();
            track_data.emplace(voices.location, note);
        } else if (ext_vocs || voices.type == LocTraits::eExtVoices) {
            // The voices were not in the file. Print a comment with their
            // location and a dummy label.
            stringstream message(ios::in | ios::out);
            message << "; Voices were not within the file: they are the first "
                    << (last_voice + 1) << " voices located at " << dec
                    << voices.location << " bytes from the start of the song."
                    << endl;
            message << "; The following label is a dummy label and should be "
                       "moved "
                       "to the correct location.";
            message << endl << project_name << "_Voices";
            labels.emplace(voices.location, message.str());
            labels.emplace(voices.location, message.str());
            auto note = make_shared<NullNote>();
            track_data.emplace(voices.location, note);
        } else if (last_voice >= 0) {
            input.clear();
            input.seekg(voices.location);
            labels.emplace(voices.location, project_name + "_Voices");

            // Read each voice in turn.
            for (int i = 0; i <= last_voice; i++) {
                if (input.tellg() + streamoff(25) > length) {
                    // End of file reached in the middle of a voice.
                    cerr << "Broken voice! The end-of-file was reached in the "
                            "middle of an FM voice used by the song."
                         << endl;
                    break;
                }

                int  last_location = input.tellg();
                auto voice = make_shared<FMVoice>(input, sonic_version, i);
                track_data.emplace(last_location, voice);

                // Add in freshly explored data to list.
                for (int j = last_location; j < input.tellg(); j++) {
                    explored.emplace(j, LocTraits::eVoices);
                }
            }
        }

        int last_label = -1;
        for (auto& elem : track_data) {
            int                  offset = elem.first;
            shared_ptr<BaseNote> note   = elem.second;
            if (offset > last_label) {
                auto lower = labels.upper_bound(last_label);
                auto upper = labels.upper_bound(offset);

                if (lower != upper) {
                    BaseNote::force_line_break(output, true);
                }

                for (; lower != upper; ++lower) {
                    string label = lower->second;
                    if (track_labels.find(label) != track_labels.end()) {
                        output << "; " << label.substr(label.rfind('_') + 1)
                               << " Data" << endl;
                    }
                    output << lower->second << ":" << endl;
                }

                last_label = offset;
            }

            auto found = explored.find(offset);
            // assert(ty != explored.end());
            if (found != explored.end()) {
                note->print(
                        output, sonic_version, found->second, labels, s3kmode);
            }
        }

        /*
        if (uses_uvb)
            // Universal voice bank or null voice pointer. Print nothing, read
        no voices. return; else if (last_voc < 0)
        {
            // There are no voices in use; so we insert a comment saying so and
            // add a dummy label for the voice pointer.
            out << endl << endl << "; Song seems to not use any FM voices" <<
        endl; out << project_name << "_Voices:" << endl; return;
        }
        else if (todo.empty())
        {
            // This should be impossible.
            out << endl << "; FM voices used by the song were not within the
        file; moreover,"
                << endl << "; due to an unknown error, this software could not
        locate them." << endl; return;
        }

        // Grab the entry for voices.
        LocTraits voices = todo.top();
        todo.pop();

        if (voices.type == LocTraits::eExtVoices)
        {
            // The voices were not in the file. Print a comment with their
            // location and a dummy label.
            out << endl << endl << "; Voices were not within the file: they are
        the first "
                << (last_voc + 1) << " voices located at " << dec << voices.loc
                << " bytes from the start of the song." << endl;
            out << "; The following label is a dummy label and should be moved
        to the correct location."; out << endl << project_name << "_Voices:" <<
        endl; return;
        }

        // Clear errors and reposition stream.
        in.clear();
        in.seekg(voices.loc);
        out << endl << endl << project_name << "_Voices:" << endl;

        // Print each voice in turn.
        for (int i = 0; i <= last_voc; i++)
        {
            if (in.tellg() + streamoff(25) > len)
            {
                // End of file reached in the middle of a voice.
                cerr << "Broken voice! The end-of-file was reached in the middle
        of an FM voice used by the song." << endl; break;
            }

            // Print the voice.
            fm_voice voc;
            voc.read(in, sonic_version);
            voc.print(out, sonic_version, i);
        }
        */
    }
};

static void usage() {
    cerr << "Usage: smps2asm [-b|--bank {ptrtable} [-x|--extract [{pointer}]] "
            "[-u|--saxman] [-o|--offset {offsetval}] [-s|--sfx]"
         << endl
         << "                {-v|--sonicver} {version} [-3|--s3kmode] "
            "{input_filename} {output_filename} {project_name}"
         << endl;
    cerr << endl;
    cerr << "\t-s,--sfx     \tFile is SFX, not music." << endl;
    cerr << "\t-b,--bank    \tExtracts an entire z80 bank whose pointer table "
            "is at ptrtable."
         << endl
         << "\t             \tThe pointer table must reside in the bank, and "
            "this option cannot"
         << endl
         << "\t             \tbe combined with --extract or with --saxman."
         << endl
         << "\t             \tSince this is for extracting z80 banks, it is "
            "not compatible with"
         << endl
         << "\t             \t--sonicver 1. If missing, ptrtable is assumed to "
            "be zero."
         << endl;
    cerr << "\t-u,--saxman  \tAssume music file is Saxman-compressed. In most "
            "cases, this"
         << endl
         << "\t             \tshould be combined with --offset 0x1380 "
            "--sonicver 2."
         << endl;
    cerr << "\t-x,--extract \tExtract from {pointer} address in file. This "
            "should never be"
         << endl
         << "\t             \tcombined with --offset unless --saxman is also "
            "used."
         << endl;
    cerr << "\t-o,--offset  \tAssumes starting pointer for music/sfx within "
            "its "
            "sound bank"
         << endl
         << "\t             \tis {offsetval}. Ignored if used with --sonicver "
            "1."
         << endl;
    cerr << "\t-v,--sonicver\tSets Sonic version to {version}. This also sets "
            "underlying"
         << endl
         << "\t             \tSMPS type. {version} can be '1' Sonic 1, '2' for "
            "Sonic 2 or"
         << endl
         << "\t             \t'3' for Sonic 3, '4' for Sonic & Knuckles, or "
            "'5' for Sonic"
         << endl
         << "\t             \t3D Blast." << endl;
    cerr << "\t-3,--s3kmode \tThis flag is valid for Sonic 1 and Sonic 2 only; "
            "this will"
         << endl
         << "\t             \tcause all sequences of durations after a rest to "
            "be printed"
         << endl
         << "\t             \twith the rests shown explicitly." << endl
         << "\t             \tWARNING: A smpsCall is assumed to set the note "
            "to something."
         << endl
         << "\t             \tother than a rest; this assumption turns out to "
            "be correct"
         << endl
         << "\t             \tfor all Sonic 1 and Sonic 2 songs and SFX, but "
            "it could be"
         << endl
         << "\t             \twrong for another game with a compatible SMPS "
            "implementation."
         << endl
         << "\t             \tUse with care." << endl
         << endl;
}

void dump_single_entry(
        istream& input, ostream& output, string const& project_name,
        int pointer, int offset, int sonicver, bool saxman, bool is_sfx,
        bool s3kmode) {
    if (pointer != 0) {
        if (saxman) {
            offset = -offset;
        } else {
            offset = pointer;
            if (sonicver != 1) {
                offset -= (offset % 0x8000);
            }
        }
    } else if (sonicver != 1) {
        offset = -offset;
    } else {
        offset = 0;
    }

    istream*     source;
    stringstream input_buffer(ios::in | ios::out | ios::binary);

    input.seekg(0);
    if (!saxman) {
        source = &input;
        input.seekg(pointer);
    } else {
        source = &input_buffer;
        input.seekg(pointer);
        saxman::decode(input, input_buffer, 0U);
        input_buffer.seekg(0);
    }

    if (sonicver == 1) {
        DumpSmps<S1IO> smps(
                *source, output, sonicver, offset, project_name, is_sfx,
                s3kmode);
        smps.dump_smps();
    } else {
        DumpSmps<SNIO> smps(
                *source, output, sonicver, offset, project_name, is_sfx,
                s3kmode);
        smps.dump_smps();
    }
}

int main(int argc, char* argv[]) {
    constexpr static const std::array long_options{
            option{"bank", optional_argument, nullptr, 'b'},
            option{"extract", optional_argument, nullptr, 'x'},
            option{"saxman", no_argument, nullptr, 'u'},
            option{"offset", required_argument, nullptr, 'o'},
            option{"sonicver", required_argument, nullptr, 'v'},
            option{"sfx", no_argument, nullptr, 's'},
            option{"s3kmode", no_argument, nullptr, '3'},
            option{nullptr, 0, nullptr, 0}};

    bool    is_sfx          = false;
    bool    saxman          = false;
    bool    s3kmode         = false;
    bool    bank_mode       = false;
    int64_t extract_pointer = 0;
    int64_t offset          = 0;
    int64_t pointer_table   = 0;
    int64_t sonic_version   = -1;

    while (true) {
        int option_index = 0;
        int option_char  = getopt_long(
                 argc, argv, "b::x::uo:v:s3", long_options.data(),
                 &option_index);
        if (option_char == -1) {
            break;
        }

        switch (option_char) {
        case 'b':
            if (optarg != nullptr) {
                pointer_table = strtol(optarg, nullptr, 0);
            }
            bank_mode = true;
            break;

        case 'x':
            if (optarg != nullptr) {
                extract_pointer = strtol(optarg, nullptr, 0);
            }
            break;

        case 'u':
            saxman = true;
            break;

        case 'o':
            assert(optarg != nullptr);
            offset = strtol(optarg, nullptr, 0);
            break;

        case 's':
            is_sfx = true;
            break;

        case 'v':
            assert(optarg != nullptr);
            sonic_version = strtol(optarg, nullptr, 0);
            break;

        case '3':
            s3kmode = true;
            break;
        default:
            break;
        }
    }

    if (argc - optind < 3 || sonic_version < 1 || sonic_version > 5
        || (!saxman && extract_pointer != 0 && offset != 0)
        || (bank_mode && (extract_pointer != 0 || saxman || sonic_version == 1))
        || (s3kmode && sonic_version > 2)) {
        usage();
        return 1;
    }

    ifstream input(argv[optind + 0], ios::in | ios::binary);
    if (!input.good()) {
        cerr << "Input file '" << argv[optind + 0] << "' could not be opened."
             << endl
             << endl;
        return 2;
    }

    ofstream output(argv[optind + 1], ios::out | ios::binary);
    if (!output.good()) {
        cerr << "Output file '" << argv[optind + 1] << "' could not be opened."
             << endl
             << endl;
        return 3;
    }

    string project_name(argv[optind + 2]);

    if (bank_mode) {
        input.seekg(pointer_table);
        vector<int> pointerTable;
        int         least_pointer = 0xFFFF;
        pointerTable.reserve(128);

        while (input.good() && input.tellg() < (least_pointer % 0x8000)) {
            int pointer = LittleEndian::Read2(input);
            if (pointer < 0x8000) {
                break;
            }
            if (pointer < least_pointer) {
                least_pointer = pointer;
            }
            pointerTable.push_back(pointer);
        }

        int const width = (pointerTable.size() < 256) ? 2 : 4;
        for (size_t ii = 0; ii < pointerTable.size(); ii++) {
            string buffer = fmt::format("{}{:0{}X}", project_name, ii, width);
            dump_single_entry(
                    input, output, buffer, pointerTable[ii] % 0x8000, 0,
                    sonic_version, saxman, is_sfx, s3kmode);
            if (ii + 1 < pointerTable.size()) {
                output << endl;
            }
        }
    } else {
        dump_single_entry(
                input, output, project_name, extract_pointer, offset,
                sonic_version, saxman, is_sfx, s3kmode);
    }
}
