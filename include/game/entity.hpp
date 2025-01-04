#ifndef ENTITY_H
#define ENTITY_H

#include <glm/glm.hpp>
#include <game/blocks.hpp>
#include <rendering/model.hpp>

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <unordered_set>

#include <vec_hash.hpp>

class World;


class Entity;

class Collidable{
    public:
        virtual bool collidesWith(glm::vec3 position, Entity* collider, bool vertical_check = false) = 0;
};

class DroppedItem;

#define VALID_ENTITY_DATA(t) static_assert(sizeof(t) <= 128, "Entity data too large!");

class Entity{
    private:
        glm::vec3 position = glm::vec3(0);
        glm::vec3 velocity = glm::vec3(0);
        std::unordered_set<glm::ivec3, IVec3Hash, IVec3Equal> regionPositions = {};
        glm::vec3 lastPosition = glm::vec3(0);

        float maxVelocityHorizontal = 6.0f;
        float maxVelocityVertical = 20.0f;
        float friction = 4.0f;
        bool hasGravity = true;
        bool on_ground = false;

    protected:
        std::string entity_typename = "entity";

        RectangularCollider collider;
        std::shared_ptr<Model> model;
        unsigned char entity_data[128];
        bool solid = true;

    public:
        Entity(glm::vec3 position, glm::vec3 colliderDimensions);

        std::function<void(Entity*, Entity*)> onCollision;
        bool destroy = false;

        void update(Collidable* world, bool altered, float deltatime);
        bool checkForCollision(Collidable* collidable, bool withVelocity, glm::vec3 offset = {0,0,0}, bool vertical_check = false);
        
        void accelerate(glm::vec3 direction, float deltatime);
        void decellerate(float strength, float deltatime);
        void setGravity(bool value){hasGravity = value;}
        void setModel(std::shared_ptr<Model> model) {this->model = model;}
        bool isSolid(){return solid;}
        bool setOnGround(bool value){on_ground = true;}

        std::shared_ptr<Model>& getModel() {return model;}

        const glm::vec3& getPosition() const {return position;};
        void setPosition(const glm::vec3& position) {this->position = position;}
        const glm::vec3& getVelocity() const {return velocity;};
        const RectangularCollider& getCollider() const {return collider;}

        const std::string getTypename() const {return entity_typename;}

        std::unordered_set<glm::ivec3, IVec3Hash, IVec3Equal>& getRegionPositions() { return regionPositions; }

        bool shouldGetDestroyed(){return destroy;}
        const unsigned char* getData() {return entity_data; }
};


#endif