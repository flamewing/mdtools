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

#ifndef TILE_HH
#define TILE_HH

#include <mdtools/pattern_name.hh>

#include <array>
#include <cassert>
#include <cstring>
#include <iosfwd>
#include <iterator>

using DistTable_t = std::array<std::array<uint32_t, 16>, 16>;

// Base tile class template that represents a tile with nlines lines of lsize
// length each, for a total of lsize * nlines pixels (or half that in bytes).
template <int lsize, int nlines>
class BaseTile {
public:
    // Exported constants.
    static constexpr size_t const Line_size = lsize;
    static constexpr size_t const Num_lines = nlines;
    static constexpr size_t const Tile_size = nlines * lsize;
    static constexpr size_t const Byte_size = nlines * lsize / 2;

private:
    std::array<uint8_t, Tile_size> tile_data{};

    // Complete random-access iterator for pixels in the tile.
    // Being a template class allows code reuse for const/nonconst iterators.
    template <typename T>
    class tile_iterator {
    public:
        // Exported constants.
        static constexpr size_t const Line_size = lsize;
        static constexpr size_t const Num_lines = nlines;
        static constexpr size_t const Tile_size = nlines * lsize;
        static constexpr size_t const Byte_size = nlines * lsize / 2;
        // Exported types.
        using value_type        = T;
        using difference_type   = int64_t;
        using pointer           = T*;
        using reference         = T&;
        using iterator_category = std::random_access_iterator_tag;
        // So it can use the protected constructors.
        friend class BaseTile<lsize, nlines>;

