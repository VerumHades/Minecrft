#include <game/blocks.hpp>

BlockRegistry global_block_registry{};

/*
    Adds a full block, where texture path is the texture name for all sides
*/
void BlockRegistry::addFullBlock(std::string name, std::string texture_name, bool transparent){
    blocks.push_back({
        blocks.size(),
        name,
        {{0, 0, 0, 1.0f, 1.0f, 1.0f}},
        true,
        transparent,
        {getTextureIndex(texture_name)},
        FULL_BLOCK,
        {texture_name},
        {getTextureByName(texture_name)->path}
    });
}

/*
    Adds a full block, with the defined texture paths
*/
void BlockRegistry::addFullBlock(std::string name, std::array<std::string,6> texture_names, bool transparent){
    std::array<size_t, 6> textures{};
    std::array<std::string, 6> texture_paths{};

    int i = 0;
    for(auto& name: texture_names){
        auto* texture = getTextureByName(name);
        if(!texture) {
            std::cerr << "Invalid texture name: " << name << std::endl;
            return;
        }
        texture_paths[i] = texture->path;
        //std::cout << texture_paths[i] << " " << name << std::endl;

        textures[i++] = getTextureIndex(name);
    }

    blocks.push_back({
        blocks.size(),
        name,
        {{0, 0, 0, 1.0f, 1.0f, 1.0f}},
        false,
        transparent,
        textures,
        FULL_BLOCK,
        texture_names,
        texture_paths
    });
}

/*
    Adds a billboard block with the defined texture
*/
void BlockRegistry::addBillboardBlock(std::string name, std::string texture_name){
    blocks.push_back({
        blocks.size(),
        name,
        {},
        true,
        true,
        {getTextureIndex(texture_name)},
        BILLBOARD,
        {texture_name},
        {getTextureByName(texture_name)->path}
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