#include <rendering/chunk_buffer.hpp>

bool MeshRegion::setSubregionMeshState(uint32_t x, uint32_t y, uint32_t z, bool state){
    if(x > 1 || y > 1 || z > 1) return false;

    uint32_t index = getSubregionIndexFromPosition(x,y,z);
    if(state) child_states |=  (1 << index); 
    else      child_states &= ~(1 << index);

    if(child_states == 0xFF) setStateInParent(true); // If all children are meshed
    else if(merged){
        // Split apart the meshes because they are no longer valid
    }
}

bool MeshRegion::setStateInParent(bool state){
    auto* parent = getParentRegion();
    if(!parent) return false;
    
    auto relative_position = getParentRelativePosition();
    parent->setSubregionMeshState(relative_position.x, relative_position.y, relative_position.z, state);
    
    return true;
}

MeshRegion* MeshRegion::getParentRegion(){
    glm::ivec3 parent_position = getParentPosition();
    uint32_t parent_level = transform.level + 1;

    return registry.getRegion({parent_position, parent_level});
}

glm::ivec3 MeshRegion::getParentRelativePosition(){
    auto* parent = getParentRegion();
    if(!parent) return {0,0,0};

    return transform.position - parent->transform.position * 2;
}

MeshRegion* MeshRegion::getSubregion(uint32_t x, uint32_t y, uint32_t z){
    if(transform.level <= 1) return nullptr;

    glm::ivec3 subregion_position = {
        (transform.position.x * 2) + x,
        (transform.position.y * 2) + y, 
        (transform.position.z * 2) + z,
    };
    uint32_t subregion_level = transform.level - 1;

    return  registry.getRegion({subregion_position, subregion_level});
}

bool MeshRegion::moveMesh(size_t new_vertex_position, size_t new_index_position, size_t index_offset = 0){
    if(!merged && (transform.level != 1)) return; // Moving is not possible for non level 1 or merged regions

    float*   vertex_buffer = registry.getVertexBuffer();
    uint32_t* index_buffer = registry.getIndexBuffer();
    
    if(
        new_vertex_position + mesh_information.vertex_data_size > registry.getVertexBufferSize() ||
        new_vertex_position < 0
    
    ) return false; // Do not overflow the vertex buffer

    if(
        new_index_position  + mesh_information.index_data_size  > registry.getIndexBufferSize() ||
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

            *(index_buffer + (new_index_position + i)) = (index - mesh_information.index_offset) + index_offset;
        }
    }

    // Register the changes
    mesh_information.vertex_data_start = new_vertex_position;
    mesh_information.index_data_start = new_index_position;

    mesh_information.first_index = new_index_position;
    mesh_information.base_vertex = new_vertex_position / registry.getVertexFormat().getVertexSize(); // Calculate the base vertex in a single vertex sizes

    mesh_information.index_offset = index_offset;
    
    return true;
}

