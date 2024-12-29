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
#include <bitset>
#include <list>
#include <coherency.hpp>
#include <memory>

using CompressedArray = std::vector<uint64_t>;

struct TCacheMember;
/*
  A tree dimensional array of bits (64 * 64 * 64), stored as and array of unsigned 64 bit integers
*/
static size_t last_id = 0;
class BitField3D{
    private:
        size_t id = 0; // Unique identifier
        TCacheMember* transposed_cache_version_pointer = nullptr;

        std::array<uint64_t, 64 * 64> _internal_data = {0};

        static bool inBounds(uint x, uint y, uint z = 0) {
            return x >= 0 && y >= 0 && z >= 0 && x < 64 && y < 64 && z < 64;
        }
        static uint calculateIndex(uint x, uint y) {
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

        const std::array<uint64_t, 64 * 64>& data() const{
            return _internal_data;
        }
        /* 
          Sets a bit at x,y,z.
        
          If coordinates are out of bounds returns false, otherwise returns true
        */
        bool set(uint x, uint y, uint z);
        /* 
          Resets a bit at x,y,z.
        
          If coordinates are out of bounds returns false, otherwise returns true
        */
        bool reset(uint x, uint y, uint z);

        /*
            Returns the value of a bit at x,y,z

            Returns false when out of bounds.
        */
        bool get(uint x, uint y, uint z) const;
        /*
            Sets a whole row to a 64bit value.

            If coordinates are out of bounds returns false, otherwise returns true
        */
        bool setRow(uint x, uint y, uint64_t value);
        /*
            Returns a row at x,y

            If out of bounds returns 0
        */
        uint64_t getRow(uint x, uint y) const;

        /*
            Returnes a pointer to a transposed version of the bitfield (rotated) in the cache
        */
        BitField3D* getTransposed();

        CompressedArray getCompressed();

        static CompressedArray compress(const std::array<uint64_t, 64 * 64>& source);
        static void decompress(std::array<uint64_t, 64 * 64>& destination, CompressedArray& source);

        friend class BitFieldCache;
};

struct TCacheMember{
    size_t creator_id;
    BitField3D field;
};
/*
    A cache that gives out the last unused bitfield. Doesn't care about taken or not
*/
class BitFieldCache{
    private:
        const static int max_cached = 2 * 1024; // 2 * 32MB cache (32KB per field * 1024)
        
        std::array<TCacheMember, max_cached> cached_fields{};
        size_t next_spot = 0;
    public:
        TCacheMember* next(size_t id){
            next_spot = (next_spot + 1) % (max_cached - 1);
            auto& member = cached_fields[next_spot];

            member.field = {}; // zero out
            member.creator_id = id;

            return &member;
        }
};

struct CCacheMember{
    std::shared_ptr<CompressedArray> compressed_data = nullptr;
    BitField3D field;
};

class CompressedBitField3D{
    private:
        std::shared_ptr<CompressedArray> data_ptr = nullptr;
        CCacheMember* cached_ptr = nullptr;

    public:
        void set(const BitField3D& source);
        BitField3D* get();
};

class CompressedBitFieldCache{
    private:
        const static int max_cached = 2 * 1024; // 2 * 32MB cache (32KB per field * 1024)
        std::array<CCacheMember, max_cached> cached_fields{};

        size_t next_spot = 0;
    public:
        CCacheMember* next(std::shared_ptr<CompressedArray> compressed_data);
};     