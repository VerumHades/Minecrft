#include <entity.hpp>

Entity::Entity(glm::vec3 position, glm::vec3 colliderDimensions): position(position) {
    colliders.push_back({0,0,0,colliderDimensions.x,colliderDimensions.y,colliderDimensions.z});
}

void Entity::accelerate(glm::vec3 direction){
    auto newVelocity = velocity + direction;
    if(glm::length(newVelocity) > maxVelocity) newVelocity = glm::normalize(newVelocity) * maxVelocity;
    velocity = newVelocity;
}

CollisionCheckResult Entity::checkForCollision(Collidable& collidable, bool withVelocity, glm::vec3 offset){
    glm::vec3 pos = this->position + offset;
    if(withVelocity) pos += this->velocity;
    CollisionCheckResult result = {nullptr, false, 0,0,0};

    int size = colliders.size();
    for(int i = 0;i < size;i++){
        auto temp = collidable.checkForRectangularCollision(pos.x,pos.y,pos.z, &colliders[i]);
        if(temp.collision){
            result = temp;
            break;
        }
    }

    return result; 
}

void Entity::update(Collidable& collidable){
    glm::vec3& vel = this->velocity;
    if(hasGravity) vel += glm::vec3(0,-0.01,0);

    if(checkForCollision(collidable, false, {vel.x, 0, 0}).collision) vel.x = 0;
    if(checkForCollision(collidable, false, {0, vel.y, 0}).collision) vel.y = 0;
    if(checkForCollision(collidable, false, {0, 0, vel.z}).collision) vel.z = 0;

    float len = glm::length(vel);
    if(len > friction) vel = glm::normalize(vel) * (len - friction);
    else vel = glm::vec3(0);

    this->position += vel;
}


