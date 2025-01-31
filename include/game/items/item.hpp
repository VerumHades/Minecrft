#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <iostream>
#include <list>
#include <game/entity.hpp>
#include <game/items/sprite_model.hpp>
#include <game/blocks.hpp>
#include <game/items/block_model.hpp>

class ItemTextureAtlas;
class Item;

class ItemPrototype{
    private:
        std::string name = "undefined";

        enum IconDisplayType{
            SIMPLE,
            BLOCK
        } display_type = SIMPLE;
        bool is_block = false;

        BlockID block_id = 0;
        
        std::array<std::string,3> texture_paths = {"","",""};
        
        std::shared_ptr<Model> model;

        friend class ItemPrototypeRegistry;
        friend class ItemTextureAtlas;

        /*
            Block textures from clockwise from top
        */
        ItemPrototype(std::string name, const BlockRegistry::BlockPrototype* block_prototype);

    public:
        ItemPrototype(std::string name, std::string texture_path);

        bool isBlock(){return is_block;}
        BlockID getBlockID(){return block_id;}
        std::shared_ptr<Model>& getModel() {return model;}
};

class ItemPrototypeRegistry{
    private:
        std::unordered_map<std::string, std::list<ItemPrototype>::iterator> name_to_iterator = {};
        std::list<ItemPrototype> prototypes = {};

    public:
        ItemPrototypeRegistry(){}
        ItemPrototype* addPrototype(ItemPrototype prototype);
        ItemPrototype* getPrototype(std::string name);

        ItemPrototype* createPrototypeForBlock(const BlockRegistry::BlockPrototype* prototype);

        bool prototypeExists(std::string name);

        void drawItemModels();
        void swapModelBuffers();

        Item createItem(std::string prototype_name);
        Item createItem(ItemPrototype* prototype);
};

class Item{
    private:
        ItemPrototype* prototype = nullptr;
        int quantity = 1;

        Item(ItemPrototype* prototype_ptr): prototype(prototype_ptr) {}
        friend class ItemPrototypeRegistry;
        friend class ItemTextureAtlas;
    
    public:
        ItemPrototype* getPrototype() {return prototype;}
        int getQuantity(){ return quantity; }
        void setQuantity(int number) {quantity = number;}
};

class DroppedItem: public Entity{
    public:
        struct Data{
            Item item;
        };

    public:
        DroppedItem(Item item, glm::vec3 position);
};