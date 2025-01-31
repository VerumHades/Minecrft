#include <game/items/item.hpp>

ItemPrototype* ItemPrototypeRegistry::addPrototype(ItemPrototype prototype){
    name_to_iterator[prototype.name] = prototypes.insert(prototypes.end(),prototype);
    return &(*name_to_iterator.at(prototype.name));
}

ItemPrototype* ItemPrototypeRegistry::getPrototype(std::string name){
    if(!name_to_iterator.contains(name)) return nullptr;
    return &(*name_to_iterator.at(name));
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
ItemPrototype::ItemPrototype(std::string name, std::string texture_path): name(name){
    display_type = SIMPLE;
    texture_paths[0] = texture_path;
    model = std::make_shared<SpriteModel>(texture_path);
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
    auto prototype = getPrototype(prototype_name);
    if(!prototype){
        std::cerr << "Invalid item creation!" << std::endl;
        throw std::logic_error("Invalid item creation");
    }
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

DroppedItem::DroppedItem(Item item, glm::vec3 position): Entity(position, {0.3,0.3,0.3}) {
    if(!item.getPrototype()){
        std::cerr << "Dropped item without prototype!" << std::endl;
        return;
    }
    VALID_ENTITY_DATA(Data);

    solid = false;

    entity_typename = "dropped_item";
    reinterpret_cast<Data*>(entity_data)->item = item;

    onCollision = [](Entity* self, Entity* entity) {
        self->destroy = true;
    };

    model = item.getPrototype()->getModel();
}