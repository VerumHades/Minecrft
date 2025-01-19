#include <game/entity.hpp>
#include <iostream>
Entity::Entity(glm::vec3 position, glm::vec3 colliderDimensions): position(position) {
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
