#include <rendering/instanced_mesh.hpp>

void InstancedMesh::addQuadFace(const glm::vec3& position, float width, float height, int texture_index, FaceType type, Direction direction, const std::array<float, 4>& occlusion){
    std::array<float, InstancedMesh::instance_data_size> data = {
        position.x,
        position.y,
        position.z,

        width,
        height,

        type,
        direction,
        texture_index,

        occlusion[0],
        occlusion[1],
        occlusion[3], // Swapped because of how occlusion is calculated, this changes clockwise order from the top left into one for GL_TRIGNALE_STRIP
        occlusion[2]
    };

    auto& instance_data_list = instance_data.at(type);
    instance_data_list.insert(instance_data_list.end(), data.begin(), data.end());
}

void InstancedMesh::preallocate(size_t size, FaceType type){
    instance_data.at(type).reserve(size * instance_data_size);
}
const std::vector<float>& InstancedMesh::getInstanceData(FaceType type){
    return  instance_data[type];
}

bool InstancedMesh::empty(){
    for(int i = 0;i < 4;i++){
        if(instance_data.size() == 0) continue;
        return false;
    }
    return true;
}

void InstancedMesh::shrink(){
    for(int i = 0;i < 4;i++) instance_data[i].shrink_to_fit();
}

InstancedMeshLoader::InstancedMeshLoader(){
    std::array<float, 20 * 5> aligned_quad_data = {
        // X aligned face
        0.0f, -1.0f, 0.0f,  0.0f, 1.0f, 
        0.0f, -1.0f, 1.0f,  1.0f, 1.0f, 
        0.0f,  0.0f, 0.0f,  0.0f, 0.0f, 
        0.0f,  0.0f, 1.0f,  1.0f, 0.0f, 
        // Y aligned face
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 
        0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 
        1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 
        1.0f, 0.0f, 1.0f,  1.0f, 0.0f, 
        // Z aligned face
        0.0f, -1.0f, 0.0f,  0.0f, 1.0f, 
        1.0f, -1.0f, 0.0f,  1.0f, 1.0f, 
        0.0f,  0.0f, 0.0f,  0.0f, 0.0f, 
        1.0f,  0.0f, 0.0f,  1.0f, 0.0f, 

        // Diagonal billboard faces
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 
        1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 
        0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 
        1.0f, 1.0f, 1.0f,  1.0f, 0.0f, 

        0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 
        1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 
        0.0f, 1.0f, 1.0f,  0.0f, 0.0f, 
        1.0f, 1.0f, 0.0f,  1.0f, 0.0f, 
    };

    shared_program.setSamplerSlot("textureArray", 0);
    shared_program.setSamplerSlot("shadowMap", 1);

    loaded_face_buffer.initialize(aligned_quad_data.size(), aligned_quad_data.data());

    for(auto& info: render_information){
        info.vao.attachBuffer(&info.instance_data.getBuffer(), {{VEC3, VEC2, FLOAT, FLOAT, FLOAT, VEC4}, true});
        info.vao.attachBuffer(&loaded_face_buffer, {VEC3, VEC2});
    }
}

void InstancedMeshLoader::LoadedMesh::destroy(){
    if(!valid) throw std::logic_error("Cannot destroy destroyed mesh.");
    creator.removeMesh(*this);
    valid = false;
}

void InstancedMeshLoader::LoadedMesh::update(MeshInterface* mesh_){
    if(!valid) throw std::logic_error("Cannot update destroyed mesh.");
    
    auto mesh_ptr = dynamic_cast<InstancedMesh*>(mesh_);
    if(!mesh_ptr) return;

    auto& mesh = *mesh_ptr;

    creator.updateMesh(*this, mesh);
}

void InstancedMeshLoader::LoadedMesh::render(){
    if(!valid) throw std::logic_error("Cannot render destroyed mesh.");
    creator.renderMesh(*this);
}

