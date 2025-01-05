#include <bitarray.hpp>



bool BitField3D::set(uint x, uint y, uint z){
    if(!inBounds(x,y,z)) return false;

    if(transposed_cache_version_pointer){ // Do we have a cached rotated version?
        if(transposed_cache_version_pointer->creator_id == id) transposed_cache_version_pointer->field.set(z,y,x); // Is it still ours?
        else transposed_cache_version_pointer = nullptr; // Its not
    }

    _internal_data[calculateIndex(x,y)] |= (1ULL << (63 - z));
    return true;
}

bool BitField3D::reset(uint x, uint y, uint z){
    if(!inBounds(x,y,z)) return false;

    if(transposed_cache_version_pointer){ // Do we have a cached rotated version?
        if(transposed_cache_version_pointer->creator_id == id) transposed_cache_version_pointer->field.reset(z,y,x); // Is it still ours?
        else transposed_cache_version_pointer = nullptr; // Its not
    }

    _internal_data[calculateIndex(x,y)] &= ~(1ULL << (63 - z));
    return true;
}

bool BitField3D::get(uint x, uint y, uint z) const {
    if(!inBounds(x,y,z)) return false;

    return _internal_data[calculateIndex(x,y)] & (1ULL << (63 - z));
}


bool BitField3D::setRow(uint x, uint y, uint64_t value){
    if(!inBounds(x,y)) return false;

    _internal_data[calculateIndex(x,y)] = value;
    return true;
}

uint64_t BitField3D::getRow(uint x, uint y) const {
    if(!inBounds(x,y)) return 0ULL;

    return _internal_data[calculateIndex(x,y)];
}


static BitFieldCache transposed_cache = {};

BitField3D* BitField3D::getTransposed() {
    if(transposed_cache_version_pointer && transposed_cache_version_pointer->creator_id == id){
        return &transposed_cache_version_pointer->field;
    }

    /*
        TODO: implement transposing
    */

    auto transposed = transposed_cache.next(id);
    transposed_cache_version_pointer = transposed;

    for(int z = 0;z < 64;z++){
        for(int y = 0; y < 64;y++){
            uint64_t value = getRow(z,y);

            for(int x = 0;x < 64;x++){
                uint64_t mask = 1ULL << (63 - x);

                if(!(value & mask)) continue;

                transposed->field.setRow(x,y, transposed->field.getRow(x,y) | (1ULL << (63 - z)));
            }
        }
    }

    return &transposed->field;
}

CompressedArray BitField3D::getCompressed(){
    return compress(_internal_data);
}

/*
    Row based compression
*/
CompressedArray BitField3D::compress(const std::array<uint64_t, 64 * 64>& source){
    CompressedArray data_output{};
    data_output.reserve(64*64);

    const uint64_t all_zeroes = 0ULL;
    const uint64_t all_ones = ~0ULL; 

    std::array<uint64_t, 64> compression_mask{};
    std::array<uint64_t, 64> value_mask{};

    for(int x = 0;x < 64;x++)
    for(int y = 0;y < 64;y++){
        auto& row = source[calculateIndex(x,y)];

        if(row == all_zeroes || row == all_ones){
            compression_mask[y] |= (1ULL << x);
            if(row == all_ones) value_mask[y] |= (1ULL << x);
        }
        else data_output.push_back(row);
    }
    
    CompressedArray output{};
    output.reserve(data_output.size() + 64 * 2);

    output.insert(output.end(), compression_mask.begin(), compression_mask.end());
    output.insert(output.end(), value_mask.begin(), value_mask.end());
    output.insert(output.end(), data_output.begin(), data_output.end());

    output.shrink_to_fit();
    return output;
}

void BitField3D::decompress(std::array<uint64_t, 64 * 64>& destination, CompressedArray& source){
    if(source.size() == 0) return;

    const uint64_t all_zeroes = 0ULL;
    const uint64_t all_ones = ~0ULL;

    uint64_t* compression_mask = source.data();
    uint64_t* value_mask       = compression_mask + 64;
    uint64_t* data             = value_mask + 64;

    for(int x = 0;x < 64;x++)
    for(int y = 0;y < 64;y++){
        auto& row = destination[calculateIndex(x,y)];

        if(compression_mask[y] & (1ULL << x)) 
            row = (value_mask[y] & (1ULL << x)) ? all_ones : all_zeroes;  

        else row = *data++;
    }
}


static CompressedBitFieldCache compressed_cache = {};

CCacheMember* CompressedBitFieldCache::next(std::shared_ptr<CompressedArray> new_compressed_data){
    next_spot = (next_spot + 1) % (max_cached - 1);

    auto& member = cached_fields[next_spot];
    if(member.compressed_data) *member.compressed_data = BitField3D::compress(member.field.data());

    member.compressed_data = new_compressed_data;
    member.field.resetID();
    member.field = {}; // Decompression expects a zeroed out field
    
    BitField3D::decompress(member.field.data(), *new_compressed_data);

    return &member;
}  

BitField3D* CompressedBitField3D::get(){
    if(cached_ptr && cached_ptr->compressed_data.get() == data_ptr.get()) return &cached_ptr->field;
    
    cached_ptr = compressed_cache.next(data_ptr);
    return &cached_ptr->field;
}

void CompressedBitField3D::set(const BitField3D& source){
    cached_ptr = nullptr;
    data_ptr = std::make_shared<CompressedArray>(BitField3D::compress(source.data()));
}


CompressedBitField3D::CompressedBitField3D(){
    data_ptr = std::make_shared<CompressedArray>();
}