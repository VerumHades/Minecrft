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

bool Entity::checkForCollision(Collidable* collidable, bool withVelocity, glm::vec3 offset, bool vertical_check){
    glm::vec3 pos = this->position + offset;
    if(withVelocity) pos += this->velocity;
    return collidable->collidesWith(pos, this, vertical_check);
}

void Entity::update(Collidable* world, bool altered, float deltatime){
    if(altered) on_ground = false;
    
    if(hasGravity) this->accelerate(glm::vec3(0,-(0.98 + friction * 2),0), deltatime);
    
    if(this->velocity.x != 0 || this->velocity.y != 0 || this->velocity.z != 0) on_ground = false;

    glm::vec3& vel = this->velocity;


    float relative_friction = friction * deltatime;

    float len = glm::length(vel);
    if(len > relative_friction) vel = glm::normalize(vel) * (len - relative_friction);
    else vel = glm::vec3(0);

    if(vel.x != 0 && checkForCollision(world, false, {vel.x * deltatime, 0, 0}      )) vel.x = 0;
    if(vel.y != 0 && checkForCollision(world, false, {0, vel.y * deltatime, 0}, true)) vel.y = 0;
    if(vel.z != 0 && checkForCollision(world, false, {0, 0, vel.z * deltatime}      )) vel.z = 0;

    this->position += vel * deltatime;
}


