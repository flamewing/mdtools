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

#ifndef _SONGTRACK_H_
#define _SONGTRACK_H_

#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include "fmvoice.h"

#ifdef UNUSED
#elif defined(__GNUC__)
#	define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
#	define UNUSED(x) /*@unused@*/ x
#elif defined(__cplusplus)
#	define UNUSED(x)
#else
#	define UNUSED(x) x
#endif

struct LocTraits {
	int loc;
	enum LocType {
		eHeader = 0,
		eDACInit,
		ePCMInit,
		ePWMInit,
		eFMInit,
		ePSGInit,
		eDACTrack,
		ePCMTrack,
		ePWMTrack,
		eFMTrack,
		ePSGTrack,
		eVoices,
		eExtVoices
	} type;
	unsigned char keydisp;

	LocTraits(int l, LocType t, unsigned char k = 0)
		: loc(l), type(t), keydisp(k)
	{       }
	bool operator<(LocTraits const &other) const {
		return type > other.type || (type == other.type && loc < other.loc);
	}
};

class BaseNote {
protected:
	static size_t notesprinted;
	static BaseNote const *last_note;
	static bool need_rest;
	unsigned char val;
	unsigned char keydisp;
public:
	BaseNote(unsigned char v, unsigned char k) : val(v), keydisp(k) {  }
	virtual ~BaseNote() {  }
	template<typename IO>
	static BaseNote *read(std::istream &in, int sonicver, int offset,
	                      std::string const &projname, LocTraits::LocType tracktype,
	                      std::multimap<int, std::string> &labels,
	                      int &last_voc, unsigned char keydisp);
	void write(std::ostream &out, int sonicver, size_t offset) const;
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const = 0;
	static void force_linebreak(std::ostream &out, bool force = false);
	static void print_psg_tone(std::ostream &out, int tone, int sonicver, bool last);
	virtual bool ends_track() const {
		return false;
	}
	virtual bool has_pointer() const {
		return false;
	}
	virtual int get_pointer() const {
		return 0;
	}
	virtual bool is_rest() const {
		return false;
	}
	virtual unsigned char get_keydisp() const {
		return keydisp;
	}
};

class RealNote : public BaseNote {
public:
	RealNote(unsigned char v, unsigned char k) : BaseNote(v, k) {  }
	virtual bool is_rest() const {
		return val == 0x80;
	}
};

class Duration : public BaseNote {
public:
	Duration(unsigned char v, unsigned char k) : BaseNote(v, k) {  }
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const;
};

class NullNote : public BaseNote {
public:
	NullNote() : BaseNote(0, 0) {  }
	virtual void print(std::ostream &UNUSED(out), int UNUSED(sonicver),
	                   LocTraits::LocType UNUSED(tracktype),
	                   std::multimap<int, std::string> &UNUSED(labels),
	                   bool UNUSED(s3kmode)) const
	{       }
};

class FMVoice : public BaseNote {
	fm_voice voc;
	int id;
public:
	FMVoice(std::istream &in, int sonicver, int n);
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const;
};

class DACNote : public RealNote {
public:
	DACNote(unsigned char v, unsigned char k) : RealNote(v, k) {  }
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const;
};

class FMPSGNote : public RealNote {
public:
	FMPSGNote(unsigned char v, unsigned char k) : RealNote(v, k) {  }
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const;
};

template<bool noret>
class CoordFlagNoParams : public BaseNote {
public:
	CoordFlagNoParams(unsigned char v, unsigned char k) : BaseNote(v, k) {  }
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const;
	virtual bool ends_track() const {
		return noret;
	}
};

template<bool noret>
class CoordFlag1ParamByte : public BaseNote {
protected:
	unsigned char param;
public:
	CoordFlag1ParamByte(unsigned char v, unsigned char k, unsigned char p) : BaseNote(v, k), param(p) {  }
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const;
	virtual bool ends_track() const {
		return noret;
	}
};

