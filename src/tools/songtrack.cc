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

#include <boost/io/ios_state.hpp>
#include <mdtools/songtrack.hh>

#include <array>
#include <iomanip>
#include <iostream>
#include <istream>
#include <ostream>
#include <vector>

#ifdef __GNUG__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#    pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif
#define FMT_HEADER_ONLY  1
#define FMT_STRING_ALIAS 1
#include <fmt/format.h>
#ifdef __GNUG__
#    pragma GCC diagnostic pop
#endif

#include <mdcomp/bigendian_io.hh>

using std::cerr;
using std::endl;
using std::hex;
using std::ios;
using std::istream;
using std::multimap;
using std::nouppercase;
using std::ostream;
using std::setfill;
using std::setw;
using std::string;
using std::uppercase;
using std::vector;

size_t          BaseNote::notesprinted = 0;
BaseNote const* BaseNote::last_note    = nullptr;
bool            BaseNote::need_rest    = false;

void BaseNote::force_linebreak(ostream& out, bool force) {
    if (force) {
        out << '\n';
    }

    if (notesprinted != 0) {
        out << '\n';
    }
    notesprinted = 0;
}

void BaseNote::write(ostream& out, int sonicver, size_t offset) const {
    ignore_unused_variable_warning(this, out, sonicver, offset);
}

void Duration::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    // Note: DAC tracks, PWM tracks and PCM tracks always store the last
    // sample played, rests included. It is only FM and PSG tracks that
    // need this to fix playback of rests when porting from S1/S2 to S3+.
    if ((tracktype == LocTraits::eFMTrack || tracktype == LocTraits::ePSGTrack)
        && s3kmode && (last_note != nullptr) && last_note->is_rest()
        && need_rest) {
        last_note->print(out, sonicver, tracktype, labels, s3kmode);
    }

    need_rest = true;
    // Print durations.
    if (notesprinted == 0) {
        out << "\tdc.b\t";
    }

    PrintHex2Pre(out, get_value(), notesprinted == 0);

    if (++notesprinted == 12) {
        out << '\n';
        notesprinted = 0;
    }
}

FMVoice::FMVoice(istream& in, int sonicver, int n) : BaseNote(0, 0), id(n) {
    voc.read(in, sonicver);
}

void FMVoice::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    ignore_unused_variable_warning(tracktype, labels, s3kmode);
    voc.print(out, sonicver, id);
}

