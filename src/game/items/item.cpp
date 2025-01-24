#include <game/items/item.hpp>

ItemPrototype* ItemPrototypeRegistry::addPrototype(ItemPrototype prototype){
    name_to_iterator[prototype.name] = prototypes.insert(prototypes.end(),prototype);
    return &(*name_to_iterator[prototype.name]);
}

ItemPrototype* ItemPrototypeRegistry::getPrototype(std::string name){
    if(!name_to_iterator.contains(name)) return nullptr;
    return &(*name_to_iterator[name]);
}

ItemPrototype::ItemPrototype(std::string name, const BlockRegistry::BlockPrototype* prototype): name(name){
    if(prototype->render_type != BlockRegistry::BILLBOARD) display_type = BLOCK;
    is_block = true;

    if(prototype->single_texture){
        auto texture_path = prototype->texture_paths[0];
        texture_paths = {texture_path, texture_path, texture_path};
    }
    else if(prototype->render_type == BlockRegistry::FULL_BLOCK){
        texture_paths = {
            prototype->texture_paths[0],
            prototype->texture_paths[2],
            prototype->texture_paths[4]
        };
    }

    if(prototype->render_type == BlockRegistry::BILLBOARD) model = std::make_shared<SpriteModel>(prototype->texture_paths[0]);
    else model = std::make_shared<BlockModel>(prototype);
    this->block_id = prototype->id;
}

ItemPrototype* ItemPrototypeRegistry::createPrototypeForBlock(const BlockRegistry::BlockPrototype* prototype){
    std::string prototype_name = "block_" + prototype->name;

    auto* existing_prototype = getPrototype(prototype_name);
    if(existing_prototype) return existing_prototype;

    return addPrototype({prototype_name, prototype});
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