#include <game/items/item.hpp>

ItemPrototype* ItemPrototypeRegistry::addPrototype(ItemPrototype prototype){
    name_to_iterator[prototype.name] = prototypes.insert(prototypes.end(),prototype);
    return &(*name_to_iterator[prototype.name]);
}

ItemPrototype* ItemPrototypeRegistry::getPrototype(std::string name){
    if(!name_to_iterator.contains(name)) return nullptr;
    return &(*name_to_iterator[name]);
}

ItemPrototype* ItemPrototypeRegistry::createPrototypeForBlock(BlockRegistry::BlockPrototype* prototype, TextureRegistry& texture_registry){
    std::string prototype_name = "block_" + prototype->name;
    
    auto* existing_prototype = getPrototype(prototype_name);
    if(existing_prototype) return existing_prototype;

    std::array<std::string, 3> texture_paths = {};

    if(prototype->single_texture){
        auto texture_path = texture_registry.getTextureByName(prototype->texture_names[0])->path;
        texture_paths = {texture_path, texture_path, texture_path};
    }
    else if(prototype->render_type == BlockRegistry::FULL_BLOCK){
        texture_paths = {
            texture_registry.getTextureByName(prototype->texture_names[0])->path,
            texture_registry.getTextureByName(prototype->texture_names[2])->path,
            texture_registry.getTextureByName(prototype->texture_names[4])->path
        };
    }

    return addPrototype({prototype_name, texture_paths});
}

bool ItemPrototypeRegistry::prototypeExists(std::string name){
    return name_to_iterator.contains(name);
}

Item ItemPrototypeRegistry::createItem(ItemPrototype* prototype){
    return Item(prototype);
}  
Item ItemPrototypeRegistry::createItem(std::string prototype_name){
    return createItem(getPrototype(prototype_name));
}

void ItemPrototypeRegistry::drawItemModels(){
    for(auto& prototype: prototypes){
        prototype.getModel()->drawAllRequests();
    }
}

void ItemPrototypeRegistry::swapModelBuffers(){
    for(auto& prototype: prototypes){
        prototype.getModel()->swap();
    }
}