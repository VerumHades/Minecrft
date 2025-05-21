#include <rendering/region_culler.hpp>



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

void MeshRegion::SetChild(uint x, uint y, uint z, bool value){
    if(value) child_registry.set(x + y * 2 + z * 4);
    else child_registry.reset(x + y * 2 + z * 4);

    if(child_registry == 0) registry.removeRegion(this->transform);
}

RegionCuller::RegionCuller(){
    actualRegionSizes.push_back(1);
    for(uint i = 1;i < maxRegionLevel;i++){
        actualRegionSizes.push_back(pow(2, i));
    }
}

bool RegionCuller::addMesh(MeshInterface* mesh, const glm::ivec3& pos){
    std::lock_guard lock(mesh_change_mutex);
    MeshRegion::Transform transform = {pos,1};

    if(mesh->empty()) return true;    
    if(getRegion(transform)) return updateMeshUnguarded(mesh, pos); // InstancedMesh and region already exist
    
    MeshRegion* region = createRegion(transform);
    region->loaded_mesh = mesh_loader->loadMesh(mesh);
    
    return true;
}
bool RegionCuller::updateMeshUnguarded(MeshInterface* mesh, const glm::ivec3& pos){
    MeshRegion::Transform transform = {pos,1};

    if(!getRegion(transform)) return false; // InstancedMesh region doesn't exist
    if(mesh->empty()) return false; // Don't register empty meshes

    auto& region = regions.at(transform);

    if(region.loaded_mesh) region.loaded_mesh->update(mesh);
    else region.loaded_mesh = mesh_loader->loadMesh(mesh);
    
    return true;
}
bool RegionCuller::updateMesh(MeshInterface* mesh, const glm::ivec3& pos){
    std::lock_guard lock(mesh_change_mutex);
    return updateMeshUnguarded(mesh,pos);
}   
bool RegionCuller::removeMesh(const glm::ivec3& pos){
    std::lock_guard lock(mesh_change_mutex);
    MeshRegion::Transform transform = {pos,1};

    if(!getRegion(transform)) return false; // InstancedMesh region doesn't exist

    auto& region = regions.at(transform);

    if(region.loaded_mesh){
        region.loaded_mesh->destroy();
        region.loaded_mesh = nullptr;
    }

    removeRegion(transform);
    
    return true;
}

MeshRegion* RegionCuller::createRegion(MeshRegion::Transform transform){
    if(transform.level > maxRegionLevel) return nullptr; // Over the max region level

    regions.emplace(transform, MeshRegion(transform, *this));
    auto& region = regions.at(transform);

    if(!region.getParentRegion()) 
        createRegion(
            {region.getParentPosition(), region.transform.level + 1}
        );
        
    auto* parent = region.getParentRegion();
    auto pos = region.getParentRelativePosition();
    if(parent) parent->SetChild(pos.x,pos.y,pos.z, true);
    
    return &regions.at(transform);
}


const int halfChunkSize = (CHUNK_SIZE / 2);
void RegionCuller::processRegionForDrawing(Frustum& frustum, MeshRegion* region){

    int level_size_in_chunks = getRegionSizeForLevel(region->transform.level);
    int level_size_in_blocks = level_size_in_chunks * CHUNK_SIZE; 
    
    glm::ivec3 min = region->transform.position * level_size_in_blocks;
    if(!frustum.isAABBWithing(min, min + level_size_in_blocks)) return; // Not visible

    if(region->transform.level == 1){ // The region is directly drawable
        if(!region->loaded_mesh){
            LogError("A loaded region has to have a  mesh!");
            return;
        }

        region->loaded_mesh->addDrawCall(region->transform.position);
        return;
    }

    for(int x = 0; x < 2; x++)
    for(int y = 0; y < 2; y++) 
    for(int z = 0; z < 2; z++)
    { // Go trough all subregions and check them if possible
        MeshRegion* subregion = region->getSubregion(x,y,z);
        if(!subregion) continue; // Region doesn't exist
        
        processRegionForDrawing(frustum, subregion);
    }
    
}

void RegionCuller::updateDrawCalls(const glm::ivec3& camera_position, Frustum& frustum){
    std::lock_guard lock(mesh_change_mutex);
    mesh_loader->clearDrawCalls();

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

        processRegionForDrawing(frustum, region);
    }   

    mesh_loader->flushDrawCalls();
}

void RegionCuller::draw(){
    std::lock_guard lock(mesh_change_mutex);
    mesh_loader->render();
}

void RegionCuller::clear(){
    std::lock_guard lock(mesh_change_mutex);
    regions.clear();
}