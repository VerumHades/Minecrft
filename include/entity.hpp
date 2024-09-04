#ifndef ENTITY_H
#define ENTITY_H

#include <glm/glm.hpp>
#include <vector>
#include <blocks.hpp>
#include <rendering/model.hpp>

typedef struct CollisionCheckResult{
    Block* collidedBlock;
    bool collision;
    int x;
    int y;
    int z;
} CollisionCheckResult;

class Collidable{
    public:
        virtual CollisionCheckResult checkForRectangularCollision(float x, float y, float z, RectangularCollider* collider) = 0;
};

class Entity{
    private:
        glm::vec3 position = glm::vec3(0);
        glm::vec3 velocity = glm::vec3(0);

        float maxVelocityHorizontal = 0.1f;
        float maxVelocityVertical = 0.5f;
        float friction = 0.005f;
        bool hasGravity = true;

        std::vector<RectangularCollider> colliders;
        
        std::unique_ptr<Model> model;

    public:
        Entity(glm::vec3 position, glm::vec3 colliderDimensions);

        void update(Collidable& collidable);
        CollisionCheckResult checkForCollision(Collidable& collidable, bool withVelocity, glm::vec3 offset = {0,0,0});
        
        void accelerate(glm::vec3 direction);

        const glm::vec3& getPosition() {return position;};
        const glm::vec3& getVelocity() {return velocity;};
        const std::vector<RectangularCollider>& getColliders() {return colliders;}
};

#endif