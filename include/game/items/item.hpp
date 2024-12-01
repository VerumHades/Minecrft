#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <iostream>
#include <list>

class ItemTextureAtlas;
class Item;

class ItemPrototype{
    private:
        std::string name;
        std::string texture_path;

        friend class ItemPrototypeRegistry;
        friend class ItemTextureAtlas;
    
    public:
        ItemPrototype(std::string name, std::string texture_path): name(name), texture_path(texture_path){}
};

class ItemPrototypeRegistry{
    private:
        std::unordered_map<std::string, std::list<ItemPrototype>::iterator> name_to_iterator = {};
        std::list<ItemPrototype> prototypes = {};

    public:
        ItemPrototype* addPrototype(ItemPrototype prototype);
        ItemPrototype* getPrototype(std::string name);

        Item createItem(std::string prototype_name);
};

class Item{
    private:
        ItemPrototype* prototype = nullptr;

        Item(ItemPrototype* prototype_ptr): prototype(prototype_ptr) {}
        friend class ItemPrototypeRegistry;
        friend class ItemTextureAtlas;
    
    public:
        ItemPrototype* getPrototype() {return prototype;}
};