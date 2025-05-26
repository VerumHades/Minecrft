#pragma once

#include <array>
#include <structure/definitions.hpp>

/**
 * @brief A tree dimensional array of bits (64 * 64 * 64), stored as and array of unsigned 64 bit integers;
 * y is the largest dimensions that jumps around in memory the most
 *
 */
class BitField {
  private:
    std::array<uint64_t, 64 * 64> _internal_data = {0};

  protected:
    static bool inBounds(uint x, uint y, uint z = 0) {
        return x < 64 && y < 64 && z < 64;
    }
    static uint calculateIndex(uint x, uint y) {
        return x + y * 64;
    }

  public:
    /**
     * @brief Sets a bit at x,y,z.
     * 
     */
    virtual bool set(uint x, uint y, uint z);
    /**
     * @brief Resets a bit at x,y,z.
     * 
     */
    virtual bool reset(uint x, uint y, uint z);
    /**
     * @brief Returns the value of a bit at x,y,z
     * 
     */
    virtual bool get(uint x, uint y, uint z);
    /**
     * @brief Sets a whole row to a 64bit value.
     * 
     */
    virtual bool setRow(uint x, uint y, uint64_t value);
    /**
     * @brief Returns a row at x,y
     * 
     */
    virtual uint64_t getRow(uint x, uint y);
    /**
     * @brief Fill the array with the set value
     * 
     * @param value 
     * @return Fill 
     */
    virtual void fill(bool value);

    std::array<uint64_t, 64 * 64>& data();
    const std::array<uint64_t, 64 * 64>& data() const;
};
