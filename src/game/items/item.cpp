#include <game/items/item.hpp>

ItemRegistry& ItemRegistry::get(){
    static ItemRegistry registry{};
    return registry;
}

ItemPrototype* ItemRegistry::addPrototype(ItemPrototype prototype){
    prototypes[prototype.name] = std::make_unique<ItemPrototype>(prototype);
    return prototypes.at(prototype.name).get();
}

ItemPrototype* ItemRegistry::getPrototype(const std::string& name){
    if(!prototypes.contains(name)) return nullptr;
    return prototypes.at(name).get();
}

ItemPrototype::ItemPrototype(std::string name, const BlockRegistry::BlockPrototype* prototype): name(name){
    if(prototype->render_type != BlockRegistry::BILLBOARD) display_type = BLOCK;
    is_block = true;

    if(prototype->single_texture){
        auto texture_path = prototype->texture_paths[0];
        texture_paths = {texture_path, texture_path, texture_path};
    }
    else if(prototype->render_type == BlockRegistry::FULL_BLOCK){
        texture_paths = {
            prototype->texture_paths[0],
            prototype->texture_paths[2],
            prototype->texture_paths[4]
        };
    }

    if(prototype->render_type == BlockRegistry::BILLBOARD) model = std::make_shared<SpriteModel>(prototype->texture_paths[0]);
    else model = std::make_shared<BlockModel>(prototype);
    this->block_id = prototype->id;
}
ItemPrototype::ItemPrototype(std::string name, std::string texture_path): name(name){
    display_type = SIMPLE;
    texture_paths[0] = texture_path;
    model = std::make_shared<SpriteModel>(texture_path);
}

bool ItemRegistry::LoadFromXML(const std::string& path){
    XMLDocument doc;

    if (doc.LoadFile(path.c_str()) != XML_SUCCESS) {
        LogError("Error loading items XML file.");
        return false;
    }

    XMLElement* root = doc.FirstChildElement("items");
    if (!root) {
        LogError("No root items element found.");
        return false;
    }

    xml_for_each_child_as(root, item_definition)
    {
        std::string name = "undefined_item";
        std::string texture_path = "texture_path";
        std::vector<ItemPrototype::ToolEffectiveness> effectiveness{};

        xml_for_each_child_as(item_definition, item_attribute){
            std::string attr_name = item_attribute->Name();

            if(attr_name == "name"){
                const char* text = item_attribute->GetText();
                if(!text) continue;
                name = text;
            }
            else if(attr_name == "texture"){
                const char* text = item_attribute->GetText();
                if(!text) continue;
                texture_path  = (fs::path("resources/textures/items") / text).string();
            }
            else if(attr_name == "effective_againist_materials"){
                xml_for_each_child_as(item_attribute, material_effectiveness){
                    effectiveness.push_back(
                        XMLExtras::Load<ItemPrototype::ToolEffectiveness>(material_effectiveness, {
                            {"name", offsetof(ItemPrototype::ToolEffectiveness, material_name), XType::STRING},
                            {"mining_power", offsetof(ItemPrototype::ToolEffectiveness, mining_power), XType::INT},
                        }
                    ));
                }
            }
        }
        auto prototype = ItemPrototype(name,texture_path);
        prototype.effective_againist_materials = effectiveness;
        ItemRegistry::get().addPrototype(prototype);
    }

    return true;
}

ItemPrototype* ItemRegistry::createPrototypeForBlock(const BlockRegistry::BlockPrototype* prototype){
    std::string prototype_name = "block_" + prototype->name;

    auto* existing_prototype = getPrototype(prototype_name);
    if(existing_prototype) return existing_prototype;

    return addPrototype({prototype_name, prototype});
}

bool ItemRegistry::prototypeExists(const std::string& name){
    return prototypes.contains(name);
}

ItemRef Item::Create(const std::string& name){
    auto* prototype = ItemRegistry::get().getPrototype(name);
    if(!prototype) return nullptr;
    return Create(prototype);
}

ItemRef Item::Create(ItemPrototype* prototype){
    return std::make_shared<Item>(prototype);
}


