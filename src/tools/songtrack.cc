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

#include "songtrack.h"

#include <istream>
#include <ostream>
#include <iostream>
#include <iomanip>
#include <cstdio>

#include "bigendian_io.h"

using namespace std;

size_t BaseNote::notesprinted = 0;
BaseNote const *BaseNote::last_note = 0;
bool BaseNote::need_rest = false;

void BaseNote::force_linebreak(ostream &out, bool force) {
	if (force)
		out << endl;

	if (notesprinted != 0)
		out << endl;
	notesprinted = 0;
}

void BaseNote::write(ostream &UNUSED(out), int UNUSED(sonicver),
                     size_t UNUSED(offset)) const {
}

void Duration::print(ostream &out,
                     int sonicver,
                     LocTraits::LocType tracktype,
                     multimap<int, string> &labels,
                     bool s3kmode) const {
	// Note: DAC tracks, PWM tracks and PCM tracks always store the last
	// sample played, rests included. It is only FM and PSG tracks that
	// need this to fix playback of rests when porting from S1/S2 to S3+.
	if ((tracktype == LocTraits::eFMTrack || tracktype == LocTraits::ePSGTrack)
	    && s3kmode && last_note && last_note->is_rest() && need_rest)
		last_note->print(out, sonicver, tracktype, labels, s3kmode);

	need_rest = true;
	// Print durations.
	if (notesprinted == 0)
		out << "\tdc.b\t";

	PrintHex2Pre(out, val, notesprinted == 0);

	if (++notesprinted == 12) {
		out << endl;
		notesprinted = 0;
	}
}

FMVoice::FMVoice(istream &in, int sonicver, int n)
	: BaseNote(0, 0), id(n) {
	voc.read(in, sonicver);
}

void FMVoice::print(ostream &out,
                    int sonicver,
                    LocTraits::LocType UNUSED(tracktype),
                    multimap<int, string> &UNUSED(labels),
                    bool UNUSED(s3kmode)) const {
	voc.print(out, sonicver, id);
}

