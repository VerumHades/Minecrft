#include <rendering/chunk_buffer.hpp>

void MeshRegion::setMeshless(bool value){
    if(value == meshless && !meshless_first) return; // Already set
    meshless_first = false;

    meshless = value;

    if(value){ // Being set to meshless
        //updateMeshInformation({0});
    }
    else{ // Being set to having a mesh
        propagateDrawCall();
        updatePropagatedDrawCalls();
    }
}

bool MeshRegion::propagateDrawCall(){
    if(transform.level != 1 || propagated) return false; // Non level 1 regions cannot propagate, cannot propagate twice
    
    MeshRegion* current_region = this;
    DrawElementsIndirectCommand command = generateDrawCommand();

    propagated = true;

    for(;;){
        current_region = current_region->getParentRegion();
        if(!current_region) return false;
        
        in_parent_draw_call_references.push_back( 
            {current_region->transform, current_region->draw_commands.size()}
        );

        current_region->draw_commands.push_back(command);
    }

    return true;
}

MeshRegion* MeshRegion::getParentRegion(){
    glm::ivec3 parent_position = getParentPosition();
    uint parent_level = transform.level + 1;

    return registry.getRegion({parent_position, parent_level});
}

glm::ivec3 MeshRegion::getParentRelativePosition(){
    auto* parent = getParentRegion();
    if(!parent) return {0,0,0};

    return transform.position - parent->transform.position * 2;
}

MeshRegion* MeshRegion::getSubregion(uint x, uint y, uint z){
    if(transform.level <= 1) return nullptr;

    glm::ivec3 subregion_position = {
        (transform.position.x * 2) + x,
        (transform.position.y * 2) + y, 
        (transform.position.z * 2) + z,
    };
    uint subregion_level = transform.level - 1;

    return  registry.getRegion({subregion_position, subregion_level});
}



bool MeshRegion::updateMeshInformation(MeshInformation information){
    if(transform.level != 1 || !propagated) return false;

    mesh_information = information;
    updatePropagatedDrawCalls();

    return true;
}

bool MeshRegion::updatePropagatedDrawCalls(){
    DrawElementsIndirectCommand command = generateDrawCommand();
    
    for(auto& [transform, index]: in_parent_draw_call_references){
        MeshRegion* region = registry.getRegion(transform);
        if(!region){
            std::cerr << "Draw call reference is invalid for the region doesnt exist?" <<  std::endl;
            continue;
        }

        if(index >= region->draw_commands.size()){
            std::cerr << "Invalid draw call index stored: " << index << std::endl;
            continue;
        }

        region->draw_commands[index] = command;
    }

    return true;
}

