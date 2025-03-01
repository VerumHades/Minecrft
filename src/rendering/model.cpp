#include <rendering/model.hpp>

Model::Model(){
    getModelSet().emplace(this);
    for(auto& buffer: instance_buffers) buffer.initialize(1);
}
Model::~Model(){
    getModelSet().erase(this);
}

void Model::DrawAll(){
    for(auto& model: getModelSet())
        model->drawAllRequests();
}
void Model::SwapAll(){
    for(auto& model: getModelSet())
        model->swap();
}
void Model::CleanupAll(){
    for(auto& model: getModelSet()){
        model->loaded_meshes.clear();
        for(auto& buffer: model->instance_buffers) buffer.cleanup();
    }
}

void Model::addMesh(Mesh& mesh){
    auto loaded_mesh = mesh.load();

    for(int i = 0;i < loaded_mesh->getVAO().size();i++){
        auto& vao = loaded_mesh->getVAO()[i];
        vao.attachBuffer(&instance_buffers[i], {{VEC4,VEC4,VEC4,VEC4}, true}, 0);
        vao.attachBuffer(&instance_buffers[(i + 2) % 3], {{VEC4,VEC4,VEC4,VEC4}, true}, 1);
    }

    loaded_meshes.push_back(std::move(loaded_mesh));
}

Mesh Model::createMesh(){
    return Mesh({VEC3,VEC3,VEC2,FLOAT,VEC3,FLOAT});
}

void Model::requestDraw(
    const glm::vec3& position,
    const glm::vec3& scale,
    const glm::vec3& rotation,
    const glm::vec3& rotation_center_offset,
    const std::array<Rotation,3>& rotation_order
){
    const std::array<glm::mat4,3> rotation_matrices = {
        glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0)),  // Rotation around X axis
        glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0)),  // Rotation around Y axis
        glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1))  // Rotation around Z axis
    };

    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position);
    modelMatrix = glm::scale(modelMatrix, scale);
    //modelMatrix = glm::translate(modelMatrix, -rotation_center_offset);
    modelMatrix *= rotation_matrices[rotation_order[0]] * rotation_matrices[rotation_order[1]] * rotation_matrices[rotation_order[2]];
    modelMatrix = glm::translate(modelMatrix, rotation_center_offset);

    //modelMatrix = glm::scale(modelMatrix, scale);
        
    auto& request_back_buffer = request_buffers[backIndex()];

    auto* data = glm::value_ptr(modelMatrix);
    request_back_buffer.insert(request_back_buffer.end(), data, data + 16);
}

void Model::drawAllRequests(){
    std::lock_guard<std::mutex> lock(swap_mutex);

    auto& request_buffer = request_buffers[selected];

    if(request_buffer.size() == 0) return;

    if(upload_data){
        auto& front_buffer = getFrontInstanceBuffer();
        
        if(front_buffer.size() < request_buffer.size()) front_buffer.initialize(request_buffer.size());
        front_buffer.insert(0, request_buffer.size(), request_buffer.data());

        upload_data = false;
    }

    for(auto& mesh: loaded_meshes){
        if(mesh->getTexture()) mesh->getTexture()->bind(0);
        
        mesh->getVAO()[selected].bind();
        glDrawElementsInstanced(GL_TRIANGLES, mesh->indicesTotal(), GL_UNSIGNED_INT, 0, request_buffer.size() / request_size);
        mesh->getVAO()[selected].unbind();
    }
}