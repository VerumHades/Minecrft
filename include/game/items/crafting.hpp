#pragma once

#include <game/blocks.hpp>
#include <game/items/item.hpp>
#include <game/items/item_renderer.hpp>
#include <ui/core.hpp>
#include <game/game_state.hpp>
#include <unordered_map>

class CraftingRecipeRegistry;
class CraftingInterface;

class CraftingRecipe{
    public:
        struct RecipeItemRequirement{
            bool required;
            int amount;
            std::string name;

            RecipeItemRequirement(): required(0), amount(0), name(""){}
            RecipeItemRequirement(const std::string& name, int amount): required(true), amount(amount), name(name){}
        };
    private:
        std::string tag = "";

        std::array<RecipeItemRequirement, 9> required_items;
        std::string result_prototype_name;

        friend class CraftingRecipeRegistry;
        friend class CraftingInterface;
    public:
        CraftingRecipe(const std::array<RecipeItemRequirement, 9>& required_items, const std::string& result_prototype_name);
};

class CraftingRecipeRegistry{
    private:
        std::unordered_map<std::string, CraftingRecipe> recipes{};

        CraftingRecipeRegistry() {}
    public:
        void addRecipe(const CraftingRecipe& recipe);
        CraftingRecipe* getCraftingFor(LogicalItemInventory& inventory);

        static CraftingRecipeRegistry& get(){
            static CraftingRecipeRegistry registry{};
            return registry;
        }

};

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
        std::shared_ptr<InventoryDisplay> player_hotbar;

        CraftingRecipe* current_recipe = nullptr;

    public:
        CraftingInterface(const std::string& name, ItemTextureAtlas& textureAtlas, std::shared_ptr<ItemSlot> held_item_ptr);

        void open(std::shared_ptr<BlockMetadata> metadata, GameState* game_state) override;
        const std::string& getName() override {return ui_layer->name;}

        std::shared_ptr<UILayer> getLayer(){return ui_layer;}
        std::shared_ptr<BlockMetadata> createMetadata() override;
};