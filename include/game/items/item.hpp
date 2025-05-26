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

#include <structure/bitworks.hpp>
#include <structure/serialization/serializer.hpp>
#include <path_config.hpp>


#include <memory>

class ItemTextureAtlas;
class Item;

using ItemRef = std::shared_ptr<Item>;
#define NO_ITEM nullptr;

/**
 * @brief A prototype for an item and its display and behaviour
 * 
 */
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

/**
 * @brief An instance of an item prototype
 * 
 */
class Item{
    private:
        ItemPrototype* prototype = nullptr;
        int quantity = 1;

        friend class ItemRegistry;
        friend class ItemTextureAtlas;
        friend class Serializer;

        Item (const Item&) = delete;
        Item& operator= (const Item&) = delete;

    public:
        // DO NOT USE
        Item(ItemPrototype* prototype_ptr): prototype(prototype_ptr) {}

        /**
         * @brief Create an item with prototype by name
         * 
         * @param name 
         * @return ItemRef 
         */
        static ItemRef Create(const std::string& name);
        static ItemRef Create(ItemPrototype* prototype);

        ItemPrototype* getPrototype() {return prototype;}
        int getQuantity(){ return quantity; }
        void setQuantity(int number) {quantity = number;}
};

/**
 * @brief Registers all existing item prototypes
 * 
 */
class ItemRegistry{
    private:
        std::unordered_map<std::string, std::unique_ptr<ItemPrototype>> prototypes = {};

        ItemRegistry(){}
    public:
        ItemPrototype* addPrototype(ItemPrototype prototype);
        ItemPrototype* getPrototype(const std::string& name);

        /**
         * @brief Create an item prototype for a block prototype
         * 
         * @param prototype 
         * @return ItemPrototype* 
         */
        ItemPrototype* createPrototypeForBlock(const BlockRegistry::BlockPrototype* prototype);

        /**
         * @brief Check if a prototype exists
         * 
         * @param name 
         * @return true 
         * @return false 
         */
        bool prototypeExists(const std::string& name);

        static ItemRegistry& get();
        static bool LoadFromXML(const std::string& path);

        const std::unordered_map<std::string, std::unique_ptr<ItemPrototype>>& GetPrototypes() {return prototypes; }
};

/**
 * @brief An item slot that holds an item and provides convinient functions associated, has no display
 * 
 */
class LogicalItemSlot{
    private:
        ItemRef item;
        bool block_placing = false;

    public:
        LogicalItemSlot(){}
        bool takeItemFrom(LogicalItemSlot& source, int quantity = -1);
        bool moveItemTo(LogicalItemSlot& destination, int quantity = -1);
        bool swap(LogicalItemSlot& slot);

        /**
         * @brief Adds an item if possible
         * 
         * @param item 
         * @return true added
         * @return false not added
         */
        bool addItem(ItemRef item){
            LogicalItemSlot slot;
            slot.setItem(item);

            return takeItemFrom(slot);
        }

        /**
         * @brief Destory the held item
         * 
         */
        void clear();
        bool hasItem() {return getItem() != nullptr; }

        /**
         * @brief  Decreses the item amount and returns the count removed
         * 
         * @param number 
         * @return int 
         */
        int decreaseQuantity(int number);

        /**
         * @brief Takes a portion of the items in the slot and returns an id of an item that has exactly that quantity or less if there was not enough.
         * 
         * @param quantity quantity taken
         * @return ItemRef 
         */
        ItemRef getPortion(int quantity = -1);

        ItemRef getItem() {return item;}
        void setItem(ItemRef item) {this->item = item;}
};

/**
 * @brief A larger 2D set of logical item slots
 * 
 */
class LogicalItemInventory{
    private:
        std::vector<LogicalItemSlot> slots{};

        int slots_horizontaly = 0;
        int slots_verticaly = 0;

        friend class Serializer;
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
};

/**
 * @brief Special entity data for a dropped item
 * 
 */
class DroppedItem: public EntityData{
    private:
        ItemRef item;

    public:
        DroppedItem(ItemRef item): item(item){type = DROPPED_ITEM;}

        void serialize(ByteArray& array) override;
        void update(GameState* state) override;
        ItemRef getItem(){return item;}

        void setup(Entity* entity) override;
        /**
         * @brief Create a dropped item entity that holds the set item
         * 
         * @param position 
         * @param item 
         * @return Entity 
         */
        static Entity create(glm::vec3 position, ItemRef item){
            if(!item) return {};

            return Entity(position, glm::vec3{0.4,0.4,0.4}, std::make_shared<DroppedItem>(item));
        }
        static std::shared_ptr<EntityData> deserializeData(ByteArray& array);
};