    private:
        value_type* tile_data;
        // Controls how we are iterating.
        FlipMode flip;
        // Information about the iteration.
        uint32_t start, finish, loc;
        // Common logic for add 1/sub 1. No bounds checking!
        void incr_decr(FlipMode const flip_) noexcept {
            // We have to decide how we are going to iterate.
            switch (flip_) {
            case NoFlip:
                // Simple increment. Inverse of XYFlip.
                loc++;
                return;
            case XYFlip:
                // Simple decrement. Inverse of NoFlip.
                loc--;
                return;
            case XFlip:
                // Lines in normal order, each line reversed.
                // Inverse of YFlip.
                // Are we at the start of a line?
                if ((loc % lsize) == 0) {
                    // Yes, so move to the end of the next line.
                    loc += lsize + (lsize - 1);
                } else {
                    // No, so do a simple decrement.
                    loc--;
                }
                return;
            case YFlip:
                // Lines in reverse order, each line normal.
                // Inverse of XFlip.
                // Are we at the end of a line?
                if ((loc % lsize) == (lsize - 1)) {
                    // Yes, so move to the start of the previous line.
                    loc -= lsize + (lsize - 1);
                } else {
                    // No, so do a simple increment.
                    loc++;
                }
                return;
            }
        }
        // To simplify operator logic.
        void incr() noexcept {
            // Have we finished iterating?
            if (loc == Tile_size) {
                // Go no further.
                return;
            }
            if (loc == finish) {
                // If we are at the last element, flag end of iteration an
                // return.
                loc = Tile_size;
                return;
            }
            incr_decr(flip);
        }
        // To simplify operator logic.
        void decr() noexcept {
            // Have we finished iterating?
            if (loc == Tile_size) {
                // Return to last valid position.
                loc = finish;
                return;
            }
            if (loc == start) {
                // Do not decrement.
                return;
            }
            incr_decr(flip_xy(flip));
        }
        void offset(difference_type delta) noexcept {
            FlipMode flip_;
            bool     subtract;
            if (delta == 0) {
                return;
            }
            if (delta > 0) {
                if (loc == Tile_size) {
                    // Go no further if we finished iterating.
                    return;
                }
                if (loc == finish) {
                    // If we are at the last element, flag end of iteration an
                    // return.
                    loc = Tile_size;
                    return;
                }
                // Simple increment?
                if (delta == 1) {
                    incr_decr(flip);
                    return;
                }
                flip_    = flip;
                subtract = false;
            } else {
                if (loc == Tile_size) {
                    // Return to last valid position if we had finished
                    // iterating.
                    loc = finish;
                    delta++;
                    // Do we still have to decrement?
                    if (delta == 0) {
                        // No, so bail out.
                        return;
                    }
                }
                if (loc == start) {
                    // Do not decrement further.
                    return;
                }
                if (delta == -1) {
                    incr_decr(flip_xy(flip));
                    return;
                }
                delta    = -delta;
                flip_    = flip_xy(flip);
                subtract = true;
            }
            uint32_t line_off  = delta % lsize;
            uint32_t num_lines = delta / lsize;
            uint32_t atoff     = loc % lsize;
            uint32_t atline    = loc / lsize;
            // Otherwise, we have to decide how we are going to iterate.
            switch (flip_) {
            case NoFlip:
                // Simple increment. Inverse of XYFlip.
                loc += delta;
                // Are we adding with no flip or subtracting with XY flip?
                if (subtract) {
                    // Subtracting with XY flip. Can't overshoot start.
                    if (loc > start) {
                        loc = start;
                    }
                } else {
                    // Adding with no flip. Must check if we went past the end.
                    if (loc > finish) {
                        // If we are past the end, flag end of iteration.
                        loc = Tile_size;
                    }
                }
                return;
            case XYFlip:
                // Simple decrement. Inverse of NoFlip.
                loc -= delta;
                // Are we adding with XY flip or subtracting with no flip?
                if (subtract) {
                    // Subtracting with no flip. Can't overshoot start.
                    if (loc < start) {
                        loc = start;
                    }
                } else {
                    // Adding with XY flip. Must check if we went past the end.
                    if (loc < finish) {
                        // If we are past the end, flag end of iteration.
                        loc = Tile_size;
                    }
                }
                return;
            case XFlip:
                // Lines in normal order, each line reversed.
                // Inverse of YFlip.
                // Are we at the end of a line?
                if (line_off > atoff) {
                    num_lines++;
                    atoff += lsize - line_off;
                } else {
                    atoff -= line_off;
                }
                // Can't overshoot start if subtracting with Y flip, and we
                // must check for going past the end if adding with X flip.
                if (num_lines + atline >= nlines) {
                    loc = subtract ? start : static_cast<uint32_t>(Tile_size);
                } else {
                    loc = (atline + num_lines) * lsize + atoff;
                }
                return;
            case YFlip:
                // Lines in reverse order, each line normal.
                // Inverse of XFlip.
                if (line_off + atoff >= lsize) {
                    num_lines++;
                    atoff -= lsize - line_off;
                } else {
                    atoff += line_off;
                }
                // Can't overshoot start if subtracting with X flip, and we
                // must check for going past the end if adding with Y flip.
                if (num_lines > atline) {
                    loc = subtract ? start : static_cast<uint32_t>(Tile_size);
                } else {
                    loc = (atline - num_lines) * lsize + atoff;
                }
                return;
            }
        }
        // Subtracts rhs from lhs and returns result; this result is the number
        // of increments (if positive) of decrements (if negative) that would
        // turn rhs into lhs.
        template <typename U, typename V>
        static difference_type difference(
                tile_iterator<U> const& left,
                tile_iterator<V> const& right) noexcept {
            assert(left.tile_data == right.tile_data);
            assert(left.flip == right.flip);
            difference_type left_offset  = left.loc % lsize;
            difference_type left_lines   = left.loc / lsize;
            difference_type right_offset = right.loc % lsize;
            difference_type right_lines  = right.loc / lsize;
            switch (left.flip) {
            case NoFlip:
                // Simple increment. Inverse of XYFlip.
                return difference_type(left.loc) - right.loc;
            case XYFlip:
                // Simple decrement. Inverse of NoFlip.
                return difference_type(right.loc) - left.loc;
            case XFlip:
                // Lines in normal order, each line reversed.
                // Inverse of YFlip.
                return (left_lines - right_lines) * lsize
                       + (right_offset - left_offset);
            case YFlip:
                // Lines in reverse order, each line normal.
                // Inverse of XFlip.
                return (right_lines - left_lines) * lsize
                       + (left_offset - right_offset);
            default:
                assert(false);
            }
        }
        // Checks if two iterators are at the same point in the iteration.
        template <typename U, typename V>
        static bool equal(
                tile_iterator<U> const& left,
                tile_iterator<V> const& right) noexcept {
            if (left.loc == tile_iterator<U>::Tile_size
                || right.loc == tile_iterator<V>::Tile_size) {
                return left.loc == right.loc;
            }
            assert(left.tile_data == right.tile_data);
            assert(left.flip == right.flip);
            return left.loc == right.loc;
        }
        // Checks if lhs is an iteration to a position before rhs.
        template <typename U, typename V>
        static bool less(
                tile_iterator<U> const& left,
                tile_iterator<V> const& right) noexcept {
            return difference(left, right) < 0;
        }

