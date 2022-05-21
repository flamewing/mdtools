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

#ifndef TOOLS_SONG_TRACK_HH
#define TOOLS_SONG_TRACK_HH

#include <mdtools/fmvoice.hh>
#include <mdtools/ignore_unused_variable_warning.hh>

#include <iosfwd>
#include <map>
#include <set>
#include <string>

struct LocTraits {
    int location;
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
    uint8_t key_displacement;

    LocTraits(int location_, LocType type_, uint8_t key_disp = 0) noexcept
            : location(location_), type(type_), key_displacement(key_disp) {}
    bool operator<(LocTraits const& other) const noexcept {
        return type > other.type
               || (type == other.type && location < other.location);
    }
};

class BaseNote {
protected:
    static size_t          notes_printed;
    static BaseNote const* last_note;
    static bool            need_rest;

private:
    uint8_t value;
    uint8_t key_displacement;

protected:
    [[nodiscard]] uint8_t get_value() const noexcept {
        return value;
    }
    [[nodiscard]] uint8_t get_base_key_displacement() const noexcept {
        return key_displacement;
    }

public:
    BaseNote(uint8_t value_, uint8_t key_disp) noexcept
            : value(value_), key_displacement(key_disp) {}
    BaseNote(BaseNote const&) noexcept            = default;
    BaseNote(BaseNote&&) noexcept                 = default;
    BaseNote& operator=(BaseNote const&) noexcept = default;
    BaseNote& operator=(BaseNote&&) noexcept      = default;
    virtual ~BaseNote() noexcept                  = default;
    template <typename IO>
    static BaseNote* read(
            std::istream& input, int sonic_version, int offset,
            std::string const& project_name, LocTraits::LocType track_type,
            std::multimap<int, std::string>& labels, int& last_voc,
            uint8_t key_disp);
    void write(std::ostream& output, int sonic_version, size_t offset) const;
    virtual void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const = 0;
    static void force_line_break(std::ostream& output, bool force = false);
    static void print_psg_tone(
            std::ostream& output, int tone, int sonic_version, bool last);
    [[nodiscard]] virtual bool ends_track() const noexcept {
        return false;
    }
    [[nodiscard]] virtual bool has_pointer() const noexcept {
        return false;
    }
    [[nodiscard]] virtual int get_pointer() const noexcept {
        return 0;
    }
    [[nodiscard]] virtual bool is_rest() const noexcept {
        return false;
    }
    [[nodiscard]] virtual uint8_t get_key_displacement() const noexcept {
        return key_displacement;
    }
};

class RealNote : public BaseNote {
public:
    RealNote(uint8_t value, uint8_t key_displacement) noexcept
            : BaseNote(value, key_displacement) {}
    [[nodiscard]] bool is_rest() const noexcept final {
        return get_value() == 0x80;
    }
};

class Duration : public BaseNote {
public:
    Duration(uint8_t value, uint8_t key_displacement) noexcept
            : BaseNote(value, key_displacement) {}
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
};

class NullNote final : public BaseNote {
public:
    NullNote() noexcept : BaseNote(0, 0) {}
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final {
        ignore_unused_variable_warning(
                output, sonic_version, track_type, labels, s3kmode);
    }
};

class FMVoice final : public BaseNote {
    fm_voice voice{};
    int      index;

public:
    FMVoice(std::istream& input, int sonic_version, int index) noexcept;
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
};

class DACNote : public RealNote {
public:
    DACNote(uint8_t version, uint8_t key_displacement) noexcept
            : RealNote(version, key_displacement) {}
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
};

class FMPSGNote : public RealNote {
public:
    FMPSGNote(uint8_t value, uint8_t key_displacement) noexcept
            : RealNote(value, key_displacement) {}
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
};

template <bool noret>
class CoordFlagNoParams : public BaseNote {
public:
    CoordFlagNoParams(uint8_t value, uint8_t key_displacement) noexcept
            : BaseNote(value, key_displacement) {}
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    [[nodiscard]] bool ends_track() const noexcept final {
        return noret;
    }
};

template <bool noret>
class CoordFlag1ParamByte : public BaseNote {
private:
    uint8_t param;

public:
    CoordFlag1ParamByte(
            uint8_t value, uint8_t key_displacement, uint8_t param_) noexcept
            : BaseNote(value, key_displacement), param(param_) {}
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    [[nodiscard]] bool ends_track() const noexcept final {
        return noret;
    }
};

