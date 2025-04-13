#include <rendering/model.hpp>

Model::Model() {
    getModelSet().emplace(this);
    for (auto& buffer : instance_buffers)
        buffer.initialize(1);
}
Model::~Model() {
    getModelSet().erase(this);
}

void Model::DrawAll() {
    for (auto& model : getModelSet())
        model->drawAllRequests();
}
void Model::SwapAll() {
    for (auto& model : getModelSet())
        model->swap();
}
void Model::CleanupAll() {
    for (auto& model : getModelSet()) {
        model->loaded_meshes.clear();
        for (auto& buffer : model->instance_buffers)
            buffer.cleanup();
    }
}

void Model::addMesh(Mesh& mesh) {
    auto loaded_mesh = mesh.load();

    for (int i = 0; i < loaded_mesh->getVAOs().size(); i++) {
        auto& vao = loaded_mesh->getVAOs()[i];
        vao.attachBuffer(&instance_buffers[i], {{VEC3, VEC3, VEC4, VEC3}, true}, 0);
        vao.attachBuffer(&instance_buffers[(i + 2) % 3], {{VEC3, VEC3, VEC4, VEC3}, true}, 1);
    }

    loaded_meshes.push_back(std::move(loaded_mesh));
}

Mesh Model::createMesh() {
    return Mesh({VEC3, VEC3, VEC2, FLOAT, VEC3, FLOAT});
}

void Model::requestDraw(const glm::vec3& position, const glm::vec3& scale, const glm::quat& rotation,
                        const glm::vec3& rotation_center_offset) {
    /*glm::mat4 modelMatrix = glm::translate(glm::mat4(1.0f), position);
    modelMatrix = glm::scale(modelMatrix, scale);
    modelMatrix *= glm::mat4_cast(rotation);
    modelMatrix = glm::translate(modelMatrix, rotation_center_offset); // if needed

    //modelMatrix = glm::scale(modelMatrix, scale);*/

    auto& request_back_buffer = request_buffers[backIndex()];

    request_back_buffer.insert(request_back_buffer.end(), glm::value_ptr(position), glm::value_ptr(position) + 3);
    request_back_buffer.insert(request_back_buffer.end(), glm::value_ptr(scale), glm::value_ptr(scale) + 3);
    request_back_buffer.insert(request_back_buffer.end(), glm::value_ptr(rotation), glm::value_ptr(rotation) + 4);
    request_back_buffer.insert(request_back_buffer.end(), glm::value_ptr(rotation_center_offset),
                               glm::value_ptr(rotation_center_offset) + 3);
}

void Model::drawAllRequests() {
    std::lock_guard<std::mutex> lock(swap_mutex);

    auto& request_buffer = request_buffers[selected];

    if (request_buffer.size() == 0)
        return;

    if (upload_data) {
        auto& front_buffer = getFrontInstanceBuffer();

        if (front_buffer.size() < request_buffer.size())
            front_buffer.initialize(request_buffer.size());
        front_buffer.insert(0, request_buffer.size(), request_buffer.data());

        upload_data = false;
    }

    for (auto& mesh : loaded_meshes) {
        if (mesh->getTexture())
            mesh->getTexture()->bind(0);

        mesh->getVAOs()[selected].bind();
        GL_CALL(glDrawElementsInstanced(GL_TRIANGLES, mesh->indicesTotal(), GL_UNSIGNED_INT, 0,
                                        request_buffer.size() / request_size));
        mesh->getVAOs()[selected].unbind();
    }
}