    protected:
        // Constructors.
        tile_iterator(
                FlipMode const flip_, value_type* const tile_,
                bool const ending) noexcept
                : tile_data(tile_), flip(flip_) {
            // The values of start, finish and loc depend on flip mode:
            switch (flip) {
            case NoFlip:
                // Inverse of XYFlip.
                start  = 0;
                finish = Tile_size - 1;
                break;
            case XYFlip:
                // Inverse of NoFlip.
                start  = Tile_size - 1;
                finish = 0;
                break;
            case XFlip:
                // Inverse of YFlip.
                start  = lsize - 1;
                finish = lsize * (nlines - 1);
                break;
            case YFlip:
                // Inverse of XFlip.
                start  = lsize * (nlines - 1);
                finish = lsize - 1;
                break;
            }
            // Point to start.
            loc = ending ? static_cast<uint32_t>(Tile_size) : start;
        }

    public:
        // Conversion constructor that does any one of:
        // * convert iterator to const_iterator;
        // * copies const_iterator to const_iterator;
        // * copies iterator to iterator.
        template <typename U>
        explicit tile_iterator(tile_iterator<U> const& other) noexcept
                : tile_data(other.tile_data), flip(other.flip),
                  start(other.start), finish(other.finish), loc(other.loc) {}
        // Conversion assignment that does any one of:
        // * convert iterator to const_iterator;
        // * copies const_iterator to const_iterator;
        // * copies iterator to iterator.
        template <typename U>
        tile_iterator& operator=(tile_iterator<U> const& right) noexcept {
            if (this != &right) {
                tile_data = right.tile_data;
                flip      = right.flip;
                start     = right.start;
                finish    = right.finish;
                loc       = right.loc;
            }
            return *this;
        }
        // Conversion operator that does any one of:
        // * convert iterator to const_iterator;
        // * copies const_iterator to const_iterator;
        // * copies iterator to iterator.
        template <typename U>
        explicit operator tile_iterator<U>() const noexcept {
            return tile_iterator<U>(*this);
        }
        // Prefix increment.
        tile_iterator<T>& operator++() noexcept {
            incr();
            return *this;
        }
        // Postfix increment.
        tile_iterator<T> operator++(int const) noexcept {
            tile_iterator<T> value(*this);
            incr();
            return value;
        }
        // Prefix decrement.
        tile_iterator<T>& operator--() noexcept {
            decr();
            return *this;
        }
        // Postfix decrement.
        tile_iterator<T> operator--(int const) noexcept {
            tile_iterator<T> value(*this);
            decr();
            return value;
        }
        // Add amount to iterator. Can add positive or negative values.
        tile_iterator<T> operator+=(difference_type right) noexcept {
            offset(right);
            return *this;
        }
        // Returns iterator obtained by adding amount to lhs.
        tile_iterator<T> operator+(difference_type right) const noexcept {
            tile_iterator<T> value(*this);
            value += right;
            return value;
        }
        // Makes addition of difference_type to iterator commutative.
        friend tile_iterator<T> operator+(
                difference_type left, tile_iterator<T> const& right) noexcept {
            return right + left;
        }
        // Subtract amount from iterator. Can add positive or negative values.
        tile_iterator<T> operator-=(difference_type right) noexcept {
            return operator+=(-right);
        }
        // Returns iterator obtained by subtracting amount from lhs.
        tile_iterator<T> operator-(difference_type right) const noexcept {
            tile_iterator<T> value(*this);
            value += -right;
            return value;
        }
        // Returns how many increments (if positive) or decrements (if negative)
        // would turn rhs into lhs.
        // It is a template so as to be able to subtract:
        // * const_iterator from iterator;
        // * const_iterator from const_iterator;
        // * iterator from iterator.
        template <typename U>
        difference_type operator-(
                tile_iterator<U> const& right) const noexcept {
            return difference(*this, right);
        }
        // Comparison operators.
        // They are templates so as to be able to compare:
        // * const_iterator to iterator;
        // * const_iterator to const_iterator;
        // * iterator to iterator.
        template <typename U>
        bool operator==(tile_iterator<U> const& right) const noexcept {
            return equal(*this, right);
        }
        template <typename U>
        bool operator!=(tile_iterator<U> const& right) const noexcept {
            return !equal(*this, right);
        }
        template <typename U>
        bool operator<(tile_iterator<U> const& right) const noexcept {
            return less(*this, right);
        }
        template <typename U>
        bool operator>=(tile_iterator<U> const& right) const noexcept {
            return !less(*this, right);
        }
        template <typename U>
        bool operator>(tile_iterator<U> const& right) const noexcept {
            return less(right, *this);
        }
        template <typename U>
        bool operator<=(tile_iterator<U> const& right) const noexcept {
            return !less(right, *this);
        }
        // Dereferencing operators.
        value_type operator*() const noexcept {
            return tile_data[loc];
        }
        reference operator*() noexcept {
            return tile_data[loc];
        }
        pointer operator->() noexcept {
            return &(tile_data[loc]);
        }
        pointer operator->() const noexcept {
            return &(tile_data[loc]);
        }
        value_type operator[](difference_type n) const noexcept {
            return *operator+(n);
        }
        reference operator[](difference_type n) noexcept {
            return *operator+(n);
        }
    };

public:
    // Exported iterator types.
    using iterator               = tile_iterator<uint8_t>;
    using const_iterator         = tile_iterator<uint8_t const>;
    using reverse_iterator       = tile_iterator<uint8_t>;
    using const_reverse_iterator = tile_iterator<uint8_t const>;

