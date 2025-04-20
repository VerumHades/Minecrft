#pragma once

#include <game/items/item.hpp>
#include <game/save_structure.hpp>
#include <game/world/terrain.hpp>

#include <structure/bytearray.hpp>
#include <structure/record_store.hpp>

#include <filesystem>
#include <list>

class TerrainManager;

class GameState {
  private:
    std::string path;

    Terrain           terrain;
    std::list<Entity> entities;

    LogicalItemInventory player_inventory{10, 5};
    LogicalItemInventory player_hotbar{9, 1};

    LogicalItemInventory player_crafting_inventory{2, 2};
    LogicalItemInventory player_crafting_inventory_result{1, 1};

    int player_health = 20;

    FileSaveStructure save_structure;

    struct Header {
        char name[256];
        int  seed = 0;
    };

    using SegmentStore = RecordStore<glm::ivec3, Header, IVec3Hash, IVec3Equal>;
    std::shared_ptr<SegmentStore> world_storage;
    std::shared_ptr<WorldStream>  world_saver;

    FileStream* world_stream;
    FileStream* player_stream;
    FileStream* entity_stream;

    void savePlayer();
    void loadPlayer();

    void saveEntities();
    void loadEntities();

    friend class TerrainManager;

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

    bool entityCollision(Entity& entity, const glm::vec3& offset = {0, 0, 0});

    void giveItemToPlayer(ItemRef item);

    int getSeed() {
        return world_storage->GetHeader().seed;
    }
    Entity& getPlayer() {
        return entities.front();
    }

    std::string GetName() {
        return world_storage->GetHeader().name;
    }
    void SetName(std::string name) {
        if (name.size() > 256)
            name = name.substr(0, 256);

        strcpy(world_storage->GetHeader().name, name.c_str());
    }

    LogicalItemInventory& getPlayerInventory() {
        return player_inventory;
    };
    LogicalItemInventory& getPlayerHotbar() {
        return player_hotbar;
    }
    LogicalItemInventory& getPlayerCraftingInventory() {
        return player_crafting_inventory;
    };
    LogicalItemInventory& getPlayerCraftingResultInventory() {
        return player_crafting_inventory_result;
    }

    Terrain& GetTerrain() {
        return terrain;
    }
    int& getPlayerHealth() {
        return player_health;
    }
};