DrawElementsIndirectCommand MeshRegion::generateDrawCommand(){
    return {
        static_cast<GLuint>(mesh_information.count),
        1,
        static_cast<GLuint>(mesh_information.first_index),
        static_cast<GLuint>(mesh_information.base_vertex) ,
        0
    };
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


void ChunkMeshRegistry::initialize(uint renderDistance){
    vertexFormat = VertexFormat({3,1,2,1,1});

    actualRegionSizes.push_back(1);
    for(int i = 1;i < maxRegionLevel;i++){
        actualRegionSizes.push_back(pow(i, 2));
    }
    
    maxDrawCalls = pow((renderDistance + 1) * 2, 3);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /*
        Create and map buffer for draw calls
    */

    drawCallBuffer = GLConstantDoubleBuffer<DrawElementsIndirectCommand, GL_DRAW_INDIRECT_BUFFER>(maxDrawCalls);

    //syncObj = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    CHECK_GL_ERROR();

    /*
        Create and map vertex and index buffers
    */
    size_t totalMemoryToAllocate = (1024ULL * 1024ULL * 1024ULL) / 4ULL; // 1/4GB of video memory

    size_t vertexPerFace = vertexFormat.getVertexSize() * 4; // For vertices per face
    size_t indexPerFace  = 6;

    size_t total = vertexPerFace + indexPerFace;
    size_t segment = totalMemoryToAllocate / total;

    vertexBufferSize = segment * vertexPerFace;
    indexBufferSize = segment * indexPerFace;
    
    maxVertexCount = vertexBufferSize / sizeof(float);
    maxIndexCount  = indexBufferSize / sizeof(uint);

    std::cout << "Video RAM allocated for vertices total: " << formatSize(vertexBufferSize) << std::endl;
    std::cout << "Video RAM allocated for indices total : " << formatSize(indexBufferSize)  << std::endl;
    std::cout << "Total Video RAM allocated: " << formatSize(vertexBufferSize + indexBufferSize) << std::endl;

    vertexAllocator = Allocator(maxVertexCount, [this](size_t requested_amount) {return false;});
    indexAllocator = Allocator(maxIndexCount, [this](size_t requested_amount) {return false;});

    persistentVertexBuffer = std::make_unique<GLPersistentBuffer<float>>(
        maxVertexCount * sizeof(float),
        GL_ARRAY_BUFFER
    );

    CHECK_GL_ERROR();

    persistentIndexBuffer = std::make_unique<GLPersistentBuffer<uint>>(
        maxIndexCount * sizeof(uint),
        GL_ELEMENT_ARRAY_BUFFER
    );

    CHECK_GL_ERROR();

    vertexFormat.apply();
}

ChunkMeshRegistry::~ChunkMeshRegistry(){
    glDeleteVertexArrays(1, &vao);
}

std::tuple<bool,size_t,size_t> ChunkMeshRegistry::allocateAndUploadMesh(Mesh* mesh){
    auto [vertexSuccess, vertexBufferOffset] = vertexAllocator.allocate(mesh->getVertices().size());
    auto [indexSuccess, indexBufferOffset] = indexAllocator.allocate(mesh->getIndices().size());

    if(!vertexSuccess || !indexSuccess) return {false,0,0};

    /*
        Allocate space for vertex data and save it
    */
    std::memcpy(
        persistentVertexBuffer->data() + vertexBufferOffset, // Copy to the vertex buffer at the offset
        mesh->getVertices().data(), // Copy the vertex data
        mesh->getVertices().size() *  sizeof(float) // Copy the size of the data in bytes
    );
    //mappedVertexBuffer + vertexBufferOffset, mesh->getVertices().data(), mesh->getVertices().size() *  sizeof(GLfloat)
    
    std::memcpy(
        persistentIndexBuffer->data()  + indexBufferOffset, // Copy to the vertex buffer at the offset
        mesh->getIndices().data(), // Copy the vertex data
        mesh->getIndices().size() *  sizeof(uint) // Copy the size of the data in bytes
    );

    return {true, vertexBufferOffset, indexBufferOffset};
}

bool ChunkMeshRegistry::addMesh(Mesh* mesh, const glm::ivec3& pos){
    MeshRegion::Transform transform = {pos,1};
    
    if(getRegion(transform)) return false; // Mesh and region already exist

    if(mesh->getVertices().size() == 0){
        MeshRegion* region = createRegion(transform);
        region->setMeshless(true);
        return true;
    }

    auto [success, vertexBufferOffset, indexBufferOffset] = allocateAndUploadMesh(mesh);
    if(!success) return false;
    
    MeshRegion* region = createRegion(transform);
    region->mesh_information = {
        vertexBufferOffset,
        indexBufferOffset,

        mesh->getVertices().size(),
        mesh->getIndices().size(),

        indexBufferOffset,
        mesh->getIndices().size(),
        vertexBufferOffset / vertexFormat.getVertexSize(),
    };
    region->setMeshless(false);

    return true;
}

bool ChunkMeshRegistry::updateMesh(Mesh* mesh, const glm::ivec3& pos){
    MeshRegion::Transform transform = {pos,1};

    if(!getRegion(transform)) return false; // Mesh region doesn't exist
    if(mesh->getVertices().size() == 0){
        getRegion(transform)->setMeshless(true);
        return false; // Don't register empty meshes
    }

    auto [success, vertexBufferOffset, indexBufferOffset] = allocateAndUploadMesh(mesh);
    if(!success) return false;

    auto& region = regions.at(transform);

    size_t old_vertex_data = region.mesh_information.vertex_data_start;
    size_t old_index_data  = region.mesh_information.index_data_start;

    bool update_success = region.updateMeshInformation(
        {
            vertexBufferOffset,
            indexBufferOffset,

            mesh->getVertices().size(),
            mesh->getIndices().size(),

            indexBufferOffset,
            mesh->getIndices().size(),
            vertexBufferOffset / vertexFormat.getVertexSize(),
        }
    );
    if(!update_success){ // If updating the information fails clean up the allocations
        vertexAllocator.free(vertexBufferOffset);
        indexAllocator .free(indexBufferOffset);
        return false;
    }

    vertexAllocator.free(old_vertex_data);
    indexAllocator .free(old_index_data);

    region.setMeshless(false);

    return true;
}   

DrawElementsIndirectCommand ChunkMeshRegistry::getCommandFor(const glm::ivec3& position){
    MeshRegion* region = getRegion({position,1});
    if(!region) return {};

    return region->generateDrawCommand();
}

MeshRegion* ChunkMeshRegistry::createRegion(MeshRegion::Transform transform){
    if(transform.level > maxRegionLevel) return nullptr; // Over the max region level

    regions.emplace(transform, MeshRegion(transform, *this));
    auto& region = regions.at(transform);

    if(!region.getParentRegion()) 
        createRegion(
            {region.getParentPosition(), region.transform.level + 1}
        );

    return &regions.at(transform);
}


const int halfChunkSize = (CHUNK_SIZE / 2);
void ChunkMeshRegistry::processRegionForDrawing(Frustum& frustum, MeshRegion* region, size_t& draw_call_counter){
    if(region->draw_commands.size() == 0 && region->transform.level != 1) return; // No meshes are present, no point in searching
    
    int level_size_in_chunks = getRegionSizeForLevel(region->transform.level);
    int level_size_in_blocks = level_size_in_chunks * CHUNK_SIZE; 

    //glm::ivec3 min = region->transform.position * level_size_in_blocks;
    //if(!frustum.isAABBWithing(min, min + level_size_in_blocks)) return; // Not visible

    if(region->transform.level == 1){ // The region is directly drawable
        DrawElementsIndirectCommand command = region->generateDrawCommand();
        drawCallBuffer.appendData(&command, 1);
        draw_call_counter++;
        return;
    }
    else{
        size_t draw_calls_size = region->draw_commands.size();
        drawCallBuffer.appendData(region->draw_commands.data(), draw_calls_size);

        draw_call_counter += draw_calls_size;

        return;
    }


    for(int x = 0; x < 2; x++)
    for(int y = 0; y < 2; y++) 
    for(int z = 0; z < 2; z++)
    { // Go trough all subregions and check them if possible
        MeshRegion* subregion = region->getSubregion(x,y,z);
        if(!subregion) continue; // Region doesn't exist
        if(subregion->meshless) continue; // Dont bother calling the function for meshless chunks
        
        processRegionForDrawing(frustum, subregion, draw_call_counter);
    }
    
}

void ChunkMeshRegistry::updateDrawCalls(glm::ivec3 camera_position, Frustum& frustum){
    drawCallCount = 0;

    int max_level_size_in_chunks = getRegionSizeForLevel(maxRegionLevel);

    glm::ivec3 center_position = {
        std::floor(static_cast<float>(camera_position.x) / (max_level_size_in_chunks * CHUNK_SIZE)),
        std::floor(static_cast<float>(camera_position.y) / (max_level_size_in_chunks * CHUNK_SIZE)),
        std::floor(static_cast<float>(camera_position.z) / (max_level_size_in_chunks * CHUNK_SIZE))
    };

    const int range = 1;

    for(int x = -range;x <= range;x++)
    for(int y = -range;y <= range;y++)
    for(int z = -range;z <= range;z++)
    {   
        auto position = center_position + glm::ivec3(x,y,z);
        MeshRegion* region = getRegion({position, maxRegionLevel});
        if(!region){
            //std::cout << "Core region not found?" << std::endl;
            continue;
        }

        processRegionForDrawing(frustum, region, drawCallCount);
    }   

    drawCallBuffer.flush();
}

void ChunkMeshRegistry::draw(){
    glBindVertexArray(vao);

    //int drawCalls = maxDrawCalls - freeDrawCallIndices.size();
    //std::cout << "Active draw calls: " << drawCallCount << " " <<  sizeof(DrawElementsIndirectCommand) << std::endl;

    CHECK_GL_ERROR();

    glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (void*)(drawCallBufferOffset * sizeof(DrawElementsIndirectCommand)), drawCallCount, sizeof(DrawElementsIndirectCommand));

    CHECK_GL_ERROR();
}

void ChunkMeshRegistry::clear(){
    vertexAllocator.clear();
    indexAllocator.clear();
    regions.clear();
}