#include <structure/serialization/serializer.hpp>
#include <structure/serialization/serializer_extras.hpp>

/*
    BlockArray serialization
*/
#include <chunk.hpp> 

SerializeFunction(Chunk) {
    array.append<int>(this_.worldPosition.x);
    array.append<int>(this_.worldPosition.y);
    array.append<int>(this_.worldPosition.z);
    
    return Serialize<SparseBlockArray>(this_, array);
}
SerializeInstatiate(Chunk)

DeserializeFunction(Chunk){
    this_.worldPosition = {
        array.read<int>(),
        array.read<int>(),
        array.read<int>(),
    };

    return Deserialize<SparseBlockArray>(this_, array);
}
DeserializeInstatiate(Chunk);