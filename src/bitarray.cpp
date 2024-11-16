#include <bitarray.hpp>

const BitField3D& BitField3D::getTransposed(BitFieldCache* cache) const {
    if(cache && cache->has(cache_id)){
        return *cache->get(cache_id);
    }

    /*
        TODO: implement transposing
    */

    BitField3D transposed = *this;

    if(cache) cache->add(transposed, cache_id);

    return transposed; 
}

void BitFieldCache::dropOldestField(){
    if(drop_queue.empty()) return;

    auto [iterator,id] = drop_queue.front();
    drop_queue.pop();
    cached_registry.erase(id);
    cached_fields.erase(iterator);
}

void BitFieldCache::add(BitField3D& field,size_t id){
    auto iterator = cached_fields.insert(cached_fields.end(), field);
    cached_registry[id] = iterator;
    drop_queue.push({iterator,id});

    while(drop_queue.size() > max_cached){
        dropOldestField();
    }
}

BitFieldCache::BitFieldArray::iterator BitFieldCache::get(size_t id){
    if(!cached_registry.contains(id)) return cached_fields.end();
    return cached_registry[id];
}