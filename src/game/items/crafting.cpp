#include <game/items/crafting.hpp>

CraftingInterface::CraftingInterface(const std::string& name, ItemTextureAtlas& textureAtlas, std::shared_ptr<ItemSlot> held_item_ptr){
    ui_layer = std::make_shared<UILayer>();
    ui_layer->name = name;

    crafting_display = std::make_shared<InventoryDisplay>(textureAtlas, held_item_ptr);
    crafting_display->onStateUpdate = [this](){
        result_display->update();
        if(!crafting_display->getInventory()) return;
        if(!result_display->getInventory()) return;

        current_recipe = CraftingRecipeRegistry::get().getCraftingFor(*crafting_display->getInventory());
        
        auto* result_inventory = result_display->getInventory();
        if(!current_recipe){
            result_inventory->getSlot(0,0)->clear();
            return;
        }

        auto* slot = result_inventory->getSlot(0,0);
        slot->clear();
        ItemID item = ItemRegistry::get().createItem(current_recipe->result_prototype_name);
        slot->setItem(item);
        if(slot->hasItem()) slot->getItem()->setQuantity(current_recipe->result_amount);

        result_display->update();
    };
    
    result_display   = std::make_shared<InventoryDisplay>(textureAtlas, held_item_ptr);
    result_display->setLockPlacing(true);
    result_display->setAttribute(&UIFrame::Style::margin, UISideSizesT(TValue(PIXELS, 0),TValue(PIXELS, 0),TValue(PIXELS, 0),TValue(PIXELS,40)));
    result_display->onStateUpdate = crafting_display->onStateUpdate;
    result_display->onItemTaken = [this](int amount){
        if(!crafting_display->getInventory()) return;
        if(!result_display->getInventory()) return;
        if(!current_recipe){
            std::cout << "Cheetah!" << std::endl;
            return;
        }

        auto inventory = crafting_display->getInventory();

        for(int y = 0;y < 3;y++)
        for(int x = 0;x < 3;x++)
        {
            auto* slot = inventory->getSlot(x,y);
            auto& requirement = current_recipe->required_items[x + y * 3];
            if(!slot || !slot->hasItem()) continue;
            slot->decreaseQuantity(requirement.amount);
        }
        current_recipe = CraftingRecipeRegistry::get().getCraftingFor(*crafting_display->getInventory());
        crafting_display->update();
    };

    player_inventory = std::make_shared<InventoryDisplay>(textureAtlas, held_item_ptr);
    
    player_hotbar = std::make_shared<InventoryDisplay>(textureAtlas, held_item_ptr);
    player_hotbar->setPosition(
        TValue::Center(),
        TValue::Bottom(20_px)
    );

    auto frame = std::make_shared<UIFrame>();
    frame->setPosition(TValue::Pixels(0),TValue::Pixels(0));
    frame->setSize(TValue(PERCENT,100),TValue(PERCENT,100));
    frame->setLayout(std::make_shared<UIFlexLayout>());

    auto upper_frame = std::make_shared<UIFrame>();
    upper_frame->setLayout(std::make_shared<UIFlexLayout>(UIFlexLayout::HORIZONTAL));
    upper_frame->setAttribute(&UIFrame::Style::margin, UISideSizesT(TValue(PIXELS, 200),TValue(PIXELS, 0),TValue(PIXELS, 0),TValue(PIXELS,0)));
    upper_frame->setSize(TValue(FIT_CONTENT,0),TValue(PIXELS,200));

    upper_frame->appendChild(crafting_display);
    upper_frame->appendChild(result_display);

    auto lower_frame = std::make_shared<UIFrame>();
    lower_frame->setLayout(std::make_shared<UIFlexLayout>(UIFlexLayout::HORIZONTAL));
    lower_frame->setSize(TValue(FIT_CONTENT,0),TValue(PERCENT,50));
    
    lower_frame->appendChild(player_inventory);

    frame->appendChild(upper_frame);
    frame->appendChild(lower_frame);
    
    ui_layer->addElement(frame);
    ui_layer->addElement(player_hotbar);

    ui_layer->addElement(held_item_ptr);
}

void CraftingInterface::open(std::shared_ptr<BlockMetadata> metadata, GameState* game_state){
    if(metadata){
        if(auto result = std::dynamic_pointer_cast<CraftingMetadata>(metadata)){
            crafting_display->setInventory(&result->crafting_field);
            result_display->setInventory(&result->result_slot);
        }
    }
    player_inventory->setInventory(&game_state->getPlayerInventory());
    player_hotbar->setInventory(&game_state->getPlayerHotbar());
}

std::shared_ptr<BlockMetadata> CraftingInterface::createMetadata(){
    return std::make_shared<CraftingMetadata>();
}

CraftingRecipe::CraftingRecipe(const std::array<CraftingRecipe::RecipeItemRequirement, 9>& required_items, const std::string& result_prototype_name, int result_amount):
 required_items(required_items), result_prototype_name(result_prototype_name), result_amount(result_amount){
    tag = "";
    for(auto& requirement: required_items){
        if(requirement.required){
            tag += "_" + requirement.name;
        }
        else tag += "_NONE";
    }
}

void CraftingRecipeRegistry::addRecipe(const CraftingRecipe& recipe){
    recipes.emplace(recipe.tag,recipe);
}

CraftingRecipe* CraftingRecipeRegistry::getCraftingFor(LogicalItemInventory& inventory){
    std::string tag = "";
    
    for(int y = 0;y < 3;y++)
    for(int x = 0;x < 3;x++)
    {
        auto* slot = inventory.getSlot(x,y);
        if(slot && slot->getItem() && slot->getItem()->getPrototype()){
            auto* prototype = slot->getItem()->getPrototype();

            tag += "_" + prototype->getName();
        }
        else tag += "_NONE";
    }

    if(recipes.contains(tag)){
        auto& recipe = recipes.at(tag);

        for(int y = 0;y < 3;y++)
        for(int x = 0;x < 3;x++)
        {
            auto* slot = inventory.getSlot(x,y);
            auto& requirement = recipe.required_items[x + y * 3];

            bool hasItem = slot && slot->hasItem() && slot->getItem()->getPrototype();
            bool shouldHaveItem = requirement.required;

            if(!hasItem && shouldHaveItem) return nullptr;
            if(hasItem && !shouldHaveItem) return nullptr;
            if(!hasItem && !shouldHaveItem) continue;

            if(slot->getItem()->getQuantity() < requirement.amount) return nullptr;
        }

        return &recipe;
    }  

    return nullptr;
}
