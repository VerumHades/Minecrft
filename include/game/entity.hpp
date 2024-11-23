#ifndef ENTITY_H
#define ENTITY_H

#include <glm/glm.hpp>
#include <vector>
#include <game/blocks.hpp>
#include <string>

typedef struct CollisionCheckResult{
    Block* collidedBlock;
    bool collision;
    int x;
    int y;
    int z;
} CollisionCheckResult;

class Collidable{
    public:
        virtual CollisionCheckResult checkForRectangularCollision(glm::vec3 position, RectangularCollider* collider) = 0;
};

class Entity{
    private:
        glm::vec3 position = glm::vec3(0);
        glm::vec3 velocity = glm::vec3(0);

        float maxVelocityHorizontal = 0.5f;
        float maxVelocityVertical = 0.5f;
        float friction = 0.005f;
        bool hasGravity = true;

        std::vector<RectangularCollider> colliders;
        
        std::string modelName = "default"; // This is going to be bad if models get added on the run

    public:
        Entity(glm::vec3 position, glm::vec3 colliderDimensions);

        void update(Collidable& collidable);
        CollisionCheckResult checkForCollision(Collidable& collidable, bool withVelocity, glm::vec3 offset = {0,0,0});
        
        void accelerate(glm::vec3 direction);
        void setGravity(bool value){hasGravity = value;}

        const glm::vec3& getPosition() {return position;};
        void setPosition(const glm::vec3& position) {this->position = position;}
        const glm::vec3& getVelocity() {return velocity;};
        const std::vector<RectangularCollider>& getColliders() {return colliders;}
        std::string getModelName() {return modelName;};
        void setModel(const std::string& name) {modelName = name;}
};


#endif