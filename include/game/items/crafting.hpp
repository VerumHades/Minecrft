#pragma once

#include <game/blocks.hpp>
#include <game/items/item.hpp>
#include <game/items/item_renderer.hpp>
#include <ui/core.hpp>
#include <game/game_state.hpp>
#include <unordered_map>
#include <limits>
#include <algorithm>

class CraftingRecipeRegistry;
class CraftingInterface;
class CraftingDisplay;

/**
 * @brief A recipe for crafting
 * 
 */
class CraftingRecipe{
    public:
        struct ItemRequirement{
            int slotX = 0;
            int slotY = 0;

            int amount = 0;
            std::string name = "";

            ItemRequirement(const std::string& name, int amount, int slotX, int slotY): slotX(slotX), slotY(slotY), amount(amount), name(name){}
            ItemRequirement(const std::string& name, int amount): slotX(0), slotY(0), amount(amount), name(name){}
            ItemRequirement() {}
        };
    private:
        std::string tag = "";

        std::vector<ItemRequirement> required_items;
        std::string result_prototype_name;
        int result_amount = 1;
        bool shapeless = false;

        friend class CraftingRecipeRegistry;
        friend class CraftingInterface;
        friend class CraftingDisplay;

        /**
         * @brief Generates a tag member for a complete tag
         * 
         * @param item_name 
         * @param slotX 
         * @param slotY 
         * @param shapeless 
         * @return std::string 
         */
        static std::string GenerateTagMember(const std::string& item_name, int slotX, int slotY, bool shapeless);
        
        /**
         * @brief Generates a unique string tag for the recipe
         * 
         */
        void GenerateTag();

        CraftingRecipe() {}
    public:
        CraftingRecipe(const std::vector<ItemRequirement>& required_items, const std::string& result_prototype_name, int result_amount, bool shapeless = false);
};

/**
 * @brief Registers all existing recipes
 * 
 */
class CraftingRecipeRegistry{
    private:
        std::unordered_map<std::string, CraftingRecipe> recipes{};

        CraftingRecipeRegistry() {}
    public:
        void addRecipe(const CraftingRecipe& recipe);

        /**
         * @brief Returns the relevat recipe found withing an inventory
         * 
         * @param inventory 
         * @return std::tuple<CraftingRecipe*, int, int> the recipe, and its offset in the inventory (every lookup snaps the recipe to the top left corner)
         */
        std::tuple<CraftingRecipe*, int, int> getCraftingFor(LogicalItemInventory& inventory);

        static CraftingRecipeRegistry& get(){
            static CraftingRecipeRegistry registry{};
            return registry;
        }

        static bool LoadRecipesFromXML(const std::string& path);
};

/**
 * @brief Data for a crafting block
 * 
 */
class CraftingMetadata: public BlockMetadata{
    public:
        LogicalItemInventory crafting_field{3,3};
        LogicalItemInventory result_slot{1,1};

        void serialize(ByteArray& to) override;
};

/**
 * @brief Display ui for a crafting block
 * 
 */
class CraftingDisplay: public UIFrame{
    private:
        std::shared_ptr<InventoryDisplay> crafting_display;
        std::shared_ptr<InventoryDisplay> result_display;

        CraftingRecipe* current_recipe = nullptr;
        int recipe_offset_x = 0;
        int recipe_offset_y = 0;
    public:
        CraftingDisplay(ItemTextureAtlas& textureAtlas, std::shared_ptr<ItemSlot> held_item_ptr);
        void setInventories(LogicalItemInventory* crafting_inventory, LogicalItemInventory* result_inventory);
        
};

/**
 * @brief Interface for a crafting block, handles all the relevant actions associated
 * 
 */
class CraftingInterface: public BlockInterface{
    private:
        std::shared_ptr<UILayer> ui_layer;

        std::shared_ptr<CraftingDisplay> crafting_display;
        std::shared_ptr<InventoryDisplay> player_inventory;
        std::shared_ptr<InventoryDisplay> player_hotbar;

        CraftingRecipe* current_recipe = nullptr;

    public:
        CraftingInterface(const std::shared_ptr<UILayer>& layer, ItemTextureAtlas& textureAtlas, std::shared_ptr<ItemSlot> held_item_ptr);

        void open(std::shared_ptr<BlockMetadata> metadata, GameState* game_state) override;
        const std::string& getName() override {return ui_layer->name;}

        std::shared_ptr<UILayer> getLayer(){return ui_layer;}
        std::shared_ptr<BlockMetadata> createMetadata() override;
        std::shared_ptr<BlockMetadata> deserialize(ByteArray& from) override;
};