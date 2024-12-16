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
    std::array<float, 20> x_aligned_quad_data = {
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 
        0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 
        0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 
        0.0f, 1.0f, 1.0f,  1.0f, 0.0f, 
    };

    std::array<float, 20> y_aligned_quad_data = {
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 
        0.0f, 0.0f, 1.0f,  1.0f, 1.0f, 
        1.0f, 0.0f, 0.0f,  0.0f, 0.0f, 
        1.0f, 0.0f, 1.0f,  1.0f, 0.0f, 
    };

    std::array<float, 20> z_aligned_quad_data = {
        0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 
        1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 
        0.0f, 1.0f, 0.0f,  0.0f, 0.0f, 
        1.0f, 1.0f, 0.0f,  1.0f, 0.0f, 
    };

    loaded_face_buffers[InstancedMesh::X_ALIGNED].initialize(x_aligned_quad_data.size(), x_aligned_quad_data.data());
    loaded_face_buffers[InstancedMesh::Y_ALIGNED].initialize(y_aligned_quad_data.size(), y_aligned_quad_data.data());
    loaded_face_buffers[InstancedMesh::Z_ALIGNED].initialize(z_aligned_quad_data.size(), z_aligned_quad_data.data());

    for(int i = 0;i < distinct_face_count;i++){
        vaos[i].attachBuffer(&instance_buffers[i], {VEC3, VEC2, FLOAT, FLOAT});
        vaos[i].attachBuffer(&loaded_face_buffers[i], {VEC3, VEC2});
    }
}

void InstancedMeshBuffer::renderMesh(LoadedMeshIterator iterator){
    for(int i = 0;i < distinct_face_count;i++){
        vaos[i].bind();

        glDrawArraysInstancedBaseInstance(
            GL_TRIANGLE_STRIP, 
            0, 
            4, 
            iterator->loaded_regions[i]->size  / InstancedMesh::instance_data_size,
            iterator->loaded_regions[i]->start / InstancedMesh::instance_data_size
        );
    }
    vaos[0].unbind();
}

InstancedMeshBuffer::LoadedMeshIterator InstancedMeshBuffer::loadMesh(InstancedMesh& mesh){
    LoadedMesh loadedMesh = {};

    for(int i = 0;i < distinct_face_count;i++){
        auto& component_data = mesh.getInstanceData(static_cast<InstancedMesh::FaceType>(i));
        if(component_data.size() == 0) continue;

        loadedMesh.loaded_regions[i] = instance_data[i].append(component_data.data(), component_data.size());
    }

    return loadedMeshes.insert(loadedMeshes.end(), loadedMesh);
}

void InstancedMeshBuffer::removeMesh(InstancedMeshBuffer::LoadedMeshIterator iterator){
    for(int i = 0;i < distinct_face_count;i++){
        instance_data[i].remove(iterator->loaded_regions[i]);
    }
    loadedMeshes.erase(iterator);
}