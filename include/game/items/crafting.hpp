#pragma once

#include <game/blocks.hpp>
#include <game/items/item.hpp>
#include <game/items/item_renderer.hpp>
#include <ui/core.hpp>
#include <game/game_state.hpp>

class CraftingMetadata: public BlockMetadata{
    public:
        LogicalItemInventory crafting_field{3,3};
        LogicalItemInventory result_slot{1,1};
};

class CraftingInterface: public BlockInterface{
    private:
        std::shared_ptr<UILayer> ui_layer;

        std::shared_ptr<InventoryDisplay> crafting_display;
        std::shared_ptr<InventoryDisplay> result_display;
        std::shared_ptr<InventoryDisplay> player_inventory;

    public:
        CraftingInterface(const std::string& name, ItemTextureAtlas& textureAtlas, std::shared_ptr<ItemSlot> held_item_ptr);

        void open(std::shared_ptr<BlockMetadata> metadata, GameState* game_state) override;
        const std::string& getName() override {return ui_layer->name;}

        std::shared_ptr<UILayer> getLayer(){return ui_layer;}
        std::shared_ptr<BlockMetadata> createMetadata() override;
};