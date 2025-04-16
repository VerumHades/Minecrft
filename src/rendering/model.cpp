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

std::shared_ptr<ModelInstance> Model::NewInstance() {
    std::lock_guard<std::mutex> lock(swap_mutex);

    size_t index = request_pool.NextIndex();

    auto deleter = [index, this](Instance* instance) {
        request_pool.Free(index);
        request_pool[index].scale = {0, 0, 0};
        delete instance;
        upload_data = true;
    };

    upload_data = true;

    auto instance = std::shared_ptr<Instance>(new Instance(*this, index), deleter);
    instance->Scale({1.0, 1.0, 1.0});
    instance->Rotate(glm::quat());
    instance->MoveRotationOffset({0.0, 0.0, 0.0});
    instance->MoveTo({0.0, 0.0, 0.0});

    return instance;
}

Model::Request& Model::GetRequest(size_t index) {
    upload_data = true;
    return request_pool[index];
}
void Model::Instance::MoveTo(const glm::vec3& position) {
    model.GetRequest(index).position = position;
}
void Model::Instance::Scale(const glm::vec3& scale) {
    model.GetRequest(index).scale = scale;
}
void Model::Instance::Rotate(const glm::quat& rotation) {
    model.GetRequest(index).rotation = glm::vec4(rotation.x, rotation.y, rotation.z, rotation.w);
}
void Model::Instance::MoveRotationOffset(const glm::vec3& rotation_center) {
    model.GetRequest(index).rotation_offset = rotation_center;
}
bool Model::Instance::IsOfModel(Model& model) {
    return &this->model == &model;
}

void Model::drawAllRequests() {
    if (request_pool.vector().size() == 0)
        return;

    if (upload_data) {
        selected           = (selected + 1) % 3;
        auto& front_buffer = getFrontInstanceBuffer();

        std::lock_guard<std::mutex> lock(swap_mutex);

        if (front_buffer.size() < request_pool.vector().size() * sizeof(Request))
            front_buffer.initialize(request_pool.vector().size() * sizeof(Request));
        front_buffer.insert(0, request_pool.vector().size() * sizeof(Request),
                            reinterpret_cast<float*>(request_pool.vector().data()));

        upload_data = false;
    }

    for (auto& mesh : loaded_meshes) {
        if (mesh->getTexture())
            mesh->getTexture()->bind(0);

        mesh->getVAOs()[selected].bind();
        GL_CALL(glDrawElementsInstanced(GL_TRIANGLES, mesh->indicesTotal(), GL_UNSIGNED_INT, 0,
                                        request_pool.vector().size()));
        mesh->getVAOs()[selected].unbind();
    }
}
