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
#include <string_view>
#include <vector>

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

#include <mdcomp/bigendian_io.hh>

using std::array;
using std::cerr;
using std::endl;
using std::hex;
using std::ios;
using std::istream;
using std::multimap;
using std::nouppercase;
using std::ostream;
using std::pair;
using std::setfill;
using std::setw;
using std::string;
using std::string_view;
using std::uppercase;

using namespace std::literals::string_view_literals;

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

FMVoice::FMVoice(istream& in, int sonicver, int n) noexcept
        : BaseNote(0, 0), id(n) {
    voc.read(in, sonicver);
}

void FMVoice::print(
        ostream& out, int sonicver, LocTraits::LocType tracktype,
        multimap<int, string>& labels, bool s3kmode) const {
    ignore_unused_variable_warning(tracktype, labels, s3kmode);
    voc.print(out, sonicver, id);
}

static void print_dac_sample(ostream& out, int val, int sonicver, bool flag) {
    constexpr const static array s12daclut{
            "nRst"sv,       "dKick"sv,       "dSnare"sv,      "dClap"sv,
            "dScratch"sv,   "dTimpani"sv,    "dHiTom"sv,      "dVLowClap"sv,
            "dHiTimpani"sv, "dMidTimpani"sv, "dLowTimpani"sv, "dVLowTimpani"sv,
            "dMidTom"sv,    "dLowTom"sv,     "dFloorTom"sv,   "dHiClap"sv,
            "dMidClap"sv,   "dLowClap"sv};

    constexpr const static array s3daclut{
            "nRst"sv,
            "dSnareS3"sv,
            "dHighTom"sv,
            "dMidTomS3"sv,
            "dLowTomS3"sv,
            "dFloorTomS3"sv,
            "dKickS3"sv,
            "dMuffledSnare"sv,
            "dCrashCymbal"sv,
            "dRideCymbal"sv,
            "dLowMetalHit"sv,
            "dMetalHit"sv,
            "dHighMetalHit"sv,
            "dHigherMetalHit"sv,
            "dMidMetalHit"sv,
            "dClapS3"sv,
            "dElectricHighTom"sv,
            "dElectricMidTom"sv,
            "dElectricLowTom"sv,
            "dElectricFloorTom"sv,
            "dTightSnare"sv,
            "dMidpitchSnare"sv,
            "dLooseSnare"sv,
            "dLooserSnare"sv,
            "dHiTimpaniS3"sv,
            "dLowTimpaniS3"sv,
            "dMidTimpaniS3"sv,
            "dQuickLooseSnare"sv,
            "dClick"sv,
            "dPowerKick"sv,
            "dQuickGlassCrash"sv,
            "dGlassCrashSnare"sv,
            "dGlassCrash"sv,
            "dGlassCrashKick"sv,
            "dQuietGlassCrash"sv,
            "dOddSnareKick"sv,
            "dKickExtraBass"sv,
            "dComeOn"sv,
            "dDanceSnare"sv,
            "dLooseKick"sv,
            "dModLooseKick"sv,
            "dWoo"sv,
            "dGo"sv,
            "dSnareGo"sv,
            "dPowerTom"sv,
            "dHiWoodBlock"sv,
            "dLowWoodBlock"sv,
            "dHiHitDrum"sv,
            "dLowHitDrum"sv,
            "dMetalCrashHit"sv,
            "dEchoedClapHit_S3"sv,
            "dLowerEchoedClapHit_S3"sv,
            "dHipHopHitKick"sv,
            "dHipHopHitPowerKick"sv,
            "dBassHey"sv,
            "dDanceStyleKick"sv,
            "dHipHopHitKick2"sv,
            "dHipHopHitKick3"sv,
            "dReverseFadingWind"sv,
            "dScratchS3"sv,
            "dLooseSnareNoise"sv,
            "dPowerKick2"sv,
            "dCrashingNoiseWoo"sv,
            "dQuickHit"sv,
            "dKickHey"sv,
            "dPowerKickHit"sv,
            "dLowPowerKickHit"sv,
            "dLowerPowerKickHit"sv,
            "dLowestPowerKickHit"sv};

    constexpr const static array skdaclut{
            "nRst"sv,
            "dSnareS3"sv,
            "dHighTom"sv,
            "dMidTomS3"sv,
            "dLowTomS3"sv,
            "dFloorTomS3"sv,
            "dKickS3"sv,
            "dMuffledSnare"sv,
            "dCrashCymbal"sv,
            "dRideCymbal"sv,
            "dLowMetalHit"sv,
            "dMetalHit"sv,
            "dHighMetalHit"sv,
            "dHigherMetalHit"sv,
            "dMidMetalHit"sv,
            "dClapS3"sv,
            "dElectricHighTom"sv,
            "dElectricMidTom"sv,
            "dElectricLowTom"sv,
            "dElectricFloorTom"sv,
            "dTightSnare"sv,
            "dMidpitchSnare"sv,
            "dLooseSnare"sv,
            "dLooserSnare"sv,
            "dHiTimpaniS3"sv,
            "dLowTimpaniS3"sv,
            "dMidTimpaniS3"sv,
            "dQuickLooseSnare"sv,
            "dClick"sv,
            "dPowerKick"sv,
            "dQuickGlassCrash"sv,
            "dGlassCrashSnare"sv,
            "dGlassCrash"sv,
            "dGlassCrashKick"sv,
            "dQuietGlassCrash"sv,
            "dOddSnareKick"sv,
            "dKickExtraBass"sv,
            "dComeOn"sv,
            "dDanceSnare"sv,
            "dLooseKick"sv,
            "dModLooseKick"sv,
            "dWoo"sv,
            "dGo"sv,
            "dSnareGo"sv,
            "dPowerTom"sv,
            "dHiWoodBlock"sv,
            "dLowWoodBlock"sv,
            "dHiHitDrum"sv,
            "dLowHitDrum"sv,
            "dMetalCrashHit"sv,
            "dEchoedClapHit"sv,
            "dLowerEchoedClapHit"sv,
            "dHipHopHitKick"sv,
            "dHipHopHitPowerKick"sv,
            "dBassHey"sv,
            "dDanceStyleKick"sv,
            "dHipHopHitKick2"sv,
            "dHipHopHitKick3"sv,
            "dReverseFadingWind"sv,
            "dScratchS3"sv,
            "dLooseSnareNoise"sv,
            "dPowerKick2"sv,
            "dCrashingNoiseWoo"sv,
            "dQuickHit"sv,
            "dKickHey"sv,
            "dPowerKickHit"sv,
            "dLowPowerKickHit"sv,
            "dLowerPowerKickHit"sv,
            "dLowestPowerKickHit"sv};

    constexpr const static array s3ddaclut{
            "nRst"sv,
            "dSnareS3"sv,
            "dHighTom"sv,
            "dMidTomS3"sv,
            "dLowTomS3"sv,
            "dFloorTomS3"sv,
            "dKickS3"sv,
            "dMuffledSnare"sv,
            "dCrashCymbal"sv,
            "dCrashCymbal2"sv,
            "dLowMetalHit"sv,
            "dMetalHit"sv,
            "dHighMetalHit"sv,
            "dHigherMetalHit"sv,
            "dMidMetalHit"sv,
            "dClapS3"sv,
            "dElectricHighTom"sv,
            "dElectricMidTom"sv,
            "dElectricLowTom"sv,
            "dElectricFloorTom"sv,
            "dTightSnare"sv,
            "dMidpitchSnare"sv,
            "dLooseSnare"sv,
            "dLooserSnare"sv,
            "dHiTimpaniS3"sv,
            "dLowTimpaniS3"sv,
            "dMidTimpaniS3"sv,
            "dQuickLooseSnare"sv,
            "dClick"sv,
            "dPowerKick"sv,
            "dQuickGlassCrash"sv,
            "dIntroKick"sv,
            "dFinalFightMetalCrash"sv};

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

    constexpr const static array fmpsglut{
            "nRst"sv, "nC0"sv,  "nCs0"sv, "nD0"sv,  "nEb0"sv, "nE0"sv,
            "nF0"sv,  "nFs0"sv, "nG0"sv,  "nAb0"sv, "nA0"sv,  "nBb0"sv,
            "nB0"sv,  "nC1"sv,  "nCs1"sv, "nD1"sv,  "nEb1"sv, "nE1"sv,
            "nF1"sv,  "nFs1"sv, "nG1"sv,  "nAb1"sv, "nA1"sv,  "nBb1"sv,
            "nB1"sv,  "nC2"sv,  "nCs2"sv, "nD2"sv,  "nEb2"sv, "nE2"sv,
            "nF2"sv,  "nFs2"sv, "nG2"sv,  "nAb2"sv, "nA2"sv,  "nBb2"sv,
            "nB2"sv,  "nC3"sv,  "nCs3"sv, "nD3"sv,  "nEb3"sv, "nE3"sv,
            "nF3"sv,  "nFs3"sv, "nG3"sv,  "nAb3"sv, "nA3"sv,  "nBb3"sv,
            "nB3"sv,  "nC4"sv,  "nCs4"sv, "nD4"sv,  "nEb4"sv, "nE4"sv,
            "nF4"sv,  "nFs4"sv, "nG4"sv,  "nAb4"sv, "nA4"sv,  "nBb4"sv,
            "nB4"sv,  "nC5"sv,  "nCs5"sv, "nD5"sv,  "nEb5"sv, "nE5"sv,
            "nF5"sv,  "nFs5"sv, "nG5"sv,  "nAb5"sv, "nA5"sv,  "nBb5"sv,
            "nB5"sv,  "nC6"sv,  "nCs6"sv, "nD6"sv,  "nEb6"sv, "nE6"sv,
            "nF6"sv,  "nFs6"sv, "nG6"sv,  "nAb6"sv, "nA6"sv,  "nBb6"sv,
            "nB6"sv,  "nC7"sv,  "nCs7"sv, "nD7"sv,  "nEb7"sv, "nE7"sv,
            "nF7"sv,  "nFs7"sv, "nG7"sv,  "nAb7"sv, "nA7"sv,  "nBb7"sv};

    const auto [noteName, workAround] = [&]() -> pair<string_view, bool> {
        if ((tracktype == LocTraits::ePSGInit
             || tracktype == LocTraits::ePSGTrack)
            && get_value() != 0x80) {
            const uint8_t newbyte
                    = uint32_t(get_value() + get_base_keydisp()) & 0x7fU;
            if (sonicver >= 3 && (newbyte == 0x53 || newbyte == 0x54)) {
                if (newbyte == 0x54) {
                    return {"nMaxPSG2"sv, false};
                }
                return {"nMaxPSG1"sv, false};
            }
            if (sonicver <= 2 && newbyte == 0x46) {
                return {"nMaxPSG"sv, false};
            }
            if (sonicver == 1 && (newbyte & 1U) == 0 && newbyte >= 0x4c) {
                // Workaround for xm2smps/xm3smps/xm4smps songs.
                return {"nMaxPSG"sv, true};
            }
        }
        return {""sv, false};
    }();

    if (!noteName.empty()) {
        if (get_base_keydisp() != 0) {
            string buf = fmt::format(
                    "({}-${:X})&$FF", noteName, get_base_keydisp());
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
    const auto [s, notelike] = [&]() -> pair<string_view, bool> {
        if (sonicver >= 3) {
            switch (get_value()) {
            case 0xe2:
                return {"smpsFade"sv, false};    // For $E2, $FF
            case 0xe3:
                return {"smpsStopFM"sv, false};
            case 0xe7:
                return {"smpsNoAttack"sv, true};
            case 0xe9:
                return {"smpsSpindashRev"sv, false};
            case 0xf2:
                return {"smpsStop"sv, false};
            case 0xf9:
                return {"smpsReturn"sv, false};
            case 0xfa:
                return {"smpsModOff"sv, false};
            }
        } else {
            switch (get_value()) {
            case 0xe3:
                return {"smpsReturn"sv, false};
            case 0xe4:
                return {"smpsFade"sv, false};
            case 0xe7:
                return {"smpsNoAttack"sv, true};
            case 0xed:
                return {"smpsClearPush"sv, false};
            case 0xee:
                if (sonicver == 1) {
                    return {"smpsStopSpecial"sv, false};
                }
                break;
            case 0xf1:
                return {"smpsModOn"sv, false};
            case 0xf2:
                return {"smpsStop"sv, false};
            case 0xf4:
                return {"smpsModOff"sv, false};
            case 0xf9:
                return {"smpsMaxRelRate"sv, false};
            }
        }
        return {""sv, false};
    }();
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

    const auto [s, metacf] = [&]() -> pair<string_view, bool> {
        if (sonicver >= 3) {
            switch (get_value()) {
            case 0xe0:
                return {"smpsPan"sv, false};
            case 0xe1:
                return {"smpsDetune"sv, false};
            case 0xe2:
                return {"smpsFade"sv, false};    // For $E2, XX with XX != $FF
            case 0xe4:
                return {"smpsSetVol"sv, false};
            case 0xe6:
                return {"smpsFMAlterVol"sv, false};
            case 0xe8:
                return {"smpsNoteFill"sv, false};
            case 0xea:
                return {"smpsPlayDACSample"sv, false};
            case 0xec:
                return {"smpsPSGAlterVol"sv, false};
            case 0xed:
                return {"smpsSetNote"sv, false};
            case 0xef:
                return {"smpsSetvoice"sv, false};    // Case with param >= 0
            case 0xf3:
                return {"smpsPSGform"sv, false};
            case 0xf4:
                return {"smpsModChange"sv, false};
            case 0xf5:
                return {"smpsPSGvoice"sv, false};
            case 0xfb:
                return {"smpsChangeTransposition"sv, false};
            case 0xfd:
                return {"smpsAlternameSMPS"sv, false};
            case 0xff:
                switch (param) {
                case 0x02:
                    return {"smpsHaltMusic"sv, true};
                case 0x07:
                    return {"smpsResetSpindashRev"sv, true};
                }
                return {""sv, true};
            }
        } else {
            switch (get_value()) {
            case 0xe0:
                return {"smpsPan"sv, false};
            case 0xe1:
                return {"smpsDetune"sv, false};
            case 0xe2:
                return {"smpsNop"sv, false};
            case 0xe5:
                return {"smpsChanTempoDiv"sv, false};
            case 0xe6:
                return {"smpsAlterVol"sv, false};
            case 0xe8:
                return {"smpsNoteFill"sv, false};
            case 0xe9:
                return {"smpsChangeTransposition"sv, false};
            case 0xea:
                return {"smpsSetTempoMod"sv, false};
            case 0xeb:
                return {"smpsSetTempoDiv"sv, false};
            case 0xec:
                return {"smpsPSGAlterVol"sv, false};
            case 0xef:
                return {"smpsSetvoice"sv, false};
            case 0xf3:
                return {"smpsPSGform"sv, false};
            case 0xf5:
                return {"smpsPSGvoice"sv, false};
                // case 0xed:
                //     return {""sv, false};    // Sonic 2 version
            }
        }
        return {""sv, false};
    }();
    if (s.empty()) {
        out << "\tdc.b\t";
        PrintHex2(out, get_value(), false);
    } else if (metacf) {
        out << "\t" << s << '\n';
        return;
    } else {
        PrintMacro(out, s);
    }

    if (get_value() == 0xe0) {
        int dir = param & 0xc0U;
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
        PrintHex2(out, param & 0x3fU, true);
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
    PrintMacro(out, "smpsChangeTransposition"sv);
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

    const auto [s, metacf] = [&]() -> pair<string_view, bool> {
        if (sonicver >= 3) {
            switch (get_value()) {
            case 0xe5:
                return {"smpsFMAlterVol"sv, false};
            case 0xee:
                return {"smpsFMICommand"sv, false};
            case 0xef:
                return {"smpsSetvoice"sv, false};    // Case with param < 0
            case 0xf1:
                return {"smpsModChange2"sv, false};
            case 0xff:
                switch (param1) {
                case 0x00:
                    return {"smpsSetTempoMod"sv, true};
                case 0x01:
                    return {"smpsPlaySound"sv, true};
                case 0x04:
                    return {"smpsSetTempoDiv"sv, true};
                }
                return {""sv, true};
            }
        }
        return {""sv, false};
    }();

    if (s.empty()) {
        out << "\tdc.b\t";
        PrintHex2(out, get_value(), false);
    } else {
        PrintMacro(out, s);
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

    const auto [s, metacf] = [&]() -> pair<string_view, bool> {
        if (sonicver >= 3) {
            switch (get_value()) {
            case 0xff:
                switch (param1) {
                case 0x06:
                    return {"smpsFMVolEnv"sv, true};
                }
                return {""sv, true};
            }
        }
        return {""sv, false};
    }();

    if (s.empty()) {
        out << "\tdc.b\t";
        PrintHex2(out, get_value(), false);
    } else {
        PrintMacro(out, s);
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

    const auto [s, metacf] = [&]() -> pair<string_view, bool> {
        if (sonicver >= 3) {
            switch (get_value()) {
            case 0xf0:
                return {"smpsModSet"sv, false};
            case 0xfe:
                return {"smpsFM3SpecialMode"sv, false};
            }
        } else {
            switch (get_value()) {
            case 0xf0:
                return {"smpsModSet"sv, false};
            }
        }
        return {""sv, false};
    }();

    if (s.empty()) {
        out << "\tdc.b\t";
        PrintHex2(out, get_value(), false);
    } else {
        PrintMacro(out, s);
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

    const auto [s, metacf] = [&]() -> pair<string_view, bool> {
        if (sonicver >= 3) {
            switch (get_value()) {
            case 0xff:
                switch (param1) {
                case 0x05:
                    return {"smpsSSGEG"sv, true};
                }
                return {""sv, true};
            }
        }
        return {""sv, false};
    }();

    if (s.empty()) {
        out << "\tdc.b\t";
        PrintHex2(out, get_value(), false);
    } else {
        PrintMacro(out, s);
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
        PrintMacro(out, "smpsJump"sv);
    } else if (get_value() == 0xf8) {
        last_note = nullptr;
        PrintMacro(out, "smpsCall"sv);
    } else if (get_value() == 0xfc) {    // Sonic 3 only
        PrintMacro(out, "smpsContinuousLoop"sv);
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

    const auto [s, metacf] = [&]() -> pair<string_view, bool> {
        if (sonicver >= 3) {
            switch (get_value()) {
            case 0xeb:
                return {"smpsConditionalJump"sv, false};
            }
        }
        return {""sv, false};
    }();

    if (s.empty()) {
        out << "\tdc.b\t";
        PrintHex2(out, get_value(), false);
    } else {
        PrintMacro(out, s);
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

    const auto [s, metacf] = [&]() -> pair<string_view, bool> {
        if (sonicver >= 3) {
            switch (get_value()) {
            case 0xf7:
                return {"smpsLoop"sv, false};
            case 0xff:
                switch (param1) {
                case 0x03:
                    return {"smpsCopyData"sv, true};
                }
                return {"smpsCopyData"sv, true};
            }
        } else {
            switch (get_value()) {
            case 0xf7:
                return {"smpsLoop"sv, false};
            }
        }
        return {""sv, false};
    }();

    if (s.empty()) {
        out << "\tdc.b\t";
        PrintHex2(out, get_value(), false);
    } else {
        PrintMacro(out, s);
    }

    if (!metacf) {
        PrintHex2(out, param1, false);
        PrintHex2(out, param2, false);
    }

    if (auto it = labels.find(jumptarget); it != labels.end()) {
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