    // Constructors.
    BaseTile() noexcept = default;    // Uninitialized
    // From stream.
    explicit BaseTile(std::istream& input) noexcept;
    // From (start, finish) range in iterators.
    template <typename Iter>
    BaseTile(
            Iter& start, Iter const& finish, FlipMode flip_,
            uint32_t nreps = 1) noexcept;

    // Computes (square of) distance between two tiles. This distance is defined
    // as being the sum of th squares of the differences between corresponding
    // pixels in the tiles.
    uint32_t distance(
            DistTable_t const& DistTable, FlipMode flip, const_iterator start,
            const_iterator const& finish) const noexcept;

    // Functions for starting iteration. Note how the reverse iterators are the
    // same as forward iterators with X and Y both flipped.
    iterator begin(FlipMode const flip_) noexcept {
        return iterator(flip_, tile_data.data(), false);
    }
    iterator end(FlipMode const flip_) noexcept {
        return iterator(flip_, tile_data.data(), true);
    }
    const_iterator begin(FlipMode const flip_) const noexcept {
        return const_iterator(flip_, tile_data.data(), false);
    }
    const_iterator end(FlipMode const flip_) const noexcept {
        return const_iterator(flip_, tile_data.data(), true);
    }
    const_iterator cbegin(FlipMode const flip_) const noexcept {
        return const_iterator(flip_, tile_data.data(), false);
    }
    const_iterator cend(FlipMode const flip_) const noexcept {
        return const_iterator(flip_, tile_data.data(), true);
    }
    reverse_iterator rbegin(FlipMode const flip_) noexcept {
        return begin(flip_xy(flip_));
    }
    reverse_iterator rend(FlipMode const flip_) noexcept {
        return end(flip_, tile_data.data());
    }
    const_reverse_iterator rbegin(FlipMode const flip_) const noexcept {
        return begin(flip_xy(flip_));
    }
    const_reverse_iterator rend(FlipMode const flip_) const noexcept {
        return end(flip_, tile_data.data());
    }
    const_reverse_iterator crbegin(FlipMode const flip_) const noexcept {
        return begin(flip_xy(flip_));
    }
    const_reverse_iterator crend(FlipMode const flip_) const noexcept {
        return end(flip_, tile_data.data());
    }

