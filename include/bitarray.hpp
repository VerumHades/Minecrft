#pragma once

#include <general.hpp>
#include <array>
#include <vector>
#include <queue>
#include <cstdint>
#include <unordered_map>
#include <cstdlib>

class BitFieldCache;
/*
  A tree dimensional array of bits (64 * 64 * 64), stored as and array of unsigned 64 bit integers
*/

static size_t last_cache_id = 0;
class BitField3D{
    private:
        size_t cache_id = 0;

        std::array<uint64_t, 64> _internal_data;

        bool inBounds(uint x, uint y, uint z = 0) const {
            return x >= 0 && y >= 0 && z >= 0 && x < 64 && y < 64 && z < 64;
        }
        uint calculateIndex(uint x, uint y) const {
            return x + y * 64;
        }

    public:
        BitField3D(){
            cache_id = last_cache_id++;
        }
        /* 
          Sets a bit at x,y,z.
        
          If coordinates are out of bounds returns false, otherwise returns true
        */
        bool set(uint x, uint y, uint z){
            if(!inBounds(x,y,z)) return false;

            _internal_data[calculateIndex(x,y)] |= (1ULL << z);
        }

        /* 
          Resets a bit at x,y,z.
        
          If coordinates are out of bounds returns false, otherwise returns true
        */
        bool reset(uint x, uint y, uint z){
            if(!inBounds(x,y,z)) return false;

            _internal_data[calculateIndex(x,y)] &= ~(1ULL << z);
        }

        /*
            Returns the value of a bit at x,y,z

            Returns false when out of bounds.
        */
        bool get(uint x, uint y, uint z) const {
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
        uint64_t getRow(uint x, uint y) const {
            if(!inBounds(x,y)) return 0ULL;

            return _internal_data[calculateIndex(x,y)];
        }

        /*
            Returnes a transposed version of the bitfield (rotated)
        
            If supplied with a cache will pull from it or save to it.
        */
        const BitField3D& getTransposed(BitFieldCache* cache = nullptr) const;
};

class BitFieldCache{
    private:
        using BitFieldArray = std::vector<BitField3D>;

        BitFieldArray cached_fields;
        std::unordered_map<size_t,BitFieldArray::iterator> cached_registry; // Id -> Bitfield
        std::queue<std::tuple<BitFieldArray::iterator,size_t>> drop_queue; // Dropping the oldest
        
        const int max_cached = 1024; // 32MB cache (32KB per field * 1024)

        void dropOldestField();

    public:
        void add(BitField3D& field, size_t id);
        
        bool has(size_t id) {return cached_registry.contains(id);};
        /*
            Get a bitfield by cache id. If it doesnt exist return nullptr.
        */
        BitFieldArray::iterator get(size_t id);
};