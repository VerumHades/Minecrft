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

class ItemTextureAtlas;
class Item;

class ItemPrototype{
    private:
        std::string name;

        enum IconDisplayType{
            SIMPLE,
            BLOCK
        } display_type = SIMPLE;
        
        std::array<std::string,3> texture_paths = {};
        
        std::shared_ptr<SpriteModel> model;

        friend class ItemPrototypeRegistry;
        friend class ItemTextureAtlas;
    
    public:
        ItemPrototype(std::string name, std::string texture_path): name(name){
            display_type = SIMPLE;
            texture_paths[0] = texture_path;
            model = std::make_shared<SpriteModel>(texture_path);
        }

        /*
            Block textures from clockwise from top
        */
        ItemPrototype(std::string name, std::array<std::string,3> texture_path): name(name), texture_paths(texture_path){
            display_type = BLOCK;
            model = std::make_shared<SpriteModel>(texture_path[0]);
        }
        std::shared_ptr<SpriteModel>& getModel() {return model;}
};

class ItemPrototypeRegistry{
    private:
        std::unordered_map<std::string, std::list<ItemPrototype>::iterator> name_to_iterator = {};
        std::list<ItemPrototype> prototypes = {};

    public:
        ItemPrototype* addPrototype(ItemPrototype prototype);
        ItemPrototype* getPrototype(std::string name);

        ItemPrototype* createPrototypeForBlock(BlockRegistry::BlockPrototype* prototype, TextureRegistry& texture_registry);

        bool prototypeExists(std::string name);

        void drawItemModels();
        
        void resetModelsDrawRequests();
        void passModelsDrawRequests();

        void updateModelsDrawRequestBuffer();

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
        DroppedItem(Item item, glm::vec3 position): Entity(position, {0.3,0.3,0.3}) {
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
};