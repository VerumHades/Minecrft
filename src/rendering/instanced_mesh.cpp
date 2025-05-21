#include <rendering/instanced_mesh.hpp>

static MultilevelPool<float> mesh_pool{};

InstancedMesh::InstancedMesh(): instance_data(){
    for(auto& element: instance_data)
        element = std::make_unique<MultilevelPool<float>::List>(mesh_pool.Next(64));
    
}
void InstancedMesh::addQuadFace(const glm::vec3& position, float width, float height, int texture_index, FaceType type, Direction direction, const std::array<float, 4>& occlusion){
    std::array<float, InstancedMesh::instance_data_size> data = {
        position.x,
        position.y,
        position.z,

        width,
        height,

        static_cast<float>(type),
        static_cast<float>(direction),
        static_cast<float>(texture_index),

        occlusion[0],
        occlusion[1],
        occlusion[3], // Swapped because of how occlusion is calculated, this changes clockwise order from the top left into one for GL_TRIGNALE_STRIP
        occlusion[2]
    };

    auto& instance_data_list = instance_data.at(type);
    for(auto& value: data)
        instance_data_list->Push(value);
}

void InstancedMesh::preallocate(size_t size, FaceType type){
    if(size != 0 && size > instance_data.at(type)->Size()) instance_data.at(type)->Resize((size + 1) * instance_data_size);
}
const MultilevelPool<float>::List& InstancedMesh::getInstanceData(FaceType type){
    return *instance_data[type];
}

bool InstancedMesh::empty(){
    for(int i = 0;i < 4;i++){
        if(instance_data.size() == 0) continue;
        return false;
    }
    return true;
}

void InstancedMesh::shrink(){
    //for(int i = 0;i < 4;i++) instance_data[i].shrink_to_fit();
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
    for(size_t i = 0;i < distinct_face_count;i++){
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

    for(size_t i = 0;i < distinct_face_count;i++){
        auto& component_data = mesh.getInstanceData(static_cast<InstancedMesh::FaceType>(i));
        if(component_data.Size() == 0){
            loaded_mesh->has_region[i] = false;
            continue;
        }

        loaded_mesh->loaded_regions[i] = render_information[i].instance_data.append(component_data.Data(), component_data.Size());
        loaded_mesh->has_region[i] = true;

        render_information[i].instance_data.flush();

        updated = true;
    }

    return loaded_mesh;
}

void InstancedMeshLoader::addDrawCall(LoadedMesh& mesh){
    for(size_t i = 0;i < distinct_face_count;i++){
        if(!mesh.has_region[i]) continue;
        
        size_t instances_total = mesh.loaded_regions[i]->size  / InstancedMesh::instance_data_size;
        size_t instance_offset = mesh.loaded_regions[i]->start / InstancedMesh::instance_data_size;

        GLDrawCallBuffer::DrawCommand draw_call = {
            4, // Count
            static_cast<GLuint>(instances_total), // Number of instances to draw,
            4 * static_cast<GLuint>(i), // First vertex
            static_cast<GLuint>(instance_offset) // Instance offset
        };

        render_information[i].draw_call_buffer.push(draw_call);
        if(i == 3){ // Draw the seconds diagonal
            draw_call.first += 4;
            render_information[i].draw_call_buffer.push(draw_call);
        }
    }
}

void InstancedMeshLoader::updateMesh(LoadedMesh& loaded_mesh, InstancedMesh& new_mesh){
    for(size_t i = 0;i < distinct_face_count;i++){
        auto& component_data = new_mesh.getInstanceData(static_cast<InstancedMesh::FaceType>(i));
        if(component_data.Size() == 0){
            loaded_mesh.has_region[i] = false;
            continue;
        }
    
        if(loaded_mesh.has_region[i])
            loaded_mesh.loaded_regions[i] = render_information[i].instance_data.update(loaded_mesh.loaded_regions[i], component_data.Data(), component_data.Size());
        else
            loaded_mesh.loaded_regions[i] = render_information[i].instance_data.append(component_data.Data(), component_data.Size());

        loaded_mesh.has_region[i] = true;

        updated = true;
    }
}

void InstancedMeshLoader::removeMesh(LoadedMesh& mesh){
    for(size_t i = 0;i < distinct_face_count;i++){
        if(!mesh.has_region[i]) continue;

        render_information[i].instance_data.remove(mesh.loaded_regions[i]);

        updated = true;
    }
}

void InstancedMeshLoader::render(){
    if(max_draw_calls == 0) return;
    
    shared_program.updateUniforms();

    for(auto& info: render_information){
        info.vao.bind();
        info.draw_call_buffer.bind();

        while(true){
            for(size_t i = 0; i < info.draw_call_buffer.count();i += max_draw_calls) 
                glMultiDrawArraysIndirect(GL_TRIANGLE_STRIP, (const void*)(i * sizeof(GLDrawCallBuffer::DrawCommand)), std::min(static_cast<uint>(info.draw_call_buffer.count() - i), max_draw_calls), sizeof(GLDrawCallBuffer::DrawCommand));

            GLenum error = glGetError();
            if(error == GL_NO_ERROR) break;

            max_draw_calls /= 2;
            if(max_draw_calls <= 0) break;
        }
        
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
        if(updated) info.instance_data.flush();
    }
}
