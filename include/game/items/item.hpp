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
#include <rendering/bitworks.hpp>

class ItemTextureAtlas;
class Item;

using ItemID = size_t;
#define NO_ITEM 0ULL

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

        friend class ItemRegistry;
        friend class ItemTextureAtlas;

        /*
            Block textures from clockwise from top
        */
        ItemPrototype(std::string name, const BlockRegistry::BlockPrototype* block_prototype);

    public:
        ItemPrototype(std::string name, std::string texture_path);

        bool isBlock(){return is_block;}
        BlockID getBlockID(){return block_id;}
        const std::string& getName(){return name;}
        std::shared_ptr<Model>& getModel() {return model;}
};

class Item{
    private:
        ItemPrototype* prototype = nullptr;
        int quantity = 1;

        friend class ItemRegistry;
        friend class ItemTextureAtlas;

        Item (const Item&) = delete;
        Item& operator= (const Item&) = delete;
    
    public:
        // DO NOT USE, USE createItem from ItemRegistry instead!
        Item(ItemPrototype* prototype_ptr): prototype(prototype_ptr) {}

        ItemPrototype* getPrototype() {return prototype;}
        int getQuantity(){ return quantity; }
        void setQuantity(int number) {quantity = number;}

        void serialize(ByteArray& to);
        static ItemID deserialize(ByteArray& from);
};

class ItemRegistry{
    private:
        std::unordered_map<std::string, std::unique_ptr<ItemPrototype>> prototypes = {};
        std::unordered_map<size_t, Item> items = {};

        ItemRegistry(){}
    public:
        ItemPrototype* addPrototype(ItemPrototype prototype);
        ItemPrototype* getPrototype(std::string name);

        ItemPrototype* createPrototypeForBlock(const BlockRegistry::BlockPrototype* prototype);
        
        bool prototypeExists(std::string name);

        ItemID createItem(std::string prototype_name);
        ItemID createItem(ItemPrototype* prototype);

        void deleteItem(ItemID id);
        Item* getItem(ItemID id);

        void drawItemModels();
        void swapModelBuffers();

        static ItemRegistry& get();
};

class LogicalItemSlot{
    private:
        ItemID item = NO_ITEM;
        bool block_placing = false;

    public:
        LogicalItemSlot(){}
        bool takeItemFrom(LogicalItemSlot& source, int quantity = -1);
        bool moveItemTo(LogicalItemSlot& destination, int quantity = -1);
        bool swap(LogicalItemSlot& slot);

        bool addItem(ItemID id){
            LogicalItemSlot slot;
            slot.setItem(id);

            return takeItemFrom(slot);
        }

        void clear();
        bool hasItem() {return getItem() != nullptr; }
        
        /*
            Decreses the item amount and returns the count removed,
            automatically clears and manages the slot
        */
        int decreaseQuantity(int number);

        /*
            Takes a portion of the items in the slot and returns an id of an item that has exactly that quantity or less if there was not enough,
            automatically clears and manages the slot
        */
        ItemID getPortion(int quantity = -1);

        Item* getItem() {return ItemRegistry::get().getItem(item);}
        void setItem(ItemID id) {item = id;}
};

class LogicalItemInventory{
    private:
        int slots_horizontaly = 0;
        int slots_verticaly = 0;

        std::vector<LogicalItemSlot> slots{};
    public:
        LogicalItemInventory(int slots_horizontaly, int slots_verticaly);

        LogicalItemSlot* getSlot(int x, int y){
            if(x < 0 || x >= slots_horizontaly || y < 0 || y > slots_verticaly) return nullptr;

            return &slots[x + y * slots_horizontaly];
        }

        bool setItem(int x, int y, ItemID item) {
            auto* slot = getSlot(x,y);
            if(!slot) return false;
            
            return slot->addItem(item);
        }

        void removeItem(int x, int y){
            auto* slot = getSlot(x,y);
            if(!slot) return;

            slot->clear();
        }

        bool addItem(ItemID item);

        int getWidth() const {return slots_horizontaly;}
        int getHeight() const {return slots_verticaly;}

        std::vector<LogicalItemSlot>& getItemSlots() {return slots;};

        void serialize(ByteArray& to);
        static LogicalItemInventory deserialize(ByteArray& from);
};

class DroppedItem: public Entity{
    public:
        struct Data{
            Item item;
        };

    public:
        DroppedItem(Item item, glm::vec3 position);
};