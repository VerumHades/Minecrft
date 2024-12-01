#pragma once

#include <optional>
#include <rendering/texture.hpp>
#include <memory>
#include <stb_image.h>
#include <ui/manager.hpp>
#include <game/items/item.hpp>

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

class ItemSlot: public UIFrame{
    private:
        std::optional<Item> item;
        ItemTextureAtlas& textureAtlas;

    public:
        ItemSlot(ItemTextureAtlas& textureAtlas, UIManager& manager): UIFrame(manager), textureAtlas(textureAtlas){
            dedicated_texture_array = textureAtlas.getTextureArray();
        }

        void setItem(Item item) {this->item.emplace(item);}
        void getRenderingInformation(RenderYeetFunction& yeet) override;
};