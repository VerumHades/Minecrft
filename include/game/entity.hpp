#ifndef ENTITY_H
#define ENTITY_H

#include <glm/glm.hpp>
#include <game/blocks.hpp>
#include <rendering/model.hpp>

#include <memory>
#include <string>
#include <vector>

class World;


class Entity;

class Collidable{
    public:
        virtual bool collidesWith(glm::vec3 position, Entity* collider) const = 0;
};

class DroppedItem;

#define VALID_ENTITY_DATA(t) static_assert(sizeof(t) <= 128, "Entity data too large!");

class Entity{
    private:
        glm::vec3 position = glm::vec3(0);
        glm::vec3 velocity = glm::vec3(0);
        std::optional<glm::ivec3> lastRegionPosition = std::nullopt;

        float maxVelocityHorizontal = 0.5f;
        float maxVelocityVertical = 0.5f;
        float friction = 0.005f;
        bool hasGravity = true;

    protected:
        std::string entity_typename = "entity";

        RectangularCollider collider;
        std::shared_ptr<Model> model;
        unsigned char entity_data[128];

    public:
        Entity(glm::vec3 position, glm::vec3 colliderDimensions);

        std::function<void(Entity*, Entity*)> onCollision;
        bool destroy = false;

        void update(Collidable* world);
        bool checkForCollision(Collidable* collidable, bool withVelocity, glm::vec3 offset = {0,0,0});
        
        void accelerate(glm::vec3 direction);
        void setGravity(bool value){hasGravity = value;}
        void setModel(std::shared_ptr<Model> model) {this->model = model;}

        std::shared_ptr<Model>& getModel() {return model;}

        const glm::vec3& getPosition() const {return position;};
        void setPosition(const glm::vec3& position) {this->position = position;}
        const glm::vec3& getVelocity() const {return velocity;};
        const RectangularCollider& getCollider() const {return collider;}

        const std::string getTypename() const {return entity_typename;}

        std::optional<glm::ivec3>& getLastRegionPosition() {return lastRegionPosition;}

        bool shouldGetDestroyed(){return destroy;}
        const unsigned char* getData() {return entity_data; }
};


#endif