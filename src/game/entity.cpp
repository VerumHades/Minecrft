#include <game/entity.hpp>
#include <iostream>
Entity::Entity(glm::vec3 position, glm::vec3 colliderDimensions): position(position) {
    collider = {0,0,0,colliderDimensions.x,colliderDimensions.y,colliderDimensions.z};
}

void Entity::accelerate(glm::vec3 direction){
    auto newVelocity = velocity + direction;

    glm::vec3 horizontalVelocity = glm::vec3(newVelocity.x, 0, newVelocity.z);
    glm::vec3 verticalVelocity = glm::vec3(0,newVelocity.y,0);

    if(glm::length(horizontalVelocity) > maxVelocityHorizontal) horizontalVelocity = glm::normalize(horizontalVelocity) * maxVelocityHorizontal;
    if(glm::length(verticalVelocity) > maxVelocityVertical) verticalVelocity = glm::normalize(verticalVelocity) * maxVelocityVertical;

    velocity = horizontalVelocity + verticalVelocity;
}

bool Entity::checkForCollision(Collidable* collidable, bool withVelocity, glm::vec3 offset, bool vertical_check){
    glm::vec3 pos = this->position + offset;
    if(withVelocity) pos += this->velocity;
    return collidable->collidesWith(pos, this, vertical_check);
}

void Entity::update(Collidable* world, bool altered){
    if(altered) on_ground = false;
    
    if(hasGravity && !on_ground) this->accelerate(glm::vec3(0,-0.01,0));
    
    glm::vec3& vel = this->velocity;

    float len = glm::length(vel);
    if(len > friction) vel = glm::normalize(vel) * (len - friction);
    else vel = glm::vec3(0);

    if(vel.x != 0 && checkForCollision(world, false, {vel.x, 0, 0}      )) vel.x = 0;
    if(vel.y != 0 && checkForCollision(world, false, {0, vel.y, 0}, true)) vel.y = 0;
    if(vel.z != 0 && checkForCollision(world, false, {0, 0, vel.z}      )) vel.z = 0;

    this->position += vel;
}


