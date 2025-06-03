#include <rendering/vertex_pooling/pooled_mesh.hpp>

void PooledMesh::addQuadFace(const glm::ivec3& position, float width, float height, int texture_index, FaceType type, Direction direction, const std::array<float, 4>& occlusion, const glm::vec3& world_position){
    uint32_t first_portion = 0;

    if(position.x > 63 || position.z > 63) std::cout << "Uhh oh!" << position.x << " " << position.y << " " << position.z << std::endl;

    first_portion |= (static_cast<unsigned int>(position.x) & 0b111111);
    first_portion |= (static_cast<unsigned int>(position.y) & 0b1111111) << 6;
    first_portion |= (static_cast<unsigned int>(position.z) & 0b111111) << 13;
    
    first_portion |= (static_cast<unsigned int>(width)      & 0b111111) << 19;
    first_portion |= (static_cast<unsigned int>(height)     & 0b111111) << 25;

    first_portion |= (0b1 & direction) << 31;

    uint32_t second_portion = 0;
    for(int i = 0;i < 4;i++)
        second_portion |= (static_cast<unsigned int>(occlusion[i]) & 0b11) << (i * 2);
    
    second_portion |= (static_cast<unsigned int>(texture_index) & 0xFFFFFF) << 8;

    data.Push(type, std::vector<uint32_t>{first_portion, second_portion});
}

void PooledMesh::preallocate(size_t size, FaceType type){
    data.Reserve(type,size);
}

bool PooledMesh::empty(){
    return data.GetAll().empty();
}

void PooledMesh::shrink(){
    data.Shrink();
}


PooledMeshLoader::PooledMeshLoader(){
    
}

void PooledMeshLoader::LoadedMesh::destroy(){
    if(!valid) throw std::logic_error("Cannot destroy destroyed mesh.");
    creator.removeMesh(*this);
    valid = false;
}

void PooledMeshLoader::LoadedMesh::update(MeshInterface* mesh_){
    if(!valid) throw std::logic_error("Cannot update destroyed mesh.");
    
    auto mesh_ptr = dynamic_cast<PooledMesh*>(mesh_);
    if(!mesh_ptr) return;

    auto& mesh = *mesh_ptr;

    creator.updateMesh(*this, mesh);
}

void PooledMeshLoader::LoadedMesh::addDrawCall(const glm::ivec3& position){
    if(!valid) throw std::logic_error("Cannot add draw call of destroyed mesh.");
    creator.addDrawCall(*this, position);
}

std::unique_ptr<LoadedMeshInterface> PooledMeshLoader::loadMesh(MeshInterface* mesh_){
    auto mesh_ptr = dynamic_cast<PooledMesh*>(mesh_);
    if(!mesh_ptr) return nullptr;

    auto& mesh = *mesh_ptr;

    auto loaded_mesh = std::make_unique<PooledMeshLoader::LoadedMesh>(*this);

    for(size_t i = 0;i < distinct_face_count;i++){
        auto type = static_cast<PooledMesh::FaceType>(i);

        auto& component_data = mesh.GetData().Get(type);
        if(component_data.size() == 0){
            loaded_mesh->has_region[i] = false;
            continue;
        }

        loaded_mesh->loaded_regions[i] = render_information[i].mesh_data.append(component_data.data(), component_data.size());
        loaded_mesh->has_region[i] = true;

        render_information[i].mesh_data.flush();
    }

    return loaded_mesh;
}

void PooledMeshLoader::updateMesh(LoadedMesh& loaded_mesh, PooledMesh& new_mesh){
    for(size_t i = 0;i < distinct_face_count;i++){
        auto& component_data = new_mesh.GetData().Get(static_cast<PooledMesh::FaceType>(i));
        if(component_data.size() == 0){
            loaded_mesh.has_region[i] = false;
            continue;
        }
    
        if(loaded_mesh.has_region[i])
            loaded_mesh.loaded_regions[i] = render_information[i].mesh_data.update(loaded_mesh.loaded_regions[i], component_data.data(), component_data.size());
        else
            loaded_mesh.loaded_regions[i] = render_information[i].mesh_data.append(component_data.data(), component_data.size());

        loaded_mesh.has_region[i] = true;

        render_information[i].mesh_data.flush();
    }
}

