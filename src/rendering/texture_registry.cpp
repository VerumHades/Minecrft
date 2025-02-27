#include <rendering/texture_registry.hpp>

void TextureRegistry::addTexture(std::string name, const std::string& path){   
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
 TextureRegistry::RegisteredTexture* TextureRegistry::getTextureByName(std::string name){
    if(!textures.contains(name)) return nullptr;
    return &textures.at(name);
}

std::unique_ptr<GLTextureArray> TextureRegistry::load(){
    std::vector<std::string> ordered_paths;
    ordered_paths.resize(textures.size(), "");

    for(auto& [name,texture]: textures){
        ordered_paths[texture.index] = texture.path;
    }

    auto out = std::make_unique<GLTextureArray>();
    out->loadFromFiles(ordered_paths, texture_width, texture_height);

    return out;
}

void TextureRegistry::loadFromFolder(const std::string& path){
    for (const auto& entry : fs::directory_iterator(path)){
        auto& path = entry.path();

        int width, height, channels;
        if (!stbi_info(path.string().c_str(), &width, &height, &channels)) continue;

        addTexture(path.stem().string(), path.string());
    }
}