static void print_dac_sample(ostream& out, int val, int sonicver, bool flag) {
    static vector<string> s12daclut{
            "nRst",       "dKick",       "dSnare",      "dClap",
            "dScratch",   "dTimpani",    "dHiTom",      "dVLowClap",
            "dHiTimpani", "dMidTimpani", "dLowTimpani", "dVLowTimpani",
            "dMidTom",    "dLowTom",     "dFloorTom",   "dHiClap",
            "dMidClap",   "dLowClap"};

    static vector<string> s3daclut{
            "nRst",
            "dSnareS3",
            "dHighTom",
            "dMidTomS3",
            "dLowTomS3",
            "dFloorTomS3",
            "dKickS3",
            "dMuffledSnare",
            "dCrashCymbal",
            "dRideCymbal",
            "dLowMetalHit",
            "dMetalHit",
            "dHighMetalHit",
            "dHigherMetalHit",
            "dMidMetalHit",
            "dClapS3",
            "dElectricHighTom",
            "dElectricMidTom",
            "dElectricLowTom",
            "dElectricFloorTom",
            "dTightSnare",
            "dMidpitchSnare",
            "dLooseSnare",
            "dLooserSnare",
            "dHiTimpaniS3",
            "dLowTimpaniS3",
            "dMidTimpaniS3",
            "dQuickLooseSnare",
            "dClick",
            "dPowerKick",
            "dQuickGlassCrash",
            "dGlassCrashSnare",
            "dGlassCrash",
            "dGlassCrashKick",
            "dQuietGlassCrash",
            "dOddSnareKick",
            "dKickExtraBass",
            "dComeOn",
            "dDanceSnare",
            "dLooseKick",
            "dModLooseKick",
            "dWoo",
            "dGo",
            "dSnareGo",
            "dPowerTom",
            "dHiWoodBlock",
            "dLowWoodBlock",
            "dHiHitDrum",
            "dLowHitDrum",
            "dMetalCrashHit",
            "dEchoedClapHit_S3",
            "dLowerEchoedClapHit_S3",
            "dHipHopHitKick",
            "dHipHopHitPowerKick",
            "dBassHey",
            "dDanceStyleKick",
            "dHipHopHitKick2",
            "dHipHopHitKick3",
            "dReverseFadingWind",
            "dScratchS3",
            "dLooseSnareNoise",
            "dPowerKick2",
            "dCrashingNoiseWoo",
            "dQuickHit",
            "dKickHey",
            "dPowerKickHit",
            "dLowPowerKickHit",
            "dLowerPowerKickHit",
            "dLowestPowerKickHit"};

    static vector<string> skdaclut{
            "nRst",
            "dSnareS3",
            "dHighTom",
            "dMidTomS3",
            "dLowTomS3",
            "dFloorTomS3",
            "dKickS3",
            "dMuffledSnare",
            "dCrashCymbal",
            "dRideCymbal",
            "dLowMetalHit",
            "dMetalHit",
            "dHighMetalHit",
            "dHigherMetalHit",
            "dMidMetalHit",
            "dClapS3",
            "dElectricHighTom",
            "dElectricMidTom",
            "dElectricLowTom",
            "dElectricFloorTom",
            "dTightSnare",
            "dMidpitchSnare",
            "dLooseSnare",
            "dLooserSnare",
            "dHiTimpaniS3",
            "dLowTimpaniS3",
            "dMidTimpaniS3",
            "dQuickLooseSnare",
            "dClick",
            "dPowerKick",
            "dQuickGlassCrash",
            "dGlassCrashSnare",
            "dGlassCrash",
            "dGlassCrashKick",
            "dQuietGlassCrash",
            "dOddSnareKick",
            "dKickExtraBass",
            "dComeOn",
            "dDanceSnare",
            "dLooseKick",
            "dModLooseKick",
            "dWoo",
            "dGo",
            "dSnareGo",
            "dPowerTom",
            "dHiWoodBlock",
            "dLowWoodBlock",
            "dHiHitDrum",
            "dLowHitDrum",
            "dMetalCrashHit",
            "dEchoedClapHit",
            "dLowerEchoedClapHit",
            "dHipHopHitKick",
            "dHipHopHitPowerKick",
            "dBassHey",
            "dDanceStyleKick",
            "dHipHopHitKick2",
            "dHipHopHitKick3",
            "dReverseFadingWind",
            "dScratchS3",
            "dLooseSnareNoise",
            "dPowerKick2",
            "dCrashingNoiseWoo",
            "dQuickHit",
            "dKickHey",
            "dPowerKickHit",
            "dLowPowerKickHit",
            "dLowerPowerKickHit",
            "dLowestPowerKickHit"};

    static vector<string> s3ddaclut{
            "nRst",
            "dSnareS3",
            "dHighTom",
            "dMidTomS3",
            "dLowTomS3",
            "dFloorTomS3",
            "dKickS3",
            "dMuffledSnare",
            "dCrashCymbal",
            "dCrashCymbal2",
            "dLowMetalHit",
            "dMetalHit",
            "dHighMetalHit",
            "dHigherMetalHit",
            "dMidMetalHit",
            "dClapS3",
            "dElectricHighTom",
            "dElectricMidTom",
            "dElectricLowTom",
            "dElectricFloorTom",
            "dTightSnare",
            "dMidpitchSnare",
            "dLooseSnare",
            "dLooserSnare",
            "dHiTimpaniS3",
            "dLowTimpaniS3",
            "dMidTimpaniS3",
            "dQuickLooseSnare",
            "dClick",
            "dPowerKick",
            "dQuickGlassCrash",
            "dIntroKick",
            "dFinalFightMetalCrash"};

    size_t note = val - 0x80;
    if (sonicver == 5 && note < s3ddaclut.size()) {
        PrintName(out, s3ddaclut[note], flag);
    } else if (sonicver == 4 && note < skdaclut.size()) {
        PrintName(out, skdaclut[note], flag);
    } else if (sonicver == 3 && note < s3daclut.size()) {
        PrintName(out, s3daclut[note], flag);
    } else if (
            (sonicver == 1 && (note < 0x4 || (note >= 0x8 && note <= 0xb)))
            || (sonicver == 2 && note < s12daclut.size())) {
        PrintName(out, s12daclut[note], flag);
    } else {
        PrintHex2Pre(out, val, flag);
    }
}

