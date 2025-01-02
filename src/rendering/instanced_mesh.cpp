#include <rendering/instanced_mesh.hpp>

void InstancedMesh::addQuadFace(glm::vec3 position, float width, float height, int texture_index, FaceType type, Direction direction, const std::array<float, 4>& occlusion){
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

    auto& instance_data_list =  instance_data[type];
    instance_data_list.insert(instance_data_list.end(), data.begin(), data.end());
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

InstancedMeshBuffer::InstancedMeshBuffer(){
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
        0.0f,  0.0f, 0.0f,  0.0f, 1.0f, 
        1.0f,  0.0f, 1.0f,  1.0f, 1.0f, 
        0.0f, -1.0f, 0.0f,  0.0f, 0.0f, 
        1.0f, -1.0f, 1.0f,  1.0f, 0.0f, 

        0.0f,  0.0f, 1.0f,  0.0f, 1.0f, 
        1.0f,  0.0f, 0.0f,  1.0f, 1.0f, 
        0.0f, -1.0f, 1.0f,  0.0f, 0.0f, 
        1.0f, -1.0f, 0.0f,  1.0f, 0.0f, 
    };


    loaded_face_buffer.initialize(aligned_quad_data.size(), aligned_quad_data.data());

    for(int i = 0;i < distinct_face_count;i++){
        vaos[i].attachBuffer(&instance_data[i].getBuffer(), {{VEC3, VEC2, FLOAT, FLOAT, FLOAT, VEC4}, true});
        vaos[i].attachBuffer(&loaded_face_buffer, {VEC3, VEC2});
    }
}

void InstancedMeshBuffer::LoadedMesh::destroy(){
    if(!valid) throw std::logic_error("Cannot destroy destroyed mesh.");
    creator.removeMesh(*this);
    valid = false;
}

void InstancedMeshBuffer::LoadedMesh::update(InstancedMesh& mesh){
    if(!valid) throw std::logic_error("Cannot update destroyed mesh.");
    creator.updateMesh(*this, mesh);
}

void InstancedMeshBuffer::LoadedMesh::render(){
    if(!valid) throw std::logic_error("Cannot render destroyed mesh.");
    creator.renderMesh(*this);
}

void InstancedMeshBuffer::LoadedMesh::addDrawCall(){
    if(!valid) throw std::logic_error("Cannot add draw call of destroyed mesh.");
    creator.addDrawCall(*this);
}
void InstancedMeshBuffer::renderMesh(LoadedMesh& mesh){
    for(int i = 0;i < distinct_face_count;i++){
        vaos[i].bind();

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
    }
    vaos[0].unbind();
}

InstancedMeshBuffer::LoadedMesh InstancedMeshBuffer::loadMesh(InstancedMesh& mesh){
    LoadedMesh loaded_mesh = {*this};

    for(int i = 0;i < distinct_face_count;i++){
        auto& component_data = mesh.getInstanceData(static_cast<InstancedMesh::FaceType>(i));
        if(component_data.size() == 0){
            loaded_mesh.has_region[i] = false;
            continue;
        }

        loaded_mesh.loaded_regions[i] = instance_data[i].append(component_data.data(), component_data.size());
        loaded_mesh.has_region[i] = true;

        instance_data[i].flush();
    }

    return loaded_mesh;
}

void InstancedMeshBuffer::addDrawCall(LoadedMesh& mesh){
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

        draw_call_buffers[i].push(draw_call);
        if(i == 3){ // Draw the seconds diagonal
            draw_call.first += 4;
            draw_call_buffers[i].push(draw_call);
        }
    }
}

void InstancedMeshBuffer::updateMesh(LoadedMesh& loaded_mesh, InstancedMesh& new_mesh){
    for(int i = 0;i < distinct_face_count;i++){
        auto& component_data = new_mesh.getInstanceData(static_cast<InstancedMesh::FaceType>(i));
        if(component_data.size() == 0){
            loaded_mesh.has_region[i] = false;
            continue;
        }
    
        if(loaded_mesh.has_region[i])
            loaded_mesh.loaded_regions[i] = instance_data[i].update(loaded_mesh.loaded_regions[i], component_data.data(), component_data.size());
        else
            loaded_mesh.loaded_regions[i] = instance_data[i].append(component_data.data(), component_data.size());

        loaded_mesh.has_region[i] = true;

        instance_data[i].flush();
    }
}

void InstancedMeshBuffer::removeMesh(LoadedMesh& mesh){
    for(int i = 0;i < distinct_face_count;i++){
        instance_data[i].remove(mesh.loaded_regions[i]);
    }
}

void InstancedMeshBuffer::render(){
    for(int i = 0;i < distinct_face_count;i++){
        vaos[i].bind();
        draw_call_buffers[i].bind();
        glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, 0, draw_call_buffers[i].count(), sizeof(GLDrawCallBuffer::DrawCommand));
    }
    vaos[0].unbind();
}

void InstancedMeshBuffer::clearDrawCalls(){
    for(int i = 0;i < distinct_face_count;i++) draw_call_buffers[i].clear();
}

void InstancedMeshBuffer::flushDrawCalls(){
    for(int i = 0;i < distinct_face_count;i++) draw_call_buffers[i].flush();
}

void InstancedMesh::shrink(){
    for(int i = 0;i < 4;i++) instance_data[i].shrink_to_fit();
}