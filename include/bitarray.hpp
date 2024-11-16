#pragma once

#include <general.hpp>
#include <array>
#include <vector>

/*
  A tree dimensional array of bits (64 * 64 * 64), stored as and array of unsigned 64 bit integers
*/
class BitField3D{
    private:
        std::array<uint64_t, 64> _internal_data;

        bool inBounds(uint x, uint y, uint z = 0){
            return x >= 0 && y >= 0 && z >= 0 && x < 64 && y < 64 && z < 64;
        }
        uint calculateIndex(uint x, uint y){
            return x + y * 64;
        }

    public:
        /* 
          Sets a bit at x,y,z.
        
          If coordinates are out of bounds returns false, otherwise returns true
        */
        bool set(uint x, uint y, uint z){
            if(!inBounds(x,y,z)) return false;

            _internal_data[calculateIndex(x,y)] |= (1ULL << z);
        }

        /*
            Returns the value of a bit at x,y,z

            Returns false when out of bounds.
        */
        bool get(uint x, uint y, uint z){
            if(!inBounds(x,y,z)) return false;

            return _internal_data[calculateIndex(x,y)] & (1ULL << z);
        }

        /*
            Sets a whole row to a 64bit value.

            If coordinates are out of bounds returns false, otherwise returns true
        */
        bool setRow(uint x, uint y, uint64_t value){
            if(!inBounds(x,y)) return false;

            _internal_data[calculateIndex(x,y)] = value;
        }

        /*
            Returns a row at x,y

            If out of bounds returns 0
        */
        uint64_t getRow(uint x, uint y){
            if(!inBounds(x,y)) return 0ULL;

            return _internal_data[calculateIndex(x,y)];
        }
};