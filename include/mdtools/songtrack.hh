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

#ifndef __TOOLS_SONGTRACK_H
#define __TOOLS_SONGTRACK_H

#include <mdtools/fmvoice.hh>
#include <mdtools/ignore_unused_variable_warning.hh>

#include <iosfwd>
#include <map>
#include <set>
#include <string>

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
    uint8_t keydisp;

    LocTraits(int l, LocType t, uint8_t k = 0) : loc(l), type(t), keydisp(k) {}
    bool operator<(LocTraits const& other) const {
        return type > other.type || (type == other.type && loc < other.loc);
    }
};

class BaseNote {
protected:
    static size_t          notesprinted;
    static BaseNote const* last_note;
    static bool            need_rest;

private:
    uint8_t val;
    uint8_t keydisp;

protected:
    uint8_t get_value() const {
        return val;
    }
    uint8_t get_base_keydisp() const {
        return keydisp;
    }

public:
    BaseNote(uint8_t v, uint8_t k) noexcept : val(v), keydisp(k) {}
    BaseNote(BaseNote const&) noexcept = default;
    BaseNote(BaseNote&&) noexcept      = default;
    BaseNote& operator=(BaseNote const&) noexcept = default;
    BaseNote& operator=(BaseNote&&) noexcept = default;
    virtual ~BaseNote() noexcept             = default;
    template <typename IO>
    static BaseNote* read(
            std::istream& in, int sonicver, int offset,
            std::string const& projname, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, int& last_voc,
            uint8_t keydisp);
    void         write(std::ostream& out, int sonicver, size_t offset) const;
    virtual void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const = 0;
    static void force_linebreak(std::ostream& out, bool force = false);
    static void print_psg_tone(
            std::ostream& out, int tone, int sonicver, bool last);
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
    virtual uint8_t get_keydisp() const {
        return keydisp;
    }
};

class RealNote : public BaseNote {
public:
    RealNote(uint8_t v, uint8_t k) : BaseNote(v, k) {}
    bool is_rest() const final {
        return get_value() == 0x80;
    }
};

class Duration : public BaseNote {
public:
    Duration(uint8_t v, uint8_t k) : BaseNote(v, k) {}
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
};

class NullNote final : public BaseNote {
public:
    NullNote() : BaseNote(0, 0) {}
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final {
        ignore_unused_variable_warning(
                out, sonicver, tracktype, labels, s3kmode);
    }
};

class FMVoice final : public BaseNote {
    fm_voice voc{};
    int      id;

public:
    FMVoice(std::istream& in, int sonicver, int n);
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
};

class DACNote : public RealNote {
public:
    DACNote(uint8_t v, uint8_t k) : RealNote(v, k) {}
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
};

class FMPSGNote : public RealNote {
public:
    FMPSGNote(uint8_t v, uint8_t k) : RealNote(v, k) {}
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
};

template <bool noret>
class CoordFlagNoParams : public BaseNote {
public:
    CoordFlagNoParams(uint8_t v, uint8_t k) : BaseNote(v, k) {}
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    bool ends_track() const final {
        return noret;
    }
};

template <bool noret>
class CoordFlag1ParamByte : public BaseNote {
private:
    uint8_t param;

public:
    CoordFlag1ParamByte(uint8_t v, uint8_t k, uint8_t p)
            : BaseNote(v, k), param(p) {}
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    bool ends_track() const final {
        return noret;
    }
};

class CoordFlagChgKeydisp : public BaseNote {
private:
    uint8_t param;

public:
    CoordFlagChgKeydisp(uint8_t v, uint8_t k, uint8_t p)
            : BaseNote(v, k), param(p) {}
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    uint8_t get_keydisp() const final {
        return get_base_keydisp() + param;
    }
};

template <bool noret>
class CoordFlag2ParamBytes : public BaseNote {
private:
    uint8_t param1, param2;

public:
    CoordFlag2ParamBytes(uint8_t v, uint8_t k, uint8_t p1, uint8_t p2)
            : BaseNote(v, k), param1(p1), param2(p2) {}
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    bool ends_track() const final {
        return noret;
    }
};

