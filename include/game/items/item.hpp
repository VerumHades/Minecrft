#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <iostream>
#include <list>
#include <game/entity.hpp>
#include <game/items/sprite_model.hpp>

class ItemTextureAtlas;
class Item;

class ItemPrototype{
    private:
        std::string name;
        std::string texture_path;
        std::shared_ptr<SpriteModel> model;

        friend class ItemPrototypeRegistry;
        friend class ItemTextureAtlas;
    
    public:
        ItemPrototype(std::string name, std::string texture_path): name(name), texture_path(texture_path){
            model = std::make_shared<SpriteModel>(texture_path);
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

        void drawItemModels();
        
        void resetModelsDrawRequests();
        void passModelsDrawRequests();

        void updateModelsDrawRequestBuffer();

        Item createItem(std::string prototype_name);
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