void InstancedMeshLoader::LoadedMesh::addDrawCall(const glm::ivec3& position){
    if(!valid) throw std::logic_error("Cannot add draw call of destroyed mesh.");
    creator.addDrawCall(*this);
}
void InstancedMeshLoader::renderMesh(LoadedMesh& mesh){
    for(int i = 0;i < distinct_face_count;i++){
        render_information[i].vao.bind();

        size_t instances_total = mesh.loaded_regions[i]->size  / InstancedMesh::instance_data_size;
        size_t instance_offset = mesh.loaded_regions[i]->start / InstancedMesh::instance_data_size;

        glDrawArraysInstancedBaseInstance(
            GL_TRIANGLE_STRIP, 
            4 * i, // Offset in the buffer
            4, 
            instances_total,
            instance_offset
        );

        if(i == 3){ // Draw the seconds diagonal
            glDrawArraysInstancedBaseInstance(
                GL_TRIANGLE_STRIP, 
                4 * (i + 1), // Offset in the buffer
                4, 
                instances_total,
                instance_offset
            );
        }
        render_information[i].vao.unbind();
    }
}

std::unique_ptr<LoadedMeshInterface> InstancedMeshLoader::loadMesh(MeshInterface* mesh_){
    auto mesh_ptr = dynamic_cast<InstancedMesh*>(mesh_);
    if(!mesh_ptr) return nullptr;

    auto& mesh = *mesh_ptr;
    auto loaded_mesh = std::make_unique<InstancedMeshLoader::LoadedMesh>(*this);

    for(int i = 0;i < distinct_face_count;i++){
        auto& component_data = mesh.getInstanceData(static_cast<InstancedMesh::FaceType>(i));
        if(component_data.size() == 0){
            loaded_mesh->has_region[i] = false;
            continue;
        }

        loaded_mesh->loaded_regions[i] = render_information[i].instance_data.append(component_data.data(), component_data.size());
        loaded_mesh->has_region[i] = true;

        render_information[i].instance_data.flush();
    }

    return loaded_mesh;
}

void InstancedMeshLoader::addDrawCall(LoadedMesh& mesh){
    for(int i = 0;i < distinct_face_count;i++){
        if(!mesh.has_region[i]) continue;
        
        size_t instances_total = mesh.loaded_regions[i]->size  / InstancedMesh::instance_data_size;
        size_t instance_offset = mesh.loaded_regions[i]->start / InstancedMesh::instance_data_size;

        GLDrawCallBuffer::DrawCommand draw_call = {
            4, // Count
            instances_total, // Number of instances to draw,
            4 * i, // First vertex
            instance_offset // Instance offset
        };

        render_information[i].draw_call_buffer.push(draw_call);
        if(i == 3){ // Draw the seconds diagonal
            draw_call.first += 4;
            render_information[i].draw_call_buffer.push(draw_call);
        }
    }
}

void InstancedMeshLoader::updateMesh(LoadedMesh& loaded_mesh, InstancedMesh& new_mesh){
    for(int i = 0;i < distinct_face_count;i++){
        auto& component_data = new_mesh.getInstanceData(static_cast<InstancedMesh::FaceType>(i));
        if(component_data.size() == 0){
            loaded_mesh.has_region[i] = false;
            continue;
        }
    
        if(loaded_mesh.has_region[i])
            loaded_mesh.loaded_regions[i] = render_information[i].instance_data.update(loaded_mesh.loaded_regions[i], component_data.data(), component_data.size());
        else
            loaded_mesh.loaded_regions[i] = render_information[i].instance_data.append(component_data.data(), component_data.size());

        loaded_mesh.has_region[i] = true;

        render_information[i].instance_data.flush();
    }
}

void InstancedMeshLoader::removeMesh(LoadedMesh& mesh){
    for(int i = 0;i < distinct_face_count;i++){
        render_information[i].instance_data.remove(mesh.loaded_regions[i]);
    }
}

void InstancedMeshLoader::render(){
    shared_program.updateUniforms();

    for(auto& info: render_information){
        info.vao.bind();
        info.draw_call_buffer.bind();
        GL_CALL( glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, 0, info.draw_call_buffer.count(), sizeof(GLDrawCallBuffer::DrawCommand)));
        info.vao.unbind();
    }
}

void InstancedMeshLoader::clearDrawCalls(){
    for(auto& info: render_information) info.draw_call_buffer.clear();
}

void InstancedMeshLoader::flushDrawCalls(){
    for(auto& info: render_information){
        //std::cout << "Flushed draw calls: " << info.draw_call_buffer.count() << std::endl;
        info.draw_call_buffer.flush();
    }
}
