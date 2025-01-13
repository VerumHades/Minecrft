#include <game/chunk.hpp>

void Chunk::serialize(ByteArray& output_array){
    SparseBlockArray::serialize(output_array);
    output_array.append<int>(worldPosition.x);
    output_array.append<int>(worldPosition.y);
    output_array.append<int>(worldPosition.z);
}

ByteArray Chunk::serialize(){
    ByteArray output{};
    serialize(output);
    return output;
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