static void print_dac_sample(ostream &out, int val, int sonicver, bool flag) {
	static string s12daclut[] = {
		"nRst", "dKick", "dSnare", "dClap", "dScratch", "dTimpani", "dHiTom",
		"dVLowClap", "dHiTimpani", "dMidTimpani", "dLowTimpani", "dVLowTimpani",
		"dMidTom", "dLowTom", "dFloorTom", "dHiClap", "dMidClap", "dLowClap"
	};

	static string s3daclut[] = {
		"nRst", "dSnareS3", "dHighTom", "dMidTomS3", "dLowTomS3", "dFloorTomS3",
		"dKickS3", "dMuffledSnare", "dCrashCymbal", "dRideCymbal", "dLowMetalHit",
		"dMetalHit", "dHighMetalHit", "dHigherMetalHit", "dMidMetalHit",
		"dClapS3", "dElectricHighTom", "dElectricMidTom", "dElectricLowTom",
		"dElectricFloorTom", "dTightSnare", "dMidpitchSnare", "dLooseSnare",
		"dLooserSnare", "dHiTimpaniS3", "dLowTimpaniS3", "dMidTimpaniS3",
		"dQuickLooseSnare", "dClick", "dPowerKick", "dQuickGlassCrash",
		"dGlassCrashSnare", "dGlassCrash", "dGlassCrashKick", "dQuietGlassCrash",
		"dOddSnareKick", "dKickExtraBass", "dComeOn", "dDanceSnare", "dLooseKick",
		"dModLooseKick", "dWoo", "dGo", "dSnareGo", "dPowerTom", "dHiWoodBlock",
		"dLowWoodBlock", "dHiHitDrum", "dLowHitDrum", "dMetalCrashHit",
		"dEchoedClapHit_S3", "dLowerEchoedClapHit_S3", "dHipHopHitKick",
		"dHipHopHitPowerKick", "dBassHey", "dDanceStyleKick", "dHipHopHitKick2",
		"dHipHopHitKick3", "dReverseFadingWind", "dScratchS3", "dLooseSnareNoise",
		"dPowerKick2", "dCrashingNoiseWoo", "dQuickHit", "dKickHey",
		"dPowerKickHit", "dLowPowerKickHit", "dLowerPowerKickHit",
		"dLowestPowerKickHit"
	};

	static string skdaclut[] = {
		"nRst", "dSnareS3", "dHighTom", "dMidTomS3", "dLowTomS3", "dFloorTomS3",
		"dKickS3", "dMuffledSnare", "dCrashCymbal", "dRideCymbal", "dLowMetalHit",
		"dMetalHit", "dHighMetalHit", "dHigherMetalHit", "dMidMetalHit",
		"dClapS3", "dElectricHighTom", "dElectricMidTom", "dElectricLowTom",
		"dElectricFloorTom", "dTightSnare", "dMidpitchSnare", "dLooseSnare",
		"dLooserSnare", "dHiTimpaniS3", "dLowTimpaniS3", "dMidTimpaniS3",
		"dQuickLooseSnare", "dClick", "dPowerKick", "dQuickGlassCrash",
		"dGlassCrashSnare", "dGlassCrash", "dGlassCrashKick", "dQuietGlassCrash",
		"dOddSnareKick", "dKickExtraBass", "dComeOn", "dDanceSnare", "dLooseKick",
		"dModLooseKick", "dWoo", "dGo", "dSnareGo", "dPowerTom", "dHiWoodBlock",
		"dLowWoodBlock", "dHiHitDrum", "dLowHitDrum", "dMetalCrashHit",
		"dEchoedClapHit", "dLowerEchoedClapHit", "dHipHopHitKick",
		"dHipHopHitPowerKick", "dBassHey", "dDanceStyleKick", "dHipHopHitKick2",
		"dHipHopHitKick3", "dReverseFadingWind", "dScratchS3", "dLooseSnareNoise",
		"dPowerKick2", "dCrashingNoiseWoo", "dQuickHit", "dKickHey",
		"dPowerKickHit", "dLowPowerKickHit", "dLowerPowerKickHit",
		"dLowestPowerKickHit"
	};

	static string s3ddaclut[] = {
		"nRst", "dSnareS3", "dHighTom", "dMidTomS3", "dLowTomS3", "dFloorTomS3",
		"dKickS3", "dMuffledSnare", "dCrashCymbal", "dCrashCymbal2", "dLowMetalHit",
		"dMetalHit", "dHighMetalHit", "dHigherMetalHit", "dMidMetalHit",
		"dClapS3", "dElectricHighTom", "dElectricMidTom", "dElectricLowTom",
		"dElectricFloorTom", "dTightSnare", "dMidpitchSnare", "dLooseSnare",
		"dLooserSnare", "dHiTimpaniS3", "dLowTimpaniS3", "dMidTimpaniS3",
		"dQuickLooseSnare", "dClick", "dPowerKick", "dQuickGlassCrash",
		"dIntroKick", "dFinalFightMetalCrash"
	};

	size_t note = val - 0x80;
	if (sonicver == 5 && note <= sizeof(s3ddaclut) / sizeof(s3ddaclut[0]))
		PrintName(out, s3ddaclut[note], flag);
	else if (sonicver == 4 && note <= sizeof(skdaclut) / sizeof(skdaclut[0]))
		PrintName(out, skdaclut[note], flag);
	else if (sonicver == 3 && note <= sizeof(s3daclut) / sizeof(s3daclut[0]))
		PrintName(out, s3daclut[note], flag);
	else if (sonicver == 1 && (note < 0x4 || (note >= 0x8 && note <= 0xb)))
		PrintName(out, s12daclut[note], flag);
	else if (sonicver == 2 && note <= sizeof(s12daclut) / sizeof(s12daclut[0]))
		PrintName(out, s12daclut[note], flag);
	else
		PrintHex2Pre(out, val, flag);
}

void DACNote::print(ostream &out,
                    int sonicver,
                    LocTraits::LocType UNUSED(tracktype),
                    multimap<int, string> &UNUSED(labels),
                    bool UNUSED(s3kmode)) const {
	last_note = this;
	need_rest = false;

	// Print durations.
	if (notesprinted == 0)
		out << "\tdc.b\t";

	print_dac_sample(out, val, sonicver, notesprinted == 0);

	if (++notesprinted == 12) {
		out << endl;
		notesprinted = 0;
	}
}