void DACNote::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    ignore_unused_variable_warning(tracktype, labels, s3kmode);
    last_note = this;
    need_rest = false;

    // Print durations.
    if (notesprinted == 0) {
        out << "\tdc.b\t";
    }

    print_dac_sample(out, get_value(), sonicver, notesprinted == 0);

    if (++notesprinted == 12) {
        out << '\n';
        notesprinted = 0;
    }
}

void FMPSGNote::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    ignore_unused_variable_warning(labels, s3kmode);
    last_note = this;
    need_rest = false;

    // Print durations.
    if (notesprinted == 0) {
        out << "\tdc.b\t";
    }

    // TODO: string -> string_view.
    static const std::array<string, 96> fmpsglut{
            "nRst", "nC0",  "nCs0", "nD0",  "nEb0", "nE0",  "nF0",  "nFs0",
            "nG0",  "nAb0", "nA0",  "nBb0", "nB0",  "nC1",  "nCs1", "nD1",
            "nEb1", "nE1",  "nF1",  "nFs1", "nG1",  "nAb1", "nA1",  "nBb1",
            "nB1",  "nC2",  "nCs2", "nD2",  "nEb2", "nE2",  "nF2",  "nFs2",
            "nG2",  "nAb2", "nA2",  "nBb2", "nB2",  "nC3",  "nCs3", "nD3",
            "nEb3", "nE3",  "nF3",  "nFs3", "nG3",  "nAb3", "nA3",  "nBb3",
            "nB3",  "nC4",  "nCs4", "nD4",  "nEb4", "nE4",  "nF4",  "nFs4",
            "nG4",  "nAb4", "nA4",  "nBb4", "nB4",  "nC5",  "nCs5", "nD5",
            "nEb5", "nE5",  "nF5",  "nFs5", "nG5",  "nAb5", "nA5",  "nBb5",
            "nB5",  "nC6",  "nCs6", "nD6",  "nEb6", "nE6",  "nF6",  "nFs6",
            "nG6",  "nAb6", "nA6",  "nBb6", "nB6",  "nC7",  "nCs7", "nD7",
            "nEb7", "nE7",  "nF7",  "nFs7", "nG7",  "nAb7", "nA7",  "nBb7"};

    string noteName;
    bool   workAround = false;
    if ((tracktype == LocTraits::ePSGInit || tracktype == LocTraits::ePSGTrack)
        && get_value() != 0x80) {
        uint8_t newbyte = (get_value() + get_base_keydisp()) & 0x7f;
        if (sonicver >= 3 && (newbyte == 0x53 || newbyte == 0x54)) {
            if (newbyte == 0x54) {
                noteName = "nMaxPSG2";
            } else {
                noteName = "nMaxPSG1";
            }
        } else if (sonicver <= 2 && newbyte == 0x46) {
            noteName = "nMaxPSG";
        } else if (sonicver == 1 && (newbyte & 1) == 0 && newbyte >= 0x4c) {
            // Workaround for xm2smps/xm3smps/xm4smps songs.
            workAround = true;
            noteName   = "nMaxPSG";
        }
    }

    if (noteName.length() != 0) {
        if (get_base_keydisp() != 0) {
            string buf = fmt::format(
                    FMT_STRING("({}-${:X})&$FF"), noteName, get_base_keydisp());
            if (workAround) {
                cerr << "Converting PSG noise note $" << hex << setw(2)
                     << setfill('0') << uppercase
                     << static_cast<int>(get_value()) << nouppercase
                     << " from xm*smps with broken transposition '$" << hex
                     << setw(2) << setfill('0') << uppercase
                     << static_cast<int>(get_base_keydisp()) << nouppercase
                     << "' to '" << buf << "'" << endl;
                if ((get_value() - get_base_keydisp()) >= 0xE0
                    || (get_value() - get_base_keydisp()) <= 0x80) {
                    cerr << "Error: however, this conversion will result in an "
                            "invalid note.\n"
                         << "You must edit the channel's transposition and the "
                            "above note\n"
                         << "manually in order to fix this. Blame Tweaker, not "
                            "me."
                         << endl;
                }
            }
            PrintName(out, buf, notesprinted == 0);
        } else {
            PrintName(out, noteName, notesprinted == 0);
        }
    } else {
        size_t note = get_value() - 0x80;
        if (note >= sizeof(fmpsglut) / sizeof(fmpsglut[0])) {
            PrintHex2Pre(out, get_value(), notesprinted == 0);
        } else {
            PrintName(out, fmpsglut[note], notesprinted == 0);
        }
    }

    if (++notesprinted == 12) {
        out << '\n';
        notesprinted = 0;
    }
}

