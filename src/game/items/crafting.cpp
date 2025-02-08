#include <game/items/crafting.hpp>

CraftingInterface::CraftingInterface(const std::string& name, ItemTextureAtlas& textureAtlas, std::shared_ptr<ItemSlot> held_item_ptr){
    ui_layer = std::make_shared<UILayer>();
    ui_layer->name = name;

    crafting_display = std::make_shared<InventoryDisplay>(textureAtlas, held_item_ptr);
    result_display   = std::make_shared<InventoryDisplay>(textureAtlas, held_item_ptr);
    player_inventory = std::make_shared<InventoryDisplay>(textureAtlas, held_item_ptr);

    auto frame = std::make_shared<UIFrame>();
    frame->setPosition(TValue::Pixels(0),TValue::Pixels(0));
    frame->setSize(TValue(PERCENT,100),TValue(PERCENT,100));
    frame->setLayout(std::make_shared<UIFlexLayout>());

    auto upper_frame = std::make_shared<UIFrame>();
    upper_frame->setLayout(std::make_shared<UIFlexLayout>(UIFlexLayout::HORIZONTAL));
    upper_frame->setSize(TValue(FIT_CONTENT,0),TValue(PERCENT,50));

    upper_frame->appendChild(crafting_display);
    upper_frame->appendChild(result_display);

    auto lower_frame = std::make_shared<UIFrame>();
    lower_frame->setLayout(std::make_shared<UIFlexLayout>(UIFlexLayout::HORIZONTAL));
    lower_frame->setSize(TValue(FIT_CONTENT,0),TValue(PERCENT,50));
    
    lower_frame->appendChild(player_inventory);

    frame->appendChild(upper_frame);
    frame->appendChild(lower_frame);

    ui_layer->addElement(frame);
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
}