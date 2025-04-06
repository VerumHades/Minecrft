#include <structure/serialization/serializer.hpp>
#include <structure/serialization/serializer_extras.hpp>

/*
    BlockArray serialization
*/
#include <game/chunk.hpp>

SerializeFunction(Chunk) {
    array.Append<int>(this_.worldPosition.x);
    array.Append<int>(this_.worldPosition.y);
    array.Append<int>(this_.worldPosition.z);

    return Serialize<SparseBlockArray>(this_, array);
}
SerializeInstatiate(Chunk)

DeserializeFunction(Chunk){
    ResolvedOption(x, Read<int>)
    ResolvedOption(y, Read<int>)
    ResolvedOption(z, Read<int>)

    this_.worldPosition = glm::ivec3{x,y,z};

    return Deserialize<SparseBlockArray>(this_, array);
}
DeserializeInstatiate(Chunk);
