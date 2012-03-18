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

#include "songtrack.h"

#include <istream>
#include <ostream>
#include <iomanip>
#include <cstdio>

#include "bigendian_io.h"

size_t BaseNote::notesprinted = 0;
BaseNote const *BaseNote::last_note = 0;
bool BaseNote::need_rest = false;

void BaseNote::force_linebreak(std::ostream& out, bool force)
{
	if (force)
		out << std::endl;
	
	if (notesprinted != 0)
		out << std::endl;
	notesprinted = 0; 
}

void BaseNote::write(std::ostream& out, int sonicver, size_t offset) const
{
}

void Duration::print(std::ostream& out,
                     int sonicver,
                     LocTraits::LocType tracktype,
                     std::multimap<int,std::string>& labels,
                     bool s3kmode) const
{
	if (s3kmode && last_note && last_note->is_rest() && need_rest)
		last_note->print(out, sonicver, tracktype, labels, s3kmode);

	need_rest = true;
	// Print durations.
	if (notesprinted == 0)
		out << "\tdc.b\t";

	PrintHex2Pre(out, val, notesprinted == 0);

	if (++notesprinted == 12)
	{
		out << std::endl;
		notesprinted = 0;
	}
}

FMVoice::FMVoice(std::istream& in, int sonicver, int n)
	: BaseNote(0x00), id(n)
{
	voc.read(in, sonicver);
}

void FMVoice::print(std::ostream& out,
                    int sonicver,
                    LocTraits::LocType tracktype,
                    std::multimap<int,std::string>& labels,
                    bool s3kmode) const
{
	voc.print(out, sonicver, id);
}

