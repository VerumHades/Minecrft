#include <rendering/texture_registry.hpp>

void TextureRegistry::addTexture(std::string name, std::string path){   
    textures[name] = {
        name,
        path,
        last_index++
    };
}

size_t TextureRegistry::getTextureIndex(std::string name){
    if(!textures.contains(name)) return 0;
    return textures.at(name).index;
}

void TextureRegistry::load(){
    std::vector<std::string> ordered_paths;
    ordered_paths.resize(textures.size(), "");

    for(auto& [name,texture]: textures){
        ordered_paths[texture.index] = texture.path;
    }

    opengl_loaded_textures.loadFromFiles(ordered_paths, texture_width, texture_height);
}