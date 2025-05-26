#pragma once

#include <fstream>
#include <cstring>
#include <iomanip>
#include <bit>
#include <bitset>
#include <cstdint>
#include <vector>
#include <array>
#include <iostream>
#include <string>
#include <memory>
#include <variant>
#include <cstring>

#include <path_config.hpp>
/**
 * @brief Type that selects the smallest type that fits the set number of bits, max 64
 * 
 * @tparam Bits 
 */
template <size_t Bits>
using uint_t = typename std::conditional<
    Bits <= 8, uint8_t,
    typename std::conditional<
        Bits <= 16, uint16_t,
        typename std::conditional<Bits <= 32, uint32_t,
                                  typename std::conditional<Bits <= 64, uint64_t, void>::type>::type>::type>::type;

namespace bitworks {
/**
 * @brief Saves T into a filestream
 * 
 * @tparam T 
 * @param file 
 * @param value 
 */
template <typename T> static inline void saveValue(std::fstream& file, T value) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(T));
}
/**
 * @brief Reads T from a filestream
 * 
 * @tparam T 
 * @param file 
 * @param value 
 */
template <typename T> static inline T readValue(std::fstream& file) {
    T value;
    file.read(reinterpret_cast<char*>(&value), sizeof(T));
    return value;
}
}; // namespace bitworks

/**
 * @brief Counts the leading zeroes in an unsigned integer
 * 
 * @tparam T 
 * @param x 
 * @return uint8_t 
 */
template <typename T> inline uint8_t count_leading_zeros(T x) {
    return std::countl_zero(x);
}

/**
 * @brief Counts the number of ones in an unsigned integer
 * 
 * @tparam T 
 * @param x 
 * @return uint8_t 
 */
template <typename T> inline uint8_t count_ones(T x) {
    return std::popcount(x);
}

/**
 * @brief A 2D plane of bits
 * 
 * @tparam size 
 */
template <int size = 64> class BitPlane {
  private:
    std::array<uint_t<size>, size> data{};

  public:
    BitPlane() {}
    void set(int x, int y) {
        data[y] |= 1 << (63 - x);
    }
    void reset(int x, int y) {
        data[y] &= ~(1 << (63 - x));
    }
    bool get(int x, int y) {
        return data[y] & (1 << (63 - x));
    }

    uint_t<size>& operator[](std::size_t index) {
        return data[index];
    }

    const uint_t<size>& operator[](std::size_t index) const {
        return data[index];
    }

    void clear() {
        data.fill(0);
    }
};

/**
 * @brief A 2D plane mask that shouldnt allow setting same bits twice
 * For future use
 * @tparam size 
 */
template <int size = 64> class BitMask {
  private:
    BitPlane<size> plane;
    std::bitset<size> finished_rows;
    size_t lower_bound = 0;
    size_t upper_bound = size - 1;

    void UpdateBounds(int updated_row) {
        if (plane[updated_row] == ~0ULL)
            finished_rows.set(updated_row);

        if (updated_row <= lower_bound)
            while (finished_rows[lower_bound] && lower_bound < upper_bound) {
                lower_bound++;
            }

        if (updated_row >= upper_bound)
            while (finished_rows[upper_bound] && upper_bound > lower_bound) {
                upper_bound--;
            }
    }

  public:
    uint_t<size> Apply(int row, uint_t<size> value) {
        uint_t<size> result = ~plane[row] & value;
        plane[row] |= value;

        if (value != 0ULL)
            UpdateBounds(value);

        return result;
    }

    size_t GetLowerBound() const {
        return lower_bound;
    }
    size_t GetUpperBound() const {
        return upper_bound;
    }
};

#include <game/blocks.hpp>
