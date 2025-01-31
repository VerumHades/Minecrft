#pragma once

#include <optional>
#include <memory>
#include <stb_image.h>
#include <ui/core.hpp>
#include <game/items/item.hpp>
#include <vector>

#include <vec_hash.hpp>
#include <rendering/opengl/texture.hpp>
#include <rendering/image_processing.hpp>

class ItemTextureAtlas{
    private:
        const size_t single_texture_size = 32;
        const size_t atlas_size = 1024;
        const size_t layer_count = 1;

        std::shared_ptr<GLTextureArray> texture_array{};
        size_t textures_stored_total = 0;
        
        struct StoredTexture{
            UIRegion uvs = {{0,0},{1,1}};
            int index = 0;
        };

        struct TextureSet{
            std::array<StoredTexture,3> textures{};
        };

        std::unordered_map<ItemPrototype*, TextureSet> stored_textures{};

        StoredTexture storeImage(const Image& image);

    public:
        ItemTextureAtlas() {
            texture_array = std::make_shared<GLTextureArray>();
            texture_array->setup(atlas_size, atlas_size, layer_count);
        }
        /*
            Loads the texture if not loaded, returs its uv coordinates and index
        */
        TextureSet* getPrototypeTextureSet(ItemPrototype* prototype);

        void RenderItemIntoSlot(UIRenderBatch& batch, ItemPrototype* prototype, UITransform slot_transform);

        std::shared_ptr<GLTextureArray>& getTextureArray() {return texture_array;};
};

class LogicalItemSlot{
    private:
        std::optional<Item> item_option{};

    public:
        LogicalItemSlot(){}
        bool takeItemFrom(LogicalItemSlot& source, int quantity = -1);
        bool moveItemTo(LogicalItemSlot& destination, int quantity = -1);

        bool addItem(Item item){
            LogicalItemSlot slot;
            slot.getItem().emplace(item);

            return takeItemFrom(slot);
        }

        void clear();
        bool hasItem() {return item_option.has_value();}
        void decreaseQuantity(int number);

        std::optional<Item>& getItem() {return item_option;}
};

class ItemSlot: public UIFrame{
    private:
        const int slot_size = 64;
        const int slot_padding = 4;
        const int quantity_number_font_size = 12;

        LogicalItemSlot item_slot{};
        ItemTextureAtlas& textureAtlas;

    public:
        ItemSlot(ItemTextureAtlas& textureAtlas): textureAtlas(textureAtlas){
            dedicated_texture_array = textureAtlas.getTextureArray();
            setSize(TValue::Pixels(slot_size),TValue::Pixels(slot_size));
        }

        LogicalItemSlot& getSlot() {return item_slot;}
        void getRenderingInformation(UIRenderBatch& batch) override;
        
};

class ItemInventory: public UIFrame{
    protected:
        const int slot_size = 64;
        const int slot_padding = 4;
        const int quantity_number_font_size = 12;
        
    private:
        std::unordered_map<glm::ivec3, LogicalItemSlot, IVec3Hash, IVec3Equal> items{};
        ItemTextureAtlas& textureAtlas;
        int slots_horizontaly = 0;
        int slots_verticaly = 0;
        std::shared_ptr<ItemSlot> held_item_ptr = nullptr;
    protected:
        LogicalItemSlot& getItemSlot(int x, int y){
            return items[{x,y,0}];
        }
    public:
        ItemInventory(ItemTextureAtlas& textureAtlas, int slots_horizontaly, int slots_verticaly, std::shared_ptr<ItemSlot> held_item_ptr);

        bool setItem(int x, int y, Item item) {
            auto position = glm::ivec3{x,y,0};

            if(!items.contains(position))
                items[position] = {};

            return items.at(position).addItem(item);
        }
        void removeItem(int x, int y){
            items[{x,y,0}].clear();
        }

        bool addItem(Item item);

        void getRenderingInformation(UIRenderBatch& batch) override;
};