#include <rendering/instanced_mesh.hpp>

void InstancedMesh::addQuadFace(glm::vec3 position, float width, float height, int texture_index, FaceType type, Direction direction){
    std::array<float, InstancedMesh::instance_data_size> data = {
        position.x,
        position.y,
        position.z,

        width,
        height,

        direction,
        texture_index
    };

    auto& instance_data_list =  instance_data[type];
    instance_data_list.insert(instance_data_list.end(), data.begin(), data.end());
}

const std::vector<float>& InstancedMesh::getInstanceData(FaceType type){
    return  instance_data[type];
}

InstancedMeshBuffer::InstancedMeshBuffer(){
    std::array<float, 20 * 5> aligned_quad_data = {
        // X aligned face
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 
        0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 
        0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 
        0.0f, 1.0f, 1.0f,  1.0f, 0.0f, 
        // Y aligned face
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 
        0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 
        1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 
        1.0f, 0.0f, 1.0f,  1.0f, 0.0f, 
        // Z aligned face
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 
        1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 
        0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 
        1.0f, 1.0f, 0.0f,  1.0f, 0.0f, 

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


    loaded_face_buffer.initialize(aligned_quad_data.size(), aligned_quad_data.data());

    for(int i = 0;i < distinct_face_count;i++){
        vaos[i].attachBuffer(&instance_buffers[i], {VEC3, VEC2, FLOAT, FLOAT});
        vaos[i].attachBuffer(&loaded_face_buffer, {VEC3, VEC2});
    }
}

InstancedMeshBuffer::LoadedMesh::~LoadedMesh(){
    creator.removeMesh(*this);
}

void InstancedMeshBuffer::LoadedMesh::update(InstancedMesh& mesh){
    creator.updateMesh(*this, mesh);
}

void InstancedMeshBuffer::LoadedMesh::render(){
    creator.renderMesh(*this);
}

void InstancedMeshBuffer::renderMesh(LoadedMesh& mesh){
    for(int i = 0;i < distinct_face_count;i++){
        vaos[i].bind();

        glDrawArraysInstancedBaseInstance(
            GL_TRIANGLE_STRIP, 
            4 * i, // Offset in the buffer
            4, 
            mesh.loaded_regions[i]->size  / InstancedMesh::instance_data_size,
            mesh.loaded_regions[i]->start / InstancedMesh::instance_data_size
        );

        if(i == 3){ // Draw the seconds diagonal
            glDrawArraysInstancedBaseInstance(
                GL_TRIANGLE_STRIP, 
                4 * (i + 1), // Offset in the buffer
                4, 
                mesh.loaded_regions[i]->size  / InstancedMesh::instance_data_size,
                mesh.loaded_regions[i]->start / InstancedMesh::instance_data_size
            );
        }
    }
    vaos[0].unbind();
}

InstancedMeshBuffer::LoadedMesh InstancedMeshBuffer::loadMesh(InstancedMesh& mesh){
    LoadedMesh loadedMesh = {*this};

    for(int i = 0;i < distinct_face_count;i++){
        auto& component_data = mesh.getInstanceData(static_cast<InstancedMesh::FaceType>(i));
        if(component_data.size() == 0) continue;

        loadedMesh.loaded_regions[i] = instance_data[i].append(component_data.data(), component_data.size());
    }

    return loadedMesh;
}

void InstancedMeshBuffer::updateMesh(LoadedMesh& loaded_mesh, InstancedMesh& new_mesh){
    for(int i = 0;i < distinct_face_count;i++){
        auto& component_data = new_mesh.getInstanceData(static_cast<InstancedMesh::FaceType>(i));
        if(component_data.size() == 0) continue;

        loaded_mesh.loaded_regions[i] = instance_data[i].update(loaded_mesh.loaded_regions[i], component_data.data(), component_data.size());
    }
}

void InstancedMeshBuffer::removeMesh(LoadedMesh& mesh){
    for(int i = 0;i < distinct_face_count;i++){
        instance_data[i].remove(mesh.loaded_regions[i]);
    }
}