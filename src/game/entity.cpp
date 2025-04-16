#include <game/entity.hpp>
#include <iostream>

std::shared_ptr<EntityData> Entity::deserializeData(ByteArray& array) {
    auto type_opt = array.Read<EntityData::Type>();
    if (!type_opt)
        return nullptr;
    auto type = type_opt.value();
    if (type == EntityData::DROPPED_ITEM)
        return DroppedItem::deserializeData(array);
    return nullptr;
}

Entity::Entity(glm::vec3 position, glm::vec3 colliderDimensions) : position(position) {
    collider = {0, 0, 0, colliderDimensions.x, colliderDimensions.y, colliderDimensions.z};
}
Entity::Entity(glm::vec3 position, glm::vec3 colliderDimensions, std::shared_ptr<EntityData> data)
    : position(position) {
    setData(data);
    collider = {0, 0, 0, colliderDimensions.x, colliderDimensions.y, colliderDimensions.z};
}

void Entity::setModel(std::shared_ptr<Model> model) {
    this->model = model;
    if (model)
        model_instance = model->NewInstance();
}
void Entity::setPosition(const glm::vec3& position) {
    this->position = position;
    if (model_instance)
        model_instance->MoveTo(position);
}

void Entity::accelerate(glm::vec3 direction, float deltatime) {
    auto newVelocity = velocity + direction * deltatime;

    glm::vec3 horizontalVelocity = glm::vec3(newVelocity.x, 0, newVelocity.z);
    glm::vec3 verticalVelocity   = glm::vec3(0, newVelocity.y, 0);

    if (glm::length(horizontalVelocity) > maxVelocityHorizontal)
        horizontalVelocity = glm::normalize(horizontalVelocity) * maxVelocityHorizontal;
    if (glm::length(verticalVelocity) > maxVelocityVertical)
        verticalVelocity = glm::normalize(verticalVelocity) * maxVelocityVertical;

    velocity = horizontalVelocity + verticalVelocity;
}

void Entity::decellerate(float strength, float deltatime) {
    glm::vec3 horizontalVelocity = glm::vec3(velocity.x, 0, velocity.z);
    if (glm::length(horizontalVelocity) >= 0)
        accelerate(-horizontalVelocity * glm::min(strength, 1.0f), deltatime);
}
