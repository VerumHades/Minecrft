#pragma once

#include <optional>
#include <rendering/texture.hpp>
#include <memory>
#include <stb_image.h>
#include <ui/manager.hpp>
#include <game/items/item.hpp>
#include <vector>

class ItemTextureAtlas{
    private:
        const size_t single_texture_size = 32;
        const size_t atlas_size = 1024;
        const size_t layer_count = 1;

        std::shared_ptr<GLTextureArray> texture_array;
        size_t textures_stored_total;
        
        struct StoredTexture{
            glm::vec2 uv_min;
            glm::vec2 uv_max;
            int index;
        };

        std::unordered_map<ItemPrototype*, StoredTexture> stored_textures;

    public:
        ItemTextureAtlas() {
            texture_array = std::make_shared<GLTextureArray>();
            texture_array->setup(atlas_size, atlas_size, layer_count);
        }
        /*
            Loads the texture if not loaded, returs its uv coordinates and index
        */
        StoredTexture* getPrototypeTexture(ItemPrototype* prototype);
        std::shared_ptr<GLTextureArray>& getTextureArray() {return texture_array;};
};

class LogicalItemSlot{
    private:
        std::optional<Item> item_option;

    public:
        bool takeItemFrom(LogicalItemSlot& source, int quantity = -1);
        bool moveItemTo(LogicalItemSlot& destination, int quantity = -1);

        bool addItem(Item item){
            LogicalItemSlot slot;
            slot.getItem().emplace(item);

            return takeItemFrom(slot);
        }

        void clear();
        bool hasItem() {return item_option.has_value();}

        std::optional<Item>& getItem() {return item_option;}
};

class ItemSlot: public UIFrame{
    private:
        const int slot_size = 64;
        const int slot_padding = 4;
        const int quantity_number_font_size = 12;

        LogicalItemSlot item_slot;
        ItemTextureAtlas& textureAtlas;

    public:
        ItemSlot(ItemTextureAtlas& textureAtlas, UIManager& manager): UIFrame(manager), textureAtlas(textureAtlas){
            dedicated_texture_array = textureAtlas.getTextureArray();
            setSize(slot_size,slot_size);
        }

        LogicalItemSlot& getSlot() {return item_slot;}
        void getRenderingInformation(UIRenderBatch& batch) override;
        
};

class ItemInventory: public UIFrame{
    private:
        const int slot_size = 64;
        const int slot_padding = 4;
        const int quantity_number_font_size = 12;

        std::vector<LogicalItemSlot> items;
        ItemTextureAtlas& textureAtlas;
        int slots_horizontaly;
        int slots_verticaly;
        std::shared_ptr<ItemSlot> held_item_ptr;

        size_t getIndex(int x, int y){
            size_t index = x + y * slots_horizontaly;
            if(index >= items.size()) return 0;
            return index;
        }

    public:
        ItemInventory(ItemTextureAtlas& textureAtlas, UIManager& manager, int slots_horizontaly, int slots_verticaly, std::shared_ptr<ItemSlot> held_item_ptr);

        void setItem(int x, int y, Item item) {
            items[getIndex(x,y)].addItem(item);
        }
        void removeItem(int x, int y){
            items[getIndex(x,y)].clear();
        }

        bool addItem(Item item);

        void getRenderingInformation(UIRenderBatch& batch) override;
};