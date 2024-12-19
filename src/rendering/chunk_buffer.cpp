#include <rendering/chunk_buffer.hpp>



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
    actualRegionSizes.push_back(1);
    for(int i = 1;i < maxRegionLevel;i++){
        actualRegionSizes.push_back(pow(i, 2));
    }
}

bool ChunkMeshRegistry::addMesh(InstancedMesh* mesh, const glm::ivec3& pos){
    MeshRegion::Transform transform = {pos,1};

    if(mesh->empty()) return true;    
    if(getRegion(transform)) return updateMesh(mesh, pos); // InstancedMesh and region already exist
    
    MeshRegion* region = createRegion(transform);
   
    region->loaded_mesh = 
        std::make_unique<InstancedMeshBuffer::LoadedMesh>(
            std::move(mesh_buffer.loadMesh(*mesh))
        );

    return true;
}

bool ChunkMeshRegistry::updateMesh(InstancedMesh* mesh, const glm::ivec3& pos){
    MeshRegion::Transform transform = {pos,1};

    if(!getRegion(transform)) return false; // InstancedMesh region doesn't exist
    if(mesh->empty()) return false; // Don't register empty meshes

    auto& region = regions.at(transform);
    region.loaded_mesh->update(*mesh);
    
    return true;
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
    //if(region->draw_commands.size() == 0 && region->transform.level != 1) return; // No meshes are present, no point in searching
    
    int level_size_in_chunks = getRegionSizeForLevel(region->transform.level);
    int level_size_in_blocks = level_size_in_chunks * CHUNK_SIZE; 

    glm::ivec3 min = region->transform.position * level_size_in_blocks;
    if(!frustum.isAABBWithing(min, min + level_size_in_blocks)) return; // Not visible

    if(region->transform.level == 1){ // The region is directly drawable
        region->loaded_mesh->addDrawCall();
        return;
    }
    /*else{
        size_t draw_calls_size = region->draw_commands.size();
        drawCallBuffer->appendData(region->draw_commands.data(), draw_calls_size);

        draw_call_counter += draw_calls_size;

        return;
    }*/


    for(int x = 0; x < 2; x++)
    for(int y = 0; y < 2; y++) 
    for(int z = 0; z < 2; z++)
    { // Go trough all subregions and check them if possible
        MeshRegion* subregion = region->getSubregion(x,y,z);
        if(!subregion) continue; // Region doesn't exist
        
        processRegionForDrawing(frustum, subregion, draw_call_counter);
    }
    
}

void ChunkMeshRegistry::updateDrawCalls(glm::ivec3 camera_position, Frustum& frustum){
    mesh_buffer.clearDrawCalls();

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
        if(!region) continue;

        processRegionForDrawing(frustum, region, drawCallCount);
    }   

    mesh_buffer.flushDrawCalls();
}

void ChunkMeshRegistry::draw(){
    mesh_buffer.render();
}

void ChunkMeshRegistry::clear(){
    regions.clear();
}