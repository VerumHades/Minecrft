#include <game/chunk.hpp>

bool Chunk::serialize(ByteArray& output_array){
    output_array.append<int>(worldPosition.x);
    output_array.append<int>(worldPosition.y);
    output_array.append<int>(worldPosition.z);
    
    return SparseBlockArray::serialize(output_array);
}

void Chunk::deserialize(Chunk& chunk, ByteArray& array){
    chunk.worldPosition = {
        array.read<int>(),
        array.read<int>(),
        array.read<int>(),
    };

    SparseBlockArray::deserialize(chunk, array);
}