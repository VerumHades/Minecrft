#include <game/entity.hpp>
#include <iostream>
Entity::Entity(glm::vec3 position, glm::vec3 colliderDimensions): position(position) {
    colliders.push_back({0,0,0,colliderDimensions.x,colliderDimensions.y,colliderDimensions.z});
}

void Entity::accelerate(glm::vec3 direction){
    auto newVelocity = velocity + direction;

    glm::vec3 horizontalVelocity = glm::vec3(newVelocity.x, 0, newVelocity.z);
    glm::vec3 verticalVelocity = glm::vec3(0,newVelocity.y,0);

    if(glm::length(horizontalVelocity) > maxVelocityHorizontal) horizontalVelocity = glm::normalize(horizontalVelocity) * maxVelocityHorizontal;
    if(glm::length(verticalVelocity) > maxVelocityVertical) verticalVelocity = glm::normalize(verticalVelocity) * maxVelocityVertical;

    velocity = horizontalVelocity + verticalVelocity;
}

CollisionCheckResult Entity::checkForCollision(Collidable& collidable, bool withVelocity, glm::vec3 offset){
    glm::vec3 pos = this->position + offset;
    if(withVelocity) pos += this->velocity;
    CollisionCheckResult result = {nullptr, false, 0,0,0};

    size_t size = colliders.size();
    for(size_t i = 0;i < size;i++){
        auto temp = collidable.checkForRectangularCollision(pos, &colliders[i]);
        if(temp.collision){
            result = temp;
            break;
        }
    }

    return result; 
}

void Entity::update(Collidable& collidable){
    if(hasGravity) this->accelerate(glm::vec3(0,-0.01,0));
    
    glm::vec3& vel = this->velocity;

    float len = glm::length(vel);
    if(len > friction) vel = glm::normalize(vel) * (len - friction);
    else vel = glm::vec3(0);

    if(checkForCollision(collidable, false, {vel.x, 0, 0}).collision) vel.x = 0;
    if(checkForCollision(collidable, false, {0, vel.y, 0}).collision) vel.y = 0;
    if(checkForCollision(collidable, false, {0, 0, vel.z}).collision) vel.z = 0;

    this->position += vel;
}


