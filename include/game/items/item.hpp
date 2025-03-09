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
#include <path_config.hpp>

#include <memory>

class ItemTextureAtlas;
class Item;

using ItemRef = std::shared_ptr<Item>;
#define NO_ITEM nullptr;

class ItemPrototype{
    public:
        struct ToolEffectiveness{
            std::string material_name;
            int mining_power;
        };
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
        std::vector<ToolEffectiveness> effective_againist_materials{};

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
        const std::vector<ToolEffectiveness>& getToolEffectiveness() {return effective_againist_materials;}
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
        // DO NOT USE
        Item(ItemPrototype* prototype_ptr): prototype(prototype_ptr) {}

        static ItemRef Create(const std::string& name);
        static ItemRef Create(ItemPrototype* prototype);

        ItemPrototype* getPrototype() {return prototype;}
        int getQuantity(){ return quantity; }
        void setQuantity(int number) {quantity = number;}

        void serialize(ByteArray& to);
        static std::shared_ptr<Item> deserialize(ByteArray& from);
};

class ItemRegistry{
    private:
        std::unordered_map<std::string, std::unique_ptr<ItemPrototype>> prototypes = {};

        ItemRegistry(){}
    public:
        ItemPrototype* addPrototype(ItemPrototype prototype);
        ItemPrototype* getPrototype(const std::string& name);

        ItemPrototype* createPrototypeForBlock(const BlockRegistry::BlockPrototype* prototype);
        bool prototypeExists(const std::string& name);

        static ItemRegistry& get();
        static bool LoadFromXML(const std::string& path);
};

class LogicalItemSlot{
    private:
        ItemRef item;
        bool block_placing = false;

    public:
        LogicalItemSlot(){}
        bool takeItemFrom(LogicalItemSlot& source, int quantity = -1);
        bool moveItemTo(LogicalItemSlot& destination, int quantity = -1);
        bool swap(LogicalItemSlot& slot);

        bool addItem(ItemRef item){
            LogicalItemSlot slot;
            slot.setItem(item);

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
        ItemRef getPortion(int quantity = -1);

        Item* getItem() {return item.get();}
        void setItem(ItemRef item) {this->item = item;}
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

        bool setItem(int x, int y, ItemRef item) {
            auto* slot = getSlot(x,y);
            if(!slot) return false;
            
            return slot->addItem(item);
        }

        void removeItem(int x, int y){
            auto* slot = getSlot(x,y);
            if(!slot) return;

            slot->clear();
        }

        bool addItem(ItemRef item);

        int getWidth() const {return slots_horizontaly;}
        int getHeight() const {return slots_verticaly;}

        std::vector<LogicalItemSlot>& getItemSlots() {return slots;};

        void serialize(ByteArray& to);
        static LogicalItemInventory deserialize(ByteArray& from);
};

class DroppedItem: public EntityData{
    private:
        ItemRef item;

    public:
        DroppedItem(ItemRef item): item(item){type = DROPPED_ITEM;}

        void serialize(ByteArray& array) override;
        void update(GameState* state) override;
        ItemRef getItem(){return item;}

        void setup(Entity* entity) override;

        static Entity create(glm::vec3 position, ItemRef item){
            if(!item) return {};

            return Entity(position, glm::vec3{0.4,0.4,0.4}, std::make_shared<DroppedItem>(item));
        }
        static std::shared_ptr<EntityData> deserializeData(ByteArray& array);
};