#include <rendering/chunk_buffer.hpp>

bool MeshRegion::setSubregionMeshState(uint32_t x, uint32_t y, uint32_t z, bool state){
    if(x > 1 || y > 1 || z > 1) return false;

    uint32_t index = getSubregionIndexFromPosition(x,y,z);
    if(state) child_states |=  (1 << index); 
    else      child_states &= ~(1 << index);

    if(child_states == 0xFF){ // If all children are meshed
        // Join all the meshes if neccesary
        auto* parent = getParentRegion();
        if(parent) parent->setSubregionMeshState()
    }
    else if(merged){
        // Split apart the meshes because they are no longer valid
    }
}

MeshRegion* MeshRegion::getParentRegion(){
    glm::ivec3 parent_position = {
        transform.position.x / 2,
        transform.position.y / 2,
        transform.position.z / 2, 
    };
    uint32_t parent_level = transform.level + 1;

    return manager.getRegion({parent_position, parent_level});
}

glm::ivec3 MeshRegion::getParentRelativePosition(){
    auto* parent = getParentRegion();
    if(!parent) return {0,0,0};

    return {
        transform.position.x - parent->transform.position.x * 2,
        transform.position.y - parent->transform.position.y * 2,
        transform.position.z - parent->transform.position.z * 2
    };
}

MeshRegion* MeshRegion::getSubregion(uint32_t x, uint32_t y, uint32_t z){
    if(transform.level <= 1) return nullptr;

    glm::ivec3 subregion_position = {
        (transform.position.x * 2) + x,
        (transform.position.y * 2) + y, 
        (transform.position.z * 2) + z,
    };
    uint32_t subregion_level = transform.level - 1;

    return  manager.getRegion({subregion_position, subregion_level});
}

bool MeshRegion::moveMesh(size_t new_vertex_position, size_t new_index_position, size_t index_offset = 0){
    if(!merged && (transform.level != 1)) return; // Moving is not possible for non level 1 or merged regions

    float*   vertex_buffer = manager.getVertexBuffer();
    uint32_t* index_buffer = manager.getIndexBuffer();
    
    if(
        new_vertex_position + mesh_information.vertex_data_size > manager.getVertexBufferSize() ||
        new_vertex_position < 0
    
    ) return false; // Do not overflow the vertex buffer

    if(
        new_index_position  + mesh_information.index_data_size  > manager.getIndexBufferSize() ||
        new_index_position < 0
    ) return false; // Do not overflow the index buffer 

    // Cpoy the vertex data
    std::memcpy(
        vertex_buffer + new_vertex_position, // The new position
        vertex_buffer + mesh_information.vertex_data_start, // The old position
        mesh_information.vertex_data_size * sizeof(float) // The size in bytes
    );

    if(index_offset == 0){ // If there are no recalculations in indices we can just copy them
        std::memcpy(
            index_buffer + new_index_position, // The new position
            index_buffer + mesh_information.index_data_start, // The old position
            mesh_information.index_data_size * sizeof(uint32_t) // The size in bytes
        );
    }
    else{ // Recalculate and copy the indices one by one
        for(int i = 0;i < mesh_information.index_data_size; i++){
            uint32_t index = *(index_buffer + (mesh_information.index_data_start + i)); // Get the index

            *(index_buffer + (new_index_position + i)) = index + index_offset;
        }
    }

    // Register the changes
    mesh_information.vertex_data_start = new_vertex_position;
    mesh_information.index_data_start = new_index_position;

    mesh_information.first_index = new_index_position;
    mesh_information.base_vertex = new_vertex_position / manager.getVertexFormat().getVertexSize(); // Calculate the base vertex in a single vertex sizes
    
    return true;
}


DrawElementsIndirectCommand MeshRegion::generateDrawCommand(){
    return {
        static_cast<GLuint>(mesh_information.count),
        1,
        static_cast<GLuint>(mesh_information.first_index),
        static_cast<GLuint>(mesh_information.base_vertex),
        0
    };
}

void MeshRegionManager::unloadFarawayChunks(const glm::ivec3& from, float treshold){
    for(auto& [position, chunk]: this->loadedChunks){
        float distance = glm::distance(glm::vec3(from), glm::vec3(position));
        if(distance <= treshold) continue;
        unloadChunkMesh(position);
    }
}