bool LogicalItemSlot::takeItemFrom(LogicalItemSlot& source, int quantity){
    bool source_has_item = source.hasItem();
    bool destination_has_item = hasItem();

    if(!source_has_item) return false;

    Item& source_item = *source.getItem();
    auto* source_item_prototype = source_item.getPrototype();
    if(!source_item_prototype) return false;

    int source_quantity = source_item.getQuantity();
    if(quantity == -1) quantity = source_quantity;

    if(destination_has_item){
        Item& destination_item = *getItem();
        auto* destination_item_prototype = destination_item.getPrototype();

        if(source_item_prototype != destination_item_prototype) return false;

        if(source_quantity <= quantity){
            source.clear();
            quantity = source_quantity;
        }
        else source_item.setQuantity(source_quantity - quantity);

        destination_item.setQuantity(destination_item.getQuantity() + quantity);
        return true;
    }

    item = source.getPortion(quantity);
    return true;
}
bool LogicalItemSlot::moveItemTo(LogicalItemSlot& destination, int quantity){
    return destination.takeItemFrom(*this, quantity);
}

ItemRef LogicalItemSlot::getPortion(int quantity){
    if(!hasItem()) return NO_ITEM;

    Item& source_item = *getItem();
    auto* source_item_prototype = source_item.getPrototype();
    if(!source_item_prototype) return NO_ITEM;

    int source_quantity = source_item.getQuantity();
    if(quantity == -1) quantity = source_quantity;

    ItemRef output = NO_ITEM;

    if(source_quantity <= quantity){
        output = item;
        item = NO_ITEM;
    }
    else{
        source_item.setQuantity(source_quantity - quantity);

        output = Item::Create(source_item.getPrototype());
        output->setQuantity(quantity);
    }
    return output;
}

int LogicalItemSlot::decreaseQuantity(int number){
    if(!hasItem()) return 0;
    auto& source_item = *getItem();
    int quantity = source_item.getQuantity();

    if(quantity <= number){
        clear();
        return quantity;
    }
    else{
        source_item.setQuantity(quantity - number);
        return number;
    }
}
void LogicalItemSlot::clear(){
    item = NO_ITEM;
}

bool LogicalItemSlot::swap(LogicalItemSlot& slot){
    ItemRef other = slot.item;
    slot.item = item;
    item = other;
    return true;
}

LogicalItemInventory::LogicalItemInventory(int slots_horizontaly, int slots_verticaly): slots(slots_horizontaly * slots_verticaly),
slots_horizontaly(slots_horizontaly), slots_verticaly(slots_verticaly){}

bool LogicalItemInventory::addItem(ItemRef item){
    if(!item) return false;

    auto* prototype = item->getPrototype();
    if(!prototype) return false;
    //std::cout << "Adding item " << prototype << std::endl;

    LogicalItemSlot* first_empty_slot = nullptr;

    for(int y = 0;y < slots_verticaly;y++)
    for(int x = 0;x < slots_horizontaly;x++){
        auto* slot = getSlot(x,y);

        if(!slot->hasItem()){
            if(first_empty_slot == nullptr) first_empty_slot = slot;
            continue;
        }

        if(slot->addItem(item)) return true;
    }

    if(first_empty_slot)
        return first_empty_slot->addItem(item);

    return false;
}

void DroppedItem::serialize(ByteArray& array){
    if(!item) return;
    Serializer::Serialize<Item>(*item, array);
}

void DroppedItem::setup(Entity* entity){
    if(!item) return;

    entity->setModel(item->getPrototype()->getModel());
    entity->onCollision = [](Entity* self, Entity* entity) {
        if(!entity->hasTag("player")) return;
        self->destroy = true;
    };
    entity->setSolid(false);
}

std::shared_ptr<EntityData> DroppedItem::deserializeData(ByteArray& array){
    ItemRef item = Item::Create(nullptr);
    Serializer::Deserialize<Item>(*item,array);
    return std::make_shared<DroppedItem>(item);
}

void DroppedItem::update(GameState* state){

}
