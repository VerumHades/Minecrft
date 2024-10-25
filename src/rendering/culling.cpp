#include <rendering/culling.hpp>

const int halfChunkSize = (CHUNK_SIZE / 2);
void processSubChunks(Frustum& frustum, glm::ivec3 offset, int size, ChunkFoundCallback chunkFound){
    int sub_size = size / 2;
    int real_size = halfChunkSize * sub_size;

    if(sub_size < 1){
        std::cout << "shouldnt happen" << std::endl;
        return;
    }

    for(int x = 0;x < 2;x++) for(int y = 0;y < 2;y++) for(int z = 0;z < 2;z++){
        glm::ivec3 pos = offset + glm::ivec3(x * sub_size, y * sub_size, z * sub_size);
        //std::cout << pos.x << " " << pos.y << " " << pos.z  << " of size: " << sub_size << std::endl;
        
        if(!frustum.isAABBWithing(pos * real_size + (glm::ivec3(real_size) / 2), real_size)) continue;
        
        if(sub_size == 1){
            chunkFound(pos);
            continue;
        }

        processSubChunks(frustum, pos, sub_size, chunkFound);
    }
}

void findVisibleChunks(
    Frustum& frustum,
    int renderDistance,
    ChunkFoundCallback chunkFound
){
    processSubChunks(frustum, {-renderDistance,-renderDistance,-renderDistance}, renderDistance * 2, chunkFound);
}