template <bool noret>
class CoordFlag3ParamBytes : public BaseNote {
private:
    uint8_t param1, param2, param3;

public:
    CoordFlag3ParamBytes(
            uint8_t v, uint8_t k, uint8_t p1, uint8_t p2, uint8_t p3)
            : BaseNote(v, k), param1(p1), param2(p2), param3(p3) {}
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    bool ends_track() const final {
        return noret;
    }
};

template <bool noret>
class CoordFlag4ParamBytes : public BaseNote {
private:
    uint8_t param1, param2, param3, param4;

public:
    CoordFlag4ParamBytes(
            uint8_t v, uint8_t k, uint8_t p1, uint8_t p2, uint8_t p3,
            uint8_t p4)
            : BaseNote(v, k), param1(p1), param2(p2), param3(p3), param4(p4) {}
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    bool ends_track() const final {
        return noret;
    }
};

template <bool noret>
class CoordFlag5ParamBytes : public BaseNote {
private:
    uint8_t param1, param2, param3, param4, param5;

public:
    CoordFlag5ParamBytes(
            uint8_t v, uint8_t k, uint8_t p1, uint8_t p2, uint8_t p3,
            uint8_t p4, uint8_t p5)
            : BaseNote(v, k), param1(p1), param2(p2), param3(p3), param4(p4),
              param5(p5) {}
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    bool ends_track() const final {
        return noret;
    }
};

template <bool noret>
class CoordFlagPointerParam : public BaseNote {
private:
    int jumptarget;

public:
    CoordFlagPointerParam(uint8_t v, uint8_t k, int p)
            : BaseNote(v, k), jumptarget(p) {}
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    bool ends_track() const final {
        return noret;
    }
    bool has_pointer() const final {
        return true;
    }
    int get_pointer() const final {
        return jumptarget;
    }
};

template <bool noret>
class CoordFlagPointer1ParamByte : public BaseNote {
private:
    int     jumptarget;
    uint8_t param1;

public:
    CoordFlagPointer1ParamByte(uint8_t v, uint8_t k, uint8_t p1, int ptr)
            : BaseNote(v, k), jumptarget(ptr), param1(p1) {}
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    bool ends_track() const final {
        return noret;
    }
    bool has_pointer() const final {
        return true;
    }
    int get_pointer() const final {
        return jumptarget;
    }
};

template <bool noret>
class CoordFlagPointer2ParamBytes : public BaseNote {
private:
    int     jumptarget;
    uint8_t param1, param2;

public:
    CoordFlagPointer2ParamBytes(
            uint8_t v, uint8_t k, uint8_t p1, uint8_t p2, int ptr)
            : BaseNote(v, k), jumptarget(ptr), param1(p1), param2(p2) {}
    void print(
            std::ostream& out, int sonicver, LocTraits::LocType tracktype,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    bool ends_track() const final {
        return noret;
    }
    bool has_pointer() const final {
        return true;
    }
    int get_pointer() const final {
        return jumptarget;
    }
};

// Explicit instantiation of all templates
template class CoordFlagNoParams<true>;
template class CoordFlagNoParams<false>;
template class CoordFlag1ParamByte<true>;
template class CoordFlag1ParamByte<false>;
template class CoordFlag2ParamBytes<true>;
template class CoordFlag2ParamBytes<false>;
template class CoordFlag3ParamBytes<true>;
template class CoordFlag3ParamBytes<false>;
template class CoordFlag4ParamBytes<true>;
template class CoordFlag4ParamBytes<false>;
template class CoordFlag5ParamBytes<true>;
template class CoordFlag5ParamBytes<false>;
template class CoordFlagPointerParam<true>;
template class CoordFlagPointerParam<false>;
template class CoordFlagPointer1ParamByte<true>;
template class CoordFlagPointer1ParamByte<false>;
template class CoordFlagPointer2ParamBytes<true>;
template class CoordFlagPointer2ParamBytes<false>;

#endif    // __TOOLS_SONGTRACK_H
