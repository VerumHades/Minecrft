#include <bitarray.hpp>

BitField3D* BitField3D::getTransposed(BitFieldCache* cache){
    if(cached_version_pointer && cached_version_pointer->creator_id == id){
        return cached_version_pointer;
    }

    /*
        TODO: implement transposing
    */

    auto [transposed,new_cache_id] = cache->getNext(id);
    cached_version_pointer = transposed;
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

    //cache->add(transposed, cache_id);
    return transposed;//cache->get(cache_id); 
}