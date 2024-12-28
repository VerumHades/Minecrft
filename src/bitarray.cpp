#include <bitarray.hpp>

static BitFieldCache transposed_cache = {};

BitField3D* BitField3D::getTransposed(){
    if(transposed_cache_version_pointer && transposed_cache_version_pointer->creator_id == id){
        return transposed_cache_version_pointer;
    }

    /*
        TODO: implement transposing
    */

    auto [transposed,new_cache_id] = transposed_cache.getNext(id);
    transposed_cache_version_pointer = transposed;
    cache_id = new_cache_id;

    for(int z = 0;z < 64;z++){
        for(int y = 0; y < 64;y++){
            uint64_t value = getRow(z,y);

            for(int x = 0;x < 64;x++){
                uint64_t mask = 1ULL << (63 - x);

                if(!(value & mask)) continue;

                transposed->setRow(x,y, transposed->getRow(x,y) | (1ULL << (63 - z)));
            }
        }
    }

    return transposed;
}

CompressedArray BitField3D::getCompressed(){
    return compress(_internal_data);
}

/*
    Row based compression
*/
CompressedArray BitField3D::compress(std::array<uint64_t, 64 * 64>& source){
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

void BitField3D::decompress(std::array<uint64_t, 64 * 64>& destination, CompressedArray source){
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

    std::cout << data - (value_mask + 64) << std::endl;
}