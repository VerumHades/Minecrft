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

        static std::string GenerateTagMember(const std::string& item_name, int slotX, int slotY, bool shapeless);
        void GenerateTag();

        CraftingRecipe() {}
    public:
        CraftingRecipe(const std::vector<ItemRequirement>& required_items, const std::string& result_prototype_name, int result_amount, bool shapeless = false);
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

        static bool LoadRecipesFromXML(const std::string& path);
};

class CraftingMetadata: public BlockMetadata{
    public:
        LogicalItemInventory crafting_field{3,3};
        LogicalItemInventory result_slot{1,1};

        void serialize(ByteArray& to) override;
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
        CraftingInterface(const std::shared_ptr<UILayer>& layer, ItemTextureAtlas& textureAtlas, std::shared_ptr<ItemSlot> held_item_ptr);

        void open(std::shared_ptr<BlockMetadata> metadata, GameState* game_state) override;
        const std::string& getName() override {return ui_layer->name;}

        std::shared_ptr<UILayer> getLayer(){return ui_layer;}
        std::shared_ptr<BlockMetadata> createMetadata() override;
        std::shared_ptr<BlockMetadata> deserialize(ByteArray& from) override;
};