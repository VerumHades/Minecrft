#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <array>

#include <rendering/texture_registry.hpp>
#include <game/colliders.hpp>

#define CHUNK_SIZE 64

using BlockID = size_t;
#define BLOCK_AIR_INDEX 0

class BlockRegistry: public TextureRegistry{
    public:
        enum BlockRenderType{
            FULL_BLOCK,
            BILLBOARD
        } type;

        struct BlockPrototype{
            BlockID id;
            std::string name;
            std::vector<RectangularCollider> colliders;
            
            bool single_texture = false; // If the block is same from all directions
            bool transparent = false; // If faces around it should get culled
            std::array<size_t,6> textures; // Top, bottom, left, rigth, front, back                     
            BlockRenderType render_type;

            std::array<std::string,6> texture_names;
            std::array<std::string,6> texture_paths;
        };

    private:
        std::vector<BlockPrototype> blocks;
        
    public:
        BlockRegistry(){
            blocks.push_back({
                0,
                "air",
                {},
                true,
                true,
                {},
                FULL_BLOCK,
                {}
            });
        }

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

        size_t registeredBlocksTotal(){return blocks.size();}
        const std::vector<BlockPrototype>& prototypes() {return blocks;};
};

extern BlockRegistry global_block_registry;

struct Block{
    BlockID id = 0;

    Block() {}
    Block(BlockID id): id(id) {}
};