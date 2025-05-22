#pragma once

#include <array>
#include <vector>
#include <memory>
#include <structure/synchronization/guard.hpp>

#include <structure/bitfield.hpp>

using CompressedArray = std::vector<uint64_t>;

struct TCacheMember;

/**
 * @brief A tree dimensional array of bits (64 * 64 * 64), stored as and array of unsigned 64 bit integers
 * 
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
    /**
     * @brief Sets bit at x,y,z.
     * 
     * @param x 
     * @param y 
     * @param z 
     * @return true success
     * @return false failiure
     */
    bool set(uint x, uint y, uint z) override;
    /**
     * @brief Resets a bit at x,y,z.
     * 
     * @param x 
     * @param y 
     * @param z 
     * @return true success
     * @return false failiure
     */
    bool reset(uint x, uint y, uint z) override;

    /**
     * @brief Returns the value of a bit at x,y,z
     * 
     * @param x 
     * @param y 
     * @param z 
     * @return true success
     * @return false failiure
     */
    bool get(uint x, uint y, uint z) override;
    
    /**
     * @brief Sets a whole row to a 64bit value.
     * 
     * @param x 
     * @param y 
     * @param value 
     * @return true 
     * @return false 
     */
    bool setRow(uint x, uint y, uint64_t value) override;
    
    /**
     * @brief Returns a row at x,y
     * 
     * @param x 
     * @param y 
     * @return uint64_t 
     */
    uint64_t getRow(uint x, uint y) override;

    /**
     * @brief Returns a pointer to a transposed version of the bitfield (rotated) in the cache
     * 
     * @return BitField3D* 
     */
    BitField3D* getTransposed();

    /**
     * @brief Returns a pointer to a simplified version of the bitfield (avaraged out to form a simple mesh)
     * 
     * @param level 
     * @return BitField3D* 
     */
    BitField3D* getSimplified(SimplificationLevel level);

    /**
     * @brief Returns the simplified version and itself for the NONE level.
     * 
     * @param level 
     * @return BitField3D* 
     */
    BitField3D* getSimplifiedWithNone(SimplificationLevel level);

    /**
     * @brief Fill the array with the set value
     * 
     * @param value 
     */
    void fill(bool value) override;
    /**
     * @brief Get a brand new unique ID
     * 
     */
    void resetID();
    
    Sync::Guard& Guard() {
        return guard;
    }

    /**
     * @brief Get compressed as an array of uint64_t
     * 
     * @return CompressedArray 
     */
    CompressedArray getCompressed();

    /**
     * @brief Compress an array of uint64_t
     * 
     * @param source 
     * @return CompressedArray 
     */
    static CompressedArray compress(const std::array<uint64_t, 64 * 64>& source);
    /**
     * @brief Decompress an array of uint64_t
     * 
     * @param destination Array that will be overwritten, expected to be zeroed out
     * @param source 
     */
    static void decompress(std::array<uint64_t, 64 * 64>& destination, CompressedArray& source);

    friend class BitFieldCache;
};

/**
 * @brief Cached rotated field
 * 
 */
struct TCacheMember {
    size_t creator_id;
    BitField3D field;
};
/**
 * @brief A cache that gives out the last unused bitfield. Doesn't care about taken or not
 * 
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
    /**
     * @brief Returns the next cache spot, zeroes out the field or sets it to the immediate value
     * 
     * @param id Id of the creator field
     * @param immediate_value An optional value that can be copied into the newly picked bitarray right away
     * @return TCacheMember* 
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

/**
 * @brief Temporarily decompressed cached field
 * 
 */
struct CCacheMember {
    std::shared_ptr<CompressedArray> compressed_data = nullptr;
    BitField3D field{};
};

/**
 * @brief Holds a reference to compressed data that is decompressed and cached on demand
 * 
 */
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

    /**
     * @brief Returns the compressed array right away, pontentially avoiding recompression
     * 
     * @return const CompressedArray& 
     */
    const CompressedArray& getCompressed();
};

/**
 * @brief Cache for the compressed field
 * 
 */
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