void PooledMeshLoader::addDrawCall(LoadedMesh& mesh, const glm::ivec3& position){
    if(!legacy_mode){
        world_positions.push_back(position);
        updated_world_positions = true;
    }

    LegacyCall call{};
    call.world_position = position;

    for(size_t i = 0;i < distinct_face_count;i++){
        auto& info = render_information[i];

        if(!mesh.has_region[i]){ // Maintain alignment
            if(legacy_mode){
                call.starts[i] = 0;
                call.counts[i] = 0;
            }
            else{
                GLDrawCallBuffer::DrawCommand draw_call = {
                    0, // Count
                    0, // Number of instances to draw,
                    0, // First vertex
                    0 // Instance offset
                };
        
                render_information[i].draw_call_buffer.push(draw_call);
            }

            continue;
        }
        
        size_t instances_total = mesh.loaded_regions[i]->size  / PooledMesh::face_size;
        size_t instance_offset = mesh.loaded_regions[i]->start / PooledMesh::face_size;

        if(legacy_mode) {
            call.starts[i] = instance_offset * 6;
            call.counts[i] = instances_total * 6;
        }
        else{
            GLDrawCallBuffer::DrawCommand draw_call = {
                static_cast<GLuint>(instances_total) * 6, // CountPin
                1, // Number of instances to draw,
                static_cast<GLuint>(instance_offset) * 6, // First vertex
                0 // Instance offset
            };
    
            render_information[i].draw_call_buffer.push(draw_call);
        }
    }

    if(legacy_mode) legacy_calls.push_back(call);
}

void PooledMeshLoader::removeMesh(LoadedMesh& mesh){
    for(size_t i = 0;i < distinct_face_count;i++){
        render_information[i].mesh_data.remove(mesh.loaded_regions[i]);
    }
}

void PooledMeshLoader::render(){
    GL_CALL( glEnable(GL_CULL_FACE));

    dummy_vao.bind();
    GetProgram().use();
    
    if(updated_world_positions && !legacy_mode){
        world_position_buffer.insert_or_resize(reinterpret_cast<int*>(world_positions.data()), world_positions.size());
        updated_world_positions = false;
    }

    if(!legacy_mode) world_position_buffer.bindBase(0);

    for(size_t i = 0;i < distinct_face_count;i++){
        auto& info = render_information[i];

        info.mesh_data.getBuffer().bindBase(1);
        info.draw_call_buffer.bind();
        face_type_uniform = i;
        GetProgram().updateUniforms();

        if(legacy_mode){
            for(auto& call: legacy_calls){
                world_position = call.world_position;
                GetProgram().updateUniforms();

                if(call.counts[i] == 0) continue;   
                glDrawArrays(GL_TRIANGLES, call.starts[i], call.counts[i]);
            }
        } //glMultiDrawArrays(GL_TRIANGLES, info.draw_starts.data(), info.draw_sizes.data(), info.draw_starts.size());
        else{
            glMultiDrawArraysIndirect(GL_TRIANGLES, 0, info.draw_call_buffer.count(), 0);
        } //glMultiDrawArraysIndirect(GL_TRIANGLES, 0, info.draw_call_buffer.count(), 0);
    }

    dummy_vao.unbind();

    GL_CALL( glDisable(GL_CULL_FACE));
}

void PooledMeshLoader::clearDrawCalls(){
    world_positions.clear();

    if(legacy_mode) legacy_calls.clear();

    for(auto& info: render_information){
        if(legacy_mode){
            info.draw_starts.clear();
            info.draw_sizes.clear();
        }
        else info.draw_call_buffer.clear();
    }

    updated_world_positions = true;
}

void PooledMeshLoader::flushDrawCalls(){
    if(legacy_mode) return;

    for(auto& info: render_information){
        //std::cout << "Flushed draw calls: " << info.draw_call_buffer.count() << std::endl;
        info.draw_call_buffer.flush();
    }
}
