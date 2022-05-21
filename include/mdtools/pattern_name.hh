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

#ifndef PATTERN_NAME_HH
#define PATTERN_NAME_HH

#include <mdcomp/bigendian_io.hh>

#include <cstdint>

// Possible flips for tiles in pattern names.
enum FlipMode : uint16_t { NoFlip = 0, XFlip = 1, YFlip = 2, XYFlip = 3 };

// Convenience operator for flip modes.
constexpr inline FlipMode operator^(
        FlipMode const left, FlipMode const right) noexcept {
    return static_cast<FlipMode>(
            static_cast<uint16_t>(left) ^ static_cast<uint16_t>(right));
}

// A couple more operations on flip modes: flipping on X, on Y and on both.
constexpr inline FlipMode flip_x(FlipMode const value) noexcept {
    return value ^ XFlip;
}
constexpr inline FlipMode flip_y(FlipMode const value) noexcept {
    return value ^ YFlip;
}
constexpr inline FlipMode flip_xy(FlipMode const value) noexcept {
    return value ^ XYFlip;
}

enum PaletteLine : uint16_t { Line0 = 0, Line1 = 1, Line2 = 2, Line3 = 3 };

// Convenience operators for palette lines.
constexpr inline PaletteLine operator+(
        PaletteLine const left, uint32_t const right) noexcept {
    return static_cast<PaletteLine>((static_cast<uint32_t>(left) + right) % 4);
}
constexpr inline PaletteLine operator-(
        PaletteLine const left, uint32_t const right) noexcept {
    return static_cast<PaletteLine>((static_cast<uint32_t>(left) - right) % 4);
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
        priority_mask  = 1U << priority_shift,
        no_priority    = 0
    };
    uint16_t pattern_name{0U};

public:
    // Constructors.
    constexpr Pattern_Name() noexcept = default;
    constexpr explicit Pattern_Name(uint16_t value) noexcept : pattern_name(value) {}
    constexpr Pattern_Name(Pattern_Name const&) noexcept = default;
    constexpr Pattern_Name(Pattern_Name&&) noexcept      = default;
    // Assignment operators.
    Pattern_Name& operator=(Pattern_Name const&) noexcept = default;
    Pattern_Name& operator=(Pattern_Name&&) noexcept = default;
    ~Pattern_Name() noexcept                         = default;

    // Getters.
    [[nodiscard]] constexpr uint16_t get_tile() const noexcept {
        return (pattern_name & tile_mask);
    }
    [[nodiscard]] constexpr FlipMode get_flip() const noexcept {
        return static_cast<FlipMode>((pattern_name & xyflip_mask) >> xyflip_shift);
    }
    [[nodiscard]] constexpr PaletteLine get_palette() const noexcept {
        return static_cast<PaletteLine>((pattern_name & palette_mask) >> palette_shift);
    }
    [[nodiscard]] constexpr bool high_priority() const noexcept {
        return (pattern_name & priority_mask) != 0;
    }

    // Setters.
    constexpr void set_tile(uint16_t const tile) noexcept {
        pattern_name = (pattern_name & ~tile_mask) | (tile & tile_mask);
    }
    constexpr void set_flip(FlipMode const flip) noexcept {
        pattern_name = (pattern_name & ~xyflip_mask) | static_cast<uint16_t>(static_cast<uint32_t>(flip) << xyflip_shift);
    }
    constexpr void set_palette(PaletteLine const palette) noexcept {
        pattern_name = (pattern_name & ~palette_mask) | static_cast<uint16_t>(static_cast<uint32_t>(palette) << palette_shift);
    }
    constexpr void set_priority(bool const priority) noexcept {
        pattern_name = (pattern_name & ~priority_mask) | (priority ? priority_mask : no_priority);
    }

    // Comparison operator for STL containers and algorithms. Only the tile is
    // important, flip, palette and priority are ignored.
    constexpr bool operator<(Pattern_Name const& right) const noexcept {
        return get_tile() < right.get_tile();
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
        Pattern_Name value(*this);
        return value += delta;
    }
    // Get new pattern name equal to old plus offset. Saturates at tile_mask.
    constexpr friend Pattern_Name operator+(
            uint16_t const delta, Pattern_Name const& right) noexcept {
        return right + delta;
    }
    // Prefix increment pattern name. Saturates at tile_mask.
    constexpr Pattern_Name& operator++() noexcept {
        return operator+=(1);
    }
    // Postfix increment pattern name. Saturates at tile_mask.
    constexpr Pattern_Name operator++(int) noexcept {
        Pattern_Name value(*this);
        ++(*this);
        return value;
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
        Pattern_Name value(*this);
        return value -= delta;
    }
    // Prefix decrement pattern name. Saturates at 0.
    constexpr Pattern_Name& operator--() noexcept {
        return operator-=(1);
    }
    // Prefix decrement pattern name. Saturates at 0.
    constexpr Pattern_Name operator--(int) noexcept {
        Pattern_Name value(*this);
        --(*this);
        return value;
    }

    void write(std::ostream& output) const noexcept {
        BigEndian::Write2(output, pattern_name);
    }
};

#endif    // PATTERN_NAME_HH
