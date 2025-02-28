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
    public:
        // Used for indexing, don't change!
        enum SimplificationLevel{
            TO_32 = 0,
            TO_16,
            TO_8,
            TO_4,
            TO_2,
            TO_1,

            NONE = -1
        };

    private:
        size_t id = 0; // Unique identifier
        TCacheMember* transposed_cache_version_pointer = nullptr;
        TCacheMember* simplified_version_pointer = nullptr;

        std::array<uint64_t, 64 * 64> _internal_data = {0};

        static bool inBounds(uint x, uint y, uint z = 0) {
            return x < 64 && y < 64 && z < 64;
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

        /*
            Returns a pointer to a simplified version of the bitfield (avaraged out to form a simple mesh)
        */
        BitField3D* getSimplified(SimplificationLevel level);
        
        /*
            Returns the simplified version and itself for the NONE level.

            Be careful the pointer can become invalid if the original field is lost.
        */
        BitField3D* getSimplifiedWithNone(SimplificationLevel level);

        /*
            Fill the array with the set value
        */
        void fill(bool value);

        CompressedArray getCompressed();

        static CompressedArray compress(const std::array<uint64_t, 64 * 64>& source);
        static void decompress(std::array<uint64_t, 64 * 64>& destination, CompressedArray& source);

        void resetID(){
            id = last_id++;
        }
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
        const static int max_cached = 1024 * 2; // 2 * 32MB cache (32KB per field * 1024)
        
        std::vector<TCacheMember> cached_fields{};
        size_t next_spot = 0;
        
        BitFieldCache(){
            cached_fields = std::vector<TCacheMember>(max_cached);
        }
            
    public:
        /*
            Returns the next cache spot, zeroes out the field
        */
        TCacheMember* next(size_t id){
            next_spot = (next_spot + 1) % (max_cached - 1);
            auto& member = cached_fields[next_spot];

            member.field.resetID();
            member.field = {}; // zero out
            member.creator_id = id;

            return &member;
        }

        static BitFieldCache& get(){
            static BitFieldCache cache{};
            return cache;
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
        CompressedBitField3D();
        CompressedBitField3D(const BitField3D& source){
            data_ptr = std::make_shared<CompressedArray>(BitField3D::compress(source.data()));
        }
        CompressedBitField3D(const CompressedArray& array){
            data_ptr = std::make_shared<CompressedArray>(array);
        }

        void set(const BitField3D& source);
        BitField3D* get();
        const CompressedArray& getCompressed();
};

class CompressedBitFieldCache{
    private:
        const static int max_cached = 1024; // 2 * 32MB cache (32KB per field * 1024)
        std::vector<CCacheMember> cached_fields{};

        CompressedBitFieldCache(){
            cached_fields = std::vector<CCacheMember>(max_cached);
        }
        size_t next_spot = 0;
    public:
        CCacheMember* next(std::shared_ptr<CompressedArray> compressed_data);

        static CompressedBitFieldCache& get(){
            static CompressedBitFieldCache cache{};
            return cache;
        }
};     