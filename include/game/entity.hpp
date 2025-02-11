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

class Terrain;


class Entity;

class DroppedItem;
class GameState;

#define VALID_ENTITY_DATA(t) static_assert(sizeof(t) <= 128, "Entity data too large!");

class EntityData{
    public:
        enum Type{
            NONE
        };

        virtual void serialize(ByteArray& array) = 0;
        virtual void update(GameState* state) = 0;
        static std::shared_ptr<EntityData> deserialize(ByteArray& array);
};

class Entity{
    private:
        glm::vec3 position = glm::vec3(0);
        glm::vec3 velocity = glm::vec3(0);
        glm::vec3 lastPosition = glm::vec3(0);

        float maxVelocityHorizontal = 6.0f;
        float maxVelocityVertical = 20.0f;
        float friction = 4.0f;
        bool hasGravity = true;
        bool on_ground = false;

        Entity(){}
        
    protected:
        std::shared_ptr<EntityData> data;
        RectangularCollider collider;
        std::shared_ptr<Model> model;
        bool solid = true;

    public:
        Entity(glm::vec3 position, glm::vec3 colliderDimensions);

        std::function<void(Entity*, Entity*)> onCollision;
        bool destroy = false;

        void accelerate(glm::vec3 direction, float deltatime);
        void decellerate(float strength, float deltatime);
        void setGravity(bool value){hasGravity = value;}
        void setModel(std::shared_ptr<Model> model) {this->model = model;}
        bool isSolid(){return solid;}
        void setOnGround(bool value){on_ground = value;}

        std::shared_ptr<Model>& getModel() {return model;}

        const glm::vec3& getPosition() const {return position;};
        void setPosition(const glm::vec3& position) {this->position = position;}
        const glm::vec3& getVelocity() const {return velocity;};
        const RectangularCollider& getCollider() const {return collider;}

        bool shouldGetDestroyed(){return destroy;}

        void serialize(ByteArray& array);
        static Entity deserialize(ByteArray& array);

        friend class GameState;
};


#endif