template <bool noret>
void CoordFlagNoParams<noret>::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    ignore_unused_variable_warning(tracktype, labels, s3kmode);
    // Note-like macros:
    string s;
    bool   notelike = false;
    if (sonicver >= 3) {
        switch (get_value()) {
        case 0xe2:
            s = "smpsFade";
            break;    // For $E2, $FF
        case 0xe3:
            s = "smpsStopFM";
            break;
        case 0xe7:
            s        = "smpsNoAttack";
            notelike = true;
            break;
        case 0xe9:
            s = "smpsSpindashRev";
            break;
        case 0xf2:
            s = "smpsStop";
            break;
        case 0xf9:
            s = "smpsReturn";
            break;
        case 0xfa:
            s = "smpsModOff";
            break;
        }
    } else {
        switch (get_value()) {
        case 0xe3:
            s = "smpsReturn";
            break;
        case 0xe4:
            s = "smpsFade";
            break;
        case 0xe7:
            s        = "smpsNoAttack";
            notelike = true;
            break;
        case 0xed:
            s = "smpsClearPush";
            break;    // Sonic 1 version
        case 0xee:
            if (sonicver == 1) {
                s = "smpsStopSpecial";
            }
            break;
        case 0xf1:
            s = "smpsModOn";
            break;
        case 0xf2:
            s = "smpsStop";
            break;
        case 0xf4:
            s = "smpsModOff";
            break;
        case 0xf9:
            s = "smpsMaxRelRate";
            break;
        }
    }
    need_rest = true;

    if (notelike) {
        // Print durations.
        if (notesprinted == 0) {
            out << "\tdc.b\t";
        }

        PrintName(out, s, notesprinted == 0);

        if (++notesprinted == 12) {
            out << '\n';
            notesprinted = 0;
        }
    } else {
        if (notesprinted != 0) {
            out << '\n';
        }
        notesprinted = 0;

        if (s.empty()) {
            out << "\tdc.b\t";
            PrintHex2(out, get_value(), false);
        } else {
            out << "\t";
            PrintName(out, s, true);
        }

        out << '\n';
    }
}

void BaseNote::print_psg_tone(ostream& out, int tone, int sonicver, bool last) {
    if (tone == 0) {
        PrintHex2(out, tone, true);
        return;
    }

    if (sonicver >= 3) {
        out << "sTone_";
    } else {
        out << "fTone_";
    }

    boost::io::ios_all_saver flags(out);
    out << hex << setw(2) << setfill('0') << uppercase << tone;

    if (!last) {
        out << ", ";
    }
}

template <bool noret>
void CoordFlag1ParamByte<noret>::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    ignore_unused_variable_warning(labels, s3kmode);
    if (notesprinted != 0) {
        out << '\n';
    }
    notesprinted = 0;
    need_rest    = true;

    string s;
    bool   metacf = false;
    if (sonicver >= 3) {
        switch (get_value()) {
        case 0xe0:
            s = "smpsPan";
            break;
        case 0xe1:
            s = "smpsDetune";
            break;
        case 0xe2:
            s = "smpsFade";
            break;    // For $E2, XX with XX != $FF
        case 0xe4:
            s = "smpsSetVol";
            break;
        case 0xe6:
            s = "smpsFMAlterVol";
            break;
        case 0xe8:
            s = "smpsNoteFill";
            break;
        case 0xea:
            s = "smpsPlayDACSample";
            break;
        case 0xec:
            s = "smpsPSGAlterVol";
            break;
        case 0xed:
            s = "smpsSetNote";
            break;
        case 0xef:
            s = "smpsSetvoice";
            break;    // Case with param >= 0
        case 0xf3:
            s = "smpsPSGform";
            break;
        case 0xf4:
            s = "smpsModChange";
            break;
        case 0xf5:
            s = "smpsPSGvoice";
            break;
        case 0xfb:
            s = "smpsChangeTransposition";
            break;
        case 0xfd:
            s = "smpsAlternameSMPS";
            break;
        case 0xff:
            metacf = true;
            switch (param) {
            case 0x02:
                s = "smpsHaltMusic";
                break;
            case 0x07:
                s = "smpsResetSpindashRev";
                break;
            }
            break;
        }
    } else {
        switch (get_value()) {
        case 0xe0:
            s = "smpsPan";
            break;
        case 0xe1:
            s = "smpsDetune";
            break;
        case 0xe2:
            s = "smpsNop";
            break;
        case 0xe5:
            s = "smpsChanTempoDiv";
            break;
        case 0xe6:
            s = "smpsAlterVol";
            break;
        case 0xe8:
            s = "smpsNoteFill";
            break;
        case 0xe9:
            s = "smpsChangeTransposition";
            break;
        case 0xea:
            s = "smpsSetTempoMod";
            break;
        case 0xeb:
            s = "smpsSetTempoDiv";
            break;
        case 0xec:
            s = "smpsPSGAlterVol";
            break;
        case 0xef:
            s = "smpsSetvoice";
            break;
        case 0xf3:
            s = "smpsPSGform";
            break;
        case 0xf5:
            s = "smpsPSGvoice";
            break;
            // case 0xed:  s = ""; break;  // Sonic 2 version
        }
    }

    if (s.empty()) {
        out << "\tdc.b\t";
        PrintHex2(out, get_value(), false);
    } else if (metacf) {
        out << "\t" << s << '\n';
        return;
    } else {
        PrintMacro(out, s.c_str());
    }

    if (get_value() == 0xe0) {
        int dir = param & 0xc0;
        switch (dir) {
        case 0x40:
            out << "panRight, ";
            break;
        case 0x80:
            out << "panLeft, ";
            break;
        case 0xc0:
            out << "panCenter, ";
            break;
        default:
            out << "panNone, ";
            break;
        }
        PrintHex2(out, param & 0x3f, true);
    } else if (
            get_value() == 0xf5
            || (sonicver >= 3 && get_value() == 0xef
                && tracktype == LocTraits::ePSGTrack)) {
        BaseNote::print_psg_tone(out, param, sonicver, true);
    } else if (sonicver >= 3 && get_value() == 0xea) {
        print_dac_sample(out, param, sonicver, true);
    } else {
        PrintHex2(out, param, true);
    }
    out << '\n';
}