/*
    Creates a persistently mapped buffer,
    returns a tuple of [buffer_id, pointer to mapped data]
*/
template <typename T>
static std::tuple<uint32_t, T*> createPersistentBuffer(size_t size, uint32_t type){
    uint32_t buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(type, buffer);
    glBufferStorage(type, size, nullptr, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

    T* pointer = static_cast<T*>(glMapBufferRange(type, 0, size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT));

    if(!pointer) {
        throw std::runtime_error("Failed to map persistent buffer.");
    }

    return {buffer,pointer};
}


std::string formatSize(size_t bytes) {
    const char* suffixes[] = {"B", "KB", "MB", "GB", "TB"};
    size_t suffixIndex = 0;
    double size = static_cast<double>(bytes);

    // Loop to reduce size and find appropriate suffix
    while (size >= 1024 && suffixIndex < 4) {  // Up to TB
        size /= 1024;
        ++suffixIndex;
    }

    // Format size with 2 decimal places
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << size << " " << suffixes[suffixIndex];
    return out.str();
}


void MeshRegionManager::initialize(uint32_t renderDistance){
    vertexFormat = VertexFormat({3,1,2,1,1});
    
    maxDrawCalls = pow(renderDistance * 2, 3);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /*
        Create and map buffer for draw calls
    */

    std::tie(indirectBuffer, persistentDrawCallBuffer) = createPersistentBuffer<DrawElementsIndirectCommand>(
        sizeof(DrawElementsIndirectCommand) * maxDrawCalls * 2,
        GL_DRAW_INDIRECT_BUFFER
    );

    drawCallBufferSize = maxDrawCalls;

    CHECK_GL_ERROR();

    /*
        Create and map vertex and index buffers
    */
    size_t totalMemoryToAllocate = (1024ULL * 1024ULL * 1024ULL) / 2ULL; // 500MB of video memory

    size_t vertexPerFace = vertexFormat.getVertexSize() * 4; // For vertices per face
    size_t indexPerFace  = 6;

    size_t total = vertexPerFace + indexPerFace;
    size_t segment = totalMemoryToAllocate / total;

    vertexBufferSize = segment * vertexPerFace;
    indexBufferSize = segment * indexPerFace;
    
    maxVertexCount = vertexBufferSize / sizeof(float);
    maxIndexCount  = indexBufferSize / sizeof(uint32_t);

    std::cout << "Video RAM allocated for vertices total: " << formatSize(vertexBufferSize) << std::endl;
    std::cout << "Video RAM allocated for indices total : " << formatSize(indexBufferSize)  << std::endl;
    std::cout << "Total Video RAM allocated: " << formatSize(vertexBufferSize + indexBufferSize) << std::endl;

    vertexAllocator = Allocator(maxVertexCount, [this](size_t requested_amount) {return false;});
    indexAllocator = Allocator(maxIndexCount, [this](size_t requested_amount) {return false;});

    std::tie(vertexBuffer, persistentVertexBuffer) = createPersistentBuffer<float>(
        maxVertexCount * sizeof(float),
        GL_ARRAY_BUFFER
    );

    CHECK_GL_ERROR();

    std::tie(indexBuffer, persistentIndexBuffer) = createPersistentBuffer<uint32_t>(
        maxIndexCount * sizeof(uint32_t),
        GL_ELEMENT_ARRAY_BUFFER
    );

    CHECK_GL_ERROR();

    vertexFormat.apply();
}

MeshRegionManager::~MeshRegionManager(){
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteBuffers(1, &indexBuffer);
    glDeleteBuffers(1,  &indirectBuffer);
    glDeleteVertexArrays(1, &vao);
}
std::tuple<bool,size_t,size_t> MeshRegionManager::allocateAndUploadMesh(Mesh& mesh){
    auto vertexAlloc = vertexAllocator.allocate(mesh.getVertices().size());
    auto indexAlloc = indexAllocator.allocate(mesh.getIndices().size());

    if(vertexAlloc.failed || indexAlloc.failed) return {false,0,0};

    size_t vertexBufferOffset = vertexAlloc.location;
    size_t indexBufferOffset = indexAlloc.location;

    /*
        Allocate space for vertex data and save it
    */
    std::memcpy(
        persistentVertexBuffer + vertexBufferOffset, // Copy to the vertex buffer at the offset
        mesh.getVertices().data(), // Copy the vertex data
        mesh.getVertices().size() *  sizeof(float) // Copy the size of the data in bytes
    );
    //mappedVertexBuffer + vertexBufferOffset, mesh.getVertices().data(), mesh.getVertices().size() *  sizeof(GLfloat)
    
    std::memcpy(
        persistentIndexBuffer + indexBufferOffset, // Copy to the vertex buffer at the offset
        mesh.getIndices().data(), // Copy the vertex data
        mesh.getIndices().size() *  sizeof(uint32_t) // Copy the size of the data in bytes
    );

    return {true, vertexBufferOffset, indexBufferOffset};
}

bool MeshRegionManager::addChunkMesh(Mesh& mesh, const glm::ivec3& pos){
    if(loadedChunks.count(pos) != 0) return false;
    if(mesh.getVertices().size() == 0) return false;
    
    //auto start  = std::chrono::high_resolution_clock::now();

    auto [success, vertexBufferOffset, indexBufferOffset] = allocateAndUploadMesh(mesh);
    if(!success) return false;
    /*
        Register the chunk save it as loaded
    */
    loadedChunks[pos] = {
        vertexBufferOffset,
        indexBufferOffset,

        indexBufferOffset,
        mesh.getIndices().size(),
        vertexBufferOffset / vertexFormat.getVertexSize(),

        mesh.getVertices().size(),
        mesh.getIndices().size()
    };

    //auto end = std::chrono::high_resolution_clock::now();

    //auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    //std::cout << "Mesh allocated and updated in: " << duration << " microseconds" << std::endl;
    
    return true;
}
bool MeshRegionManager::swapChunkMesh(Mesh& mesh, const glm::ivec3& pos){
    if(loadedChunks.count(pos) == 0) return false;
    
    auto [success, vertexBufferOffset, indexBufferOffset] = allocateAndUploadMesh(mesh);
    if(!success) return false;

    LoadedChunk& chunk = loadedChunks.at(pos);

    vertexAllocator.free(chunk.vertexData);
    indexAllocator .free(chunk.indexData);
    /*
        Change the draw call arguments
    */
    chunk.vertexData = vertexBufferOffset;
    chunk.indexData = indexBufferOffset;
    
    chunk.firstIndex = indexBufferOffset;
    chunk.count = mesh.getIndices().size();
    chunk.baseVertex = vertexBufferOffset / vertexFormat.getVertexSize();

    return true;
}
void MeshRegionManager::unloadChunkMesh(const glm::ivec3& pos){
    if(loadedChunks.count(pos) == 0) return;
    //removeDrawCall(pos);
    vertexAllocator.free(loadedChunks[pos].vertexData);
    indexAllocator.free(loadedChunks[pos].indexData);
    loadedChunks.erase(pos);
}

DrawElementsIndirectCommand MeshRegionManager::getCommandFor(const glm::ivec3& position){
    if(loadedChunks.count(position) == 0) return {};

    LoadedChunk& chunk = loadedChunks.at(position);

    return {static_cast<GLuint>(chunk.count),1,static_cast<GLuint>(chunk.firstIndex),static_cast<GLuint>(chunk.baseVertex),0};
}

/*void MeshRegionManager::updateDrawCalls(std::vector<DrawElementsIndirectCommand>& commands){
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);

    if (commands.size() > maxDrawCalls) {
        throw std::runtime_error("Exceeded maximum number of draw calls.");
    }

    bufferOffset = (bufferOffset + maxDrawCalls) % (maxDrawCalls * 2);
    std::memcpy(persistentDrawCallBuffer + bufferOffset, commands.data(), sizeof(DrawElementsIndirectCommand) * commands.size());

    drawCallCount = commands.size();

    std::memcpy(persistentDrawCallBuffer, commands.data(), sizeof(DrawElementsIndirectCommand) * commands.size());

    drawCallCount = commands.size();

    CHECK_GL_ERROR();
}*/

void MeshRegionManager::draw(){
    glBindVertexArray(vao);

    //int drawCalls = maxDrawCalls - freeDrawCallIndices.size();
    //std::cout << "Active draw calls: " << lastDrawCall << std::endl;
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(bufferOffset * sizeof(DrawElementsIndirectCommand)), drawCallCount, sizeof(DrawElementsIndirectCommand));

    CHECK_GL_ERROR();
}

void MeshRegionManager::clear(){
    vertexAllocator.clear();
    indexAllocator.clear();
    loadedChunks.clear();
}