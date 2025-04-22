#pragma once

#include <array>
#include <structure/definitions.hpp>

/*
  A tree dimensional array of bits (64 * 64 * 64), stored as and array of unsigned 64 bit integers
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
    /*
      Sets a bit at x,y,z.

      If coordinates are out of bounds returns false, otherwise returns true
    */
    virtual bool set(uint x, uint y, uint z);
    /*
      Resets a bit at x,y,z.

      If coordinates are out of bounds returns false, otherwise returns true
    */
    virtual bool reset(uint x, uint y, uint z);
    /*
        Returns the value of a bit at x,y,z

        Returns false when out of bounds.
    */
    virtual bool get(uint x, uint y, uint z);
    /*
        Sets a whole row to a 64bit value.

        If coordinates are out of bounds returns false, otherwise returns true
    */
    virtual bool setRow(uint x, uint y, uint64_t value);
    /*
        Returns a row at x,y

        If out of bounds returns 0
    */
    virtual uint64_t getRow(uint x, uint y);
    /*
        Fill the array with the set value
    */
    virtual void fill(bool value);

    std::array<uint64_t, 64 * 64>& data();
    const std::array<uint64_t, 64 * 64>& data() const;
};
