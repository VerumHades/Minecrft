#pragma once

#include <general.hpp>
#include <array>
#include <vector>
#include <queue>
#include <cstdint>
#include <unordered_map>
#include <cstdlib>
#include <cstring>
#include <iostream>

using std::vector<uint8_t> = ByteArray;

class BitFieldCache;
/*
  A tree dimensional array of bits (64 * 64 * 64), stored as and array of unsigned 64 bit integers
*/
static int last_id = 0;
class BitField3D{
    private:
        size_t id = 0; // Unique identifier
        union {
            size_t creator_id;
            size_t cache_id = -1ULL; // Index in the cache
        };
        BitField3D* transposed_cache_version_pointer = nullptr;

        std::array<uint64_t, 64 * 64> _internal_data = {0};

        bool inBounds(uint x, uint y, uint z = 0) const {
            return x >= 0 && y >= 0 && z >= 0 && x < 64 && y < 64 && z < 64;
        }
        uint calculateIndex(uint x, uint y) const {
            return x + y * 64;
        }

    public:
        BitField3D(){
            id = last_id++;
        }

        void applyOR(const std::array<uint64_t, 64 * 64>& data){
            for(int i = 0;i < 64 * 64;i++){
                _internal_data[i] |= data[i];
            }
        }

        std::array<uint64_t, 64 * 64>& data(){
            return _internal_data;
        }
        /* 
          Sets a bit at x,y,z.
        
          If coordinates are out of bounds returns false, otherwise returns true
        */
        bool set(uint x, uint y, uint z){
            if(!inBounds(x,y,z)) return false;

            if(transposed_cache_version_pointer){ // Do we have a cached rotated version?
                if(transposed_cache_version_pointer->creator_id == id) transposed_cache_version_pointer->set(z,y,x); // Is it still ours?
                else transposed_cache_version_pointer = nullptr; // Its not
            }

            _internal_data[calculateIndex(x,y)] |= (1ULL << (63 - z));
            return true;
        }

        /* 
          Resets a bit at x,y,z.
        
          If coordinates are out of bounds returns false, otherwise returns true
        */
        bool reset(uint x, uint y, uint z){
            if(!inBounds(x,y,z)) return false;

            if(transposed_cache_version_pointer){ // Do we have a cached rotated version?
                if(transposed_cache_version_pointer->creator_id == id) transposed_cache_version_pointer->reset(z,y,x); // Is it still ours?
                else transposed_cache_version_pointer = nullptr; // Its not
            }

            _internal_data[calculateIndex(x,y)] &= ~(1ULL << (63 - z));
            return true;
        }

        /*
            Returns the value of a bit at x,y,z

            Returns false when out of bounds.
        */
        bool get(uint x, uint y, uint z) const {
            if(!inBounds(x,y,z)) return false;

            return _internal_data[calculateIndex(x,y)] & (1ULL << (63 - z));
        }

        /*
            Sets a whole row to a 64bit value.

            If coordinates are out of bounds returns false, otherwise returns true
        */
        bool setRow(uint x, uint y, uint64_t value){
            if(!inBounds(x,y)) return false;

            _internal_data[calculateIndex(x,y)] = value;
            return true;
        }

        /*
            Returns a row at x,y

            If out of bounds returns 0
        */
        uint64_t getRow(uint x, uint y) const {
            if(!inBounds(x,y)) return 0ULL;

            return _internal_data[calculateIndex(x,y)];
        }

        /*
            Returnes a pointer to a transposed version of the bitfield (rotated) in the cache
        */
        BitField3D* getTransposed();

        ByteArray getCompressed();

        friend class BitFieldCache;
};

/*
    A cache that gives out the last unused bitfield. Doesn't care about taken or not
*/
class BitFieldCache{
    private:
        const static int max_cached = 2 * 1024; // 2 * 32MB cache (32KB per field * 1024)
        BitField3D cached_fields[max_cached];

        size_t next_spot = 0;

    public:
        std::tuple<BitField3D*, size_t> getNext(size_t id){
            BitField3D* out = &cached_fields[(next_spot + 1) % (max_cached - 1)];
            *out = {}; // zero out
            out->creator_id = id;
            return {out, next_spot++};
        }
};