void CoordFlagChgKeydisp::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    ignore_unused_variable_warning(sonicver, tracktype, labels, s3kmode);
    if (notesprinted != 0) {
        out << '\n';
    }
    notesprinted = 0;
    need_rest    = true;
    PrintMacro(out, "smpsChangeTransposition");
    PrintHex2(out, param, true);
    out << '\n';
}

template <bool noret>
void CoordFlag2ParamBytes<noret>::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    ignore_unused_variable_warning(tracktype, labels, s3kmode);
    if (notesprinted != 0) {
        out << '\n';
    }
    notesprinted = 0;
    need_rest    = true;

    string s;
    bool   metacf = false;
    if (sonicver >= 3) {
        switch (get_value()) {
        case 0xe5:
            s = "smpsFMAlterVol";
            break;
        case 0xee:
            s = "smpsFMICommand";
            break;
        case 0xef:
            s = "smpsSetvoice";
            break;    // Case with param < 0
        case 0xf1:
            s = "smpsModChange2";
            break;
        case 0xff:
            metacf = true;
            switch (param1) {
            case 0x00:
                s = "smpsSetTempoMod";
                break;
            case 0x01:
                s = "smpsPlaySound";
                break;
            case 0x04:
                s = "smpsSetTempoDiv";
                break;
            }
            break;
        }
    }

    if (s.empty()) {
        out << "\tdc.b\t";
        PrintHex2(out, get_value(), false);
    } else {
        PrintMacro(out, s.c_str());
    }

    if (!metacf) {
        PrintHex2(out, param1, false);
    }

    PrintHex2(out, param2, true);
    out << '\n';
}

template <bool noret>
void CoordFlag3ParamBytes<noret>::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    ignore_unused_variable_warning(tracktype, labels, s3kmode);
    if (notesprinted != 0) {
        out << '\n';
    }
    notesprinted = 0;
    need_rest    = true;

    string s;
    bool   metacf = false;
    if (sonicver >= 3) {
        switch (get_value()) {
        case 0xff:
            metacf = true;
            switch (param1) {
            case 0x06:
                s = "smpsFMVolEnv";
                break;
            }
            break;
        }
    }

    if (s.empty()) {
        out << "\tdc.b\t";
        PrintHex2(out, get_value(), false);
    } else {
        PrintMacro(out, s.c_str());
    }

    if (!metacf) {
        PrintHex2(out, param1, false);
    }

    PrintHex2(out, param2, false);
    PrintHex2(out, param3, true);
    out << '\n';
}

