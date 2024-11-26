#include <rendering/model.hpp>

Model::Model(){
    vao.attachBuffer(&instance_buffer, {VEC4,VEC4,VEC4,VEC4}, true);
    vao.attachIndexBuffer(&index_buffer);
}

void Model::setupBufferFormat(std::vector<GLSlotBinding> bindings){
    vao.attachBuffer(&vertex_buffer, bindings);
}

void Model::requestDraw(glm::vec3 position, glm::vec3 rotation){
    if(last_request >= draw_request_data.size()){
        draw_request_data.resize(draw_request_data.size() + request_size);
    }

    glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0));  // Rotation around X axis
    glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0));  // Rotation around Y axis
    glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1));  // Rotation around Z axis

    // Combine the rotations in the correct order (commonly Z * Y * X)
    glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position);
    modelMatrix *= rotationZ * rotationY * rotationX;

    std::memcpy(draw_request_data.data() + last_request, glm::value_ptr(modelMatrix), 4 * 4 * sizeof(float));

    last_request += request_size;
}
void Model::drawAllRequests(){
    if(last_request == 0) return;
    if(instance_buffer.size() < last_request) instance_buffer.initialize(last_request);
    
    instance_buffer.insert(0, last_request, draw_request_data.data());

    vao.bind();
    glDrawElementsInstanced(GL_TRIANGLES, index_buffer.size(), GL_UNSIGNED_INT, 0, last_request / request_size);

    last_request = 0;
}