    // Draws line_count lines of the tile, starting at the position specified by
    // start iterator, to output stream out.
    void draw_tile(
            std::ostream& output, const_iterator& start,
            uint32_t line_count = nlines) const noexcept;
};

// Constructs a tile by reading it from a stream.
template <int lsize, int nlines>
BaseTile<lsize, nlines>::BaseTile(std::istream& input) noexcept {
    // Unpack nibbles to bytes as we go along.
    for (uint32_t ii = 0; ii < Tile_size; ii += 2) {
        unsigned value = input.get();
        // If we reach the end-of-stream, fill the rest with zeroes.
        if (!input.good()) {
            tile_data[ii + 0] = 0;
            tile_data[ii + 1] = 0;
        } else {
            tile_data[ii + 0] = (value >> 4U) & 0x0fU;
            tile_data[ii + 1] = value & 0x0fU;
        }
    }
}

// Construct by copying from given iterators several times.
template <int lsize, int nlines>
template <typename Iter>
BaseTile<lsize, nlines>::BaseTile(
        Iter& start, Iter const& finish, FlipMode const flip_,
        uint32_t nreps) noexcept {
    iterator dest = begin(flip_);
    // First nreps-1 repeats.
    for (uint32_t ii = 1; ii < nreps; ii++) {
        for (Iter from(start); dest != end(flip_) && from != finish;
             ++dest, ++from) {
            *dest = *from;
        }
    }
    // Last repeat updates start iterator.
    for (; dest != end(flip_) && start != finish; ++dest, ++start) {
        *dest = *start;
    }
    // Fill the rest with zeroes, if needed.
    for (; dest != end(flip_); ++dest) {
        *dest = 0;
    }
}

// Computes distance between this tile and the data at the given iterators.
template <int lsize, int nlines>
uint32_t BaseTile<lsize, nlines>::distance(
        DistTable_t const& DistTable, FlipMode flip, const_iterator start,
        const_iterator const& finish) const noexcept {
    uint32_t dist = 0;
    for (const_iterator it = begin(flip); it != end(flip) && start != finish;
         ++it, ++start) {
        uint32_t const color_left  = *it;
        uint32_t const color_right = *start;
        dist += DistTable[color_left][color_right];
    }
    return dist;
}

// Copies line_count lines starting from from the given iterator to the output
// stream.
template <int lsize, int nlines>
void BaseTile<lsize, nlines>::draw_tile(
        std::ostream& output, const_iterator& start,
        uint32_t line_count) const noexcept {
    const_iterator finish = start + line_count * Line_size;
    while (start != finish) {
        uint8_t value = (*start++) << 4;
        if (start != finish) {
            value |= (*start++);
        }
        output.put(value);
    }
}

using Tile = BaseTile<8, 8>;

#endif    // TILE_HH
