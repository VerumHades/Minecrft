#include <game/blocks.hpp>

/*
    Adds a full block, where texture path is the texture name for all sides
*/
void BlockRegistry::addFullBlock(std::string name, std::string texture_name, bool transparent){
    blocks.push_back({
        name,
        {{0, 0, 0, 1.0f, 1.0f, 1.0f}},
        true,
        transparent,
        {texture_registry.getTextureIndex(texture_name)},
        FULL_BLOCK,
        {texture_name}
    });
}

/*
    Adds a full block, with the defined texture paths
*/
void BlockRegistry::addFullBlock(std::string name, std::array<std::string,6> texture_names, bool transparent){
    std::array<size_t, 6> textures;

    int i = 0;
    for(auto& name: texture_names){
        textures[i++] = texture_registry.getTextureIndex(name);
    }

    blocks.push_back({
        name,
        {{0, 0, 0, 1.0f, 1.0f, 1.0f}},
        false,
        transparent,
        textures,
        FULL_BLOCK,
        texture_names
    });
}

/*
    Adds a billboard block with the defined texture
*/
void BlockRegistry::addBillboardBlock(std::string name, std::string texture_name){
    blocks.push_back({
        name,
        {},
        true,
        false,
        {texture_registry.getTextureIndex(texture_name)},
        BILLBOARD,
        {texture_name}
    });
}

size_t BlockRegistry::getIndexByName(std::string name){
    int i = -1;
    for(auto& registered_block: blocks){
        i++;
        if(registered_block.name != name) continue;

        return i;
    }

    return 0;
}

BlockRegistry::BlockPrototype* BlockRegistry::getBlockPrototypeByIndex(size_t id){
    if(id >= blocks.size()) return nullptr;
    return &blocks[id];
}