class CoordFlagChgKeydisp : public BaseNote {
protected:
	unsigned char param;
public:
	CoordFlagChgKeydisp(unsigned char v, unsigned char k, unsigned char p) : BaseNote(v, k), param(p) {  }
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const;
	virtual unsigned char get_keydisp() const {
		return keydisp + param;
	}
};

template<bool noret>
class CoordFlag2ParamBytes : public BaseNote {
protected:
	unsigned char param1, param2;
public:
	CoordFlag2ParamBytes(unsigned char v, unsigned char k, unsigned char p1, unsigned char p2)
		: BaseNote(v, k), param1(p1), param2(p2) {  }
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const;
	virtual bool ends_track() const {
		return noret;
	}
};

template<bool noret>
class CoordFlag3ParamBytes : public BaseNote {
protected:
	unsigned char param1, param2, param3;
public:
	CoordFlag3ParamBytes(unsigned char v, unsigned char k, unsigned char p1, unsigned char p2,
	                     unsigned char p3)
		: BaseNote(v, k), param1(p1), param2(p2), param3(p3) {  }
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const;
	virtual bool ends_track() const {
		return noret;
	}
};

template<bool noret>
class CoordFlag4ParamBytes : public BaseNote {
protected:
	unsigned char param1, param2, param3, param4;
public:
	CoordFlag4ParamBytes(unsigned char v, unsigned char k, unsigned char p1, unsigned char p2,
	                     unsigned char p3, unsigned char p4)
		: BaseNote(v, k), param1(p1), param2(p2), param3(p3), param4(p4) {  }
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const;
	virtual bool ends_track() const {
		return noret;
	}
};

template<bool noret>
class CoordFlag5ParamBytes : public BaseNote {
protected:
	unsigned char param1, param2, param3, param4, param5;
public:
	CoordFlag5ParamBytes(unsigned char v, unsigned char k, unsigned char p1, unsigned char p2,
	                     unsigned char p3, unsigned char p4, unsigned char p5)
		: BaseNote(v, k), param1(p1), param2(p2), param3(p3), param4(p4),
		  param5(p5) {  }
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const;
	virtual bool ends_track() const {
		return noret;
	}
};

template<bool noret>
class CoordFlagPointerParam : public BaseNote {
protected:
	int jumptarget;
public:
	CoordFlagPointerParam(unsigned char v, unsigned char k, int p) : BaseNote(v, k), jumptarget(p) {  }
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const;
	virtual bool ends_track() const {
		return noret;
	}
	virtual bool has_pointer() const {
		return true;
	}
	virtual int get_pointer() const {
		return jumptarget;
	}
};

template<bool noret>
class CoordFlagPointer1ParamByte : public BaseNote {
protected:
	int jumptarget;
	unsigned char param1;
public:
	CoordFlagPointer1ParamByte(unsigned char v, unsigned char k, unsigned char p1, int ptr)
		: BaseNote(v, k), jumptarget(ptr), param1(p1) {  }
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const;
	virtual bool ends_track() const {
		return noret;
	}
	virtual bool has_pointer() const {
		return true;
	}
	virtual int get_pointer() const {
		return jumptarget;
	}
};

template<bool noret>
class CoordFlagPointer2ParamBytes : public BaseNote {
protected:
	int jumptarget;
	unsigned char param1, param2;
public:
	CoordFlagPointer2ParamBytes(unsigned char v, unsigned char k, unsigned char p1, unsigned char p2, int ptr)
		: BaseNote(v, k), jumptarget(ptr), param1(p1), param2(p2) {  }
	virtual void print(std::ostream &out, int sonicver, LocTraits::LocType tracktype,
	                   std::multimap<int, std::string> &labels, bool s3kmode) const;
	virtual bool ends_track() const {
		return noret;
	}
	virtual bool has_pointer() const {
		return true;
	}
	virtual int get_pointer() const {
		return jumptarget;
	}
};

#endif // _SONGTRACK_H_