void FMPSGNote::print(ostream &out,
                      int sonicver,
                      LocTraits::LocType tracktype,
                      multimap<int, string> &UNUSED(labels),
                      bool UNUSED(s3kmode)) const {
	last_note = this;
	need_rest = false;

	// Print durations.
	if (notesprinted == 0)
		out << "\tdc.b\t";

	static string fmpsglut[] = {
		"nRst",
		"nC0" , "nCs0", "nD0" , "nEb0", "nE0" , "nF0" , "nFs0", "nG0" , "nAb0", "nA0" , "nBb0", "nB0" ,
		"nC1" , "nCs1", "nD1" , "nEb1", "nE1" , "nF1" , "nFs1", "nG1" , "nAb1", "nA1" , "nBb1", "nB1" ,
		"nC2" , "nCs2", "nD2" , "nEb2", "nE2" , "nF2" , "nFs2", "nG2" , "nAb2", "nA2" , "nBb2", "nB2" ,
		"nC3" , "nCs3", "nD3" , "nEb3", "nE3" , "nF3" , "nFs3", "nG3" , "nAb3", "nA3" , "nBb3", "nB3" ,
		"nC4" , "nCs4", "nD4" , "nEb4", "nE4" , "nF4" , "nFs4", "nG4" , "nAb4", "nA4" , "nBb4", "nB4" ,
		"nC5" , "nCs5", "nD5" , "nEb5", "nE5" , "nF5" , "nFs5", "nG5" , "nAb5", "nA5" , "nBb5", "nB5" ,
		"nC6" , "nCs6", "nD6" , "nEb6", "nE6" , "nF6" , "nFs6", "nG6" , "nAb6", "nA6" , "nBb6", "nB6" ,
		"nC7" , "nCs7", "nD7" , "nEb7", "nE7" , "nF7" , "nFs7", "nG7" , "nAb7", "nA7" , "nBb7"
	};

	string noteName;
	bool workAround = false;
	if ((tracktype == LocTraits::ePSGInit || tracktype == LocTraits::ePSGTrack) && val != 0x80) {
		unsigned char newbyte = (val + keydisp) & 0x7f;
		if (sonicver >= 3 && (newbyte == 0x53 || newbyte == 0x54)) {
			noteName = newbyte == 0x54 ? "nMaxPSG2" : "nMaxPSG1";
		} else if (sonicver <= 2 && newbyte == 0x46) {
			noteName = "nMaxPSG";
		} else if (sonicver == 1 && (newbyte & 1) == 0 && newbyte >= 0x4c) {
			// Workaround for xm2smps/xm3smps/xm4smps songs.
			workAround = true;
			noteName = "nMaxPSG";
		}
	}

	if (noteName.length() != 0) {
		if (keydisp != 0) {
			char buf[30];
			snprintf(buf, sizeof(buf), "-$%X)&$FF", static_cast<unsigned>(keydisp));
			if (workAround) {
				cerr << "Converting PSG noise note $" << hex << setw(2) << setfill('0')
					 << uppercase << static_cast<int>(val) << nouppercase
					 << " from xm*smps with broken transposition '$"
					 << hex << setw(2) << setfill('0') << uppercase << static_cast<int>(keydisp)
					 << nouppercase << "' to '(" << noteName << buf << "'" << endl;
				if ((val - keydisp) >= 0xE0 || (val - keydisp) <= 0x80) {
					cerr << "Error: however, this conversion will result in an invalid note.\n"
						 << "You must edit the channel's transposition and the above note\n"
						 << "manually in order to fix this. Blame Tweaker, not me." << endl;
				}
			}
			PrintName(out, "(" + noteName + buf, notesprinted == 0);
		} else {
			PrintName(out, noteName, notesprinted == 0);
		}
	} else {
		size_t note = val - 0x80;
		if (note >= sizeof(fmpsglut) / sizeof(fmpsglut[0])) {
			PrintHex2Pre(out, val, notesprinted == 0);
		} else {
			PrintName(out, fmpsglut[note], notesprinted == 0);
		}
	}

	if (++notesprinted == 12) {
		out << endl;
		notesprinted = 0;
	}
}

