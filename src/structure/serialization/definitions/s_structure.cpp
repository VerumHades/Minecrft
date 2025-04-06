#include <structure/serialization/serializer.hpp>
#include <structure/serialization/serializer_extras.hpp>

/*
    BlockArray serialization
*/
#include <game/structure.hpp>

SerializeFunction(Structure) {
    array.Append<size_t>(this_.width);
    array.Append<size_t>(this_.height);
    array.Append<size_t>(this_.depth);

    array.Append<size_t>(this_.block_arrays.size());
    for(auto& [position, block_array]: this_.block_arrays){
        array.Append<int>(position.x);
        array.Append<int>(position.y);
        array.Append<int>(position.z);

        Serialize<SparseBlockArray>(block_array, array);
    }

    return true;
}
SerializeInstatiate(Structure)

DeserializeFunction(Structure){
    ResolvedOption(width , Read<size_t>);
    ResolvedOption(height, Read<size_t>);
    ResolvedOption(depth , Read<size_t>);

    ResolvedOption(arrays_total, Read<size_t>);
    for(int i = 0;i < arrays_total;i++){
        ResolvedOption(x, Read<int>)
        ResolvedOption(y, Read<int>)
        ResolvedOption(z, Read<int>)

        glm::ivec3 position = glm::ivec3{x,y,z};

        Deserialize<SparseBlockArray>(this_.block_arrays[position], array);
    }

    return true;
}
DeserializeInstatiate(Structure);
