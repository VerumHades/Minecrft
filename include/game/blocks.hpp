#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <array>

#include <rendering/texture_registry.hpp>
#include <game/colliders.hpp>

class GameState;

#define CHUNK_SIZE 64

using BlockID = size_t;
#define BLOCK_AIR_INDEX 0

class BlockMetadata{
    public:
        virtual ~BlockMetadata() = default;
};

class BlockInterface{
    public:
        virtual void open(std::shared_ptr<BlockMetadata> metadata, GameState* game_state) = 0;
        virtual const std::string& getName() = 0;
};
class BlockBehaviour{
    public:
        virtual void update(BlockMetadata* metadata) = 0;
};


class BlockRegistry: public TextureRegistry{
    public:
        enum BlockRenderType{
            FULL_BLOCK,
            BILLBOARD
        } type;

        struct BlockPrototype{
            BlockID id;
            std::string name;
            std::vector<RectangularCollider> colliders{};
            
            bool single_texture = false; // If the block is same from all directions
            bool transparent = false; // If faces around it should get culled
            std::array<size_t,6> textures{}; // Top, bottom, left, rigth, front, back                     
            BlockRenderType render_type = FULL_BLOCK;

            std::array<std::string,6> texture_names{};
            std::array<std::string,6> texture_paths{};

            std::unique_ptr<BlockInterface> interface = nullptr;
            std::unique_ptr<BlockBehaviour> behaviour = nullptr;
        };

    private:
        std::vector<BlockPrototype> blocks{};
        
        BlockRegistry(){
            blocks.push_back({
                BLOCK_AIR_INDEX,
                "air",
                {},
                true,
                true,
                {},
                FULL_BLOCK
            });
        }

    public:

        /*
            Adds a full block with a collider, where texture path is the texture name for all sides
        */
        void addFullBlock(std::string name, std::string texture_name, bool transparent = false);

        /*
            Adds a full block with a collider, with the defined texture paths
        */
        void addFullBlock(std::string name, std::array<std::string,6> texture_names, bool transparent = false);

        /*
            Adds a billboard block without a collider with the defined texture
        */
        void addBillboardBlock(std::string name, std::string texture_name);

        BlockID getIndexByName(std::string name);
        BlockPrototype* getBlockPrototypeByIndex(BlockID id);

        void setPrototypeInterface(BlockID id, std::unique_ptr<BlockInterface> interface);

        size_t registeredBlocksTotal(){return blocks.size();}
        const std::vector<BlockPrototype>& prototypes() {return blocks;};

        static BlockRegistry& get();
};

struct Block{
    BlockID id = 0;
    std::shared_ptr<BlockMetadata> metadata = nullptr;

    Block() {}
    Block(BlockID id): id(id) {}
};