bool MeshRegion::merge(){
    if(transform.level <= 1) return false; // Cannot merge level 1 mesh
    if(child_states != 0xFF) return false; // Not all children are meshed
    if(merged) return false; // Cant merge a mesh twice

    size_t vertex_size_total = 0;
    size_t index_size_total = 0;

    for(int x = 0; x < 2; x++)
    for(int y = 0; y < 2; y++) 
    for(int z = 0; z < 2; z++)
    { // Go trough all subregions and merge them if possible
        MeshRegion* subregion = getSubregion(x,y,z);
        if(!subregion) return false; // Region doesn't exist
        
        if(subregion->hasContiguousMesh() || subregion->merge()){
            vertex_size_total += subregion->mesh_information.vertex_data_size;
            index_size_total  += subregion->mesh_information.index_data_size;
        } // A mergable mesh
        else return false; // Fail if isn't and cannot be made contiguous
    }
    
    auto [vertex_success, new_vertex_position] = registry.getVertexAllocator().allocate(vertex_size_total);
    auto [index_success , new_index_position] = registry.getIndexAllocator().allocate(index_size_total);

    if(!vertex_success || !index_success) return false; // Failed to allocate space for the new mesh

    size_t vertex_offset = 0;
    size_t index_offset  = 0;

    // All meshes should be mergable at this point
    for(int x = 0; x < 2; x++)
    for(int y = 0; y < 2; y++) 
    for(int z = 0; z < 2; z++)
    { // Go trough all subregions and merge them if possible
        MeshRegion* subregion = getSubregion(x,y,z);
        if(!subregion) return false; // Region doesn't exist, but this would have already failed in the previous loop

        bool moved = subregion->moveMesh(
            new_vertex_position + vertex_offset,
            new_index_position  + index_offset,
            vertex_offset / registry.getVertexFormat().getVertexSize() // Offset is in individual floats, this division makes it in vertices 
        ); 

        if(!moved) throw std::runtime_error("Mesh move failed when merging, something is very wrong."); // Shouldnt ever happen, there is no way to revert moved meshes so just crash

        vertex_offset += subregion->mesh_information.vertex_data_size;
        index_offset  += subregion->mesh_information.index_data_size;

        registry.getVertexAllocator().free(subregion->mesh_information.vertex_data_start);
        registry.getIndexAllocator().free(subregion->mesh_information.index_data_start);

        subregion->part_of_parent_mesh = true;
    }

    mesh_information.vertex_data_start = new_vertex_position;
    mesh_information.index_data_start = new_index_position;

    mesh_information.first_index = new_index_position;
    mesh_information.base_vertex = new_vertex_position / registry.getVertexFormat().getVertexSize(); // Calculate the base vertex in a single vertex sizes

    mesh_information.vertex_data_size = vertex_size_total;
    mesh_information.index_data_size = index_size_total;

    mesh_information.index_offset = 0;

    return true;
}

void MeshRegion::recalculateIndicies(){
    uint32_t* index_buffer = registry.getIndexBuffer();
    
    for(int i = 0;i < mesh_information.index_data_size; i++){
        *(index_buffer + (mesh_information.index_data_start + i)) -= mesh_information.index_offset;
    }

    mesh_information.index_offset = 0;
}

bool MeshRegion::split(){
    if(!merged) return false; // Cannot split non-merged chunk

    if(part_of_parent_mesh){ // Split parent mesh if its merged
        MeshRegion* parent_region = getParentRegion();
        if(!parent_region) return false; // There is no parent region but its the part of it?
        if(!parent_region->split()) return false; // Parent region couldn't be split, so this can't either
    }

    for(int x = 0; x < 2; x++)
    for(int y = 0; y < 2; y++) 
    for(int z = 0; z < 2; z++)
    { // Go trough all subregions and merge them if possible
        MeshRegion* subregion = getSubregion(x,y,z);
        if(!subregion) return false; // Region doesn't exist
        
        subregion->recalculateIndicies();
        subregion->part_of_parent_mesh = false;
    }

    merged = false;
    return true;
}

