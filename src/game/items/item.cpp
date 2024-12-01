#include <game/items/item.hpp>


ItemPrototype* ItemPrototypeRegistry::addPrototype(ItemPrototype prototype){
    name_to_iterator[prototype.name] = prototypes.insert(prototypes.end(),prototype);
    return &(*name_to_iterator[prototype.name]);
}

ItemPrototype* ItemPrototypeRegistry::getPrototype(std::string name){
    if(!name_to_iterator.contains(name)) return nullptr;
    return &(*name_to_iterator[name]);
}

Item ItemPrototypeRegistry::createItem(std::string prototype_name){
    return Item(getPrototype(prototype_name));
}