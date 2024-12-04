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
        virtual bool collidesWith(glm::vec3 position, RectangularCollider* collider) const = 0;
        virtual std::vector<std::shared_ptr<Entity>>& getRegionEntities(const glm::ivec3 region_position) {}
};

class Entity: public Collidable{
    protected:
        glm::vec3 position = glm::vec3(0);
        glm::vec3 velocity = glm::vec3(0);
        std::optional<glm::ivec3> lastRegionPosition = std::nullopt;

        float maxVelocityHorizontal = 0.5f;
        float maxVelocityVertical = 0.5f;
        float friction = 0.005f;
        bool hasGravity = false;

        RectangularCollider collider;
        
        std::shared_ptr<Model> model;

    public:
        Entity(glm::vec3 position, glm::vec3 colliderDimensions);

        void update(Collidable* world);
        bool checkForCollision(Collidable* collidable, bool withVelocity, glm::vec3 offset = {0,0,0});
        bool collidesWith(glm::vec3 position, RectangularCollider* collider) const override;
        
        void accelerate(glm::vec3 direction);
        void setGravity(bool value){hasGravity = value;}
        void setModel(std::shared_ptr<Model> model) {this->model = model;}

        std::shared_ptr<Model>& getModel() {return model;}

        const glm::vec3& getPosition() {return position;};
        void setPosition(const glm::vec3& position) {this->position = position;}
        const glm::vec3& getVelocity() {return velocity;};
        const RectangularCollider& getCollider(){return collider;}

        std::optional<glm::ivec3>& getLastRegionPosition() {return lastRegionPosition;}
};


#endif