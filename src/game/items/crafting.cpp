#include <game/items/crafting.hpp>


CraftingDisplay::CraftingDisplay(ItemTextureAtlas& textureAtlas, std::shared_ptr<ItemSlot> held_item_ptr){
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
        ItemRef item = Item::Create(current_recipe->result_prototype_name);
        slot->setItem(item);
        if(slot->hasItem()) slot->getItem()->setQuantity(current_recipe->result_amount);

        result_display->update();
    };
    
    result_display  = std::make_shared<InventoryDisplay>(textureAtlas, held_item_ptr);
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

        if(current_recipe->shapeless){
            for(auto& requirement: current_recipe->required_items){
                for(int y = 0;y < inventory->getHeight();y++)
                for(int x = 0;x < inventory->getWidth();x++)
                {
                    auto* slot = inventory->getSlot(x,y);
    
                    bool hasItem = slot && slot->hasItem() && slot->getItem()->getPrototype();
                    if(!hasItem) continue;
                    if(slot->getItem()->getPrototype()->getName() != requirement.name) continue;

                    slot->decreaseQuantity(requirement.amount);
                }
            }
        }
        else{
            for(auto& requirement: current_recipe->required_items){
                auto* slot = inventory->getSlot(requirement.slotX,requirement.slotY);
                
                slot->decreaseQuantity(requirement.amount);
            }
        }
        current_recipe = CraftingRecipeRegistry::get().getCraftingFor(*crafting_display->getInventory());
        crafting_display->update();
    };

    setLayout(std::make_shared<UIFlexLayout>(UIFlexLayout::HORIZONTAL));
    setSize(TValue(FIT_CONTENT,0),TValue(PIXELS,200));

    appendChild(crafting_display);
    appendChild(result_display);
}

void CraftingDisplay::setInventories(LogicalItemInventory* crafting_inventory, LogicalItemInventory* result_inventory){
    crafting_display->setInventory(crafting_inventory);
    result_display->setInventory(result_inventory);
}

CraftingInterface::CraftingInterface(const std::shared_ptr<UILayer>& layer, ItemTextureAtlas& textureAtlas, std::shared_ptr<ItemSlot> held_item_ptr):
 ui_layer(layer){

    crafting_display = std::make_shared<CraftingDisplay>(textureAtlas, held_item_ptr);
    player_inventory = std::make_shared<InventoryDisplay>(textureAtlas, held_item_ptr);
    
    player_hotbar = std::make_shared<InventoryDisplay>(textureAtlas, held_item_ptr);
    player_hotbar->setPosition(
        TValue::Center(),
        TValue::Bottom(20_px)
    );

    auto frame = std::make_shared<UIFrame>();
    frame->setPosition(TValue::Center(), TValue::Center());
    frame->setSize(TValue::Pixels(600), TValue::Pixels(400));
    frame->setLayout(std::make_shared<UIFlexLayout>());

    frame->appendChild(crafting_display);
    player_inventory->setAttribute(&UIFrame::Style::margin, {TValue::Pixels(40),TValue::Pixels(0),TValue::Pixels(0),TValue::Pixels(0)});
    frame->appendChild(player_inventory);
    
    ui_layer->addElement(frame);
    ui_layer->addElement(player_hotbar);

    ui_layer->addElement(held_item_ptr);
}

void CraftingInterface::open(std::shared_ptr<BlockMetadata> metadata, GameState* game_state){
    if(metadata){
        if(auto result = std::dynamic_pointer_cast<CraftingMetadata>(metadata)){
            crafting_display->setInventories(&result->crafting_field, &result->result_slot);
        }
    }
    player_inventory->setInventory(&game_state->getPlayerInventory());
    player_hotbar->setInventory(&game_state->getPlayerHotbar());
}

std::shared_ptr<BlockMetadata> CraftingInterface::createMetadata(){
    return std::make_shared<CraftingMetadata>();
}

std::string CraftingRecipe::GenerateTagMember(const std::string& item_name, int slotX, int slotY, bool shapeless){
    std::string out = "";
    out += "_" + item_name;
    if(!shapeless) out += "(" + std::to_string(slotX) + "x" + std::to_string(slotY) +  ")";
    return out;
}
CraftingRecipe::CraftingRecipe(const std::vector<CraftingRecipe::ItemRequirement>& required_items, const std::string& result_prototype_name, int result_amount, bool shapeless):
 required_items(required_items), result_prototype_name(result_prototype_name), result_amount(result_amount), shapeless(shapeless) {
    GenerateTag();
}