template <bool noret>
void CoordFlag4ParamBytes<noret>::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    ignore_unused_variable_warning(tracktype, labels, s3kmode);
    if (notesprinted != 0) {
        out << '\n';
    }
    notesprinted = 0;
    need_rest    = true;

    string s;
    bool   metacf = false;
    if (sonicver >= 3) {
        switch (get_value()) {
        case 0xf0:
            s = "smpsModSet";
            break;
        case 0xfe:
            s = "smpsFM3SpecialMode";
            break;
        }
    } else {
        switch (get_value()) {
        case 0xf0:
            s = "smpsModSet";
            break;
        }
    }

    if (s.empty()) {
        out << "\tdc.b\t";
        PrintHex2(out, get_value(), false);
    } else {
        PrintMacro(out, s.c_str());
    }

    if (!metacf) {
        PrintHex2(out, param1, false);
    }

    PrintHex2(out, param2, false);
    PrintHex2(out, param3, false);
    PrintHex2(out, param4, true);
    out << '\n';
}

template <bool noret>
void CoordFlag5ParamBytes<noret>::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    ignore_unused_variable_warning(tracktype, labels, s3kmode);
    if (notesprinted != 0) {
        out << '\n';
    }
    notesprinted = 0;
    need_rest    = true;

    string s;
    bool   metacf = false;
    if (sonicver >= 3) {
        switch (get_value()) {
        case 0xff:
            metacf = true;
            switch (param1) {
            case 0x05:
                s = "smpsSSGEG";
                break;
            }
            break;
        }
    }

    if (s.empty()) {
        out << "\tdc.b\t";
        PrintHex2(out, get_value(), false);
    } else {
        PrintMacro(out, s.c_str());
    }

    if (!metacf) {
        PrintHex2(out, param1, false);
    }

    PrintHex2(out, param2, false);
    PrintHex2(out, param3, false);
    PrintHex2(out, param4, false);
    PrintHex2(out, param5, true);
    out << '\n';
}

template <bool noret>
void CoordFlagPointerParam<noret>::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    ignore_unused_variable_warning(sonicver, tracktype, s3kmode);
    if (notesprinted != 0) {
        out << '\n';
    }
    notesprinted = 0;
    need_rest    = true;

    if (get_value() == 0xf6) {
        PrintMacro(out, "smpsJump");
    } else if (get_value() == 0xf8) {
        last_note = nullptr;
        PrintMacro(out, "smpsCall");
    } else if (get_value() == 0xfc) {    // Sonic 3 only
        PrintMacro(out, "smpsContinuousLoop");
    }

    auto it = labels.find(jumptarget);
    out << it->second << '\n';
}

template <bool noret>
void CoordFlagPointer1ParamByte<noret>::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    ignore_unused_variable_warning(tracktype, s3kmode);
    if (notesprinted != 0) {
        out << '\n';
    }
    notesprinted = 0;
    need_rest    = true;

    string s;
    bool   metacf = false;
    if (sonicver >= 3) {
        switch (get_value()) {
        case 0xeb:
            s = "smpsConditionalJump";
            break;
        }
    }

    if (s.empty()) {
        out << "\tdc.b\t";
        PrintHex2(out, get_value(), false);
    } else {
        PrintMacro(out, s.c_str());
    }

    if (!metacf) {
        PrintHex2(out, param1, false);
    }

    auto it = labels.find(jumptarget);
    out << it->second << '\n';
}

template <bool noret>
void CoordFlagPointer2ParamBytes<noret>::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    ignore_unused_variable_warning(tracktype, s3kmode);
    if (notesprinted != 0) {
        out << '\n';
    }
    notesprinted = 0;
    need_rest    = true;

    string s;
    bool   metacf = false;
    if (sonicver >= 3) {
        switch (get_value()) {
        case 0xf7:
            s = "smpsLoop";
            break;
        case 0xff:
            metacf = true;
            switch (param1) {
            case 0x03:
                s = "smpsCopyData";
                break;
            }
            break;
        }
    } else {
        switch (get_value()) {
        case 0xf7:
            s = "smpsLoop";
            break;
        }
    }

    if (s.empty()) {
        out << "\tdc.b\t";
        PrintHex2(out, get_value(), false);
    } else {
        PrintMacro(out, s.c_str());
    }

    if (!metacf) {
        PrintHex2(out, param1, false);
        PrintHex2(out, param2, false);
    }

    auto it = labels.find(jumptarget);
    if (it != labels.end()) {
        out << it->second;
    } else {
        PrintHex4(out, jumptarget, true);
    }

    if (metacf) {
        out << ", ";
        PrintHex2(out, param2, true);
    }
    out << '\n';
}
