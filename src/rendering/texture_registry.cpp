#include <rendering/texture_registry.hpp>

void TextureRegistry::addTexture(std::string name, std::string path){   
    if(textures.contains(name)){
        std::cerr << "Duplicate texture name '" << name << "'" << std::endl;
        return;
    }
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

void TextureRegistry::loadFromFolder(std::string path){
    for (const auto& entry : std::filesystem::directory_iterator(path)){
        auto& path = entry.path();

        int width, height, channels;
        if (!stbi_info(path.c_str(), &width, &height, &channels)) continue;

        addTexture(path.stem().string(), path);
    }
}