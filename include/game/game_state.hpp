#pragma once

#include <game/world/terrain.hpp>

class GameState{
    private:
        Terrain terrain;
        std::vector<Entity> entities;

        WorldGenerator world_generator;

        void drawEntity(Entity& entity);
        
    public:
        GameState();
        void loadChunk(const glm::ivec3& position);
        void unloadChunk(const glm::ivec3& position);

        void updateEntity(Entity& entity, float deltatime);
        void updateEntities(float deltatime);

        void addEntity(Entity entity) {
            entities.push_back(entity);
        }
        void drawEntityColliders(WireframeCubeRenderer& renderer, size_t start_index = 50);
        
        bool entityCollision(Entity& entity, const glm::vec3& offset = {0,0,0});

        Entity& getPlayer(){return entities[0];}
        Terrain& getTerrain(){return terrain;}
};