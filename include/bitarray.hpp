#pragma once

#include <array>
#include <vector>
#include <memory>
#include <structure/synchronization/guard.hpp>

#include <structure/bitfield.hpp>

using CompressedArray = std::vector<uint64_t>;

struct TCacheMember;

/*
  A tree dimensional array of bits (64 * 64 * 64), stored as and array of unsigned 64 bit integers
*/
static size_t last_id = 0;
class BitField3D : public BitField {
  public:
    // Used for indexing, don't change!
    enum SimplificationLevel {
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

    mutable std::shared_mutex mutex;

    Sync::Guard guard;

  public:
    BitField3D() {
        id = last_id++;
    }
    /*
      Sets a bit at x,y,z.

      If coordinates are out of bounds returns false, otherwise returns true
    */
    bool set(uint x, uint y, uint z) override;
    /*
      Resets a bit at x,y,z.

      If coordinates are out of bounds returns false, otherwise returns true
    */
    bool reset(uint x, uint y, uint z) override;

    /*
        Returns the value of a bit at x,y,z

        Returns false when out of bounds.
    */
    bool get(uint x, uint y, uint z) override;
    /*
        Sets a whole row to a 64bit value.

        If coordinates are out of bounds returns false, otherwise returns true
    */
    bool setRow(uint x, uint y, uint64_t value) override;
    /*
        Returns a row at x,y

        If out of bounds returns 0
    */
    uint64_t getRow(uint x, uint y) override;

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
    void fill(bool value) override;
    void resetID();

    Sync::Guard& Guard() {
        return guard;
    }

    CompressedArray getCompressed();

    static CompressedArray compress(const std::array<uint64_t, 64 * 64>& source);
    static void decompress(std::array<uint64_t, 64 * 64>& destination, CompressedArray& source);

    friend class BitFieldCache;
};

struct TCacheMember {
    size_t creator_id;
    BitField3D field;
};
/*
    A cache that gives out the last unused bitfield. Doesn't care about taken or not
*/
class BitFieldCache {
  private:
    const static int max_cached = 1024 * 2; // 2 * 32MB cache (32KB per field * 1024)

    std::vector<TCacheMember> cached_fields{};
    size_t next_spot = 0;

    BitFieldCache() {
        cached_fields = std::vector<TCacheMember>(max_cached);
    }

    std::mutex mutex;

  public:
    /*
        Returns the next cache spot, zeroes out the field or sets it to the immediate value
    */
    TCacheMember* next(size_t id, std::array<uint64_t, 64 * 64>* immediate_value = nullptr) {
        std::lock_guard<std::mutex> lock(mutex);

        next_spot = (next_spot + 1) % (max_cached - 1);
        auto& member = cached_fields[next_spot];

        member.field.resetID();
        member.creator_id = id;

        if (immediate_value)
            member.field.data() = *immediate_value;
        else
            member.field.fill(0); // zero out

        return &member;
    }

    static BitFieldCache& get() {
        static BitFieldCache cache{};
        return cache;
    }
};

struct CCacheMember {
    std::shared_ptr<CompressedArray> compressed_data = nullptr;
    BitField3D field{};
};

class CompressedBitField3D {
  private:
    std::shared_ptr<CompressedArray> data_ptr = nullptr;
    CCacheMember* cached_ptr = nullptr;

  public:
    CompressedBitField3D();
    CompressedBitField3D(const BitField3D& source) {
        data_ptr = std::make_shared<CompressedArray>(BitField3D::compress(source.data()));
    }
    CompressedBitField3D(const CompressedArray& array) {
        data_ptr = std::make_shared<CompressedArray>(array);
    }

    void set(const BitField3D& source);
    BitField3D* get();
    const CompressedArray& getCompressed();
};

class CompressedBitFieldCache {
  private:
    const static int max_cached = 1024; // 2 * 32MB cache (32KB per field * 1024)
    std::vector<CCacheMember> cached_fields{};

    CompressedBitFieldCache() {
        cached_fields = std::vector<CCacheMember>(max_cached);
    }
    size_t next_spot = 0;

    std::mutex mutex;

  public:
    CCacheMember* next(std::shared_ptr<CompressedArray> compressed_data);

    static CompressedBitFieldCache& get() {
        static CompressedBitFieldCache cache{};
        return cache;
    }
};
