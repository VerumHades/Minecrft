#include <rendering/model.hpp>

Model::Model(){
    for(int i = 0;i < vaos.size();i++){
        auto& vao = vaos[i];
        auto& instance_buffer = instance_buffers[i];

        vao.attachBuffer(&instance_buffer, {{VEC4,VEC4,VEC4,VEC4}, true});
        vao.attachBuffer(&index_buffer);
        vao.attachBuffer(&vertex_buffer, {VEC3,VEC3,VEC2,FLOAT,VEC3,FLOAT});

        instance_buffer.initialize(1);
    }
}

void Model::requestDraw(glm::vec3 position, glm::vec3 scale, glm::vec3 rotation, glm::vec3 rotation_center_offset, std::array<Rotation,3> rotation_order){
    std::array<glm::mat4,3> rotation_matrices = {
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
        
    auto& request_back_buffer = request_buffers[!selected];

    auto* data = glm::value_ptr(modelMatrix);
    request_back_buffer.insert(request_back_buffer.end(), data, data + 16);
}

void Model::drawAllRequests(){
    auto& request_buffer = request_buffers[selected];

    if(request_buffer.size() == 0) return;
    if(texture) texture->bind(0);

    if(upload_data){
        auto& front_buffer = getFrontInstanceBuffer();
        
        if(front_buffer.size() < request_buffer.size()) front_buffer.initialize(request_buffer.size());
        front_buffer.insert(0, request_buffer.size(), request_buffer.data());

        upload_data = false;
    }

    vaos[selected].bind();
    glDrawElementsInstanced(GL_TRIANGLES, index_buffer.size(), GL_UNSIGNED_INT, 0, request_buffer.size() / request_size);
    vaos[selected].unbind();
}