class CoordFlagChangeKeyDisplacement : public BaseNote {
private:
    uint8_t param;

public:
    CoordFlagChangeKeyDisplacement(
            uint8_t value, uint8_t key_displacement, uint8_t param_) noexcept
            : BaseNote(value, key_displacement), param(param_) {}
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    [[nodiscard]] uint8_t get_key_displacement() const noexcept final {
        return get_base_key_displacement() + param;
    }
};

template <bool noret>
class CoordFlag2ParamBytes : public BaseNote {
private:
    uint8_t param1, param2;

public:
    CoordFlag2ParamBytes(
            uint8_t value, uint8_t key_displacement, uint8_t param_1,
            uint8_t param_2) noexcept
            : BaseNote(value, key_displacement), param1(param_1),
              param2(param_2) {}
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    [[nodiscard]] bool ends_track() const noexcept final {
        return noret;
    }
};

template <bool noret>
class CoordFlag3ParamBytes : public BaseNote {
private:
    uint8_t param1, param2, param3;

public:
    CoordFlag3ParamBytes(
            uint8_t value, uint8_t key_displacement, uint8_t param_1,
            uint8_t param_2, uint8_t param_3) noexcept
            : BaseNote(value, key_displacement), param1(param_1),
              param2(param_2), param3(param_3) {}
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    [[nodiscard]] bool ends_track() const noexcept final {
        return noret;
    }
};

template <bool noret>
class CoordFlag4ParamBytes : public BaseNote {
private:
    uint8_t param1, param2, param3, param4;

public:
    CoordFlag4ParamBytes(
            uint8_t value, uint8_t key_displacement, uint8_t param_1,
            uint8_t param_2, uint8_t param_3, uint8_t param_4) noexcept
            : BaseNote(value, key_displacement), param1(param_1),
              param2(param_2), param3(param_3), param4(param_4) {}
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    [[nodiscard]] bool ends_track() const noexcept final {
        return noret;
    }
};

template <bool noret>
class CoordFlag5ParamBytes : public BaseNote {
private:
    uint8_t param1, param2, param3, param4, param5;

public:
    CoordFlag5ParamBytes(
            uint8_t value, uint8_t key_displacement, uint8_t param_1,
            uint8_t param_2, uint8_t param_3, uint8_t param_4,
            uint8_t param_5) noexcept
            : BaseNote(value, key_displacement), param1(param_1),
              param2(param_2), param3(param_3), param4(param_4),
              param5(param_5) {}
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    [[nodiscard]] bool ends_track() const noexcept final {
        return noret;
    }
};

template <bool noret>
class CoordFlagPointerParam : public BaseNote {
private:
    int jump_target;

public:
    CoordFlagPointerParam(
            uint8_t value, uint8_t key_displacement, int param) noexcept
            : BaseNote(value, key_displacement), jump_target(param) {}
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    [[nodiscard]] bool ends_track() const noexcept final {
        return noret;
    }
    [[nodiscard]] bool has_pointer() const noexcept final {
        return true;
    }
    [[nodiscard]] int get_pointer() const noexcept final {
        return jump_target;
    }
};

template <bool noret>
class CoordFlagPointer1ParamByte : public BaseNote {
private:
    int     jump_target;
    uint8_t param1;

public:
    CoordFlagPointer1ParamByte(
            uint8_t value, uint8_t key_displacement, uint8_t param_1,
            int pointer) noexcept
            : BaseNote(value, key_displacement), jump_target(pointer),
              param1(param_1) {}
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    [[nodiscard]] bool ends_track() const noexcept final {
        return noret;
    }
    [[nodiscard]] bool has_pointer() const noexcept final {
        return true;
    }
    [[nodiscard]] int get_pointer() const noexcept final {
        return jump_target;
    }
};

template <bool noret>
class CoordFlagPointer2ParamBytes : public BaseNote {
private:
    int     jump_target;
    uint8_t param1, param2;

public:
    CoordFlagPointer2ParamBytes(
            uint8_t value, uint8_t key_displacement, uint8_t param_1,
            uint8_t param_2, int pointer) noexcept
            : BaseNote(value, key_displacement), jump_target(pointer),
              param1(param_1), param2(param_2) {}
    void print(
            std::ostream& output, int sonic_version,
            LocTraits::LocType               track_type,
            std::multimap<int, std::string>& labels, bool s3kmode) const final;
    [[nodiscard]] bool ends_track() const noexcept final {
        return noret;
    }
    [[nodiscard]] bool has_pointer() const noexcept final {
        return true;
    }
    [[nodiscard]] int get_pointer() const noexcept final {
        return jump_target;
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

#endif    // TOOLS_SONG_TRACK_HH
