#include <game/entity.hpp>
#include <iostream>

std::shared_ptr<EntityData> Entity::deserializeData(ByteArray& array){
    EntityData::Type type = array.read<EntityData::Type>();
    if(type == EntityData::DROPPED_ITEM) return DroppedItem::deserializeData(array);
    return nullptr;
}

Entity::Entity(glm::vec3 position, glm::vec3 colliderDimensions): position(position) {
    collider = {0,0,0,colliderDimensions.x,colliderDimensions.y,colliderDimensions.z};
}
Entity::Entity(glm::vec3 position, glm::vec3 colliderDimensions, std::shared_ptr<EntityData> data): position(position){
    setData(data);
    collider = {0,0,0,colliderDimensions.x,colliderDimensions.y,colliderDimensions.z};
}


void Entity::accelerate(glm::vec3 direction, float deltatime){
    auto newVelocity = velocity + direction * deltatime;

    glm::vec3 horizontalVelocity = glm::vec3(newVelocity.x, 0, newVelocity.z);
    glm::vec3 verticalVelocity = glm::vec3(0,newVelocity.y,0);

    if(glm::length(horizontalVelocity) > maxVelocityHorizontal) horizontalVelocity = glm::normalize(horizontalVelocity) * maxVelocityHorizontal;
    if(glm::length(verticalVelocity) > maxVelocityVertical) verticalVelocity = glm::normalize(verticalVelocity) * maxVelocityVertical;

    velocity = horizontalVelocity + verticalVelocity;
}

void Entity::decellerate(float strength, float deltatime){
    glm::vec3 horizontalVelocity = glm::vec3(velocity.x, 0, velocity.z);
    if(glm::length(horizontalVelocity) >= 0) accelerate(-horizontalVelocity * glm::min(strength,1.0f), deltatime);
}

void Entity::serialize(ByteArray& array){
    array.append<glm::vec3>(position);
    array.append<glm::vec3>(velocity);
    array.append<glm::vec3>(lastPosition);
    collider.serialize(array);

    array.append<size_t>(tags.size());
    for(auto& tag: tags) array.append(tag);

    if(data){
        array.append(data->type);
        data->serialize(array);
    }
    else array.append(EntityData::Type::NONE);
}

Entity Entity::deserialize(ByteArray& array){
    Entity entity{{},glm::vec3(0.6, 1.8, 0.6)};

    entity.position = array.read<glm::vec3>();
    entity.velocity = array.read<glm::vec3>();
    entity.lastPosition = array.read<glm::vec3>();

    entity.collider = RectangularCollider::deserialize(array);
    
    size_t tag_count = array.read<size_t>();
    for(size_t i = 0;i < tag_count;i++) entity.tags.emplace(array.sread());

    entity.setData(deserializeData(array));

    return entity;
}