static void print_dac_sample(std::ostream& out, int val, int sonicver, bool flag)
{
	static std::string s12daclut[] = {
		"nRst", "dKick", "dSnare", "dClap", "dScratch", "dTimpani", "dHiTom",
		"dVLowClap", "dHiTimpani", "dMidTimpani", "dLowTimpani", "dVLowTimpani",
		"dMidTom", "dLowTom", "dFloorTom", "dHiClap", "dMidClap", "dLowClap"
	};

	static std::string s3daclut[] = {
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

	static std::string skdaclut[] = {
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

	static std::string s3ddaclut[] = {
		"nRst", "dSnareS3", "dHighTom", "dMidTomS3", "dLowTomS3", "dFloorTomS3",
		"dKickS3", "dMuffledSnare", "dCrashCymbal", "dCrashCymbal2", "dLowMetalHit",
		"dMetalHit", "dHighMetalHit", "dHigherMetalHit", "dMidMetalHit",
		"dClapS3", "dElectricHighTom", "dElectricMidTom", "dElectricLowTom",
		"dElectricFloorTom", "dTightSnare", "dMidpitchSnare", "dLooseSnare",
		"dLooserSnare", "dHiTimpaniS3", "dLowTimpaniS3", "dMidTimpaniS3",
		"dQuickLooseSnare", "dClick", "dPowerKick", "dQuickGlassCrash",
		"dIntroKick","dFinalFightMetalCrash"
	};

	if (sonicver == 5 && val - 0x80 <= sizeof(s3ddaclut)/sizeof(s3ddaclut[0]))
		PrintName(out, s3ddaclut[val - 0x80], flag);
	else if (sonicver == 4 && val - 0x80 <= sizeof(skdaclut)/sizeof(skdaclut[0]))
		PrintName(out, skdaclut[val - 0x80], flag);
	else if (sonicver == 3 && val - 0x80 <= sizeof(s3daclut)/sizeof(s3daclut[0]))
		PrintName(out, s3daclut[val - 0x80], flag);
	else if (sonicver == 1 && (val < 0x84 || (val >= 0x88 && val <=0x8b)))
		PrintName(out, s12daclut[val - 0x80], flag);
	else if (sonicver == 2 && val - 0x80 <= sizeof(s12daclut)/sizeof(s12daclut[0]))
		PrintName(out, s12daclut[val - 0x80], flag);
	else
		PrintHex2Pre(out, val, flag);
}

void DACNote::print(std::ostream& out,
                    int sonicver,
                    LocTraits::LocType tracktype,
                    std::multimap<int,std::string>& labels,
                    bool s3kmode) const
{
	last_note = this;
	need_rest = false;

	// Print durations.
	if (notesprinted == 0)
		out << "\tdc.b\t";
	
	print_dac_sample(out, val, sonicver, notesprinted == 0);
	
	if (++notesprinted == 12)
	{
		out << std::endl;
		notesprinted = 0;
	}
}

void FMPSGNote::print(std::ostream& out,
                      int sonicver,
                      LocTraits::LocType tracktype,
                      std::multimap<int,std::string>& labels,
                      bool s3kmode) const
{
	last_note = this;
	need_rest = false;

	// Print durations.
	if (notesprinted == 0)
		out << "\tdc.b\t";

	static std::string fmpsglut[] = {
		"nRst", "nC0" , "nCs0", "nD0" , "nEb0", "nE0" , "nF0" , "nFs0", "nG0" ,
		"nAb0", "nA0" , "nBb0", "nB0" , "nC1" , "nCs1", "nD1" , "nEb1", "nE1" ,
		"nF1" , "nFs1", "nG1" , "nAb1", "nA1" , "nBb1", "nB1" , "nC2" , "nCs2",
		"nD2" , "nEb2", "nE2" , "nF2" , "nFs2", "nG2" , "nAb2", "nA2" , "nBb2",
		"nB2" , "nC3" , "nCs3", "nD3" , "nEb3", "nE3" , "nF3" , "nFs3", "nG3" ,
		"nAb3", "nA3" , "nBb3", "nB3" , "nC4" , "nCs4", "nD4" , "nEb4", "nE4" ,
		"nF4" , "nFs4", "nG4" , "nAb4", "nA4" , "nBb4", "nB4" , "nC5" , "nCs5",
		"nD5" , "nEb5", "nE5" , "nF5" , "nFs5", "nG5" , "nAb5", "nA5" , "nBb5",
		"nB5" , "nC6" , "nCs6", "nD6" , "nEb6", "nE6" , "nF6" , "nFs6", "nG6" ,
		"nAb6", "nA6" , "nBb6", "nB6" , "nC7" , "nCs7", "nD7" , "nEb7", "nE7" ,
		"nF7" , "nFs7", "nG7" , "nAb7", "nA7" , "nBb7"
	};

	if (val - 0x80 >= sizeof(fmpsglut)/sizeof(fmpsglut[0]))
		PrintHex2Pre(out, val, notesprinted == 0);
	else
		PrintName(out, fmpsglut[val - 0x80], notesprinted == 0);

	if (++notesprinted == 12)
	{
		out << std::endl;
		notesprinted = 0;
	}
}

template<bool noret>
void CoordFlagNoParams<noret>::print(std::ostream& out,
                                     int sonicver,
                                     LocTraits::LocType tracktype,
                                     std::multimap<int,std::string>& labels,
                                     bool s3kmode) const
{
	// Note-like macros:
	std::string s;
	bool notelike = false;
	if (sonicver >= 3)
	{
		switch (val)
		{
			case 0xe2:  s = "smpsFade"       ; break;   // For $E2, $FF
			case 0xe3:  s = "smpsStopFM"     ; break;
			case 0xe7:  s = "smpsNoAttack"   ; notelike = true ; break;
			case 0xe9:  s = "smpsSpindashRev"; break;
			case 0xf2:  s = "smpsStop"       ; break;
			case 0xf9:  s = "smpsReturn"     ; break;
			case 0xfa:  s = "smpsModOff"     ; break;
		}
	}
	else
	{
		switch (val)
		{
			case 0xe3:  s = "smpsReturn"     ; break;
			case 0xe4:  s = "smpsFade"       ; break;
			case 0xe7:  s = "smpsNoAttack"   ; notelike = true ; break;
			case 0xed:  s = "smpsClearPush"  ; break;   // Sonic 1 version
			case 0xee:  if (sonicver == 1) s = "smpsStopSpecial"; break;
			case 0xf1:  s = "smpsModOn"      ; break;
			case 0xf2:  s = "smpsStop"       ; break;
			case 0xf4:  s = "smpsModOff"     ; break;
			case 0xf9:  s = "smpsWeirdD1LRR" ; break;
		}
	}

	if (notelike)
	{
		// Print durations.
		if (notesprinted == 0)
			out << "\tdc.b\t";

		PrintName(out, s, notesprinted == 0);

		if (++notesprinted == 12)
		{
			out << std::endl;
			notesprinted = 0;
		}
	}
	else
	{
		if (notesprinted != 0)
			out << std::endl;
		notesprinted = 0;

		if (s.size() == 0)
		{
			out << "\tdc.b\t";
			PrintHex2(out, val, false);
		}
		else
		{
			out << "\t";
			PrintName(out, s.c_str(), true);
		}

		out << std::endl;
	}
}

void BaseNote::print_psg_tone(std::ostream& out, int tone, int sonicver,
                              bool last)
{
	if (tone == 0)
	{
		PrintHex2(out, tone, true);
		return;
	}
	
	if (sonicver >= 3)
		out << "sTone_";
	else
		out << "fTone_";
	
	out << std::hex << std::setw(2) << std::setfill('0') << std::uppercase
		<< tone << std::nouppercase;
	
	if (!last)
		out << ", ";
}

template<bool noret>
void CoordFlag1ParamByte<noret>::print(std::ostream& out,
                                       int sonicver,
                                       LocTraits::LocType tracktype,
                                       std::multimap<int,std::string>& labels,
                                       bool s3kmode) const
{
	if (notesprinted != 0)
		out << std::endl;
	notesprinted = 0;

	std::string s;
	bool metacf = false;
	if (sonicver >= 3)
	{
		switch (val)
		{
			case 0xe0:  s = "smpsPan"          ; break;
			case 0xe1:  s = "smpsAlterNote"    ; break;
			case 0xe2:  s = "smpsFade"         ; break;   // For $E2, XX with XX != $FF
			case 0xe4:  s = "smpsSetVol"       ; break;
			case 0xe6:  s = "smpsFMAlterVol"   ; break;
			case 0xe8:  s = "smpsNoteFill"     ; break;
			case 0xea:  s = "smpsPlayDACSample"; break;
			case 0xec:  s = "smpsPSGAlterVol"  ; break;
			case 0xed:  s = "smpsSetNote"      ; break;
			case 0xef:  s = "smpsSetvoice"     ; break;   // Case with param >= 0
			case 0xf3:  s = "smpsPSGform"      ; break;
			case 0xf4:  s = "smpsModChange"    ; break;
			case 0xf5:  s = "smpsPSGvoice"     ; break;
			case 0xfb:  s = "smpsAlterPitch"   ; break;
			case 0xfd:  s = "smpsAlternameSMPS"; break;
			case 0xff:
				metacf = true;
				switch (param)
				{
					case 0x02:  s = "smpsHaltMusic"       ; break;
					case 0x07:  s = "smpsResetSpindashRev"; break;
				}
				break;
		}
	}
	else
	{
		switch (val)
		{
			case 0xe0:  s = "smpsPan"         ; break;
			case 0xe1:  s = "smpsAlterNote"   ; break;
			case 0xe2:  s = "smpsNop"         ; break;
			case 0xe5:  s = "smpsChanTempoDiv"; break;
			case 0xe6:  s = "smpsAlterVol"    ; break;
			case 0xe8:  s = "smpsNoteFill"    ; break;
			case 0xe9:  s = "smpsAlterPitch"  ; break;
			case 0xea:  s = "smpsSetTempoMod" ; break;
			case 0xeb:  s = "smpsSetTempoDiv" ; break;
			case 0xec:  s = "smpsPSGAlterVol" ; break;
			case 0xef:  s = "smpsSetvoice"    ; break;
			case 0xf3:  s = "smpsPSGform"     ; break;
			case 0xf5:  s = "smpsPSGvoice"    ; break;
			//case 0xed:  s = ""                ; break;  // Sonic 2 version
		}
	}
	
	if (s.size() == 0)
	{
		out << "\tdc.b\t";
		PrintHex2(out, val, false);
	}
	else if (metacf)
	{
		out << "\t" << s << std::endl;
		return;
	}
	else
		PrintMacro(out, s.c_str());
	
	if (val == 0xe0)
	{
		int dir = param & 0xc0;
		switch (dir)
		{
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
		PrintHex2(out, param&0x3f, true);
	}
	else if (val == 0xf5)
		BaseNote::print_psg_tone(out, param, sonicver, true);
	else if (sonicver >= 3 && val == 0xef && tracktype == LocTraits::ePSGTrack)
		BaseNote::print_psg_tone(out, param, sonicver, true);
	else if (sonicver >= 3 && val == 0xea)
		print_dac_sample(out, param, sonicver, true);
	else
		PrintHex2(out, param, true);
	out << std::endl;
}

template<bool noret>
void CoordFlag2ParamBytes<noret>::print(std::ostream& out,
                                        int sonicver,
                                        LocTraits::LocType tracktype,
                                        std::multimap<int,std::string>& labels,
                                        bool s3kmode) const
{
	if (notesprinted != 0)
		out << std::endl;
	notesprinted = 0;

	std::string s;
	bool metacf = false;
	if (sonicver >= 3)
	{
		switch (val)
		{
			case 0xe5:  s = "smpsFMAlterVol"   ; break;
			case 0xee:  s = "smpsFMICommand"   ; break;
			case 0xef:  s = "smpsSetvoice"     ; break;   // Case with param < 0
			case 0xf1:  s = "smpsModChange2"   ; break;
			case 0xff:
				metacf = true;
				switch (param1)
				{
					case 0x00:  s = "smpsSetTempoMod"     ; break;
					case 0x01:  s = "smpsPlaySound"       ; break;
					case 0x04:  s = "smpsSetTempoDiv"     ; break;
				}
				break;
		}
	}
	
	if (s.size() == 0)
	{
		out << "\tdc.b\t";
		PrintHex2(out, val, false);
	}
	else
		PrintMacro(out, s.c_str());
	
	if (!metacf)
		PrintHex2(out, param1, false);
	
	PrintHex2(out, param2, true);
	out << std::endl;
}

template<bool noret>
void CoordFlag3ParamBytes<noret>::print(std::ostream& out,
                                        int sonicver,
                                        LocTraits::LocType tracktype,
                                        std::multimap<int,std::string>& labels,
                                        bool s3kmode) const
{
	if (notesprinted != 0)
		out << std::endl;
	notesprinted = 0;

	std::string s;
	bool metacf = false;
	if (sonicver >= 3)
	{
		switch (val)
		{
			case 0xff:
				metacf = true;
				switch (param1)
				{
					case 0x06:  s = "smpsFMFlutter"       ; break;
				}
				break;
		}
	}
	
	if (s.size() == 0)
	{
		out << "\tdc.b\t";
		PrintHex2(out, val, false);
	}
	else
		PrintMacro(out, s.c_str());
	
	if (!metacf)
		PrintHex2(out, param1, false);
	
	PrintHex2(out, param2, false);
	PrintHex2(out, param3, true);
	out << std::endl;
}

template<bool noret>
void CoordFlag4ParamBytes<noret>::print(std::ostream& out,
                                        int sonicver,
                                        LocTraits::LocType tracktype,
                                        std::multimap<int,std::string>& labels,
                                        bool s3kmode) const
{
	if (notesprinted != 0)
		out << std::endl;
	notesprinted = 0;
	
	std::string s;
	bool metacf = false;
	if (sonicver >= 3)
	{
		switch (val)
		{
			case 0xf0:  s = "smpsModSet"        ; break;
			case 0xfe:  s = "smpsFM3SpecialMode"; break;
		}
	}
	else
	{
		switch (val)
		{
			case 0xf0:  s = "smpsModSet"       ; break;
		}
	}
	
	if (s.size() == 0)
	{
		out << "\tdc.b\t";
		PrintHex2(out, val, false);
	}
	else
		PrintMacro(out, s.c_str());
	
	if (!metacf)
		PrintHex2(out, param1, false);

	PrintHex2(out, param2, false);
	PrintHex2(out, param3, false);
	PrintHex2(out, param4, true);
	out << std::endl;
}

template<bool noret>
void CoordFlag5ParamBytes<noret>::print(std::ostream& out,
                                        int sonicver,
                                        LocTraits::LocType tracktype,
                                        std::multimap<int,std::string>& labels,
                                        bool s3kmode) const
{
	if (notesprinted != 0)
		out << std::endl;
	notesprinted = 0;

	std::string s;
	bool metacf = false;
	if (sonicver >= 3)
	{
		switch (val)
		{
			case 0xff:
				metacf = true;
				switch (param1)
				{
					case 0x05:  s = "smpsSSGEG"           ; break;
				}
				break;
		}
	}
	
	if (s.size() == 0)
	{
		out << "\tdc.b\t";
		PrintHex2(out, val, false);
	}
	else
		PrintMacro(out, s.c_str());
	
	if (!metacf)
		PrintHex2(out, param1, false);

	PrintHex2(out, param2, false);
	PrintHex2(out, param3, false);
	PrintHex2(out, param4, false);
	PrintHex2(out, param5, true);
	out << std::endl;
}

template<bool noret>
void CoordFlagPointerParam<noret>::print(std::ostream& out,
                                         int sonicver,
                                         LocTraits::LocType tracktype,
                                         std::multimap<int,std::string>& labels,
                                         bool s3kmode) const
{
	if (notesprinted != 0)
		out << std::endl;
	notesprinted = 0;

	if (val == 0xf6)
		PrintMacro(out, "smpsJump");
	else if (val == 0xf8)
	{
		last_note = 0;
		PrintMacro(out, "smpsCall");
	}
	else if (val == 0xfc)   // Sonic 3 only
		PrintMacro(out, "smpsContinuousLoop");

	std::multimap<int,std::string>::iterator it = labels.find(jumptarget);
	out << it->second << std::endl;
}

template<bool noret>
void CoordFlagPointer1ParamByte<noret>::print(std::ostream& out,
                                              int sonicver,
                                              LocTraits::LocType tracktype,
                                              std::multimap<int,std::string>& labels,
                                              bool s3kmode) const
{
	if (notesprinted != 0)
		out << std::endl;
	notesprinted = 0;
	
	std::string s;
	bool metacf = false;
	if (sonicver >= 3)
	{
		switch (val)
		{
			case 0xeb:  s = "smpsConditionalJump"; break;
		}
	}
	
	if (s.size() == 0)
	{
		out << "\tdc.b\t";
		PrintHex2(out, val, false);
	}
	else
		PrintMacro(out, s.c_str());
	
	if (!metacf)
		PrintHex2(out, param1, false);

	std::multimap<int,std::string>::iterator it = labels.find(jumptarget);
	out << it->second << std::endl;
}

template<bool noret>
void CoordFlagPointer2ParamBytes<noret>::print(std::ostream& out,
                                               int sonicver,
                                               LocTraits::LocType tracktype,
                                               std::multimap<int,std::string>& labels,
                                               bool s3kmode) const
{
	if (notesprinted != 0)
		out << std::endl;
	notesprinted = 0;
	
	std::string s;
	bool metacf = false;
	if (sonicver >= 3)
	{
		switch (val)
		{
			case 0xf7:  s = "smpsLoop"          ; break;
			case 0xff:
				metacf = true;
				switch (param1)
				{
					case 0x03:  s = "smpsCopyData"        ; break;
				}
				break;
		}
	}
	else
	{
		switch (val)
		{
			case 0xf7:  s = "smpsLoop"          ; break;
		}
	}
	
	if (s.size() == 0)
	{
		out << "\tdc.b\t";
		PrintHex2(out, val, false);
	}
	else
		PrintMacro(out, s.c_str());
	
	if (!metacf)
	{
		PrintHex2(out, param1, false);
		PrintHex2(out, param2, false);
	}
	std::multimap<int,std::string>::iterator it = labels.find(jumptarget);
	
	if (it != labels.end())
		out << it->second;
	else
		PrintHex4(out, jumptarget, true);
	
	if (metacf)
	{
		out << ", ";
		PrintHex2(out, param2, true);
	}
	out << std::endl;
}

#include <iostream>
void InstantiateTemplates()
{
	std::multimap<int,std::string> labels;
	CoordFlagNoParams<true > ft0(0);
	CoordFlagNoParams<false> ff0(0);
	CoordFlag1ParamByte<true > ft1(0, 0);
	CoordFlag1ParamByte<false> ff1(0, 0);
	CoordFlag2ParamBytes<true > ft2(0, 0, 0);
	CoordFlag2ParamBytes<false> ff2(0, 0, 0);
	CoordFlag3ParamBytes<true > ft3(0, 0, 0, 0);
	CoordFlag3ParamBytes<false> ff3(0, 0, 0, 0);
	CoordFlag4ParamBytes<true > ft4(0, 0, 0, 0, 0);
	CoordFlag4ParamBytes<false> ff4(0, 0, 0, 0, 0);
	CoordFlag5ParamBytes<true > ft5(0, 0, 0, 0, 0, 0);
	CoordFlag5ParamBytes<false> ff5(0, 0, 0, 0, 0, 0);
	CoordFlagPointerParam<true > ftp0(0, 0);
	CoordFlagPointerParam<false> ffp0(0, 0);
	CoordFlagPointer1ParamByte<true > ftp1(0, 0, 0);
	CoordFlagPointer1ParamByte<false> ffp1(0, 0, 0);
	CoordFlagPointer2ParamBytes<true > ftp2(0, 0, 0, 0);
	CoordFlagPointer2ParamBytes<false> ffp2(0, 0, 0, 0);
	ft0.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ff0.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ft1.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ff1.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ft2.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ff2.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ft3.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ff3.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ft4.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ff4.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ft5.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ff5.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ftp0.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ffp0.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ftp1.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ffp1.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ftp2.print(std::cout, 1, LocTraits::eHeader, labels, false);
	ffp2.print(std::cout, 1, LocTraits::eHeader, labels, false);
}
