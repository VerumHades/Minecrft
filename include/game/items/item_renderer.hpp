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

/**
 * @brief A 2d texture atlas for item textures
 * 
 */
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
        /**
         * @brief Loads the texture for a given prototype if not loaded, returs its uv coordinates and index
         * 
         * @param prototype 
         * @return TextureSet* 
         */
        TextureSet* getPrototypeTextureSet(ItemPrototype* prototype);
        
        /**
         * @brief Helper function that renders an item into a slot
         * 
         * @param batch rendering batch for the ui component
         * @param prototype 
         * @param slot_transform size and position of the slot to draw in
         */
        void RenderItemIntoSlot(UIRenderBatch& batch, ItemPrototype* prototype, UITransform slot_transform);

        std::shared_ptr<GLTextureArray>& getTextureArray() {return texture_array;};
};

/**
 * @brief An interactable ui slot component
 * 
 */
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

/**
 * @brief A ui component that allows interaction with a logical inventory
 * 
 */
class InventoryDisplay: public UIFrame{
    protected:
        const int slot_size = 64;
        const int slot_padding = 4;
        const int quantity_number_font_size = 12;

        bool lock_placing = false;
        
    protected:
        ItemTextureAtlas& textureAtlas;
        LogicalItemInventory* inventory = nullptr;
        std::shared_ptr<ItemSlot> held_item_ptr = nullptr;
    public:
        InventoryDisplay(ItemTextureAtlas& textureAtlas, std::shared_ptr<ItemSlot> held_item_ptr);

        std::function<void()> onStateUpdate;
        std::function<void(int amount)> onItemTaken;

        void setInventory(LogicalItemInventory* new_inventory);
        LogicalItemInventory* getInventory() {return inventory;};

        void getRenderingInformation(UIRenderBatch& batch) override;
        void setLockPlacing(bool value) {lock_placing = value;}

};