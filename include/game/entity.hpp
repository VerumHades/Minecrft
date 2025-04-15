#ifndef ENTITY_H
#define ENTITY_H

#include <glm/glm.hpp>
#include <game/blocks.hpp>

#include <structure/serialization/serializer.hpp>

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

class EntityData{
    public:
        enum Type{
            NONE,
            DROPPED_ITEM
        } type;
        bool do_update = false;

        virtual void serialize(ByteArray& array) = 0;
        virtual void update(GameState* state) = 0;
        virtual void setup(Entity* entity) = 0;
};

class Entity{
    private:
        glm::vec3 position = glm::vec3(0);
        glm::vec3 velocity = glm::vec3(0);
        glm::vec3 lastPosition = glm::vec3(0);

        float maxVelocityHorizontal = 6.0f;
        float maxVelocityVertical = 50.0f;
        float friction = 4.0f;
        bool hasGravity = true;

        std::unordered_set<std::string> tags{};

        static std::shared_ptr<EntityData> deserializeData(ByteArray& array);

        std::shared_ptr<ModelInstance> model_instance;

    protected:
        std::shared_ptr<EntityData> data;
        RectangularCollider collider;
        std::shared_ptr<Model> model;
        bool solid = true;

        friend class Serializer;

    public:
        Entity(glm::vec3 position, glm::vec3 colliderDimensions);
        Entity(glm::vec3 position, glm::vec3 colliderDimensions, std::shared_ptr<EntityData> data);
        Entity(): Entity(glm::vec3{0,0,0},{1,1,1}){}

        std::function<void(Entity*, Entity*)> onCollision;
        std::function<void(Entity*)> onTerrainCollision;
        bool destroy = false;

        void accelerate(glm::vec3 direction, float deltatime);
        void decellerate(float strength, float deltatime);
        void setGravity(bool value){hasGravity = value;}
        bool isSolid(){return solid;}
        void setSolid(bool value){solid = value;}

        float getFriction() { return friction; }

        bool HasGravity() {return hasGravity; };

        void setModel(std::shared_ptr<Model> model);
        std::shared_ptr<Model>& getModel() {return model;}

        const glm::vec3& getPosition() const {return position;};
        void setPosition(const glm::vec3& position);

        glm::vec3& getVelocity() {return velocity;};
        const RectangularCollider& getCollider() const {return collider;}

        bool shouldGetDestroyed(){return destroy;}

        std::shared_ptr<EntityData>& getData(){return data;}
        void setData(const std::shared_ptr<EntityData>& data){
            this->data = data;
            if(this->data) this->data->setup(this);
        }

        void addTag(const std::string& tag){tags.emplace(tag);}
        bool hasTag(const std::string& tag){return tags.contains(tag);}
};

#include <game/items/item.hpp>

#endif
