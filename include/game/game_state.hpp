#pragma once

#include <game/world/terrain.hpp>
#include <game/items/item.hpp>
#include <list>

class GameState{
    private:
        Terrain terrain;
        std::list<Entity> entities;

        LogicalItemInventory player_inventory{10,5};
        LogicalItemInventory player_hotbar{9,1};

        WorldGenerator world_generator;
        WorldStream world_stream;

        void drawEntity(Entity& entity);
        
    public:
        GameState(const std::string& filename);

        void unload();

        void loadChunk(const glm::ivec3& position);
        void unloadChunk(const glm::ivec3& position);

        void updateEntity(Entity& entity, float deltatime);
        void updateEntities(float deltatime);

        void addEntity(Entity entity) {
            entities.push_back(entity);
        }
        void drawEntityColliders(WireframeCubeRenderer& renderer, size_t start_index = 50);
        
        bool entityCollision(Entity& entity, const glm::vec3& offset = {0,0,0});

        Entity& getPlayer(){return entities.front();}
        LogicalItemInventory& getPlayerInventory(){return player_inventory;};
        LogicalItemInventory& getPlayerHotbar(){return player_hotbar;}
        Terrain& getTerrain(){return terrain;}
};