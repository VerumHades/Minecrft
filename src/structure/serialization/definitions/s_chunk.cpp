#include <structure/serialization/serializer.hpp>
#include <structure/serialization/serializer_extras.hpp>

/*
    BlockArray serialization
*/
#include <game/chunk.hpp> 

SerializeFunction(Chunk) {
    array.append<int>(this_.worldPosition.x);
    array.append<int>(this_.worldPosition.y);
    array.append<int>(this_.worldPosition.z);
    
    return Serialize<SparseBlockArray>(this_, array);
}
SerializeInstatiate(Chunk)

DeserializeFunction(Chunk){
    ResolvedOption(x, read<int>)
    ResolvedOption(y, read<int>)
    ResolvedOption(z, read<int>)

    this_.worldPosition = glm::ivec3{x,y,z};

    return Deserialize<SparseBlockArray>(this_, array);
}
DeserializeInstatiate(Chunk);