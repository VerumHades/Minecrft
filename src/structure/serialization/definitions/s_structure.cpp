#include <structure/serialization/serializer.hpp>
#include <structure/serialization/serializer_extras.hpp>

/*
    BlockArray serialization
*/
#include <game/structure.hpp> 

SerializeFunction(Structure) {
    array.append<size_t>(this_.width);
    array.append<size_t>(this_.height);
    array.append<size_t>(this_.depth);

    array.append<size_t>(this_.block_arrays.size());
    for(auto& [position, block_array]: this_.block_arrays){
        array.append<size_t>(position.x);
        array.append<size_t>(position.y);
        array.append<size_t>(position.z);

        Serialize<SparseBlockArray>(block_array, array);
    }

    return true;
}
SerializeInstatiate(Structure)

DeserializeFunction(Structure){
    ResolvedOption(width , read<size_t>);
    ResolvedOption(height, read<size_t>);
    ResolvedOption(depth , read<size_t>);

    ResolvedOption(arrays_total, read<size_t>);
    for(int i = 0;i < arrays_total;i++){
        glm::ivec3 position = {
            array.read<size_t>(),
            array.read<size_t>(),
            array.read<size_t>()
        };

        Deserialize<SparseBlockArray>(this_.block_arrays[position], array);
    }

    return true;
}
DeserializeInstatiate(Structure);