void CraftingRecipe::GenerateTag(){
    tag = "";
    for(auto& requirement: required_items)
        tag += GenerateTagMember(requirement.name, requirement.slotX, requirement.slotY, shapeless);
}

void CraftingRecipeRegistry::addRecipe(const CraftingRecipe& recipe){
    recipes.emplace(recipe.tag,recipe);
}

CraftingRecipe* CraftingRecipeRegistry::getCraftingFor(LogicalItemInventory& inventory){
    std::string tag = "";
    std::string shapeless_tag = "";


    for(int y = 0;y < inventory.getHeight();y++)
    for(int x = 0;x < inventory.getWidth();x++)
    {
        auto* slot = inventory.getSlot(x,y);
        if(slot && slot->getItem() && slot->getItem()->getPrototype()){
            auto* prototype = slot->getItem()->getPrototype();

            tag += CraftingRecipe::GenerateTagMember(prototype->getName(), x, y, false);
            shapeless_tag += CraftingRecipe::GenerateTagMember(prototype->getName(), x, y, true);
        }
    }

    if(recipes.contains(tag)){
        auto& recipe = recipes.at(tag);

        for(auto& requirement: recipe.required_items){
            auto* slot = inventory.getSlot(requirement.slotX,requirement.slotY);

            if(slot->getItem()->getQuantity() < requirement.amount) return nullptr;
        }

        return &recipe;
    }    
    if(recipes.contains(shapeless_tag)){
        auto& recipe = recipes.at(shapeless_tag);

        for(auto& requirement: recipe.required_items){
            bool found = false;
            
            for(int y = 0;y < inventory.getHeight();y++)
            for(int x = 0;x < inventory.getWidth();x++)
            {
                auto* slot = inventory.getSlot(x,y);

                bool hasItem = slot && slot->hasItem() && slot->getItem()->getPrototype();
                if(!hasItem) continue;

                if(
                    slot->getItem()->getQuantity() < requirement.amount ||
                    slot->getItem()->getPrototype()->getName() != requirement.name
                ) return nullptr;

                found = true;
            }

            if(!found) return nullptr;
        }

        return &recipe;
    }

    return nullptr;
}

bool CraftingRecipeRegistry::LoadRecipesFromXML(const std::string& path){
    XMLDocument doc;

    if (doc.LoadFile(path.c_str()) != XML_SUCCESS) {
        std::cerr << "Error loading block XML file." << std::endl;
        return false;
    }

    XMLElement* root = doc.FirstChildElement("recipes");
    if (!root) {
        std::cerr << "No root blocks element found." << std::endl;
        return false;
    }

    xml_for_each_child_as(root, recipe_definition)
    {
        CraftingRecipe recipe{};
        
        XMLExtras::Load<CraftingRecipe>(
            recipe,
            recipe_definition,
            {
                {"result", offsetof(CraftingRecipe, result_prototype_name), XType::STRING},
                {"shapeless", offsetof(CraftingRecipe, shapeless), XType::BOOL},
                {"result_amount", offsetof(CraftingRecipe, result_amount), XType::INT},
            },
            [](XMLElement* element, CraftingRecipe& recipe){
                std::string name = element->Name();

                if(name == "items"){
                    xml_for_each_child_as(element, recipe_item){
                        recipe.required_items.push_back( 
                            XMLExtras::Load<CraftingRecipe::ItemRequirement>(
                                recipe_item, {
                                    {"slotX", offsetof(CraftingRecipe::ItemRequirement, slotX), XType::INT},
                                    {"slotY", offsetof(CraftingRecipe::ItemRequirement, slotY), XType::INT},
                                    {"amount", offsetof(CraftingRecipe::ItemRequirement, amount), XType::INT},
                                    {"name", offsetof(CraftingRecipe::ItemRequirement, name), XType::STRING}
                                }
                            )
                        );
                    }
                }
                else{
                    std::cout << "Unrecognized attribute for recipe in xml: " << name << std::endl;
                }
            }
        );

        recipe.GenerateTag();

        CraftingRecipeRegistry::get().addRecipe(recipe);
    }

    return true;
}

void CraftingMetadata::serialize(ByteArray& to){
    crafting_field.serialize(to);
    result_slot.serialize(to);
}
std::shared_ptr<BlockMetadata> CraftingInterface::deserialize(ByteArray& from){
    auto output = std::make_shared<CraftingMetadata>();

    output->crafting_field = LogicalItemInventory::deserialize(from);
    output->result_slot = LogicalItemInventory::deserialize(from);

    return output;
}