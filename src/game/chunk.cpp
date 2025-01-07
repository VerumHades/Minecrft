#include <game/chunk.hpp>

ByteArray Chunk::serialize(){
    auto block_array = SparseBlockArray::serialize();
    
    block_array.append<int>(worldPosition.x);
    block_array.append<int>(worldPosition.y);
    block_array.append<int>(worldPosition.z);

    return block_array;
}
Chunk Chunk::deserialize(ByteArray& array){
    auto data = SparseBlockArray::deserialize(array);

    glm::ivec3 worldPosition = {
        array.read<int>(),
        array.read<int>(),
        array.read<int>(),
    };

    Chunk out{worldPosition};
    *reinterpret_cast<SparseBlockArray*>(&out) = data;

    return out;
}