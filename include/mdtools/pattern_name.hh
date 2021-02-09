/*
 * Copyright (C) Flamewing 2014 <flamewing.sonic@gmail.com>
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

#ifndef __PATTERN_NAME_H
#define __PATTERN_NAME_H

#include <mdcomp/bigendian_io.hh>

#include <cstdint>

// Possible flips for tiles in pattern names.
enum FlipMode : uint16_t { NoFlip = 0, XFlip = 1, YFlip = 2, XYFlip = 3 };

// Convenience operator for flip modes.
constexpr inline FlipMode
        operator^(FlipMode const lhs, FlipMode const rhs) noexcept {
    return static_cast<FlipMode>(
            static_cast<uint16_t>(lhs) ^ static_cast<uint16_t>(rhs));
}

// A couple more operations on flip modes: flipping on X, on Y and on both.
constexpr inline FlipMode flip_x(FlipMode const src) noexcept {
    return src ^ XFlip;
}
constexpr inline FlipMode flip_y(FlipMode const src) noexcept {
    return src ^ YFlip;
}
constexpr inline FlipMode flip_xy(FlipMode const src) noexcept {
    return src ^ XYFlip;
}

enum PaletteLine : uint16_t { Line0 = 0, Line1 = 1, Line2 = 2, Line3 = 3 };

// Convenience operators for palette lines.
constexpr inline PaletteLine
        operator+(PaletteLine const lhs, uint32_t const rhs) noexcept {
    return static_cast<PaletteLine>((static_cast<uint32_t>(lhs) + rhs) % 4);
}
constexpr inline PaletteLine
        operator-(PaletteLine const lhs, uint32_t const rhs) noexcept {
    return static_cast<PaletteLine>((static_cast<uint32_t>(lhs) - rhs) % 4);
}

// Class representing a VDP pattern name and abstracting away its pieces.
class Pattern_Name {
private:
    // NO MAGIC NUMBERS.
    enum : uint32_t {
        xyflip_shift   = 11,
        xflip_shift    = xyflip_shift,
        yflip_shift    = xyflip_shift + 1,
        palette_shift  = 13,
        priority_shift = 15,
        tile_mask      = 0x07ffU,
        xflip_mask     = 1U << xflip_shift,
        yflip_mask     = 1U << yflip_shift,
        xyflip_mask    = 3U << xyflip_shift,
        palette_mask   = 3U << palette_shift,
        priority_mask  = 1U << priority_shift
    };
    uint16_t pn{0U};

public:
    // Constructors.
    constexpr Pattern_Name() noexcept = default;
    constexpr explicit Pattern_Name(uint16_t _pn) noexcept : pn(_pn) {}
    constexpr Pattern_Name(Pattern_Name const&) noexcept = default;
    constexpr Pattern_Name(Pattern_Name&&) noexcept      = default;
    // Assignment operators.
    Pattern_Name& operator=(Pattern_Name const&) noexcept = default;
    Pattern_Name& operator=(Pattern_Name&&) noexcept = default;
    ~Pattern_Name() noexcept                         = default;

    // Getters.
    constexpr uint16_t get_tile() const noexcept {
        return (pn & tile_mask);
    }
    constexpr FlipMode get_flip() const noexcept {
        return static_cast<FlipMode>((pn & xyflip_mask) >> xyflip_shift);
    }
    constexpr PaletteLine get_palette() const noexcept {
        return static_cast<PaletteLine>((pn & palette_mask) >> palette_shift);
    }
    constexpr bool high_priority() const noexcept {
        return (pn & priority_mask) != 0;
    }

    // Setters.
    constexpr void set_tile(uint16_t const t) noexcept {
        pn = (pn & ~tile_mask) | (t & tile_mask);
    }
    constexpr void set_flip(FlipMode const f) noexcept {
        pn = (pn & ~xyflip_mask) | (static_cast<uint32_t>(f) << xyflip_shift);
    }
    constexpr void set_palette(PaletteLine const p) noexcept {
        pn = (pn & ~palette_mask) | (static_cast<uint32_t>(p) << palette_shift);
    }
    constexpr void set_priority(bool const b) noexcept {
        pn = (pn & ~priority_mask) | (b ? priority_mask : 0);
    }

    // Comparison operator for STL containers and algorithms. Only the tile is
    // important, flip, palette and priority are ignored.
    constexpr bool operator<(Pattern_Name const& rhs) const noexcept {
        return get_tile() < rhs.get_tile();
    }

    // A few arithmetic operators defined for convenience. They operate only on
    // the tile part.
    // Increment tile by given offset. Saturates at tile_mask.
    constexpr Pattern_Name& operator+=(uint16_t const delta) noexcept {
        if (get_tile() + delta <= tile_mask) {
            set_tile(get_tile() + delta);
        } else {
            set_tile(tile_mask);
        }
        return *this;
    }
    // Get new pattern name equal to old plus offset. Saturates at tile_mask.
    constexpr Pattern_Name operator+(uint16_t const delta) const noexcept {
        Pattern_Name ret(*this);
        return ret += delta;
    }
    // Get new pattern name equal to old plus offset. Saturates at tile_mask.
    constexpr friend Pattern_Name
            operator+(uint16_t const delta, Pattern_Name const& rhs) noexcept {
        return rhs + delta;
    }
    // Prefix increment pattern name. Saturates at tile_mask.
    constexpr Pattern_Name& operator++() noexcept {
        return operator+=(1);
    }
    // Postfix increment pattern name. Saturates at tile_mask.
    constexpr Pattern_Name operator++(int) noexcept {
        Pattern_Name ret(*this);
        ++(*this);
        return ret;
    }
    // Decrement tile by given amount. Saturates at 0.
    constexpr Pattern_Name& operator-=(uint16_t const delta) noexcept {
        if (get_tile() >= delta) {
            set_tile(get_tile() - delta);
        } else {
            set_tile(0);
        }
        return *this;
    }
    // Get new pattern name equal to old minus offset. Saturates at 0.
    constexpr Pattern_Name operator-(uint16_t const delta) const noexcept {
        Pattern_Name ret(*this);
        return ret -= delta;
    }
    // Prefix decrement pattern name. Saturates at 0.
    constexpr Pattern_Name& operator--() noexcept {
        return operator-=(1);
    }
    // Prefix decrement pattern name. Saturates at 0.
    constexpr Pattern_Name operator--(int) noexcept {
        Pattern_Name ret(*this);
        --(*this);
        return ret;
    }

    void write(std::ostream& out) const noexcept {
        BigEndian::Write2(out, pn);
    }
};

#endif    // __PATTERN_NAME_H
