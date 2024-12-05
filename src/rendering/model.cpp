#include <rendering/model.hpp>

Model::Model(){
    for(int i = 0;i < vaos.size();i++){
        auto& vao = vaos[i];
        auto& instance_buffer = instance_buffers[i];

        vao.attachBuffer(&instance_buffer, {VEC4,VEC4,VEC4,VEC4}, true);
        vao.attachIndexBuffer(&index_buffer);
        vao.attachBuffer(&vertex_buffer, {VEC3,VEC3,VEC2,FLOAT,VEC3});

        instance_buffer.initialize(1);
    }
}

void Model::requestDraw(glm::vec3 position, glm::vec3 scale, glm::vec3 rotation, glm::vec3 rotation_center_offset){
    glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0));  // Rotation around X axis
    glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0));  // Rotation around Y axis
    glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1));  // Rotation around Z axis


    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position);
    modelMatrix = glm::scale(modelMatrix, scale);
    //modelMatrix = glm::translate(modelMatrix, -rotation_center_offset);
    modelMatrix *= rotationZ * rotationY * rotationX;
    modelMatrix = glm::translate(modelMatrix, rotation_center_offset);

    //modelMatrix = glm::scale(modelMatrix, scale);

    request_buffer.append(glm::value_ptr(modelMatrix), 4 * 4);
    pending_update = true;
}
void Model::updateRequestBuffer(){
    if(!upload_data) return;
    if(request_buffer.size() == 0) return;
    
    auto& back_buffer = getBackInstanceBuffer();

    if(back_buffer.size() < request_buffer.size()) back_buffer.initialize(request_buffer.size());
    request_buffer.upload(back_buffer);

    swapInstanceBuffers();

    upload_data = false;
}

void Model::drawAllRequests(){
    if(request_buffer.size() == 0) return;
    if(texture) texture->bind(0);

    vaos[selected].bind();
    glDrawElementsInstanced(GL_TRIANGLES, index_buffer.size(), GL_UNSIGNED_INT, 0, request_buffer.size() / request_size);
    vaos[selected].unbind();
}

void Model::resetRequests(){
    request_buffer.clear();
    pending_update = true;
}

void Model::passRequests(){
    if(!pending_update) return;
    request_buffer.pass();
    upload_data = true;
}