template<bool noret>
void CoordFlagNoParams<noret>::print(ostream &out,
                                     int sonicver,
                                     LocTraits::LocType UNUSED(tracktype),
                                     multimap<int, string> &UNUSED(labels),
                                     bool UNUSED(s3kmode)) const {
	// Note-like macros:
	string s;
	bool notelike = false;
	if (sonicver >= 3) {
		switch (val) {
			case 0xe2:
				s = "smpsFade";
				break;   // For $E2, $FF
			case 0xe3:
				s = "smpsStopFM";
				break;
			case 0xe7:
				s = "smpsNoAttack";
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
		switch (val) {
			case 0xe3:
				s = "smpsReturn";
				break;
			case 0xe4:
				s = "smpsFade";
				break;
			case 0xe7:
				s = "smpsNoAttack";
				notelike = true;
				break;
			case 0xed:
				s = "smpsClearPush";
				break;   // Sonic 1 version
			case 0xee:
				if (sonicver == 1) s = "smpsStopSpecial";
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
		if (notesprinted == 0)
			out << "\tdc.b\t";

		PrintName(out, s, notesprinted == 0);

		if (++notesprinted == 12) {
			out << endl;
			notesprinted = 0;
		}
	} else {
		if (notesprinted != 0)
			out << endl;
		notesprinted = 0;

		if (s.size() == 0) {
			out << "\tdc.b\t";
			PrintHex2(out, val, false);
		} else {
			out << "\t";
			PrintName(out, s.c_str(), true);
		}

		out << endl;
	}
}

void BaseNote::print_psg_tone(ostream &out, int tone, int sonicver,
                              bool last) {
	if (tone == 0) {
		PrintHex2(out, tone, true);
		return;
	}

	if (sonicver >= 3)
		out << "sTone_";
	else
		out << "fTone_";

	out << hex << setw(2) << setfill('0') << uppercase
	    << tone << nouppercase;

	if (!last)
		out << ", ";
}

template<bool noret>
void CoordFlag1ParamByte<noret>::print(ostream &out,
                                       int sonicver,
                                       LocTraits::LocType tracktype,
                                       multimap<int, string> &UNUSED(labels),
                                       bool UNUSED(s3kmode)) const {
	if (notesprinted != 0)
		out << endl;
	notesprinted = 0;
	need_rest = true;

	string s;
	bool metacf = false;
	if (sonicver >= 3) {
		switch (val) {
			case 0xe0:
				s = "smpsPan"  ;
				break;
			case 0xe1:
				s = "smpsDetune";
				break;
			case 0xe2:
				s = "smpsFade" ;
				break;   // For $E2, XX with XX != $FF
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
				break;   // Case with param >= 0
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
		switch (val) {
			case 0xe0:
				s = "smpsPan" ;
				break;
			case 0xe1:
				s = "smpsDetune";
				break;
			case 0xe2:
				s = "smpsNop" ;
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
				//case 0xed:  s = ""        ; break;  // Sonic 2 version
		}
	}

	if (s.size() == 0) {
		out << "\tdc.b\t";
		PrintHex2(out, val, false);
	} else if (metacf) {
		out << "\t" << s << endl;
		return;
	} else
		PrintMacro(out, s.c_str());

	if (val == 0xe0) {
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
	} else if (val == 0xf5)
		BaseNote::print_psg_tone(out, param, sonicver, true);
	else if (sonicver >= 3 && val == 0xef && tracktype == LocTraits::ePSGTrack)
		BaseNote::print_psg_tone(out, param, sonicver, true);
	else if (sonicver >= 3 && val == 0xea)
		print_dac_sample(out, param, sonicver, true);
	else
		PrintHex2(out, param, true);
	out << endl;
}

void CoordFlagChgKeydisp::print(ostream &out,
                                int UNUSED(sonicver),
                                LocTraits::LocType UNUSED(tracktype),
                                multimap<int, string> &UNUSED(labels),
                                bool UNUSED(s3kmode)) const {
	if (notesprinted != 0)
		out << endl;
	notesprinted = 0;
	need_rest = true;
	PrintMacro(out, "smpsChangeTransposition");
	PrintHex2(out, param, true);
	out << endl;
}

template<bool noret>
void CoordFlag2ParamBytes<noret>::print(ostream &out,
                                        int sonicver,
                                        LocTraits::LocType UNUSED(tracktype),
                                        multimap<int, string> &UNUSED(labels),
                                        bool UNUSED(s3kmode)) const {
	if (notesprinted != 0)
		out << endl;
	notesprinted = 0;
	need_rest = true;

	string s;
	bool metacf = false;
	if (sonicver >= 3) {
		switch (val) {
			case 0xe5:
				s = "smpsFMAlterVol";
				break;
			case 0xee:
				s = "smpsFMICommand";
				break;
			case 0xef:
				s = "smpsSetvoice";
				break;   // Case with param < 0
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

	if (s.size() == 0) {
		out << "\tdc.b\t";
		PrintHex2(out, val, false);
	} else
		PrintMacro(out, s.c_str());

	if (!metacf)
		PrintHex2(out, param1, false);

	PrintHex2(out, param2, true);
	out << endl;
}

template<bool noret>
void CoordFlag3ParamBytes<noret>::print(ostream &out,
                                        int sonicver,
                                        LocTraits::LocType UNUSED(tracktype),
                                        multimap<int, string> &UNUSED(labels),
                                        bool UNUSED(s3kmode)) const {
	if (notesprinted != 0)
		out << endl;
	notesprinted = 0;
	need_rest = true;

	string s;
	bool metacf = false;
	if (sonicver >= 3) {
		switch (val) {
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

	if (s.size() == 0) {
		out << "\tdc.b\t";
		PrintHex2(out, val, false);
	} else
		PrintMacro(out, s.c_str());

	if (!metacf)
		PrintHex2(out, param1, false);

	PrintHex2(out, param2, false);
	PrintHex2(out, param3, true);
	out << endl;
}

template<bool noret>
void CoordFlag4ParamBytes<noret>::print(ostream &out,
                                        int sonicver,
                                        LocTraits::LocType UNUSED(tracktype),
                                        multimap<int, string> &UNUSED(labels),
                                        bool UNUSED(s3kmode)) const {
	if (notesprinted != 0)
		out << endl;
	notesprinted = 0;
	need_rest = true;

	string s;
	bool metacf = false;
	if (sonicver >= 3) {
		switch (val) {
			case 0xf0:
				s = "smpsModSet";
				break;
			case 0xfe:
				s = "smpsFM3SpecialMode";
				break;
		}
	} else {
		switch (val) {
			case 0xf0:
				s = "smpsModSet";
				break;
		}
	}

	if (s.size() == 0) {
		out << "\tdc.b\t";
		PrintHex2(out, val, false);
	} else
		PrintMacro(out, s.c_str());

	if (!metacf)
		PrintHex2(out, param1, false);

	PrintHex2(out, param2, false);
	PrintHex2(out, param3, false);
	PrintHex2(out, param4, true);
	out << endl;
}

template<bool noret>
void CoordFlag5ParamBytes<noret>::print(ostream &out,
                                        int sonicver,
                                        LocTraits::LocType UNUSED(tracktype),
                                        multimap<int, string> &UNUSED(labels),
                                        bool UNUSED(s3kmode)) const {
	if (notesprinted != 0)
		out << endl;
	notesprinted = 0;
	need_rest = true;

	string s;
	bool metacf = false;
	if (sonicver >= 3) {
		switch (val) {
			case 0xff:
				metacf = true;
				switch (param1) {
					case 0x05:
						s = "smpsSSGEG"   ;
						break;
				}
				break;
		}
	}

	if (s.size() == 0) {
		out << "\tdc.b\t";
		PrintHex2(out, val, false);
	} else
		PrintMacro(out, s.c_str());

	if (!metacf)
		PrintHex2(out, param1, false);

	PrintHex2(out, param2, false);
	PrintHex2(out, param3, false);
	PrintHex2(out, param4, false);
	PrintHex2(out, param5, true);
	out << endl;
}

template<bool noret>
void CoordFlagPointerParam<noret>::print(ostream &out,
                                         int UNUSED(sonicver),
                                         LocTraits::LocType UNUSED(tracktype),
                                         multimap<int, string> &labels,
                                         bool UNUSED(s3kmode)) const {
	if (notesprinted != 0)
		out << endl;
	notesprinted = 0;
	need_rest = true;

	if (val == 0xf6)
		PrintMacro(out, "smpsJump");
	else if (val == 0xf8) {
		last_note = 0;
		PrintMacro(out, "smpsCall");
	} else if (val == 0xfc) // Sonic 3 only
		PrintMacro(out, "smpsContinuousLoop");

	multimap<int, string>::iterator it = labels.find(jumptarget);
	out << it->second << endl;
}

template<bool noret>
void CoordFlagPointer1ParamByte<noret>::print(ostream &out,
                                              int sonicver,
                                              LocTraits::LocType UNUSED(tracktype),
                                              multimap<int, string> &labels,
                                              bool UNUSED(s3kmode)) const {
	if (notesprinted != 0)
		out << endl;
	notesprinted = 0;
	need_rest = true;

	string s;
	bool metacf = false;
	if (sonicver >= 3) {
		switch (val) {
			case 0xeb:
				s = "smpsConditionalJump";
				break;
		}
	}

	if (s.size() == 0) {
		out << "\tdc.b\t";
		PrintHex2(out, val, false);
	} else
		PrintMacro(out, s.c_str());

	if (!metacf)
		PrintHex2(out, param1, false);

	multimap<int, string>::iterator it = labels.find(jumptarget);
	out << it->second << endl;
}

template<bool noret>
void CoordFlagPointer2ParamBytes<noret>::print(ostream &out,
                                               int sonicver,
                                               LocTraits::LocType UNUSED(tracktype),
                                               multimap<int, string> &labels,
                                               bool UNUSED(s3kmode)) const {
	if (notesprinted != 0)
		out << endl;
	notesprinted = 0;
	need_rest = true;

	string s;
	bool metacf = false;
	if (sonicver >= 3) {
		switch (val) {
			case 0xf7:
				s = "smpsLoop"  ;
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
		switch (val) {
			case 0xf7:
				s = "smpsLoop"  ;
				break;
		}
	}

	if (s.size() == 0) {
		out << "\tdc.b\t";
		PrintHex2(out, val, false);
	} else
		PrintMacro(out, s.c_str());

	if (!metacf) {
		PrintHex2(out, param1, false);
		PrintHex2(out, param2, false);
	}
	multimap<int, string>::iterator it = labels.find(jumptarget);

	if (it != labels.end())
		out << it->second;
	else
		PrintHex4(out, jumptarget, true);

	if (metacf) {
		out << ", ";
		PrintHex2(out, param2, true);
	}
	out << endl;
}

#include <iostream>
void InstantiateTemplates() {
	multimap<int, string> labels;
	CoordFlagNoParams<true > ft0(0, 0);
	CoordFlagNoParams<false> ff0(0, 0);
	CoordFlag1ParamByte<true > ft1(0, 0, 0);
	CoordFlag1ParamByte<false> ff1(0, 0, 0);
	CoordFlag2ParamBytes<true > ft2(0, 0, 0, 0);
	CoordFlag2ParamBytes<false> ff2(0, 0, 0, 0);
	CoordFlag3ParamBytes<true > ft3(0, 0, 0, 0, 0);
	CoordFlag3ParamBytes<false> ff3(0, 0, 0, 0, 0);
	CoordFlag4ParamBytes<true > ft4(0, 0, 0, 0, 0, 0);
	CoordFlag4ParamBytes<false> ff4(0, 0, 0, 0, 0, 0);
	CoordFlag5ParamBytes<true > ft5(0, 0, 0, 0, 0, 0, 0);
	CoordFlag5ParamBytes<false> ff5(0, 0, 0, 0, 0, 0, 0);
	CoordFlagPointerParam<true > ftp0(0, 0, 0);
	CoordFlagPointerParam<false> ffp0(0, 0, 0);
	CoordFlagPointer1ParamByte<true > ftp1(0, 0, 0, 0);
	CoordFlagPointer1ParamByte<false> ffp1(0, 0, 0, 0);
	CoordFlagPointer2ParamBytes<true > ftp2(0, 0, 0, 0, 0);
	CoordFlagPointer2ParamBytes<false> ffp2(0, 0, 0, 0, 0);
	ft0.print(cout, 1, LocTraits::eHeader, labels, false);
	ff0.print(cout, 1, LocTraits::eHeader, labels, false);
	ft1.print(cout, 1, LocTraits::eHeader, labels, false);
	ff1.print(cout, 1, LocTraits::eHeader, labels, false);
	ft2.print(cout, 1, LocTraits::eHeader, labels, false);
	ff2.print(cout, 1, LocTraits::eHeader, labels, false);
	ft3.print(cout, 1, LocTraits::eHeader, labels, false);
	ff3.print(cout, 1, LocTraits::eHeader, labels, false);
	ft4.print(cout, 1, LocTraits::eHeader, labels, false);
	ff4.print(cout, 1, LocTraits::eHeader, labels, false);
	ft5.print(cout, 1, LocTraits::eHeader, labels, false);
	ff5.print(cout, 1, LocTraits::eHeader, labels, false);
	ftp0.print(cout, 1, LocTraits::eHeader, labels, false);
	ffp0.print(cout, 1, LocTraits::eHeader, labels, false);
	ftp1.print(cout, 1, LocTraits::eHeader, labels, false);
	ffp1.print(cout, 1, LocTraits::eHeader, labels, false);
	ftp2.print(cout, 1, LocTraits::eHeader, labels, false);
	ffp2.print(cout, 1, LocTraits::eHeader, labels, false);
}
