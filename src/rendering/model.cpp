#include <rendering/model.hpp>

Model::Model(){
    vao.attachBuffer(&vertex_buffer, {VEC3});
    vao.attachBuffer(&instance_buffer, {VEC3}, true);
    vao.attachIndexBuffer(&index_buffer);
}

void Model::requestDraw(glm::vec3 position){
    if(last_request >= draw_request_data.size()){
        draw_request_data.resize(draw_request_data.size() + request_size);
    }

    draw_request_data[0] = position.x;
    draw_request_data[1] = position.y;
    draw_request_data[2] = position.z;   

    last_request += request_size;
}
void Model::drawAllRequests(){
    if(instance_buffer.size() < last_request) instance_buffer.initialize(last_request);
    
    instance_buffer.insert(0, last_request, draw_request_data.data());

    glDrawElementsInstanced(GL_TRIANGLES, index_buffer.size(), GL_UNSIGNED_INT, 0, last_request / request_size);
}