#pragma once

#include <game/world/terrain.hpp>
#include <game/items/item.hpp>
#include <game/save_structure.hpp>
#include <list>
#include <filesystem>

class TerrainManager;

class GameState{
    private:
        std::string path;

        Terrain terrain;
        std::list<Entity> entities;

        LogicalItemInventory player_inventory{10,5};
        LogicalItemInventory player_hotbar{9,1};
        
        LogicalItemInventory player_crafting_inventory{2,2};
        LogicalItemInventory player_crafting_inventory_result{1,1};

        int player_health = 20;

        FileSaveStructure save_structure;
        
        WorldStream* world_stream;
        FileStream* player_stream;
        FileStream* entity_stream;

        void savePlayer();
        void loadPlayer();

        void saveEntities();
        void loadEntities();

        void drawEntity(Entity& entity);

        friend class TerrainManager;

        int seed = 0;
        
    public:
        GameState(const std::string& filename, int worldSeed = -1);

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

        void giveItemToPlayer(ItemRef item);

        int getSeed(){return world_stream->getSeed();}
        Entity& getPlayer(){return entities.front();}
        LogicalItemInventory& getPlayerInventory(){return player_inventory;};
        LogicalItemInventory& getPlayerHotbar(){return player_hotbar;}
        LogicalItemInventory& getPlayerCraftingInventory(){return player_crafting_inventory;};
        LogicalItemInventory& getPlayerCraftingResultInventory(){return player_crafting_inventory_result;}
        Terrain& getTerrain(){return terrain;}
        WorldStream* getWorldStream(){return world_stream;}
        int& getPlayerHealth(){return player_health;}
};