bool MeshRegion::updateMeshInformation(MeshInformation information){
    if(part_of_parent_mesh){ // Split parent mesh if its merged
        MeshRegion* parent_region = getParentRegion();
        if(!parent_region) return false; // There is no parent region but its the part of it?
        if(!parent_region->split()) return false; // Parent region couldn't be split, so this can't either
    }

    mesh_information = information;
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

void ChunkMeshRegistry::unloadFarawayChunks(const glm::ivec3& from, float treshold){
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


void ChunkMeshRegistry::initialize(uint32_t renderDistance){
    vertexFormat = VertexFormat({3,1,2,1,1});

    actualRegionSizes.push_back(1);
    for(int i = 1;i < maxRegionLevel;i++){
        actualRegionSizes.push_back(pow(i, 2));
    }
    
    maxDrawCalls = pow(renderDistance * 2, 3);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /*
        Create and map buffer for draw calls
    */

    std::tie(indirectBufferID, persistentDrawCallBuffer) = createPersistentBuffer<DrawElementsIndirectCommand>(
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

    std::tie(vertexBufferID, persistentVertexBuffer) = createPersistentBuffer<float>(
        maxVertexCount * sizeof(float),
        GL_ARRAY_BUFFER
    );

    CHECK_GL_ERROR();

    std::tie(indexBufferID, persistentIndexBuffer) = createPersistentBuffer<uint32_t>(
        maxIndexCount * sizeof(uint32_t),
        GL_ELEMENT_ARRAY_BUFFER
    );

    CHECK_GL_ERROR();

    vertexFormat.apply();
}

ChunkMeshRegistry::~ChunkMeshRegistry(){
    glDeleteBuffers(1, &vertexBufferID);
    glDeleteBuffers(1, &indexBufferID);
    glDeleteBuffers(1,  &indirectBufferID);
    glDeleteVertexArrays(1, &vao);
}

std::tuple<bool,size_t,size_t> ChunkMeshRegistry::allocateAndUploadMesh(Mesh& mesh){
    auto [vertexSuccess, vertexBufferOffset] = vertexAllocator.allocate(mesh.getVertices().size());
    auto [indexSuccess, indexBufferOffset] = indexAllocator.allocate(mesh.getIndices().size());

    if(!vertexSuccess || !indexSuccess) return {false,0,0};

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

bool ChunkMeshRegistry::addMesh(Mesh& mesh, const glm::ivec3& pos){
    MeshRegion::Transform transform = {pos,1};
    
    if(getRegion(transform)) return false; // Mesh and region already exist
    if(mesh.getVertices().size() == 0) return false; // Dont register empty meshes
    
    auto [success, vertexBufferOffset, indexBufferOffset] = allocateAndUploadMesh(mesh);
    if(!success) return false;
    
    MeshRegion* region = createRegion(transform);
    region->mesh_information = {
        vertexBufferOffset,
        indexBufferOffset,

        mesh.getVertices().size(),
        mesh.getIndices().size(),

        indexBufferOffset,
        mesh.getIndices().size(),
        vertexBufferOffset / vertexFormat.getVertexSize(),
    };
    region->setStateInParent(true);

    return true;
}

bool ChunkMeshRegistry::updateMesh(Mesh& mesh, const glm::ivec3& pos){
    MeshRegion::Transform transform = {pos,1};

    if(!getRegion(transform)) return false; // Mesh region doesn't exist
    if(mesh.getVertices().size() == 0) return false; // Don't register empty meshes

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
}   

bool ChunkMeshRegistry::swapChunkMesh(Mesh& mesh, const glm::ivec3& pos){
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
void ChunkMeshRegistry::unloadChunkMesh(const glm::ivec3& pos){
    if(loadedChunks.count(pos) == 0) return;
    //removeDrawCall(pos);
    vertexAllocator.free(loadedChunks[pos].vertexData);
    indexAllocator.free(loadedChunks[pos].indexData);
    loadedChunks.erase(pos);
}

DrawElementsIndirectCommand ChunkMeshRegistry::getCommandFor(const glm::ivec3& position){
    if(loadedChunks.count(position) == 0) return {};

    LoadedChunk& chunk = loadedChunks.at(position);

    return {static_cast<GLuint>(chunk.count),1,static_cast<GLuint>(chunk.firstIndex),static_cast<GLuint>(chunk.baseVertex),0};
}

MeshRegion* ChunkMeshRegistry::createRegion(MeshRegion::Transform transform){
    if(transform.level > maxRegionLevel) return nullptr; // Over the max region level

    regions.emplace(transform, transform, *this);
    auto& region = regions.at(transform);

    if(!region.getParentRegion()) 
        createRegion(
            {region.getParentPosition(), region.transform.level + 1}
        );

    return &regions.at(transform);
}

/*void ChunkMeshRegistry::updateDrawCalls(std::vector<DrawElementsIndirectCommand>& commands){
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBufferID);

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

void ChunkMeshRegistry::draw(){
    glBindVertexArray(vao);

    //int drawCalls = maxDrawCalls - freeDrawCallIndices.size();
    //std::cout << "Active draw calls: " << lastDrawCall << std::endl;
    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(bufferOffset * sizeof(DrawElementsIndirectCommand)), drawCallCount, sizeof(DrawElementsIndirectCommand));

    CHECK_GL_ERROR();
}

void ChunkMeshRegistry::clear(){
    vertexAllocator.clear();
    indexAllocator.clear();
    loadedChunks.clear();
}